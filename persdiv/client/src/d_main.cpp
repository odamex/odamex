// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#ifdef _XBOX
#include <xtl.h>
#else
#include <windows.h>
#endif // _XBOX
#else
#include <sys/stat.h>
#endif

#ifdef UNIX
#include <unistd.h>
#include <dirent.h>
#endif

#include <time.h>
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
#include "c_dispatch.h"
#include "i_system.h"
#include "i_sound.h"
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
#include "cmdlib.h"
#include "s_sound.h"
#include "m_swap.h"
#include "v_text.h"
#include "gi.h"
#include "stats.h"
#include "p_ctf.h"
#include "cl_main.h"

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifdef _XBOX
#include "i_xbox.h"
#endif

extern size_t got_heapsize;

//extern void M_RestoreMode (void); // [Toke - Menu]
extern void R_ExecuteSetViewSize (void);
void V_InitPalette (void);

void D_CheckNetGame (void);
void D_ProcessEvents (void);
void D_DoAdvanceDemo (void);

void D_DoomLoop (void);

extern QWORD testingmode;
extern BOOL setsizeneeded;
extern BOOL setmodeneeded;
extern int NewWidth, NewHeight, NewBits, DisplayBits;
EXTERN_CVAR (st_scale)
extern BOOL gameisdead;
extern BOOL demorecording;
extern bool M_DemoNoPlay;	// [RH] if true, then skip any demos in the loop
extern DThinker ThinkerCap;
extern int NoWipe;			// [RH] Don't wipe when travelling in hubs

BOOL devparm;				// started game with -devparm
const char *D_DrawIcon;			// [RH] Patch name of icon to draw on next refresh
int NoWipe;					// [RH] Allow wipe? (Needs to be set each time)
char startmap[8];
BOOL autostart;
BOOL autorecord;
std::string demorecordfile;
BOOL advancedemo;
event_t events[MAXEVENTS];
int eventhead;
int eventtail;
gamestate_t wipegamestate = GS_DEMOSCREEN;	// can be -1 to force a wipe
DCanvas *page;
bool demotest;

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

const char *LOG_FILE;

void M_RestoreMode (void);
void M_ModeFlashTestText (void);

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
		if (testingmode <= I_GetTime())
		{
			M_RestoreMode ();
		}
		else
		{
			M_ModeFlashTestText();
		}

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
	BOOL wipe;

	if (nodrawers)
		return; 				// for comparative timing / profiling

	BEGIN_STAT(D_Display);

	// [RH] change the screen mode if needed
	if (setmodeneeded)
	{
		int oldwidth = screen->width;
		int oldheight = screen->height;
		int oldbits = DisplayBits;

		// Change screen mode.
		if (!V_SetResolution (NewWidth, NewHeight, NewBits))
			if (!V_SetResolution (oldwidth, oldheight, oldbits))
				I_FatalError ("Could not change screen mode");

		// Recalculate various view parameters.
		setsizeneeded = true;
		// Trick status bar into rethinking its position
		st_scale.Callback ();
		// Refresh the console.
		C_NewModeAdjust ();
	}

	// [AM] Moved to below setmodeneeded so we have accurate screen size info.
	if (gamestate == GS_LEVEL && viewactive && consoleplayer().camera)
	{
		if (consoleplayer().camera->player)
			R_SetFOV(consoleplayer().camera->player->fov, setmodeneeded || setsizeneeded);
		else
			R_SetFOV(90.0f, setmodeneeded || setsizeneeded);
	}

	// change the view size if needed
	if (setsizeneeded)
	{
		R_ExecuteSetViewSize ();
		setmodeneeded = false;
	}

	I_BeginUpdate ();

	// [RH] Allow temporarily disabling wipes
	if (NoWipe)
	{
		NoWipe--;
		wipe = false;
		wipegamestate = gamestate;
	}
	else if (gamestate != wipegamestate && gamestate != GS_FULLCONSOLE)
	{ // save the current screen if about to wipe
		wipe = true;
		wipe_StartScreen ();
		wipegamestate = gamestate;
	}
	else
	{
		wipe = false;
	}

	switch (gamestate)
	{
		case GS_FULLCONSOLE:
		case GS_DOWNLOAD:
		case GS_CONNECTING:
        case GS_CONNECTED:
			C_DrawConsole ();
			M_Drawer ();
			I_FinishUpdate ();
			return;

		case GS_LEVEL:
			if (!gametic)
				break;

			// denis - freshen the borders (ffs..)
			R_DrawViewBorder ();    // erase old menu stuff

			if (viewactive)
				R_RenderPlayerView (&displayplayer());
			if (automapactive)
				AM_Drawer ();
			C_DrawMid ();
			C_DrawGMid();
			CTF_DrawHud ();
			ST_Drawer ();
			HU_Drawer ();
			break;

		case GS_INTERMISSION:
			if (viewactive)
				R_RenderPlayerView (&displayplayer());
			C_DrawMid ();
			CTF_DrawHud ();
			WI_Drawer ();
			HU_Drawer ();
			break;

		case GS_FINALE:
			F_Drawer ();
			break;

		case GS_DEMOSCREEN:
			D_PageDrawer ();
			break;

	default:
	    break;
	}

	// draw pause pic
	if (paused && !menuactive)
	{
		patch_t *pause = W_CachePatch ("M_PAUSE");
		int y;

		y = (automapactive && !viewactive) ? 4 : viewwindowy + 4;
		screen->DrawPatchCleanNoMove (pause, (screen->width-(pause->width())*CleanXfac)/2, y);
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

	static bool live_wiping = false;

	if (!wipe)
	{
		if(live_wiping)
		{
			// wipe update online (multiple calls, not just looping here)
			wipe_EndScreen();
			live_wiping = !wipe_ScreenWipe (1);
			C_DrawConsole ();
			M_Drawer ();			// menu is drawn even on top of wipes
			I_FinishUpdate ();		// page flip or blit buffer
		}
		else
		{
			// normal update
			C_DrawConsole ();	// draw console
			M_Drawer ();		// menu is drawn even on top of everything
			I_FinishUpdate ();	// page flip or blit buffer
		}
	}
	else
	{
		if(!connected)
		{
			// wipe update offline
			int wipestart, wipecont, nowtime, tics;
			BOOL done;

			wipe_EndScreen ();
			I_FinishUpdateNoBlit ();

			extern int canceltics;

			wipestart = I_GetTime ();
			wipecont = wipestart - 1;

			do
			{
				do
				{
					nowtime = I_GetTime ();
					tics = nowtime - wipecont;
				} while (!tics);
				wipecont = nowtime;
				I_BeginUpdate ();
				done = wipe_ScreenWipe (tics);
				C_DrawConsole ();
				M_Drawer ();			// menu is drawn even on top of wipes
				I_FinishUpdate ();		// page flip or blit buffer
			} while (!done);

			if(!connected)
				canceltics += I_GetTime () - wipestart;
		}
		else
		{
			// wipe update online
			live_wiping = true;

			// wipe update online (multiple calls, not just looping here)
			wipe_EndScreen();
			live_wiping = !wipe_ScreenWipe (1);
			C_DrawConsole ();
			M_Drawer ();			// menu is drawn even on top of wipes
			I_FinishUpdate ();		// page flip or blit buffer
		}
	}

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
			TryRunTics (); // will run at least one tic

			if (!connected)
				CL_RequestConnectInfo();

			// [RH] Use the consoleplayer's camera to update sounds
			S_UpdateSounds (listenplayer().camera);	// move positional sounds
			S_UpdateMusic();	// play another chunk of music

			// Update display, next frame, with current state.
			D_Display ();
		}
		catch (CRecoverableError &error)
		{
			Printf_Bold ("\n%s\n", error.GetMsg().c_str());

			CL_QuitNetGame ();

			G_ClearSnapshots ();

			DThinker::DestroyAllThinkers();

			players.clear();

			gameaction = ga_fullconsole;
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
	if (page)
	{
		page->Blit (0, 0, page->width, page->height,
			screen, 0, 0, screen->width, screen->height);
	}
	else
	{
		screen->Clear (0, 0, screen->width, screen->height, 0);
		//screen->PrintStr (0, 0, "Page graphic goes here", 22);
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

            S_StartMusic(gameinfo.titleMusic);

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
                S_StartMusic(gameinfo.titleMusic);
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
		int width = 320, height = 200;
		patch_t *data;

		if (page && (page->width != screen->width || page->height != screen->height))
		{
			I_FreeScreen(page);
			page = NULL;
		}

		data = W_CachePatch (pagename);

		if (page == NULL)
        {
            if (screen->isProtectedRes())
            {
                page = I_AllocateScreen (data->width(), data->height(), 8);
            }
            else
            {
                page = I_AllocateScreen (screen->width, screen->height, 8);
            }
        }

		page->Lock ();

		if (gameinfo.flags & GI_PAGESARERAW)
        {
            page->DrawBlock (0, 0, width, height, (byte *)data);
        }
		else if (screen->isProtectedRes())
        {
            page->DrawPatch(data,0,0);
        }
        else if ((float)screen->width/screen->height < (float)4.0f/3.0f)
        {
            page->DrawPatchStretched (data, 0, (screen->height / 2) - ((height * RealYfac) / 2), screen->width, (height * RealYfac));
        }
        else
        {
            page->DrawPatchStretched (data, (screen->width / 2) - ((width * RealXfac) / 2), 0, (width * RealXfac), screen->height);
        }

		page->Unlock ();
	}
}

//
// D_Close
//
void STACK_ARGS D_Close (void)
{
	if(page)
	{
		I_FreeScreen(page);
		page = NULL;
	}
}

//
// D_StartTitle
//
void D_StartTitle (void)
{
	// CL_QuitNetGame();
	bool firstTime = true;
	if(firstTime)
		atterm (D_Close);

	gameaction = ga_nothing;
	demosequence = -1;
	D_AdvanceDemo ();
}

bool HashOk(std::string &required, std::string &available)
{
	if(!required.length())
		return false;

	return required == available;
}

//
// D_AddDefSkins
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

//
// D_NewWadInit
//
// Client code that should be reset every time a new set of WADs are loaded
//
void D_NewWadInit()
{
	AM_Stop();

	HU_Init ();

	if(!(DefaultPalette = InitPalettes("PLAYPAL")))
		I_Error("Could not reinitialize palette");
	V_InitPalette();

	G_SetLevelStrings ();
	G_ParseMapInfo ();
	G_ParseMusInfo ();
	S_ParseSndInfo();

	M_Init();
	R_Init();
	P_InitEffects();	// [ML] Do this here so we don't have to put particle crap in server
	P_Init();

	S_Init (snd_sfxvolume, snd_musicvolume);
	ST_Init();
}

void CL_NetDemoRecord(const std::string &filename);
void CL_NetDemoPlay(const std::string &filename);

//
// D_DoomMain
//
void D_DoomMain (void)
{
	unsigned p;
	const char *iwad;
	extern std::string defdemoname;

	M_ClearRandom();

	gamestate = GS_STARTUP;
	SetLanguageIDs ();
	M_FindResponseFile();		// [ML] 23/1/07 - Add Response file support back in

	if (lzo_init () != LZO_E_OK)	// [RH] Initialize the minilzo package.
		I_FatalError ("Could not initialize LZO routines");

    C_ExecCmdLineParams (false, true);	// [Nes] test for +logfile command

	Printf (PRINT_HIGH, "Heapsize: %u megabytes\n", got_heapsize);

	M_LoadDefaults ();					// load before initing other systems
	C_ExecCmdLineParams (true, false);	// [RH] do all +set commands on the command line

	iwad = Args.CheckValue("-iwad");
	if(!iwad)
		iwad = "";

	D_AddDefWads(iwad);
	D_AddCmdParameterFiles();

	wadhashes = W_InitMultipleFiles (wadfiles);

	// [RH] Initialize localizable strings.
	GStrings.LoadStrings (W_GetNumForName ("LANGUAGE"), STRING_TABLE_SIZE, false);
	GStrings.Compact ();

	// [RH] Initialize configurable strings.
	//D_InitStrings ();
	D_DoDefDehackedPatch ();

	// [RH] Moved these up here so that we can do most of our
	//		startup output in a fullscreen console.

	HU_Init ();
	I_Init ();
	V_Init ();

    // SDL needs video mode set up first before input code can be used
    I_InitInput();

	// Base systems have been inited; enable cvar callbacks
	cvar_t::EnableCallbacks ();

	// [RH] User-configurable startup strings. Because BOOM does.
	if (GStrings(STARTUP1)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP1));
	if (GStrings(STARTUP2)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP2));
	if (GStrings(STARTUP3)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP3));
	if (GStrings(STARTUP4)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP4));
	if (GStrings(STARTUP5)[0])	Printf (PRINT_HIGH, "%s\n", GStrings(STARTUP5));

	// Nomonsters
	sv_nomonsters = Args.CheckParm("-nomonsters");

	// Respawn
	sv_monstersrespawn = Args.CheckParm("-respawn");

	// Fast
	sv_fastmonsters = Args.CheckParm("-fast");

    // developer mode
	devparm = Args.CheckParm ("-devparm");

	// Record a vanilla demo
	p = Args.CheckParm ("-record");
	if (p)
	{
		autorecord = true;
		autostart = true;
		demorecordfile = Args.GetArg (p+1);
	}

	// get skill / episode / map from parms
	strcpy (startmap, (gameinfo.flags & GI_MAPxx) ? "MAP01" : "E1M1");

	// Check for -playdemo, play a single demo then quit.
	p = Args.CheckParm ("-playdemo");
	// Hack to check for +playdemo command, since if you just add it normally
	// it won't run because it's attempting to run a demo and still set up the
	// first map as normal.
	if (!p)
		p = Args.CheckParm ("+playdemo");
	if (p && p < Args.NumArgs()-1)
	{
		Printf (PRINT_HIGH, "Playdemo parameter found on command line.\n");
		singledemo = true;
		defdemoname = Args.GetArg (p+1);
	}

	const char *val = Args.CheckValue ("-skill");
	if (val)
	{
		sv_skill.Set (val[0]-'0');
	}

	p = Args.CheckParm ("-warp");
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
	if (devparm)
		Printf (PRINT_HIGH, "%s", GStrings(D_DEVSTR));        // D_DEVSTR

	// [RH] Now that all text strings are set up,
	// insert them into the level and cluster data.
	G_SetLevelStrings ();

	// [RH] Parse through all loaded mapinfo lumps
	G_ParseMapInfo ();

	// [ML] Parse musinfo lump
	G_ParseMusInfo ();

	// [RH] Parse any SNDINFO lumps
	S_ParseSndInfo();

	// Check for -file in shareware
	if (modifiedgame && (gameinfo.flags & GI_SHAREWARE))
		I_Error ("You cannot -file with the shareware version. Register!");

#ifdef WIN32
	const char *sdlv = getenv("SDL_VIDEODRIVER");
	Printf (PRINT_HIGH, "Using %s video driver.\n",sdlv);
#endif

	Printf (PRINT_HIGH, "M_Init: Init miscellaneous info.\n");
	M_Init ();

	Printf (PRINT_HIGH, "R_Init: Init DOOM refresh daemon.\n");
	R_Init ();

	Printf (PRINT_HIGH, "P_Init: Init Playloop state.\n");
	P_InitEffects();	// [ML] Do this here so we don't have to put particle crap in server
	P_Init ();

	Printf (PRINT_HIGH, "S_Init: Setting up sound.\n");
	Printf (PRINT_HIGH, "S_Init: default sfx volume is %g\n", (float)snd_sfxvolume);
	Printf (PRINT_HIGH, "S_Init: default music volume is %g\n", (float)snd_musicvolume);
	S_Init (snd_sfxvolume, snd_musicvolume);

	I_FinishClockCalibration ();

	Printf (PRINT_HIGH, "D_CheckNetGame: Checking network game status.\n");
	D_CheckNetGame ();

	Printf (PRINT_HIGH, "ST_Init: Init status bar.\n");
	ST_Init ();

	// [RH] Initialize items. Still only used for the give command. :-(
	InitItems ();

	// [RH] Lock any cvars that should be locked now that we're
	// about to begin the game.
	cvar_t::EnableNoSet ();

	// [RH] Now that all game subsystems have been initialized,
	// do all commands on the command line other than +set
	C_ExecCmdLineParams (false, false);

	Printf_Bold("\n\35\36\36\36\36 Odamex Client Initialized \36\36\36\36\37\n");
	if(gamestate != GS_CONNECTING)
		Printf(PRINT_HIGH, "Type connect <address> or use the Odamex Launcher to connect to a game.\n");
    Printf(PRINT_HIGH, "\n");

	setmodeneeded = false; // [Fly] we don't need to set a video mode here!
    //gamestate = GS_FULLCONSOLE;

	// [SL] allow the user to pass the name of a netdemo as the first argument.
	// This allows easy launching of netdemos from Windows Explorer or other GUIs.

	// [Xyltol]
	if (Args.GetArg(1))
	{
		std::string demoarg = Args.GetArg(1);
		if (demoarg.find(".odd") != std::string::npos)
			CL_NetDemoPlay(demoarg);
	}

	p = Args.CheckParm("-netplay");
	if (p)
	{
		if (Args.GetArg(p + 1))
		{
			std::string filename = Args.GetArg(p + 1);
			CL_NetDemoPlay(filename);
		}
		else
		{
			Printf(PRINT_HIGH, "No netdemo filename specified.\n");
		}
	}

	// denis - bring back the demos
    if ( gameaction != ga_loadgame )
    {
		if (autostart || netgame || singledemo)
		{
			if (singledemo)
				G_DoPlayDemo();
			else
			{
				if(autostart)
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

				G_InitNew (startmap);
				if (autorecord)
					if (G_RecordDemo(demorecordfile.c_str()))
						G_BeginRecording();
			}
		}
        else
		{
            if (gamestate != GS_CONNECTING)
                gamestate = GS_HIDECONSOLE;

			C_ToggleConsole();

			if (gamemode == commercial_bfg) // DOOM 2 BFG Edtion
                AddCommandString("menu_main");

			D_StartTitle (); // start up intro loop
		}
    }

	// denis - this will run a demo and quit
	p = Args.CheckParm ("+demotest");
	if (p && p < Args.NumArgs()-1)
	{
		demotest = 1;
		defdemoname = Args.GetArg (p+1);
		G_DoPlayDemo();

		while(demoplayback)
		{
			DObject::BeginFrame ();
			G_Ticker();
			DObject::EndFrame ();
			gametic++;
		}
	}
	else
	{
		demotest = 0;
		D_DoomLoop ();		// never returns
	}
}

VERSION_CONTROL (d_main_cpp, "$Id$")





