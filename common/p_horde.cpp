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

const int SPAWN_BOSS = BIT(0);
const int SPAWN_AMBUSH = BIT(1);

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
	const roundMonster_t* monsters; // Monsters we can spawn this round.
	int minGroupHealth;             // Minimum health of a group of monsters to spawn.
	int maxGroupHealth;             // Maximum health of a group of monsters to spawn.
	int minTotalHealth; // Lower bound on the amount of health in the map at once, aside
	                    // from round end.
	int maxTotalHealth; // Upper bound on the amount of health in the map at once.
	int goalHealth;     // Base target health to win the round.
};

const roundMonster_t R1_MONSTERS[] = {
    // Round 1 - Knee Deep in the Dead
    {RM_NORMAL, MT_POSSESSED, 1.0f},
    {RM_NORMAL, MT_SHOTGUY, 1.0f},
    {RM_NORMAL, MT_TROOP, 1.0f},
    {RM_NORMAL, MT_SERGEANT, 1.0f},
    {RM_NORMAL, MT_SHADOWS, 1.0f},
    {RM_BOSSONLY, MT_BRUISER, 1.0f},
    {RM_NULL}};

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

const roundMonster_t R3_MONSTERS[] = {
    // Round 3 - Go 2 It
    {RM_NORMAL, MT_CHAINGUY, 1.0f},
    {RM_NORMAL, MT_SERGEANT, 1.0f},
    {RM_NORMAL, MT_UNDEAD, 1.0f},
    {RM_NORMAL, MT_PAIN, 1.0f},
    {RM_NORMAL, MT_BRUISER, 1.0f},
    {RM_NORMAL, MT_BABY, 1.0f},
    {RM_NORMAL, MT_FATSO, 1.0f},
    {RM_NORMAL, MT_VILE, 1.0f},
    {RM_BOSS, MT_CYBORG, 0.25f},
    {RM_BOSSONLY, MT_SPIDER, 1.0f},
    {RM_NULL}};

const roundDefine_t ROUND_DEFINES[3] = {
    // Round 1
    {
        R1_MONSTERS, // monsters
        150,         // minGroupHealth
        300,         // maxGroupHealth
        600,         // minTotalHealth
        1200,        // maxTotalHealth
        2400,        // goalHealth
    },
    // Round 2
    {
        R2_MONSTERS, // monsters
        600,         // minGroupHealth
        1200,        // maxGroupHealth
        2000,        // minTotalHealth
        4800,        // maxTotalHealth
        9600,        // goalHealth
    },
    // Round 3
    {
        R3_MONSTERS, // monsters
        1000,        // minGroupHealth
        2000,        // maxGroupHealth
        4000,        // minTotalHealth
        8000,        // maxTotalHealth
        16000,       // goalHealth
    }};

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
		info.spawned = m_spawnedHealth;
		info.killed = m_killedHealth;
		info.goal = m_roundState.getDefine().goalHealth + m_roundStartHealth;
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

		switch (m_state)
		{
		case HS_STARTING:
			setState(HS_PRESSURE);
		case HS_PRESSURE: {
			if (m_killedHealth > define.goalHealth + m_roundStartHealth)
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
			if (m_killedHealth > define.goalHealth + m_roundStartHealth)
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
};

/**
 * @brief Get a recipe of monsters to spawn.
 *
 * @param out Recipe to write to.
 * @param define Round information to use.
 * @param flags Spawn flag classifications to allow.
 * @param healthLeft Amount of health left in the group.
 */
static bool GetSpawnRecipe(recipe_t& out, const roundDefine_t& define, const int type,
                           const int flags, const int healthLeft)
{
	const bool wantBoss = flags & ::SPAWN_BOSS;

	std::vector<const roundMonster_t*> monsters;

	// Figure out which monster we want to spawn.
	for (size_t i = 0;; i++)
	{
		const roundMonster_t& roundMon = define.monsters[i];
		if (roundMon.monster == RM_NULL)
			break;

		const mobjinfo_t& info = ::mobjinfo[roundMon.mobj];

		// Boss spawns have to spawn boss things.
		if (type == ::TTYPE_HORDE_BOSS && roundMon.monster == RM_NORMAL)
			continue;

		// Non-boss spawns have to spawn non-boss things.
		if (type != ::TTYPE_HORDE_BOSS && roundMon.monster == RM_BOSSONLY)
			continue;

		// Flying spawns have to spawn flying monsters.
		if (type == ::TTYPE_HORDE_FLYING && !(info.flags & (MF_NOGRAVITY | MF_FLOAT)))
			continue;

		// Snipers have to have a ranged attack.
		if (type == ::TTYPE_HORDE_SNIPER &&
		    (info.missilestate == S_NULL || roundMon.mobj == MT_SKULL))
			continue;

		monsters.push_back(&roundMon);
	}

	// No valid monsters can be spawned at this point.
	if (monsters.empty())
		return false;

	for (size_t i = 0; i < 8; i++)
	{
		size_t resultIdx = P_RandomInt(monsters.size());
		out.type = monsters.at(resultIdx)->mobj;

		// Chance is 100%?
		if (monsters.at(resultIdx)->chance == 1.0f)
			break;

		// Chance is within bounds of a random number?
		float chance = P_RandomFloat();
		if (monsters.at(resultIdx)->chance >= chance)
			break;
	}

	// Figure out how many monsters we can spawn of our given type - at least one.
	const int maxHealth = MIN(define.maxGroupHealth, healthLeft);
	const int health = ::mobjinfo[out.type].spawnhealth;
	const int upper = clamp(maxHealth, 1, 4);
	const int lower = clamp(define.minGroupHealth / health, 1, 4);
	if (upper <= lower)
	{
		// Only one possibility.
		out.count = upper;
		return true;
	}

	out.count = P_RandomInt(upper - lower) + lower;
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

static SpawnPoints spawns;

static bool CmpWeights(const SpawnPointWeight& a, const SpawnPointWeight& b)
{
	return a.score > b.score;
}

/**
 * @brief Get a spawn point to place a monster at.
 */
static SpawnPoint* GetSpawnCandidate(player_t* player, const int flags)
{
	const bool boss = flags & ::SPAWN_BOSS;

	float totalScore = 0.0f;

	SpawnPointWeights weights;
	for (SpawnPoints::iterator sit = spawns.begin(); sit != spawns.end(); ++sit)
	{
		// Don't spawn bosses at non-boss spawns.
		if (boss && sit->type != TTYPE_HORDE_BOSS)
			continue;
		else if (!boss && sit->type == TTYPE_HORDE_BOSS)
			continue;

		SpawnPointWeight weight;
		weight.spawn = &*sit;

		int dist = P_AproxDistance2(sit->mo, player->mo) >> FRACBITS;
		bool visible = P_CheckSight(sit->mo, player->mo);

		weight.dist = dist;
		weight.visible = visible;

		// Base score is based on distance.
		float score;
		if (dist < 192)
			score = 0.125f; // Try not to spawn close by.
		else if (dist < 512)
			score = 1.0f;
		else if (dist < 1024)
			score = 0.75f;
		else if (dist < 2048)
			score = 0.5f;
		else if (dist < 3072)
			score = 0.25f;
		else
			score = 0.125f;

		// Having LoS gives a significant bonus, because intuitively there
		// are many more spanws in a map you can't see vs those you can.
		score *= visible ? 1.0f : 0.5f;
		weight.score = score;
		totalScore += score;

		weights.push_back(weight);
	}

	// Did we find any spawns?
	if (weights.empty())
	{
		Printf(PRINT_WARNING, "Could not find a spawn point (boss:%s).\n",
		       boss ? "y" : "n");
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
 * @param flags SPAWN_BOSS spawns boss monster, SPAWN_AMBUSH spawns monster
 *              without a spawn flash or see sound.
 * @return Actor pointer we just spawned, or NULL if the spawn failed.
 */
static AActor* SpawnMonster(spawn::SpawnPoint& spawn, const recipe::recipe_t& recipe,
                            const v2fixed_t offset, player_t* target, const int flags)
{
	const bool boss = flags & ::SPAWN_BOSS;
	const bool ambush = flags & ::SPAWN_AMBUSH;

	AActor* mo = new AActor(spawn.mo->x + offset.x, spawn.mo->y + offset.y, spawn.mo->z,
	                        recipe.type);
	if (mo)
	{
		if (P_TestMobjLocation(mo))
		{
			P_AddHealthPool(mo);

			// Don't respawn
			mo->flags |= MF_DROPPED;

			if (boss)
			{
				// Purple is the noblest shroud.
				mo->effects = FX_PURPLEFOUNTAIN;
			}

			// This monster wakes up hating you and knows where you live.
			mo->target = target->mo->ptr();
			P_SetMobjState(mo, mo->info->seestate, true);

			// Play the see sound if we have one and it's not an ambush.
			if (!ambush && mo->info->seesound)
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

			if (!ambush)
			{
				// Spawn a teleport fog if it's not an ambush.
				AActor* tele = new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z, MT_TFOG);
				S_Sound(tele, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
			}
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
 * @param target Player to target.
 * @param flags Flags to pass to SpawnMonster.
 * @return Actors spawned by this function.  Can be discarded.
 */
static AActors SpawnMonsterCount(spawn::SpawnPoint& spawn, const recipe::recipe_t& recipe,
                                 player_t* target, const int flags)
{
	AActors ok;
	const char* mobjname = ::mobjinfo[recipe.type].name;

	// We might need the radius.
	fixed_t rad = ::mobjinfo[recipe.type].radius;

	if (recipe.count == 4)
	{
		// A big square.
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, -rad), target, flags));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, -rad), target, flags));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, rad), target, flags));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, rad), target, flags));
	}
	else if (recipe.count == 3)
	{
		// A wedge.
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, -rad), target, flags));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, -rad), target, flags));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(0, rad), target, flags));
	}
	else if (recipe.count == 2)
	{
		// Next to each other.
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(-rad, 0), target, flags));
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(rad, 0), target, flags));
	}
	else if (recipe.count == 1)
	{
		// All by themselves :(
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(0, 0), target, flags));
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
		ok.push_back(SpawnMonster(spawn, recipe, v2fixed_t(0, 0), target, flags));
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
	spawn::spawns.clear();
}

void P_AddHordeSpawnPoint(AActor* mo, const int type)
{
	spawn::SpawnPoint sp;
	sp.mo = mo->self;
	sp.type = type;
	spawn::spawns.push_back(sp);
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

			// Pick the unlucky SOB who is about to get spawned near.
			PlayerResults pr = PlayerQuery().hasHealth().execute();
			if (!pr.count)
				break;
			player_t* target = pr.players.at(P_RandomInt(pr.count));

			// Choose a spawn point.
			spawn::SpawnPoint* spawn = spawn::GetSpawnCandidate(target, 0);
			if (spawn == NULL)
				continue;

			// Spawn some monsters.
			recipe::recipe_t recipe;
			const bool ok = recipe::GetSpawnRecipe(recipe, define, spawn->type, 0,
			                                       pressureHealth - spawned);
			if (!ok)
				continue;

			AActors actors = spawn::SpawnMonsterCount(*spawn, recipe, target, 0);
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
		// Do we already have bosses spawned?
		if (::gDirector.hasBosses())
			break;

		// Spawn a boss if we don't have one near a living player.
		PlayerResults pr = PlayerQuery().hasHealth().execute();
		if (!pr.count)
			break;
		player_t* target = pr.players.at(P_RandomInt(pr.count));

		spawn::SpawnPoint* spawn = spawn::GetSpawnCandidate(target, ::SPAWN_BOSS);
		if (spawn == NULL)
			break;

		const roundDefine_t& define = ::gDirector.getRoundState().getDefine();

		recipe::recipe_t recipe;
		const bool ok = recipe::GetSpawnRecipe(recipe, define, spawn->type, ::SPAWN_BOSS,
		                                       define.maxGroupHealth);
		if (!ok)
			break;

		AActors actors = spawn::SpawnMonsterCount(*spawn, recipe, target, ::SPAWN_BOSS);
		::gDirector.setBosses(actors);
		break;
	}
	}
}

bool P_IsHordeMode()
{
	return sv_gametype == GM_HORDE;
}

bool P_IsHordeThing(const int type)
{
	return type >= TTYPE_HORDE_MONSTER && type <= TTYPE_HORDE_SNIPER;
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
