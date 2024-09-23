// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//   Non-ingame horde functionality.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "g_horde.h"

#include <set>

#include "cmdlib.h"
#include "d_items.h"
#include "hashtable.h"
#include "infomap.h"
#include "oscanner.h"
#include "w_wad.h"

extern std::vector<hordeDefine_t> WAVE_DEFINES;

static hordeDefine_t EMPTY_WAVE_DEFINE;

typedef OHashTable<std::string, mobjtype_t> AliasMap;
AliasMap g_aliasMap;

static mobjtype_t NameOrAliasToMobj(const std::string& name)
{
	AliasMap::iterator it = ::g_aliasMap.find(name);
	if (it != ::g_aliasMap.end())
	{
		return it->second;
	}
	else
	{
		return P_NameToMobj(name);
	}
}

static void ParsePowerupConfig(OScanner& os, hordeDefine_t::powConfig_t& outConfig)
{
	for (;;)
	{
		os.mustScan(); // Prime the next token.
		if (os.compareToken("}"))
		{
			// Bail out of the block.
			break;
		}
		else if (os.compareToken("chance"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScanFloat();
			outConfig.chance = os.getTokenFloat();
		}
		else
		{
			// We don't know what this token is.
			std::string buffer;
			StrFormat(buffer, "Unknown Powerup Token \"%s\".", os.getToken().c_str());
			os.error(buffer.c_str());
		}
	}
}

static void ParseMonsterConfig(OScanner& os, hordeDefine_t::monConfig_t& outConfig)
{
	for (;;)
	{
		os.mustScan(); // Prime the next token.
		if (os.compareToken("}"))
		{
			// Bail out of the block.
			break;
		}
		else if (os.compareToken("grouphealth"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScanInt();
			outConfig.minGroupHealth = os.getTokenInt();
			os.mustScan();
			os.assertTokenIs(",");
			os.mustScanInt();
			outConfig.maxGroupHealth = os.getTokenInt();
		}
		else if (os.compareToken("chance"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScanFloat();
			outConfig.chance = os.getTokenFloat();
		}
		else
		{
			// We don't know what this token is.
			std::string buffer;
			StrFormat(buffer, "Unknown Monster/Boss Token \"%s\".", os.getToken().c_str());
			os.error(buffer.c_str());
		}
	}
}

static void ParseDefine(OScanner& os)
{
	hordeDefine_t define;

	os.assertTokenIs("define");
	os.mustScan();
	define.name = os.getToken();
	os.mustScan();
	os.assertTokenIs("{");

	for (;;)
	{
		os.mustScan(); // Prime the next token.
		if (os.compareToken("}"))
		{
			// Bail out of the block.
			break;
		}
		else if (os.compareToken("grouphealth"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScanInt();
			define.minGroupHealth = os.getTokenInt();
			os.mustScan();
			os.assertTokenIs(",");
			os.mustScanInt();
			define.maxGroupHealth = os.getTokenInt();
		}
		else if (os.compareToken("bosshealth"))
		{
			os.mustScan();
			os.assertTokenIs("=");
			os.mustScanInt();
			define.minBossHealth = os.getTokenInt();
			os.mustScan();
			os.assertTokenIs(",");
			os.mustScanInt();
			define.maxBossHealth = os.getTokenInt();
		}
		else if (os.compareToken("weapons"))
		{
			os.mustScan();
			os.assertTokenIs("=");

			for (;;)
			{
				os.mustScan();
				const weapontype_t weapon = P_NameToWeapon(os.getToken());
				if (weapon == wp_none)
				{
					// Special case for berserk.
					if (os.compareTokenNoCase("Berserk"))
					{
						// wp_none will be added as a special case.
					}
					else
					{
						std::string buffer;
						StrFormat(buffer, "Unknown weapon \"%s\".",
						          os.getToken().c_str());
						os.error(buffer.c_str());
					}
				}
				define.weapons.push_back(weapon);
				os.mustScan();
				if (!os.compareToken(","))
				{
					// End of the list.
					os.unScan();
					break;
				}
			}
		}
		else if (os.compareToken("addpowerup"))
		{
			// Powerup name.
			os.mustScan();
			const mobjtype_t type = NameOrAliasToMobj(os.getToken());
			if (type == MT_NULL)
			{
				std::string buffer;
				StrFormat(buffer, "Unknown powerup \"%s\".", os.getToken().c_str());
				os.error(buffer.c_str());
			}

			// Config block.
			hordeDefine_t::powConfig_t config;
			os.mustScan();
			if (os.getToken() == "{")
			{
				ParsePowerupConfig(os, config);
			}
			else
			{
				// Use default config.
				os.unScan();
			}

			define.addPowerup(type, config);
		}
		else if (os.compareToken("addmonster"))
		{
			// Monster name.
			os.mustScan();
			const mobjtype_t type = NameOrAliasToMobj(os.getToken());
			if (type == MT_NULL)
			{
				std::string buffer;
				StrFormat(buffer, "Unknown monster \"%s\".", os.getToken().c_str());
				os.error(buffer.c_str());
			}

			// Config block.
			hordeDefine_t::monConfig_t config;
			os.mustScan();
			if (os.getToken() == "{")
			{
				ParseMonsterConfig(os, config);
			}
			else
			{
				// Use default config.
				os.unScan();
			}

			define.addMonster(hordeDefine_t::RM_NORMAL, type, config);
		}
		else if (os.compareToken("addboss"))
		{
			// Monster name.
			os.mustScan();
			const mobjtype_t type = NameOrAliasToMobj(os.getToken());
			if (type == MT_NULL)
			{
				std::string buffer;
				StrFormat(buffer, "Unknown boss \"%s\".", os.getToken().c_str());
				os.error(buffer.c_str());
			}

			// Config block.
			hordeDefine_t::monConfig_t config;
			os.mustScan();
			if (os.getToken() == "{")
			{
				ParseMonsterConfig(os, config);
			}
			else
			{
				// Use default config.
				os.unScan();
			}

			define.addMonster(hordeDefine_t::RM_BOSS, type, config);
		}
		else
		{
			// We don't know what this token is.
			std::string buffer;
			StrFormat(buffer, "Unknown Token \"%s\".", os.getToken().c_str());
			os.error(buffer.c_str());
		}
	}

	// Add ammo for the weapons in order of their definition.
	std::set<ammotype_t> ammoAdded;
	for (size_t i = 0; i < define.weapons.size(); i++)
	{
		// Is the weapon valid?
		const weapontype_t& weap = define.weapons.at(i);
		if (weap < wp_fist || weap >= NUMWEAPONS)
		{
			continue;
		}

		// Does the weapon have ammo?
		const ammotype_t& ammo = ::weaponinfo[weap].ammotype;
		if (ammo == am_noammo)
		{
			continue;
		}

		// Have we already added ammo for it?
		if (ammoAdded.find(ammo) != ammoAdded.end())
		{
			continue;
		}

		ammoAdded.insert(ammo);
		define.ammos.push_back(ammo);
	}

	std::string buf;
	if (define.name.empty())
	{
		os.error("Define doesn't have a name.");
	}
	if (define.weapons.empty())
	{
		StrFormat(buf, "No weapon pickups found for define \"%s\".", define.name.c_str());
		os.warning(buf.c_str());
	}
	if (define.monsters.empty())
	{
		StrFormat(buf, "No monsters found for define \"%s\".", define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.powerups.empty())
	{
		StrFormat(buf, "No powerups found for define \"%s\".", define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.minGroupHealth < 0)
	{
		StrFormat(buf, "Minimum group health for define \"%s\" was not set.",
		          define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.maxGroupHealth <= 0)
	{
		StrFormat(buf, "Maximum group health for define \"%s\" was not set.",
		          define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.minGroupHealth > define.maxGroupHealth)
	{
		StrFormat(buf, "Maximum group health for define \"%s\" is less than minimum.",
		          define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.minBossHealth < 0)
	{
		StrFormat(buf, "Minimum boss health for define \"%s\" was not set.",
		          define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.maxBossHealth <= 0)
	{
		StrFormat(buf, "Maximum boss health for define \"%s\" was not set.",
		          define.name.c_str());
		os.error(buf.c_str());
	}
	if (define.minBossHealth > define.maxBossHealth)
	{
		StrFormat(buf, "Maximum boss health for define \"%s\" is less than minimum.",
		          define.name.c_str());
		os.error(buf.c_str());
	}

	::WAVE_DEFINES.push_back(define);
}

static void ParseAlias(OScanner& os)
{
	os.assertTokenIs("alias");
	os.mustScan();
	std::string alias = os.getToken();
	os.mustScan();
	std::string original = os.getToken();

	const mobjtype_t otype = P_NameToMobj(original);
	if (otype == MT_NULL)
	{
		// We don't know what this token is.
		std::string buffer;
		StrFormat(buffer, "Can't alias unknown thing \"%s\".", original.c_str());
		os.error(buffer.c_str());
	}

	if (!CheckIfDehActorDefined(otype))
	{
		// [Blair] DEHEXTRA monster not defined
		std::string buffer;
		StrFormat(buffer, "The following actor is undefined: \"%s\".", original.c_str());
		os.error(buffer.c_str());
	}

	g_aliasMap.insert(std::make_pair(alias, otype));
}

static void ParseHordeDef(const int lump, const char* name)
{
	const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

	OScannerConfig config = {
	    name,  // lumpName
	    false, // semiComments
	    true,  // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	// Right now we only understand one top-level token - "define".
	while (os.scan())
	{
		if (os.compareTokenNoCase("define"))
		{
			ParseDefine(os);
		}
		else if (os.compareTokenNoCase("cleardefines"))
		{
			::WAVE_DEFINES.clear();
		}
		else if (os.compareTokenNoCase("alias"))
		{
			ParseAlias(os);
		}
		else
		{
			// We don't know what this token is.
			std::string buffer;
			StrFormat(buffer, "Unknown Token \"%s\".", os.getToken().c_str());
			os.error(buffer.c_str());
		}
	}
}

/**
 * @brief Cmp function for sorting defines by max group health.
 */
static bool CmpHordeDefs(const hordeDefine_t& a, const hordeDefine_t& b)
{
	return a.maxGroupHealth < b.maxGroupHealth;
}

struct FindHordeDef
{
	std::string name;
	FindHordeDef(const std::string& name) : name(name) { }
	bool operator()(const hordeDefine_t& def) { return def.name == name; }
};

static void ParseHordeDefs()
{
	int lump = -1;
	while ((lump = W_FindLump("HORDEDEF", lump)) != -1)
	{
		ParseHordeDef(lump, "HORDEDEF");
	}

	if (::WAVE_DEFINES.empty())
	{
		return;
	}

	// Must be stable for wave ID's to be the same on client and server.
	std::stable_sort(::WAVE_DEFINES.begin(), ::WAVE_DEFINES.end(), CmpHordeDefs);

	// Dedupe wave defines.  Note that this has a gigantic hack for 10.0 that keeps
	// track of the original wave ID so it can be sent to 10.0 clients.  This hack
	// should be removed in 11.0 at the soonest.

	for (size_t i = 0; i < ::WAVE_DEFINES.size(); i++)
	{
		::WAVE_DEFINES[i].legacyID = i;
	}

	for (std::vector<hordeDefine_t>::iterator it = ::WAVE_DEFINES.begin();
	     it != ::WAVE_DEFINES.end(); ++it)
	{
		std::vector<hordeDefine_t>::iterator after = it;
		after++;

		// No need to search if we're at the end.
		if (after == ::WAVE_DEFINES.end())
			continue;

		std::vector<hordeDefine_t>::iterator found =
		    std::find_if(after, ::WAVE_DEFINES.end(), FindHordeDef(it->name));

		// If we didn't find any matches, don't remove.
		if (found == ::WAVE_DEFINES.end())
			continue;

		// Erase and set our iterator to the one after it.
		it = ::WAVE_DEFINES.erase(it);
	}
}

/**
 * @brief Initialize horde defines with the current set of loaded WAD files.
 */
void G_ParseHordeDefs()
{
	::WAVE_DEFINES.clear();
	::g_aliasMap.clear();
	ParseHordeDefs();
}

/**
 * @brief Resolve a horde define ID to an actual define.  Should be identical on client
 * and server.
 *
 * @param id ID of define.
 */
const hordeDefine_t& G_HordeDefine(size_t id)
{
	if (id >= ::WAVE_DEFINES.size())
	{
		Printf(PRINT_WARNING,
		       "Tried to access horde wave %lu but only have %lu horde defines!\n", id,
		       ::WAVE_DEFINES.size());
		return EMPTY_WAVE_DEFINE;
	}

	return ::WAVE_DEFINES[id];
}
