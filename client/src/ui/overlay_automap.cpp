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
//   Automap overlay adapter.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "ui/overlay_automap.h"

#include "g_game.h"

bool AutomapOverlay::responder(const event_t* ev)
{
	return G_AutomapResponder(ev);
}

int AutomapOverlay::inputPriority() const
{
	return viewactive ? UIPRIO_AUTOMAP_POST : UIPRIO_AUTOMAP_PRE;
}

AutomapOverlay& UI_AutomapOverlay()
{
	static AutomapOverlay automap;
	return automap;
}
