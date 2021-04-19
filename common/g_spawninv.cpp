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
//   Spawn inventory.
//
//-----------------------------------------------------------------------------

#include "g_spawninv.h"

#include <assert.h>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "doomstat.h"

extern const char* weaponnames[];

/**
 * @brief Container for a spawn inventory.
 */
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
	      readyweapon(other.readyweapon), berserk(other.berserk), backpack(other.backpack)
	{
		ArrayCopy(weaponowned, other.weaponowned);
		ArrayCopy(ammo, other.ammo);
	}
};

// Berserk time that prevents showing any red.  If you want to show a teensy
// bit of red, you're going to need to send the player's powers on map change.
const int INV_BERSERK_TIME = 64 * 12;

/**
 * @brief Convert a string form of a boolean to an actual boolean.
 *
 * @detail Working with C strings is _much_ faster here.
 *
 * @param check String to check.
 * @return True if string is a boolean.
 */
static const bool StrBoolean(const char* check)
{
	size_t len = strlen(check);
	if (len == 0)
		return false;

	if (len == 1 && check[0] == '1')
		return true;

	if (check[0] == 'T' || check[0] == 't')
		return true;

	if (check[0] == 'Y' || check[0] == 'y')
		return true;

	if (check[0] == 'O' || check[0] == 'o')
	{
		if (len >= 2 && (check[1] == 'N' || check[1] == 'n'))
			return true;
	}

	return false;
}

/**
 * @brief Turn a weapon type index into its character representation.
 */
static char WeaponTypeToChar(const weapontype_t type)
{
	if (type >= wp_fist && type <= wp_bfg)
		return '1' + type;
	else if (type == wp_chainsaw)
		return 'a';
	else if (type == wp_supershotgun)
		return 'c';
	else if (type == NUMWEAPONS)
		return 'x';

	return '?';
}

/**
 * @brief Turn a weapon character into its type index.
 */
static int WeaponTypeFromChar(const char ch)
{
	if (ch >= '1' && ch <= '7')
		return ch - '1';
	else if (ch == 'A' || ch == 'a')
		return wp_chainsaw;
	else if (ch == 'C' || ch == 'c')
		return wp_supershotgun;
	else if (ch == 'X' || ch == 'x')
		return NUMWEAPONS;

	return MININT; // best chance of loud and obvious crash
}

/**
 * @brief Turn Health into a string.
 */
static std::string InvHealthStr(const spawnInventory_t& inv)
{
	std::string rvo;
	StrFormat(rvo, "%d", inv.health);
	return rvo;
}

/**
 * @brief Turn armor points into a string.
 */
static std::string InvArmorPointsStr(const spawnInventory_t& inv)
{
	std::string rvo;
	StrFormat(rvo, "%d", inv.armorpoints);
	return rvo;
}

/**
 * @brief Turn ready weapon into a string.
 */
static std::string InvReadyWeaponStr(const spawnInventory_t& inv)
{
	std::string rvo;
	StrFormat(rvo, "%c", WeaponTypeToChar(inv.readyweapon));
	return rvo;
}

/**
 * @brief Weapons to string.
 */
static std::string InvWeaponsStr(const spawnInventory_t& inv)
{
	std::string rvo;
	rvo.reserve(ARRAY_LENGTH(inv.weaponowned));
	for (size_t i = 0; i < ARRAY_LENGTH(inv.weaponowned); i++)
	{
		if (!inv.weaponowned[i])
			continue;

		rvo += WeaponTypeToChar(static_cast<weapontype_t>(i));
	}
	return rvo;
}

/**
 * @brief Ammo index to string.
 */
static std::string InvAmmoStr(const spawnInventory_t& inv, const ammotype_t type)
{
	if (type < am_clip || type >= NUMAMMO)
	{
		return "";
	}
	std::string rvo;
	StrFormat(rvo, "%d", inv.ammo[type]);
	return rvo;
}

/**
 * @brief Berserk status to string.
 */
static std::string InvBerserkStr(const spawnInventory_t& inv)
{
	return inv.berserk ? "Yes" : "No";
}

/**
 * @brief Backpack status to a string.
 */
static std::string InvBackpackStr(const spawnInventory_t& inv)
{
	return inv.backpack ? "Yes" : "No";
}

/**
 * @brief Set health from a string.
 */
static void InvSetHealth(spawnInventory_t& inv, const std::string& value)
{
	inv.health = MAX(1, atoi(value.c_str()));
	inv.isdefault = false;
}

/**
 * @brief Set armor of a given type from a string.
 */
static void InvSetArmor(spawnInventory_t& inv, const int type, const std::string& value)
{
	inv.armortype = type;
	inv.armorpoints = MAX(0, atoi(value.c_str()));
	inv.isdefault = false;
}

/**
 * @brief Set the ready weapon from a string.
 */
static bool InvSetReadyWeapon(spawnInventory_t& inv, const std::string& value)
{
	if (value.empty())
		return false;

	int weap = WeaponTypeFromChar(value.at(0));
	if (weap == MININT)
		return false;

	inv.readyweapon = static_cast<weapontype_t>(weap);
	inv.isdefault = false;

	return true;
}

/**
 * @brief Set the weapon inventory from a string.
 */
static bool InvSetWeapons(spawnInventory_t& inv, const std::string& value)
{
	// Check our input string first.
	bool newowned[NUMWEAPONS];
	ArrayInit(newowned, false);

	for (std::string::const_iterator it = value.begin(); it != value.end(); ++it)
	{
		int owned = WeaponTypeFromChar(*it);
		if (owned == MININT)
			return false;

		newowned[owned] = true;
	}

	// Commit our new weapons.
	ArrayCopy(inv.weaponowned, newowned);
	inv.isdefault = false;

	return true;
}

/**
 * @brief Set the ammo count of a given type from a string.
 */
static void InvSetAmmo(spawnInventory_t& inv, const ammotype_t type,
                       const std::string& value)
{
	if (type < am_clip || type >= NUMAMMO)
	{
		return;
	}
	inv.ammo[type] = MAX(0, atoi(value.c_str()));
	inv.isdefault = false;
}

static void InvSetBerserk(spawnInventory_t& inv, const std::string& value)
{
	inv.berserk = StrBoolean(value.c_str());
	inv.isdefault = false;
}

static void InvSetBackpack(spawnInventory_t& inv, const std::string& value)
{
	inv.backpack = StrBoolean(value.c_str());
	inv.isdefault = false;
}

/**
 * @brief A reasonable "default" inventory.
 */
spawnInventory_t gDefaultInv;

/**
 * @brief The currently-loaded spawn inventory.
 */
spawnInventory_t gSpawnInv;

/**
 * @brief Serialize the spawn inventory to a string.
 *
 * @detail If you are using this, it is assumed that the caller wants to
 *         start from nothing.
 */
static std::string SpawnInvSerialize(const spawnInventory_t& inv)
{
	StringTokens params;
	std::string buf;

	StrFormat(buf, "health:%s", InvHealthStr(inv).c_str());
	params.push_back(buf);

	if (inv.armortype > 0 && inv.armortype <= 2 && inv.armorpoints > 0)
	{
		if (inv.armortype == 1)
			StrFormat(buf, "armor1:%s", InvArmorPointsStr(inv).c_str());
		else if (inv.armortype == 2)
			StrFormat(buf, "armor2:%s", InvArmorPointsStr(inv).c_str());

		params.push_back(buf);
	}

	if (inv.readyweapon != NUMWEAPONS)
	{
		StrFormat(buf, "rweapon:%s", InvReadyWeaponStr(inv).c_str());
		params.push_back(buf);
	}

	if (!InvWeaponsStr(inv).empty())
	{
		StrFormat(buf, "weapons:%s", InvWeaponsStr(inv).c_str());
		params.push_back(buf);
	}

	if (inv.ammo[am_clip] > 0)
	{
		StrFormat(buf, "bullets:%s", InvAmmoStr(inv, am_clip).c_str());
		params.push_back(buf);
	}

	if (inv.ammo[am_shell] > 0)
	{
		StrFormat(buf, "shells:%s", InvAmmoStr(inv, am_shell).c_str());
		params.push_back(buf);
	}

	if (inv.ammo[am_misl] > 0)
	{
		StrFormat(buf, "rockets:%s", InvAmmoStr(inv, am_misl).c_str());
		params.push_back(buf);
	}

	if (inv.ammo[am_cell] > 0)
	{
		StrFormat(buf, "cells:%s", InvAmmoStr(inv, am_cell).c_str());
		params.push_back(buf);
	}

	if (inv.berserk)
		params.push_back("berserk");
	if (inv.backpack)
		params.push_back("backpack");

	return JoinStrings(params, " ");
}

/**
 * @brief Assign settings for a "default" cleared inventory.
 */
static void SetupDefaultInv()
{
	::gDefaultInv.isdefault = true;
	::gDefaultInv.health = deh.StartHealth; // [RH] Used to be MAXHEALTH
	::gDefaultInv.armorpoints = 0;
	::gDefaultInv.armortype = 0;
	::gDefaultInv.readyweapon = wp_pistol;
	ArrayInit(::gDefaultInv.weaponowned, false);
	::gDefaultInv.weaponowned[wp_fist] = true;
	::gDefaultInv.weaponowned[wp_pistol] = true;
	ArrayInit(::gDefaultInv.ammo, 0);
	::gDefaultInv.ammo[am_clip] = deh.StartBullets; // [RH] Used to be 50
	::gDefaultInv.berserk = false;
	::gDefaultInv.backpack = false;
}

CVAR_FUNC_IMPL(g_spawninv)
{
	SetupDefaultInv();

	spawnInventory_t inv;

	// Split value into comma-separated tokens.
	const std::string& str = var.str();
	StringTokens tok = TokenizeString(str, " ");
	for (StringTokens::iterator it = tok.begin(); it != tok.end(); ++it)
	{
		TrimString(*it);
		std::string token = StdStringToLower(*it);
		if (token == "default")
		{
			// Set the default settings.
			inv = ::gDefaultInv;
		}
		else if (token == "clear")
		{
			// Clear the inventory if we've parsed a token already, otherwise
			// the token does nothing except state intent.
			if (it != tok.begin())
				inv = spawnInventory_t();
		}
		else if (token == "berserk")
		{
			InvSetBerserk(inv, "Y");
		}
		else if (token == "backpack")
		{
			InvSetBackpack(inv, "Y");
		}
		else
		{
			StringTokens param = TokenizeString(token, ":");
			if (param.size() != 2)
			{
				Printf(PRINT_WARNING,
				       "g_spawninv: Unknown parameter \"%s\", falling back to default "
				       "inventory.\n",
				       token.c_str());
				::gSpawnInv = ::gDefaultInv;
				return;
			}

			std::string key = StdStringToLower(param.at(0));
			if (key.empty())
			{
				Printf(PRINT_WARNING,
				       "g_spawninv: Missing key for parameter \"%s\", falling back to "
				       "default inventory.\n",
				       token.c_str());
				::gSpawnInv = ::gDefaultInv;
				return;
			}

			std::string value = StdStringToLower(param.at(1));
			if (value.empty())
			{
				Printf(PRINT_WARNING,
				       "g_spawninv: Missing value for parameter \"%s\", falling back to "
				       "default inventory.\n",
				       token.c_str());
				::gSpawnInv = ::gDefaultInv;
				return;
			}

			if (key == "health")
			{
				InvSetHealth(inv, value);
			}
			else if (key == "armor1")
			{
				InvSetArmor(inv, 1, value);
			}
			else if (key == "armor2")
			{
				InvSetArmor(inv, 2, value);
			}
			else if (key == "rweapon")
			{
				if (!InvSetReadyWeapon(inv, value))
				{
					Printf(PRINT_WARNING,
					       "g_spawninv: Unknown value for parameter \"%s\", falling back "
					       "to default inventory.\n",
					       token.c_str());
					::gSpawnInv = ::gDefaultInv;
					return;
				}
			}
			else if (key == "weapons")
			{
				if (!InvSetWeapons(inv, value))
				{
					Printf(PRINT_WARNING,
					       "g_spawninv: Unknown value for parameter \"%s\", falling back "
					       "to default inventory.\n",
					       token.c_str());
					::gSpawnInv = ::gDefaultInv;
					return;
				}
			}
			else if (key == "bullets")
			{
				InvSetAmmo(inv, am_clip, value);
			}
			else if (key == "shells")
			{
				InvSetAmmo(inv, am_shell, value);
			}
			else if (key == "rockets")
			{
				InvSetAmmo(inv, am_misl, value);
			}
			else if (key == "cells")
			{
				InvSetAmmo(inv, am_cell, value);
			}
			else
			{
				Printf(PRINT_WARNING,
				       "g_spawninv: Unknown parameter \"%s\", falling back to default "
				       "inventory.\n",
				       token.c_str());
				::gSpawnInv = ::gDefaultInv;
				return;
			}
		}
	}

	// Commit our new inventory settings.
	::gSpawnInv = inv;
}

static void SpawninvHelp()
{
	Printf("Commands:\n");
	Printf("  info\n");
	Printf("    Show the current spawn inventory settings.\n");
	Printf("  default\n");
	Printf("    Reset to default settings.\n");
	Printf("Inventory Commands:");
	Printf("  health <HEALTH>\n");
	Printf("  armor1 <POINTS>\n");
	Printf("  armor2 <POINTS>\n");
	Printf("  rweapon <WEAPON>\n");
	Printf("  weapons <WEAPS>\n");
	Printf("  bullets <BULLETS>\n");
	Printf("  shells <SHELLS>\n");
	Printf("  rockets <ROCKETS>\n");
	Printf("  cells <CELLS>\n");
	Printf("  berserk <Y/N>\n");
	Printf("  backpack <Y/N>\n");
	Printf("    Inventory commands show an explanation of their parameter if you omit "
	       "it.\n");
}

static void SpawninvCommandHelp(const char* cmd)
{
	if (!stricmp(cmd, "health"))
	{
		Printf("health <HEALTH>\n");
		Printf("  Set initial health to HEALTH.\n");
	}
	else if (!stricmp(cmd, "armor1"))
	{
		Printf("armor1 <POINTS>\n");
		Printf("  Set initial armor to POINTS points of green armor.\n");
	}
	else if (!stricmp(cmd, "armor2"))
	{
		Printf("armor2 <POINTS>\n");
		Printf("  Set initial armor to POINTS points of blue armor.\n");
	}
	else if (!stricmp(cmd, "rweapon"))
	{
		Printf("rweapon <WEAPON>\n");
		Printf("  Set starting weapon to WEAPON.\n");
		Printf("  Valid values are 1-7, A(Saw), and C(SSG).\n");
	}
	else if (!stricmp(cmd, "weapons"))
	{
		Printf("weapons <WEAPS>\n");
		Printf("  Set starting inventory of weapons to contain WEAPS.\n");
		Printf("  Valid values are 1-7, A(Saw), and C(SSG).\n");
		Printf(
		    "  Multiple weapons are passed as a single block of letters and numbers.\n");
		Printf("  (example: \"1A23\" gives you fist, saw, pistol, shotgun)\n");
	}
	else if (!stricmp(cmd, "bullets"))
	{
		Printf("bullets <BULLETS>\n");
		Printf("  Set initial bullet count to BULLETS.\n");
	}
	else if (!stricmp(cmd, "shells"))
	{
		Printf("shells <SHELLS>\n");
		Printf("  Set initial shell count to SHELLS.\n");
	}
	else if (!stricmp(cmd, "rockets"))
	{
		Printf("rockets <ROCKETS>\n");
		Printf("  Set initial rocket count to ROCKETS.\n");
	}
	else if (!stricmp(cmd, "cells"))
	{
		Printf("cells <CELLS>\n");
		Printf("  Set initial cell count to CELLS.\n");
	}
	else if (!stricmp(cmd, "berserk"))
	{
		Printf("berserk <Y/N>\n");
		Printf("  Give or take away berserk.\n");
	}
	else if (!stricmp(cmd, "backpack"))
	{
		Printf("backpack <Y/N>\n");
		Printf("  Give or take away backpack.\n");
		Printf("  Having a backpack doubles your max ammo and gives you a backpack "
		       "inventory item.\n");
		Printf("  It does not double \"spawninv\" ammo counts.\n");
	}
	else
	{
		Printf(PRINT_WARNING, "spawninv: Unknown subcommand \"%s\".", cmd);
	}
}

static void SpawninvCommand(const std::string& cmd, const std::string& param)
{
	if (iequals(cmd, "health"))
	{
		InvSetHealth(::gSpawnInv, param);
	}
	else if (iequals(cmd, "armor1"))
	{
		InvSetArmor(::gSpawnInv, 1, param);
	}
	else if (iequals(cmd, "armor2"))
	{
		InvSetArmor(::gSpawnInv, 2, param);
	}
	else if (iequals(cmd, "rweapon"))
	{
		InvSetReadyWeapon(::gSpawnInv, param);
	}
	else if (iequals(cmd, "weapons"))
	{
		InvSetWeapons(::gSpawnInv, param);
	}
	else if (iequals(cmd, "bullets"))
	{
		InvSetAmmo(::gSpawnInv, am_clip, param);
	}
	else if (iequals(cmd, "shells"))
	{
		InvSetAmmo(::gSpawnInv, am_shell, param);
	}
	else if (iequals(cmd, "rockets"))
	{
		InvSetAmmo(::gSpawnInv, am_misl, param);
	}
	else if (iequals(cmd, "cells"))
	{
		InvSetAmmo(::gSpawnInv, am_cell, param);
	}
	else if (iequals(cmd, "berserk"))
	{
		InvSetBerserk(::gSpawnInv, param);
	}
	else if (iequals(cmd, "backpack"))
	{
		InvSetBackpack(::gSpawnInv, param);
	}
	else
	{
		Printf(PRINT_WARNING, "spawninv: Unknown subcommand \"%s\".", param.c_str());
		return;
	}

	::g_spawninv.ForceSet(SpawnInvSerialize(::gSpawnInv).c_str());
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
		Printf("g_spawninv: %s\n", ::g_spawninv.cstring());
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
	else if (argc == 2)
	{
		SpawninvCommandHelp(argv[1]);
		return;
	}
	else if (argc == 3)
	{
		SpawninvCommand(argv[1], argv[2]);
		return;
	}

	SpawninvHelp();
}
END_COMMAND(spawninv)

/**
 * @brief Give players their spawn inventory.
 *
 *       This function touches health, armor, ready/pending weapon, owned
 *       weapons, starting ammo, berserk power, and if it gives you a backpack
 *       it sets your backpack flag and double your current maxammo.
 */
void G_GiveSpawnInventory(player_t& player)
{
	spawnInventory_t& inv = ::gSpawnInv;

	player.health = inv.health;
	player.armorpoints = inv.armorpoints;
	player.armortype = inv.armortype;
	player.readyweapon = player.pendingweapon = inv.readyweapon;
	ArrayCopy(player.weaponowned, inv.weaponowned);
	ArrayCopy(player.ammo, inv.ammo);

	if (inv.berserk)
	{
		player.powers[pw_strength] = INV_BERSERK_TIME;
	}

	if (inv.backpack)
	{
		player.backpack = true;
		for (size_t i = 0; i < ARRAY_LENGTH(player.maxammo); i++)
		{
			player.maxammo[i] *= 2;
		}
	}
}

/**
 * @brief Give players their between-level inventory.
 *
 *       This function only touches powers that are lost between levels.
 */
void G_GiveBetweenInventory(player_t& player)
{
	spawnInventory_t& inv = ::gSpawnInv;

	if (inv.berserk)
	{
		player.powers[pw_strength] = INV_BERSERK_TIME;
	}
}
