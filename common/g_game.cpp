// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//   Game functions that are common between the client and server.
// 
//-----------------------------------------------------------------------------

#include "g_game.h"

#include "cmdlib.h"
#include "c_dispatch.h"
#include "doomstat.h"

void G_PlayerRebornInventory(player_t& p);

extern const char* weaponnames[];

template <typename A, typename T>
void ArrayInit(A& dst, const T& val)
{
	for (size_t i = 0; i < ARRAY_LENGTH(dst); i++)
		dst[i] = val;
}

template <typename A1, typename A2>
void ArrayCopy(A1& dst, const A2& src)
{
	for (size_t i = 0; i < ARRAY_LENGTH(src); i++)
		dst[i] = src[i];
}

struct spawnInventory_t
{
	int health;
	int armorpoints;
	int armortype;
	weapontype_t readyweapon;
	bool weaponowned[NUMWEAPONS];
	int ammo[NUMAMMO];
	int maxammo[NUMAMMO];
	bool berserk;
	bool backpack;

	spawnInventory_t()
	    : health(100), armorpoints(0), armortype(0), readyweapon(NUMWEAPONS),
	      berserk(false), backpack(false)
	{
		ArrayInit(weaponowned, false);
		ArrayInit(ammo, 0);
		ArrayInit(maxammo, 0);
	}

	spawnInventory_t(const spawnInventory_t& other)
	    : health(other.health), armorpoints(other.armorpoints),
	      armortype(other.armortype), berserk(other.berserk), backpack(other.backpack)
	{
		ArrayCopy(weaponowned, other.weaponowned);
		ArrayCopy(ammo, other.ammo);
		ArrayCopy(maxammo, other.maxammo);
	}

	/**
	 * @brief Set to default settings.
	 */
	void setDefault()
	{
		health = deh.StartHealth;
		armorpoints = 0;
		armortype = 0;
		readyweapon = wp_pistol;
		ArrayInit(weaponowned, false);
		ArrayInit(ammo, 0);
		ammo[0] = deh.StartBullets;
		ArrayCopy(maxammo, ::maxammo);
		berserk = false;
		backpack = false;
	}
};

spawnInventory_t gSpawnInv;

CVAR_FUNC_IMPL(g_spawninv)
{
	// Split value into comma-separated tokens.
	const std::string& str = var.str();
	StringTokens tok = TokenizeString(str, ",");
	for (StringTokens::iterator it = tok.begin(); it != tok.end(); ++it)
	{
		TrimString(*it);
		std::string token = StdStringToLower(*it);
		if (token == "default")
		{
			// Set the default settings.
			::gSpawnInv.setDefault();
		}
	}
}

static void SpawninvHelp()
{
	Printf("Flags:\n");
	Printf("  info\n");
	Printf("    Show the current spawn inventory settings.\n");
	Printf("  default\n");
	Printf("    Reset to default settings.\n");
}

BEGIN_COMMAND(spawninv)
{
	static const char* ammonames[] = {"Bullets", "Shells", "Rockets", "Cells"};

	if (argc < 2)
	{
		SpawninvHelp();
		return;
	}

	if (!stricmp(argv[1], "info"))
	{
		// Information about our currently-set spawn inventory.
		Printf("g_spawninv: %s\n", g_spawninv.cstring());

		Printf("Health: %d\n", ::gSpawnInv.health);
		if (::gSpawnInv.armortype == 1)
			Printf("Green Armor: %d\n", ::gSpawnInv.armorpoints);
		else if (::gSpawnInv.armortype == 2)
			Printf("Blue Armor: %d\n", ::gSpawnInv.armorpoints);

		if (::gSpawnInv.readyweapon < 0 || ::gSpawnInv.readyweapon >= NUMWEAPONS)
			Printf("Ready Weapon: None\n");
		else
			Printf("Ready Weapon: %s\n", ::weaponnames[::gSpawnInv.readyweapon]);

		StringTokens weapons;
		for (size_t i = 0; i < ARRAY_LENGTH(::gSpawnInv.weaponowned); i++)
		{
			if (::gSpawnInv.weaponowned[i])
				weapons.push_back(::weaponnames[i]);
		}
		if (!weapons.empty())
			Printf("Weapons: %s\n", JoinStrings(weapons, ", ").c_str());
		else
			Printf("Weapons: None\n");

		for (size_t i = 0; i < NUMAMMO; i++)
		{
			Printf("%s: %d/%d\n", ammonames[i], ::gSpawnInv.ammo[i],
			       ::gSpawnInv.maxammo[i]);
		}

		StringTokens other;
		if (::gSpawnInv.berserk)
			other.push_back("Berserk");
		if (::gSpawnInv.backpack)
			other.push_back("Backpack");
		if (!other.empty())
			Printf("Other: %s\n", JoinStrings(other, ", ").c_str());

		return;
	}

	SpawninvHelp();
}
END_COMMAND(spawninv)

void G_PlayerRebornInventory(player_t& p)
{

}