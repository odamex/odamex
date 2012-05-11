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
//      Random number LUT.
//
// 1/19/98 killough: Rewrote random number generator for better randomness,
// while at the same time maintaining demo sync and backward compatibility.
//
// 2/16/98 killough: Made each RNG local to each control-equivalent block,
// to reduce the chances of demo sync problems.
//
//-----------------------------------------------------------------------------


#include <time.h>

#include "doomstat.h"
#include "m_random.h"
#include "farchive.h"

//
// M_Random
// Returns a 0-255 number
//

// [Dash|RD] -- Make random number generation use the original random number chart.
// 		The way we pick numbers from this chart is NOT the way the original
//		game does it, but that's hardly relevant. We aren't trying to play back
//		demos afterall. More work may need to be done on this.
//
//		Why did I do it? With this chart the game appears to pick consistently higher
//		numbers. This should address the reports that the SSG (and various other weapons)
//		do less damage in ZDaemon.
//
// SSG damage statistics, 20 shot survey.
// PrBoom     -- Max: 245    Min: 165    Median: 210   Average: 206.5  Total: 4,130
// ZDaemon    -- Max: 215    Min: 155    Median: 195   Average: 194.5  Total: 3,890
// This Code  -- Max: 245    Min: 155    Median: 205   Average: 202.75 Total: 4,055
//
// Obviously you want to give a bit of room for randomness, but has is definitely been my experience that
// this code statistically gives off higher `random' numbers. Other shotgun guru's have also claimed that
// this code fixes their issues with weapon damage.
//

static const unsigned char rndtable[256] = {
    0,   8, 109, 220, 222, 241, 149, 107,  75, 248, 254, 140,  16,  66 ,
    74,  21, 211,  47,  80, 242, 154,  27, 205, 128, 161,  89,  77,  36 ,
    95, 110,  85,  48, 212, 140, 211, 249,  22,  79, 200,  50,  28, 188 ,
    52, 140, 202, 120,  68, 145,  62,  70, 184, 190,  91, 197, 152, 224 ,
    149, 104,  25, 178, 252, 182, 202, 182, 141, 197,   4,  81, 181, 242 ,
    145,  42,  39, 227, 156, 198, 225, 193, 219,  93, 122, 175, 249,   0 ,
    175, 143,  70, 239,  46, 246, 163,  53, 163, 109, 168, 135,   2, 235 ,
    25,  92,  20, 145, 138,  77,  69, 166,  78, 176, 173, 212, 166, 113 ,
    94, 161,  41,  50, 239,  49, 111, 164,  70,  60,   2,  37, 171,  75 ,
    136, 156,  11,  56,  42, 146, 138, 229,  73, 146,  77,  61,  98, 196 ,
    135, 106,  63, 197, 195,  86,  96, 203, 113, 101, 170, 247, 181, 113 ,
    80, 250, 108,   7, 255, 237, 129, 226,  79, 107, 112, 166, 103, 241 ,
    24, 223, 239, 120, 198,  58,  60,  82, 128,   3, 184,  66, 143, 224 ,
    145, 224,  81, 206, 163,  45,  63,  90, 168, 114,  59,  33, 159,  95 ,
    28, 139, 123,  98, 125, 196,  15,  70, 194, 253,  54,  14, 109, 226 ,
    71,  17, 161,  93, 186,  87, 244, 138,  20,  52, 123, 251,  26,  36 ,
    17,  46,  52, 231, 232,  76,  31, 221,  84,  37, 216, 165, 212, 106 ,
    197, 242,  98,  43,  39, 175, 254, 145, 190,  84, 118, 222, 187, 136 ,
    120, 163, 236, 249
};

unsigned char rndindex, prndindex;

extern bool step_mode;

//
// denis - Every actor has a random number generator so they can be sync'ed
// across the network without knowledge of other actors
//
int P_Random (AActor *actor)
{
	if(!actor || demoplayback || demorecording || step_mode)
		return P_Random ();

	return (rndtable[++actor->rndindex]);
}

int P_RandomDiff (AActor *actor)
{
	int a = P_Random (actor);
	int b = P_Random (actor);

	return a - b;
}

int P_Random ()
{
	return (rndtable[++prndindex]);
}

int P_RandomDiff ()
{
	int a = P_Random ();
	int b = P_Random ();

	return a - b;
}

int M_Random()
{
	return (rndtable[++rndindex]);
}

// Initialize all the seeds
//
// This initialization method is critical to maintaining demo sync.
// Each seed is initialized according to its class, so if new classes
// are added they must be added to end of pr_class_t list. killough
//

void M_ClearRandom (void)
{
	rndindex = prndindex = 0;
}

void P_SerializeRNGState (FArchive &arc)
{
	if (arc.IsStoring ())
	{
		arc << prndindex;
	}
	else
	{
		arc >> prndindex;
	}
}


VERSION_CONTROL (m_random_cpp, "$Id$")

