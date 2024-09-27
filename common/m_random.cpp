// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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


#include "odamex.h"

#include <time.h>

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
	if(!actor || demoplayback || step_mode)
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

//
// A PRNG commonly known as "Jenkins Small Fast" by Bob Jenkins.
// Released into the public domain.
// http://burtleburtle.net/bob/rand/talksmall.html
// 

struct jsf32ctx_t
{
	uint32_t a;
	uint32_t b;
	uint32_t c;
	uint32_t d;
};

static jsf32ctx_t g_rngState;
static jsf32ctx_t g_prngState;

#define ROT32(x, k) (((x) << (k)) | ((x) >> (32 - (k))))

/**
 * @brief Run one iteration of JSF32.
 */
static uint32_t JSF32(jsf32ctx_t* x)
{
	const uint32_t e = x->a - ROT32(x->b, 27);
	x->a = x->b ^ ROT32(x->c, 17);
	x->b = x->c + x->d;
	x->c = x->d + e;
	x->d = e + x->a;
	return x->d;
}

/**
 * @brief Seed the JSF state.
 */
static void JSF32Seed(jsf32ctx_t* x, const uint32_t seed)
{
	x->a = 0xf1ea5eed, x->b = x->c = x->d = seed;
	for (uint32_t i = 0; i < 20; i++)
	{
		(void)JSF32(x);
	}
}

/**
 * @brief Return a random integer that is not tied to game state.
 *
 * @param range One past the maximum number you want to roll.
 * @return A random integer in the given range.
 */
uint32_t M_RandomInt(const uint32_t range)
{
	const uint32_t t = (0U - range) % range;
	for (;;)
	{
		const uint32_t r = JSF32(&::g_rngState);
		if (r >= t)
		{
			return r % range;
		}
	}
}

/**
 * @brief Return a random integer that is tied to game state.
 *
 * @param range One past the maximum number you want to roll.
 * @return A random integer in the given range.
 */
uint32_t P_RandomInt(const uint32_t range)
{
	const uint32_t t = (0U - range) % range;
	for (;;)
	{
		const uint32_t r = JSF32(&::g_prngState);
		if (r >= t)
		{
			return r % range;
		}
	}
}

/**
 * @brief Return a random floating point number that is not tied to game state.
 * 
 * @return A random float in the half-open range of [0.0, 1.0).
 */
float M_RandomFloat()
{
	const uint32_t n = JSF32(&::g_rngState);
	return static_cast<float>(n) / (static_cast<float>(0xFFFFFFFFU) + 1.0f);
}

/**
 * @brief Return a random floating point number that is tied to game state.
 * 
 * @return A random float in the half-open range of [0.0, 1.0).
 */
float P_RandomFloat()
{
	const uint32_t n = JSF32(&::g_prngState);
	return static_cast<float>(n) / (static_cast<float>(0xFFFFFFFFU) + 1.0f);
}

// Initialize all the seeds
//
// This initialization method is critical to maintaining demo sync.
// Each seed is initialized according to its class, so if new classes
// are added they must be added to end of pr_class_t list. killough
//

void M_ClearRandom()
{
	static bool firsttime = true;

	::prndindex = 0;

	// Odamex's release date.
	JSF32Seed(&::g_prngState, 20071801U);

	// Only init M_Random values once.
	if (firsttime)
	{
		firsttime = false;
		::rndindex = ::prndindex;

		// [AM] Make non-playsim random unpredictable.
		uint32_t seed = static_cast<uint32_t>(time(NULL));
		JSF32Seed(&::g_rngState, seed);
	}
}

void P_SerializeRNGState(FArchive& arc)
{
	if (arc.IsStoring())
	{
		arc << ::prndindex << ::g_prngState.a << ::g_prngState.b << ::g_prngState.c
		    << ::g_prngState.d;
	}
	else
	{
		arc >> ::prndindex >> ::g_prngState.a >> ::g_prngState.b >> ::g_prngState.c >>
		    ::g_prngState.d;
	}
}

//
// P_RandomHitscanAngle
// Outputs a random angle between (-spread, spread), as an int ('cause it can be
// negative).
//   spread: Maximum angle (degrees, in fixed point -- not BAM!)
//
int P_RandomHitscanAngle(fixed_t spread)
{
	int t;
	uint64_t spread_bam;

	// FixedToAngle doesn't work for negative numbers,
	// so for convenience take just the absolute value.
	spread_bam = (spread < 0 ? FixedToAngle(-spread) : FixedToAngle(spread));
	t = P_Random();
	return (int)((spread_bam * (t - P_Random())) / 255);
}

//
// P_RandomHitscanSlope
// Outputs a random angle between (-spread, spread), converted to values used for slope
//   spread: Maximum vertical angle (degrees, in fixed point -- not BAM!)
//
int P_RandomHitscanSlope(fixed_t spread)
{
	int angle;

	angle = P_RandomHitscanAngle(spread);

	// clamp it, yo
	if (angle > ANG90)
		return finetangent[0];
	else if (-angle > ANG90)
		return finetangent[FINEANGLES / 2 - 1];
	else
		return finetangent[(ANG90 - angle) >> ANGLETOFINESHIFT];
}

VERSION_CONTROL (m_random_cpp, "$Id$")

