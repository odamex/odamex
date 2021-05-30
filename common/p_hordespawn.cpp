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
//  "Horde" game mode spawn selection.
//
//-----------------------------------------------------------------------------

#include "p_hordespawn.h"

#include <algorithm>

#include "actor.h"
#include "c_effect.h"
#include "m_random.h"
#include "p_hordedefine.h"
#include "p_local.h"
#include "s_sound.h"

struct SpawnPointWeight
{
	hordeSpawn_t* spawn;
	float score;
	int dist;
	bool visible;
};
typedef std::vector<SpawnPointWeight> SpawnPointWeights;

static hordeSpawns_t itemSpawns;
static hordeSpawns_t monsterSpawns;

static bool CmpWeights(const SpawnPointWeight& a, const SpawnPointWeight& b)
{
	return a.score > b.score;
}

static bool CmpDist(const SpawnPointWeight& a, const SpawnPointWeight& b)
{
	return a.dist < b.dist;
}

/**
 * @brief Spawn a monster.
 *
 * @param point Spawn point of the monster.
 * @param recipe Recipe of a monster to spawn.
 * @param offset Offset from the spawn point to spawn at.
 * @param target Player to target.
 * @return Actor pointer we just spawned, or NULL if the spawn failed.
 */
static AActor* SpawnMonster(hordeSpawn_t& spawn, const hordeRecipe_t& recipe,
                            const v2fixed_t offset)
{
	AActor* mo = new AActor(spawn.mo->x + offset.x, spawn.mo->y + offset.y, spawn.mo->z,
	                        recipe.type);
	if (mo)
	{
		if (P_TestMobjLocation(mo))
		{
			// Don't respawn
			mo->flags |= MF_DROPPED;

			if (recipe.isBoss)
			{
				// Purple is the noblest shroud.
				mo->effects = FX_PURPLEFOUNTAIN;

				// Set flags as a boss.
				mo->oflags = MFO_INFIGHTINVUL;
			}

			// Spawn a teleport fog if it's not an ambush.
			AActor* tele = new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z, MT_TFOG);
			S_Sound(tele, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
			return mo;
		}
		else
		{
			// Spawn blocked.
			mo->Destroy();
		}
	}
	return NULL;
}

/**
 * @brief Spawn multiple monsters close to each other.
 *
 * @param point Spawn point of the monster.
 * @param recipe Recipe of a monster to spawn.
 * @param count Number of monsters to spawn on the point.
 * @return Actors spawned by this function.  Can be discarded.
 */
static AActors SpawnMonsterGroup(hordeSpawn_t& spawn, const hordeRecipe_t& recipe,
                                 const int count)
{
	AActors ok;

	const fixed_t rad = ::mobjinfo[recipe.type].radius;
	const char* name = ::mobjinfo[recipe.type].name;

	if (count == 4)
	{
		// A big square.
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, -rad)));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, -rad)));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, rad)));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, rad)));
	}
	else if (count == 3)
	{
		// A wedge.
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, -rad)));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, -rad)));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(0, rad)));
	}
	else if (count == 2)
	{
		// Next to each other.
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, 0)));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, 0)));
	}
	else if (count == 1)
	{
		// All by themselves :(
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(0, 0)));
	}
	else
	{
		Printf(PRINT_WARNING, "Invalid spawn count %d of %s.\n", count, name);
	}

	// Remove unspawned actors - probably spawnblocked.
	AActors ret;
	for (AActors::iterator it = ok.begin(); it != ok.end(); ++it)
	{
		if ((*it) != NULL)
		{
			ret.push_back(*it);
		}
	}

	if (ret.size() < count)
	{
		Printf(PRINT_WARNING,
		       "Spawned %" PRIuSIZE "/%d of type %s at a %s spawn (%f, %f).\n",
		       ret.size(), count, name, HordeThingStr(spawn.type),
		       FIXED2FLOAT(spawn.mo->x), FIXED2FLOAT(spawn.mo->y));
	}

	return ret;
}

/**
 * @brief Add a spawn point to the list of known spawns.
 *
 * @param mo Actor to add.
 * @param type Type of spawn.
 */
void P_HordeAddSpawn(AActor* mo, const int type)
{
	hordeSpawn_t sp;
	sp.mo = mo->self;
	sp.type = type;
	if (type == TTYPE_HORDE_ITEM)
	{
		::itemSpawns.push_back(sp);
	}
	else
	{
		::monsterSpawns.push_back(sp);
	}
}

/**
 * @brief Clear all tracked spawn points.
 */
void P_HordeClearSpawns()
{
	::itemSpawns.clear();
	::monsterSpawns.clear();
}

/**
 * @brief Get a spawn point to place a monster at.
 */
hordeSpawn_t* P_HordeSpawnPoint(const hordeRecipe_t& recipe)
{
	float totalScore = 0.0f;

	SpawnPointWeights weights;
	for (hordeSpawns_t::iterator sit = monsterSpawns.begin(); sit != monsterSpawns.end();
	     ++sit)
	{
		// Don't spawn bosses at non-boss spawns.
		if (recipe.isBoss && sit->type != TTYPE_HORDE_BOSS)
			continue;
		else if (!recipe.isBoss && sit->type == TTYPE_HORDE_BOSS)
			continue;

		mobjinfo_t& info = ::mobjinfo[recipe.type];

		// Flying spawns have to spawn flying monsters.
		const bool isFlying = info.flags & (MF_NOGRAVITY | MF_FLOAT);
		if (sit->type == TTYPE_HORDE_FLYING && !isFlying)
			continue;

		// Snipers have to:
		// - Have a ranged attack.
		// - Not be flying.
		// - Not be a boss monster for this round.
		// - Be skinny enough to fit in a 64x64 square.
		if (sit->type == TTYPE_HORDE_SNIPER &&
		    (info.missilestate == S_NULL || isFlying || recipe.isBoss ||
		     info.radius > (32 * FRACUNIT)))
			continue;

		// Big snipers are similar, but they get to fit
		// into a 128x128 square.
		if (sit->type == TTYPE_HORDE_BIGSNIPER &&
		    (info.missilestate == S_NULL || isFlying || recipe.isBoss ||
		     info.radius > (64 * FRACUNIT)))
			continue;

		SpawnPointWeight weight;
		weight.spawn = &*sit;

		// [AM] During development we used to have a complicated spawn system
		//      that spawned near players, but this resulted in it being
		//      easy to exploit.

		float score = 1.0f;
		if (sit->type == TTYPE_HORDE_FLYING || sit->type == TTYPE_HORDE_SNIPER ||
		    sit->type == TTYPE_HORDE_BIGSNIPER)
			score = 0.75f;

		weight.score = score;
		totalScore += score;

		weights.push_back(weight);
	}

	// Did we find any spawns?
	if (weights.empty())
	{
		Printf(PRINT_WARNING, "Could not find a spawn point (boss:%s).\n",
		       recipe.isBoss ? "y" : "n");
		return NULL;
	}

	// Sort by weights.
	std::sort(weights.begin(), weights.end(), CmpWeights);
	float choice = P_RandomFloat() * totalScore;

	SpawnPointWeights::const_iterator it = weights.begin();
	for (; it != weights.end(); ++it)
	{
		// If our choice number is less than the score, we found our spawn.
		if (choice < it->score)
			break;

		// Since scores origin at 0.0 we subtract the current score from our
		// choice so the next comparison lines up.
		choice -= it->score;
	}

	// This should not happen unless we were careless with our math.
	if (it == weights.end())
		return NULL;

	return it->spawn;
}

/**
 * @brief Spawn multiple monsters close to each other.
 *
 * @param point Spawn point of the monster.
 * @param recipe Recipe of a monster to spawn.
 * @return Actors spawned by this function.  Can be discarded.
 */
AActors P_HordeSpawn(hordeSpawn_t& spawn, const hordeRecipe_t& recipe)
{
	AActors ok;

	SpawnPointWeights weights;
	for (hordeSpawns_t::iterator it = ::monsterSpawns.begin();
	     it != ::monsterSpawns.end(); ++it)
	{
		if (it->type != spawn.type)
			continue;

		SpawnPointWeight spw = {0};
		spw.spawn = &*it;
		spw.dist = P_AproxDistance2(it->mo, spawn.mo);
		weights.push_back(spw);
	}

	// Sort by distance.
	std::sort(weights.begin(), weights.end(), CmpDist);

	// Ensure we only spawn as many monsters as can fit in the spawn.
	// Snipers must fit in a 64x64 square, big snipers must fit into a 128x128 square,
	// bosses must fit in a 256x256 square, everything else must fit in a 128x128 square.
	const int rad = ::mobjinfo[recipe.type].radius >> FRACBITS;
	int maxGroupSize = 4;
	if (spawn.type == TTYPE_HORDE_SNIPER || spawn.type == TTYPE_HORDE_BIGSNIPER ||
	    (spawn.type == TTYPE_HORDE_BOSS && rad * 2 > 128) || rad * 2 > 64)
	{
		// We can only fit one monster per spawn point.
		maxGroupSize = 1;
	}

	// Printf("Spawning %d of type %s\n", recipe.count, ::mobjinfo[recipe.type].name);

	// Place monsters in spawn points in order of approx distance.
	for (SpawnPointWeights::iterator it = weights.begin(); it != weights.end(); ++it)
	{
		const int left = recipe.count - ok.size();
		if (left < 1)
			break;

		if (it->dist > (1024 * FRACUNIT))
			continue;

		int groupIter = clamp(left, 1, maxGroupSize);

		AActors okIter = SpawnMonsterGroup(*it->spawn, recipe, groupIter);
		ok.insert(ok.end(), okIter.begin(), okIter.end());
	}

	return ok;
}

/**
 * @brief Possibly spawn an item at one of the available item spawners.
 */
void P_HordeSpawnItem()
{
	// Find all empty points.
	hordeSpawns_t emptys;
	for (hordeSpawns_t::iterator it = ::itemSpawns.begin(); it != ::itemSpawns.end();
	     ++it)
	{
		if (it->mo->target == NULL)
		{
			emptys.push_back(*it);
		}
	}

	// Items should exist at around half of our points.
	if (emptys.size() > ::itemSpawns.size() / 2)
	{
		// Select a random spot.
		size_t idx = P_RandomInt(emptys.size());
		hordeSpawn_t& point = emptys.at(idx);
		if (point.mo->target == NULL)
		{
			AActor* pack = new AActor(point.mo->x, point.mo->y, point.mo->z, MT_CAREPACK);
			point.mo->target = pack->ptr();

			// Don't respawn the usual way.
			pack->flags |= MF_DROPPED;

			// Play the item respawn sound, so people can listen for it.
			AActor* tele = new AActor(pack->x, pack->y, pack->z, MT_IFOG);
			S_Sound(pack, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);
		}
	}
}
