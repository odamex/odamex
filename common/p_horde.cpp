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
#include "m_random.h"
#include "p_hordespawn.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"

EXTERN_CVAR(g_horde_waves)

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
	int m_stateTime;
	int m_wave;
	const hordeDefine_t* m_waveDefine;
	int m_spawnedHealth;
	int m_killedHealth;
	int m_waveStartHealth;
	AActors m_bosses;
	hordeRecipe_t m_bossRecipe;
	int m_nextPowerup;

	/**
	 * @brief Set the given state.
	 */
	void setState(const hordeState_e state)
	{
		m_state = state;
		m_stateTime = ::level.time;
	}

  public:
	/**
	 * @brief Reset director state.
	 */
	void reset()
	{
		setState(HS_STARTING);
		m_wave = 1;
		m_waveDefine = &P_HordeDefine(m_wave, ::g_horde_waves);
		m_spawnedHealth = 0;
		m_killedHealth = 0;
		m_waveStartHealth = 0;
		m_bosses.clear();
		m_bossRecipe.clear();
		m_nextPowerup = ::level.time + (30 * TICRATE);
	}

	/**
	 * @brief Start the given wave.
	 */
	void nextWave()
	{
		if (::g_horde_waves && m_wave >= ::g_horde_waves)
		{
			G_ExitLevel(0, 1);
			return;
		}

		setState(HS_STARTING);
		m_wave += 1;
		m_waveDefine = &P_HordeDefine(m_wave, ::g_horde_waves);
		m_waveStartHealth = m_killedHealth;
		m_bosses.clear();
		m_bossRecipe.clear();
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
	 * @brief Returns health, like for the HUD.
	 */
	hordeInfo_t info() const
	{
		hordeInfo_t info;
		info.state = m_state;
		info.name = m_waveDefine->name;
		info.wave = m_wave;
		info.alive = m_spawnedHealth - m_killedHealth;
		info.killed = m_killedHealth - m_waveStartHealth;
		info.goal = m_waveDefine->goalHealth();
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

	const hordeDefine_t& getDefine() const
	{
		return *m_waveDefine;
	}

	void preTick();
	void tick();
} g_HordeDirector;

/**
 * @brief Run functionality that decides if we need to switch states.
 */
void HordeState::preTick()
{
	int aliveHealth = m_spawnedHealth - m_killedHealth;
	int goalHealth = m_waveDefine->goalHealth() + m_waveStartHealth;

	switch (m_state)
	{
	case HS_STARTING: {
		// Do anything that we would need to do upon starting any wave.
		m_bosses.clear();
		setState(HS_PRESSURE);
		// fallthrough
	}
	case HS_PRESSURE: {
		if (m_killedHealth > goalHealth && m_bosses.empty())
		{
			setState(HS_WANTBOSS);
			return;
		}
		else if (aliveHealth > m_waveDefine->maxTotalHealth())
		{
			setState(HS_RELAX);
			return;
		}
		return;
	}
	case HS_RELAX: {
		if (m_killedHealth > goalHealth && m_bosses.empty())
		{
			setState(HS_WANTBOSS);
			return;
		}
		else if (aliveHealth < m_waveDefine->minTotalHealth())
		{
			setState(HS_PRESSURE);
			return;
		}
		return;
	case HS_WANTBOSS: {
		if (m_bossRecipe.isValid() && m_bosses.size() >= m_bossRecipe.count)
		{
			// Doesn't matter which state we enter, but we're more likely
			// to be in the relax state after spawning a big hunk of HP.
			setState(HS_RELAX);
			return;
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
	// Are the bosses taken care of?
	if (m_state != HS_WANTBOSS && m_bossRecipe.isValid())
	{
		size_t alive = 0;
		for (AActors::iterator it = m_bosses.begin(); it != m_bosses.end(); ++it)
		{
			if ((*it)->health > 0)
				alive += 1;
		}
		if (!alive)
		{
			nextWave();
			return;
		}
	}

	// Should we spawn a monster?
	switch (m_state)
	{
	case HS_PRESSURE: {
		const int pressureHealth =
		    P_RandomInt(m_waveDefine->maxGroupHealth - m_waveDefine->minGroupHealth) +
		    m_waveDefine->minGroupHealth;

		// Pick a recipe for some monsters.
		hordeRecipe_t recipe;
		const bool ok = P_HordeSpawnRecipe(recipe, *m_waveDefine, false);
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
		Printf("Spawning %d %s (%d hp) at a %s spawn\n", recipe.count,
		       ::mobjinfo[recipe.type].name, hp, HordeThingStr(spawn->type));

		AActors mobjs = P_HordeSpawn(*spawn, recipe);
		ActivateMonsters(mobjs);
		break;
	}
	case HS_RELAX: {
		// Pick a monster that the player can't see right now and teleport
		// them close to the player.
		break;
	}
	case HS_WANTBOSS: {
		// Do we already have bosses spawned?
		if (m_bossRecipe.isValid() && m_bosses.size() >= m_bossRecipe.count)
			break;

		hordeRecipe_t recipe;
		if (!m_bossRecipe.isValid())
		{
			// Pick a recipe for the boss.
			const bool ok = P_HordeSpawnRecipe(recipe, *m_waveDefine, true);
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
		m_bosses = mobjs;
		ActivateMonsters(mobjs);
		break;
	}
	}

	// Always try to spawn an item.
	P_HordeSpawnItem();

	// Always try to spawn a powerup between 30-45 seconds.
	if (!getDefine().powerups.empty() && ::level.time > m_nextPowerup)
	{
		const int offset = P_RandomInt(16) + 30;
		m_nextPowerup = ::level.time + (offset * TICRATE);

		const size_t idx = P_RandomInt(getDefine().powerups.size());
		const mobjtype_t pw = getDefine().powerups[idx];
		P_HordeSpawnPowerup(pw);

		Printf("Spawned powerup %s, next in %d.\n", ::mobjinfo[pw].name, offset);
	}
}

hordeInfo_t P_HordeInfo()
{
	return ::g_HordeDirector.info();
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

	// Pause game logic if nobody is in the game.
	PlayerResults targets = PlayerQuery().hasHealth().execute();
	if (targets.count == 0)
		return;

	// Only run logic once a second.
	if (!P_AtInterval(TICRATE))
		return;

	::g_HordeDirector.preTick();
	::g_HordeDirector.tick();
}

bool P_IsHordeMode()
{
	return sv_gametype == GM_HORDE;
}

bool P_IsHordeThing(const int type)
{
	return type >= TTYPE_HORDE_ITEM && type <= TTYPE_HORDE_POWERUP;
}

const hordeDefine_t::weapons_t& P_HordeWeapons()
{
	if (::sv_gametype == GM_HORDE)
	{
		return ::g_HordeDirector.getDefine().weapons;
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
	const hordeDefine_t& define = ::g_HordeDirector.getDefine();

	Printf("[Define: %s]\n", define.name);
	Printf("Min Group Health: %d\n", define.minGroupHealth);
	Printf("Max Group Health: %d\n", define.maxGroupHealth);
	Printf("Min Total Health: %d = %d * %s\n", define.minTotalHealth(),
	       define.maxGroupHealth, ::g_horde_mintotalhp.cstring());
	Printf("Max Total Health: %d = %d * %s\n", define.maxTotalHealth(),
	       define.maxGroupHealth, ::g_horde_maxtotalhp.cstring());
	Printf("Goal Health: %d = %d * %s\n", define.goalHealth(), define.maxGroupHealth,
	       ::g_horde_goalhp.cstring());

	const char* stateStr = NULL;
	switch (::g_HordeDirector.info().state)
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

	Printf("[Wave: %d]\n", ::g_HordeDirector.info().wave);
	Printf("State: %s\n", stateStr);
	Printf("Alive Health: %d\n", ::g_HordeDirector.info().alive);
	Printf("Killed Health: %d\n", ::g_HordeDirector.info().killed);
}
END_COMMAND(hordeinfo)
