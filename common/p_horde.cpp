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

EXTERN_CVAR(g_horde_waves)

void A_PainDie(AActor* actor);

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

			S_Sound(mo, CHAN_VOICE, sound, 1, ATTN_NORM);
		}
	}
}

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
	case HS_WANTBOSS:
		return "HS_WANTBOSS";
	default:
		return "UNKNOWN";
	}
}

class HordeState
{
	hordeState_e m_state;
	int m_wave;
	int m_waveTime;
	int m_bossTime; // If == waveTime, boss not spawned.
	size_t m_defineID;
	int m_spawnedHealth;
	int m_killedHealth;
	int m_waveStartHealth;
	AActors m_bosses;
	hordeRecipe_t m_bossRecipe;
	int m_nextSpawn;
	int m_nextPowerup;

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
			m_bossTime = ::level.time;
		}
	}

  public:
	/**
	 * @brief Reset director state.
	 */
	void reset()
	{
		setState(HS_STARTING);
		m_wave = 1;
		m_waveTime = ::level.time;
		m_bossTime = ::level.time;
		m_defineID = P_HordePickDefine(m_wave, ::g_horde_waves);
		m_spawnedHealth = 0;
		m_killedHealth = 0;
		m_waveStartHealth = 0;
		m_bosses.clear();
		m_bossRecipe.clear();
		m_nextSpawn = ::level.time;
		m_nextPowerup = ::level.time + (30 * TICRATE);
	}

	/**
	 * @brief Start the given wave.
	 */
	void nextWave()
	{
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
		m_bosses.clear();
		m_bossRecipe.clear();
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
		m_bosses.clear();
		m_bossRecipe.clear();

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
		info.spawnedHealth = m_spawnedHealth;
		info.killedHealth = m_killedHealth;
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
		m_waveStartHealth = info.waveStartHealth;
	}

	void addSpawnHealth(const int health)
	{
		m_spawnedHealth += health;
	}

	void addKilledHealth(const int health)
	{
		m_killedHealth += health;
	}

	size_t getDefineID() const
	{
		return m_defineID;
	}

	void changeState();
	void tick();
} g_HordeDirector;

/**
 * @brief Run functionality that decides if we need to switch states.
 */
void HordeState::changeState()
{
	const hordeDefine_t& define = G_HordeDefine(m_defineID);

	int aliveHealth = m_spawnedHealth - m_killedHealth;
	int goalHealth = define.goalHealth() + m_waveStartHealth;

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
		else if (aliveHealth > define.maxTotalHealth())
		{
			// There's enough monsters in the level, back off.
			setState(HS_RELAX);
		}
		return;
	}
	case HS_RELAX: {
		if (m_killedHealth > goalHealth && m_bosses.empty())
		{
			// We reached the goal, spawn the boss.
			setState(HS_WANTBOSS);
		}
		else if (aliveHealth < define.minTotalHealth())
		{
			// There's not enough monsters in the level, add more.
			setState(HS_PRESSURE);
		}
		return;
	case HS_WANTBOSS: {
		if (m_bossRecipe.isValid() && m_bosses.size() >= m_bossRecipe.count)
		{
			// Doesn't matter which state we enter, but we're more likely
			// to be in the relax state after spawning a big hunk of HP.
			setState(HS_RELAX);
		}
		return;
	}
	}
	default:
		return;
	}
}

/**
 * @brief Run the state logic itself.
 */
void HordeState::tick()
{
	const hordeDefine_t& define = G_HordeDefine(m_defineID);

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
			// Give all ingame players who are out of lives an extra life.
			if (G_IsLivesGame())
			{
				PlayerResults pr = PlayerQuery().notHasLives().execute();
				for (PlayersView::iterator it = pr.players.begin();
				     it != pr.players.end(); ++it)
				{
					(*it)->lives += 1;
					(*it)->playerstate = PST_REBORN;
					if (!::clientside)
					{
						SV_BroadcastPrintf("%s gets a new lease on life.\n",
						                   (*it)->userinfo.netname.c_str());
						MSG_WriteSVC(&(*it)->client.reliablebuf, SVC_PlayerInfo(**it));
						MSG_BroadcastSVC(CLBUF_RELIABLE,
						                 SVC_PlayerMembers(**it, SVC_PM_LIVES),
						                 (*it)->id);
					}
				}
			}

			// Start the next wave.
			nextWave();
			return;
		}
	}

	if (::level.time > m_nextSpawn)
	{
		// Determine our next spawn time.
		const int offset = P_RandomInt(5) + 1;
		m_nextSpawn = ::level.time + (offset * TICRATE);

		// Should we spawn a monster?
		switch (m_state)
		{
		case HS_PRESSURE: {
			const int pressureHealth =
			    P_RandomInt(define.maxGroupHealth - define.minGroupHealth) +
			    define.minGroupHealth;

			// Pick a recipe for some monsters.
			hordeRecipe_t recipe;
			const bool ok = P_HordeSpawnRecipe(recipe, define, false);
			if (!ok)
			{
				Printf("%s: No recipe for non-boss monster.\n", __FUNCTION__);
				return;
			}

			// Choose a spawn point.
			hordeSpawn_t* spawn = P_HordeSpawnPoint(recipe);
			if (spawn == NULL)
			{
				Printf("%s: No spawn candidate for %s.\n", __FUNCTION__,
				       ::mobjinfo[recipe.type].name);
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
			// Do we already have bosses spawned?
			if (m_bossRecipe.isValid() && m_bosses.size() >= m_bossRecipe.count)
				break;

			hordeRecipe_t recipe;
			if (!m_bossRecipe.isValid())
			{
				// Pick a recipe for the boss.
				const bool ok = P_HordeSpawnRecipe(recipe, define, true);
				if (!ok)
				{
					Printf("%s: No recipe for boss monster.\n", __FUNCTION__);
					break;
				}

				// Save for later - in case we need to spawn twice.
				m_bossRecipe = recipe;
			}
			else
			{
				// Adjust the count based on how many bosses we've spawned.
				recipe = m_bossRecipe;
				recipe.count = m_bossRecipe.count - m_bosses.size();
			}

			// Spawn a boss if we don't have one.
			hordeSpawn_t* spawn = P_HordeSpawnPoint(recipe);
			if (spawn == NULL)
			{
				Printf("%s: No spawn candidate for %s.\n", __FUNCTION__,
				       ::mobjinfo[recipe.type].name);
				break;
			}

			AActors mobjs = P_HordeSpawn(*spawn, recipe);
			m_bosses.insert(m_bosses.end(), mobjs.begin(), mobjs.end());
			ActivateMonsters(mobjs);
			break;
		}
		}
	}

	// Always try to spawn an item.
	P_HordeSpawnItem();

	// Always try to spawn a powerup between 30-45 seconds.
	if (!define.powerups.empty() && ::level.time > m_nextPowerup)
	{
		const int offset = P_RandomInt(16) + 30;
		m_nextPowerup = ::level.time + (offset * TICRATE);

		const mobjtype_t pw = define.randomPowerup().mobj;
		P_HordeSpawnPowerup(pw);

		Printf("Spawned powerup %s, next in %d.\n", ::mobjinfo[pw].name, offset);
	}
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

void P_RunHordeTics()
{
	if (::sv_gametype != GM_HORDE)
		return;

	// Move this function to tick inside someplace that can be paused.
	if (::paused)
		return;

	if (::level.time == 0)
	{
		P_HordeAddSpawns();
		::g_HordeDirector.reset();
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

	// The directory is only run once a second.
	if (P_AtInterval(TICRATE))
		::g_HordeDirector.tick();
}

bool P_IsHordeMode()
{
	return sv_gametype == GM_HORDE;
}

bool P_IsHordeThing(const int type)
{
	return type >= TTYPE_HORDE_SMALLMONSTER && type <= TTYPE_HORDE_POWERUP;
}

const hordeDefine_t::weapons_t& P_HordeWeapons()
{
	if (::sv_gametype == GM_HORDE)
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
	if (::sv_gametype == GM_HORDE)
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

	if (!::g_HordeDirector.forceWave(argv[1]))
	{
		Printf("Could not find wave define starting with \"%s\"\n", argv[1]);
	}
}
END_COMMAND(hordewave)

BEGIN_COMMAND(hordenextwave)
{
	::g_HordeDirector.nextWave();
}
END_COMMAND(hordenextwave)

BEGIN_COMMAND(hordeboss)
{
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
	const hordeDefine_t& define = G_HordeDefine(::g_HordeDirector.getDefineID());

	Printf("[Define: %s]\n", define.name.c_str());
	Printf("Min Group Health: %d\n", define.minGroupHealth);
	Printf("Max Group Health: %d\n", define.maxGroupHealth);
	Printf("Min Total Health: %d = %d * %s\n", define.minTotalHealth(),
	       define.maxGroupHealth, ::g_horde_mintotalhp.cstring());
	Printf("Max Total Health: %d = %d * %s\n", define.maxTotalHealth(),
	       define.maxGroupHealth, ::g_horde_maxtotalhp.cstring());
	Printf("Goal Health: %d = %d * %s\n", define.goalHealth(), define.maxGroupHealth,
	       ::g_horde_goalhp.cstring());

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

	Printf("[Wave: %d]\n", ::g_HordeDirector.serialize().wave);
	Printf("State: %s\n", stateStr);
	Printf("Alive Health: %d\n", ::g_HordeDirector.serialize().alive());
	Printf("Killed Health: %d\n", ::g_HordeDirector.serialize().killed());
}
END_COMMAND(hordeinfo)
