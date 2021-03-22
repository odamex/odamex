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

#include "c_dispatch.h"
#include "cmdlib.h"
#include "doomstat.h"

void G_PlayerRebornInventory(player_t& p);
void G_SetupDefaultInv();

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

static char WeaponTypeChar(int type)
{
	if (type >= wp_fist && type <= wp_bfg)
		return '1' + type;
	else if (type == wp_chainsaw)
		return 'A';
	else if (type == wp_supershotgun)
		return 'C';

	return ' ';
}

struct spawnInventory_t
{
	bool isdefault;
	int health;
	int armorpoints;
	int armortype;
	weapontype_t readyweapon;
	bool weaponowned[NUMWEAPONS];
	int ammo[NUMAMMO];
	bool berserk;
	bool backpack;

	spawnInventory_t()
	    : isdefault(false), health(100), armorpoints(0), armortype(0),
	      readyweapon(NUMWEAPONS), berserk(false), backpack(false)
	{
		ArrayInit(weaponowned, false);
		ArrayInit(ammo, 0);
	}

	spawnInventory_t(const spawnInventory_t& other)
	    : isdefault(other.isdefault), health(other.health),
	      armorpoints(other.armorpoints), armortype(other.armortype),
	      berserk(other.berserk), backpack(other.backpack)
	{
		ArrayCopy(weaponowned, other.weaponowned);
		ArrayCopy(ammo, other.ammo);
	}
};

spawnInventory_t gDefaultInv;
spawnInventory_t gSpawnInv;

/**
 * @brief Serialize the spawn inventory to a string.
 */
static std::string SpawnInvSerialize(const spawnInventory_t& inv)
{
	StringTokens params;
	std::string buf;

	StrFormat(buf, "health:%d", inv.health);
	params.push_back(buf);

	if (inv.armortype > 0 && inv.armortype <= 2 && inv.armorpoints > 0)
	{
		if (inv.armortype == 1)
			StrFormat(buf, "armor1:%d", inv.health);
		else if (inv.armortype == 2)
			StrFormat(buf, "armor2:%d", inv.health);

		params.push_back(buf);
	}

	if (inv.readyweapon != NUMWEAPONS)
	{
		StrFormat(buf, "rweapon:%c", WeaponTypeChar(inv.readyweapon));
		params.push_back(buf);
	}

	std::string weapons;
	for (size_t i = 0; i < ARRAY_LENGTH(inv.weaponowned); i++)
	{
		if (inv.weaponowned[i])
			weapons += WeaponTypeChar(i);
	}
	if (!weapons.empty())
	{
		StrFormat(buf, "weapons:%s", weapons.c_str());
		params.push_back(buf);
	}

	if (inv.ammo[0] > 0)
	{
		StrFormat(buf, "bullets:%d", inv.ammo[0]);
		params.push_back(buf);
	}

	if (inv.ammo[1] > 0)
	{
		StrFormat(buf, "shells:%d", inv.ammo[1]);
		params.push_back(buf);
	}

	if (inv.ammo[2] > 0)
	{
		StrFormat(buf, "rockets:%d", inv.ammo[2]);
		params.push_back(buf);
	}

	if (inv.ammo[3] > 0)
	{
		StrFormat(buf, "cells:%d", inv.ammo[3]);
		params.push_back(buf);
	}

	if (inv.berserk)
		params.push_back("berserk");
	if (inv.backpack)
		params.push_back("backpack");

	return JoinStrings(params, " ");
}

CVAR_FUNC_IMPL(g_spawninv)
{
	G_SetupDefaultInv();

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
			::gSpawnInv = ::gDefaultInv;
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
		Printf("serialized: %s\n", SpawnInvSerialize(::gSpawnInv).c_str());

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
			Printf("%s: %d\n", ammonames[i], ::gSpawnInv.ammo[i]);
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

/**
 * @brief Assign settings for a "default" cleared inventory.
 */
void G_SetupDefaultInv()
{
	::gDefaultInv.isdefault = false;
	::gDefaultInv.health = deh.StartHealth;
	::gDefaultInv.armorpoints = 0;
	::gDefaultInv.armortype = 0;
	::gDefaultInv.readyweapon = wp_pistol;
	ArrayInit(::gDefaultInv.weaponowned, false);
	::gDefaultInv.weaponowned[wp_fist] = true;
	::gDefaultInv.weaponowned[wp_pistol] = true;
	ArrayInit(::gDefaultInv.ammo, 0);
	::gDefaultInv.ammo[0] = deh.StartBullets;
	::gDefaultInv.berserk = false;
	::gDefaultInv.backpack = false;
}

void G_PlayerRebornInventory(player_t& p)
{
}