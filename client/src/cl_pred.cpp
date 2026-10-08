// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	Client-side prediction of local player, other players and moving sectors
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "d_player.h"
#include "p_local.h"
#include "cl_main.h"
#include "cl_demo.h"
#include "cl_netgraph.h"
#include "clc_message.h"

#include "p_snapshot.h"

EXTERN_CVAR (cl_prednudge)
EXTERN_CVAR (cl_predictsectors)

extern NetGraph netgraph;

void P_MovePlayer (player_t& player);
void P_CalcHeight (player_t& player);
void P_HeightClipAllSectorThings(sector_t& sector);

extern odaproto::clc::PlayerInput localcmds[MAXSAVETICS];

bool predicting;

extern std::map<unsigned short, SectorSnapshotManager> sector_snaps;

namespace
{
std::array<PlayerSnapshot, MAXSAVETICS> cl_savedsnaps;

using ActivePredictingSectorsVector = std::vector<decltype(movingsectors)::iterator>;

ActivePredictingSectorsVector s_predictingSectors;

//
// CL_GetSnapshotManager
//
// Returns the SectorSnapshotManager for the sector.
// Returns NULL if a snapshots aren't currently stored for the sector.
//
SectorSnapshotManager *CL_GetSectorSnapshotManager(sector_t *sector)
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

bool CL_SectorHasSnapshots(sector_t *sector)
{
	SectorSnapshotManager *mgr = CL_GetSectorSnapshotManager(sector);

	return (mgr && !mgr->empty());
}

//
// CL_ResetSectors
//
// Moves predicting sectors to the given server snapshot tic.
// Also performs cleanup on the list of predicting sectors when
// sectors have finished their movement.
//
void CL_ResetSectors(int snapBasisServerTic, ActivePredictingSectorsVector& io_predictingSectors)
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
			SectorSnapshot snap = mgr->getSnapshot(snapBasisServerTic);

			// Double-check to make sure it's REALLY from the server and not extrapolated/etc.
			if (snap.isValid() and snap.isAuthoritative())
			{
				const bool ceilingdone = P_CeilingSnapshotDone(&snap);
				const bool floordone   = P_FloorSnapshotDone(&snap);

				if (ceilingdone and floordone)
					snapfinished = true;
				else
				{
					// snapshots have been received for this sector recently, so
					// reset this sector to the most recent snapshot from the server
					snap.toSector(sector);
					io_predictingSectors.push_back(itr);
				}
			}
		}
		else
			snapfinished = true;


		if (    snapfinished
		    and P_MovingCeilingCompleted(sector)
		    and P_MovingFloorCompleted(sector))
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

void CL_PredictSector(const movingsector_t& movsector)
{
	sector_t *sector = movsector.sector;

	const fixed_t originalFloorHeight   = P_FloorHeight  (sector);
	const fixed_t originalCeilingHeight = P_CeilingHeight(sector);

	if (sector and sector->ceilingdata and movsector.moving_ceiling)
		sector->ceilingdata->RunThink();
	if (sector and sector->floordata and movsector.moving_floor)
		sector->floordata->RunThink();

	// Because of the RunThinkers that takes place on a rolled-back state via ResetSectors, it's possible for
	// a recently-stopped predicted sector to be motionless, but the things on it have their prevz (and
	// potentially more attributes) left at an earlier state, causing visible, temporary glitches.  We avoid
	// this by detecting if a predicted sector is actually NOT in motion and then if so, re-clip its things.
	// A sector that IS in motion makes its own P_ThingHeightClip call in P_ChangeSector.
	if (sector
	    and originalFloorHeight   == P_FloorHeight(sector)
	    and originalCeilingHeight == P_CeilingHeight(sector))
	{
		P_HeightClipAllSectorThings(*sector);
	}
}

void CL_PredictSectors(const ActivePredictingSectorsVector& io_predictingSectors)
{
	for (auto& iter : io_predictingSectors)
	{
		CL_PredictSector(*iter);
	}
}

void CL_PredictAllSectors()
{
	for (const auto& movsector : movingsectors)
	{
		CL_PredictSector(movsector);
	}
}

//
// CL_PredictSpying
//
// Handles calling the thinker routines for the player being spied with spynext.
//
void CL_PredictSpying()
{
	player_t& player = displayplayer();
	if (consoleplayer_id == displayplayer_id)
		return;

	// Save and restore the prevangle and prevpitch so that the client viewangle interpolation
	// actually works when spying.
	//
	// This is needed because the spy target's prevangle and prevpitch are already accurate
	// thanks to CL_SimulatePlayers, but P_PlayerThink overwrites them, assuming that it's
	// ultimately responsible for moving players.  That just isn't the case for spied remote
	// players.  We can simply restore the overwritten states to allow interpolation to work.

	const auto prevangle = player.mo->prevangle;
	const auto prevpitch = player.mo->prevpitch;

	predicting = false;

	P_PlayerThink(player);
	P_CalcHeight(player);

	player.mo->prevangle = prevangle;
	player.mo->prevpitch = prevpitch;
}

//
// CL_PredictRemotePlayers
//
//
void CL_PredictRemotePlayers()
{
	for (auto& player : players)
	{
		if (player.ingame() 
			&& player.mo 
			&& player.id != consoleplayer_id   // handled in CL_PredictWorld
			&& player.id != displayplayer_id)  // handled in CL_PredictSpying
		{
			P_BumpPlayerCounters(player);
		}
	}
}

//
// CL_PredictFreecam
//
//
void CL_PredictFreecam()
{
	player_t& player = displayplayer();
	if (not player.isFreecam)
		return;

	if (player.mo)
	{
		player.mo->prevx = player.mo->x;
		player.mo->prevy = player.mo->y;
		player.mo->prevz = player.mo->z;
	}

	predicting = true;

	P_PlayerThink(player);

	predicting = false;
}

//
// CL_PredictSpectator
//
//
void CL_PredictSpectator()
{
	player_t& player = consoleplayer();
	if (!player.spectator)
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
bool CL_PredictLocalPlayer(int predtic, int inputTic)
{
	player_t& player = consoleplayer();

	if (!player.ingame() || !player.mo || player.tic >= predtic)
		return false;

	// Restore the angle, viewheight, etc for the player
	P_SetPlayerSnapshotNoPosition(player, cl_savedsnaps[predtic % MAXSAVETICS]);

	// Note that we allow the caller to specify the input tic separately so
	// that we can predict what happens when multiple inputs are applied to
	// the same player snapshot, which sometimes happens on the server.
	odaproto::clc::PlayerInput& netcmd = localcmds[inputTic % MAXSAVETICS];
	CLC_UnpackPlayerInputMessageToPlayer(netcmd, player);

	if (!predicting)
		P_PlayerThink(player);
	else
		P_MovePlayer(player);

	player.mo->RunThink();
	return true;
}

}   // anonymous namespace

//
// CL_SectorIsPredicting
//
// Returns true if the client is predicting sector
//
bool CL_SectorIsPredicting(sector_t *sector)
{
	if (not sector or not cl_predictsectors)
		return false;

	const auto itr = P_FindMovingSector(sector);
	if (itr != movingsectors.end() and sector == itr->sector)
		return (itr->moving_ceiling or itr->moving_floor);

	// sector not found
	return false;
}


namespace
{
	int s_previousBasisServerTic;
	int s_previousReceivedClientTic;
	int s_persistentDiff;
}

extern int world_index;
extern int lastEchoedClientTic;

//
// CL_PredictWorld
//
// Main function for client-side prediction.  Returns true if the prediction included
// actually stepping all the mobj thinkers for the current gametic, in which case, the
// caller must take care to not step them again.
//
bool CL_PredictWorld()
{
	if (gamestate != GS_LEVEL)
		return false;

	if (netdemo.isPaused() && displayplayer().isFreecam)
	{
		CL_PredictFreecam();
		return false;
	}

	player_t& p = consoleplayer();

	if (!validplayer(p) || !p.mo || noservermsgs || netdemo.isPaused())
		return false;

	// tenatively tell the netgraph that our prediction was successful
	netgraph.setMisprediction(false);

	if (consoleplayer_id != displayplayer_id && displayplayer().isFreecam)
		CL_PredictFreecam();

	if (consoleplayer_id != displayplayer_id && not displayplayer().isFreecam)
		CL_PredictSpying();

	CL_PredictRemotePlayers();

	// [SL] 2012-03-10 - Spectators can predict their position without server
	// correction.  Handle them as a special case and leave.
	if (consoleplayer().spectator)
	{
		CL_PredictSpectator();
		return false;
	}

	if (p.tic <= 0 or world_index <= 0 or lastEchoedClientTic <= 0)     // No verified position from the server?
		return false;

	// Disable sounds, etc, during prediction
	predicting = true;

	// Save a snapshot of the player's state before prediction
	PlayerSnapshot prevsnap(p.tic, p);
	cl_savedsnaps[gametic % MAXSAVETICS] = prevsnap;

	// Mobjs are already in the last position received from the server.
	bool mobjsHaveBeenPredicted = false;

	// Figure out where to start predicting from.
	//
	// lastEchoedClientTic is the absolute latest clientside gametic that the server knows about.
	// p.tic is the latest local gametic whose PlayerInput is integrated into the player state.
	// p.tic can be the same as, or behind, the lastEchoedClientTic, but never ahead of it.
	// If we see that p.tic is behind, we guess that the server is going to do two player tics
	// before advancing world state to get caught up.
	//
	// We start predicting from the lastEchoedClientTic.

	const int snaptime           = p.snapshots.getMostRecentTime();
	const int idealPredictionTic = std::max(lastEchoedClientTic + 1, gametic - MAXSAVETICS);
	const int idealInputTic      = std::max(p.tic + 1,               gametic - MAXSAVETICS);

	if (s_previousBasisServerTic == 0)
	{
		s_previousBasisServerTic = snaptime;
	}
	if (s_previousReceivedClientTic == 0)
	{
		s_previousReceivedClientTic = idealPredictionTic;
	}

	const int deltaServerTic = snaptime           - s_previousBasisServerTic;
	const int deltaPredTic   = idealPredictionTic - s_previousReceivedClientTic;

	s_previousBasisServerTic    = snaptime;
	s_previousReceivedClientTic = idealPredictionTic;

	s_persistentDiff += deltaServerTic - deltaPredTic;

	int predictionTic = idealPredictionTic + s_persistentDiff;
	int inputTic      = idealInputTic;

//#define ODAMEX_PREDICTION_DEBUG           // SUPER IMPORTANT NOTE WITH THIS BLOCK:
#ifdef ODAMEX_PREDICTION_DEBUG              //  Sector 1 is checked because nuts.wad is such a great test case here,
	const sector_t& sector = sectors[1];    //  and sector 1 is the large Lift sector that the player spawns on and
	                                        //  has a bunch of weapons and items.
	DPrintFmt("gt {}, snaptime {}, pred {}, input {}, initsecheight(off-one) {}, world_index {}, cor_pred {}, cor_input {}, persdiff {}\n",
	        gametic,
	        snaptime,
	        idealPredictionTic,
	        idealInputTic,
	        P_FloorHeight(&sector),
	        world_index,
	        predictionTic,
	        inputTic,
	        s_persistentDiff
	        );
#endif

	// Move the client to the last position received from the server.
	PlayerSnapshot snap = p.snapshots.getSnapshot(snaptime);
	snap.toPlayer(p);

	s_predictingSectors.clear();

	// Move sectors to the last position received from the server.
	if (cl_predictsectors)
		CL_ResetSectors(snaptime, s_predictingSectors);

	bool playerWasPredicted = false;

	// Because input tic can be behind or match, but never exceed, predictionTic,
	// make sure we integrate any extra older inputs that we know the server hasn't
	// integrated into the player state it has.
	//
	// This will predict the player state up to predictionTic.

	for (;inputTic < predictionTic; ++inputTic)
	{
		// We exclude thinkers and sectors here because the latest integrated input
		// is from BEFORE the latest server-originated states (sectors, mobj state).
		//
		// Please note that we supply predictionTic as the basis tic so that we're
		// simulating the server integrating multiple inputs onto its latest player state.
		if (CL_PredictLocalPlayer(predictionTic, inputTic))
		{
			playerWasPredicted = true;
		}
	}

	// Now we're on the predictionTic.  We advance the player, the thinkers/mobjs,
	// and the sectors up to (but NOT including) gametic.
	for (;predictionTic < gametic; ++predictionTic)
	{
		if (CL_PredictLocalPlayer(predictionTic, predictionTic))
		{
			playerWasPredicted = true;
		}
		if (not mobjsHaveBeenPredicted)
		{
			mobjsHaveBeenPredicted = true;

			// We're doing our genuine thinker step now, and it must always be on the
			// tic following the latest from the server.  This ensures that mobj actions
			// that reference the player's position are working from the player state
			// that the server almost certainly had when it ran the tic for real.
			predicting = false;
			DThinker::RunThinkers();
			predicting = true;
		}

		if (cl_predictsectors)
			CL_PredictSectors(s_predictingSectors);
	}

	// If the player didn't just spawn or teleport, nudge the player from
	// his position last tic to this new corrected position.  This smooths the
	// view when there's a misprediction.
	if (playerWasPredicted and snap.isContinuous())
	{
		PlayerSnapshot correctedprevsnap(p.tic, p);

		// Did we predict correctly?
		const bool correct = (correctedprevsnap.getX() == prevsnap.getX()) and
		                     (correctedprevsnap.getY() == prevsnap.getY()) and
		                     (correctedprevsnap.getZ() == prevsnap.getZ());

		if (not correct)
		{
			// Update the netgraph concerning our prediction's error
			netgraph.setMisprediction(true);

			// Lerp from the our previous position to the correct position
			PlayerSnapshot lerpedsnap = P_LerpPlayerPosition(prevsnap, correctedprevsnap, cl_prednudge);
			lerpedsnap.toPlayer(p);
		}
	}

	// Now we're doing the big final canonical client side update of player state,
	// all thinkers, and sectors.  Disable the predicting control and do the gametic.
	predicting = false;

	CL_PredictLocalPlayer(gametic, gametic);

	if (not mobjsHaveBeenPredicted)
	{
		mobjsHaveBeenPredicted = true;
		DThinker::RunThinkers();
	}

	if (cl_predictsectors)
		CL_PredictAllSectors();

	return mobjsHaveBeenPredicted;
}

void CL_ResetWorldPrediction()
{
	s_previousBasisServerTic = 0;
	s_previousReceivedClientTic = 0;
	s_persistentDiff = 0;
	for (auto& savedPlayerSnapshot : cl_savedsnaps)
	{
		savedPlayerSnapshot = PlayerSnapshot{};
	}
}

VERSION_CONTROL (cl_pred_cpp, "$Id$")
