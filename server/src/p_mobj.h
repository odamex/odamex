// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------


#ifndef __P_MOBJ_H__
#define __P_MOBJ_H__

void	G_PlayerReborn		(player_t &player);
void	CTF_RememberFlagPos (mapthing2_t *mthing);

extern	int			numspechit;
extern	BOOL		demonew;
extern	fixed_t		attackrange;
extern	line_t	  **spechit;

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

#endif
