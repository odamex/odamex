// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Marisa "Randi" Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	Calculate and draw speed of player.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "hu_speedometer.h"

#include <math.h>

double gDisplaySpeed;

/**
 * @brief Add the given player's speed to the speedometer
 * 
 * @param id ID of the player to check speed for.
 * @param start Starting position.
 * @param end Ending position.
 */
void HU_AddPlayerSpeed(const v3double_t& start, const v3double_t& end)
{
	const v3double_t origin(end.x - start.x, end.y - start.y, end.z - start.z);
	const double dist = sqrt(pow(origin.x, 2) + pow(origin.y, 2) + pow(origin.z, 2));
	::gDisplaySpeed = dist * TICRATE;
}

/**
 * @brief Calculate and retrieve the current player speed.
 * 
 * @return Speed in map units.
 */
double HU_GetPlayerSpeed()
{
	return ::gDisplaySpeed;
}
