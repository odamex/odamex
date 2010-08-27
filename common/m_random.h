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
//	Random number LUT.
//    
//-----------------------------------------------------------------------------


#ifndef __M_RANDOM__
#define __M_RANDOM__

#include "actor.h"

// The random number generator.
int M_Random();
int P_Random();

// As P_Random, but used by the play simulation, one per actor
int P_Random(AActor *actor);


// Optimization safe random difference functions -- Hyper_Eye 04/11/2010
int P_RandomDiff ();
int P_RandomDiff (AActor *actor);


// Fix randoms for demos.
void M_ClearRandom(void);

#endif
