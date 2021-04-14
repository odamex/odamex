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
//  "Death Wish" game mode.
//
//-----------------------------------------------------------------------------

#include "p_dwish.h"

#include "c_dispatch.h"
#include "d_player.h"
#include "doomstat.h"
#include "m_random.h"
#include "m_vectors.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"

bool P_LookForPlayers(AActor* actor, bool allaround);

// [AM] TODO: At some point I need to add this to the header.

#define CASE_STR(str) \
	case str:         \
		return #str

class DirectorState
{
	enum state_e
	{
		DS_STARTING,
		DS_PRESSURE,
		DS_RELAX,
	};

	static const char* stateString(state_e state)
	{
		switch (state)
		{
			CASE_STR(DS_STARTING);
			CASE_STR(DS_PRESSURE);
			CASE_STR(DS_RELAX);
		}
	}

	state_e m_state;
	int m_stateTime;
	int m_spawnedHealth;
	int m_targetHealth;
	int m_killedHealth;

	const int MAX_HEALTH = 250;

	void setState(const state_e state)
	{
		m_state = state;
		m_stateTime = ::gametic;
	}

  public:
	void reset()
	{
		setState(DS_STARTING);
		m_spawnedHealth = 0;
		m_targetHealth = 0;
		m_killedHealth = 0;
	}

	int shouldSpawn()
	{
		Printf("%12s: s:%d k:%d t:%d\n", stateString(m_state), m_spawnedHealth,
		       m_killedHealth, m_targetHealth);

		switch (m_state)
		{
		case DS_STARTING:
			setState(DS_PRESSURE);
			m_targetHealth = DirectorState::MAX_HEALTH;
		case DS_PRESSURE: {
			int left = m_targetHealth - m_spawnedHealth;
			if (left < 100.0)
			{
				setState(DS_RELAX);
				return 0;
			}

			return ceil(left / 100.0);
		}
		case DS_RELAX: {
			int left = m_targetHealth - m_killedHealth;
			if (left < 100.0)
			{
				setState(DS_PRESSURE);
				m_targetHealth += DirectorState::MAX_HEALTH;
				return 0;
			}

			return 0;
		}
		default:
			return 0;
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
} gDirector;

struct SpawnPoint
{
	AActor::AActorPtr mo;
};
typedef std::vector<SpawnPoint> SpawnPoints;
typedef std::vector<SpawnPoint*> SpawnPointView;

static SpawnPoints gSpawns;
static bool DEBUG_enabled;

void P_AddDWSpawnPoint(AActor* mo)
{
	SpawnPoint sp;
	sp.mo = mo->self;
	::gSpawns.push_back(sp);
}

void P_AddHealthPool(AActor* mo)
{
	// Already added?
	if (mo->oflags & MFO_HEALTHPOOL)
		return;

	// Counts as a monster?  (Lost souls don't)
	if (!(mo->flags & MF_COUNTKILL))
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

void P_CategorizeMonsters()
{
	for (size_t i = 0; i < ARRAY_LENGTH(::mobjinfo); i++)
	{
		// Is this a monster?
		if (!(::mobjinfo[i].flags & MF_COUNTKILL))
			continue;

		// Does this monster have any attacks?  (Skips keen)
		if (::mobjinfo[i].missilestate == S_NULL && ::mobjinfo[i].meleestate == S_NULL)
			continue;

		// Is this monster a flying monster?
		if (::mobjinfo[i].flags & (MF_FLOAT | MF_NOGRAVITY))
		{
			// Monster is a flying monster.
			Printf("Flying: %s\n", ::mobjinfo[i].name);
		}
		else if (::mobjinfo[i].missilestate == S_NULL &&
		         ::mobjinfo[i].meleestate != S_NULL)
		{
			// Monster is melee-only monster.
			Printf("Melee: %s\n", ::mobjinfo[i].name);
		}
		else
		{
			// Monster is melee-only monster.
			Printf("Standard: %s\n", ::mobjinfo[i].name);
		}
	}
}

void P_RunDWTics()
{
	// Move this function to tick inside someplace that can be paused.
	if (::paused)
		return;

	if (::level.time == 0)
		::gDirector.reset();

	if (!P_AtInterval(TICRATE))
		return;

	P_CategorizeMonsters();

	if (!::gDirector.shouldSpawn())
		return;

	PlayerResults pr = PlayerQuery().execute();
	PlayersView::const_iterator it;
	for (it = pr.players.begin(); it != pr.players.end(); ++it)
	{
		player_t* player = *it;

		// Do not consider dead players.
		if (player->health <= 0)
			continue;

		// Spawn monsters at visible spawn points.
		SpawnPointView view;
		for (SpawnPoints::iterator sit = ::gSpawns.begin(); sit != ::gSpawns.end(); ++sit)
		{
			if (P_CheckSight(sit->mo, player->mo))
			{
				view.push_back(&*sit);
			}
		}

		if (view.empty())
			continue;

		size_t idx = P_Random() % view.size();
		SpawnPoint& spawn = *view.at(idx);

		// Spawn the monster - possibly.
		AActor* mo = new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z,
		                        static_cast<mobjtype_t>(spawn.mo->health));
		if (mo)
		{
			if (P_TestMobjLocation(mo))
			{
				// Don't respawn
				mo->flags |= MF_DROPPED;

				P_AddHealthPool(mo);
				if (P_LookForPlayers(mo, true))
				{
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

					// If monster has a target (which they should by now)
					// force them to start chasing them immediately.
					if (mo->target)
						P_SetMobjState(mo, mo->info->seestate, true);
				}

				AActor* tele = new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z, MT_TFOG);
				S_Sound(tele, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
			}
			else
			{
				// Spawn blocked.
				mo->Destroy();
			}
		}
	}
}

bool P_IsDWMode()
{
	return ::DEBUG_enabled;
}

BEGIN_COMMAND(debug_dwish)
{
	::DEBUG_enabled = true;
}
END_COMMAND(debug_dwish)
