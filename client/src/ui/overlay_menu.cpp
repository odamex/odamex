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
//   Menu overlay adapter.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "ui/overlay_menu.h"

#include "c_cvars.h"
#include "m_menu.h"

void MenuOverlay::tick()
{
	M_Ticker();
}

void MenuOverlay::draw()
{
	M_Drawer();
}

bool MenuOverlay::responder(event_t* ev)
{
	return M_Responder(ev);
}

MenuOverlay& UI_MenuOverlay()
{
	static MenuOverlay menu;
	return menu;
}
