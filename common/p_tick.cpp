// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Ticker.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "c_effect.h"
#include "p_acs.h"
#include "c_console.h"
#include "p_horde.h"

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

//
// P_Ticker
//
void P_Ticker (void)
{
	if(paused)
		return;

#ifdef CLIENT_APP
	// Game pauses when in the menu and not online/demo
	if (!multiplayer
		&& !demoplayback 
		&& (menuactive || ConsoleState == c_down || ConsoleState == c_falling)
		&& players.begin()->viewz != 1)
		return;
#endif

	if (serverside)
		P_RunHordeTics();

	if (clientside)
		P_ThinkParticles ();	// [RH] make the particles think

	if (clientside && serverside)
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
			if (it->ingame())
				P_PlayerThink(&*(it));
	}

	// [SL] 2011-06-05 - Tick player actor animations here since P_Ticker is
	// called only once per tick.  AActor::RunThink is called whenever the
	// server receives a cmd from the client, which can happen multiple times
	// in a single gametic.
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		P_AnimationTick(it->mo);
	}

	DThinker::RunThinkers ();
	
	P_UpdateSpecials ();
	P_RespawnSpecials ();

	if (clientside)
		P_RunEffects ();	// [RH] Run particle effects

	// for par times
	level.time++;
}

VERSION_CONTROL (p_tick_cpp, "$Id$")
