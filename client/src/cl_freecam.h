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
//   The freecam is a clientside-only player that is injected and used
//   with spynext to free roam the map during netdemos or allowed gamemodes
//
//-----------------------------------------------------------------------------

#pragma once

#include "odamex.h"
#include "cl_main.h"

inline constexpr byte freecamplayer_id = 255;

class Freecam
{
public:
	Freecam() = delete; // static / no constructor
	static std::string prevmap;
	static bool wadchanged;
	static void addFreecamPlayer();
	static void savePosition();
	static void reset();
	static void setStartPosition(fixed_t x, fixed_t y, fixed_t z, angle_t angle);
	static bool needPosition();
	static bool allowAdd();
	static bool allowSpy();
	static void retireFor255thPlayer(player_t* cam);

private:
	static fixed_t x;
	static fixed_t y;
	static fixed_t z;
	static angle_t angle;
	static fixed_t pitch;
	static bool wipedOnLevelChange(player_t* cam);
	static void buildCam(player_t* p_cam);
};
