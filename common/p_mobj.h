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
//   Map Objects, MObj, definition and handling.
//
//-----------------------------------------------------------------------------

#pragma once

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

// [AM] 2021-03-05
// Changed NetID system to not "give back" NetID's.  This was causing issues
// when resetting the level too many times.

#include "i_system.h"

#define MAX_NETID 0xFFFFFFFF

class NetIDHandler
{
  private:
	uint32_t m_nextID;

  public:
	NetIDHandler() : m_nextID(1)
	{
	}

	~NetIDHandler()
	{
	}

	uint32_t peekNetID()
	{
		return m_nextID;
	}

	/**
	 * @brief Obtain a netID for an AActor.
	 */
	uint32_t obtainNetID()
	{
		if (m_nextID == MAX_NETID)
		{
			I_Error("Exceeded maximum number of netids (%u)", MAX_NETID);
		}

		m_nextID += 1;
		return m_nextID - 1;
	}

	/**
	 * @brief Reset the netID back to 1.
	 *
	 * @detail Probably not a good idea to call this method in the middle
	 *         of a level, reusing netID's doesn't tend to go very well.
	 */
	void resetNetIDs()
	{
		m_nextID = 1;
	}
};

// [XA] Clamped angle->slope, for convenience
inline static fixed_t AngleToSlope(int a)
{
	if (a > ANG90)
		return finetangent[0];
	else if (-a > ANG90)
		return finetangent[FINEANGLES / 2 - 1];
	else
		return finetangent[(ANG90 - a) >> ANGLETOFINESHIFT];
}

// [XA] Ditto, using fixed-point-degrees input
inline static fixed_t DegToSlope(fixed_t a)
{
	if (a >= 0)
		return AngleToSlope(FixedToAngle(a));
	else
		return AngleToSlope(-(int)FixedToAngle(-a));
}

extern NetIDHandler ServerNetID;

// All oflag mods that are sent to horde bosses.
const uint32_t hordeBossModMask = MFO_INFIGHTINVUL | MFO_UNFLINCHING | MFO_ARMOR |
                                  MFO_QUICK | MFO_NORAISE | MFO_FULLBRIGHT;

void P_ClearAllNetIds();
AActor* P_FindThingById(uint32_t id);
void P_SetThingId(AActor* mo, uint32_t newnetid);
void P_ClearId(uint32_t id);

bool P_SetMobjState(AActor *mobj, statenum_t state, bool cl_update);
void P_XYMovement(AActor *mo);
void P_ZMovement(AActor *mo);
void PlayerLandedOnThing(AActor *mo, AActor *onmobj); // [CG] Used to be 'static'
void P_NightmareRespawn(AActor *mo);
void P_SpawnPuff(fixed_t x, fixed_t y, fixed_t z);
void P_SpawnTracerPuff(fixed_t x, fixed_t y, fixed_t z);
void P_SpawnBlood(fixed_t x, fixed_t y, fixed_t z, angle_t dir, int damage);
bool P_CheckMissileSpawn(AActor* th);
AActor* P_SpawnMissile(AActor *source, AActor *dest, mobjtype_t type);
void P_SpawnPlayerMissile(AActor *source, mobjtype_t type);
size_t P_GetMapThingPlayerNumber(mapthing2_t* mthing);
bool P_VisibleToPlayers(AActor *mo);
void P_SetMobjBaseline(AActor& mo);
uint32_t P_GetMobjBaselineFlags(AActor& mo);

// [ML] From EE
int P_ThingInfoHeight(mobjinfo_t *mi);
bool P_HealCorpse(AActor* actor, int radius, int healstate, int healsound);
void SpawnFlag(mapthing2_t* mthing, team_t flag);

// From MBF
bool P_SeekerMissile(AActor* actor, AActor* seekTarget, angle_t thresh, angle_t turnMax, bool seekcenter);
