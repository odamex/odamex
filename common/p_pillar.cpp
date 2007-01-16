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
//	[RH] p_pillar.c: New file to handle pillars
//
//-----------------------------------------------------------------------------


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_spec.h"
#include "g_level.h"
#include "s_sound.h"

IMPLEMENT_SERIAL (DPillar, DMover)

DPillar::DPillar ()
{
}

void DPillar::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_FloorSpeed
			<< m_CeilingSpeed
			<< m_FloorTarget
			<< m_CeilingTarget
			<< m_Crush;
	}
	else
	{
		arc >> m_Type
			>> m_FloorSpeed
			>> m_CeilingSpeed
			>> m_FloorTarget
			>> m_CeilingTarget
			>> m_Crush;
	}
}

void DPillar::RunThink ()
{
	int r, s;

	if (m_Type == pillarBuild)
	{
		r = MoveFloor (m_FloorSpeed, m_FloorTarget, m_Crush, 1);
		s = MoveCeiling (m_CeilingSpeed, m_CeilingTarget, m_Crush, -1);
	}
	else
	{
		r = MoveFloor (m_FloorSpeed, m_FloorTarget, m_Crush, -1);
		s = MoveCeiling (m_CeilingSpeed, m_CeilingTarget, m_Crush, 1);
	}

	if (r == pastdest && s == pastdest)
	{
		S_StopSound (m_Sector->soundorg);
		Destroy ();
	}
}

DPillar::DPillar (sector_t *sector, EPillar type, fixed_t speed,
				  fixed_t height, fixed_t height2, bool crush)
	: DMover (sector)
{
	fixed_t	ceilingdist, floordist;

	sector->floordata = sector->ceilingdata = this;

	m_Type = type;
	m_Crush = crush;

	if (type == pillarBuild)
	{
		// If the pillar height is 0, have the floor and ceiling meet halfway
		if (height == 0)
		{
			m_FloorTarget = m_CeilingTarget =
				(sector->ceilingheight - sector->floorheight) / 2 + sector->floorheight;
			floordist = m_FloorTarget - sector->floorheight;
		}
		else
		{
			m_FloorTarget = m_CeilingTarget =
				sector->floorheight + height;
			floordist = height;
		}
		ceilingdist = sector->ceilingheight - m_CeilingTarget;
	}
	else
	{
		// If one of the heights is 0, figure it out based on the
		// surrounding sectors
		if (height == 0)
		{
			m_FloorTarget = P_FindLowestFloorSurrounding (sector);
			floordist = sector->floorheight - m_FloorTarget;
		}
		else
		{
			floordist = height;
			m_FloorTarget = sector->floorheight - height;
		}
		if (height2 == 0)
		{
			m_CeilingTarget = P_FindHighestCeilingSurrounding (sector);
			ceilingdist = m_CeilingTarget - sector->ceilingheight;
		}
		else
		{
			m_CeilingTarget = sector->ceilingheight + height2;
			ceilingdist = height2;
		}
	}

	// The speed parameter applies to whichever part of the pillar
	// travels the farthest. The other part's speed is then set so
	// that it arrives at its destination at the same time.
	if (floordist > ceilingdist)
	{
		m_FloorSpeed = speed;
		m_CeilingSpeed = FixedDiv (FixedMul (speed, ceilingdist), floordist);
	}
	else
	{
		m_CeilingSpeed = speed;
		m_FloorSpeed = FixedDiv (FixedMul (speed, floordist), ceilingdist);
	}

	S_Sound (sector->soundorg, CHAN_BODY, "plats/pt1_mid", 1, ATTN_NORM);
}

BOOL EV_DoPillar (DPillar::EPillar type, int tag, fixed_t speed, fixed_t height,
				  fixed_t height2, bool crush)
{
	BOOL rtn = false;
	int secnum = -1;

	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sector_t *sec = &sectors[secnum];

		if (sec->floordata || sec->ceilingdata)
			continue;

		if (type == DPillar::pillarBuild && sec->floorheight == sec->ceilingheight)
			continue;

		if (type == DPillar::pillarOpen && sec->floorheight != sec->ceilingheight)
			continue;

		rtn = true;
		new DPillar (sec, type, speed, height, height2, crush);
	}
	return rtn;
}

VERSION_CONTROL (p_pillar_cpp, "$Id:$")

