// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//   The freecam is a clientside-only player that is injected and used
//   with spynext to free roam the map during netdemos or allowed gamemodes
//
//-----------------------------------------------------------------------------

#pragma once

#include "odamex.h"
#include "cl_main.h"

inline constexpr byte freecamplayer_id = 255;

namespace Freecam
{
	extern std::string prevmap;
	void addFreecamPlayer();
	void savePosition();
	void reset();
	void setStartPosition(fixed_t x, fixed_t y, fixed_t z, angle_t angle);
	bool needPosition();
	bool allowAdd();
	bool allowSpy();
	void retireFor255thPlayer(player_t* cam);	
	bool wipedOnLevelChange(player_t* cam);
	void buildCam(player_t* p_cam);
};
