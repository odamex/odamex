// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2015 by The Odamex Team.
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
#include "m_vectors.h"
#include "cl_netgraph.h"

#include "p_snapshot.h"

EXTERN_CVAR (co_realactorheight)
EXTERN_CVAR (cl_prednudge)
EXTERN_CVAR (cl_predictsectors)
EXTERN_CVAR (cl_predictlocalplayer)

extern NetGraph netgraph;

void P_DeathThink (player_t *player);
void P_MovePlayer (player_t *player);
void P_CalcHeight (player_t *player);

extern NetCommand localcmds[MAXSAVETICS];
static PlayerSnapshot cl_savedsnaps[MAXSAVETICS];

bool predicting;

extern std::map<unsigned short, SectorSnapshotManager> sector_snaps;


//
// CL_GetSnapshotManager
//
// Returns the SectorSnapshotManager for the sector.
// Returns NULL if a snapshots aren't currently stored for the sector.
//
static SectorSnapshotManager *CL_GetSectorSnapshotManager(sector_t *sector)
{
	unsigned short sectornum = sector - sectors;
	if (!sector || sectornum >= numsectors)
		return NULL;

	std::map<unsigned short, SectorSnapshotManager>::iterator mgr_itr;
	mgr_itr = sector_snaps.find(sectornum);
	
	if (mgr_itr != sector_snaps.end())
		return &(mgr_itr->second);
	
	return NULL;
}

static bool CL_SectorHasSnapshots(sector_t *sector)
{
	SectorSnapshotManager *mgr = CL_GetSectorSnapshotManager(sector);
	
	return (mgr && !mgr->empty());
}

//
// CL_SectorIsPredicting
//
// Returns true if the client is predicting sector
//
bool CL_SectorIsPredicting(sector_t *sector)
{
	if (!sector || !cl_predictsectors)
		return false;
		
	std::list<movingsector_t>::iterator itr = P_FindMovingSector(sector);
	if (itr != movingsectors.end() && sector == itr->sector)
		return (itr->moving_ceiling || itr->moving_floor);

	// sector not found	
	return false;
}

//
// CL_ResetSectors
//
// Moves predicting sectors to their most recent snapshot received from the
// server.  Also performs cleanup on the list of predicting sectors when
// sectors have finished their movement.
//
static void CL_ResetSectors()
{
	std::list<movingsector_t>::iterator itr;
	itr = movingsectors.begin();
	
	// Iterate through all predicted sectors
	while (itr != movingsectors.end())
	{
		sector_t *sector = itr->sector;
		unsigned short sectornum = sector - sectors;
		if (sectornum >= numsectors)
			continue;
		
		// Find the most recent snapshot received from the server for this sector
		SectorSnapshotManager *mgr = CL_GetSectorSnapshotManager(sector);

		bool snapfinished = false;
		
		if (mgr && !mgr->empty())
		{
			int mostrecent = mgr->getMostRecentTime();
			SectorSnapshot snap = mgr->getSnapshot(mostrecent);
			
			bool ceilingdone = P_CeilingSnapshotDone(&snap);
			bool floordone = P_FloorSnapshotDone(&snap);
			
			if (ceilingdone && floordone)
				snapfinished = true;
			else
			{
				// snapshots have been received for this sector recently, so
				// reset this sector to the most recent snapshot from the server
				snap.toSector(sector);
			}
		}
		else
			snapfinished = true;


		if (snapfinished && P_MovingCeilingCompleted(sector) &&
			P_MovingFloorCompleted(sector))
		{
			// no valid snapshots in the container so remove this sector from the
			// movingsectors list whenever prediction is done
			movingsectors.erase(itr++);
		}
		else
		{
			++itr;
		}
	}	
}

//
// CL_PredictSectors
//
//
static void CL_PredictSectors(int predtic)
{
	std::list<movingsector_t>::iterator itr;
	for (itr = movingsectors.begin(); itr != movingsectors.end(); ++itr)
	{
		sector_t *sector = itr->sector;
		
		// If we haven't started receiving updates for this sector from the server,
		// we only need to run the thinker for the current tic, not any past tics
		// since the sector hasn't been reset to a previous update snapshot
		if (predtic < gametic && !CL_SectorHasSnapshots(sector))
			continue;

		if (sector && sector->ceilingdata && itr->moving_ceiling)
			sector->ceilingdata->RunThink();
		if (sector && sector->floordata && itr->moving_floor)
			sector->floordata->RunThink();				
	}
}

//
// CL_PredictSpying
//
// Handles calling the thinker routines for the player being spied with spynext.
//
static void CL_PredictSpying()
{
	player_t *player = &displayplayer();
	if (consoleplayer_id == displayplayer_id)
		return;

	predicting = false;
	
	P_PlayerThink(player);
	P_CalcHeight(player);
}

//
// CL_PredictSpectator
//
//
static void CL_PredictSpectator()
{
	player_t *player = &consoleplayer();
	if (!player->spectator)
		return;
		
	predicting = true;
	
	P_PlayerThink(player);
	P_CalcHeight(player);
	
	predicting = false;
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

	// Restore the angle, viewheight, etc for the player
	P_SetPlayerSnapshotNoPosition(player, cl_savedsnaps[predtic % MAXSAVETICS]);

	// Copy the player's previous input ticcmd for the tic 'predtic'
	// to player.cmd so that P_MovePlayer can simulate their movement in
	// that tic
	NetCommand *netcmd = &localcmds[predtic % MAXSAVETICS];
	netcmd->toPlayer(player);

	if (!cl_predictlocalplayer)
	{
		if (predtic == gametic)
		{
			P_PlayerThink(player);
			player->mo->RunThink();
		}

		return;
	}

	if (!predicting)
		P_PlayerThink(player);
	else
		P_MovePlayer(player);

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

	if (consoleplayer_id != displayplayer_id)
		CL_PredictSpying();

	// [SL] 2012-03-10 - Spectators can predict their position without server
	// correction.  Handle them as a special case and leave.
	if (consoleplayer().spectator)
	{
		CL_PredictSpectator();
		return;
	}
		
	if (p->tic <= 0)	// No verified position from the server
		return;

	// Disable sounds, etc, during prediction
	predicting = true;
	
	// Figure out where to start predicting from
	int predtic = consoleplayer().tic > 0 ? consoleplayer().tic: 0;
	// Last position update from the server is too old!
	if (predtic < gametic - MAXSAVETICS)
		predtic = gametic - MAXSAVETICS;
	
	// Save a snapshot of the player's state before prediction
	PlayerSnapshot prevsnap(p->tic, p);
	cl_savedsnaps[gametic % MAXSAVETICS] = prevsnap;

	// Move sectors to the last position received from the server
	if (cl_predictsectors)
		CL_ResetSectors();

	// Move the client to the last position received from the sever
	int snaptime = p->snapshots.getMostRecentTime();
	PlayerSnapshot snap = p->snapshots.getSnapshot(snaptime);
	snap.toPlayer(p);

	if (cl_predictlocalplayer)
	{
		while (++predtic < gametic)
		{
			if (cl_predictsectors)
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
	}

	predicting = false;

	// Run thinkers for current gametic
	if (cl_predictsectors)
		CL_PredictSectors(gametic);		
	CL_PredictLocalPlayer(gametic);
}


VERSION_CONTROL (cl_pred_cpp, "$Id$")

