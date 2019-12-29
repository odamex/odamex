// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2015 by The Odamex Team.
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

extern bool predicting;

void P_SetPillarDestroy(DPillar *pillar)
{
	if (!pillar)
		return;

	pillar->m_Status = DPillar::destroy;
	
	if (clientside && pillar->m_Sector)
	{
		pillar->m_Sector->ceilingdata = NULL;
		pillar->m_Sector->floordata = NULL;		
		pillar->Destroy();
	}
}

IMPLEMENT_SERIAL (DPillar, DMover)

DPillar::DPillar () :
	m_Status(init)
{
}

void DPillar::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_Status
			<< m_FloorSpeed
			<< m_CeilingSpeed
			<< m_FloorTarget
			<< m_CeilingTarget
			<< m_Crush;
	}
	else
	{
		arc >> m_Type
			>> m_Status
			>> m_FloorSpeed
			>> m_CeilingSpeed
			>> m_FloorTarget
			>> m_CeilingTarget
			>> m_Crush;
	}
}

void DPillar::PlayPillarSound()
{
	if (predicting || !m_Sector)
		return;
	
	if (m_Status == init)
		S_Sound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_mid", 1, ATTN_NORM);
	else if (m_Status == finished)
		S_StopSound(m_Sector->soundorg);
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
		m_Status = finished;
		PlayPillarSound();
		P_SetPillarDestroy(this);
	}
}

DPillar::DPillar (sector_t *sector, EPillar type, fixed_t speed,
				  fixed_t height, fixed_t height2, bool crush)
	: DMover (sector), m_Status(init)
{
	fixed_t	ceilingdist, floordist;

	sector->floordata = sector->ceilingdata = this;

	fixed_t floorheight = P_FloorHeight(sector);
	fixed_t ceilingheight = P_CeilingHeight(sector);

	m_Type = type;
	m_Crush = crush;

	if (type == pillarBuild)
	{
		// If the pillar height is 0, have the floor and ceiling meet halfway
		if (height == 0)
		{
			m_FloorTarget = m_CeilingTarget =
				 (ceilingheight - floorheight) / 2 + floorheight;
			floordist = m_FloorTarget - floorheight;
		}
		else
		{
			m_FloorTarget = m_CeilingTarget = floorheight + height;
			floordist = height;
		}
		ceilingdist = ceilingheight - m_CeilingTarget;
	}
	else
	{
		// If one of the heights is 0, figure it out based on the
		// surrounding sectors
		if (height == 0)
		{
			m_FloorTarget = P_FindLowestFloorSurrounding (sector);
			floordist = floorheight - m_FloorTarget;
		}
		else
		{
			floordist = height;
			m_FloorTarget = floorheight - height;
		}
		if (height2 == 0)
		{
			m_CeilingTarget = P_FindHighestCeilingSurrounding (sector);
			ceilingdist = m_CeilingTarget - ceilingheight;
		}
		else
		{
			m_CeilingTarget = ceilingheight + height2;
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

	PlayPillarSound();
}

// Clones a DPillar and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DPillar* DPillar::Clone(sector_t* sec) const
{
	DPillar* pillar = new DPillar(*this);

	pillar->Orphan();
	pillar->m_Sector = sec;

	return pillar;
}

BOOL EV_DoPillar (DPillar::EPillar type, int tag, fixed_t speed, fixed_t height,
				  fixed_t height2, bool crush)
{
	BOOL rtn = false;
	int secnum = -1;

	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sector_t *sec = &sectors[secnum];
		fixed_t floorheight = P_FloorHeight(sec);
		fixed_t ceilingheight = P_CeilingHeight(sec);

		if (sec->floordata || sec->ceilingdata)
			continue;

		if (type == DPillar::pillarBuild && floorheight == ceilingheight)
			continue;

		if (type == DPillar::pillarOpen && floorheight != ceilingheight)
			continue;

		rtn = true;
		new DPillar (sec, type, speed, height, height2, crush);
		P_AddMovingCeiling(sec);
	}
	return rtn;
}

VERSION_CONTROL (p_pillar_cpp, "$Id$")

