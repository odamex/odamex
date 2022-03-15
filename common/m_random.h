// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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


#pragma once

#include "actor.h"


typedef enum {
	pr_misc,					// 0
	pr_all_in_one,				// 1
	pr_dmspawn,					// 2
	pr_checkmissilerange,		// 3
	pr_trywalk,					// 4
	pr_newchasedir,				// 5
	pr_look,					// 6
	pr_chase,					// 7
	pr_facetarget,				// 8
	pr_posattack,				// 9
	pr_sposattack,				// 10
	pr_cposattack,				// 11
	pr_cposrefire,				// 12
	pr_spidrefire,				// 13
	pr_troopattack,				// 14
	pr_sargattack,				// 15
	pr_headattack,				// 16
	pr_bruisattack,				// 17
	pr_tracer,					// 18
	pr_skelfist,				// 19
	pr_scream,					// 20
	pr_brainscream,				// 21
	pr_brainexplode,			// 22
	pr_spawnfly,				// 23
	pr_killmobj,				// 24
	pr_damagemobj,				// 25
	pr_checkthing,				// 26
	pr_changesector,			// 27
	pr_explodemissile,			// 28
	pr_mobjthinker,				// 29
	pr_spawnmobj,				// 30
	pr_spawnmapthing,			// 31
	pr_spawnpuff,				// 32
	pr_spawnblood,				// 33
	pr_checkmissilespawn,		// 34
	pr_spawnmissile,			// 35
	pr_punch,					// 36
	pr_saw,						// 37
	pr_fireplasma,				// 38
	pr_gunshot,					// 39
	pr_fireshotgun2,			// 40
	pr_bfgspray,				// 41
	pr_checksight,				// 42
	pr_playerinspecialsector,	// 43
	pr_fireflicker,				// 44
	pr_lightflash,				// 45
	pr_spawnlightflash,			// 46
	pr_spawnstrobeflash,		// 47
	pr_doplat,					// 48
	pr_throwgib,				// 49
	pr_vel4dmg,					// 50
	pr_gengib,					// 51
	pr_acs,						// 52
	pr_animatepictures,			// 53
	pr_obituary,				// 54
	pr_quake,					// 55
	pr_playerscream,			// 56
	pr_playerpain,				// 57
	pr_bounce,					// 58
	pr_opendoor,				// 59
	pr_botmove,					// 60
	pr_botdofire,				// 61
	pr_botspawn,				// 62
	pr_botrespawn,				// 63
	pr_bottrywalk,				// 64
	pr_botnewchasedir,			// 65
	pr_botspawnmobj,			// 66
	pr_botopendoor,				// 67
	pr_botchecksight,			// 68
	// Start new entries -- add new entries below

	// End of new entries
	NUMPRCLASS							// MUST be last item in list
} pr_class_t;


// The random number generator.
int M_Random();
int P_Random();

// [AM] RNG's that use xoshiro128** and not Doom's random table.

uint32_t M_RandomInt(const uint32_t range);
uint32_t P_RandomInt(const uint32_t range);

float M_RandomFloat();
float P_RandomFloat();

// As P_Random, but used by the play simulation, one per actor
int P_Random(AActor *actor);


// Optimization safe random difference functions -- Hyper_Eye 04/11/2010
int P_RandomDiff ();
int P_RandomDiff (AActor *actor);


// Fix randoms for demos.
void M_ClearRandom(void);

/**
 * @brief Get a weighted random number from a vector.
 * 
 * @detail Upside of this algorithm is that it requires no preperation and
 *         only requires a single complete iteration of the vector.
 * 
 * @see https://softwareengineering.stackexchange.com/a/150642
 */
template <typename T>
const T& P_RandomFloatWeighted(const std::vector<T>& data, float (*func)(const T&))
{
	// this stores sum of weights of all elements before current
	float totalWeight = 0;

	// currently selected element
	const T* selected = NULL;
	for (typename std::vector<T>::const_iterator it = data.begin(); it != data.end();
	     ++it)
	{
		// weight of current element
		const T& ele = *it;
		const float weight = func(ele);

		// random value
		const float r = P_RandomFloat() * (totalWeight + weight);

		// probability of this is weight/(totalWeight+weight)
		if (r >= totalWeight)
		{
			// it is the probability of discarding last selected element
			// and selecting current one instead
			selected = &ele;
		}

		// increase weight sum
		totalWeight += weight;
	}

	// when iterations end, selected is some element of sequence.
	return *selected;
}

// Used for MBF21 mostly.
int P_RandomHitscanAngle(fixed_t spread);
int P_RandomHitscanSlope(fixed_t spread);
