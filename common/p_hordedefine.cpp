// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//  "Horde" game mode defines and recipes.
//
//-----------------------------------------------------------------------------

#include "p_hordedefine.h"

#include "c_cvars.h"
#include "cmdlib.h"
#include "i_system.h"
#include "infomap.h"
#include "m_random.h"
#include "oscanner.h"
#include "w_wad.h"

EXTERN_CVAR(g_horde_mintotalhp)
EXTERN_CVAR(g_horde_maxtotalhp)
EXTERN_CVAR(g_horde_goalhp)

std::vector<hordeDefine_t> WAVE_DEFINES;

void hordeDefine_t::addMonster(const waveMonsterType_e monster, const mobjtype_t mobj,
                               const float chance)
{
	monster_t mon = {monster, mobj, chance};
	monsters.push_back(mon);
}

int hordeDefine_t::minTotalHealth() const
{
	return static_cast<float>(maxGroupHealth) * ::g_horde_mintotalhp;
}

int hordeDefine_t::maxTotalHealth() const
{
	return static_cast<float>(maxGroupHealth) * ::g_horde_maxtotalhp;
}

int hordeDefine_t::goalHealth() const
{
	return static_cast<float>(maxGroupHealth) * ::g_horde_goalhp;
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
						std::string buffer;
						StrFormat(buffer, "Unknown weapon \"%s\".",
						          os.getToken().c_str());
						os.error(buffer.c_str());
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
		I_FatalError("No horde round defines were found.");
	}

	std::sort(::WAVE_DEFINES.begin(), ::WAVE_DEFINES.end(), CmpHordeDefs);
}

static void InitDefines()
{
	if (!::WAVE_DEFINES.empty())
	{
		return;
	}

	ParseHordeDefs();
}

/**
 * @brief Get a define to use for this wave.
 * 
 * @param current Current wave.
 * @param total Total waves.
 * @return Define we selected.
 */
const hordeDefine_t& P_HordeDefine(const int current, const int total)
{
	InitDefines();

	if (total > 0)
	{
		// Split our defines into equal sections and randomly pick
		// a wave from all of them.
		const float section_size = static_cast<float>(::WAVE_DEFINES.size()) / total;
		const float section_offset = (current - 1) * section_size;
		const float section_choice = M_RandomFloat() * section_size;
		const size_t choice = static_cast<size_t>(section_offset + section_choice);
		return ::WAVE_DEFINES.at(choice);
	}
	else if (current <= 1)
	{
		// In endless mode, we pick a random define every round except for
		// the first, where we pick one from the easier half so to not kick
		// players in the teeth straight away.
		const float choice = (M_RandomFloat() * ::WAVE_DEFINES.size()) / 2;
		return ::WAVE_DEFINES.at(choice);
	}
	else
	{
		// Endless mode, and the gloves are off.
		const size_t choice = M_RandomInt(::WAVE_DEFINES.size());
		return ::WAVE_DEFINES.at(choice);
	}
}

/**
 * @brief Get a recipe of monsters to spawn.
 *
 * @param out Recipe to write to.
 * @param define Round information to use.
 * @param wantBoss Caller wants a boss.
 */
bool P_HordeSpawnRecipe(hordeRecipe_t& out, const hordeDefine_t& define,
                        const bool wantBoss)
{
	std::vector<const hordeDefine_t::monster_t*> monsters;

	// Figure out which monster we want to spawn.
	for (size_t i = 0; i < define.monsters.size(); i++)
	{
		const hordeDefine_t::monster_t& waveMon = define.monsters.at(i);
		const mobjinfo_t& info = ::mobjinfo[waveMon.mobj];

		// Boss spawns have to spawn boss things.
		if (wantBoss && waveMon.monster == hordeDefine_t::RM_NORMAL)
			continue;

		// Non-boss spawns have to spawn non-boss things.
		if (!wantBoss && waveMon.monster == hordeDefine_t::RM_BOSS)
			continue;

		monsters.push_back(&waveMon);
	}

	// No valid monsters can be spawned at this point.
	if (monsters.empty())
		return false;

	// Randomly select a monster to spawn.
	mobjtype_t outType = static_cast<mobjtype_t>(-1);
	bool outIsBoss = false;
	for (size_t i = 0; i < 16; i++)
	{
		size_t resultIdx = P_RandomInt(monsters.size());
		outType = monsters.at(resultIdx)->mobj;
		outIsBoss = monsters.at(resultIdx)->monster != hordeDefine_t::RM_NORMAL;

		// Chance is 100%?
		if (monsters.at(resultIdx)->chance == 1.0f)
			break;

		// Chance is within bounds of a random number?
		const float chance = P_RandomFloat();
		if (monsters.at(resultIdx)->chance >= chance)
			break;
	}

	int outCount = 0;
	const int health = ::mobjinfo[outType].spawnhealth;
	const int upper = MAX(define.maxGroupHealth / health, 1);
	const int lower = MAX(define.minGroupHealth / health, 1);
	if (upper <= lower)
	{
		// Only one possibility.
		outCount = upper;
	}
	else
	{
		// Randomly select a possibility.
		outCount = P_RandomInt(upper - lower) + lower;
	}

	out.type = outType;
	out.count = outCount;
	out.isBoss = outIsBoss;

	return true;
}
