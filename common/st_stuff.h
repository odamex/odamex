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
//	Status bar code.
//	Does the face/direction indicator animatin.
//	Does palette indicators as well (red pain/berserk, bright pickup)
//
//-----------------------------------------------------------------------------


#ifndef __STSTUFF_H__
#define __STSTUFF_H__

#include "doomtype.h"
#include "d_event.h"

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

// Called by main loop.
bool ST_Responder (event_t* ev);

// Called by main loop.
void ST_Ticker (void);

// Called by main loop.
void ST_Drawer (void);

// Called when the console player is spawned on each level.
void ST_Start (void);

// Called by startup code.
void ST_Init (void);

// Draw the HUD (only if old status bar is not drawn)
void ST_newDraw (void);

// Called on init and whenever player's skin changes
void ST_loadGraphics (void);

// [ML] HUDified status bar
void ST_drawStatusBar (void);

namespace hud {

// [ML] New Odamex fullscreen HUD
void OdamexHUD(void);

// [AM] Spectator HUD
void SpectatorHUD(void);

// [AM] Original ZDoom HUD
void ZDoomHUD(void);

// [AM] HUD drawn with the Doom Status Bar.
void DoomHUD(void);

}

// States for status bar code.
typedef enum
{
	AutomapState,
	FirstPersonState
	
} st_stateenum_t;

bool ST_Responder(event_t* ev);

// [RH] Base blending values (for e.g. underwater)
extern int BaseBlendR, BaseBlendG, BaseBlendB;
extern float BaseBlendA;


#endif


