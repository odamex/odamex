// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//		Status bar dispatch and shared helpers.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "st_stuff.h"
#include "i_video.h"
#include "st_lib.h"
#include "gi.h"
#include "d_player.h"
#include "r_main.h"

bool st_needrefresh = true;

EXTERN_CVAR(st_scale)
EXTERN_CVAR(screenblocks)

static const stbarfns_t* gStatusBar = &DoomStatusBar;

static const stbarfns_t& ST_GetStatusBar()
{
	return *(gStatusBar ? gStatusBar : &DoomStatusBar);
}

stbarfns_t DoomStatusBar = {
    32,
    ST_DoomBaseWidth,
    ST_DoomResponder,
    ST_DoomTicker,
    ST_DoomDrawer,
    ST_DoomStart,
    ST_DoomInit,
    ST_DoomShutdown,
};

stbarfns_t HticStatusBar = {
    42,
    ST_HticBaseWidth,
    ST_HticResponder,
    ST_HticTicker,
    ST_HticDrawer,
    ST_HticStart,
    ST_HticInit,
    ST_HticShutdown,
};

// [RH] Status bar background
IWindowSurface* stbar_surface;
IWindowSurface* stnum_surface;

// [RH] Turned these into variables
// Size of statusbar.
// Now ([RH] truly) sensitive for scaling.
int ST_HEIGHT;
int ST_WIDTH;
int ST_X;
int ST_Y;

short ST_BaseHeight()
{
	return ST_GetStatusBar().height;
}

int ST_StatusBarHeight(int surface_width, int surface_height)
{
	if (!R_StatusBarVisible())
		return 0;

	if (st_scale)
		return ST_BaseHeight() * surface_height / 200;

	return ST_BaseHeight();
}

short ST_BaseWidth()
{
	return ST_GetStatusBar().BaseWidth();
}

short ST_StatusBarWidth(int surface_width, int surface_height)
{
	if (!R_StatusBarVisible())
		return 0;

	const short base_width = ST_BaseWidth();

	if (I_IsProtectedResolution(surface_width, surface_height))
	{
		int height = ST_StatusBarHeight(surface_width, surface_height);

		if (base_width > 320)
			return (height / ST_BaseHeight()) * base_width;

		return 10 * height;
	}

	if (st_scale)
		return (base_width / 80) * surface_height / 3;

	return base_width;
}

int ST_StatusBarX(int surface_width, int surface_height)
{
	if (!R_StatusBarVisible())
		return 0;

	if (consoleplayer().spectator && displayplayer_id == consoleplayer_id)
		return 0;

	return (surface_width - ST_StatusBarWidth(surface_width, surface_height)) / 2;
}

int ST_StatusBarY(int surface_width, int surface_height)
{
	if (!R_StatusBarVisible())
		return surface_height;

	if (consoleplayer().spectator && displayplayer_id == consoleplayer_id)
		return surface_height;

	return surface_height - ST_StatusBarHeight(surface_width, surface_height);
}

void ST_ForceRefresh()
{
	st_needrefresh = true;
}

void ST_SetStatusBar(const stbarfns_t* statusBar)
{
	gStatusBar = statusBar ? statusBar : &DoomStatusBar;
}

CVAR_FUNC_IMPL(st_scale)
{
	R_SetViewSize(screenblocks.asInt());
	ST_ForceRefresh();
}

bool ST_Responder(const event_t& ev)
{
	return ST_GetStatusBar().Responder(ev);
}

void ST_Ticker()
{
	ST_GetStatusBar().Ticker();
}

void ST_Drawer()
{
	ST_GetStatusBar().Drawer();
}

void ST_Start()
{
	ST_GetStatusBar().Start();
}

void ST_Init()
{
	ST_GetStatusBar().Init();
}

void STACK_ARGS ST_Shutdown()
{
	ST_GetStatusBar().Shutdown();
}

VERSION_CONTROL(st_stuff_cpp, "$Id$")
