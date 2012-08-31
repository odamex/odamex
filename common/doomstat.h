// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: doomstat.h 1650 2010-07-12 03:22:33Z russellrice $
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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------


#ifndef __D_STATE__
#define __D_STATE__

#include <vector>

#include "doomdata.h"
#include "d_net.h"
#include "g_level.h"

// We also need the definition of a cvar
#include "c_cvars.h"

// ------------------------
// Command line parameters.
//
extern	BOOL			devparm;		// DEBUG: launched with -devparm


// -----------------------------------------------------
// Game Mode - identify IWAD as shareware, retail etc.
//
extern GameMode_t		gamemode;
extern GameMission_t	gamemission;

// Set if homebrew PWAD stuff has been added.
extern	BOOL			modifiedgame;


// -------------------------------------------
// Selected skill type, map etc.
//

extern	char			startmap[8];		// [RH] Actual map name now

extern	BOOL 			autostart;

// Selected by user.
EXTERN_CVAR (sv_skill)

// Nightmare mode flag, single player.
extern	BOOL 			respawnmonsters;

// Netgame? Only true if >1 player.
extern	BOOL			netgame;

// Bot game? Like netgame, but doesn't involve network communication.
extern	BOOL			multiplayer;

extern BOOL            network_game;

// Game mode
EXTERN_CVAR (sv_gametype)

#define GM_COOP		0.0f
#define GM_DM		1.0f
#define GM_TEAMDM	2.0f
#define GM_CTF		3.0f


// -------------------------
// Internal parameters for sound rendering.
// These have been taken from the DOS version,
//	but are not (yet) supported with Linux
//	(e.g. no sound volume adjustment with menu.

// These are not used, but should be (menu).
// From m_menu.c:
//	Sound FX volume has default, 0 - 15
//	Music volume has default, 0 - 15
// These are multiplied by 8.


// -------------------------
// Status flags for refresh.
//

// Depending on view size - no status bar?
// Note that there is no way to disable the
//	status bar explicitely.
extern	BOOL			statusbaractive;

extern	BOOL			automapactive;	// In AutoMap mode?
extern	BOOL			menuactive; 	// Menu overlayed?
extern	BOOL			paused; 		// Game Pause?


extern	BOOL			viewactive;

extern	BOOL	 		nodrawers;
extern	BOOL	 		noblit;

extern	int 			viewwindowx;
extern	int 			viewwindowy;
extern	"C" int 		viewheight;
extern	"C" int 		viewwidth;

extern	"C" int			realviewwidth;		// [RH] Physical width of view window
extern	"C" int			realviewheight;		// [RH] Physical height of view window
extern	"C" int			detailxshift;		// [RH] X shift for horizontal detail level
extern	"C" int			detailyshift;		// [RH] Y shift for vertical detail level





// This one is related to the 3-screen display mode.
// ANG90 = left side, ANG270 = right
extern	int				viewangleoffset;

extern level_locals_t level;


// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern	BOOL			usergame;

extern	BOOL			demoplayback;
extern	BOOL			demorecording;
extern	int				demover;
extern	BOOL			democlassic;
extern	BOOL			autorecord;
extern	std::string		demorecordfile;

// Quit after playing a demo from cmdline.
extern	BOOL			singledemo;




extern	gamestate_t 	gamestate;






//-----------------------------
// Internal parameters, fixed.
// These are set by the engine, and not changed
//	according to user inputs. Partly load from
//	WAD, partly set at startup time.

extern	int 			gametic;

// Player spawn spots for deathmatch.
extern	int				MaxDeathmatchStarts;
extern	mapthing2_t		*deathmatchstarts;
extern	mapthing2_t*	deathmatch_p;

// Player spawn spots.
#define MAXPLAYERSTARTS		64
extern std::vector<mapthing2_t> playerstarts;
extern std::vector<mapthing2_t> voodoostarts;

// ----------------------------------------------
//	[Toke - CTF - starts]

		// Blue team starts
extern	mapthing2_t		*blueteamstarts;
extern	size_t			MaxBlueTeamStarts;
extern	mapthing2_t*	blueteam_p;

		// Red team starts
extern	mapthing2_t		*redteamstarts;
extern	size_t			MaxRedTeamStarts;
extern	mapthing2_t*	redteam_p;

// ----------------------------------------------

// Intermission stats.
// Parameters for world map / intermission.
extern	struct wbstartstruct_s wminfo;


// LUT of ammunition limits for each kind.
// This doubles with BackPack powerup item.
extern	int 			maxammo[NUMAMMO];

//-----------------------------------------
// Internal parameters, used for engine.
//

// if true, load all graphics at level load
extern	BOOL	 		precache;

// wipegamestate can be set to -1
//	to force a wipe on the next draw
extern gamestate_t wipegamestate;

// denis - is this from hexen?
extern BOOL setsizeneeded;
extern BOOL setmodeneeded;

EXTERN_CVAR (mouse_sensitivity) // removeme // ?

// Needed to store the number of the dummy sky flat.
// Used for rendering,
//	as well as tracking projectiles etc.
extern int				skyflatnum;

// Netgame stuff (buffers and pointers, i.e. indices).
extern	int 			maketic;
extern	int 			ticdup;


// ---- [RH] ----
EXTERN_CVAR (developer) // removeme

// [RH] Miscellaneous info for DeHackEd support
struct DehInfo
{
	int StartHealth;
	int StartBullets;
	int MaxHealth;
	int MaxArmor;
	int GreenAC;
	int BlueAC;
	int MaxSoulsphere;
	int SoulsphereHealth;
	int MegasphereHealth;
	int GodHealth;
	int FAArmor;
	int FAAC;
	int KFAArmor;
	int KFAAC;
	int BFGCells;
	int Infight;
};
extern struct DehInfo deh;

#endif



