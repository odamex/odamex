// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//	plus functions to determine game mode (shareware, registered),
//	parse command line parameters, configure game parameters (turbo),
//	and call the startup functions.
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include <algorithm>

#include "win32inc.h"
#ifndef _WIN32
    #include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "c_dispatch.h"
#include "d_dehacked.h"
#include "d_main.h"
#include "g_game.h"
#include "g_horde.h"
#include "g_mapinfo.h"
#include "gi.h"
#include "gstrings.h"
#include "i_system.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "m_random.h"
#include "minilzo.h"
#include "odainfo.h"
#include "p_setup.h"
#include "r_local.h"
#include "r_sky.h"
#include "s_sound.h"
#include "sv_banlist.h"
#include "sv_main.h"
#include "v_video.h"
#include "w_wad.h"
#include "z_zone.h"
#include "g_musinfo.h"

#include "w_ident.h"

EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_nomonsters)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)

extern size_t got_heapsize;

void C_DoCommand(std::string_view cmd, uint32_t key = 0);

#ifdef UNIX
void daemon_init();
#endif

void D_DoomLoop (void);

extern bool gameisdead;

extern DThinker ThinkerCap;
extern dyncolormap_t NormalLight;

bool devparm;				// started game with -devparm
OLumpName startmap;
event_t events[MAXEVENTS];
gamestate_t wipegamestate = GS_DEMOSCREEN;	// can be -1 to force a wipe

std::string LOG_FILE;

//
// D_DoomLoop
//
void D_DoomLoop (void)
{
	while (1)
	{
		try
		{
			D_RunTics(SV_RunTics, SV_DisplayTics);
		}
		catch (CRecoverableError &error)
		{
			PrintFmt("ERROR: {}\n", error.GetMsg());
			PrintFmt("sleeping for 10 seconds before map reload...");

			// denis - drop clients
			SV_SendDisconnectSignal();

			// denis - sleep 10 seconds to conserve server resources (in case of recurring problem)
			I_Sleep(10 * 1000LL * 1000LL * 1000LL);

			// denis - reload with current settings
			G_ChangeMap ();

			// denis - todo - throw I_FatalError if this keeps happening
		}
	}
}

EXTERN_CVAR(co_boomphys)
EXTERN_CVAR(co_zdoomphys)
EXTERN_CVAR(co_mbfphys)
EXTERN_CVAR(co_zdoomammo)
EXTERN_CVAR(co_allowdropoff)

void G_ReadCOMPLVL()
{
	int lumpnum = W_CheckNumForName("COMPLVL");
	if (lumpnum != -1)
	{
		char* complvl = static_cast<char*>(W_CacheLumpNum(lumpnum, PU_STATIC));

		co_zdoomphys.Set(0.0f);
		co_zdoomammo.Set(0.0f);

		if (iequals("vanilla", complvl))
		{
			co_boomphys.Set(0.0f);
			co_mbfphys.Set(0.0f);
			co_allowdropoff.Set(0.0f);
		}
		else if (iequals("boom", complvl))
		{
			co_boomphys.Set(1.0f);
			co_mbfphys.Set(0.0f);
			co_allowdropoff.Set(1.0f);
		}
		else if (iequals("mbf", complvl))
		{
			co_boomphys.Set(1.0f);
			co_mbfphys.Set(1.0f);
			co_allowdropoff.Set(1.0f);
		}
		else if (iequals("mbf21", complvl))
		{
			co_boomphys.Set(1.0f);
			co_mbfphys.Set(1.0f);
			co_allowdropoff.Set(1.0f);
		}
		else
		{
			DPrintFmt("Unrecognized COMPLVL value: {}", complvl);
		}

		Z_Free(complvl);
	}
}


//
// D_Init
//
// Called to initialize subsystems when loading a new set of WAD resource
// files.
//
void D_Init()
{
	argb_t::setChannels(3, 2, 1, 0);
	// only print init messages during startup, not when changing WADs
	static bool first_time = true;

	SetLanguageIDs();

	M_ClearRandom();

	// [AM] Init rand() PRNG, needed for non-deterministic maplist shuffling.
	srand(time(NULL));

	// start the Zone memory manager
	Z_Init();
	if (first_time)
		PrintFmt("Z_Init: Using native allocator with OZone bookkeeping.\n");

	// Load palette and set up colormaps
	V_InitPalette("PLAYPAL");
	R_InitColormaps();

	// init the renderer
	if (first_time)
		PrintFmt(PRINT_HIGH, "R_Init: Init DOOM refresh daemon.\n");
	R_Init();

	G_ParseMapInfo();
	G_ParseMusInfo();
	S_ParseSndInfo();
	G_ParseHordeDefs();
	G_ReadCOMPLVL();

	if (first_time)
		PrintFmt(PRINT_HIGH, "P_Init: Init Playloop state.\n");
	P_Init();

	first_time = false;
}


//
// D_Shutdown
//
// Called to shutdown subsystems when unloading a set of WAD resource files.
// Should be called prior to D_Init when loading a new set of WADs.
//
void STACK_ARGS D_Shutdown()
{
	if (gamestate == GS_LEVEL)
		G_ExitLevel(0, 0);

	// [ML] 9/11/10: Reset custom wad level information from MAPINFO et al.
	getLevelInfos().clear();
	getClusterInfos().clear();

	// stop sound effects and music
	S_Stop();
	S_ClearSoundLumps();

	DThinker::DestroyAllThinkers();

	D_UndoDehPatch();

	// close all open WAD files
	W_Close();

	R_ShutdownColormaps();
	R_ClearSkyDefs();

	// reset the Zone memory manager
	Z_Close();

	// [AM] Level is now invalid due to torching zone memory.
	g_ValidLevel = false;

	// [AM] All of our dyncolormaps are freed, tidy up so we
	//      don't follow wild pointers.
	NormalLight.next = NULL;
}

//
// D_DoomMain
//
// [NightFang] - Cause I cant call ArgsSet from g_level.cpp
// [ML] 23/1/07 - Add Response file support back in
//
void D_DoomMain()
{
	size_t p;

	gamestate = GS_STARTUP;

	W_SetupFileIdentifiers();

	D_InitializeDoomObjectTables();

	M_FindResponseFile();		// [ML] 23/1/07 - Add Response file support back in

	if (lzo_init () != LZO_E_OK)	// [RH] Initialize the minilzo package.
		I_FatalError("Could not initialize LZO routines");

	C_ExecCmdLineParams(false, true);	// [Nes] test for +logfile command

	// Always log by default
	if (!LOG.is_open())
		C_DoCommand("logfile");


	OWantFiles newwadfiles, newpatchfiles;

	const char* iwad_filename_cstr = Args.CheckValue("-iwad");
	if (iwad_filename_cstr)
	{
		OWantFile file;
		OWantFile::make(file, iwad_filename_cstr, OFILE_WAD);
		newwadfiles.push_back(file);
	}

	D_AddWadCommandLineFiles(newwadfiles);
	D_AddDehCommandLineFiles(newpatchfiles);

	D_LoadResourceFiles(newwadfiles, newpatchfiles);

	// Ch0wW: Loading the config here fixes the "addmap" issue.
	M_LoadDefaults();					// load before initing other systems
	C_ExecCmdLineParams(true, false);	// [RH] do all +set commands on the command line

	PrintFmt(PRINT_HIGH, "I_Init: Init hardware.\n");
	I_Init();

	// [SL] Call init routines that need to be reinitialized every time WAD changes
	D_Init();
	atterm(D_Shutdown);

	PrintFmt(PRINT_HIGH, "SV_InitNetwork: Checking network game status.\n");
	SV_InitNetwork();

	// Base systems have been inited; enable cvar callbacks
	cvar_t::EnableCallbacks();

	// [RH] User-configurable startup strings. Because BOOM does.
	if (GStrings(STARTUP1)[0])	PrintFmt(PRINT_HIGH, "{}\n", GStrings(STARTUP1));
	if (GStrings(STARTUP2)[0])	PrintFmt(PRINT_HIGH, "{}\n", GStrings(STARTUP2));
	if (GStrings(STARTUP3)[0])	PrintFmt(PRINT_HIGH, "{}\n", GStrings(STARTUP3));
	if (GStrings(STARTUP4)[0])	PrintFmt(PRINT_HIGH, "{}\n", GStrings(STARTUP4));
	if (GStrings(STARTUP5)[0])	PrintFmt(PRINT_HIGH, "{}\n", GStrings(STARTUP5));

	// developer mode
	devparm = Args.CheckParm("-devparm");

	if (devparm)
		DPrintFmt("{}", GStrings(D_DEVSTR));		// D_DEVSTR

	// Nomonsters
	if (Args.CheckParm("-nomonsters"))
		sv_nomonsters = 1;

	// Respawn
	if (Args.CheckParm("-respawn"))
		sv_monstersrespawn = 1;

	// Fast
	if (Args.CheckParm("-fast"))
		sv_fastmonsters = 1;

	// get skill / episode / map from parms
	startmap = (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1";

	const char* val = Args.CheckValue("-skill");
	if (val)
		sv_skill.Set(val[0] - '0');

	p = Args.CheckParm("-timer");
	if (p && p < Args.NumArgs() - 1)
	{
		float time = atof(Args.GetArg(p + 1));
		PrintFmt(PRINT_HIGH, "Levels will end after {:g} minute{}.\n", time, time > 1 ? "s" : "");
		sv_timelimit.Set(time);
	}

	if (Args.CheckValue("-avg"))
	{
		PrintFmt(PRINT_HIGH, "Austin Virtual Gaming: Levels will end after 20 minutes\n");
		sv_timelimit.Set(20);
	}

	// [RH] Lock any cvars that should be locked now that we're
	// about to begin the game.
	cvar_t::EnableNoSet();

	// [RH] Now that all game subsystems have been initialized,
	// do all commands on the command line other than +set
	C_ExecCmdLineParams(false, false);

	// [AM] Initialize banlist
	SV_InitBanlist();

	PrintFmt(PRINT_HIGH, "========== Odamex Server Initialized ==========\n");

	#ifdef UNIX
	if (Args.CheckParm("-fork"))
		daemon_init();
	#endif

	p = Args.CheckParm("-warp");
	if (p && p < Args.NumArgs() - (1+(gameinfo.flags & GI_MAPxx ? 0 : 1)))
	{
		int ep, map;

		if (gameinfo.flags & GI_MAPxx)
		{
			ep = 1;
			map = atoi(Args.GetArg(p+1));
		}
		else
		{
			ep = Args.GetArg(p+1)[0]-'0';
			map = Args.GetArg(p+2)[0]-'0';
		}

		startmap = CalcMapName(ep, map);
	}

	// [RH] Hack to handle +map
	p = Args.CheckParm("+map");
	if (p && p < Args.NumArgs() - 1)
	{
		startmap = Args.GetArg(p + 1);
		((char*)Args.GetArg(p))[0] = '-';
	}

	level.mapname = startmap;

	G_ChangeMap();

	D_DoomLoop();	// never returns
}

VERSION_CONTROL (d_main_cpp, "$Id$")
