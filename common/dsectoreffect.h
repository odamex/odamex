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


#ifndef __DSECTOREFFECT_H__
#define __DSECTOREFFECT_H__

#include "dobject.h"
#include "dthinker.h"
#include "r_defs.h"

typedef enum
{
	SEC_FLOOR,
	SEC_PLAT,
	SEC_CEILING,
	SEC_DOOR,
	SEC_ELEVATOR,
	SEC_PILLAR,

	SEC_INVALID
} movertype_t;

class DSectorEffect : public DThinker
{
	DECLARE_SERIAL (DSectorEffect, DThinker)
public:
	DSectorEffect (sector_t *sector);
	~DSectorEffect ();
	virtual DSectorEffect* Clone(sector_t *sector) const;
	virtual void Destroy();
protected:
	DSectorEffect ();
	sector_t	*m_Sector;
};

class DMover : public DSectorEffect
{
	DECLARE_SERIAL (DMover, DSectorEffect);
public:
	DMover (sector_t *sector);
protected:
	enum EResult { ok, crushed, pastdest };
private:
	EResult MovePlane (fixed_t speed, fixed_t dest, bool crush, int floorOrCeiling, int direction);
protected:
	DMover ();
	inline EResult MoveFloor (fixed_t speed, fixed_t dest, bool crush, int direction)
	{
		return MovePlane (speed, dest, crush, 0, direction);
	}
	inline EResult MoveFloor (fixed_t speed, fixed_t dest, int direction)
	{
		return MovePlane (speed, dest, false, 0, direction);
	}
	inline EResult MoveCeiling (fixed_t speed, fixed_t dest, bool crush, int direction)
	{
		return MovePlane (speed, dest, crush, 1, direction);
	}
	inline EResult MoveCeiling (fixed_t speed, fixed_t dest, int direction)
	{
		return MovePlane (speed, dest, false, 1, direction);
	}
};

class DMovingFloor : public DMover
{
	DECLARE_SERIAL (DMovingFloor, DMover);
public:
	DMovingFloor (sector_t *sector);
protected:
	DMovingFloor ();
};

class DMovingCeiling : public DMover
{
	DECLARE_SERIAL (DMovingCeiling, DMover);
public:
	DMovingCeiling (sector_t *sector);
protected:
	DMovingCeiling ();
};

#endif //__DSECTOREFFECT_H__

