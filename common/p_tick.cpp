// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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
#include "p_acs.h"
#include "c_console.h"
#include "doomstat.h"
#include "p_unlag.h"

EXTERN_CVAR (sv_speedhackfix)

//
// P_Ticker
//
void P_Ticker (void)
{
	if(paused)
		return;

	if (!multiplayer && !demoplayback && menuactive && players[0].viewz != 1)
		return;

	if((serverside && sv_speedhackfix) || (clientside && serverside))
	{
		for(size_t i = 0; i < players.size(); i++)
			if(players[i].ingame())
				P_PlayerThink (&players[i]);
	}

	DThinker::RunThinkers ();
	
	P_UpdateSpecials ();
	P_RespawnSpecials ();

	// [SL] 2011-05-11 - Save player positions and moving sector heights so
	// they can be reconciled later for unlagging
	Unlag::getInstance().recordPlayerPositions();
	Unlag::getInstance().recordSectorPositions();

	// for par times
	level.time++;
}

VERSION_CONTROL (p_tick_cpp, "$Id$")

