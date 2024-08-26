// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//  "Horde" game mode.
//
//-----------------------------------------------------------------------------

#pragma once

#include "actor.h"
#include "doomdata.h"
#include "g_level.h"
#include "p_hordedefine.h"

enum hordeState_e
{
	HS_STARTING, // Calm before the wave starts proper.
	HS_PRESSURE, // Spawns monsters.
	HS_RELAX,    // Doesn't spawn monsters.
	HS_WANTBOSS, // Drop everything and try and spawn a boss.
};

struct hordeInfo_t
{
	hordeState_e state;
	int wave;
	int waveTime;
	int bossTime;
	uint64_t defineID;
	uint64_t legacyID;
	int spawnedHealth;
	int killedHealth;
	int bossHealth;
	int bossDamage;
	int waveStartHealth;

	int alive() const
	{
		return spawnedHealth - killedHealth;
	}
	int killed() const
	{
		return killedHealth - waveStartHealth;
	}
	bool hasBoss() const
	{
		return waveTime != bossTime;
	}
	int bossTic() const
	{
		return ::level.time - bossTime;
	}
	bool equals(const hordeInfo_t& info) const
	{
		if (state != info.state)
			return false;
		if (wave != info.wave)
			return false;
		if (waveTime != info.waveTime)
			return false;
		if (bossTime != info.bossTime)
			return false;
		if (defineID != info.defineID)
			return false;
		if (spawnedHealth != info.spawnedHealth)
			return false;
		if (killedHealth != info.killedHealth)
			return false;
		if (bossHealth != info.bossHealth)
			return false;
		if (bossDamage != info.bossDamage)
			return false;
		if (waveStartHealth != info.waveStartHealth)
			return false;
		return true;
	}
};

void P_InitHorde();
void P_NextSpawnTime(int& min, int& max);
hordeInfo_t P_HordeInfo();
void P_SetHordeInfo(const hordeInfo_t& info);
void P_AddHealthPool(AActor* mo);
void P_RemoveHealthPool(AActor* mo);
void P_AddDamagePool(AActor* mo, const int damage);
void P_QueueCorpseForDestroy(AActor* mo);

void P_RunHordeTics();
bool P_IsHordeThing(const int type);
const hordeDefine_t::weapons_t& P_HordeWeapons();
const hordeDefine_t::ammos_t& P_HordeAmmos();
void P_SerializeHorde(FArchive& arc);
void P_HordePostLoad();
