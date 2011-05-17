// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: p_unlag.cpp $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//   [SL] 2011-05-11 - The Unlagging System is used to apply lag compensation
//   for players firing hitscan weapons such as shotguns/chaingun.  The end
//   result is that players should no longer need to lead their opponents
//   with hitscan weapons.
//  
//   It maintains a history of the positions of every player and the
//   floor/ceiling heights of every moving sector.  When a player tries to fire
//   a hitscan weapon, the system calculates that client's lag X, moves all
//   other players (excluding the shooter) to their position X tics ago, fires
//   the weapon, then moves all the players back to their original position.  
//   In the system, this is refered to as 'reconciling' (moving players to a
//   prior position) and 'restoring' (moving players back to their proper
//   positions).
//
//-----------------------------------------------------------------------------


#include "doomdef.h"
#include "doomstat.h"
#include "vectors.h"
#include "r_main.h"
#include "p_unlag.h"

const size_t Unlag::MAX_HISTORY_TICS;   // defined in p_unlag.h

EXTERN_CVAR(sv_unlag)

//
// Unlag::getInstance
//
// Singleton pattern
// Returns a pointer to the only allowable Unlag object
//

Unlag& Unlag::getInstance()
{
	static Unlag instance;
	return instance;
}

Unlag::~Unlag()
{
	Unlag::reset();   
}

//
// Unlag::enabled
//
// Denotes whether sv_unlag is set and it is a multiplayer game
// run on the server.
 
bool Unlag::enabled()
{
	return (sv_unlag && serverside && multiplayer);
}

//
// Unlag::movePlayer
//
// Moves the player to the position specified by x, y, z.  Should be called
// AFTER Unlag::moveSector so the floor and ceiling heights will be correct.
// Note: this ignores the possibility that two players could share the same
// location.
//

void Unlag::movePlayer(player_t *player, fixed_t x, fixed_t y, fixed_t z)
{
	if (!player->mo)
		return;

	subsector_t *dest_subsector = R_PointInSubsector (x,y);
	
	player->mo->SetOrigin(x, y, z);
	player->mo->floorz = dest_subsector->sector->floorheight;
	player->mo->ceilingz = dest_subsector->sector->ceilingheight;
}


//
// Unlag::moveSector
//
// Moves the ceiling and floor heights to those specified by
// ceilingheight and floorheight respectively
//

void Unlag::moveSector(sector_t *sector, fixed_t ceilingheight, 
					   fixed_t floorheight)
{
	sector->ceilingheight = ceilingheight;
	sector->floorheight = floorheight;
}


//
// Unlag::reconcilePlayerPositions
//
// Moves all of the players except 'shooter' to the position they were
// at 'ticsago' tics before.  Players who were not alive at that time
// have their MF_SHOOTABLE flag removed so they do not take damage.
//
// If Unlag::reconcile is true, restore all player positions to their state
// before reconciliation.  Restore the MF_SHOOTABLE flag if we changed it.
//
// NOTE: ticsago should be > 0 if we're reconciling and not restoring
//

void Unlag::reconcilePlayerPositions(byte shooter_id, size_t ticsago)
{
	for (size_t i=0; i<player_history.size(); i++)
	{
		player_t *player = player_history[i].player;

		// skip over the player shooting and any spectators
		if (player->id == shooter_id || player->spectator || !player->mo)
			continue;
	
		fixed_t dest_x, dest_y, dest_z; // position to move player to

		if (!reconciled)
		{
			// record the player's current position, which hasn't yet
			// been saved to the history arrays
			player_history[i].backup_x = player->mo->x;
			player_history[i].backup_y = player->mo->y;
			player_history[i].backup_z = player->mo->z;
  
			size_t cur = (player_history[i].history_size - 1 - ticsago)
						  % Unlag::MAX_HISTORY_TICS;

			dest_x = player_history[i].history_x[cur];
			dest_y = player_history[i].history_y[cur];
			dest_z = player_history[i].history_z[cur];

			if (player_history[i].history_size < ticsago)
			{
				// make the player temporarily unshootable since this player
				// was not alive when the shot was fired.  Kind of a hack.
				player_history[i].backup_flags = player->mo->flags;
				player->mo->flags &= ~(MF_SHOOTABLE | MF_SOLID);
				player_history[i].changed_flags = true;
			}
		}
		else
		{   // we're moving the player back to proper position
			dest_x = player_history[i].backup_x;
			dest_y = player_history[i].backup_y;
			dest_z = player_history[i].backup_z;

			// restore a player's shootability if we removed it previously
			if (player_history[i].changed_flags)
			{
				player->mo->flags = player_history[i].backup_flags;
				player_history[i].changed_flags = false;
			}
		}   

		movePlayer(player, dest_x, dest_y, dest_z); 
	}
}



//
// Unlag::reconcileSectorPositions
//
// Moves the ceiling and floor of any sectors considered moveable
// to the positions they were 'ticsago' tics before.
//
// If 'reconciled' is true, restore the ceiling and floors to where they
// were prior to reconciliation.
//

void Unlag::reconcileSectorPositions(size_t ticsago)
{
	for (size_t i=0; i<sector_history.size(); i++)
	{
		sector_t *sector = sector_history[i].sector;

		fixed_t dest_ceilingheight, dest_floorheight;
		if (!reconciled)
		{
			// record the player's current position, which hasn't yet
			// been saved to the history arrays
			sector_history[i].backup_ceilingheight = sector->ceilingheight;
			sector_history[i].backup_floorheight = sector->floorheight;

			size_t cur = (sector_history[i].history_size - 1 - ticsago) 
						  % Unlag::MAX_HISTORY_TICS;
			dest_ceilingheight = sector_history[i].history_ceilingheight[cur];
			dest_floorheight = sector_history[i].history_floorheight[cur];
		}
		else	// restore to original positions 
		{
			dest_ceilingheight = sector_history[i].backup_ceilingheight;
			dest_floorheight = sector_history[i].backup_floorheight;
		}
		moveSector(sector, dest_ceilingheight, dest_floorheight);
	}	
}



//
// Unlag::reset
//
// Erases the position history for players and sectors.  Unlinks
// all player_t and sector_t objects from this Unlag object.
// Should be called at the begining of each level.

void Unlag::reset()
{
	player_history.clear();
	sector_history.clear();
	player_id_map.clear();
}


//
// Unlag::recordPlayerPositions
//
// Saves the current x, y, z position of all players.  History is
// reset every time a player dies, spectates, etc.
//

void Unlag::recordPlayerPositions()
{
	if (!Unlag::enabled())
		return;

	for (size_t i=0; i<player_history.size(); i++)
	{
		player_t *player = player_history[i].player;
	
		if (player->playerstate == PST_LIVE && 
			!player->spectator && player->mo)
		{
			size_t cur = player_history[i].history_size++ 
						 % Unlag::MAX_HISTORY_TICS;
			player_history[i].history_x[cur] = player->mo->x;
			player_history[i].history_y[cur] = player->mo->y;
			player_history[i].history_z[cur] = player->mo->z;
		} 
		else
		{   // reset history for all dead, spectating, etc players
			player_history[i].history_size = 0;
		}
	}
}


//
// Unlag;:recordSectorPositions()
//
// Saves the current ceiling and floor heights of all movable sectors
//

void Unlag::recordSectorPositions()
{
	if (!Unlag::enabled())
		return;

	for (size_t i=0; i<sector_history.size(); i++)
	{
		sector_t *sector = sector_history[i].sector;

		size_t cur = sector_history[i].history_size++ 
					 % Unlag::MAX_HISTORY_TICS;
		sector_history[i].history_ceilingheight[cur] = sector->ceilingheight;
		sector_history[i].history_floorheight[cur] = sector->floorheight;
	}
}


//
// Unlag::refreshRegisteredPlayers
//
// Updates the pointer to player_t in each player history record.
// The address of a player's player_t can change when a player is added to or
// removed from the global 'players' vector.  Also recreate the player_id_map.
// 

void Unlag::refreshRegisteredPlayers()
{
	player_id_map.clear();
	for (size_t i=0; i<player_history.size(); i++)
	{
		player_history[i].player = &idplayer(player_history[i].player_id);		
		player_id_map[player_history[i].player_id] = i;
	}
}

//
// Unlag::registerPlayer
//
// Adds a player to the player registry that maintains the history of
// players' positions
//

void Unlag::registerPlayer(byte player_id)
{
	if (!Unlag::enabled())
		return;

	player_history.push_back(PlayerHistoryRecord());
	player_history.back().player_id = player_id;
	player_history.back().history_size = 0;
	player_history.back().changed_flags = false;

	refreshRegisteredPlayers();
}


//
// Unlag::unregisterPlayer
//
// Removes a player from the player registry that mainins the history
// of players' positions.
//
// NOTE: unregisterPlayer should be called immediately following the call to
// players.erase() to update the pointers to the player objects.
//

void Unlag::unregisterPlayer(byte player_id)
{
	if (!Unlag::enabled())
		return;

	// look up the index corresponding to this player in player_history
	size_t player_index = player_id_map[player_id];
	player_history.erase(player_history.begin() + player_index);
	player_id_map.erase(player_id);

	refreshRegisteredPlayers();
}



//
// Unlag::registerSector
//
// Adds a sector to the sector registry that maintains the history
// of positions for all movable sectors.
//

void Unlag::registerSector(sector_t *sector)
{
	if (!Unlag::enabled())
		return;

	// Check if this sector already is in sector_history
	for (size_t i=0; i<sector_history.size(); i++)
	{
		// note: comparing the pointers to the sector_t objects
		if (sector_history[i].sector == sector)
			return;
	}

	sector_history.push_back(SectorHistoryRecord());
	size_t new_index = sector_history.size() - 1;
	sector_history[new_index].sector = sector;

	// sector.moveable is not set until the sector is triggered to move
	// so fill the history buffers with the sector's initial heights
	sector_history[new_index].history_size = Unlag::MAX_HISTORY_TICS;
	memset(sector_history[new_index].history_ceilingheight,
		   sector->ceilingheight,
		   Unlag::MAX_HISTORY_TICS);
	memset(sector_history[new_index].history_floorheight,
		   sector->floorheight,
		   Unlag::MAX_HISTORY_TICS);
}


//
// Unlag::unregisterSector
//
// Removes a sector from the sector registry that maintains the history
// of positions for all moveable sectors.
//

void Unlag::unregisterSector(sector_t *sector)
{
	if (!Unlag::enabled())
		return;

	for (size_t i=0; i<sector_history.size(); i++)
	{
		// note: comparing the pointers to the sector_t objects
		if (sector_history[i].sector == sector)  
		{
			sector_history.erase(sector_history.begin() + i);
			return;
		}
	}
}


//
// Unlag::reconcile
//
// Temporarily moves all sectors and players to the positions they were
// in when a lagging client (shooter) pressed the fire button on the client's
// end.  This allows a client to aim directly at opponents with hitscan
// weapons instead of leading them.
//

void Unlag::reconcile(byte shooter_id)
{
	if (!Unlag::enabled())
		return;	

	size_t player_index = player_id_map[shooter_id];

	// Check if client disables unlagging for their weapons
	if (!player_history[player_index].player->userinfo.unlag)
		return;

	size_t lag = player_history[player_index].current_lag;
	if (lag > 0 && lag <= Unlag::MAX_HISTORY_TICS) 
	{
		reconcileSectorPositions(lag);
		reconcilePlayerPositions(shooter_id, lag);
		reconciled = true;
	}
}


//
// Unlag::restore
//
// Moves all sectors and players to their original positions before reconcile
// was called.
//

void Unlag::restore(byte shooter_id)
{
	if (!Unlag::enabled())
		return;

	if (reconciled)
	{
		reconcileSectorPositions(0);
		reconcilePlayerPositions(shooter_id, 0);
		reconciled = false;	 // reset after restoring original positions
	}
}


//
// Unlag::setRoundtripDelay
//
// Sets the current_lag member variable for this particular player based on
// the delay for the server to ping the client and receive a reply.  Since
// lag can spike/have sudden changes, we only care about this value at the time
// a player fires a weapon.  Note: delay is measured in tics
//

void Unlag::setRoundtripDelay(byte player_id, size_t delay)
{
	size_t player_index = player_id_map[player_id];

	// set the lag to half the roundtrip delay since we have 
	// clientside prediction of other players' positions
	player_history[player_index].current_lag = delay / 2;
}


void P_SpawnBlood (fixed_t x, fixed_t y, fixed_t z, angle_t dir, int damage);

//
// Unlag::spawnUnreconciledBlood
//
// Moves blood splatters spawned at a target's reconciled position to
// the target's actual position.  More visually pleasing when the blood
// is spawned near the player taking damage rather than spawned out of thin
// air.
//

void Unlag::spawnUnreconciledBlood(byte shooter_id, byte target_id, 
				   	 			   fixed_t x, fixed_t y, fixed_t z,
								   angle_t dir, int damage)
{
	size_t target_index  = player_id_map[target_id];
	size_t shooter_index = player_id_map[shooter_id];

	// how many tics was the target reconciled?
	size_t ticsago = player_history[shooter_index].current_lag;
    size_t cur = (player_history[target_index].history_size - 1 - ticsago)
                  % Unlag::MAX_HISTORY_TICS;
	// calculate how far the target was moved during reconciliation
	fixed_t dist_x = 	player_history[target_index].backup_x - 
						player_history[target_index].history_x[cur];
	fixed_t dist_y = 	player_history[target_index].backup_y - 
						player_history[target_index].history_y[cur];
	fixed_t dist_z = 	player_history[target_index].backup_z - 
						player_history[target_index].history_z[cur];
	if (reconciled) // reconciled will only be true if sv_unlag is 1
	{
		// spawn blood but shift it based on how far the target was
		// moved during reconciliation
		P_SpawnBlood(x + dist_x, y + dist_y, z + dist_z, dir, damage);	
	}
	else
	{
		P_SpawnBlood(x, y, z, dir, damage);
	}
}


