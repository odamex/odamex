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
//   All the global variables that store the internal state.
//   Theoretically speaking, the internal state of the engine
//    should be found by looking at the variables collected
//    here, and every relevant module will have to include
//    this header file.
//   In practice, things are a bit messy.
//
//-----------------------------------------------------------------------------


#pragma once

#include "doomdata.h"
#include "d_net.h"
#include "g_level.h"

// We also need the definition of a cvar
#include "c_cvars.h"
#include "d_netinf.h"

// ------------------------
// Command line parameters.
//
extern bool				devparm;		// DEBUG: launched with -devparm


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

// Bot game? Like netgame, but doesn't involve network communication.
extern	BOOL			multiplayer;

extern BOOL            network_game;

// Game mode
EXTERN_CVAR (sv_gametype)
EXTERN_CVAR (sv_maxplayers)

#define GM_COOP		0.0f
#define GM_DM		1.0f
#define GM_TEAMDM	2.0f
#define GM_CTF		3.0f
#define GM_HORDE	4.0f

#define FPS_NONE	0
#define FPS_FULL	1
#define FPS_COUNTER	2

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

extern	bool			menuactive; 	// Menu overlayed?
extern	BOOL			paused; 		// Game Pause?


extern	BOOL			viewactive;

extern	bool	 		nodrawers;
extern	bool	 		noblit;

extern	int 			viewwindowx;
extern	int 			viewwindowy;
extern	"C" int 		viewheight;
extern	"C" int 		viewwidth;


extern level_locals_t level;


// --------------------------------------
// DEMO playback/recording related stuff.
// No demo, there is a human player in charge?
// Disable save/end game?
extern	BOOL			usergame;

extern	BOOL			demoplayback;

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
extern std::vector<mapthing2_t> DeathMatchStarts;

// Player spawn spots.
#define MAXPLAYERSTARTS		64
extern std::vector<mapthing2_t> playerstarts;
extern std::vector<mapthing2_t> voodoostarts;

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

EXTERN_CVAR (mouse_sensitivity) // removeme // ?

// Needed to store the number of the dummy sky flat.
// Used for rendering,
//	as well as tracking projectiles etc.
extern int				skyflatnum;

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
extern DehInfo deh;
