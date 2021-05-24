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

#include "p_horde.h"

#include <algorithm>
#include <math.h>

#include "c_dispatch.h"
#include "c_effect.h"
#include "d_player.h"
#include "doomstat.h"
#include "doomtype.h"
#include "i_system.h"
#include "m_random.h"
#include "m_vectors.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"

bool P_LookForPlayers(AActor* actor, bool allaround);

typedef std::vector<AActor*> AActors;

enum roundMonsterType_e
{
	RM_NULL,
	RM_NORMAL,
	RM_BOSS,
	RM_BOSSONLY,
};

struct roundMonster_t
{
	roundMonsterType_e monster;
	mobjtype_t mobj;
	float chance;
};

struct roundDefine_t
{
	const weapontype_t* weapons;    // Weapons we can spawn this round.
	const roundMonster_t* monsters; // Monsters we can spawn this round.
	int minGroupHealth;             // Minimum health of a group of monsters to spawn.
	int maxGroupHealth;             // Maximum health of a group of monsters to spawn.
	int minTotalHealth; // Lower bound on the amount of health in the map at once, aside
	                    // from round end.
	int maxTotalHealth; // Upper bound on the amount of health in the map at once.
	int goalHealth;     // Base target health to win the round.
};

const weapontype_t R1_WEAPONS[] = {wp_shotgun, wp_chaingun, wp_missile, wp_nochange};
const roundMonster_t R1_MONSTERS[] = {
    // Round 1 - Knee Deep in the Dead
    {RM_NORMAL, MT_POSSESSED, 1.0f},
    {RM_NORMAL, MT_SHOTGUY, 1.0f},
    {RM_NORMAL, MT_TROOP, 1.0f},
    {RM_NORMAL, MT_SERGEANT, 1.0f},
    {RM_NORMAL, MT_SHADOWS, 1.0f},
    {RM_BOSSONLY, MT_BRUISER, 1.0f},
    {RM_NULL}};

const weapontype_t R2_WEAPONS[] = {wp_shotgun, wp_chaingun, wp_supershotgun,
                                   wp_missile, wp_plasma,   wp_nochange};
const roundMonster_t R2_MONSTERS[] = {
    // Round 2 - The Crusher
    {RM_NORMAL, MT_POSSESSED, 1.0f},
    {RM_NORMAL, MT_SHOTGUY, 1.0f},
    {RM_NORMAL, MT_CHAINGUY, 1.0f},
    {RM_NORMAL, MT_TROOP, 1.0f},
    {RM_NORMAL, MT_SKULL, 1.0f},
    {RM_NORMAL, MT_SERGEANT, 1.0f},
    {RM_NORMAL, MT_SHADOWS, 1.0f},
    {RM_NORMAL, MT_UNDEAD, 1.0f},
    {RM_NORMAL, MT_KNIGHT, 1.0f},
    {RM_BOSSONLY, MT_SPIDER, 1.0f},
    {RM_NULL}};

const weapontype_t R3_WEAPONS[] = {wp_shotgun, wp_chaingun, wp_supershotgun, wp_missile,
                                   wp_plasma,  wp_bfg,      wp_nochange};
const roundMonster_t R3_MONSTERS[] = {
    // Round 3 - Go 2 It
    {RM_NORMAL, MT_CHAINGUY, 1.0f},
    {RM_NORMAL, MT_SERGEANT, 1.0f},
    {RM_NORMAL, MT_UNDEAD, 1.0f},
    {RM_NORMAL, MT_PAIN, 0.25f},
    {RM_NORMAL, MT_BRUISER, 1.0f},
    {RM_NORMAL, MT_BABY, 1.0f},
    {RM_NORMAL, MT_FATSO, 1.0f},
    {RM_NORMAL, MT_VILE, 0.25f},
    {RM_BOSS, MT_CYBORG, 0.25f},
    {RM_BOSSONLY, MT_SPIDER, 1.0f},
    {RM_NULL}};

const roundDefine_t ROUND_DEFINES[3] = {
    // Round 1
    {
        R1_WEAPONS,  // weapons
        R1_MONSTERS, // monsters
        150,         // minGroupHealth
        300,         // maxGroupHealth
        600,         // minTotalHealth
        1200,        // maxTotalHealth
        2400,        // goalHealth
    },
    // Round 2
    {
        R2_WEAPONS,  // weapons
        R2_MONSTERS, // monsters
        600,         // minGroupHealth
        1200,        // maxGroupHealth
        2000,        // minTotalHealth
        4800,        // maxTotalHealth
        9600,        // goalHealth
    },
    // Round 3
    {
        R3_WEAPONS,  // weapons
        R3_MONSTERS, // monsters
        1000,        // minGroupHealth
        2000,        // maxGroupHealth
        4000,        // minTotalHealth
        8000,        // maxTotalHealth
        16000,       // goalHealth
    }};

static const char* HordeStateStr(const hordeState_e state)
{
	switch (state)
	{
	case HS_STARTING:
		return "HS_STARTING";
	case HS_PRESSURE:
		return "HS_PRESSURE";
	case HS_RELAX:
		return "HS_RELAX";
	case HS_BOSS:
		return "HS_BOSS";
	default:
		return "UNKNOWN";
	}
}

class HordeRoundState
{
	int m_round; // 0-indexed

  public:
	int getRound() const
	{
		return m_round + 1;
	}

	const roundDefine_t& getDefine() const
	{
		return ROUND_DEFINES[m_round];
	}

	void setRound(const int round)
	{
		m_round = round - 1;
	}
};

class HordeState
{
	hordeState_e m_state;
	int m_stateTime;
	HordeRoundState m_roundState;
	int m_spawnedHealth;
	int m_killedHealth;
	int m_roundStartHealth;
	AActors m_bosses;

	void setState(const hordeState_e state)
	{
		m_state = state;
		m_stateTime = ::gametic;
	}

  public:
	void reset()
	{
		setState(HS_STARTING);
		m_roundState.setRound(1);
		m_spawnedHealth = 0;
		m_killedHealth = 0;
		m_roundStartHealth = 0;
	}

	void next()
	{
		if (m_roundState.getRound() >= ARRAY_LENGTH(ROUND_DEFINES))
		{
			G_ExitLevel(0, 1);
			return;
		}

		setState(HS_STARTING);
		m_roundState.setRound(m_roundState.getRound() + 1);
		setStartHealth();
	}

	/**
	 * @brief Returns health, like for the HUD.
	 */
	hordeInfo_t info() const
	{
		hordeInfo_t info;
		info.state = m_state;
		info.round = m_roundState.getRound();
		info.killed = m_killedHealth - m_roundStartHealth;
		info.goal = m_roundState.getDefine().goalHealth;
		return info;
	}

	void addSpawnHealth(const int health)
	{
		m_spawnedHealth += health;
	}

	void addKilledHealth(const int health)
	{
		m_killedHealth += health;
	}

	HordeRoundState& getRoundState()
	{
		return m_roundState;
	}

	const HordeRoundState& getRoundState() const
	{
		return m_roundState;
	}

	bool hasBosses() const
	{
		return !m_bosses.empty();
	}

	void setBosses(AActors& bosses)
	{
		m_bosses = bosses;
	}

	void setStartHealth()
	{
		m_roundStartHealth = m_killedHealth;
	}

	void stateSwitch()
	{
		const roundDefine_t& define = m_roundState.getDefine();
		int aliveHealth = m_spawnedHealth - m_killedHealth;
		int goalHealth = define.goalHealth + m_roundStartHealth;

		switch (m_state)
		{
		case HS_STARTING: {
			// Do anything that we would need to do upon starting any round.
			m_bosses.clear();
			setState(HS_PRESSURE);
			// fallthrough
		}
		case HS_PRESSURE: {
			if (m_killedHealth > goalHealth)
			{
				setState(HS_BOSS);
				return;
			}
			else if (aliveHealth > define.maxTotalHealth)
			{
				setState(HS_RELAX);
				return;
			}
			return;
		}
		case HS_RELAX: {
			if (m_killedHealth > goalHealth)
			{
				setState(HS_BOSS);
				return;
			}
			else if (aliveHealth < define.minTotalHealth)
			{
				setState(HS_PRESSURE);
				return;
			}
			return;
		case HS_BOSS: {
			size_t alive = 0;
			for (AActors::iterator it = m_bosses.begin(); it != m_bosses.end(); ++it)
			{
				if ((*it)->health > 0)
					alive += 1;
			}
			if (!alive)
			{
				next();
			}
			return;
		}
		}
		default:
			return;
		}
	}

	hordeState_e getState()
	{
		return m_state;
	}
} gDirector;

namespace recipe
{
struct recipe_t
{
	mobjtype_t type;
	int count;
	bool isBoss;
};

/**
 * @brief Get a recipe of monsters to spawn.
 *
 * @param out Recipe to write to.
 * @param define Round information to use.
 * @param type Spawn thing type.
 * @param healthLeft Amount of health left in the group.
 * @param wantBoss Caller wants a boss.
 */
static bool GetSpawnRecipe(recipe_t& out, const roundDefine_t& define,
                           const int healthLeft, const bool wantBoss)
{
	std::vector<const roundMonster_t*> monsters;

	// Figure out which monster we want to spawn.
	for (size_t i = 0;; i++)
	{
		const roundMonster_t& roundMon = define.monsters[i];
		if (roundMon.monster == RM_NULL)
			break;

		const mobjinfo_t& info = ::mobjinfo[roundMon.mobj];

		// Boss spawns have to spawn boss things.
		if (wantBoss && roundMon.monster == RM_NORMAL)
			continue;

		// Non-boss spawns have to spawn non-boss things.
		if (!wantBoss && roundMon.monster == RM_BOSSONLY)
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
		outIsBoss = monsters.at(resultIdx)->monster != RM_NORMAL;

		// Chance is 100%?
		if (monsters.at(resultIdx)->chance == 1.0f)
			break;

		// Chance is within bounds of a random number?
		const float chance = P_RandomFloat();
		if (monsters.at(resultIdx)->chance >= chance)
			break;
	}

	int outCount = 1;
	const int maxHealth = MIN(define.maxGroupHealth, healthLeft);
	const int health = ::mobjinfo[outType].spawnhealth;
	const int upper = clamp(maxHealth, 1, 4);
	const int lower = clamp(define.minGroupHealth / health, 1, 4);
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
}; // namespace recipe

namespace spawn
{
struct SpawnPoint
{
	AActor::AActorPtr mo;
	int type;
};
typedef std::vector<SpawnPoint> SpawnPoints;

struct SpawnPointWeight
{
	SpawnPoint* spawn;
	float score;
	int dist;
	bool visible;
};
typedef std::vector<SpawnPointWeight> SpawnPointWeights;

static SpawnPoints itemSpawns;
static SpawnPoints monsterSpawns;

static bool CmpWeights(const SpawnPointWeight& a, const SpawnPointWeight& b)
{
	return a.score > b.score;
}

/**
 * @brief Get a spawn point to place a monster at.
 */
static SpawnPoint* GetSpawnCandidate(const recipe::recipe_t& recipe)
{
	float totalScore = 0.0f;

	SpawnPointWeights weights;
	for (SpawnPoints::iterator sit = monsterSpawns.begin(); sit != monsterSpawns.end();
	     ++sit)
	{
		// Don't spawn bosses at non-boss spawns.
		if (recipe.isBoss && sit->type != ::TTYPE_HORDE_BOSS)
			continue;
		else if (!recipe.isBoss && sit->type == ::TTYPE_HORDE_BOSS)
			continue;

		mobjinfo_t& info = ::mobjinfo[recipe.type];

		// Flying spawns have to spawn flying monsters.
		const bool isFlying = info.flags & (MF_NOGRAVITY | MF_FLOAT);
		if (sit->type == ::TTYPE_HORDE_FLYING && !isFlying)
			continue;

		// Snipers have to:
		// - Have a ranged attack.
		// - Not be flying.
		// - Not be a boss monster for this round.
		// - Be skinny enough to fit in a 64x64 square.
		if (sit->type == ::TTYPE_HORDE_SNIPER &&
		    (info.missilestate == S_NULL || isFlying || recipe.isBoss ||
		     info.radius > (32 << FRACBITS)))
			continue;

		SpawnPointWeight weight;
		weight.spawn = &*sit;

		// [AM] During development we used to have a complicated spawn system
		//      that spawned near players, but this resulted in it being
		//      easy to exploit.

		float score = 1.0f;
		if (sit->type == ::TTYPE_HORDE_SNIPER || sit->type == ::TTYPE_HORDE_FLYING)
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
 * @brief Spawn a monster.
 *
 * @param point Spawn point of the monster.
 * @param recipe Recipe of a monster to spawn.
 * @param offset Offset from the spawn point to spawn at.
 * @param target Player to target.
 * @return Actor pointer we just spawned, or NULL if the spawn failed.
 */
static AActor* SpawnMonster(spawn::SpawnPoint& spawn, const recipe::recipe_t& recipe,
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

			// Add (possibly adjusted) health to pool.
			P_AddHealthPool(mo);

			// This monster wakes up hating a random player.
			PlayerResults targets = PlayerQuery().hasHealth().execute();
			if (targets.count > 0)
			{
				const size_t idx = P_RandomInt(targets.count);
				mo->target = targets.players.at(idx)->mo->ptr();
				P_SetMobjState(mo, mo->info->seestate, true);
			}

			// Play the see sound if we have one.
			if (mo->info->seesound)
			{
				char sound[MAX_SNDNAME];

				strcpy(sound, mo->info->seesound);

				if (sound[strlen(sound) - 1] == '1')
				{
					sound[strlen(sound) - 1] = P_Random(mo) % 3 + '1';
					if (S_FindSound(sound) == -1)
						sound[strlen(sound) - 1] = '1';
				}

				S_Sound(mo, CHAN_VOICE, sound, 1, ATTN_NORM);
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
 * @return Actors spawned by this function.  Can be discarded.
 */
static AActors SpawnMonsterCount(spawn::SpawnPoint& spawn, const recipe::recipe_t& recipe)
{
	AActors ok;
	const char* mobjname = ::mobjinfo[recipe.type].name;

	// We might need the radius.
	fixed_t rad = ::mobjinfo[recipe.type].radius;

	const int count = spawn.type == ::TTYPE_HORDE_SNIPER ? 1 : recipe.count;

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
		Printf(PRINT_WARNING, "Tried to spawn %d %s.\n", recipe.count, mobjname);
	}

	// Remove unspawned actors - probably spawnblocked.
	AActors ret;
	for (AActors::iterator it = ok.begin(); it != ok.end(); ++it)
	{
		if ((*it) != NULL)
			ret.push_back(*it);
	}

	// If we're blocked and tried to spawn more than 1 monster, try spawning
	// just one as a fallback.
	if (ret.empty() && recipe.count > 1)
	{
		ok.clear();
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(0, 0)));
		if (*ok.begin() != NULL)
			ok.clear();
	}

	return ret;
}

} // namespace spawn

static bool DEBUG_enabled;

hordeInfo_t P_HordeInfo()
{
	return ::gDirector.info();
}

void P_ClearHordeSpawnPoints()
{
	spawn::itemSpawns.clear();
	spawn::monsterSpawns.clear();
}

void P_AddHordeSpawnPoint(AActor* mo, const int type)
{
	spawn::SpawnPoint sp;
	sp.mo = mo->self;
	sp.type = type;
	if (type == ::TTYPE_HORDE_ITEM)
	{
		spawn::itemSpawns.push_back(sp);
	}
	else
	{
		spawn::monsterSpawns.push_back(sp);
	}
}

void P_AddHealthPool(AActor* mo)
{
	// Already added?
	if (mo->oflags & MFO_HEALTHPOOL)
		return;

	// Counts as a monster?
	if (!(mo->flags & MF_COUNTKILL || mo->type == MT_SKULL))
		return;

	// Mark as part of the health pool for cleanup later
	mo->oflags |= MFO_HEALTHPOOL;

	::gDirector.addSpawnHealth(::mobjinfo[mo->type].spawnhealth);
}

void P_RemoveHealthPool(AActor* mo)
{
	// Not a part of the pool
	if (!(mo->oflags & MFO_HEALTHPOOL))
		return;

	::gDirector.addKilledHealth(::mobjinfo[mo->type].spawnhealth);
}

void P_RunHordeTics()
{
	if (::sv_gametype != GM_HORDE)
		return;

	// Move this function to tick inside someplace that can be paused.
	if (::paused)
		return;

	if (::level.time == 0)
		::gDirector.reset();

	// Only run logic once a second.
	if (!P_AtInterval(TICRATE))
		return;

	// Switch states?
	::gDirector.stateSwitch();

	// Should we spawn a monster?
	switch (::gDirector.getState())
	{
	case HS_PRESSURE: {
		const roundDefine_t& define = ::gDirector.getRoundState().getDefine();

		const int pressureHealth =
		    P_RandomInt(define.maxGroupHealth - define.minGroupHealth) +
		    define.minGroupHealth;

		// Limit to 16 loops so we don't get stuck forever.
		int spawned = 0;
		for (int i = 0; i < 16; i++)
		{
			if (spawned >= pressureHealth)
				continue;

			// Pick a recipe for some monsters.
			recipe::recipe_t recipe;
			const bool ok =
			    recipe::GetSpawnRecipe(recipe, define, pressureHealth - spawned, false);
			if (!ok)
				continue;

			// Choose a spawn point.
			spawn::SpawnPoint* spawn = spawn::GetSpawnCandidate(recipe);
			if (spawn == NULL)
				continue;

			AActors actors = spawn::SpawnMonsterCount(*spawn, recipe);
			for (AActors::iterator it = actors.begin(); it != actors.end(); ++it)
			{
				spawned += (*it)->health;
			}
		}
		break;
	}
	case HS_RELAX: {
		// Pick a monster that the player can't see right now and teleport
		// them close to the player.
		break;
	}
	case HS_BOSS: {
		const roundDefine_t& define = ::gDirector.getRoundState().getDefine();

		// Do we already have bosses spawned?
		if (::gDirector.hasBosses())
			break;

		// Pick a recipe for some monsters.
		recipe::recipe_t recipe;
		const bool ok =
		    recipe::GetSpawnRecipe(recipe, define, define.maxGroupHealth, true);
		if (!ok)
			break;

		// Spawn a boss if we don't have one.
		spawn::SpawnPoint* spawn = spawn::GetSpawnCandidate(recipe);
		if (spawn == NULL)
			break;

		AActors actors = spawn::SpawnMonsterCount(*spawn, recipe);
		::gDirector.setBosses(actors);
		break;
	}
	}

	// Should we spawn an item?
	{
		// Find all empty points.
		spawn::SpawnPoints emptys;
		for (spawn::SpawnPoints::iterator it = spawn::itemSpawns.begin();
		     it != spawn::itemSpawns.end(); ++it)
		{
			if (it->mo->target == NULL)
			{
				emptys.push_back(*it);
			}
		}

		// Items should exist at around half of our points.
		if (emptys.size() > spawn::itemSpawns.size() / 2)
		{
			// Select a random spot.
			size_t idx = P_RandomInt(emptys.size());
			spawn::SpawnPoint& point = emptys.at(idx);
			if (point.mo->target == NULL)
			{
				AActor* pack =
				    new AActor(point.mo->x, point.mo->y, point.mo->z, MT_CAREPACK);
				point.mo->target = pack->ptr();

				// Don't respawn the usual way.
				pack->flags |= MF_DROPPED;

				// Play the item respawn sound, so people can listen for it.
				AActor* tele = new AActor(pack->x, pack->y, pack->z, MT_IFOG);
				S_Sound(pack, CHAN_VOICE, "misc/spawn", 1, ATTN_IDLE);
			}
		}
	}
}

bool P_IsHordeMode()
{
	return sv_gametype == GM_HORDE;
}

bool P_IsHordeThing(const int type)
{
	return type >= TTYPE_HORDE_ITEM && type <= TTYPE_HORDE_SNIPER;
}

const weapontype_t* P_RoundWeapons()
{
	if (sv_gametype == GM_HORDE)
	{
		return ::gDirector.getRoundState().getDefine().weapons;
	}
	else
	{
		const weapontype_t weapons[] = {wp_shotgun, wp_chaingun, wp_supershotgun,
		                                wp_missile, wp_plasma,   wp_bfg,
		                                wp_nochange};
		return weapons;
	}
}

BEGIN_COMMAND(horde_round)
{
	if (argc < 2)
	{
		Printf("horde_round <ROUND_NUMBER>\n");
		return;
	}

	int round = atoi(argv[1]);
	if (round < 1 || round > 3)
	{
		Printf("Round must be between 1-3.\n");
		return;
	}

	::gDirector.getRoundState().setRound(round);
	::gDirector.setStartHealth();
}
END_COMMAND(horde_round)
