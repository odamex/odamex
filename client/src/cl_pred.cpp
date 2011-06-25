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
//	Client-side prediction of local player, other players and moving sectors
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

// Prediction debugging info
//#define _PRED_DBG

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

TArray <plat_pred_t> real_plats;

//
// CL_ResetSectors
//
void CL_ResetSectors (void)
{
	for(size_t i = 0; i < real_plats.Size(); i++)
	{
		plat_pred_t *pred = &real_plats[i];
		sector_t *sec = &sectors[pred->secnum];

		if(!sec->floordata && !sec->ceilingdata)
		{
			real_plats.Pop(real_plats[i]);

            if (!real_plats.Size())
                break;

			continue;
		}

		// Pillars and elevators set both floordata and ceilingdata
		if(sec->ceilingdata && sec->ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
		{
			DPillar *Pillar = (DPillar *)sec->ceilingdata;
            
            sec->floorheight = pred->floorheight;
            sec->ceilingheight = pred->ceilingheight;
            P_ChangeSector(sec, false);

            Pillar->m_Type = (DPillar::EPillar)pred->Both.m_Type;
            Pillar->m_FloorSpeed = pred->Both.m_FloorSpeed;
            Pillar->m_CeilingSpeed = pred->Both.m_CeilingSpeed;
            Pillar->m_FloorTarget = pred->Both.m_FloorTarget;
            Pillar->m_CeilingTarget = pred->Both.m_CeilingTarget;
            Pillar->m_Crush = pred->Both.m_Crush;
            
            continue;
		}

        if (sec->ceilingdata && sec->ceilingdata->IsA(RUNTIME_CLASS(DElevator)))
        {
            DElevator *Elevator = (DElevator *)sec->ceilingdata;

            sec->floorheight = pred->floorheight;
            sec->ceilingheight = pred->ceilingheight;
            P_ChangeSector(sec, false);

            Elevator->m_Type = (DElevator::EElevator)pred->Both.m_Type;
            Elevator->m_Direction = pred->Both.m_Direction;
            Elevator->m_FloorDestHeight = pred->Both.m_FloorDestHeight;
            Elevator->m_CeilingDestHeight = pred->Both.m_CeilingDestHeight;
            Elevator->m_Speed = pred->Both.m_Speed;
            
            continue;
        }

		if (sec->floordata && sec->floordata->IsA(RUNTIME_CLASS(DFloor)))
        {
            DFloor *Floor = (DFloor *)sec->floordata;

            sec->floorheight = pred->floorheight;
            P_ChangeSector(sec, false);

            Floor->m_Type = (DFloor::EFloor)pred->Floor.m_Type;
            Floor->m_Crush = pred->Floor.m_Crush;
            Floor->m_Direction = pred->Floor.m_Direction;
            Floor->m_NewSpecial = pred->Floor.m_NewSpecial;
            Floor->m_Texture = pred->Floor.m_Texture;
            Floor->m_FloorDestHeight = pred->Floor.m_FloorDestHeight;
            Floor->m_Speed = pred->Floor.m_Speed;
            Floor->m_ResetCount = pred->Floor.m_ResetCount;
            Floor->m_OrgHeight = pred->Floor.m_OrgHeight;
            Floor->m_Delay = pred->Floor.m_Delay;
            Floor->m_PauseTime = pred->Floor.m_PauseTime;
            Floor->m_StepTime = pred->Floor.m_StepTime;
            Floor->m_PerStepTime = pred->Floor.m_PerStepTime;
        }

		if(sec->floordata && sec->floordata->IsA(RUNTIME_CLASS(DPlat)))
		{
			DPlat *Plat = (DPlat *)sec->floordata;
            
            sec->floorheight = pred->floorheight;
            P_ChangeSector(sec, false);

            Plat->m_Speed = pred->Floor.m_Speed;
            Plat->m_Low = pred->Floor.m_Low;
            Plat->m_High = pred->Floor.m_High;
            Plat->m_Wait = pred->Floor.m_Wait;
            Plat->m_Count = pred->Floor.m_Count;
            Plat->m_Status = (DPlat::EPlatState)pred->Floor.m_Status;
            Plat->m_OldStatus = (DPlat::EPlatState)pred->Floor.m_OldStatus;
            Plat->m_Crush = pred->Floor.m_Crush;
            Plat->m_Tag = pred->Floor.m_Tag;
            Plat->m_Type = (DPlat::EPlatType)pred->Floor.m_Type;
		}

		if (sec->ceilingdata && sec->ceilingdata->IsA(RUNTIME_CLASS(DCeiling)))
        {
            DCeiling *Ceiling = (DCeiling *)sec->ceilingdata;

            sec->ceilingheight = pred->ceilingheight;
            P_ChangeSector(sec, false);

            Ceiling->m_Type = (DCeiling::ECeiling)pred->Ceiling.m_Type;
            Ceiling->m_BottomHeight = pred->Ceiling.m_BottomHeight;
            Ceiling->m_TopHeight = pred->Ceiling.m_TopHeight;
            Ceiling->m_Speed = pred->Ceiling.m_Speed;
            Ceiling->m_Speed1 = pred->Ceiling.m_Speed1;
            Ceiling->m_Speed2 = pred->Ceiling.m_Speed2;
            Ceiling->m_Crush = pred->Ceiling.m_Crush;
            Ceiling->m_Silent = pred->Ceiling.m_Silent;
            Ceiling->m_Direction = pred->Ceiling.m_Direction;
            Ceiling->m_Texture = pred->Ceiling.m_Texture;
            Ceiling->m_NewSpecial = pred->Ceiling.m_NewSpecial;
            Ceiling->m_Tag = pred->Ceiling.m_Tag;
            Ceiling->m_OldDirection = pred->Ceiling.m_OldDirection;
        }

		if (sec->ceilingdata && sec->ceilingdata->IsA(RUNTIME_CLASS(DDoor)))
        {
            DDoor *Door = (DDoor *)sec->ceilingdata;

            sec->ceilingheight = pred->ceilingheight;
            P_ChangeSector(sec, false);

            Door->m_Type = (DDoor::EVlDoor)pred->Ceiling.m_Type;
            Door->m_TopHeight = pred->Ceiling.m_TopHeight;
            Door->m_Speed = pred->Ceiling.m_Speed;
            Door->m_Direction = pred->Ceiling.m_Direction;
            Door->m_TopWait = pred->Ceiling.m_TopWait;
            Door->m_TopCountdown = pred->Ceiling.m_TopCountdown;
			Door->m_Status = (DDoor::EVlDoorState)pred->Ceiling.m_Status;
            Door->m_Line = pred->Ceiling.m_Line;
        }
	}
}

//
// CL_PredictSectors
//
void CL_PredictSectors (int predtic)
{
	for(size_t i = 0; i < real_plats.Size(); i++)
	{
		plat_pred_t *pred = &real_plats[i];
		sector_t *sec = &sectors[pred->secnum];

		if(pred->tic < predtic)
		{
            if (sec->ceilingdata && sec->ceilingdata->IsA(RUNTIME_CLASS(DPillar)))
            {
                sec->ceilingdata->RunThink();

                continue;
            } 

            if (sec->ceilingdata && sec->ceilingdata->IsA(RUNTIME_CLASS(DElevator)))
            {
                sec->ceilingdata->RunThink();

                continue;
            }  

            if (sec->floordata && sec->floordata->IsKindOf(RUNTIME_CLASS(DMovingFloor)))
            {
                sec->floordata->RunThink();
            }

            if (sec->ceilingdata && sec->ceilingdata->IsKindOf(RUNTIME_CLASS(DMovingCeiling)))
            {
                sec->ceilingdata->RunThink();
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
// CL_PredictMove
//
void CL_PredictMove (void)
{
	if (noservermsgs || netdemoPaused)
		return;

	player_t *p = &consoleplayer();

	if (!p->tic || !p->mo)
    {
        if (!p->spectator)
            return;
        
        // Predict sectors for local player who is a spectator
        CL_ResetSectors();
            
        int predtic = gametic - MAXSAVETICS;
            
        if(predtic < 0)
            predtic = 0;
            
        while(++predtic < gametic)
        {
            CL_PredictSectors(predtic);
        }
        
		return;      
    }
    
    #ifdef _PRED_DBG
    fixed_t origx, origy, origz;
    #endif

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

    #ifdef _PRED_DBG
    // Backup original position
	origx = p->mo->x;
	origy = p->mo->y;
	origz = p->mo->z;
    #endif
    
	// Disable sounds, etc, during prediction
	predicting = true;

	// Set predictable items to their last received positions
	CL_ResetPlayers();
	CL_ResetSectors();

	int predtic = gametic - MAXSAVETICS;

	if(predtic < 0)
		predtic = 0;

	// Predict each tic
	while(++predtic < gametic)
	{
		CL_PredictPlayers(predtic);
		CL_PredictSectors(predtic);
	}

	predicting = false;

	CL_PredictPlayers(predtic);
	// [Russell] - I don't think we need to call this as DThinker::RunThinkers()
	// will already run the moving sector thinkers after prediction
    //CL_PredictSectors(predtic);
    
    #ifdef _PRED_DBG
	if ((origx == p->mo->x) && (origy == p->mo->y) && (origz == p->mo->z))
    {
        Printf(PRINT_HIGH, "%d tics predicted\n", predtic);
    }
    else
    {
        Printf(PRINT_HIGH, "%d tics failed\n", predtic);
    }
    #endif
}

VERSION_CONTROL (cl_pred_cpp, "$Id$")

