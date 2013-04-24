// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

#include "version.h"

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#endif

#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "errors.h"

#include "m_alloc.h"
#include "m_random.h"
#include "minilzo.h"
#include "doomdef.h"
#include "doomstat.h"
#include "gstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "v_video.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "g_game.h"
#include "p_setup.h"
#include "r_local.h"
#include "r_sky.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "cmdlib.h"
#include "s_sound.h"
#include "m_swap.h"
#include "gi.h"
#include "sv_main.h"
#include "sv_banlist.h"

EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_nomonsters)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)

extern size_t got_heapsize;

extern void M_RestoreMode (void);
extern void R_ExecuteSetViewSize (void);
void C_DoCommand (const char *cmd);

void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);

#ifdef UNIX
void daemon_init();
#endif

void D_DoomLoop (void);

extern gameinfo_t SharewareGameInfo;
extern gameinfo_t RegisteredGameInfo;
extern gameinfo_t RetailGameInfo;
extern gameinfo_t CommercialGameInfo;
extern gameinfo_t RetailBFGGameInfo;
extern gameinfo_t CommercialBFGGameInfo;

extern int testingmode;
extern BOOL setsizeneeded;
extern BOOL setmodeneeded;
extern BOOL netdemo;
extern int NewWidth, NewHeight, NewBits, DisplayBits;
EXTERN_CVAR (st_scale) // removeme
extern BOOL gameisdead;
extern BOOL demorecording;
extern DThinker ThinkerCap;

BOOL devparm;				// started game with -devparm
const char *D_DrawIcon;			// [RH] Patch name of icon to draw on next refresh
int NoWipe;					// [RH] Allow wipe? (Needs to be set each time)
char startmap[8];
BOOL autostart;
BOOL advancedemo;
event_t events[MAXEVENTS];
int eventhead;
int eventtail;
gamestate_t wipegamestate = GS_DEMOSCREEN;	// can be -1 to force a wipe
DCanvas *page;

static int demosequence;
static int pagetic;

const char *LOG_FILE;
static bool RebootInit;

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents (void)
{
}

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent (const event_t* ev)
{
	events[eventhead] = *ev;

	if(++eventhead >= MAXEVENTS)
		eventhead = 0;
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//
void D_Display (void)
{
}

//
// D_DoomLoop
//
void D_DoomLoop (void)
{
	while (1)
	{
		try
		{
			SV_RunTics (); // will run at least one tic
		}
		catch (CRecoverableError &error)
		{
			Printf (PRINT_HIGH, "ERROR: %s\n", error.GetMsg().c_str());
			Printf (PRINT_HIGH, "sleeping for 10 seconds before map reload...");

			// denis - drop clients
			SV_SendDisconnectSignal();

			// denis - sleep to conserve server resources (in case of recurring problem)
			I_WaitForTic(I_GetTime() + 1000*10/TICRATE);

			// denis - reload with current settings
			G_ChangeMap ();

			// denis - todo - throw I_FatalError if this keeps happening
		}
	}
}

//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker (void)
{
	if (--pagetic < 0)
		D_AdvanceDemo ();
}

//
// D_PageDrawer
//
void D_PageDrawer (void)
{
}

//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
	advancedemo = true;
}

//
// D_DoAdvanceDemo
//
void D_DoAdvanceDemo (void)
{
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
	gameaction = ga_nothing;
	demosequence = -1;
	D_AdvanceDemo ();
}

//
//D D_AddDefSkins
//
/*void D_AddDefSkins (void)
{
	// [RH] Add any .wad files in the skins directory
#ifndef UNIX // denis - fixme - 1) _findnext not implemented on linux or osx, use opendir 2) server does not need skins, does it?
	{
		char curdir[256];

		if (getcwd (curdir, 256))
		{
			char skindir[256];
			findstate_t findstate; // denis - fixme - win32 dependency == BAD!!! this is solved in later csdooms with BaseFileSearch - that could be implemented better with posix opendir stuff
			long handle;
			int stuffstart;

			std::string pd = progdir;
			if(pd[pd.length() - 1] != PATHSEPCHAR)
				pd += PATHSEPCHAR;

			stuffstart = sprintf (skindir, "%sskins", pd.c_str());

			if (!chdir (skindir))
			{
				skindir[stuffstart++] = PATHSEPCHAR;
				if ((handle = I_FindFirst ("*.wad", &findstate)) != -1)
				{
					do
					{
						if (!(I_FindAttr (&findstate) & FA_DIREC))
						{
							strcpy (skindir + stuffstart,
									I_FindName (&findstate));
							wadfiles.push_back(skindir);
						}
					} while (I_FindNext (handle, &findstate) == 0);
					I_FindClose (handle);
				}
			}

			const char *home = getenv ("HOME");
			if (home)
			{
				stuffstart = sprintf (skindir, "%s%s.odamex/skins", home,
									  home[strlen(home)-1] == PATHSEPCHAR ? "" : PATHSEP);
				if (!chdir (skindir))
				{
					skindir[stuffstart++] = PATHSEPCHAR;
					if ((handle = I_FindFirst ("*.wad", &findstate)) != -1)
					{
						do
						{
							if (!(I_FindAttr (&findstate) & FA_DIREC))
							{
								strcpy (skindir + stuffstart,
										I_FindName (&findstate));
								wadfiles.push_back(skindir);
							}
						} while (I_FindNext (handle, &findstate) == 0);
						I_FindClose (handle);
					}
				}
			}
			chdir (curdir);
		}
	}
#endif
}*/

void D_NewWadInit()
{
	if (DefaultsLoaded)	{		// [ML] This is being called while loading defaults,
		G_SetLevelStrings ();
		G_ParseMapInfo ();
		G_ParseMusInfo ();
		S_ParseSndInfo();

		R_Init();
		P_Init();
	} else {					// let DoomMain know it doesn't have to do everything
		RebootInit = true;
	}
}

//
// D_DoomMain
//
// [NightFang] - Cause I cant call ArgsSet from g_level.cpp
// [ML] 23/1/07 - Add Response file support back in
int teamplayset;

void D_DoomMain (void)
{
	const char *iwad;

	M_ClearRandom();
	// [AM] Init rand() PRNG, needed for non-deterministic maplist shuffling.
	srand(time(NULL));

	gamestate = GS_STARTUP;
	SetLanguageIDs ();
	M_FindResponseFile();		// [ML] 23/1/07 - Add Response file support back in

	if (lzo_init () != LZO_E_OK)	// [RH] Initialize the minilzo package.
		I_FatalError ("Could not initialize LZO routines");

    C_ExecCmdLineParams (false, true);	// [Nes] test for +logfile command

	// Always log by default
    if (!LOG.is_open())
    	C_DoCommand("logfile");

	Printf (PRINT_HIGH, "Heapsize: %u megabytes\n", got_heapsize);

	M_LoadDefaults ();			// load before initing other systems
	C_ExecCmdLineParams (true, false);	// [RH] do all +set commands on the command line

	if (!RebootInit) {
		iwad = Args.CheckValue("-iwad");
		if(!iwad)
			iwad = "";

		D_AddDefWads(iwad);
		D_AddCmdParameterFiles();

		wadhashes = W_InitMultipleFiles (wadfiles);
		for (size_t i = 0; i < wadfiles.size(); i++)
			wadfiles[i] = D_CleanseFileName(wadfiles[i], "wad");

		// [RH] Initialize localizable strings.
		GStrings.LoadStrings (W_GetNumForName ("LANGUAGE"), STRING_TABLE_SIZE, false);
		GStrings.Compact ();

		//D_InitStrings ();
		D_DoDefDehackedPatch();
	}

	I_Init ();

	// Base systems have been inited; enable cvar callbacks
	cvar_t::EnableCallbacks ();

	// [RH] User-configurable startup strings. Because BOOM does.
	if (GStrings(STARTUP1)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP1));
	if (GStrings(STARTUP2)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP2));
	if (GStrings(STARTUP3)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP3));
	if (GStrings(STARTUP4)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP4));
	if (GStrings(STARTUP5)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP5));

	// Nomonsters
	if (Args.CheckParm("-nomonsters"))
		sv_nomonsters = 1;

	// Respawn
	if (Args.CheckParm("-respawn"))
		sv_monstersrespawn = 1;

	// Fast
	if (Args.CheckParm("-fast"))
		sv_fastmonsters = 1;

	// developer mode
	devparm = Args.CheckParm ("-devparm");

    // get skill / episode / map from parms
	strcpy (startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	const char *val = Args.CheckValue ("-skill");
	if (val)
	{
		sv_skill.Set (val[0]-'0');
	}

	if (devparm)
		Printf (PRINT_HIGH, "%s", GStrings(D_DEVSTR));        // D_DEVSTR

	const char *v = Args.CheckValue ("-timer");
	if (v)
	{
		double time = atof (v);
		Printf (PRINT_HIGH, "Levels will end after %g minute%s.\n", time, time > 1 ? "s" : "");
		sv_timelimit.Set ((float)time);
	}

	const char *w = Args.CheckValue ("-avg");
	if (w)
	{
		Printf (PRINT_HIGH, "Austin Virtual Gaming: Levels will end after 20 minutes\n");
		sv_timelimit.Set (20);
	}

	// [RH] Now that all text strings are set up,
	// insert them into the level and cluster data.
	G_SetLevelStrings ();
	// [RH] Parse through all loaded mapinfo lumps
	G_ParseMapInfo ();
	// [ML] Parse the musinfo lump
	G_ParseMusInfo ();
	// [RH] Parse any SNDINFO lumps
	S_ParseSndInfo();

	// Check for -file in shareware
	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_Error ("You cannot -file with the shareware version. Register!");

	Printf (PRINT_HIGH, "R_Init: Init DOOM refresh daemon.\n");
	R_Init ();

	Printf (PRINT_HIGH, "P_Init: Init Playloop state.\n");
	P_Init ();

	Printf (PRINT_HIGH, "SV_InitNetwork: Checking network game status.\n");
    SV_InitNetwork();

	// [RH] Initialize items. Still only used for the give command. :-(
	InitItems ();

	// [AM] Initialize banlist
	SV_InitBanlist();

	// [RH] Lock any cvars that should be locked now that we're
	// about to begin the game.
	cvar_t::EnableNoSet ();

	// [RH] Now that all game subsystems have been initialized,
	// do all commands on the command line other than +set
	C_ExecCmdLineParams (false, false);

	Printf(PRINT_HIGH, "========== Odamex Server Initialized ==========\n");

#ifdef UNIX
	if (Args.CheckParm("-fork"))
            daemon_init();
#endif

	// Use wads mentioned on the commandline to start with
	//std::vector<std::string> start_wads;
	//std::string custwad;

	//iwad = Args.CheckValue("-iwad");
	//D_DoomWadReboot(start_wads);

	unsigned p = Args.CheckParm ("-warp");
	if (p && p < Args.NumArgs() - (1+(gameinfo.flags & GI_MAPxx ? 0 : 1)))
	{
		int ep, map;

		if (gameinfo.flags & GI_MAPxx)
		{
			ep = 1;
			map = atoi (Args.GetArg(p+1));
		}
		else
		{
			ep = Args.GetArg(p+1)[0]-'0';
			map = Args.GetArg(p+2)[0]-'0';
		}

		strncpy (startmap, CalcMapName (ep, map), 8);
		autostart = true;
	}

	// [RH] Hack to handle +map
	p = Args.CheckParm ("+map");
	if (p && p < Args.NumArgs()-1)
	{
		strncpy (startmap, Args.GetArg (p+1), 8);
		((char *)Args.GetArg (p))[0] = '-';
		autostart = true;
	}

	strncpy(level.mapname, startmap, sizeof(level.mapname));

	G_ChangeMap ();

	D_DoomLoop (); // never returns
}

VERSION_CONTROL (d_main_cpp, "$Id$")
