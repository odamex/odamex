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

#include "odamex.h"

#include "g_keepweapons.h"

#include "d_player.h"

KeepWeaponsManager& KeepWeaponsManager::getInstance()
{
	static KeepWeaponsManager instance;
	return instance;
}

KeepWeaponsManager::KeepWeaponsManager() = default;

KeepWeaponsManager::~KeepWeaponsManager()
{
	clearLoadouts();
}

void KeepWeaponsManager::setLoadout(const int playerid, const player_t& player)
{
	KeptLoadout_s& loadout = loadoutPlayerDict[playerid];

	loadout.weaponowned = player.weaponowned;
	loadout.ammo = player.ammo;
	loadout.maxammo = player.maxammo;
	loadout.backpack = player.backpack;
	loadout.readyweapon = player.readyweapon;
	loadout.pending = true;
}

bool KeepWeaponsManager::restoreLoadout(player_t& player)
{
	const auto it = loadoutPlayerDict.find(player.id);

	if (it == loadoutPlayerDict.end())
	{
		return false;
	}

	KeptLoadout_s& loadout = it->second;

	// Spectating costs you the loadout even if the respawn was already granted.
	const bool keep = loadout.pending and not player.spectator;

	// Spent either way, but the verdict stays for the spawn message to carry.
	loadout.pending = false;
	loadout.kept = keep;

	if (not keep)
	{
		return false;
	}

	// Union the weapons so whatever the spawn inventory just gave is kept too.
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		player.weaponowned[i] = player.weaponowned[i] or loadout.weaponowned[i];
	}

	player.ammo = loadout.ammo;

	// Only take the old maxammo if it is the one carrying the backpack
	// doubling, otherwise the spawn inventory's backpack would be undone.
	if (loadout.backpack)
	{
		player.backpack = true;
		player.maxammo = loadout.maxammo;
	}

	if (loadout.readyweapon >= wp_fist and loadout.readyweapon < wp_none and
	    player.weaponowned[loadout.readyweapon])
	{
		player.readyweapon = player.pendingweapon = loadout.readyweapon;
	}

	return true;
}

bool KeepWeaponsManager::keptLoadout(const int playerid) const
{
	const auto it = loadoutPlayerDict.find(playerid);

	return it != loadoutPlayerDict.end() and it->second.kept;
}

void KeepWeaponsManager::eraseLoadout(const int playerid)
{
	loadoutPlayerDict.erase(playerid);
}

void KeepWeaponsManager::clearLoadouts()
{
	loadoutPlayerDict.clear();
}
