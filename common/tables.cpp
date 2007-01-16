// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2007 by The Odamex Team.
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
//		Lookup tables.
//		Do not try to look them up :-).
//		In the order of appearance: 
//
//		int finetangent[4096]	- Tangens LUT.
//		 Should work with BAM fairly well (12 of 16bit,
//		effectively, by shifting).
//
//		int finesine[10240] 			- Sine lookup.
//		 Guess what, serves as cosine, too.
//		 Remarkable thing is, how to use BAMs with this? 
//
//		int tantoangle[2049]	- ArcTan LUT,
//		  maps tan(angle) to angle fast. Gotta search.
//		
//	  
//-----------------------------------------------------------------------------


#include "tables.h"

fixed_t finetangent[FINEANGLES/2];
fixed_t finesine[5*FINEANGLES/4];
angle_t tantoangle[SLOPERANGE+1];


VERSION_CONTROL (tables_cpp, "$Id:$")

