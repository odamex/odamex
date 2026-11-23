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
//   Bookkeeping of the loadout players carry through a respawn.
//
//-----------------------------------------------------------------------------

#pragma once
#include <array>
#include <unordered_map>

#include "doomdef.h"

class player_t;

/**
 * @brief The weapons and ammo a player had when they asked to respawn.
 */
struct KeptLoadout_s
{
	std::array<bool, NUMWEAPONS> weaponowned;
	std::array<int, NUMAMMO> ammo;
	std::array<int, NUMAMMO> maxammo;
	bool backpack;
	weapontype_t readyweapon;

	// Still waiting to be handed back to the player.
	bool pending;

	// What the player's last reborn did with it.
	bool kept;

	KeptLoadout_s()
	    : backpack(false), readyweapon(wp_none), pending(false), kept(false)
	{
		weaponowned.fill(false);
		ammo.fill(0);
		maxammo.fill(0);
	}
};

/**
 * @brief A singleton class that remembers the loadout each player is owed back
 *        after they respawn, and what their last respawn did with it.
 *
 * The loadout has to be taken when the player asks to respawn rather than read
 * off them during the reborn, because the reborn is where it gets wiped.
 *
 * Both client and server keep one of these managers.
 * 
 * The server stashes when it grants the respawn and replicates the verdict
 * with the spawner.
 * 
 * The client stashes from that verdict, so prediction and spying rebuild the
 * same loadout the server did without having to guess at it.
 */
class KeepWeaponsManager
{
public:
	KeepWeaponsManager();
	~KeepWeaponsManager();

	/**
	 * @brief Gets the only instance of this singleton class.
	 *
	 * @return A reference to the only allowable KeepWeaponsManager object.
	 */
	static KeepWeaponsManager& getInstance();

	/**
	 * @brief Stashes what a player is carrying to be handed back on respawn.
	 *
	 * @param playerid Player ID of the player who is about to be reborn.
	 * @param player Player to take the loadout from.
	 */
	void setLoadout(const int playerid, const player_t& player);

	/**
	 * @brief Hands a stashed loadout back on top of the spawn inventory.
	 *
	 * Call it from the reborn, after the spawn inventory has been given out -
	 * owned weapons are unioned onto it so the spawn weapons are never lost.
	 * 
	 * The stash is spent either way, and the verdict is recorded for
	 * replication.
	 *
	 * @param player Player being reborn.
	 * @return True if anything was handed back.
	 */
	bool restoreLoadout(player_t& player);

	/**
	 * @brief Whether a player's last reborn handed their loadout back.
	 *
	 * This is what the server puts in the spawn message, so the client's reborn
	 * reaches the same answer instead of inferring one from what it last saw.
	 *
	 * @param playerid Player ID to look up.
	 * @return True if the last reborn kept the loadout.
	 */
	bool keptLoadout(const int playerid) const;

	/**
	 * @brief Forgets a player's loadout, when they disconnect or spectate.
	 *
	 * Player IDs are handed back out to whoever connects next, so a departing
	 * player has to be forgotten or the next holder of the ID inherits it.
	 *
	 * @param playerid Player ID to forget.
	 */
	void eraseLoadout(const int playerid);

	/**
	 * @brief Forgets everyone's loadout, after a map change/restart/new round.
	 */
	void clearLoadouts();

private:
	// Bookkeeping dictionary of stashed loadouts per player.
	std::unordered_map<int, KeptLoadout_s> loadoutPlayerDict;
};
