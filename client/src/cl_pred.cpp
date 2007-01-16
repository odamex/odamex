// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2007 by The Odamex Team.
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
fixed_t cl_viewheight[MAXSAVETICS];
fixed_t cl_deltaviewheight[MAXSAVETICS];
int     reactiontime[MAXSAVETICS];

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
			//new DMovingFloor(sec);
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
void CL_PredictSectors (void)
{
	for(size_t i = 0; i < real_plats.size(); i++)
	{
		plat_pred_t *pred = &real_plats[i];
		sector_t *sec = &sectors[pred->secnum];

		if(sec->floordata && (sec->floordata->IsKindOf(RUNTIME_CLASS(DPlat))
			|| sec->floordata->IsKindOf(RUNTIME_CLASS(DMovingFloor))))
		{
			sec->floordata->RunThink();
		}
	}
}

//
// CL_ResetPlayers
//
void CL_ResetPlayers (size_t oldtic)
{
	for(size_t i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		player_t *p = &players[i];

		if(!p->mo)
			continue;

		if(p == &consoleplayer())
		{
			// set the position
			CL_MoveThing (p->mo, p->real_origin[0], p->real_origin[1], p->real_origin[2]);
		
			// set the velocity
			p->mo->momx = p->real_velocity[0];
			p->mo->momy = p->real_velocity[1];
			p->mo->momz = p->real_velocity[2];
		}
		else
		{
			// set the position
			CL_MoveThing (p->mo, p->real_origin[0], p->real_origin[1], p->real_origin[2]);
		
			// set the velocity
			p->mo->momx = p->real_velocity[0];
			p->mo->momy = p->real_velocity[1];
			p->mo->momz = p->real_velocity[2];

			// predict up to oldtic
			int reverse_tics = ((int)gametic - (int)p->last_received) + ((int)p->last_received - (int)oldtic);
			while(reverse_tics > 0)
			{
				predicting = true; // disable bobbing, sounds
				P_MovePlayer(p);
				P_CalcHeight(p);
				reverse_tics--;
				predicting = false;
			}
		}
	}
}

//
// CL_PredictPlayers
//
void CL_PredictPlayers (void)
{
	for(size_t i = 0; i < players.size(); i++)
	{
		if(!players[i].ingame())
			continue;

		player_t *p = &players[i];

		if(!p->mo)
			continue;

		if (p->playerstate == PST_DEAD)
		{
			P_DeathThink (p);
			p->mo->RunThink();
			P_CalcHeight(p);
		}
		else
		{
			P_MovePlayer(p);
			P_CalcHeight(p);
			p->mo->RunThink();
		}
	}
}

//
// CL_PredicMove
//
void CL_PredictMove (void)
{
	player_t *p = &consoleplayer();
	int       tic;
	fixed_t   pitch;

	tic = p->tic + 1;

	if (!p->tic || !p->mo || tic > gametic)
		return;

	// Save player angle, viewheight and deltaviewheight
	// Will use it later to predict movements
	cl_angle[gametic%MAXSAVETICS] = p->mo->angle;
	cl_viewheight[gametic%MAXSAVETICS] = p->viewheight;
	cl_deltaviewheight[gametic%MAXSAVETICS] = p->deltaviewheight;
	reactiontime[gametic%MAXSAVETICS] = p->mo->reactiontime;
	pitch = p->mo->pitch;

	if (noservermsgs)
		return;

	// reset simulation to last received tic
	CL_ResetSectors();
	CL_ResetPlayers(tic);
	
	if (p->playerstate == PST_DEAD)
	{
		// predict forward until tic <= gametic
		while (tic < gametic)
		{
			predicting = true; // disable bobbing, sounds
			p->mo->RunThink();
			CL_PredictSectors(); // denis - todo - replace the other calls here with CL_PredictPlayers/CL_PredictSectors
			tic++;
		}

		predicting = false;
		P_DeathThink (p);
		p->mo->RunThink();
		P_CalcHeight(p);
		return;
	}

	p->mo->angle = cl_angle[tic%MAXSAVETICS];
	p->viewheight = cl_viewheight[tic%MAXSAVETICS];
	p->deltaviewheight = cl_deltaviewheight[tic%MAXSAVETICS];
	p->mo->reactiontime = reactiontime[tic%MAXSAVETICS];
	
	// predict forward until tic <= gametic
	while (tic <= gametic)
	{
		int buf = tic%MAXSAVETICS;
		ticcmd_t *cmd = &consoleplayer().cmd;
		memcpy (cmd, &localcmds[buf], sizeof(ticcmd_t));
		
		if (tic < gametic)
		{
			predicting = true; // disable bobbing, sounds
			CL_PredictPlayers();
			CL_PredictSectors();
		}

		tic++;
	}
	

	if (!p->mo->reactiontime)
		p->mo->angle = cl_angle[gametic%MAXSAVETICS];
	p->mo->pitch = pitch;
	predicting = false; // allow bobbing, sounds

	P_PlayerThink (p);
	CL_PredictPlayers();
	CL_PredictSectors();
}


VERSION_CONTROL (cl_pred_cpp, "$Id:$")

