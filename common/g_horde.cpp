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

#include "g_horde.h"

#include "cmdlib.h"
#include "infomap.h"
#include "oscanner.h"
#include "w_wad.h"

extern std::vector<hordeDefine_t> WAVE_DEFINES;

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
							define.weapons.push_back(wp_none);
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
				os.mustScan();
				os.assertTokenIs("=");

				os.mustScan();
				const mobjtype_t type = P_NameToMobj(os.getToken());
				if (type == MT_NULL)
				{
					std::string buffer;
					StrFormat(buffer, "Unknown object \"%s\".", os.getToken().c_str());
					os.error(buffer.c_str());
				}

				define.powerups.push_back(type);
			}
			else if (os.compareToken("addmonster"))
			{
				os.mustScan();
				os.assertTokenIs("=");

				// Monster name.
				os.mustScan();
				const mobjtype_t type = P_NameToMobj(os.getToken());
				if (type == MT_NULL)
				{
					std::string buffer;
					StrFormat(buffer, "Unknown object \"%s\".", os.getToken().c_str());
					os.error(buffer.c_str());
				}

				// Chance.
				float chance = 1.0f;

				os.mustScan();
				if (os.getToken() == ",")
				{
					os.mustScanFloat();
					chance = os.getTokenFloat();
				}
				else
				{
					// Default to 1.0.
					os.unScan();
				}

				define.addMonster(hordeDefine_t::RM_NORMAL, type, chance);
			}
			else if (os.compareToken("addboss"))
			{
				os.mustScan();
				os.assertTokenIs("=");

				// Monster name.
				os.mustScan();
				const mobjtype_t type = P_NameToMobj(os.getToken());
				if (type == MT_NULL)
				{
					std::string buffer;
					StrFormat(buffer, "Unknown object \"%s\".", os.getToken().c_str());
					os.error(buffer.c_str());
				}

				// Chance.
				float chance = 1.0f;

				os.mustScan();
				if (os.getToken() == ",")
				{
					os.mustScanFloat();
					chance = os.getTokenFloat();
				}
				else
				{
					// Default to 1.0.
					os.unScan();
				}

				define.addMonster(hordeDefine_t::RM_BOSS, type, chance);
			}
		}

		std::string buf;
		if (define.name.empty())
		{
			os.error("Define doesn't have a name.");
		}
		if (define.weapons.empty())
		{
			StrFormat(buf, "No weapon pickups found for define \"%s\".",
			          define.name.c_str());
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
}

/**
 * @brief Cmp function for sorting defines by max group health.
 */
static bool CmpHordeDefs(const hordeDefine_t& a, const hordeDefine_t& b)
{
	return a.maxGroupHealth < b.maxGroupHealth;
}

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

	std::sort(::WAVE_DEFINES.begin(), ::WAVE_DEFINES.end(), CmpHordeDefs);
}

/**
 * @brief Initialize horde defines with the current set of loaded WAD files.
 */
void G_ParseHordeDefs()
{
	::WAVE_DEFINES.clear();
	ParseHordeDefs();
}

/**
 * @brief Resolve a horde define ID to an actual define.  Should be identical on client and server.
 *
 * @param id ID of define.
 */
const hordeDefine_t& G_HordeDefine(size_t id)
{
	return ::WAVE_DEFINES.at(id);
}
