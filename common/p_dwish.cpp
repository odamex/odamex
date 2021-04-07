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
#include "doomstat.h"
#include "d_player.h"
#include "m_random.h"
#include "m_vectors.h"
#include "p_local.h"
#include "p_tick.h"
#include "s_sound.h"

bool P_LookForPlayers(AActor* actor, bool allaround);

struct DirectorState
{
	int spawned_health;
	int killed_health;
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

void P_RunDWTics()
{
	// Move this function to tick inside someplace that can be paused.
	if (::paused)
		return;

	if (!P_AtInterval(TICRATE))
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

		// Spawn the monster.
		AActor* mo = new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z, (mobjtype_t)spawn.mo->health);
		P_LookForPlayers(mo, true);

		// Spawn teleport fog.
		new AActor(spawn.mo->x, spawn.mo->y, spawn.mo->z, MT_TFOG);
		S_Sound(spawn.mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);
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
