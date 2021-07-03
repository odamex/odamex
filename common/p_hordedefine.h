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

#pragma once

#include <vector>

#include "info.h"

struct hordeRecipe_t
{
	mobjtype_t type;
	int count;
	bool isBoss;

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
		type = MT_PLAYER;
		count = 0;
		isBoss = false;
	}

	bool isValid() const
	{
		return type != MT_PLAYER && count > 0;
	}
};

struct hordeDefine_t
{
	enum waveMonsterType_e
	{
		RM_NULL,
		RM_NORMAL,
		RM_BOSS,
	};

	struct monster_t
	{
		hordeDefine_t::waveMonsterType_e monster;
		mobjtype_t mobj;
		float chance;
	};

	typedef std::vector<weapontype_t> weapons_t;
	typedef std::vector<monster_t> monsters_t;
	typedef std::vector<mobjtype_t> powerups_t;

	std::string name;    // Name of the wave.
	weapons_t weapons;   // Weapons we can spawn this wave.
	monsters_t monsters; // Monsters we can spawn this wave.
	powerups_t powerups; // Powerups we can spawn this wave.
	int minGroupHealth;  // Minimum health of a group of monsters to spawn.
	int maxGroupHealth;  // Maximum health of a group of monsters to spawn.
	void addMonster(const waveMonsterType_e monster, const mobjtype_t mobj,
	                const float chance);
	int minTotalHealth() const;
	int maxTotalHealth() const;
	int goalHealth() const;
};

const hordeDefine_t& P_HordeDefine(const int current, const int total);
bool P_HordeSpawnRecipe(hordeRecipe_t& out, const hordeDefine_t& define,
                        const bool wantBoss);
