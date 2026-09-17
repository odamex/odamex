// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//	Ticker.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "c_effect.h"
#include "p_acs.h"
#include "c_console.h"
#include "p_unlag.h"
#include "p_horde.h"
#include "g_spree.h"

#ifdef CLIENT_APP
#   include "cl_main.h"
#endif

//
// P_AtInterval
//
// Decides if it is time to perform a function that is to be performed
// at regular intervals
//
bool P_AtInterval(int interval)
{
    return (gametic % interval) == 0;
}

void P_AnimationTick(AActor *mo);
void P_MovePlayer (player_t& player);

//
// P_Ticker
//
void P_Ticker (void)
{
#ifdef CLIENT_APP
	if (paused && displayplayer().isFreecam)
	{
		displayplayer().mo->RunThink();
	}

	// Game pauses when in the menu and not online/demo
	if ((paused || (!multiplayer && !demoplayback &&
		(menuactive || ConsoleState == c_down || ConsoleState == c_falling))) &&
		(players.begin()->viewz != 1)) // Render the first tic to get proper viewheight
	{
		return;
	}
#endif

	if (serverside)
	{
		P_RunHordeTics();
		P_RunHelperTics();
	}

	if (clientside)
		P_ThinkParticles ();	// [RH] make the particles think

	if (clientside && serverside)
	{
		for (auto& player : players)
			if (player.ingame())
				P_PlayerThink(player);
	}

	// [SL] 2011-06-05 - Tick player actor animations here since P_Ticker is
	// called only once per tick.  AActor::RunThink is called whenever the
	// server receives a cmd from the client, which can happen multiple times
	// in a single gametic.
	for (auto& player : players)
	{
		P_AnimationTick(player.mo);
	}

#ifdef CLIENT_APP
    if (clientside and not serverside)
    {
        player_t& player = consoleplayer();

        // Do a switcheroo of the player's absolute current state and the position that was
        // current as of the time that the server updated its mobjs.  This lets the mobj
        // thinkers run using the same target data they' would have had on the server.
        //
        // ... should we do this for ALL players, or just the local player?
        //
        const int            effectiveHistoricalTic = player.snapshots.getMostRecentTime();
        const PlayerSnapshot currentSnapshot(gametic, player);
        const PlayerSnapshot historicalSnapshot = player.snapshots.getSnapshot(effectiveHistoricalTic);

        odaproto::clc::PlayerInput currentInput;
        CLC_PackPlayerInputMessageFromPlayer(currentInput, player, gametic, 0);

        historicalSnapshot.toPlayer(player);
        CLC_UnpackPlayerInputMessageToPlayer(localcmds[(player.tic) % MAXSAVETICS], player);

        P_MovePlayer(player);
        player.mo->RunThink();
        DThinker::RunThinkers ();

        currentSnapshot.toPlayer(player);
        CLC_UnpackPlayerInputMessageToPlayer(currentInput, player);
    }
    else
#endif
    {
        DThinker::RunThinkers ();
    }

	P_UpdateSpecials ();
	P_RespawnSpecials ();

	P_TicSprees();

	if (clientside)
		P_RunEffects(); // [RH] Run particle effects

	// for par times
	level.time++;
}

VERSION_CONTROL (p_tick_cpp, "$Id$")
