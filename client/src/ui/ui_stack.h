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
//   UIStack - the stack of active UI overlays (menu, console).
//
//   Owns overlay ticking and drawing.
//   Tick and draw are done back to front, but
//   input is handled front layer to back layer.
//
//-----------------------------------------------------------------------------

#pragma once

#include <vector>

#include "d_event.h"

class IOverlay;

class UIStack
{
  public:
	void push(IOverlay* overlay);
	void clear();

	bool empty() const { return m_overlays.empty(); }

	void tick();
	void draw();
	bool responder(event_t* ev);

  private:
	std::vector<IOverlay*> m_overlays;
};

extern UIStack g_UIStack;

// Register the built-in global overlays (console, menu)
void UI_InitGlobalOverlays();
