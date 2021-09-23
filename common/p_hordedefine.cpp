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

static float PowerupChance(const hordeDefine_t::powerup_t& pow)
{
	return pow.config.chance;
}

static float MonsterChance(const hordeDefine_t::monster_t& mon)
{
	return mon.config.chance;
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
		return static_cast<size_t>(section_offset + section_choice);
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

// [AM] This is quite possibly the worst type parameter I've ever written.
static float MonsterWeight(const hordeDefine_t::monster_t* const& mon)
{
	return mon->config.chance;
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
	const hordeDefine_t::monster_t* const monster =
	    P_RandomFloatWeighted(monsters, MonsterWeight);

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
