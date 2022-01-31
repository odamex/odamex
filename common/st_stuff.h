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
//	Status bar code.
//	Does the face/direction indicator animation.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------


#ifndef __STSTUFF_H__
#define __STSTUFF_H__


#include "com_misc.h"
#include "w_wad.h"

struct event_t;

// [RH] Turned these into variables
// Size of statusbar.
// Now ([RH] truly) sensitive for scaling.
extern int ST_HEIGHT;
extern int ST_WIDTH;
extern int ST_X;
extern int ST_Y;

//
// STATUS BAR
//

// From EE, gameinfo defined status bar
// haleyjd 10/12/03: DOOM's status bar object

void ST_DoomTicker();
void ST_DoomDrawer();
void ST_DoomStart();
void ST_DoomInit();

void ST_HticTicker();
void ST_HticDrawer();
void ST_HticStart();
void ST_HticInit();

// [ML] From EE, another gameinfo definition
// haleyjd 10/12/03: structure for gamemode-independent status bar interface

typedef struct stbarfns_s
{
	// data
	int height;

	// function pointers
	void (*Ticker)(); // tic processing
	void (*Drawer)(); // drawing
	// void (*FSDrawer)(void); // fullscreen drawer
	void (*Start)(); // reinit
	void (*Init)();  // initialize at startup
} stbarfns_t;

extern stbarfns_t DoomStatusBar;
extern stbarfns_t HticStatusBar;

// for st_lib.cpp
extern lumpHandle_t negminus;

int ST_StatusBarWidth(int surface_width, int surface_height);
int ST_StatusBarHeight(int surface_width, int surface_height);
int ST_StatusBarX(int surface_width, int surface_height);
int ST_StatusBarY(int surface_width, int surface_height);

void ST_ForceRefresh();

//
// STATUS BAR
//

// Called by main loop.
bool ST_Responder(event_t* ev);

// Called by main loop.
void ST_Ticker();

// Called by main loop.
void ST_Drawer();

// Called when the console player is spawned on each level.
void ST_Start();

// Called by startup code.
void ST_Init();

void STACK_ARGS ST_Shutdown();

namespace hud {

void drawNetdemo();

// [ML] New Odamex fullscreen HUD
void OdamexHUD();

// [AM] Draw obituary and event toasts.
void DrawToasts();

// [AM] Tick toasts - removing old ones.
void ToastTicker();

// [AM] Push a toast to the screen.
void PushToast(const toast_t& toast);

// [AM] HUD for showing level state
void LevelStateHUD();

// [AM] Spectator HUD
void SpectatorHUD();

// [AM] HUD drawn with the Doom Status Bar.
void DoomHUD();

}

// States for status bar code.
typedef enum
{
	AutomapState,
	FirstPersonState
	
} st_stateenum_t;

bool ST_Responder(event_t* ev);


#endif
