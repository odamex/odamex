// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
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
//   HUD overlay adapter.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "ui/overlay_hud.h"

#include "g_game.h"
#include "hu_stuff.h"

void HudOverlay::tick()
{
	// HU_Ticker already ran pre-simulation (before G_Ticker) and self-gates
	// internally, so it moves here as-is. The status bar / automap tickers run
	// *after* P_Ticker inside G_Ticker, so those stay put until Phase 4.
	HU_Ticker();
}

bool HudOverlay::responder(event_t* ev)
{
	return G_HudResponder(ev);
}

HudOverlay& UI_HudOverlay()
{
	static HudOverlay hud;
	return hud;
}
