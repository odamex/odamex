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

/**
 * @brief The place a player was standing when they were killed.
 */
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

/**
 * @brief A singleton class that remembers where each player last died on the
 *        current level.
 *
 * The spot has to be recorded at the moment of death rather than read off the
 * corpse later, because the corpse can be destroyed before the player gets
 * around to respawning.
 */
class DeathSpotManager
{
public:
	DeathSpotManager();
	~DeathSpotManager();

	/**
	 * @brief Gets the only instance of this singleton class.
	 *
	 * @return A reference to the only allowable DeathSpotManager object.
	 */
	static DeathSpotManager& getInstance();

	/**
	 * @brief Records where a player fell.  Replaces any spot they already had.
	 *
	 * @param playerid Player ID of the player who died.
	 * @param x Map x coordinate of the death.
	 * @param y Map y coordinate of the death.
	 * @param z Map z coordinate of the death.
	 * @param angle Angle the player was facing when they died.
	 */
	void setDeathSpot(const int playerid, const fixed_t x, const fixed_t y,
	                  const fixed_t z, const angle_t angle);

	/**
	 * @brief Whether we have somewhere to send this player back to.
	 *
	 * @param playerid Player ID to look up.
	 * @return True if the player has a death spot on this level.
	 */
	bool hasDeathSpot(const int playerid) const;

	/**
	 * @brief Gets the spot a player last died on.
	 *
	 * @param playerid Player ID to look up.
	 * @return The player's death spot, or an empty one if they have none.
	 */
	const DeathSpot_s& getDeathSpot(const int playerid) const;

	/**
	 * @brief Forgets where a player died.
	 *
	 * @param playerid Player ID to forget.
	 */
	void eraseDeathSpot(const int playerid);

	/**
	 * @brief Forgets everyone's death spot, after a map change/restart/new round.
	 */
	void clearDeathSpots();

private:
	// Bookkeeping dictionary of death spots per player.
	std::unordered_map<int, DeathSpot_s> deathSpotPlayerDict;

	// Empty death spot struct for returning when invalid.
	DeathSpot_s emptySpot;
};

/**
 * @brief Whether the spot a player died on can be respawned onto right now, and
 *        if not, what is stopping it - the HUD tells the player which it is.
 */
enum deathSpotBlock_t
{
	DEATHSPOT_NOSPOT,           // The feature is off, or there is nowhere to go back to.
	DEATHSPOT_CLEAR,            // Free to respawn on, once anything stompable is stomped.
	DEATHSPOT_BLOCKED_DEADLY,   // The floor there kills on contact.
	DEATHSPOT_BLOCKED_NOROOM,   // A crusher, door or floor has left no room to stand.
	DEATHSPOT_BLOCKED_PLAYER,   // A player we are not allowed to telefrag is standing there.
	DEATHSPOT_BLOCKED_OBSTACLE, // Something that will never move is standing there.
};

/**
 * @brief Whether a verdict means the player cannot be put back where they fell.
 *
 * @param block Verdict from G_CheckDeathSpot.
 * @return True for every blocked reason, false for clear or no spot.
 */
inline bool G_IsDeathSpotBlocked(const deathSpotBlock_t block)
{
	return block != DEATHSPOT_NOSPOT && block != DEATHSPOT_CLEAR;
}

/**
 * @brief What the rules say to do about one thing standing on a death spot.
 */
enum blockerAction_t
{
	BLOCKER_IGNORE, // Not in the way, or allowed to share the spot.
	BLOCKER_STOMP,  // Telefrag it and take the spot.

	// Nothing we can do - the spawn has to wait. Whether that wait ever ends
	// is decided by the caller from what the thing is.
	BLOCKER_BLOCKS,
};

/**
 * @brief Whether the floor of a sector kills anything that lands on it outright.
 *
 * Both map formats have to be asked, because they store it in different places:
 * Doom/Boom keeps the kill in the sector special, while ZDoom uses either
 * Damage_InstantDeath as a sector special, or damageamount for Sector_SetDamage.
 *
 * @param sec Sector the spot sits in.
 * @return True if respawning here would just repeat the same death.
 */
bool G_IsInstantDeathSector(const sector_t& sec);

/**
 * @brief Decides what a single thing sitting on a death spot means for the spawn.
 *
 * @param thing Thing found overlapping the spot.
 * @param player Player who wants their spot back.
 * @return Whether to ignore it, telefrag it, or give up on the spot.
 */
blockerAction_t G_ClassifyDeathSpotBlocker(const AActor& thing, const player_t& player);

/**
 * @brief Looks at what is standing on the spot a player died on and decides
 *        whether they can be put back there.
 *
 * Runs the same on the client, so the HUD can warn about a blocked spot without
 * asking the server.
 *
 * @param player Player who wants to go back to where they fell.
 * @return Whether the spot is usable, blocked, or not there at all.
 */
deathSpotBlock_t G_CheckDeathSpot(const player_t& player);

/**
 * @brief Telefrags everything on a death spot that the rules allow us to move,
 *        so the freshly spawned player has the spot to themselves.
 *
 * Call it only once the player has been spawned - the newly spawned player is
 * the source of the damage, so the telefrag is credited to whoever took the
 * spot. The spot is passed in rather than looked up because spawning erases it.
 *
 * @param player Player who has just respawned on their death spot.
 * @param spot Spot they were put back on.
 */
void G_StompDeathSpot(player_t& player, const DeathSpot_s& spot);
