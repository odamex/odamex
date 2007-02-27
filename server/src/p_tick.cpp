// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
#include "c_console.h"

#include "doomstat.h"

#include "sv_ctf.h"
#include "sv_main.h"

extern constate_e ConsoleState;
extern gamestate_t wipegamestate;

EXTERN_CVAR (sv_speedhackfix)
EXTERN_CVAR (allowexit)
EXTERN_CVAR (fragexitswitch)
EXTERN_CVAR (fraglimit)
EXTERN_CVAR (timelimit)

//
//	WinningTeam					[Toke - teams]
//	
//	Determines the winning team, if there is one
//
team_t WinningTeam (void)
{
	int max = 0;
	team_t team = TEAM_NONE;
	
	for(size_t i = 0; i < NUMTEAMS; i++)
	{
		if(TEAMpoints[i] > max)
		{
			max = TEAMpoints[i];
			team = (team_t)i;
		}
	}
	
	return team;
}

void SV_LevelTimer()
{
	// LEVEL TIMER
	if (level.time >= (int)(timelimit * TICRATE * 60))
	{
		if (timelimit)
		{		
			if (deathmatch && !teamplay && !ctfmode)
				SV_BroadcastPrintf (PRINT_HIGH, "Timelimit hit.\n");

			if (teamplay && !ctfmode)
			{
				team_t winteam = WinningTeam ();
				
				if(winteam == TEAM_NONE)
					SV_BroadcastPrintf(PRINT_HIGH, "No team won this game!\n");
				else
					SV_BroadcastPrintf(PRINT_HIGH, "%s team wins with a total of %d %s!\n", team_names[winteam], TEAMpoints[winteam], ctfmode ? "captures" : "frags");
			}

			G_ExitLevel(0);
		}
	}
}


//
// P_Ticker
//

void P_Ticker (void)
{
	// [csDoom] if the floor or the ceiling of a sector is moving,
	// mark it as moveable
	for (int i=0; i<numsectors; i++)
	{
		sector_t* sec = &sectors[i];
	
		if ((sec->ceilingdata && sec->ceilingdata->IsKindOf (RUNTIME_CLASS(DMover)))
		|| (sec->floordata && sec->floordata->IsKindOf (RUNTIME_CLASS(DMover))))
			sec->moveable = true;
	}

	if(serverside && sv_speedhackfix)
	{
		for(size_t i = 0; i < players.size(); i++)
			if(players[i].ingame())
				P_PlayerThink (&players[i]);
	}
	
	DThinker::RunThinkers ();

	SV_LevelTimer();

	P_UpdateSpecials ();
	P_RespawnSpecials ();

	// for par times
	level.time++;
}

VERSION_CONTROL (p_tick_cpp, "$Id$")

