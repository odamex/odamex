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
#include "m_random.h"

EXTERN_CVAR(g_horde_mintotalhp)
EXTERN_CVAR(g_horde_maxtotalhp)
EXTERN_CVAR(g_horde_goalhp)

std::vector<hordeDefine_t> ROUND_DEFINES;

void hordeDefine_t::addMonster(const roundMonsterType_e monster, const mobjtype_t mobj,
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

static void InitDefines()
{
	if (!::ROUND_DEFINES.empty())
	{
		return;
	}

	// [AM] Item mobj names are horrible.
	const mobjtype_t T_GREENARMOR = MT_MISC0;
	const mobjtype_t T_BLUEARMOR = MT_MISC1;
	const mobjtype_t T_BERSERK = MT_MISC13;
	const mobjtype_t T_SOULSPHERE = MT_MISC12;
	const mobjtype_t T_MEGASPHERE = MT_MEGA;
	const mobjtype_t T_INVULNERABILITY = MT_INV;

	{
		hordeDefine_t define;
		define.name = "Knee Deep in Odamex";
		define.minGroupHealth = 150;
		define.maxGroupHealth = 300;

		define.weapons.push_back(wp_shotgun);
		define.weapons.push_back(wp_chaingun);
		define.weapons.push_back(wp_missile);

		define.powerups.push_back(T_GREENARMOR);
		define.powerups.push_back(T_BERSERK);

		define.addMonster(hordeDefine_t::RM_NORMAL, MT_POSSESSED, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SHOTGUY, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_TROOP, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SERGEANT, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SHADOWS, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SKULL, 0.5f);
		define.addMonster(hordeDefine_t::RM_BOSS, MT_BRUISER, 1.0f);

		::ROUND_DEFINES.push_back(define);
	}

	{
		hordeDefine_t define;
		define.name = "The Funcrusher";
		define.minGroupHealth = 600;
		define.maxGroupHealth = 1200;

		define.weapons.push_back(wp_shotgun);
		define.weapons.push_back(wp_chaingun);
		define.weapons.push_back(wp_supershotgun);
		define.weapons.push_back(wp_missile);
		define.weapons.push_back(wp_plasma);

		define.powerups.push_back(T_BLUEARMOR);
		define.powerups.push_back(T_SOULSPHERE);

		define.addMonster(hordeDefine_t::RM_NORMAL, MT_POSSESSED, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SHOTGUY, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_CHAINGUY, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_TROOP, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SKULL, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SERGEANT, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SHADOWS, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_UNDEAD, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_KNIGHT, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_HEAD, 1.0f);
		define.addMonster(hordeDefine_t::RM_BOSS, MT_SPIDER, 1.0f);

		::ROUND_DEFINES.push_back(define);
	}

	{
		hordeDefine_t define;
		define.name = "Monsoon 64";
		define.minGroupHealth = 1000;
		define.maxGroupHealth = 2000;

		define.powerups.push_back(T_BLUEARMOR);
		define.powerups.push_back(T_SOULSPHERE);
		define.powerups.push_back(T_INVULNERABILITY);

		define.weapons.push_back(wp_shotgun);
		define.weapons.push_back(wp_chaingun);
		define.weapons.push_back(wp_supershotgun);
		define.weapons.push_back(wp_missile);
		define.weapons.push_back(wp_plasma);
		define.weapons.push_back(wp_bfg);

		define.addMonster(hordeDefine_t::RM_NORMAL, MT_CHAINGUY, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_SERGEANT, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_UNDEAD, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_PAIN, 0.25f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_BRUISER, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_BABY, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_FATSO, 1.0f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_VILE, 0.25f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_CYBORG, 0.25f);
		define.addMonster(hordeDefine_t::RM_NORMAL, MT_HEAD, 1.0f);
		define.addMonster(hordeDefine_t::RM_BOSS, MT_CYBORG, 1.0f);
		define.addMonster(hordeDefine_t::RM_BOSS, MT_SPIDER, 1.0f);

		::ROUND_DEFINES.push_back(define);
	}
}

/**
 * @brief Get a define to use for this wave.
 */
const hordeDefine_t& P_HordeDefine(const uint32_t idx)
{
	InitDefines();
	return ::ROUND_DEFINES.at(idx);
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
		const hordeDefine_t::monster_t& roundMon = define.monsters.at(i);
		const mobjinfo_t& info = ::mobjinfo[roundMon.mobj];

		// Boss spawns have to spawn boss things.
		if (wantBoss && roundMon.monster == hordeDefine_t::RM_NORMAL)
			continue;

		// Non-boss spawns have to spawn non-boss things.
		if (!wantBoss && roundMon.monster == hordeDefine_t::RM_BOSS)
			continue;

		monsters.push_back(&roundMon);
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
