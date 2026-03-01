// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//   The freecam is a client-side only player thats injected
//	 during netdemo playback, lets you free roam
//
//-----------------------------------------------------------------------------

#pragma once

#include "odamex.h"
#include "cl_main.h"

class Freecam
{
public:
	static const byte freecam_id = 255;
	static void AddFreecamPlayer();
	static void BuildCam(player_t* p_cam);
	static void SavePosition();
	static void Reset();
	static void SetStartPosition(fixed_t x, fixed_t y, fixed_t z, fixed_t angle);
	static bool NeedPosition();
	static bool WipedOnLevelChange();
	static void RebuildCamOnLevelChange();

private:
	static fixed_t x;
	static fixed_t y;
	static fixed_t z;
	static fixed_t angle;
	static fixed_t pitch;
};
