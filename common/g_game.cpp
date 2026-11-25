// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   Game functions that are common between the client and server.
//
//-----------------------------------------------------------------------------

#include <utility>

#include "odamex.h"

#include "g_game.h"

#include "doomstat.h"
#include "g_deathspot.h"
#include "g_keepweapons.h"
#include "g_spawninv.h"

#ifdef SERVER_APP
#include "sv_main.h"
#endif

EXTERN_CVAR(sv_keepkeys)
EXTERN_CVAR(sv_sharekeys)

void G_SetFast(const bool wantFast)
{
	static bool isFast = false;
	if (wantFast != isFast)
	{
		for (auto&& [_, minfo] : mobjinfo)
		{
			if (minfo.altspeed != NO_ALTSPEED)
				std::swap(minfo.speed, minfo.altspeed);
		}

		if (wantFast)
		{
			for (auto&& [_, state] : states)
			{
				if (state.flags & STATEF_SKILL5FAST && (state.tics != 1 || demoplayback))
					state.tics >>= 1; // don't change 1->0 since it causes cycles
			}
		}
		else
		{
			for (auto&& [_, state] : states)
			{
				if (state.flags & STATEF_SKILL5FAST)
					state.tics <<= 1;
			}
		}

		isFast = wantFast;
	}
}

//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn(player_t& p) // [Toke - todo] clean this function
{
	for (size_t i = 0; i < NUMAMMO; i++)
	{
		p.maxammo[i] = ::maxammo[i];
		p.ammo[i] = 0;
	}

	for (size_t i = 0; i < NUMWEAPONS; i++)
		p.weaponowned[i] = false;

	if (not sv_keepkeys and not sv_sharekeys)
		P_ClearPlayerCards(p);

#ifdef SERVER_APP
	// Keys found between a player's death and their respawn are shared out here.
	// The client doesn't predict keeping keys, the server tells them what keys
	// they have on spawn.
	if (sv_sharekeys)
	{
		for (size_t i = 0; i < NUMCARDS; i++)
			p.cards[i] = keysfound[i];
	}
#endif

	P_ClearPlayerPowerups(p);

	for (size_t i = 0; i < NUMTEAMS; i++)
		p.flags[i] = false;

	p.backpack = false;

	G_GiveSpawnInventory(p);

	// sv_keepweapons - hand back the loadout this respawn earned, if any.
	KeepWeaponsManager::getInstance().restoreLoadout(p);

	p.usedown = p.attackdown = true; // don't do anything immediately
	p.playerstate = PST_LIVE;
	p.doreborn = false;
	p.weaponowned[wp_none] = true;

	if (not p.spectator)
		p.cheats = 0; // Reset cheat flags

	p.death_time = 0;
	DeathSpotManager::getInstance().eraseDeathSpot(p.id);
}
