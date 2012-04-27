// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __PMOBJ_H__
#define __PMOBJ_H__

#define REMOVECORPSESTIC TICRATE*80

//-----------------------------------------------------------------------------
//
// denis - superior NetIDHandler
//
// Very simple, very fast.
// Does not iterate when releasing a netid.
// Does not iterate when obtaining a netid unless all pooled ids are taken.
// (in which case does one allocation and does not iterate more than 
//  MEMPOOLSIZE times)
// Only downside is that it won't detect double-releasing, but shouldn't be 
// a problem.
//
// Thanks to [Dash|RD] for noticing the efficiency problem, hard work on other 
// versions of this class and giving me this great idea.
//
//-----------------------------------------------------------------------------

// [SL] 2012-04-04 
// Modified to use a std::queue, popping from the front of the queue to assign
// new netids and pushing newly freed netids on the back of the queue.  This is
// to avoid reassigning a recently freed netid to a different actor.  Otherwise
// clients can get confused when packets are dropped.

#include <queue>

#define MAX_NETID 0xFFFF

class NetIDHandler
{
	private:

	std::queue<unsigned short> free_ids;

	public:

	NetIDHandler()
	{
		if (clientside)
			return;

		for (int i = 1; i <= MAX_NETID; i++)
			free_ids.push(i);
	}

	~NetIDHandler()
	{
	}

	int ObtainNetID()
	{
		if (clientside)		// only servers can assign netids
			return 0;

		if (free_ids.empty())
			I_Error("Exceeded maximum number of netids");

		int netid = free_ids.front();
		free_ids.pop();
	
		return netid;
	}

	void ReleaseNetID(int NetID)
	{
		if (clientside)
			return;

		if (!NetID || NetID < 1 || NetID > MAX_NETID)
			I_Error("Released a non-existant netid %d", NetID);

		free_ids.push(NetID);
	}
};

extern NetIDHandler ServerNetID;

void P_ClearAllNetIds();
AActor* P_FindThingById(size_t id);
void P_SetThingId(AActor *mo, size_t newnetid);
void P_ClearId(size_t id);

bool P_SetMobjState(AActor *mobj, statenum_t state);
void P_XYMovement(AActor *mo);
void P_ZMovement(AActor *mo);
void PlayerLandedOnThing(AActor *mo, AActor *onmobj); // [CG] Used to be 'static'
void P_NightmareRespawn(AActor *mo);
void P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z, angle_t dir, int updown);
void P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, angle_t dir, int damage);
bool P_CheckMissileSpawn(AActor* th);
AActor* P_SpawnMissile(AActor *source, AActor *dest, mobjtype_t type);
void P_SpawnPlayerMissile(AActor *source, mobjtype_t type);
bool P_VisibleToPlayers(AActor *mo);

// [ML] From EE
int P_ThingInfoHeight(mobjinfo_t *mi);

#endif

