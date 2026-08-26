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

#pragma once
#include <unordered_map>

#include "m_fixed.h"
#include "tables.h"

/// <summary>
/// The place a player was standing when they were killed.
/// </summary>
struct DeathSpot_s
{
	fixed_t x;
	fixed_t y;
	fixed_t z;
	angle_t angle;

	DeathSpot_s() : x(0), y(0), z(0), angle(0) { }
	DeathSpot_s(fixed_t X, fixed_t Y, fixed_t Z, angle_t Angle)
	    : x(X), y(Y), z(Z), angle(Angle)
	{
	}
};

/// <summary>
/// A singleton class that remembers where each player last died on the current
/// level.
///
/// The spot has to be recorded at the moment of death rather than read off the
/// corpse later, because the corpse can be destroyed before the player gets
/// around to respawning.
/// </summary>
class DeathSpotManager
{
public:
	DeathSpotManager();
	~DeathSpotManager();

	/// <summary>
	/// Gets the only instance of this singleton class.
	/// </summary>
	/// <returns>A reference to the only allowable DeathSpotManager object.</returns>
	static DeathSpotManager& getInstance();

	/// <summary>
	/// Resets the state of the manager.
	/// </summary>
	void reset();

	/// <summary>
	/// Records where a player fell.
	/// Replaces any spot they already had.
	/// </summary>
	/// <param name="playerid">Player ID of the player who died.</param>
	/// <param name="x">Map x coordinate of the death.</param>
	/// <param name="y">Map y coordinate of the death.</param>
	/// <param name="z">Map z coordinate of the death.</param>
	/// <param name="angle">Angle the player was facing when they died.</param>
	void setDeathSpot(const int playerid, const fixed_t x, const fixed_t y,
	                  const fixed_t z, const angle_t angle);

	/// <summary>
	/// Whether we have somewhere to send this player back to.
	/// </summary>
	/// <param name="playerid">Player ID to look up.</param>
	/// <returns>True if the player has a death spot on this level.</returns>
	bool hasDeathSpot(const int playerid) const;

	/// <summary>
	/// Gets the spot a player last died on.
	/// </summary>
	/// <param name="playerid">Player ID to look up.</param>
	/// <returns>The player's death spot, or an empty one if they have none.</returns>
	const DeathSpot_s& getDeathSpot(const int playerid) const;

	/// <summary>
	/// Forgets where a player died.
	/// </summary>
	/// <param name="playerid">Player ID to forget.</param>
	void eraseDeathSpot(const int playerid);

	/// <summary>
	/// Forgets everyone's death spot, after a map change/restart/new round.
	/// </summary>
	void clearDeathSpots();

private:
	/// <summary>
	/// Bookkeeping dictionary of death spots per player.
	/// </summary>
	std::unordered_map<int, DeathSpot_s> deathSpotPlayerDict;

	/// <summary>
	/// Empty death spot struct for returning when invalid.
	/// </summary>
	DeathSpot_s emptySpot;
};
