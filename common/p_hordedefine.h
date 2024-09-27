// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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

#pragma once

#include <vector>

#include "info.h"
#include "d_player.h"

struct hordeRecipe_t
{
	mobjtype_t type;
	int count;
	bool isBoss;

	hordeRecipe_t() : type(MT_NULL), count(0), isBoss(false) { }

	hordeRecipe_t& operator=(const hordeRecipe_t& other)
	{
		if (this == &other)
			return *this;

		type = other.type;
		count = other.count;
		isBoss = other.isBoss;

		return *this;
	}

	void clear()
	{
		type = MT_NULL;
		count = 0;
		isBoss = false;
	}

	bool isValid() const
	{
		return type != MT_NULL && count > 0;
	}
};

struct hordeDefine_t
{
	struct powConfig_t
	{
		float chance;
		powConfig_t() : chance(1.0f) { }
	};

	struct powerup_t
	{
		mobjtype_t mobj;
		powConfig_t config;
	};

	enum waveMonsterType_e
	{
		RM_NULL,
		RM_NORMAL,
		RM_BOSS,
	};

	struct monConfig_t
	{
		int minGroupHealth;
		int maxGroupHealth;
		float chance;
		monConfig_t() : minGroupHealth(-1), maxGroupHealth(-1), chance(1.0f) { }
	};

	struct monster_t
	{
		hordeDefine_t::waveMonsterType_e monster;
		mobjtype_t mobj;
		monConfig_t config;
	};

	typedef std::vector<weapontype_t> weapons_t;
	typedef std::vector<ammotype_t> ammos_t;
	typedef std::vector<powerup_t> powerups_t;
	typedef std::vector<monster_t> monsters_t;

	uint32_t legacyID;	 // ID of wave assuming no deduplication.  Remove me.
	std::string name;    // Name of the wave.
	weapons_t weapons;   // Weapons we can spawn this wave.
	ammos_t ammos;       // Ammos we can replenish this wave.
	monsters_t monsters; // Monsters we can spawn this wave.
	powerups_t powerups; // Powerups we can spawn this wave.
	int minGroupHealth;  // Minimum health of a group of monsters to spawn.
	int maxGroupHealth;  // Maximum health of a group of monsters to spawn.
	int minBossHealth;  // Minimum health of a group of bosses to spawn.
	int maxBossHealth;  // Maximum health of a group of bosses to spawn.

	hordeDefine_t()
	    : legacyID(0), minGroupHealth(-1), maxGroupHealth(-1), minBossHealth(-1),
	      maxBossHealth(-1)
	{
	}

	void addPowerup(const mobjtype_t mobj, const powConfig_t& config);
	const powerup_t& randomPowerup() const;
	void addMonster(const waveMonsterType_e monster, const mobjtype_t mobj,
	                const monConfig_t& config);
	int minTotalHealth() const;
	int maxTotalHealth() const;
	int goalHealth() const;
	const char* difficulty(const bool colored) const;
	StringTokens weaponStrings(player_t* player) const;
};

void G_ParseHordeDefs();
const hordeDefine_t& G_HordeDefine(size_t id);

size_t P_HordePickDefine(const int current, const int total);
bool P_HordeSpawnRecipe(hordeRecipe_t& out, const hordeDefine_t& define,
                        const bool wantBoss);
bool P_HordeDefineNamed(int& out, const std::string& name);
