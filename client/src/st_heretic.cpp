// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: st_stuff.cpp 1579 2010-03-12 23:01:40Z mike $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Heretic-specific status bar code.
//-----------------------------------------------------------------------------

#include "doomstat.h"
#include "m_random.h"
#include "p_local.h"
#include "v_video.h"
#include "w_wad.h"
#include "st_stuff.h"

//
// Defines
//
#define ST_HBARHEIGHT 42
#define plyr (&players[displayplayer])

//
// Static variables
//

// cached patches
static patch_t *invnums[10];   // inventory numbers

// current state variables
static int chainhealth;        // current position of the gem
static int chainwiggle;        // small randomized addend for chain y coord.

//
// ST_HticInit
//
// Initializes the Heretic status bar:
// * Caches most patch graphics used throughout
//
static void ST_HticInit(void)
{

}

//
// ST_HticStart
//
static void ST_HticStart(void)
{
}

//
// ST_HticTicker
//
// Processing code for Heretic status bar
//
static void ST_HticTicker(void)
{
 
}

static void ST_drawInvNum(int num, int x, int y)
{

}

//
// ST_drawBackground
//
// Draws the basic status bar background
//
static void ST_drawBackground(void)
{
 
}

#define SHADOW_BOX_WIDTH  16
#define SHADOW_BOX_HEIGHT 10

static void ST_BlockDrawerS(int x, int y, int startmap, int mapdir)
{

}

//
// ST_chainShadow
//
// Draws two 16x10 shaded areas on the ends of the life chain, using
// the light-fading colormaps to darken what's already been drawn.
//
static void ST_chainShadow(void)
{

}

//
// ST_drawLifeChain
//
// Draws the chain & gem health indicator at the bottom
//
static void ST_drawLifeChain(void)
{
 
}

//
// ST_drawStatBar
//
// Draws the main status bar, shown when the inventory is not
// active.
//
static void ST_drawStatBar(void)
{
 
}

//
// ST_HticDrawer
//
// Draws the Heretic status bar
//
static void ST_HticDrawer(void)
{

}

//
// ST_HticFSDrawer
//
// Draws the Heretic fullscreen hud/status information.
//
static void ST_HticFSDrawer(void)
{
}

//
// Status Bar Object for GameModeInfo
//
stbarfns_t HticStatusBar =
{
   ST_HBARHEIGHT,

   ST_HticTicker,
   ST_HticDrawer,
   ST_HticStart,
   ST_HticInit,
};

VERSION_CONTROL (st_heretic_cpp, "$Id: st_stuff.cpp 1579 2010-03-12 23:01:40Z mike $")
