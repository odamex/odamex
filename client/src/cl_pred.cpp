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
#include "cl_demo.h"
#include "vectors.h"
#include "cl_netgraph.h"

#include "p_snapshot.h"

EXTERN_CVAR (co_realactorheight)
EXTERN_CVAR (cl_prednudge)

extern NetGraph netgraph;

void P_DeathThink (player_t *player);
void P_MovePlayer (player_t *player);
void P_CalcHeight (player_t *player);

static ticcmd_t cl_savedticcmds[MAXSAVETICS];
static PlayerSnapshot cl_savedsnaps[MAXSAVETICS];

bool predicting;

TArray <plat_pred_t> real_plats;

CVAR_FUNC_IMPL(cl_prednudge)
{
	// [SL] 2012-03-23 - Don't allow the client to set it to 0
	// That would ignore position updates from the server
	if (var < 0.05f)
		var.Set(0.05f);
	if (var > 1.0f)
		var.Set(1.0f);
}

//
// CL_ResetSectors
//
static void CL_ResetSectors (void)
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
            
			P_SetFloorHeight(sec, pred->floorheight);
			P_SetCeilingHeight(sec, pred->ceilingheight);
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

			P_SetFloorHeight(sec, pred->floorheight);
			P_SetCeilingHeight(sec, pred->ceilingheight);
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

			P_SetFloorHeight(sec, pred->floorheight);
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
            
			P_SetFloorHeight(sec, pred->floorheight);
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

			P_SetCeilingHeight(sec, pred->ceilingheight);
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

			P_SetCeilingHeight(sec, pred->ceilingheight);
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
static void CL_PredictSectors (int predtic)
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
// CL_PredictLocalPlayer
//
// 
static void CL_PredictLocalPlayer(int predtic)
{
	player_t *player = &consoleplayer();
	
	if (!player->ingame() || !player->mo || player->tic >= predtic)
		return;

	// Copy the player's previous input ticcmd for the tic 'predtic'
	// to player.cmd so that P_MovePlayer can simulate their movement in
	// that tic
	memcpy(&player->cmd, &cl_savedticcmds[predtic % MAXSAVETICS], sizeof(ticcmd_t));
	
	// Restore the angle, viewheight, etc for the player
	P_SetPlayerSnapshotNoPosition(player, cl_savedsnaps[predtic % MAXSAVETICS]);

	if (player->playerstate != PST_DEAD)
		P_MovePlayer(player);
		
	if (!predicting)
		P_PlayerThink(player);
		
	P_CalcHeight(player);	
	
	player->mo->RunThink();
}

//
// CL_PredictWorld
//
// Main function for client-side prediction.
// 
void CL_PredictWorld(void)
{
	if (gamestate != GS_LEVEL)
		return;

	player_t *p = &consoleplayer();

	if (!validplayer(*p) || !p->mo || noservermsgs || netdemo.isPaused())
		return;

	// tenatively tell the netgraph that our prediction was successful
	netgraph.setMisprediction(false);

	// [SL] 2012-03-10 - Spectators can predict their position without server
	// correction.  Handle them as a special case and leave.
	if (consoleplayer().spectator)
	{
		P_MovePlayer(&consoleplayer());
		P_PlayerThink(&consoleplayer());
		P_CalcHeight(&consoleplayer());
		
		if (consoleplayer_id != displayplayer_id)
		{
			P_PlayerThink(&displayplayer());
			P_CalcHeight(&displayplayer());		
		}
		
		return;
	}

	if (p->tic <= 0)	// No verified position from the server
		return;

	// Clear out past movements if we're dead!
	if (consoleplayer().playerstate == PST_DEAD)
		for (int i = 0; i < MAXSAVETICS; i++)
			P_ClearTiccmdMovement(&cl_savedticcmds[i]);

	// Figure out where to start predicting from
	int predtic = consoleplayer().tic > 0 ? consoleplayer().tic: 0;
	// Last position update from the server is too old!
	if (predtic < gametic - MAXSAVETICS)
		predtic = gametic - MAXSAVETICS;
	
	// Save a copy of the player's input for the current tic
	memcpy(&cl_savedticcmds[gametic % MAXSAVETICS], &p->cmd, sizeof(ticcmd_t));

	// Save a snapshot of the player's state before prediction
	PlayerSnapshot prevsnap(p->tic, p);
	cl_savedsnaps[gametic % MAXSAVETICS] = prevsnap;

	// Move sectors to the last position received from the server
	CL_ResetSectors();

	// Move the client to the last position received from the sever
	int snaptime = p->snapshots.getMostRecentTime();
	PlayerSnapshot snap = p->snapshots.getSnapshot(snaptime);
	snap.toPlayer(p);

	// Disable sounds, etc, during prediction
	predicting = true;

	while (++predtic < gametic)
	{
		CL_PredictSectors(predtic);
		CL_PredictLocalPlayer(predtic);
	}

	// If the player didn't just spawn or teleport, nudge the player from
	// his position last tic to this new corrected position.  This smooths the
	// view when there's a misprediction.
	if (snap.isContinuous())
	{
		PlayerSnapshot correctedprevsnap(p->tic, p);

		// Did we predict correctly?
		bool correct = (correctedprevsnap.getX() == prevsnap.getX()) &&
					   (correctedprevsnap.getY() == prevsnap.getY()) &&
					   (correctedprevsnap.getZ() == prevsnap.getZ());

		if (!correct)
		{
			// Update the netgraph concerning our prediction's error
			netgraph.setMisprediction(true);

			// Lerp from the our previous position to the correct position
			PlayerSnapshot lerpedsnap = P_LerpPlayerPosition(prevsnap, correctedprevsnap, cl_prednudge);	
			lerpedsnap.toPlayer(p);
		}
	}

	predicting = false;

	CL_PredictSectors(gametic);
	CL_PredictLocalPlayer(gametic);

	if (consoleplayer_id != displayplayer_id)
	{
		P_PlayerThink(&displayplayer());
		P_CalcHeight(&displayplayer());		
	}
}


VERSION_CONTROL (cl_pred_cpp, "$Id$")

