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

#include "odamex.h"

#include "p_hordespawn.h"

#include <algorithm>

#include "actor.h"
#include "c_effect.h"
#include "m_random.h"
#include "p_hordedefine.h"
#include "p_local.h"
#include "s_sound.h"

// Stub for client.
void SV_SpawnMobj(AActor* mobj);

struct SpawnPointWeight
{
	hordeSpawn_t* spawn;
	float score;
	int dist;
	bool visible;
};
typedef std::vector<SpawnPointWeight> SpawnPointWeights;

static hordeSpawns_t itemSpawns;
static hordeSpawns_t powerupSpawns;
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
static AActor::AActorPtr SpawnMonster(hordeSpawn_t& spawn, const hordeRecipe_t& recipe,
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
				// Heavy is the head that wears the crown.
				mo->effects = FX_YELLOWFOUNTAIN;
				mo->translation = translationref_t(&bosstable[0]);

				// Set flags as a boss.
				mo->oflags = MFO_INFIGHTINVUL | MFO_UNFLINCHING | MFO_ARMOR | MFO_QUICK |
				             MFO_NORAISE | MFO_BOSSPOOL | MFO_FULLBRIGHT;

				mo->flags2 = MF2_BOSS;

				mo->flags3 = MF3_FULLVOLSOUNDS | MF3_DMGIGNORED;
			}
			SV_SpawnMobj(mo);

			// Spawn a teleport fog if it's not an ambush.
			if ((spawn.mo->flags & MF_AMBUSH) == 0)
			{
				AActor* tele = new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z, MT_TFOG);
				SV_SpawnMobj(tele);
				S_NetSound(tele, CHAN_VOICE, "misc/teleport", ATTN_NORM);
			}
			return mo->ptr();
		}
		else
		{
			// Spawn blocked.
			mo->Destroy();
		}
	}

	return AActor::AActorPtr();
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
		DPrintf("Partial spawn %" "zu" "/%d of type %s at a %s spawn (%f, %f).\n",
		        ret.size(), count, name, HordeThingStr(spawn.type),
		        FIXED2FLOAT(spawn.mo->x), FIXED2FLOAT(spawn.mo->y));
	}

	return ret;
}

/**
 * @brief Find all horde spawn points and add them to our bookkeeping.
 */
void P_HordeAddSpawns()
{
	TThinkerIterator<AActor> iterator;
	AActor* mo;
	while ((mo = iterator.Next()))
	{
		if (mo->type != MT_HORDESPAWN)
			continue;

		hordeSpawn_t sp;
		sp.mo = mo->self;
		sp.type = mo->special1;
		switch (sp.type)
		{
		case TTYPE_HORDE_ITEM:
			::itemSpawns.push_back(sp);
			break;
		case TTYPE_HORDE_POWERUP:
			::powerupSpawns.push_back(sp);
			break;
		default:
			::monsterSpawns.push_back(sp);
			break;
		}
	}
}

/**
 * @brief Return true if there are any horde spawns being tracked in the world.
 */
bool P_HordeHasSpawns()
{
	return !::itemSpawns.empty() && !::powerupSpawns.empty() && !::monsterSpawns.empty();
}

/**
 * @brief Clear all tracked spawn points.
 */
void P_HordeClearSpawns()
{
	::itemSpawns.clear();
	::powerupSpawns.clear();
	::monsterSpawns.clear();
}

/**
 * @brief True if passed radius and height fits in the passed mobjinfo.
 * 
 * @param info Info to check against.
 * @param rad Radius to check in whole units (not fixed).
 * @param height Height to check in whole units (not fixed).
 */
static bool FitRadHeight(const mobjinfo_t& info, const int rad, const int height)
{
	return info.radius <= (rad * FRACUNIT) && info.height <= (height * FRACUNIT);
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
		mobjinfo_t& info = ::mobjinfo[recipe.type];
		const bool isFlying = info.flags & (MF_NOGRAVITY | MF_FLOAT);

		if (recipe.isBoss && sit->type != TTYPE_HORDE_BOSS &&
		    sit->type != TTYPE_HORDE_SMALLBOSS)
		{
			// Bosses cannot spawn at non-boss spawns.
			continue;
		}

		// [AM] Radius reference for the big bodies, so you can get an idea
		//      of what monsters are excluded from which radius comparisons.
		//      - Spider Mastermind: 128 units
		//      - Arachnotron: 64 units
		//      - Mancubus: 48 units
		//      - Cyberdemon: 40 units
		//      - Cacodemon/Pain Elemental: 31 units
		//      - Demon: 30 units

		const bool fitsNormal = FitRadHeight(info, 64, 128);
		const bool fitsSmall = FitRadHeight(info, 32, 64);

		switch (sit->type)
		{
		case TTYPE_HORDE_MONSTER:
			// Normal spawns can't spawn monsters that are too big.
			if (!fitsNormal)
				continue;
			break;
		case TTYPE_HORDE_BOSS:
			// Boss spawns can spawn non-boss monsters, but only if they're
			// too big to spawn anywhere else.
			if (!recipe.isBoss && fitsNormal)
				continue;
			break;
		case TTYPE_HORDE_SMALLMONSTER:
			// Small monster spawns have to spawn small monsters.
			if (!fitsSmall)
				continue;
			break;
		case TTYPE_HORDE_SMALLBOSS:
			// Small boss spawns can't spawn non-bosses, period.
			if (!recipe.isBoss || !fitsNormal)
				continue;
			break;
		case TTYPE_HORDE_FLYING:
			// Flying spawns have to spawn flying monsters.
			if (!isFlying || !fitsNormal)
				continue;
			break;
		case TTYPE_HORDE_SNIPER:
			// Snipers have to:
			// - Have a ranged attack.
			// - Not be flying.
			// - Not be a boss monster for this wave.
			// - Be skinny enough to fit in a 128x128 square.
			if (info.missilestate == S_NULL || isFlying || recipe.isBoss || !fitsNormal)
				continue;
			break;
		case TTYPE_HORDE_SMALLSNIPER:
			// Small snipers are similar, but they get to fit
			// into a 64x64 square.
			if (info.missilestate == S_NULL || isFlying || recipe.isBoss || !fitsSmall)
				continue;
			break;
		}

		SpawnPointWeight weight;
		weight.spawn = &*sit;

		// [AM] During development we used to have a complicated spawn system
		//      that spawned near players, but this resulted in it being
		//      easy to exploit.

		float score = 1.0f;
		if (sit->type == TTYPE_HORDE_FLYING)
		{
			// Preferring flying spawns frees up ground-level spawns for
			// ground-level monsters.
			score = 1.25f;
		}
		else if (sit->type == TTYPE_HORDE_SMALLSNIPER || sit->type == TTYPE_HORDE_SNIPER)
		{
			// Sniper spawns can be annoying to clear, allow a breather.
			score = 0.75f;
		}

		weight.score = score;
		totalScore += score;

		weights.push_back(weight);
	}

	// Did we find any spawns?
	if (weights.empty())
	{
		// Error is printed in parent.
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
	if (spawn.type == TTYPE_HORDE_SMALLSNIPER || spawn.type == TTYPE_HORDE_SNIPER ||
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
			SV_SpawnMobj(pack);

			// Play the item respawn sound, so people can listen for it.
			if ((point.mo->flags & MF_AMBUSH) == 0)
			{
				AActor* tele = new AActor(pack->x, pack->y, pack->z, MT_IFOG);
				SV_SpawnMobj(tele);
				S_NetSound(tele, CHAN_VOICE, "misc/spawn", ATTN_IDLE);
			}
		}
	}
}

/**
 * @brief Spawn a powerup at one of the available item spawners.
 */
void P_HordeSpawnPowerup(const mobjtype_t pw)
{
	// Find all empty points.
	hordeSpawns_t emptys;
	for (hordeSpawns_t::iterator it = ::powerupSpawns.begin();
	     it != ::powerupSpawns.end(); ++it)
	{
		if (it->mo->target == NULL)
		{
			emptys.push_back(*it);
		}
	}

	if (emptys.empty())
		return; // No empty spots.

	// Select a random empty spot.
	size_t idx = P_RandomInt(emptys.size());
	hordeSpawn_t& point = emptys.at(idx);
	if (point.mo->target == NULL)
	{
		AActor* pack = new AActor(point.mo->x, point.mo->y, point.mo->z, pw);
		point.mo->target = pack->ptr();

		// Don't respawn the usual way.
		pack->flags |= MF_DROPPED;
		SV_SpawnMobj(pack);

		// Play the item respawn sound, so people can listen for it.
		if ((point.mo->flags & MF_AMBUSH) == 0)
		{
			AActor* tele = new AActor(pack->x, pack->y, pack->z, MT_IFOG);
			SV_SpawnMobj(tele);
			S_NetSound(tele, CHAN_VOICE, "misc/spawn", ATTN_IDLE);
		}
	}
}
