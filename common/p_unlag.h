// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: p_unlag.h $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//-----------------------------------------------------------------------------


#ifndef __PUNLAG_H__
#define __PUNLAG_H__

#include <vector>
#include <map>
#include "doomtype.h"
#include "m_fixed.h"
#include "actor.h"
#include "d_player.h"
#include "r_defs.h"

class Unlag
{
public:
	~Unlag();
	static Unlag& getInstance();  // returns the instantiated Unlag object
	void reset();	  // called when starting a level
	void reconcile(byte player_id);
	void restore(byte player_id);
	void recordPlayerPositions();
	void recordSectorPositions();
	void registerPlayer(byte player_id);
	void unregisterPlayer(byte player_id);
	void registerSector(sector_t *sector);
	void unregisterSector(sector_t *sector);
	void setRoundtripDelay(byte player_id, byte svgametic);
	void getReconciliationOffset(	byte shooter_id, byte target_id,
									fixed_t &x, fixed_t &y, fixed_t &z);
	static bool enabled();
private:
	// keep as a power of 2 so the compiler can optimize: n % MAX_HISTORY_TICS
	// into: n & (MAX_HISTORY_TICS - 1)
	static const size_t MAX_HISTORY_TICS = 32;
		
	typedef struct {
		byte		player_id;

		// cached pointer to players[n].  Note: this needs to be updated
		// EVERYTIME a player connects or disconnects.
		player_t*	player;
	
		fixed_t		history_x[Unlag::MAX_HISTORY_TICS];
		fixed_t		history_y[Unlag::MAX_HISTORY_TICS];
		fixed_t		history_z[Unlag::MAX_HISTORY_TICS];
		size_t		history_size;
		
		// current position. restore this position after reconciliation.
		fixed_t		backup_x;
		fixed_t		backup_y;
		fixed_t		backup_z;
		
		// did we change player's MF_SHOOTABLE flag during reconciliation?
		bool		changed_flags;
		int			backup_flags; 

		size_t		current_lag;
	} PlayerHistoryRecord;
   
	class SectorHistoryRecord
	{
	public:
		SectorHistoryRecord();
		SectorHistoryRecord(sector_t *sec);

		sector_t*	sector;
		size_t		history_size;
		fixed_t		history_ceilingheight[Unlag::MAX_HISTORY_TICS];
		fixed_t		history_floorheight[Unlag::MAX_HISTORY_TICS];

		// current position. restore this position after reconciliation.
		fixed_t		backup_ceilingheight;
		fixed_t		backup_floorheight;
	};

	std::vector<PlayerHistoryRecord> player_history;
	std::vector<SectorHistoryRecord> sector_history;
	bool reconciled;	
    
    // stores an index into the player_history vector, keyed by player_id
	std::map<byte, size_t> player_id_map;

	Unlag() : reconciled(false) {}  // private contsructor (part of Singleton)
	Unlag(const Unlag &rhs);		// private copy constructor
	Unlag& operator=(const Unlag &rhs);	//private assignment operator

	void movePlayer(player_t *player, fixed_t x, fixed_t y, fixed_t z);
	void moveSector(sector_t *sector, 
					fixed_t ceilingheight, fixed_t floorheight);
	void reconcilePlayerPositions(byte shooter_id, size_t ticsago);
	void reconcileSectorPositions(size_t ticsago);
	void refreshRegisteredPlayers();

	void debugReconciliation(byte shooter_id);
};


#endif

