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
//   Bookkeeping of where players last died on the current level.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "g_deathspot.h"

DeathSpotManager& DeathSpotManager::getInstance()
{
	static DeathSpotManager instance;
	return instance;
}

DeathSpotManager::DeathSpotManager()
{
	emptySpot = DeathSpot_s();
}

DeathSpotManager::~DeathSpotManager()
{
	reset();
}

void DeathSpotManager::reset()
{
	deathSpotPlayerDict.clear();
}

void DeathSpotManager::setDeathSpot(const int playerid, const fixed_t x, const fixed_t y,
                                    const fixed_t z, const angle_t angle)
{
	deathSpotPlayerDict[playerid] = DeathSpot_s(x, y, z, angle);
}

bool DeathSpotManager::hasDeathSpot(const int playerid) const
{
	return deathSpotPlayerDict.find(playerid) != deathSpotPlayerDict.end();
}

const DeathSpot_s& DeathSpotManager::getDeathSpot(const int playerid) const
{
	const auto it = deathSpotPlayerDict.find(playerid);

	if (it == deathSpotPlayerDict.end())
	{
		return emptySpot;
	}

	return it->second;
}

void DeathSpotManager::eraseDeathSpot(const int playerid)
{
	deathSpotPlayerDict.erase(playerid);
}

void DeathSpotManager::clearDeathSpots()
{
	deathSpotPlayerDict.clear();
}

VERSION_CONTROL (g_deathspot_cpp, "$Id$")
