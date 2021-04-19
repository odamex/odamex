// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//		DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
//		plus functions to determine game mode (shareware, registered),
//		parse command line parameters, configure game parameters (turbo),
//		and call the startup functions.
//
//-----------------------------------------------------------------------------

#include "version.h"

#include <string>
#include <vector>
#include <algorithm>

#include "win32inc.h"
#ifndef _WIN32
    #include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
#endif

#include <math.h>

#include "errors.h"

#include "m_alloc.h"
#include "m_random.h"
#include "minilzo.h"
#include "doomdef.h"
#include "doomstat.h"
#include "gstrings.h"
#include "z_zone.h"
#include "w_wad.h"
#include "s_sound.h"
#include "v_video.h"
#include "f_finale.h"
#include "f_wipe.h"
#include "m_argv.h"
#include "m_fileio.h"
#include "m_misc.h"
#include "m_menu.h"
#include "c_console.h"
#include "c_bind.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "i_music.h"
#include "i_video.h"
#include "i_input.h"
#include "g_game.h"
#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "c_effect.h"
#include "p_setup.h"
#include "r_local.h"
#include "r_sky.h"
#include "d_main.h"
#include "d_dehacked.h"
#include "cl_download.h"
#include "gi.h"
#include "stats.h"
#include "p_ctf.h"
#include "cl_main.h"
#include "sc_man.h"

#include "w_ident.h"

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef _XBOX
#include "i_xbox.h"
#endif

extern size_t got_heapsize;

void D_CheckNetGame (void);
void D_ProcessEvents (void);
void D_DoAdvanceDemo (void);

void D_DoomLoop (void);

extern int testingmode;
extern BOOL gameisdead;
extern BOOL demorecording;
extern bool M_DemoNoPlay;	// [RH] if true, then skip any demos in the loop
extern DThinker ThinkerCap;
extern dyncolormap_t NormalLight;

BOOL devparm;				// started game with -devparm
const char *D_DrawIcon;			// [RH] Patch name of icon to draw on next refresh
static bool wiping_screen = false;

char startmap[8];
BOOL autostart;
BOOL autorecord;
std::string demorecordfile;
BOOL advancedemo;
event_t events[MAXEVENTS];
int eventhead;
int eventtail;
gamestate_t wipegamestate = GS_DEMOSCREEN;	// can be -1 to force a wipe
bool demotest = false;

IWindowSurface* page_surface;

static int demosequence;
static int pagetic;

EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_nomonsters)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (sv_allowjump)
EXTERN_CVAR (sv_allowredscreen)
EXTERN_CVAR (snd_sfxvolume)				// maximum volume for sound
EXTERN_CVAR (snd_musicvolume)			// maximum volume for music

EXTERN_CVAR (vid_ticker)
EXTERN_CVAR (vid_defwidth)
EXTERN_CVAR (vid_defheight)
EXTERN_CVAR (vid_32bpp)
EXTERN_CVAR (vid_widescreen)
EXTERN_CVAR (vid_fullscreen)
EXTERN_CVAR (vid_vsync)

const char *LOG_FILE;

void M_RestoreVideoMode();
void M_ModeFlashTestText();

void D_SetPlatform(void)
{
#ifdef GCONSOLE
	#ifdef _XBOX
		platform = PF_XBOX;
	#elif GEKKO
		platform = PF_WII;
	#elif __SWITCH__
		platform = PF_SWITCH;
	#else
		platform = PF_UNKNOWN;
	#endif
#else
	platform = PF_PC;
#endif
}

//
// D_ProcessEvents
// Send all the events of the given timestamp down the responder chain
//
void D_ProcessEvents (void)
{
	event_t *ev;

	// [RH] If testing mode, do not accept input until test is over
	if (testingmode)
	{
		if (testingmode <= I_MSTime() * TICRATE / 1000)
			M_RestoreVideoMode();
		else
			M_ModeFlashTestText();

		return;
	}

	for (; eventtail != eventhead ; eventtail = ++eventtail<MAXEVENTS ? eventtail : 0)
	{
		ev = &events[eventtail];
		if (C_Responder (ev))
			continue;				// console ate the event
		if (M_Responder (ev))
			continue;				// menu ate the event
		G_Responder (ev);
	}
}

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent (const event_t* ev)
{
	if (ev->type == ev_mouse && !menuactive && gamestate == GS_LEVEL &&
		!paused && ConsoleState != c_down && ConsoleState != c_falling)
	{
		G_Responder((event_t*)ev);
		return;
	}

	events[eventhead] = *ev;

	if(++eventhead >= MAXEVENTS)
		eventhead = 0;
}

//
// D_DisplayTicker
//
// Called once every gametic to provide timing for display functions such
// as screenwipes.
//
void D_DisplayTicker()
{
	if (wiping_screen)
		wiping_screen = (Wipe_Ticker() == false);
}


//
// D_Display
//  draw current display, possibly wiping it from the previous
//
void D_Display()
{
	if (nodrawers || I_IsHeadless())
		return; 				// for comparative timing / profiling

	BEGIN_STAT(D_Display);

	// video mode must be changed before surfaces are locked in I_BeginUpdate
	V_AdjustVideoMode();

	I_BeginUpdate();

	// [RH] Allow temporarily disabling wipes
	if (NoWipe)
	{
		NoWipe--;
		wipegamestate = gamestate;
	}
	else if (gamestate != wipegamestate && gamestate != GS_FULLCONSOLE)
	{
		wipegamestate = gamestate;
		Wipe_Start();
		wiping_screen = true;
	}

	// We always want to service downloads, even outside of a specific
	// download gamestate.
	CL_DownloadTick();

	switch (gamestate)
	{
		case GS_FULLCONSOLE:
		case GS_CONNECTING:
        case GS_CONNECTED:
			C_DrawConsole();
			M_Drawer();
			I_FinishUpdate();
			return;

		case GS_LEVEL:
			if (!gametic)
				break;

			V_DoPaletteEffects();

			// Drawn to R_GetRenderingSurface()
			R_RenderPlayerView(&displayplayer());
			R_DrawViewBorder();
			ST_Drawer();

			if (I_GetEmulatedSurface())
				I_BlitEmulatedSurface();

			if (AM_ClassicAutomapVisible() || AM_OverlayAutomapVisible())
				AM_Drawer();

			CTF_DrawHud();
			HU_Drawer();
			C_DrawMid();
			C_DrawGMid();
			break;

		case GS_INTERMISSION:
			CTF_DrawHud();
			WI_Drawer();
			HU_Drawer();
			C_DrawMid();
			V_ResetPalette();
			break;

		case GS_FINALE:
			F_Drawer();
			break;

		case GS_DEMOSCREEN:
			D_PageDrawer();
			break;

	default:
	    break;
	}

	// draw pause pic
	if (paused && !menuactive)
	{
		patch_t *pause = W_CachePatch ("M_PAUSE");
		int y;

		y = AM_ClassicAutomapVisible() ? 4 : viewwindowy + 4;
		screen->DrawPatchCleanNoMove (pause, (I_GetSurfaceWidth()-(pause->width())*CleanXfac)/2, y);
	}

	// [RH] Draw icon, if any
	if (D_DrawIcon)
	{
		int lump = W_CheckNumForName (D_DrawIcon);

		D_DrawIcon = NULL;
		if (lump >= 0)
		{
			patch_t *p = W_CachePatch (lump);

			screen->DrawPatchIndirect (p, 160-p->width()/2, 100-p->height()/2);
		}
		NoWipe = 10;
	}

	if (wiping_screen)
		Wipe_Drawer();

	C_DrawConsole();	// draw console
	M_Drawer();			// menu is drawn even on top of everything
	I_FinishUpdate();	// page flip or blit buffer

	END_STAT(D_Display);
}

//
//  D_DoomLoop
//
void D_DoomLoop (void)
{
	while (1)
	{
		try
		{
			D_RunTics(CL_RunTics, CL_DisplayTics);
		}
		catch (CRecoverableError &error)
		{
			Printf(PRINT_ERROR, "\nERROR: %s\n", error.GetMsg().c_str());

			// [AM] In case an error is caused by a console command.
			C_ClearCommand();

			CL_QuitNetGame();

			G_ClearSnapshots();

			DThinker::DestroyAllThinkers();

			::players.clear();

			::gameaction = ga_fullconsole;
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
void D_PageDrawer()
{
	IWindowSurface* primary_surface = I_GetPrimarySurface();
	int surface_width = primary_surface->getWidth(), surface_height = primary_surface->getHeight();
	primary_surface->clear();		// ensure black background in matted modes

	if (page_surface)
	{
		int destw, desth;

		if (I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight()))
			destw = surface_width, desth = surface_height;
		else if (surface_width * 3 >= surface_height * 4)
			destw = surface_height * 4 / 3, desth = surface_height;
		else
			destw = surface_width, desth = surface_width * 3 / 4;

		page_surface->lock();

		primary_surface->blit(page_surface, 0, 0, page_surface->getWidth(), page_surface->getHeight(),
				(surface_width - destw) / 2, (surface_height - desth) / 2, destw, desth);

		page_surface->unlock();
	}
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
// This cycles through the demo sequences.
//
void D_DoAdvanceDemo (void)
{
	const char *pagename = NULL;

	consoleplayer().playerstate = PST_LIVE;	// not reborn
	advancedemo = false;
	usergame = false;				// no save / end game here
	paused = false;
	gameaction = ga_nothing;

    // [Russell] - Old demo sequence used in original games, zdoom's
    // dynamic one was too dynamic for its own good
    // [Nes] - Newer demo sequence with better flow.
    if (W_CheckNumForName("DEMO4") >= 0 && gamemode != retail_chex)
        demosequence = (demosequence+1)%8;
    else
        demosequence = (demosequence+1)%6;

    switch (demosequence)
    {
        case 0:
            if (gameinfo.flags & GI_MAPxx)
                pagetic = TICRATE * 11;
            else
                pagetic = 170;

            gamestate = GS_DEMOSCREEN;
            pagename = gameinfo.titlePage;
            
            currentmusic = gameinfo.titleMusic;

            S_StartMusic(currentmusic.c_str());

            break;
        case 1:
            G_DeferedPlayDemo("DEMO1");

            break;
        case 2:
            pagetic = 200;
            gamestate = GS_DEMOSCREEN;
            pagename = gameinfo.creditPage1;

            break;
        case 3:
            G_DeferedPlayDemo("DEMO2");

            break;
        case 4:
            gamestate = GS_DEMOSCREEN;

            if ((gameinfo.flags & GI_MAPxx) || (gameinfo.flags & GI_MENUHACK_RETAIL))
            {
				if (gameinfo.flags & GI_MAPxx)
					pagetic = TICRATE * 11;
				else
					pagetic = 170;
                pagename = gameinfo.titlePage;
                currentmusic = gameinfo.titleMusic;
                
                S_StartMusic(currentmusic.c_str());
            }
            else
            {
                pagetic = 200;
				if (gamemode == retail_chex)	// [ML] Chex mode just cycles this screen
					pagename = gameinfo.creditPage1;
				else
					pagename = gameinfo.creditPage2;
            }

            break;
        case 5:
            G_DeferedPlayDemo("DEMO3");

            break;
        case 6:
            pagetic = 200;
            gamestate = GS_DEMOSCREEN;
            pagename = gameinfo.creditPage2;

            break;
        case 7:
            G_DeferedPlayDemo("DEMO4");

            break;
    }

    // [Russell] - Still need this toilet humor for now unfortunately
	if (pagename)
	{
		const patch_t* patch = W_CachePatch(pagename);

		I_FreeSurface(page_surface);

		if (gameinfo.flags & GI_PAGESARERAW)
		{
			page_surface = I_AllocateSurface(320, 200, 8);
			DCanvas* canvas = page_surface->getDefaultCanvas();

			page_surface->lock();
            canvas->DrawBlock(0, 0, 320, 200, (byte*)patch);
			page_surface->unlock();
		}
		else
		{
			page_surface = I_AllocateSurface(patch->width(), patch->height(), 8);
			DCanvas* canvas = page_surface->getDefaultCanvas();

			page_surface->lock();
			canvas->DrawPatch(patch, 0, 0);
			page_surface->unlock();
		}
	}
}

//
// D_Close
//
void STACK_ARGS D_Close()
{
	I_FreeSurface(page_surface);

	D_ClearTaskSchedulers();
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
	// CL_QuitNetGame();

	gameaction = ga_nothing;
	demosequence = -1;
	D_AdvanceDemo();
}

bool HashOk(std::string &required, std::string &available)
{
	if(!required.length())
		return false;

	return required == available;
}


void CL_NetDemoPlay(const std::string &filename);


//
// D_Init
//
// Called to initialize subsystems when loading a new set of WAD resource
// files.
//
void D_Init()
{
	// only print init messages during startup, not when changing WADs
	static bool first_time = true;

	D_SetPlatform();

	SetLanguageIDs();

	M_ClearRandom();

	// start the Zone memory manager
	bool use_zone = !Args.CheckParm("-nozone");
	Z_Init(use_zone);
	if (first_time)
		Printf(PRINT_HIGH, "Z_Init: Heapsize: %" PRIuSIZE " megabytes\n", got_heapsize);

	// Load palette and set up colormaps
	V_Init();

//	if (first_time)
//		Printf(PRINT_HIGH, "Res_InitTextureManager: Init image resource management.\n");
//	Res_InitTextureManager();

	// [RH] Initialize localizable strings.
	GStrings.loadStrings();

	// init the renderer
	if (first_time)
		Printf(PRINT_HIGH, "R_Init: Init DOOM refresh daemon.\n");
	R_Init();

//	V_LoadFonts();

	C_InitConsoleBackground();

	C_InitConCharsFont();

	HU_Init();

	LevelInfos& levels = getLevelInfos();
	if (levels.size() == 0)
	{
		levels.addDefaults();
	}

	ClusterInfos& clusters = getClusterInfos();
	if (clusters.size() == 0)
	{
		clusters.addDefaults();
	}

	G_SetLevelStrings();
	G_ParseMapInfo();
	G_ParseMusInfo();
	S_ParseSndInfo();

	// init the menu subsystem
	if (first_time)
		Printf(PRINT_HIGH, "M_Init: Init miscellaneous info.\n");
	M_Init();

	if (first_time)
		Printf(PRINT_HIGH, "P_Init: Init Playloop state.\n");
	P_InitEffects();
	P_Init();

	// init sound and music
	if (first_time)
	{
		Printf (PRINT_HIGH, "S_Init: Setting up sound.\n");
		Printf (PRINT_HIGH, "S_Init: default sfx volume is %g\n", (float)snd_sfxvolume);
		Printf (PRINT_HIGH, "S_Init: default music volume is %g\n", (float)snd_musicvolume);
	}
	S_Init(snd_sfxvolume, snd_musicvolume);

//	R_InitViewBorder();

	// init the status bar
	if (first_time)
		Printf(PRINT_HIGH, "ST_Init: Init status bar.\n");
	ST_Init();

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

	getLevelInfos().clear();
	getClusterInfos().clear();

	F_ShutdownFinale();

	ST_Shutdown();

//	R_ShutdownViewBorder();

	// stop sound effects and music
	S_Stop();
	S_Deinit();
	
	// shutdown automap
	AM_Stop();

	DThinker::DestroyAllThinkers();

	D_UndoDehPatch();

	// close all open WAD files
	W_Close();

//	V_UnloadFonts();

	HU_Shutdown();

	C_ShutdownConCharsFont();

	C_ShutdownConsoleBackground();

	R_Shutdown();

	SC_Close();

//	Res_ShutdownTextureManager();

//	R_ShutdownColormaps();

	V_Close();

	// reset the Zone memory manager
	Z_Close();

	// [AM] All of our dyncolormaps are freed, tidy up so we
	//      don't follow wild pointers.
	NormalLight.next = NULL;
}


void C_DoCommand(const char *cmd, uint32_t key);

//
// D_DoomMain
//
void D_DoomMain()
{
	unsigned int p;

	gamestate = GS_STARTUP;

	atterm(D_Close);

	// init console so it can capture all of the startup messages
	C_InitConsole();
	atterm(C_ShutdownConsole);

	W_SetupFileIdentifiers();

	// [RH] Initialize items. Still only used for the give command. :-(
	InitItems();

	M_FindResponseFile();		// [ML] 23/1/07 - Add Response file support back in

	if (lzo_init() != LZO_E_OK)	// [RH] Initialize the minilzo package.
		I_FatalError("Could not initialize LZO routines");

	C_ExecCmdLineParams(false, true);	// [Nes] test for +logfile command

	// Always log by default
	if (!LOG.is_open())
		C_DoCommand("logfile", 0);

	M_LoadDefaults();					// load before initing other systems
	C_BindingsInit();					// Ch0wW : Initialize bindings

	C_ExecCmdLineParams(true, false);	// [RH] do all +set commands on the command line

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

	Printf(PRINT_HIGH, "I_Init: Init hardware.\n");
	atterm(I_ShutdownHardware);
	I_Init();
	I_InitInput();

	// [SL] Call init routines that need to be reinitialized every time WAD changes
	atterm(D_Shutdown);
	D_Init();

	atterm(I_Endoom);

	// Base systems have been inited; enable cvar callbacks
	cvar_t::EnableCallbacks();

	// [RH] User-configurable startup strings. Because BOOM does.
	if (GStrings(STARTUP1)[0])	Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP1));
	if (GStrings(STARTUP2)[0])	Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP2));
	if (GStrings(STARTUP3)[0])	Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP3));
	if (GStrings(STARTUP4)[0])	Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP4));
	if (GStrings(STARTUP5)[0])	Printf(PRINT_HIGH, "%s\n", GStrings(STARTUP5));

    // developer mode
	devparm = Args.CheckParm("-devparm");

	if (devparm)
		Printf(PRINT_HIGH, "%s", GStrings(D_DEVSTR));        // D_DEVSTR
 
	// set the default value for vid_ticker based on the presence of -devparm
	if (devparm)
		vid_ticker.SetDefault("1");
 
	// Nomonsters
	sv_nomonsters = Args.CheckParm("-nomonsters");

	// Respawn
	sv_monstersrespawn = Args.CheckParm("-respawn");

	// Fast
	sv_fastmonsters = Args.CheckParm("-fast");

	// get skill / episode / map from parms
	strcpy(startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	const char* val = Args.CheckValue("-skill");
	if (val)
		sv_skill.Set(val[0]-'0');

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

		strncpy(startmap, CalcMapName(ep, map), 8);
		autostart = true;
	}

	// [RH] Hack to handle +map
	p = Args.CheckParm("+map");
	if (p && p < Args.NumArgs()-1)
	{
		strncpy(startmap, Args.GetArg(p+1), 8);
		((char *)Args.GetArg(p))[0] = '-';
		autostart = true;
	}

	// NOTE(jsd): Set up local player color
	EXTERN_CVAR(cl_color);
	R_BuildPlayerTranslation(0, V_GetColorFromString(cl_color));

	I_FinishClockCalibration();

	// Initialize HTTP subsystem
	CL_DownloadInit();
	atterm(CL_DownloadShutdown);

	Printf(PRINT_HIGH, "D_CheckNetGame: Checking network game status.\n");
	D_CheckNetGame();

	// [RH] Lock any cvars that should be locked now that we're
	// about to begin the game.
	cvar_t::EnableNoSet();

	// [RH] Now that all game subsystems have been initialized,
	// do all commands on the command line other than +set
	C_ExecCmdLineParams(false, false);

	// --- process vanilla demo cli switches ---

	// shorttics (quantize yaw like recording a vanilla demo)
	extern bool longtics;
	longtics = !(Args.CheckParm("-shorttics"));

	// Record a vanilla demo
	p = Args.CheckParm("-record");
	if (p && p < Args.NumArgs() - 1)
	{
		autorecord = true;
		autostart = true;
		demorecordfile = Args.GetArg(p + 1);

		// extended vanilla demo format
		longtics = Args.CheckParm("-longtics");
	}

	// Check for -playdemo, play a single demo then quit.
	p = Args.CheckParm("-playdemo");
	// Hack to check for +playdemo command, since if you just add it normally
	// it won't run because it's attempting to run a demo and still set up the
	// first map as normal.
	if (!p)
		p = Args.CheckParm("+playdemo");
	if (p && p < Args.NumArgs() - 1)
	{
		Printf(PRINT_HIGH, "Playdemo parameter found on command line.\n");
		singledemo = true;

		extern std::string defdemoname;
		defdemoname = Args.GetArg(p+1);
	}

	// [SL] check for -timedemo (was removed at some point)
	p = Args.CheckParm("-timedemo");
	if (p && p < Args.NumArgs() - 1)
	{
		singledemo = true;
		G_TimeDemo(Args.GetArg(p + 1));
	}

	// denis - this will run a demo and quit
	p = Args.CheckParm("+demotest");
	if (p && p < Args.NumArgs() - 1)
	{
		singledemo = true;
		G_TestDemo(Args.GetArg(p + 1));
	}


	// --- process network demo cli switches ---

	// [SL] allow the user to pass the name of a netdemo as the first argument.
	// This allows easy launching of netdemos from Windows Explorer or other GUIs.
	//
	// [Xyltol]
	if (Args.GetArg(1))
	{
		std::string demoarg = Args.GetArg(1);
		const std::string demoext(".odd");

		// does demoarg have a .odd extensions?
		if (demoarg.find(".odd") == demoarg.length() - demoext.length())
			CL_NetDemoPlay(demoarg);
	}

	p = Args.CheckParm("-netplay");
	if (p && p < Args.NumArgs() - 1)
	{
		std::string filename = Args.GetArg(p + 1);
		CL_NetDemoPlay(filename);
	}

	// --- initialization complete ---

	Printf_Bold("\n\35\36\36\36\36 Odamex Client Initialized \36\36\36\36\37\n");
	if (gamestate != GS_CONNECTING)
		Printf(PRINT_HIGH, "Type connect <address> or use the Odamex Launcher to connect to a game.\n");
    Printf(PRINT_HIGH, "\n");

	// Play a demo, start a map, or show the title screen	
	if (singledemo)
	{
		G_DoPlayDemo();
	}
	else if (autostart)
	{
		if (autostart)
		{
			// single player warp (like in g_level)
			serverside = true;
			sv_allowexit = "1";
			sv_freelook = "1";
			sv_allowjump = "1";
			sv_allowredscreen = "1";
			sv_gametype = GM_COOP;

			players.clear();
			players.push_back(player_t());
			players.back().playerstate = PST_REBORN;
			consoleplayer_id = displayplayer_id = players.back().id = 1;
		}

		G_InitNew(startmap);
		if (autorecord)
			G_RecordDemo(startmap, demorecordfile);
	}
	else if (gamestate != GS_CONNECTING)
	{
		C_HideConsole();
		D_StartTitle();		// start up intro loop

		if (gamemode == commercial_bfg) // DOOM 2 BFG Edtion
			AddCommandString("menu_main");
    }

	D_DoomLoop();		// never returns
}

VERSION_CONTROL (d_main_cpp, "$Id$")
