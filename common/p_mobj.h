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

#define MAX_NETID 0xFFFF

class NetIDHandler
{
	private:

	int *allocation;

	size_t NumAllocated;
	size_t NumUsed;

	const size_t ChunkSize;

	public:

	NetIDHandler(size_t chunk_size = 256)
		: allocation(0), NumAllocated(0), NumUsed(0), ChunkSize(chunk_size)
	{}

	~NetIDHandler()
	{
		M_Free(allocation);
	}

	int ObtainNetID()
	{
		if(NumUsed >= NumAllocated)
		{
			if(NumAllocated >= MAX_NETID - 1)
				I_Error("Exceeded maximum number of netids");

			int OldAllocated = NumAllocated;
			NumAllocated += ChunkSize;

			if(NumAllocated >= MAX_NETID - 1)
				NumAllocated = MAX_NETID - 1;

			allocation = (int *)Realloc(allocation, NumAllocated*sizeof(int));

			for(size_t i = OldAllocated; i < NumAllocated; i++)
				allocation[i] = i + 1;
		}

		return allocation[NumUsed++];
	}

	void ReleaseNetID(int NetID)
	{
		if(!NumUsed || !NetID)
			I_Error("Released a non-existant netid %d", NetID);

		allocation[--NumUsed] = NetID;
	}
};

extern NetIDHandler ServerNetID;

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

#endif

