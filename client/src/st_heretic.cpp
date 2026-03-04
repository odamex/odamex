// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//   Heretic statusbar/HUD scaffold for OdaHeretic milestone work.
//
//   This intentionally keeps the Doom statusbar code path disabled for Heretic
//   until full HUD/statusbar parity lands. The inter-level/game HUD continues
//   to render through the existing HU/ST_NEW paths.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "i_video.h"
#include "r_local.h"
#include "st_stuff.h"

static void ST_HticSetLayoutHidden()
{
	IWindowSurface* surface = R_GetRenderingSurface();
	if (!surface)
	{
		ST_X = 0;
		ST_Y = 0;
		ST_WIDTH = 0;
		ST_HEIGHT = 0;
		return;
	}

	// No classic status bar drawn yet for Heretic.
	ST_X = 0;
	ST_Y = surface->getHeight();
	ST_WIDTH = 0;
	ST_HEIGHT = 0;
}

void ST_HticInit()
{
}

void ST_HticStart()
{
	ST_ForceRefresh();
}

void ST_HticTicker()
{
}

void ST_HticDrawer()
{
	ST_HticSetLayoutHidden();
}

void ST_HticShutdown()
{
}
