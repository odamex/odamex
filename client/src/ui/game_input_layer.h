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
//   Game input layers.
//
//   GameInputLayer  - the core game input (finale, key bindings, mouse/joystick
//                     movement). Active in every gamestate, lowest of the
//                     gameplay input layers, sandwiched by automap.
//   DemoInputLayer  - the demo-attract "any key opens the menu" handler. Highest
//                     gameplay priority, when active it swallows the event.
//
//-----------------------------------------------------------------------------

#pragma once

#include "ui/ui_layer.h"

class GameInputLayer : public IOverlay
{
  public:
	bool responder(event_t* ev) override;
	int inputPriority() const override { return UIPRIO_GAMEINPUT; }
};

class DemoInputLayer : public IOverlay
{
  public:
	bool responder(event_t* ev) override;
	int inputPriority() const override { return UIPRIO_DEMO; }
};

GameInputLayer& UI_GameInputLayer();
DemoInputLayer& UI_DemoInputLayer();
