// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	CL_PRED
//
//-----------------------------------------------------------------------------


#include "doomtype.h"
#include "doomstat.h"
#include "d_player.h"
#include "g_game.h"
#include "p_local.h"
#include "gi.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "st_stuff.h"
#include "m_argv.h"
#include "c_console.h"
#include "cl_main.h"

void P_MovePlayer (player_t *player);
void P_CalcHeight (player_t *player);
void P_DeathThink (player_t *player);


angle_t cl_angle[MAXSAVETICS];
angle_t cl_pitch[MAXSAVETICS];
fixed_t cl_viewheight[MAXSAVETICS];
fixed_t cl_deltaviewheight[MAXSAVETICS];
fixed_t cl_jumpTics[MAXSAVETICS];
int     cl_reactiontime[MAXSAVETICS];
byte    cl_waterlevel[MAXSAVETICS];

bool predicting;

std::vector <plat_pred_t> real_plats;

//
// CL_ResetSectors
//
void CL_ResetSectors (void)
{
	for(size_t i = 0; i < real_plats.size(); i++)
	{
		plat_pred_t *pred = &real_plats[i];
		sector_t *sec = &sectors[pred->secnum];

		if(!sec->floordata)
		{
			if(real_plats.erase(real_plats.begin() + i) == real_plats.end())
				break;
			
			continue;
		}

		if(sec->floordata->IsKindOf(RUNTIME_CLASS(DPlat)))
		{
			DPlat *plat = (DPlat *)sec->floordata;
			sec->floorheight = pred->floorheight;
			plat->SetState(pred->state, pred->count);
		}
		else if(sec->floordata && sec->floordata->IsKindOf(RUNTIME_CLASS(DMovingFloor)))
		{
			sec->floorheight = pred->floorheight;
		}
	}
}

//
// CL_PredictSectors
//
void CL_PredictSectors (int predtic)
{
	for(size_t i = 0; i < real_plats.size(); i++)
	{
		plat_pred_t *pred = &real_plats[i];
		sector_t *sec = &sectors[pred->secnum];

		if(pred->tic < predtic)
		{
			if(sec->floordata && (sec->floordata->IsKindOf(RUNTIME_CLASS(DPlat))
				|| sec->floordata->IsKindOf(RUNTIME_CLASS(DMovingFloor))))
			{
				sec->floordata->RunThink();
			}
		}
	}
}

//
// CL_ResetPlayers
//
void CL_ResetPlayers ()
{
	size_t n = players.size();

	// Reset all player positions to their last known server tic
	for(size_t i = 0; i < n; i++)
	{
		player_t *p = &players[i];

		if(!p->mo)
			continue;
			
		// GhostlyDeath -- Ignore Spectators
		if (p->spectator)
			continue;

		// set the position
		CL_MoveThing (p->mo, p->real_origin[0], p->real_origin[1], p->real_origin[2]);
		
		// set the velocity
		p->mo->momx = p->real_velocity[0];
		p->mo->momy = p->real_velocity[1];
		p->mo->momz = p->real_velocity[2];
	}
}

//
// CL_PredictPlayers
//
void CL_PredictPlayer (player_t *p)
{
	if (p->playerstate == PST_DEAD)
	{
		P_DeathThink (p);
		p->mo->RunThink();
		P_CalcHeight(p);

		return;
	}
	else
	{
		P_MovePlayer(p);
		P_CalcHeight(p);
	}

	if (predicting)
        p->mo->RunThink();
    else
        P_PlayerThink(p);
}

//
// CL_PredictPlayers
//
void CL_PredictPlayers (int predtic)
{
	size_t n = players.size();

	for(size_t i = 0; i < n; i++)
	{
		player_t *p = &players[i];
	
		if (!p->ingame() || !p->mo)
			continue;
			
		// GhostlyDeath -- Ignore Spectators
		if (p->spectator)
			continue;

		// Update this player if their last known status is before this tic
		if(p->tic < predtic)
		{
			if(p == &consoleplayer())
			{
				int buf = predtic%MAXSAVETICS;

				ticcmd_t *cmd = &consoleplayer().cmd;
				memcpy(cmd, &localcmds[buf], sizeof(ticcmd_t));

				p->mo->angle = cl_angle[buf];
				p->mo->pitch = cl_pitch[buf];
				p->viewheight = cl_viewheight[buf];
				p->deltaviewheight = cl_deltaviewheight[buf];
				p->jumpTics = cl_jumpTics[buf];
				p->mo->reactiontime = cl_reactiontime[buf];
				p->mo->waterlevel = cl_waterlevel[buf];
			}
	
			CL_PredictPlayer(p);
		}
	}
}

//
// CL_PredicMove
//
void CL_PredictMove (void)
{
	if (noservermsgs)
		return;

	player_t *p = &consoleplayer();

	if (!p->tic || !p->mo)
		return;

	// Save player angle, viewheight,deltaviewheight and jumpTics.
	// Will use it later to predict movements
	int buf = gametic%MAXSAVETICS;
	
	cl_angle[buf] = p->mo->angle;
	cl_pitch[buf] = p->mo->pitch;
	cl_viewheight[buf] = p->viewheight;
	cl_deltaviewheight[buf] = p->deltaviewheight;
	cl_jumpTics[buf] = p->jumpTics;
	cl_reactiontime[buf] = p->mo->reactiontime;
    cl_waterlevel[buf] = p->mo->waterlevel;

	// Disable sounds, etc, during prediction
	predicting = true;

	// Set predictable items to their last received positions
	CL_ResetPlayers();
	CL_ResetSectors();

	int predtic = gametic - MAXSAVETICS;

	if(predtic < 0)
		predtic = 0;

	// Predict each tic
	while(predtic < gametic)
	{
		CL_PredictPlayers(predtic);
		CL_PredictSectors(predtic);

		++predtic;
	}

	predicting = false;

	CL_PredictPlayers(predtic);
	CL_PredictSectors(predtic);
}

VERSION_CONTROL (cl_pred_cpp, "$Id$")

