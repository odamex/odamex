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

#include "odamex.h"

#include "ui/ui_stack.h"

#include "ui/ui_layer.h"
#include "ui/overlay_console.h"
#include "ui/overlay_menu.h"

UIStack g_UIStack;

void UIStack::push(IOverlay* overlay)
{
	m_overlays.push_back(overlay);
}

void UIStack::clear()
{
	m_overlays.clear();
}

void UIStack::tick()
{
	// front -> back: console ticks before menu (matches legacy order).
	for (size_t i = 0; i < m_overlays.size(); i++)
		m_overlays[i]->tick();
}

void UIStack::draw()
{
	// front -> back: bottom overlay first, topmost overlay drawn last.
	for (size_t i = 0; i < m_overlays.size(); i++)
		m_overlays[i]->draw();
}

bool UIStack::responder(event_t* ev)
{
	// back -> front: the topmost overlay gets first crack at input. A layer
	// that consumes the event, or that blocks input, stops propagation.
	for (size_t i = m_overlays.size(); i-- > 0;)
	{
		if (m_overlays[i]->responder(ev))
			return true;
		if (m_overlays[i]->blocksInput())
			return true;
	}
	return false;
}

void UI_InitGlobalOverlays()
{
	g_UIStack.clear();
	g_UIStack.push(&UI_ConsoleOverlay()); // bottom: drawn first, input last
	g_UIStack.push(&UI_MenuOverlay());    // top: drawn last (over console)
}
