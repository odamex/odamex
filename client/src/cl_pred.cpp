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

EXTERN_CVAR (cl_interp)
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
std::array<PlayerSnapshot, MAXSAVETICS> s_savedLocalPlayerSnaps;

using ActivePredictingSectorsVector = std::vector<decltype(movingsectors)::iterator>;

ActivePredictingSectorsVector s_predictingSectors;

std::unordered_set<size_t> s_playerInfluencedBySectorNumbers;
std::unordered_set<size_t> s_previousPredictingSectorNumbers;
std::unordered_set<size_t> s_currentPredictingSectorNumbers;

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

// Identifies which of the moving sectors are ready to predict based on previously-received snapshots
// and updates the given o_predictingSectors out-parameter accordingly.
//
// Also performs cleanup on the master list of moving sectors when sectors have finished their movement.
//
// Returns true if sector prediction is ready to proceed;  false if not, in which case,
// rollback cannot be performed and we must proceed with dead reckoning.
bool CL_PrepareSectorSnapshots(int snapBasisServerTic, const player_t& player, ActivePredictingSectorsVector& o_predictingSectors)
{
	std::list<movingsector_t>::iterator itr;
	itr = movingsectors.begin();

	bool sectorRollbackIsGoodToGo = true;

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
		bool sectorHasGoodSnapshot = false;

		const bool sectorHasAnySnapshots = mgr and not mgr->empty();
		if (sectorHasAnySnapshots)
		{
			SectorSnapshot snap = mgr->getSnapshot(snapBasisServerTic);

			// Double-check to make sure it's REALLY from the server and not extrapolated/etc.
			sectorHasGoodSnapshot = snap.isValid() and snap.isAuthoritative();
			if (sectorHasGoodSnapshot)
			{
				const bool ceilingdone = P_CeilingSnapshotDone(&snap);
				const bool floordone   = P_FloorSnapshotDone(&snap);

				if (ceilingdone and floordone)
					snapfinished = true;
				else
				{
					// Snapshots have been received for this sector recently, so
					// record it as ready for rollback-based prediction.
					o_predictingSectors.push_back(itr);
					s_currentPredictingSectorNumbers.insert(sectornum);
				}
			}
		}
		else
		{
			snapfinished = true;
		}

		if ((sectorHasAnySnapshots and not sectorHasGoodSnapshot) or not sectorHasAnySnapshots)
		{
			// Since we're trying to prepare for a rollback-based prediction, if the player is on
			// a moving sector that has not yet received snapshots, we have to bail out of the
			// rollback and just dead reckon until the snapshots show up.  If we don't do this, we
			// get players that rollback successfully, but the moving sector stays stationary.
			const AActor* mobj = player.mo;
			if (mobj and mobj->subsector)
			{
				for (const msecnode_t* sectorListNode = mobj->touching_sectorlist; sectorListNode; sectorListNode = sectorListNode->m_tnext)
				{
					if (sectorListNode->m_sector == sector)
					{
						sectorRollbackIsGoodToGo = false;
					}
				}
			}
		}

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
	return sectorRollbackIsGoodToGo;
}

//
// CL_ResetSectors
//
// Moves the given predicting sectors to the given server snapshot tic.
// Assumes that there was a previous, successful call to CL_PrepareSectorSnapshots.
void CL_ResetSectors(int snapBasisServerTic, ActivePredictingSectorsVector& io_predictingSectors)
{
	for (auto& iter : io_predictingSectors)
	{
		// We've already vetted the manager and snapshot in the preparation phase.
		const SectorSnapshotManager*  manager = CL_GetSectorSnapshotManager(iter->sector);
		const SectorSnapshot          snap    = manager->getSnapshot(snapBasisServerTic);
		snap.toSector(iter->sector);
	}
}

// Runs the given sector's floor and/or ceiling thinker, advancing its state by one tic.
// If the player was moved by the sector, the sector's number is recorded as having influenced
// the player's position.
//
void CL_PredictSector(const player_t& player, const movingsector_t& movsector)
{
	sector_t *sector = movsector.sector;

	const fixed_t originalPlayerZ       = player.mo->z;
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

	if (player.mo->z != originalPlayerZ)
	{
		s_playerInfluencedBySectorNumbers.insert(sector - sectors);
	}
}

// Runs prediction for the sectors included in the given predicting-sectors vector.
// This is intended for advancing snapshot-based rollback prediction/reconciliation
// sectors that can do so.
void CL_PredictSectors(const player_t& player, const ActivePredictingSectorsVector& io_predictingSectors)
{
	for (auto& iter : io_predictingSectors)
	{
		CL_PredictSector(player, *iter);
	}
}

// Runs prediction for all sectors, regardless of whether or not they have snapshots.
// This is intended for the canonical, final predictive and/or dead-reckoning step of sectors in the current tic,
// where the result of this operation is what will ultimately be fed to the renderer and subsequent tics.
void CL_PredictAllSectors(const player_t& player)
{
	for (const auto& movsector : movingsectors)
	{
		CL_PredictSector(player, movsector);
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
	P_SetPlayerSnapshotNoPosition(player, s_savedLocalPlayerSnaps[predtic % MAXSAVETICS]);

	// Note that we allow the caller to specify the input tic separately so
	// that we can predict what happens when multiple inputs are applied to
	// the same player snapshot, which sometimes happens on the server.
	const odaproto::clc::PlayerInput& netcmd = localcmds[inputTic % MAXSAVETICS];
	CLC_UnpackPlayerInputMessageToPlayer(netcmd, player);

	if (not predicting)
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
	int s_basisServerTic;
	int s_previousServerTic;
	int s_basisWorldPredictionTic;
	int s_worldPredictionVarianceCorrection;
	int s_worldPredictionTic;
	int s_previousWorldIndex;

	// Use a nice power-of-two so the compiler can optimize easily.
	// This also happens to be servertic vs world_index difference threshold
	// which triggers a Resync in SimulateWorld.
	constexpr int WORLD_INDEX_HISTORY_SIZE = 16;

	std::array<int, WORLD_INDEX_HISTORY_SIZE> s_worldIndexDeltaHistory;
	int64_t                                   s_worldIndexDeltaSum;
}

extern int world_index;
extern int lastEchoedClientTic;

void CL_ResetWorldPrediction()
{
	s_basisServerTic                    = 0;
	s_previousServerTic                 = 0;
	s_basisWorldPredictionTic           = 0;
	s_worldPredictionVarianceCorrection = 0;
	s_worldPredictionTic                = 0;
	s_previousWorldIndex                = world_index;

	s_savedLocalPlayerSnaps.fill(PlayerSnapshot{});
	s_worldIndexDeltaHistory.fill(0);
	s_worldIndexDeltaSum = 0;

	s_playerInfluencedBySectorNumbers.clear();
	s_previousPredictingSectorNumbers.clear();
	s_currentPredictingSectorNumbers.clear();
}

void CL_AdvanceWorldPredictionTic(int serverTic)
{
	if (s_basisWorldPredictionTic != 0 and s_previousServerTic < serverTic)
	{
		const int serverTicDeltaFromBasis = serverTic - s_basisServerTic;

		s_worldPredictionTic = s_basisWorldPredictionTic + serverTicDeltaFromBasis;
		s_previousServerTic  = serverTic;
	}
}

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

	if (world_index <= 0 or lastEchoedClientTic <= 0)     // No verified position from the server?
		return false;

	// Figure out where to start predicting from.
	// Please note that there are two starting points: The rollback start point for the player
	// motion and inputs, and the start point for server-owned states such as sectors, mobjs,
	// and remote players.
	//
	// The player input rollback tic is allowed to be (slightly) further in the past than the
	// world prediction tic.  This is because from the server's perspective, the player inputs
	// can be uneven due to client->server network lag + jitter and the queueing the server
	// does internally.  So as server tics progress, the local player state and inputs may not
	// progress evenly.  In the meantime, the world-owned states are monotonic with server tics.

	const int            snaptime           = p.snapshots.getMostRecentTime();
	const PlayerSnapshot snap               = p.snapshots.getSnapshot(snaptime);
	const int            idealPredictionTic = snap.getClientTic();
	const int            idealInputTic      = snap.getPlayerTic();

	// Maintain a running average of world index deltas to determine when comms are stable enough
	// to begin prediction.
	const int deltaWorldIndex = world_index - s_previousWorldIndex;
	s_previousWorldIndex = world_index;

	int& historyEntryRef = s_worldIndexDeltaHistory[gametic % s_worldIndexDeltaHistory.size()];

	s_worldIndexDeltaSum -= historyEntryRef;
	s_worldIndexDeltaSum += deltaWorldIndex;
	historyEntryRef       = deltaWorldIndex;

	// Do we need to restart prediction following a reset?  Hold off until world_index stabilizes.
	if (s_worldPredictionTic == 0)
	{
		const fixed_t fixedWorldIndexSum = (s_worldIndexDeltaSum << FRACBITS) / s_worldIndexDeltaHistory.size();
		if (fixedWorldIndexSum == FRACUNIT)
		{
			s_basisServerTic            = snaptime;
			s_basisWorldPredictionTic   = idealPredictionTic;
			s_worldPredictionTic        = idealPredictionTic;
		}
	}
	else
	{
		// World prediction is running.
		//
		// Keep an eye out for variance in world_index pacing.  If we see any, we need to
		// adjust our world prediction basis accordingly.

		const int deltaWorldIndexPaceVariance = deltaWorldIndex - 1;        // We expect a happy, consistent pace of 1.

		if (deltaWorldIndexPaceVariance)
		{
			s_worldPredictionVarianceCorrection += deltaWorldIndexPaceVariance;
		}
	}

	// Plus one because the values from the server are the latest canonical tics, and the first
	// tics we actually predict are the ones following those.
	int predictionTic = idealPredictionTic + 1;
	int inputTic      = idealInputTic      + 1;
	int worldPredictionTic = s_worldPredictionTic ? s_worldPredictionTic + s_worldPredictionVarianceCorrection + 1 : predictionTic;

	// Now start preparing the snapshots.  Disable sounds, etc, during prediction
	predicting = true;

	// Save a snapshot of the player's state before prediction
	const PlayerSnapshot prevsnap(p.tic, p, ::lastEchoedClientTic);
	s_savedLocalPlayerSnaps[gametic % MAXSAVETICS] = prevsnap;

	s_currentPredictingSectorNumbers.swap(s_previousPredictingSectorNumbers);
	s_currentPredictingSectorNumbers.clear();
	s_playerInfluencedBySectorNumbers.clear();
	s_predictingSectors.clear();

	const bool sectorsAreReady = cl_predictsectors ? CL_PrepareSectorSnapshots(snaptime, p, s_predictingSectors) : true;

	// The value clamping here and the prediction iterations below are designed to ensure that
	// if we get an invalid predictionTic due to extreme jitter or very recent prediction reset,
	// we don't rollback - we just proceed with the next tic.

	int startingLocalTic = std::min({ predictionTic, inputTic, worldPredictionTic });

	const bool startingLocalTicIsInRange = gametic - MAXSAVETICS <= startingLocalTic and startingLocalTic <= gametic;

	const bool isReadyToRollback = sectorsAreReady and startingLocalTicIsInRange;

	if (isReadyToRollback)
	{
		// Move the client to the last position received from the server.
		snap.toPlayer(p);
	}
	else
	{
		// Not ready for rollback-based prediction+reconciliation.
		// Set tic values that force us to just simply dead reckon into gametic.
		predictionTic      = gametic;
		inputTic           = gametic;
		worldPredictionTic = gametic;
		startingLocalTic   = gametic;
	}

//#define ODAMEX_PREDICTION_DEBUG           // SUPER IMPORTANT NOTE WITH THIS BLOCK:
#ifdef ODAMEX_PREDICTION_DEBUG              //  Sector 1 is checked because nuts.wad is such a great test case here,
	const sector_t& sector = sectors[1];    //  and sector 1 is the large Lift sector that the player spawns on and
	                                        //  has a bunch of weapons and items.
	DPrintFmt("gt {}, snaptime {}, pred {}, input {}, world {}, initsecheight(off-one) {}, world_index {}\n",
	        gametic,
	        snaptime,
	        predictionTic,
	        inputTic,
	        worldPredictionTic,
	        P_FloorHeight(&sector),
	        world_index
	        );
#endif

	// Move sectors to the last position received from the server.
	if (cl_predictsectors)
		CL_ResetSectors(snaptime, s_predictingSectors);

	bool mobjsHaveBeenPredicted = false;
	bool playerHasBeenPredicted = false;
	bool playerHasBeenAcceleratedByNewlyPredictedSector = false;

	for (int i = startingLocalTic; i < gametic; ++i)
	{
		if (i >= inputTic)
		{
			if (CL_PredictLocalPlayer(std::max(i, predictionTic), i))   // std::max because inputTic *can* be behind predictionTic.
			{
				playerHasBeenPredicted = true;
			}
		}

		if (i >= worldPredictionTic)
		{
			if (not mobjsHaveBeenPredicted)
			{
				mobjsHaveBeenPredicted = true;

				// We're doing our genuine thinker step now, and it must always be on the
				// tic following the latest from the server.  This ensures that mobj actions
				// that reference the player's position are working from the player state
				// that the server almost certainly had when it ran the tic for real.
				predicting = false;

				// On the server, players are updated before the other thinkers.
				CL_SimulatePlayers(snaptime);

				DThinker::RunThinkers();
				predicting = true;
			}

			if (cl_predictsectors)
				CL_PredictSectors(p, s_predictingSectors);
		}
	}

	// Determine whether or not the player's vertical position was influenced by a sector
	// that just started prediction.  We'll use this to trigger a special case below.
	for (const int sectorNumber : s_playerInfluencedBySectorNumbers)
	{
		if (    s_currentPredictingSectorNumbers.contains(sectorNumber) and
		    not s_previousPredictingSectorNumbers.contains(sectorNumber))
		{
			playerHasBeenAcceleratedByNewlyPredictedSector = true;
			break;
		}
	}

	// If the player didn't just spawn, teleport, or make a discontinuous jump thanks to
	// a sector starting prediction due to remote activation, nudge the player from
	// his position last tic to this new corrected position.  This smooths the
	// view when there's a misprediction.
	bool wasMispredicted = false;
	if (snap.isContinuous() and playerHasBeenPredicted)
	{
		const PlayerSnapshot correctedprevsnap(p.tic, p, ::lastEchoedClientTic);

		// Did we predict correctly?
		wasMispredicted = (correctedprevsnap.getX() != prevsnap.getX()) or
		                  (correctedprevsnap.getY() != prevsnap.getY()) or
		                  (correctedprevsnap.getZ() != prevsnap.getZ());

		// Nudge the player unless the player was moved suddenly by a sector.  In that case, we'll
		// make a different adjustment after performing the final player think below.
		if (wasMispredicted and not playerHasBeenAcceleratedByNewlyPredictedSector)
		{
			// Update the netgraph concerning our prediction's error
			netgraph.setMisprediction(true);

			// Lerp from our previous-previous position to the corrected-previous position
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
		CL_SimulatePlayers(snaptime);
		DThinker::RunThinkers();
	}

	if (cl_predictsectors)
		CL_PredictAllSectors(p);

	// Special case:  The very first time that we move on account of a sector whose initial
	//                snapshot just now showed up, but required multiple prediction tics,
	//                kill vertical view interpolation.  It's much less jarring to just
	//                "remain in place" on a sector that suddenly moved more than expected,
	//                than it is to visibly accelerate, move, and decelerate between 2 tics.
	if (playerHasBeenPredicted and playerHasBeenAcceleratedByNewlyPredictedSector and wasMispredicted)
	{
		p.prevviewz = p.viewz;
	}

	return mobjsHaveBeenPredicted;
}

VERSION_CONTROL (cl_pred_cpp, "$Id$")
