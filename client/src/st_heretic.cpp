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
//   Heretic statusbar/HUD baseline for OdaHeretic milestone work.
//
//   This keeps a lightweight Heretic-specific bottom HUD strip so level play
//   remains readable while full visual parity is still pending.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "gstrings.h"
#include "i_video.h"
#include "r_local.h"
#include "st_stuff.h"

namespace
{
void ST_HticSetLayoutHidden()
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

	ST_X = 0;
	ST_Y = surface->getHeight();
	ST_WIDTH = 0;
	ST_HEIGHT = 0;
}

void ST_HticSetLayoutVisible()
{
	IWindowSurface* surface = R_GetRenderingSurface();
	if (!surface)
	{
		ST_HticSetLayoutHidden();
		return;
	}

	ST_X = 0;
	ST_WIDTH = surface->getWidth();
	ST_HEIGHT = 18 * CleanYfac;
	ST_Y = surface->getHeight() - ST_HEIGHT;
}

int ST_HticReadyAmmo(const player_t& plyr)
{
	if (weaponinfo[plyr.readyweapon].ammotype == am_noammo)
		return -1;

	return plyr.ammo[weaponinfo[plyr.readyweapon].ammotype];
}

void ST_HticDrawTextRow()
{
	const player_t& plyr = displayplayer();
	const int ammo = ST_HticReadyAmmo(plyr);
	const int lineY = ST_Y + 4 * CleanYfac;

	screen->DrawText(CR_GOLD, ST_X + 8 * CleanXfac, lineY,
	                 fmt::format("HEALTH {:3d}", std::max(0, plyr.health)).c_str());
	if (ammo >= 0)
	{
		screen->DrawText(CR_GOLD, ST_X + (ST_WIDTH / 2) - (40 * CleanXfac), lineY,
		                 fmt::format("AMMO {:3d}", ammo).c_str());
	}
	else
	{
		screen->DrawText(CR_GOLD, ST_X + (ST_WIDTH / 2) - (40 * CleanXfac), lineY,
		                 "AMMO ---");
	}
	
	screen->DrawText(CR_GOLD, ST_X + ST_WIDTH - (120 * CleanXfac), lineY,
	                 fmt::format("ARMOR {:3d}", std::max(0, plyr.armorpoints)).c_str());
}
} // namespace

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
	if (!R_StatusBarVisible())
	{
		ST_HticSetLayoutHidden();
		return;
	}

	ST_HticSetLayoutVisible();
	ST_HticDrawTextRow();
}

void ST_HticShutdown()
{
}
