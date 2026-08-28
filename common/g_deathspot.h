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

class AActor;
class player_t;
struct sector_t;

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

/// <summary>
/// Whether the spot a player died on can be respawned onto right now, and if
/// not, what is stopping it - the HUD tells the player which it is.
/// </summary>
enum deathSpotBlock_t
{
	/// <summary>The feature is off, or there is nowhere to go back to.</summary>
	DEATHSPOT_NOSPOT,

	/// <summary>Free to respawn on, once anything stompable is stomped.</summary>
	DEATHSPOT_CLEAR,

	/// <summary>The floor there kills on contact.</summary>
	DEATHSPOT_BLOCKED_DEADLY,

	/// <summary>A crusher, door or floor has left no room to stand.</summary>
	DEATHSPOT_BLOCKED_NOROOM,

	/// <summary>A player we are not allowed to telefrag is standing there.</summary>
	DEATHSPOT_BLOCKED_PLAYER,

	/// <summary>Something that will never move is standing there.</summary>
	DEATHSPOT_BLOCKED_OBSTACLE,
};

/// <summary>
/// Whether a verdict means the player cannot be put back where they fell.
/// </summary>
/// <param name="block">Verdict from G_CheckDeathSpot.</param>
/// <returns>True for every blocked reason, false for clear or no spot.</returns>
inline bool G_IsDeathSpotBlocked(const deathSpotBlock_t block)
{
	return block != DEATHSPOT_NOSPOT && block != DEATHSPOT_CLEAR;
}

/// <summary>
/// What the rules say to do about one thing standing on a death spot.
/// </summary>
enum blockerAction_t
{
	/// <summary>Not in the way, or allowed to share the spot.</summary>
	BLOCKER_IGNORE,

	/// <summary>Telefrag it and take the spot.</summary>
	BLOCKER_STOMP,

	/// <summary>
	/// Nothing we can do - the spawn has to wait. Whether that wait ever ends
	/// is decided by the caller from what the thing is.
	/// </summary>
	BLOCKER_BLOCKS,
};

/// <summary>
/// Whether the floor of a sector kills anything that lands on it outright.
///
/// Both map formats have to be asked, because they store it in different
/// places: Doom/Boom keeps the kill in the sector special, while ZDoom uses
/// either Damage_InstantDeath as a sector special, or damageamount for
/// Sector_SetDamage.
/// </summary>
/// <param name="sec">Sector the spot sits in.</param>
/// <returns>True if respawning here would just repeat the same death.</returns>
bool G_IsInstantDeathSector(const sector_t& sec);

/// <summary>
/// Decides what a single thing sitting on a death spot means for the spawn.
/// </summary>
/// <param name="thing">Thing found overlapping the spot.</param>
/// <param name="player">Player who wants their spot back.</param>
/// <returns>Whether to ignore it, telefrag it, or give up on the spot.</returns>
blockerAction_t G_ClassifyDeathSpotBlocker(const AActor& thing, const player_t& player);

/// <summary>
/// Looks at what is standing on the spot a player died on and decides whether
/// they can be put back there.
///
/// Runs the same on the client, so the HUD can warn about a blocked spot
/// without asking the server.
/// </summary>
/// <param name="player">Player who wants to go back to where they fell.</param>
/// <returns>Whether the spot is usable, blocked, or not there at all.</returns>
deathSpotBlock_t G_CheckDeathSpot(const player_t& player);

/// <summary>
/// Telefrags everything on a death spot that the rules allow us to move, so the
/// freshly spawned player has the spot to themselves.
///
/// Call it only once the player has been spawned - the newly spawned player is
/// the source of the damage, so the telefrag is credited to whoever took the
/// spot.
/// The spot is passed in rather than looked up because spawning erases it.
/// </summary>
/// <param name="player">Player who has just respawned on their death spot.</param>
/// <param name="spot">Spot they were put back on.</param>
void G_StompDeathSpot(player_t& player, const DeathSpot_s& spot);
