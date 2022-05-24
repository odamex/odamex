// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
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


#include "odamex.h"

#include "m_vectors.h"
#include "p_unlag.h"
#include "p_local.h"

#ifdef _UNLAG_DEBUG_
#include <list>
void SV_SpawnMobj(AActor *mo);
void SV_SendDestroyActor(AActor *mo);
#endif	// _UNLAG_DEBUG_

EXTERN_CVAR(sv_maxunlagtime)

Unlag::SectorHistoryRecord::SectorHistoryRecord()
	:	sector(NULL), history_size(0),
		history_ceilingheight(), history_floorheight(),
		backup_ceilingheight(0), backup_floorheight(0)
{
}

Unlag::SectorHistoryRecord::SectorHistoryRecord(sector_t *sec)
	: 	sector(sec), history_size(Unlag::MAX_HISTORY_TICS),
		history_ceilingheight(), history_floorheight(),
		backup_ceilingheight(0), backup_floorheight(0)
{
	if (!sector)
		return;

	fixed_t ceilingheight = P_CeilingHeight(sector);
	fixed_t floorheight = P_FloorHeight(sector);

	for (size_t i = 0; i < history_size; i++)
	{
		history_ceilingheight[i] = ceilingheight;
		history_floorheight[i] = floorheight;
	}

	backup_ceilingheight = ceilingheight;
	backup_floorheight = floorheight;
}

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
// Denotes whether a multiplayer game is running on the server.
//
 
bool Unlag::enabled()
{
	return (serverside && multiplayer && !demoplayback);
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

	player->mo->SetOrigin(x, y, z);
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
	P_SetCeilingHeight(sector, ceilingheight);
	P_SetFloorHeight(sector, floorheight);
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
  
  			size_t cur = (gametic - ticsago) % Unlag::MAX_HISTORY_TICS;

			dest_x = player_history[i].history_x[cur];
			dest_y = player_history[i].history_y[cur];
			dest_z = player_history[i].history_z[cur];

			player_history[i].offset_x = player_history[i].backup_x - dest_x;
			player_history[i].offset_y = player_history[i].backup_y - dest_y;
			player_history[i].offset_z = player_history[i].backup_z - dest_z;

			if (player_history[i].history_size < ticsago)
			{
				// make the player temporarily unshootable since this player
				// was not alive when the shot was fired.  Kind of a hack.
				player_history[i].backup_flags = player->mo->flags;
				player->mo->flags &= ~(MF_SHOOTABLE | MF_SOLID);
				player_history[i].changed_flags = true;
			}

			#ifdef _UNLAG_DEBUG_
			// spawn a marker sprite at the reconciled position for debugging
	        AActor *mo = new AActor(dest_x, dest_y, dest_z, MT_KEEN);
			mo->flags &= ~(MF_SHOOTABLE | MF_SOLID);
			mo->health = -187;
			SV_SpawnMobj(mo);
			#endif // _UNLAG_DEBUG_
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
			sector_history[i].backup_ceilingheight = P_CeilingHeight(sector);
			sector_history[i].backup_floorheight = P_FloorHeight(sector);

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
			player_history[i].history_size++;
			
			size_t cur = gametic % Unlag::MAX_HISTORY_TICS;
			player_history[i].history_x[cur] = player->mo->x;
			player_history[i].history_y[cur] = player->mo->y;
			player_history[i].history_z[cur] = player->mo->z;
			
			#ifdef _UNLAG_DEBUG_
			DPrintf("Unlag (%03d): recording player %d position (%d, %d)\n",
					gametic & 0xFF, player->id, 
					player->mo->x >> FRACBITS,
					player->mo->y >> FRACBITS,
					player->mo->z >> FRACBITS);
			#endif	// _UNLAG_DEBUG_			
		} 
		else
		{   // reset history for dead, spectating, etc players
			player_history[i].history_size = 0;
		}
	}
}


//
// Unlag::recordSectorPositions()
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
		sector_history[i].history_ceilingheight[cur] = P_CeilingHeight(sector);
		sector_history[i].history_floorheight[cur] = P_FloorHeight(sector);
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
		byte id = player_history[i].player_id;

		player_history[i].player = &idplayer(id);		
		player_id_map[id] = i;
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

	// don't register a bogus player id
	if (!validplayer(idplayer(player_id)))
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

	size_t history_index = player_id_map[player_id];
	if (history_index >= player_history.size())
		return;

	player_history.erase(player_history.begin() + history_index);
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

	sector_history.push_back(SectorHistoryRecord(sector));
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

	size_t lag = player_history[player_index].current_lag;
	
	#ifdef _UNLAG_DEBUG_
	DPrintf("Unlag (%03d): moving players to their positions at gametic %d (%d tics ago)\n",
			gametic & 0xFF, (gametic - lag) & 0xFF, lag);

	// remove any other debugging player markers
	AActor *mo;
	std::list<AActor*> to_destroy;
	TThinkerIterator<AActor> iterator;
	while ( (mo = iterator.Next() ) )
	{
		if (mo->type == MT_KEEN && mo->health == -187)
			to_destroy.push_back(mo);
	}

	while (!to_destroy.empty())
	{
		mo = to_destroy.front();
		to_destroy.pop_front();
		if (mo)
			mo->Destroy();
	}
	
	if (lag > Unlag::MAX_HISTORY_TICS)
		DPrintf("Unlag (%03d): player %d has too great of lag (%d tics)\n",
				gametic & 0xFF, shooter_id, lag);
	#endif	// _UNLAG_DEBUG_

	if (lag > 0 && lag < Unlag::MAX_HISTORY_TICS) 
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
	
	#ifdef _UNLAG_DEBUG_
	debugReconciliation(shooter_id);
	#endif	// _UNLAG_DEBUG_
}


//
// Unlag::setRoundtripDelay
//
// Sets the current_lag member variable for this particular player based on
// the time it takes a message from the server to reach the client and the
// reply to be received.  Since lag can spike/have sudden changes, we only
// care about this value at the time a player fires a weapon.  The parameter
// svgametic is the server gametic send when the server sends a positional
// update, which is returned to the server when the client sends a ticcmd
// that has the attack button pressed.

void Unlag::setRoundtripDelay(byte player_id, byte svgametic)
{
	if (!Unlag::enabled())
		return;

	size_t maxdelay = TICRATE * sv_maxunlagtime;
	if (maxdelay > Unlag::MAX_HISTORY_TICS)
		maxdelay = Unlag::MAX_HISTORY_TICS;

	size_t delay = ((gametic & 0xFF) + 256 - svgametic) & 0xFF;
	
	size_t player_index = player_id_map[player_id];
	player_history[player_index].current_lag = MIN(delay, maxdelay);
	
	#ifdef _UNLAG_DEBUG_
	DPrintf("Unlag (%03d): received gametic %d from player %d, lag = %d\n",
					gametic & 0xFF, svgametic, player_id, delay);
	#endif	// _UNLAG_DEBUG
}


//
// Unlag::getReconciliationOffset
//
// Changes the x, y, z parameters to reflect how much a player was moved
// during reconciliation.

void Unlag::getReconciliationOffset(	byte target_id,
			   	 			   			fixed_t &x, fixed_t &y, fixed_t &z)
{  
	x = y = z = 0;

	if (!reconciled)	// reconciled will only be true if sv_unlag is 1
		return;

	size_t target_index  = player_id_map[target_id];

	// calculate how far the target was moved during reconciliation
	x = player_history[target_index].offset_x;
	y = player_history[target_index].offset_y;
	z = player_history[target_index].offset_z;
}


//
// Unlag::getCurrentPlayerPosition
//
// Changes the x, y, z parameters to the position of a player that
// was saved prior to reconciliation.
//
void Unlag::getCurrentPlayerPosition(	byte player_id,
										fixed_t &x, fixed_t &y, fixed_t &z)
{
	x = y = z = 0;

	size_t cur = player_id_map[player_id];
	player_t* player = player_history[cur].player;

	if (!player || !player->mo || player->spectator)
		return;

	if (Unlag::enabled() && reconciled)
	{
		x = player_history[cur].backup_x;
		y = player_history[cur].backup_y;
		z = player_history[cur].backup_z;
	}
	else
	{
		x = player->mo->x;
		y = player->mo->y;
		z = player->mo->z;
	}
}


//
// Unlag::debugReconciliation
//
// Attempts to determine which tic would have been ideal to use for reconciling
// a target player's position.
void Unlag::debugReconciliation(byte shooter_id)
{
	player_t *shooter = &(idplayer(shooter_id));
	
	for (size_t i = 0; i < player_history.size(); i++)
	{
		if (player_history[i].player->id == shooter_id)
			continue;	
	
		for (size_t n = 0; n < MAX_HISTORY_TICS; n++)
		{
			if (n > player_history[i].history_size)
				break;
				
			size_t cur = (gametic - n) % Unlag::MAX_HISTORY_TICS;
		
			fixed_t x = player_history[i].history_x[cur];
			fixed_t y = player_history[i].history_y[cur];
			
			angle_t angle = P_PointToAngle(shooter->mo->x,	shooter->mo->y, x, y);
			angle_t deltaangle = 	angle - shooter->mo->angle < ANG180 ?
									angle - shooter->mo->angle :
									shooter->mo->angle - angle;

			if (deltaangle < 3 * FRACUNIT)
			{
				DPrintf("Unlag (%03d): would have hit player %d at gametic %d (%" "zu" " tics ago)\n",
						gametic & 0xFF, player_history[i].player->id, (gametic - n) & 0xFF, n);
			}
		}
	}
}
