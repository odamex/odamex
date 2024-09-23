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

#include "odamex.h"

#include "p_hordedefine.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "i_system.h"
#include "infomap.h"
#include "m_random.h"
#include "oscanner.h"
#include "w_wad.h"
#include "v_textcolors.h"

EXTERN_CVAR(g_horde_mintotalhp)
EXTERN_CVAR(g_horde_maxtotalhp)
EXTERN_CVAR(g_horde_goalhp)
EXTERN_CVAR(g_horde_waves)

std::vector<hordeDefine_t> WAVE_DEFINES;

static float SkillScaler()
{
	if (sv_skill == sk_baby || sv_skill == sk_easy)
		return 0.5f;
	else if (sv_skill == sk_medium)
		return 0.75f;
	else
		return 1.0f;
}

static float PowerupChance(const hordeDefine_t::powerup_t& pow)
{
	return pow.config.chance;
}

// [AM] This is quite possibly the worst type parameter I've ever written.
static float MonsterChance(const hordeDefine_t::monster_t* const& mon)
{
	return mon->config.chance;
}

void hordeDefine_t::addPowerup(const mobjtype_t mobj, const powConfig_t& config)
{
	powerup_t pow = {mobj, config};
	powerups.push_back(pow);
}

const hordeDefine_t::powerup_t& hordeDefine_t::randomPowerup() const
{
	return P_RandomFloatWeighted(powerups, PowerupChance);
}

void hordeDefine_t::addMonster(const waveMonsterType_e monster, const mobjtype_t mobj,
                               const monConfig_t& config)
{
	monster_t mon = {monster, mobj, config};
	monsters.push_back(mon);
}

int hordeDefine_t::minTotalHealth() const
{
	return static_cast<float>(maxGroupHealth) * ::g_horde_mintotalhp * SkillScaler();
}

int hordeDefine_t::maxTotalHealth() const
{
	return static_cast<float>(maxGroupHealth) * ::g_horde_maxtotalhp * SkillScaler();
}

int hordeDefine_t::goalHealth() const
{
	return static_cast<float>(maxGroupHealth) * ::g_horde_goalhp * SkillScaler();
}

const char* hordeDefine_t::difficulty(const bool colored) const
{
	if (maxGroupHealth < 500)
	{
		return colored ? TEXTCOLOR_LIGHTBLUE "E" : "E";
	}
	else if (maxGroupHealth < 1000)
	{
		return colored ? TEXTCOLOR_GREEN "D" : "D";
	}
	else if (maxGroupHealth < 2000)
	{
		return colored ? TEXTCOLOR_GOLD "C" : "C";
	}
	else if (maxGroupHealth < 3000)
	{
		return colored ? TEXTCOLOR_ORANGE "B" : "B";
	}
	else if (maxGroupHealth < 5000)
	{
		return colored ? TEXTCOLOR_RED "A" : "A";
	}
	else
	{
		return colored ? TEXTCOLOR_DARKRED "X" : "X";
	}
}

StringTokens hordeDefine_t::weaponStrings(player_t* player) const
{
	StringTokens rvo;
	for (size_t i = 0; i < weapons.size(); i++)
	{
		if (!(player == NULL || !player->weaponowned[weapons[i]]))
		{
			continue;
		}

		switch (weapons[i])
		{
		case wp_none:
			rvo.push_back("BSK");
			break;
		case wp_chainsaw:
			rvo.push_back("1+");
			break;
		case wp_shotgun:
			rvo.push_back("3");
			break;
		case wp_supershotgun:
			rvo.push_back("3+");
			break;
		case wp_chaingun:
			rvo.push_back("4");
			break;
		case wp_missile:
			rvo.push_back("5");
			break;
		case wp_plasma:
			rvo.push_back("6");
			break;
		case wp_bfg:
			rvo.push_back("7");
			break;
		case wp_fist:
		case wp_pistol:
		case wp_nochange:
		case NUMWEAPONS:
			break;
		}
	}

	return rvo;
}

/**
 * @brief Get a define to use for this wave.
 *
 * @param current Current wave.
 * @param total Total waves.
 * @return Define we selected.
 */
size_t P_HordePickDefine(const int current, const int total)
{
	if (::WAVE_DEFINES.empty())
	{
		I_Error("%s: No wave defines found.\n", __FUNCTION__);
	}

	if (total > 0)
	{
		// Split our defines into equal sections and randomly pick
		// a wave from all of them.

		const float section_size = static_cast<float>(::WAVE_DEFINES.size()) / total;
		const float section_offset = (current - 1) * section_size;
		const float section_choice = M_RandomFloat() * section_size;
		const float section_limit = NextAfter(section_offset + section_size, 0.0f);
		return MIN<size_t>(section_offset + section_choice, section_limit);
	}
	else if (current <= 1)
	{
		// In endless mode, we pick a random define every round except for
		// the first, where we pick one from the easier half so to not kick
		// players in the teeth straight away.
		return (M_RandomFloat() * ::WAVE_DEFINES.size()) / 2;
	}
	else
	{
		// Endless mode, and the gloves are off.
		return M_RandomInt(::WAVE_DEFINES.size());
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
	const hordeDefine_t::monster_t* const monster =
	    P_RandomFloatWeighted(monsters, MonsterChance);

	const mobjtype_t outType = monster->mobj;
	const bool outIsBoss = monster->monster != hordeDefine_t::RM_NORMAL;
	const hordeDefine_t::monConfig_t* config = &monster->config;

	int outCount = 0;
	const int health = ::mobjinfo[outType].spawnhealth;

	// Maximum health.
	int maxHealth = -1;
	if (config->maxGroupHealth >= 0)
	{
		maxHealth = config->maxGroupHealth;
	}
	else if (wantBoss)
	{
		maxHealth = define.maxBossHealth;
	}
	else
	{
		maxHealth = define.maxGroupHealth;
	}

	// Minimum health.
	int minHealth = -1;
	if (config->minGroupHealth >= 0)
	{
		minHealth = config->minGroupHealth;
	}
	else if (wantBoss)
	{
		minHealth = define.minBossHealth;
	}
	else
	{
		minHealth = define.minGroupHealth;
	}

	const int upper = MAX(maxHealth / health, 1);
	const int lower = MAX(minHealth / health, 1);

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

/**
 * @brief Get a horde define with a given name.  Name matching is partial
 *        and case-insensitive.
 *
 * @param out Index found.
 * @param name Partial name to search for.
 * @return True if the define was found, otherwise false.
 */
bool P_HordeDefineNamed(int& out, const std::string& name)
{
	for (size_t i = 0; i < ::WAVE_DEFINES.size(); i++)
	{
		std::string defname = ::WAVE_DEFINES.at(i).name;
		if (defname.size() > name.size())
		{
			defname = defname.substr(0, name.size());
		}

		if (iequals(name, defname))
		{
			out = i;
			return true;
		}
	}

	return false;
}

static void PrintDefines(const std::vector<hordeDefine_t>::const_iterator& begin,
                         const std::vector<hordeDefine_t>::const_iterator& end)
{
	std::vector<hordeDefine_t>::const_iterator it = begin;
	for (; it != end; ++it)
	{
		const ptrdiff_t idx = it - ::WAVE_DEFINES.begin();
		Printf("%" PRIdSIZE ": %s (Group HP: %d)\n", idx, it->name.c_str(),
		       it->maxGroupHealth);
	}
}

BEGIN_COMMAND(hordedefine)
{
	if (argc >= 2)
	{
		if (iequals(argv[1], "all"))
		{
			PrintDefines(::WAVE_DEFINES.begin(), ::WAVE_DEFINES.end());
			return;
		}
		else if (iequals(argv[1], "wave"))
		{
			if (argc >= 3)
			{
				const int current = atoi(argv[2]);
				if (current && current <= g_horde_waves)
				{
					int total = -1;
					if (argc >= 4)
					{
						total = atoi(argv[3]);
					}
					else
					{
						total = g_horde_waves;
					}

					if (total > 0)
					{
						const float section_size =
						    static_cast<float>(::WAVE_DEFINES.size()) / total;
						const float section_offset = (current - 1) * section_size;
						const float section_choice = NextAfter(1.0f, 0.0f) * section_size;
						const float section_limit =
						    NextAfter(section_offset + section_size, 0.0f);
						const size_t start = static_cast<size_t>(section_offset);
						const size_t end =
						    MIN<size_t>(section_offset + section_choice, section_limit);
						Printf("[Wave %d/%d - Start:%" PRIuSIZE " End:%" PRIuSIZE "]\n",
						       current, total, start, end);
						PrintDefines(::WAVE_DEFINES.begin() + start,
						             ::WAVE_DEFINES.begin() + end + 1);
						return;
					}
					else
					{
						Printf("error: Total waves must be > 0.");
						return;
					}
				}
				else
				{
					Printf("error: Must pass a valid wave number.");
					return;
				}
			}
			else
			{
				Printf("error: Must pass a wave number.");
				return;
			}
		}
		else
		{
			Printf("error: Unknown command.");
			return;
		}
	}

	Printf("Commands:\n");
	Printf("  all\n");
	Printf("    Show all defines.\n");
	Printf("  wave <NUMBER> [TOTAL]\n");
	Printf("    Show potential waves for wave NUMBER of [TOTAL].  If omitted, [TOTAL] "
	       "defaults to the cvar `g_horde_waves`.\n");
}
END_COMMAND(hordedefine)
