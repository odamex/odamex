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
//   UI layer abstractions.
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_event.h"

//
// Input dispatch priorities.
//
// The UIStack walks layers by inputPriority(),
// highest first to lowest, to determine input cascade order.
//
enum
{
	UIPRIO_SCENE          = 0,   // base: the active scene / fallthrough
	UIPRIO_AUTOMAP_POST   = 300, // automap when the 3D view is active (post-binding)
	UIPRIO_GAMEINPUT      = 400, // key bindings + mouse/joystick movement
	UIPRIO_AUTOMAP_PRE    = 500, // automap when the 3D view is inactive (pre-binding)
	UIPRIO_HUD            = 800, // net-demo/spectator keys, chat, status bar
	UIPRIO_DEMO           = 850, // demo-attract "any key opens the menu"
	UIPRIO_MENU           = 900,
	UIPRIO_CONSOLE        = 1000,
};

//
// ILayer
//
// The common lifecycle + dispatch surface for scenes and overlays.
//
class ILayer
{
  public:
	virtual ~ILayer() {}

	// Called when the layer becomes active / is removed.
	virtual void onEnter() {}
	virtual void onExit() {}

	// Per-tic update and per-frame draw.
	virtual void tick() {}
	virtual void draw() {}

	// Return true if the event was consumed and should not propagate further.
	virtual bool responder(event_t* ev) { return false; }

	// Input dispatch order key (higher = receives input earlier). Evaluated per
	// event, so a layer may vary it with runtime state (e.g. the automap).
	virtual int inputPriority() const { return UIPRIO_SCENE; }
};

//
// IOverlay
//
// A stackable UI layer drawn over the active scene. The propagation predicates
// let an overlay decide whether layers beneath it still receive input, ticks or
// draws. Covers global UI layers, like console and menu, but also layers unique
// to LevelScene, like HUD and Automap.
//
class IOverlay : public ILayer
{
  public:
	// Does this overlay swallow input before layers beneath it get a look?
	virtual bool blocksInput() const { return false; }

	// Does this overlay stop layers beneath it from ticking?
	virtual bool blocksTick() const { return false; }

	// Does this overlay fully obscure layers beneath it (skip their draw)?
	virtual bool blocksDraw() const { return false; }
};
