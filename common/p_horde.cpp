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

#include "odamex.h"

#include <math.h>

#include "p_horde.h"

#include "c_dispatch.h"
#include "d_player.h"
#include "doomstat.h"
#include "g_gametype.h"
#include "g_horde.h"
#include "i_net.h"
#include "m_random.h"
#include "p_hordespawn.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"
#include "svc_message.h"

#if defined(SERVER_APP)
#include "sv_main.h"
#endif

EXTERN_CVAR(g_horde_waves)
EXTERN_CVAR(g_lives)
EXTERN_CVAR(g_horde_spawnempty_min)
EXTERN_CVAR(g_horde_spawnempty_max)
EXTERN_CVAR(g_horde_spawnfull_min)
EXTERN_CVAR(g_horde_spawnfull_max)
EXTERN_CVAR(sv_nomonsters)

void A_PainDie(AActor* actor);

const int HORDE_STARTING_TICS = TICRATE * 3;

/**
 * @brief Garbage-collector for Horde corpses.
 *
 * @details We want to destroy corpses from previous waves so the map isn't
 *          overwhelmed with corpses.
 */
class corpseCollector_t
{
	typedef std::queue<AActor::AActorPtr> AActorQueue;

	AActorQueue m_corpses;
	size_t m_waveStart;
	size_t m_added;
	size_t m_removed;

	void popCorpse()
	{
		m_corpses.pop();
		m_removed += 1;
	}

  public:
	corpseCollector_t() : m_waveStart(0), m_added(0), m_removed(0) { }
	void clear()
	{
		m_corpses = AActorQueue();
		m_waveStart = 0;
		m_added = 0;
		m_removed = 0;
	}
	void pushCorpse(AActor* ptr)
	{
		m_corpses.push(ptr->ptr());
		m_added += 1;
	}
	void startWave()
	{
		m_waveStart = m_added;
	}
	void tick()
	{
		if (m_removed >= m_waveStart)
		{
			// Everything in the queue is from the current wave.
			return;
		}

		AActor::AActorPtr& actor = m_corpses.front();
		if (!actor)
		{
			// Actor is destroyed, just pop it.
			popCorpse();
			return;
		}

		if (actor->health > 0)
		{
			// Actor is not a corpse anymore, just pop it.
			popCorpse();
			return;
		}

		// Destroy the actor before we pop it.
		actor->Destroy();
		popCorpse();
	}
};

/**
 * @brief Wake up all the passed monsters.
 *
 * @details This is in a separate function because if we activated monsters
 *          immediately upon spawning, their activation state causes them to
 *          move a tiny bit, which can spawn block other monsters.
 */
static void ActivateMonsters(AActors& mobjs)
{
	for (AActors::iterator it = mobjs.begin(); it != mobjs.end(); ++it)
	{
		AActor* mo = *it;

		// Add them health pool now, since the function to do this is
		// in this TU anyway.
		P_AddHealthPool(mo);

		// Hate a random player.
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

			S_NetSound(mo, CHAN_VOICE, sound, ATTN_NORM);
		}
	}
}

//static const char* HordeStateStr(const hordeState_e state)
//{
//	switch (state)
//	{
//	case HS_STARTING:
//		return "HS_STARTING";
//	case HS_PRESSURE:
//		return "HS_PRESSURE";
//	case HS_RELAX:
//		return "HS_RELAX";
//	case HS_WANTBOSS:
//		return "HS_WANTBOSS";
//	default:
//		return "UNKNOWN";
//	}
//}

class HordeState
{
	hordeState_e m_state;
	int m_wave;
	int m_waveTime;
	int m_bossTime; // If == waveTime, boss not spawned.
	size_t m_defineID;
	int m_spawnedHealth;
	int m_killedHealth;
	int m_bossHealth;
	int m_bossDamage;
	int m_waveStartHealth;
	AActors m_bosses;
	hordeRecipe_t m_bossRecipe;
	int m_nextSpawn;
	int m_nextPowerup;
	corpseCollector_t m_corpses;

	/**
	 * @brief Set the given state.
	 */
	void setState(const hordeState_e state)
	{
		m_state = state;

		if (m_state == HS_WANTBOSS)
		{
			// Do the boss intro fanfare.
			SV_BroadcastPrintf("The floor trembles as the boss of the wave arrives.\n");
			S_NetSound(NULL, CHAN_GAMEINFO, "misc/horde/boss", ATTN_NONE);
			m_bossTime = ::level.time;
		}
	}

  public:
	HordeState()
	    : m_state(HS_STARTING), m_wave(0), m_waveTime(0), m_bossTime(0), m_defineID(0),
	      m_spawnedHealth(0), m_killedHealth(0), m_bossHealth(0), m_bossDamage(0),
	      m_waveStartHealth(0), m_bossRecipe(hordeRecipe_t()), m_nextSpawn(0),
	      m_nextPowerup(0)
	{
	}

	/**
	 * @brief Reset director state.
	 */
	void reset(bool printWave)
	{
		setState(HS_STARTING);
		m_wave = 1;
		m_waveTime = ::level.time;
		m_bossTime = ::level.time;
		m_defineID = P_HordePickDefine(m_wave, ::g_horde_waves);
		m_spawnedHealth = 0;
		m_killedHealth = 0;
		m_bossHealth = 0;
		m_bossDamage = 0;
		m_waveStartHealth = 0;
		m_bosses.clear();
		m_bossRecipe.clear();
		m_nextSpawn = ::level.time;
		m_nextPowerup = ::level.time + (30 * TICRATE);
		m_corpses.clear();

		// Avoid printing wave name on boot and map switch.
		if (printWave)
		{
			SV_BroadcastPrintf("Wave %d: \"%s\"\n", m_wave,
			                   G_HordeDefine(m_defineID).name.c_str());
		}
	}

	/**
	 * @brief Start the given wave.
	 */
	void nextWave()
	{
		// The server can have lives, and if that's the case we want
		// to bring back dead players.
		if (G_IsLivesGame())
		{
			// Give all ingame players an extra life for beating the wave.
			PlayersView ingame = PlayerQuery().execute().players;
			for (PlayersView::iterator it = ingame.begin(); it != ingame.end(); ++it)
			{
				// Dead players are reborn with a message.
				if ((*it)->lives <= 0)
				{
					(*it)->playerstate = PST_REBORN;
					SV_BroadcastPrintf("%s gets a new lease on life.\n",
					                   (*it)->userinfo.netname.c_str());

					// Send a res sound directly to this player.
					S_PlayerSound(*it, NULL, CHAN_INTERFACE, "misc/plraise",
					              ATTN_NONE);
				}
				// Give everyone an extra life.
				if ((*it)->lives < g_lives)
				{
					(*it)->lives += 1;
					MSG_WriteSVC(&(*it)->client.reliablebuf, SVC_PlayerInfo(**it));
					MSG_BroadcastSVC(CLBUF_RELIABLE,
					                 SVC_PlayerMembers(**it, SVC_PM_LIVES),
					                 (*it)->id);
				}
			}

		#ifdef SERVER_APP
			// Service the join queue and give all of the freshly-ingame
			// players a single life to start with.
			PlayersView queued = SpecQuery().onlyInQueue().execute();
			SV_UpdatePlayerQueuePositions(G_CanJoinGameStart, NULL);
			for (PlayersView::iterator it = queued.begin(); it != queued.end(); ++it)
			{
				(*it)->lives = 1;
				MSG_WriteSVC(&(*it)->client.reliablebuf, SVC_PlayerInfo(**it));
				MSG_BroadcastSVC(CLBUF_RELIABLE,
				                 SVC_PlayerMembers(**it, SVC_PM_LIVES), (*it)->id);
			}
		#endif
		}


		if (::g_horde_waves && m_wave >= ::g_horde_waves)
		{
			// All monsters explode!  Woo!
			AActor* actor;
			TThinkerIterator<AActor> iterator;
			while ((actor = iterator.Next()))
			{
				if (actor->oflags & MFO_HEALTHPOOL || actor->type == MT_SKULL)
				{
					// [AM] Mostly copypasted from the massacre cheat.
					if (actor->health > 0)
					{
						P_DamageMobj(actor, NULL, NULL, 10000, MOD_UNKNOWN);
					}
					if (actor->type == MT_PAIN)
					{
						A_PainDie(actor);
						P_SetMobjState(actor, S_PAIN_DIE6);
					}
				}
			}

			G_EndGame();
			return;
		}

		setState(HS_STARTING);
		m_wave += 1;
		m_waveTime = ::level.time;
		m_bossTime = ::level.time;
		m_defineID = P_HordePickDefine(m_wave, ::g_horde_waves);
		m_waveStartHealth = m_killedHealth;
		m_bossHealth = 0;
		m_bossDamage = 0;
		m_bosses.clear();
		m_bossRecipe.clear();
		m_corpses.startWave();

		SV_BroadcastPrintf("Wave %d: \"%s\"\n", m_wave,
		                   G_HordeDefine(m_defineID).name.c_str());
	}

	/**
	 * @brief Force a wave with a partial name match.
	 *
	 * @param name Partial name to match against.
	 * @return True if the wave was forced, otherwise false.
	 */
	bool forceWave(const std::string& name)
	{
		int defineID;
		if (!P_HordeDefineNamed(defineID, name))
			return false;

		setState(HS_STARTING);
		m_waveTime = ::level.time;
		m_bossTime = ::level.time;
		m_defineID = defineID;
		m_waveStartHealth = m_killedHealth;
		m_bossHealth = 0;
		m_bossDamage = 0;
		m_bosses.clear();
		m_bossRecipe.clear();
		m_corpses.startWave();

		SV_BroadcastPrintf("Wave %d: \"%s\"\n", m_wave,
		                   G_HordeDefine(m_defineID).name.c_str());
		return true;
	}

	/**
	 * @brief Force the boss to spawn.
	 *
	 * @return True if the state was switched successfully.
	 */
	bool forceBoss()
	{
		if (m_bosses.empty())
		{
			setState(HS_WANTBOSS);
			return true;
		}
		return false;
	}

	/**
	 * @brief Serialize horde data into POD struct.
	 */
	hordeInfo_t serialize() const
	{
		hordeInfo_t info;
		info.state = m_state;
		info.wave = m_wave;
		info.waveTime = m_waveTime;
		info.bossTime = m_bossTime;
		info.defineID = m_defineID;
		info.legacyID = G_HordeDefine(m_defineID).legacyID;
		info.spawnedHealth = m_spawnedHealth;
		info.killedHealth = m_killedHealth;
		info.bossHealth = m_bossHealth;
		info.bossDamage = m_bossDamage;
		info.waveStartHealth = m_waveStartHealth;
		return info;
	}

	/**
	 * @brief Unserialize horde data from a POD struct.
	 */
	void unserialize(const hordeInfo_t& info)
	{
		m_state = info.state;
		m_wave = info.wave;
		m_waveTime = info.waveTime;
		m_bossTime = info.bossTime;
		m_defineID = info.defineID;
		m_spawnedHealth = info.spawnedHealth;
		m_killedHealth = info.killedHealth;
		m_bossHealth = info.bossHealth;
		m_bossDamage = info.bossDamage;
		m_waveStartHealth = info.waveStartHealth;
	}

	/**
	 * @brief Clear the boss array and re-check all actors for boss flags.
	 *
	 * @detail Used for loading savegames.
	 */
	void rescanBosses()
	{
		AActor* mo;
		TThinkerIterator<AActor> iterator;

		m_bosses.clear();
		while ((mo = iterator.Next()))
		{
			if (mo->oflags & MFO_BOSSPOOL)
			{
				m_bosses.push_back(mo->ptr());
			}
		}
	}

	void addSpawnHealth(const int health)
	{
		m_spawnedHealth += health;
	}

	void addKilledHealth(const int health)
	{
		m_killedHealth += health;
	}

	void addBossHealth(const int health)
	{
		m_bossHealth += health;
	}

	void addBossDamage(const int health)
	{
		m_bossDamage += health;
	}

	void addCorpse(AActor* mo)
	{
		m_corpses.pushCorpse(mo);
	}

	size_t getDefineID() const
	{
		return m_defineID;
	}

	int getAliveHealth() const { return m_spawnedHealth - m_killedHealth; }
	void changeState();
	void getNextSpawnTime(int& min, int& max);
	void tick();
} g_HordeDirector;

/**
 * @brief Run functionality that decides if we need to switch states.
 */
void HordeState::changeState()
{
	const hordeDefine_t& define = G_HordeDefine(m_defineID);

	const int goalHealth = define.goalHealth() + m_waveStartHealth;

	switch (m_state)
	{
	case HS_STARTING: {
		if (::level.time > m_waveTime + (3 * TICRATE))
		{
			// We've given enough of a head-start.
			setState(HS_PRESSURE);
		}
		return;
	}
	case HS_PRESSURE: {
		if (m_killedHealth > goalHealth && m_bosses.empty())
		{
			// We reached the goal, spawn the boss.
			setState(HS_WANTBOSS);
		}
		return;
	}
	case HS_RELAX: {
		if (m_killedHealth > goalHealth && m_bosses.empty())
		{
			// We reached the goal, spawn the boss.
			setState(HS_WANTBOSS);
		}
		else
		{
			// There's not enough monsters in the level, add more.
			setState(HS_PRESSURE);
		}
		return;
	case HS_WANTBOSS: {
		if (m_bossRecipe.isValid() && static_cast<int>(m_bosses.size()) >= m_bossRecipe.count)
		{
			// Doesn't matter which state we enter, but we're more likely
			// to be in the relax state after spawning a big hunk of HP.
			setState(HS_PRESSURE);
		}
		return;
	}
	}
	default:
		return;
	}
}

/**
 * @brief Calculate the next spawn time for a monster group.
 *
 * @param min Minimum spawn time, in seconds.
 * @param max Maximum spawn time, in seconds.
 */
void HordeState::getNextSpawnTime(int& min, int& max)
{
	const hordeDefine_t& define = G_HordeDefine(::g_HordeDirector.getDefineID());

	const double EMPTY_MIN_SPAWN = g_horde_spawnempty_min;
	const double EMPTY_MAX_SPAWN = g_horde_spawnempty_max;
	const double FULL_MIN_SPAWN = g_horde_spawnfull_min;
	const double FULL_MAX_SPAWN = g_horde_spawnfull_max;

	// Minimum/maximum monster spawn time.
	int alive = getAliveHealth();

	// Slow down the spawn rate if we have bosses.
	if (!m_bosses.empty())
	{
		alive += (define.maxTotalHealth() - alive) / 2;
	}

	double minf = Remap(alive, define.minTotalHealth(),
	                    define.maxTotalHealth(), EMPTY_MIN_SPAWN, EMPTY_MAX_SPAWN);
	double maxf = Remap(alive, define.minTotalHealth(),
	                    define.maxTotalHealth(), FULL_MIN_SPAWN, FULL_MAX_SPAWN);

	// Don't absolutely pound the players in the first minute.
	const int FALLOFF_TIME = m_waveTime + ::HORDE_STARTING_TICS + TICRATE * 60;
	if (::level.time < FALLOFF_TIME)
	{
		const double falloff = Remap(::level.time, m_waveTime, FALLOFF_TIME, 1.0, 0.0);
		const double floormin = Remap(falloff, 0.0, 1.0, 0.0, EMPTY_MIN_SPAWN);
		const double floormax = Remap(falloff, 0.0, 1.0, 0.0, EMPTY_MAX_SPAWN);
		minf = MAX(minf, floormin);
		maxf = MAX(maxf, floormax);
	}

	// Turn into integers.
	min = MAX(int(round(minf)), 1);
	max = MAX(int(round(maxf)), 1);
	max = MAX(max, min);
}

/**
 * @brief Run the state logic itself.
 */
void HordeState::tick()
{
	const hordeDefine_t& define = G_HordeDefine(m_defineID);

	// Run GC on monster corpses.
	m_corpses.tick();

	// Are the bosses taken care of?
	if (m_state != HS_WANTBOSS && m_bossRecipe.isValid())
	{
		size_t alive = 0;
		for (AActors::iterator it = m_bosses.begin(); it != m_bosses.end(); ++it)
		{
			if (*it && (*it)->health > 0)
				alive += 1;
		}
		if (!alive)
		{
			// Start the next wave.
			nextWave();
			return;
		}
	}

	if (::level.time >= m_nextSpawn && getAliveHealth() <= define.maxTotalHealth())
	{
		int min, max;
		getNextSpawnTime(min, max);

		// Randomly select our next spawn time.
		const int offset = P_RandomInt(max - min + 1) + min;
		m_nextSpawn = ::level.time + (offset * TICRATE);

		// Should we spawn a monster?
		switch (m_state)
		{
		case HS_PRESSURE: {
			if (sv_nomonsters)
				break;

			// Pick a recipe for some monsters.
			hordeRecipe_t recipe;
			const bool ok = P_HordeSpawnRecipe(recipe, define, false);
			if (!ok)
			{
				Printf(PRINT_WARNING, "%s: No spawn recipe for monster.\n", __FUNCTION__);
				return;
			}

			// Choose a spawn point.
			hordeSpawn_t* spawn = P_HordeSpawnPoint(recipe);
			if (spawn == NULL)
			{
				Printf(PRINT_WARNING, "%s: Can't find a place to spawn %s.\n",
				       __FUNCTION__, ::mobjinfo[recipe.type].name);
				return;
			}

			const int hp = ::mobjinfo[recipe.type].spawnhealth * recipe.count;
			DPrintf("Spawning %d %s (%d hp) at a %s spawn\n", recipe.count,
			        ::mobjinfo[recipe.type].name, hp, HordeThingStr(spawn->type));

			AActors mobjs = P_HordeSpawn(*spawn, recipe);
			ActivateMonsters(mobjs);
			break;
		}
		case HS_RELAX:
			break;
		case HS_WANTBOSS: {
			if (sv_nomonsters)
				break;

			// Do we already have bosses spawned?
			if (m_bossRecipe.isValid() && static_cast<int>(m_bosses.size()) >= m_bossRecipe.count)
				break;

			hordeRecipe_t recipe;
			if (!m_bossRecipe.isValid())
			{
				// Pick a recipe for the boss.
				const bool ok = P_HordeSpawnRecipe(recipe, define, true);
				if (!ok)
				{
					Printf(PRINT_WARNING, "%s: No spawn recipe for boss monster.\n",
					       __FUNCTION__);
					break;
				}

				// Save for later - in case we need to spawn twice.
				m_bossRecipe = recipe;
			}
			else
			{
				// Adjust the count based on how many bosses we've spawned.
				recipe = m_bossRecipe;
				recipe.count = m_bossRecipe.count - int(m_bosses.size());
			}

			// Spawn a boss if we don't have one.
			hordeSpawn_t* spawn = P_HordeSpawnPoint(recipe);
			if (spawn == NULL)
			{
				Printf(PRINT_WARNING, "%s: Can't find a place to spawn %s.\n",
				       __FUNCTION__, ::mobjinfo[recipe.type].name);
				break;
			}

			AActors mobjs = P_HordeSpawn(*spawn, recipe);
			m_bosses.insert(m_bosses.end(), mobjs.begin(), mobjs.end());
			ActivateMonsters(mobjs);
			break;
		}
		case HS_STARTING:
			break;
		}
	}

	// Always try to spawn an item.
	P_HordeSpawnItem();

	// Always try to spawn a powerup between 30-45 seconds.
	if (!define.powerups.empty() && ::level.time >= m_nextPowerup)
	{
		const int offset = P_RandomInt(16) + 30;
		m_nextPowerup = ::level.time + (offset * TICRATE);

		const mobjtype_t pw = define.randomPowerup().mobj;
		P_HordeSpawnPowerup(pw);
	}
}

void P_InitHorde()
{
	::g_HordeDirector.reset(false);
}

void P_NextSpawnTime(int& min, int& max)
{
	::g_HordeDirector.getNextSpawnTime(min, max);
}

hordeInfo_t P_HordeInfo()
{
	return ::g_HordeDirector.serialize();
}

void P_SetHordeInfo(const hordeInfo_t& info)
{
	::g_HordeDirector.unserialize(info);
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

	::g_HordeDirector.addSpawnHealth(::mobjinfo[mo->type].spawnhealth);

	// Bosses also have health added to a separate pool for display purposes.
	if (mo->oflags & MFO_BOSSPOOL)
	{
		::g_HordeDirector.addBossHealth(::mobjinfo[mo->type].spawnhealth);
	}
}

void P_RemoveHealthPool(AActor* mo)
{
	// Not a part of the pool
	if (!(mo->oflags & MFO_HEALTHPOOL))
		return;

	// Unset the flag - we only get one try.
	mo->oflags &= ~MFO_HEALTHPOOL;

	::g_HordeDirector.addKilledHealth(::mobjinfo[mo->type].spawnhealth);
}

void P_AddDamagePool(AActor* mo, const int damage)
{
	// Not a part of the pool
	if (!(mo->oflags & MFO_BOSSPOOL))
		return;

	// Counts as a monster?
	if (!(mo->flags & MF_COUNTKILL || mo->type == MT_SKULL))
		return;

	::g_HordeDirector.addBossDamage(damage);
}

void P_QueueCorpseForDestroy(AActor* mo)
{
	// Only needed for horde.
	if (!G_IsHordeMode())
		return;

	::g_HordeDirector.addCorpse(mo);
}

void P_RunHordeTics()
{
	if (!G_IsHordeMode())
		return;

	// Move this function to tick inside someplace that can be paused.
	if (::paused)
		return;

	if (::level.time == 0)
	{
		::g_HordeDirector.reset(true);
	}

	// Add our spawns if a level reload or reset erased our previous spawns.
	if (!P_HordeHasSpawns())
	{
		P_HordeAddSpawns();
		if (!P_HordeHasSpawns() || !P_HordeHasRequiredMonsterSpawns())
		{
			if (::level.time == 0)
			{
				Printf(
				    PRINT_WARNING,
				    "WARNING: This map is missing Horde Monster, Horde Boss, Horde Supply Cache, "
				    "or Horde Powerup spawns.  At least one of each must be present.\n");
			}

			// This map has no horde things in it - probably inside a
			// non-horde map.
			return;
		}
	}

	// Pause game logic if levelstate doesn't allow it.
	if (!G_CanTickGameplay())
		return;

	// Pause game logic if nobody is in the game.
	PlayerResults targets = PlayerQuery().hasHealth().execute();
	if (targets.count == 0)
		return;

	// We can always change state.
	::g_HordeDirector.changeState();

	// The director is only run once a second.
	if (P_AtInterval(TICRATE))
		::g_HordeDirector.tick();
}

bool P_IsHordeThing(const int type)
{
	return type >= TTYPE_HORDE_SMALLMONSTER && type <= TTYPE_HORDE_SMALLBOSS;
}

const hordeDefine_t::weapons_t& P_HordeWeapons()
{
	if (G_IsHordeMode())
	{
		const hordeDefine_t& define = G_HordeDefine(::g_HordeDirector.getDefineID());
		return define.weapons;
	}
	else
	{
		static hordeDefine_t::weapons_t weapons;
		if (weapons.empty())
		{
			weapons.push_back(wp_shotgun);
			weapons.push_back(wp_chaingun);
			weapons.push_back(wp_supershotgun);
			weapons.push_back(wp_missile);
			weapons.push_back(wp_plasma);
			weapons.push_back(wp_bfg);
		}
		return weapons;
	}
}

const hordeDefine_t::ammos_t& P_HordeAmmos()
{
	if (G_IsHordeMode())
	{
		const hordeDefine_t& define = G_HordeDefine(::g_HordeDirector.getDefineID());
		return define.ammos;
	}
	else
	{
		static hordeDefine_t::ammos_t ammos;
		if (ammos.empty())
		{
			// [AM] Shells are first because a typical define leads with the
			//      shotgun and the pistol is not accounted for at all.
			ammos.push_back(am_shell);
			ammos.push_back(am_clip);
			ammos.push_back(am_misl);
			ammos.push_back(am_cell);
		}
		return ammos;
	}
}

/**
 * @brief Serialize horde using FArchive.
 */
void P_SerializeHorde(FArchive& arc)
{
	if (arc.IsStoring())
	{
		hordeInfo_t info = ::g_HordeDirector.serialize();
		const int state = info.state;
		const DWORD defineID = static_cast<DWORD>(info.defineID);
		arc << state << info.wave << info.waveTime << info.bossTime << defineID
		    << info.spawnedHealth << info.killedHealth << info.bossHealth
		    << info.bossDamage << info.waveStartHealth;
	}
	else
	{
		hordeInfo_t info;
		int state;
		DWORD defineID;
		arc >> state >> info.wave >> info.waveTime >> info.bossTime >> defineID >>
		    info.spawnedHealth >> info.killedHealth >> info.bossHealth >>
		    info.bossDamage >> info.waveStartHealth;
		info.state = static_cast<hordeState_e>(state);
		info.defineID = defineID;
		::g_HordeDirector.unserialize(info);
	}
}

/**
 * @brief Do last minute fixups after loading a savegame.
 */
void P_HordePostLoad()
{
	if (!G_IsHordeMode())
		return;

	::g_HordeDirector.rescanBosses();
}

BEGIN_COMMAND(hordewave)
{
	if (argc < 2)
	{
		Printf("hordewave - Restarts the current wave with a new definition\n"
		       "Usage:\n"
		       "  ] hordewave <DEF NAME>\n"
		       "  Starts the wave named DEF NAME.  The name can be partial.\n");
		return;
	}

	if (!G_IsHordeMode())
	{
		Printf("Can't change the wave define outside of horde mode.\n");
		return;
	}

	if (!::g_HordeDirector.forceWave(argv[1]))
	{
		Printf("Could not find wave define starting with \"%s\"\n", argv[1]);
	}
}
END_COMMAND(hordewave)

BEGIN_COMMAND(hordenextwave)
{
	if (!G_IsHordeMode())
	{
		Printf("Can't advance the wave outside of horde mode.\n");
		return;
	}

	::g_HordeDirector.nextWave();
}
END_COMMAND(hordenextwave)

BEGIN_COMMAND(hordeboss)
{
	if (!G_IsHordeMode())
	{
		Printf("Can't spawn a horde boss outside of horde mode.\n");
		return;
	}

	if (::g_HordeDirector.forceBoss())
	{
		Printf("Spawned the boss.\n");
	}
	else
	{
		Printf("Could not spawn a boss.\n");
	}
}
END_COMMAND(hordeboss)

EXTERN_CVAR(g_horde_mintotalhp)
EXTERN_CVAR(g_horde_maxtotalhp)
EXTERN_CVAR(g_horde_goalhp)

BEGIN_COMMAND(hordeinfo)
{
	if (!G_IsHordeMode())
	{
		Printf("Can't obtain horde info outside of horde mode.\n");
		return;
	}

	float skillScaler = 1.0f;
	if (sv_skill == sk_medium)
		skillScaler = 0.75f;
	else if (sv_skill == sk_baby || sv_skill == sk_easy)
		skillScaler = 0.5f;

	const hordeDefine_t& define = G_HordeDefine(::g_HordeDirector.getDefineID());

	Printf("[Define: %s]\n", define.name.c_str());
	Printf("Weapons: %s\n", JoinStrings(define.weaponStrings(NULL), " ").c_str());
	Printf("Min Group Health: %d\n", define.minGroupHealth);
	Printf("Max Group Health: %d (Difficulty: %s)\n", define.maxGroupHealth,
	       define.difficulty(false));
	Printf("Min Total Health: %d = waveMaxGroup:%d * g_horde_mintotalhp:%s * "
	       "skillLevel:%0.2f\n",
	       define.minTotalHealth(), define.maxGroupHealth, ::g_horde_mintotalhp.cstring(),
	       skillScaler);
	Printf("Max Total Health: %d = waveMaxGroup:%d * g_horde_maxtotalhp:%s * "
	       "skillLevel:%0.2f\n",
	       define.maxTotalHealth(), define.maxGroupHealth, ::g_horde_maxtotalhp.cstring(),
	       skillScaler);
	Printf("Goal Health: %d = waveMaxGroup:%d * g_horde_goalhp:%s * skillLevel:%0.2f\n",
	       define.goalHealth(), define.maxGroupHealth, ::g_horde_goalhp.cstring(),
	       skillScaler);

	const char* stateStr = NULL;
	switch (::g_HordeDirector.serialize().state)
	{
	case HS_STARTING:
		stateStr = "Starting";
		break;
	case HS_PRESSURE:
		stateStr = "Pressure";
		break;
	case HS_RELAX:
		stateStr = "Relax";
		break;
	case HS_WANTBOSS:
		stateStr = "WantBoss";
		break;
	}

	int min, max;
	::g_HordeDirector.getNextSpawnTime(min, max);

	Printf("[Wave: %d]\n", ::g_HordeDirector.serialize().wave);
	Printf("State: %s\n", stateStr);

	if (::g_HordeDirector.getAliveHealth() <= define.maxTotalHealth())
	{
		Printf("Current Spawn Rate: %d-%dsec\n", min, max);
	}
	else
	{
		Printf("Current Spawn Rate: PAUSED (Above Max Health)\n");
	}
	Printf("Empty/Full Spawn Rate: %d-%dsec, %d-%dsec\n", g_horde_spawnempty_min.asInt(),
	       g_horde_spawnempty_max.asInt(), g_horde_spawnfull_min.asInt(),
	       g_horde_spawnfull_max.asInt());
	Printf("Alive Health: %d\n", ::g_HordeDirector.serialize().alive());
	Printf("Killed Health: %d\n", ::g_HordeDirector.serialize().killed());
	Printf("Boss Health: %d\n", ::g_HordeDirector.serialize().bossHealth);
	Printf("Boss Damage: %d\n", ::g_HordeDirector.serialize().bossDamage);
}
END_COMMAND(hordeinfo)
