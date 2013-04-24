// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

#include "z_zone.h"
#include "p_local.h"
#include "c_effect.h"
#include "p_acs.h"
#include "c_console.h"
#include "doomstat.h"
#include "p_unlag.h"

EXTERN_CVAR (sv_speedhackfix)

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

	if (!multiplayer && !demoplayback && menuactive && players[0].viewz != 1)
		return;

	if (clientside)
		P_ThinkParticles ();	// [RH] make the particles think

	if((serverside && sv_speedhackfix) || (clientside && serverside))
	{
		for(size_t i = 0; i < players.size(); i++)
			if(players[i].ingame())
				P_PlayerThink (&players[i]);
	}

	// [SL] 2011-06-05 - Tick player actor animations here since P_Ticker is
	// called only once per tick.  AActor::RunThink is called whenever the
	// server receives a cmd from the client, which can happen multiple times
	// in a single gametic.
	for (size_t i = 0; i < players.size(); i++)
	{
		P_AnimationTick(players[i].mo);
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

