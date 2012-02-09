// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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


// From EE, gameinfo defined status bar
// haleyjd 10/12/03: DOOM's status bar object

void ST_DoomTicker (void);
void ST_DoomDrawer (void);
void ST_DoomStart (void);
void ST_DoomInit (void);

void ST_HticTicker (void);
void ST_HticDrawer (void);
void ST_HticStart (void);
void ST_HticInit (void);


// [ML] From EE, another gameinfo definition
// haleyjd 10/12/03: structure for gamemode-independent status bar interface

typedef struct stbarfns_s
{
   // data
   int  height;

   // function pointers
   void (*Ticker)(void);   // tic processing
   void (*Drawer)(void);   // drawing
   //void (*FSDrawer)(void); // fullscreen drawer
   void (*Start)(void);    // reinit
   void (*Init)(void);     // initialize at startup   
} stbarfns_t;

extern stbarfns_t DoomStatusBar;
extern stbarfns_t HticStatusBar;

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

// Draw the Heretic FS HUD (only if old status bar is not drawn)
void ST_HticDrawFullScreenStuff (void);

// Draw the HUD (only if old status bar is not drawn)
void ST_newDraw (void);

// Draw the New DM HUD (only if old status bar is not drawn, multiplayer and user choice)
void ST_newDrawDM (void);

// Draw the CTF HUD (separated for spectator)
void ST_newDrawCTF (void);

// Draws name of player currently in control of the status bar
void ST_nameDraw (int y);

// Called on init and whenever player's skin changes
void ST_loadGraphics (void);
void ST_HticLoadGraphics (void);

// [ML] HUDified status bar
void ST_drawStatusBar (void);

// [ML] New Odamex fullscreen HUD
void ST_odamexHudDraw(void);
void ST_odamexHudDrawCTF(void);


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


