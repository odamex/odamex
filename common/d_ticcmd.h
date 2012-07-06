// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __D_TICCMD_H__
#define __D_TICCMD_H__

#include "doomtype.h"
#include "d_protocol.h"

// The data sampled per tick (single player)
// and transmitted to other peers (multiplayer).
// Mainly movements/button commands per game tick,
// plus a checksum for internal state consistency.
struct ticcmd_t
{
	usercmd_t	ucmd;
	int			tic;	// the client's tic when this cmd was sent
/*
	char		forwardmove;	// *2048 for move
	char		sidemove;		// *2048 for move
	short		angleturn;		// <<16 for angle delta
*/
//	SWORD		consistancy;	// checks for net game
};


inline FArchive &operator<< (FArchive &arc, ticcmd_t &cmd)
{
	return arc << cmd.ucmd;
}

inline FArchive &operator>> (FArchive &arc, ticcmd_t &cmd)
{
	return arc >> cmd.ucmd;
}


#endif	// __D_TICCMD_H__


