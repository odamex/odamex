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
//	Separated Sector effects (?)
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include "p_local.h"
#include "dsectoreffect.h"



EXTERN_CVAR(co_boomphys)			// [ML] Roll-up of various compat options

IMPLEMENT_SERIAL (DSectorEffect, DThinker)

DSectorEffect::DSectorEffect ()
{
	m_Sector = NULL;
}

DSectorEffect::~DSectorEffect ()
{
}

void DSectorEffect::Destroy ()
{
	if (m_Sector)
	{
		if (m_Sector->floordata == this)
			m_Sector->floordata = NULL;
		if (m_Sector->ceilingdata == this)
			m_Sector->ceilingdata = NULL;
		if (m_Sector->lightingdata == this)
			m_Sector->lightingdata = NULL;
	}

	Super::Destroy();
}

DSectorEffect::DSectorEffect (sector_t *sector)
{
	m_Sector = sector;
}

// Base Clone method, for cloning sector actions.
//
// This class must not be called directly.  Subclass it if you need it.
//
// All subclasses should allocate a subclass with `new`, orphan it,
// then set the sector of the subclass to the passed sector.
DSectorEffect* DSectorEffect::Clone(sector_t *sector) const
{
	throw CFatalError("DSectorEffect::Clone() called as a base class.");
}

void DSectorEffect::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
		arc << m_Sector;
	else
		arc >> m_Sector;

}

IMPLEMENT_SERIAL (DMover, DSectorEffect)

DMover::DMover ()
{
}

DMover::DMover (sector_t *sector)
	: DSectorEffect (sector)
{
}

void DMover::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
}

IMPLEMENT_SERIAL (DMovingFloor, DMover)

DMovingFloor::DMovingFloor ()
{
}

DMovingFloor::DMovingFloor (sector_t *sector)
	: DMover (sector)
{
	sector->floordata = this;
}

void DMovingFloor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
}

IMPLEMENT_SERIAL (DMovingCeiling, DMover)

DMovingCeiling::DMovingCeiling ()
{
}

DMovingCeiling::DMovingCeiling (sector_t *sector)
	: DMover (sector)
{
	sector->ceilingdata = this;
}

void DMovingCeiling::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
}

//
// Move a plane (floor or ceiling) and check for crushing
// [RH] Crush specifies the actual amount of crushing damage inflictable.
//		(Use -1 to prevent it from trying to crush)
//
DMover::EResult DMover::MovePlane (fixed_t speed, fixed_t dest, bool crush,
								   int floorOrCeiling, int direction)
{
	bool	 	flag;
	fixed_t 	lastpos;

	switch (floorOrCeiling)
	{
	case 0:
		// FLOOR
		switch (direction)
		{
		case -1:
			// DOWN
			lastpos = P_FloorHeight(m_Sector);
			if (lastpos - speed < dest)
			{
				P_ChangeFloorHeight(m_Sector, dest - lastpos);
				flag = P_ChangeSector (m_Sector, crush);
				if (flag == true)
				{
					P_ChangeFloorHeight(m_Sector, lastpos - dest);
					P_ChangeSector (m_Sector, crush);
					//return crushed;
				}
				return pastdest;
			}
			else
			{
				P_ChangeFloorHeight(m_Sector, -speed);
				flag = P_ChangeSector (m_Sector, crush);
				// comp_floors: co_boomsectortouch enables Boom behaviour which
				// allows floor to lower when actors on it are stuck in ceiling
				// [ML] Now part of co_boomphys
				if (flag == true && !co_boomphys)
				{
					P_ChangeFloorHeight(m_Sector, speed);	// should be lastpos
					P_ChangeSector (m_Sector, crush);
					return crushed;
				}
			}
			break;

		case 1:
			// UP
			lastpos = P_FloorHeight(m_Sector);
			// comp_floors: jff 02/04/98 keep floor from moving thru ceilings
			// co_boomsectortouch enables Boom behaviour where a rising floor
			// is prevented from moving higher than the ceiling above it
			// [ML] Now part of co_boomphys
			if (co_boomphys)
			{
				fixed_t h = P_CeilingHeight(m_Sector);
				if (dest > h) // floor is trying to rise through ceiling
					dest = h;
			}

			if (lastpos + speed > dest)
			{
				P_ChangeFloorHeight(m_Sector, dest - lastpos);
				flag = P_ChangeSector (m_Sector, crush);
				if (flag == true)
				{
					P_ChangeFloorHeight(m_Sector, lastpos - dest);
					P_ChangeSector (m_Sector, crush);
				}
				return pastdest;
			}
			else
			{
				// COULD GET CRUSHED
				P_ChangeFloorHeight(m_Sector, speed);
				flag = P_ChangeSector (m_Sector, crush);
				if (flag == true)
				{
					// comp_floors: jff 1/25/98 fix floor crusher
					// co_boomsectortouch enables Boom behaviour where rising
					// floor holds in place until victim moves or is crushed
					// [ML] Now part of co_boomphys
					if (crush && !co_boomphys)
						return crushed;
					P_ChangeFloorHeight(m_Sector, -speed);
					P_ChangeSector (m_Sector, crush);
					return crushed;
				}
			}
			break;
		}
		break;

	  case 1:
		// CEILING
		switch(direction)
		{
		case -1:
			// DOWN
			lastpos = P_CeilingHeight(m_Sector);
			// comp_floors: jff 02/04/98 keep ceiling from moving thru floors
			// co_boomsectortouch enables Boom behaviour where a lowering
			// ceiling is prevented from moving lower than the floor below it
			// [ML] Now part of co_boomphys
			if (co_boomphys)
			{
				fixed_t h = P_FloorHeight(m_Sector);
				if (dest < h) // ceiling is trying to lower through floor
					dest = h;
			}

			if (lastpos - speed < dest)
			{
				P_SetCeilingHeight(m_Sector, dest);
				flag = P_ChangeSector (m_Sector, crush);

				if (flag == true)
				{
					P_SetCeilingHeight(m_Sector, lastpos);
					P_ChangeSector (m_Sector, crush);
				}
				return pastdest;
			}
			else
			{
				// COULD GET CRUSHED
				P_SetCeilingHeight(m_Sector, lastpos - speed);
				flag = P_ChangeSector (m_Sector, crush);

				if (flag == true)
				{
					if (crush)
						return crushed;

					P_SetCeilingHeight(m_Sector, lastpos);
					P_ChangeSector (m_Sector, crush);
					return crushed;
				}
			}
			break;

		case 1:
			// UP
			lastpos = P_CeilingHeight(m_Sector);

			if (lastpos + speed > dest)
			{
				P_SetCeilingHeight(m_Sector, dest);
				flag = P_ChangeSector (m_Sector, crush);
				if (flag == true)
				{
					P_SetCeilingHeight(m_Sector, lastpos);
					P_ChangeSector (m_Sector, crush);
				}
				return pastdest;
			}
			else
			{
				P_SetCeilingHeight(m_Sector, lastpos + speed);
				flag = P_ChangeSector (m_Sector, crush);
// UNUSED
#if 0
				if (flag == true)
				{
					P_ChangeCeilingHeight(m_Sector, -speed);
					P_ChangeSector (m_Sector, crush);
					return crushed;
				}
#endif
			}
			break;
		}
		break;

	}
	return ok;
}

VERSION_CONTROL (dsectoreffect_cpp, "$Id$")

