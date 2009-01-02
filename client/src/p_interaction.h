// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	Handling interactions (i.e., collisions).
//
//-----------------------------------------------------------------------------


#ifndef __P_INTERACTION_H__
#define __P_INTERACTION_H__

// ---------------------------------------------------------------------------------------------------------
// External Function Prototypes

bool CheckCheatmode (void);
void A_PlayerScream(AActor*);

// ---------------------------------------------------------------------------------------------------------
//	Globals

int		maxammo[NUMAMMO]	= {200, 50, 300, 50};
int		clipammo[NUMAMMO]	= {10, 4, 20, 1};

int		MeansOfDeath;


#define BONUSADD		6


#endif


