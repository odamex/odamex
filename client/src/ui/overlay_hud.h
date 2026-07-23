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

#pragma once

#include "ui/ui_layer.h"

class HudOverlay : public IOverlay
{
  public:
	void tick() override;
	bool responder(event_t* ev) override;
	int inputPriority() const override { return UIPRIO_HUD; }
};

HudOverlay& UI_HudOverlay();
