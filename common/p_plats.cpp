// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		Plats (i.e. elevator platforms) code, raising/lowering.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "m_alloc.h"
#include "i_system.h"
#include "z_zone.h"
#include "m_random.h"
#include "doomdef.h"
#include "p_local.h"
#include "doomstat.h"
#include "r_state.h"
#include "s_sound.h"

// From sv_main.cpp
void SV_BroadcastSector(int sectornum);

EXTERN_CVAR(co_boomphys)

extern bool predicting;

void P_SetPlatDestroy(DPlat *plat)
{
	if (!plat)
		return;

	plat->m_Status = DPlat::destroy;

	if (clientside && plat->m_Sector)
	{
		plat->m_Sector->floordata = NULL;
		plat->Destroy();
	}
}

IMPLEMENT_SERIAL (DPlat, DMovingFloor)

DPlat::DPlat () :
	m_Status(init)
{
}

void DPlat::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Speed
			<< m_Low
			<< m_High
			<< m_Wait
			<< m_Count
			<< m_Status
			<< m_OldStatus
			<< m_Crush
			<< m_Tag
			<< m_Type
			<< m_Height
			<< m_Lip;
	}
	else
	{
		arc >> m_Speed
			>> m_Low
			>> m_High
			>> m_Wait
			>> m_Count
			>> m_Status
			>> m_OldStatus
			>> m_Crush
			>> m_Tag
			>> m_Type
			>> m_Height
			>> m_Lip;
	}
}

void DPlat::PlayPlatSound ()
{
	if (predicting)
		return;

	const char *snd = NULL;
	switch (m_Status)
	{
	case midup:
	case middown:
		snd = "plats/pt1_mid";
		S_LoopedSound (m_Sector->soundorg, CHAN_BODY, snd, 1, ATTN_NORM);
		return;
	case up:
		snd = "plats/pt1_strt";
		break;
	case down:
		snd = "plats/pt1_strt";
		break;
	case waiting:
	case in_stasis:
	case finished:
		snd = "plats/pt1_stop";
		break;
	default:
		return;
	}

	S_Sound (m_Sector->soundorg, CHAN_BODY, snd, 1, ATTN_NORM);
}

//
// Move a plat up and down
//
void DPlat::RunThink ()
{
	EResult res;

	switch (m_Status)
	{
	case midup:
	case up:
		res = MoveFloor (m_Speed, m_High, m_Crush, 1);

		if (res == crushed && !m_Crush)
		{
			m_Count = m_Wait;
			m_Status = down;
			PlayPlatSound();
		}
		else if (res == pastdest)
		{
			if (m_Type != platToggle)
			{
				m_Count = m_Wait;
				m_Status = waiting;

				switch (m_Type)
				{
					case platDownWaitUpStay:
					case platRaiseAndStay:
					case platUpByValueStay:
					case platDownToNearestFloor:
					case platDownToLowestCeiling:
						m_Status = finished;
						break;

					default:
						break;
				}
			}
			else
			{
				m_OldStatus = m_Status;		//jff 3/14/98 after action wait
				m_Status = in_stasis;		//for reactivation of toggle
			}
			PlayPlatSound();
		}
		break;

	case middown:
	case down:
		res = MoveFloor (m_Speed, m_Low, false, -1);

		if (res == pastdest)
		{
			// if not an instant toggle, start waiting
			if (m_Type != platToggle)		//jff 3/14/98 toggle up down
			{								// is silent, instant, no waiting
				m_Count = m_Wait;
				m_Status = waiting;

				switch (m_Type)
				{
					case platUpWaitDownStay:
					case platUpByValue:
						m_Status = finished;
						break;

					default:
						break;
				}
			}
			else
			{	// instant toggles go into stasis awaiting next activation
				m_OldStatus = m_Status;		//jff 3/14/98 after action wait
				m_Status = in_stasis;		//for reactivation of toggle
			}

			PlayPlatSound();
		}
		//jff 1/26/98 remove the plat if it bounced so it can be tried again
		//only affects plats that raise and bounce

		// remove the plat if it's a pure raise type
		switch (m_Type)
		{
			case platUpByValueStay:
			case platRaiseAndStay:
				m_Status = finished;
				break;

			default:
				break;
		}

		break;

	case waiting:
		if (!--m_Count)
		{
			if (P_FloorHeight(m_Sector) <= m_Low)
				m_Status = up;
			else
				m_Status = down;

			PlayPlatSound();
		}
		break;
	case in_stasis:
		break;

	default:
		break;
	}

	if (m_Status == finished)
	{
		PlayPlatSound();
		m_Status = destroy;
	}

	if (m_Status == destroy)
		P_SetPlatDestroy(this);
}

DPlat::DPlat (sector_t *sector)
	: DMovingFloor (sector), m_Status(init)
{
}

void P_ActivateInStasis (int tag)
{
	DPlat *scan;
	TThinkerIterator<DPlat> iterator;

	while ( (scan = iterator.Next ()) )
	{
		if (scan->m_Tag == tag && scan->m_Status == DPlat::in_stasis)
			scan->Reactivate ();
	}
}


DPlat::DPlat(sector_t *sec, DPlat::EPlatType type, fixed_t height,
			 int speed, int delay, fixed_t lip)
	: DMovingFloor(sec), m_Status(init)
{
	m_Type = type;
	m_Crush = false;
	m_Speed = speed;
	m_Wait = delay;
	m_Height = height;
	m_Lip = lip;

	//jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
	//going down forever -- default lower to plat height when triggered
	m_Low = P_FloorHeight(sec);

	switch (type)
	{
	case DPlat::platRaiseAndStay:
		m_High = P_FindNextHighestFloor(sec);
		m_Status = DPlat::midup;
		PlayPlatSound();
		break;

	case DPlat::platUpByValue:
	case DPlat::platUpByValueStay:
		m_High = P_FloorHeight(sec) + height;
		m_Status = DPlat::midup;
		PlayPlatSound();
		break;

	case DPlat::platDownByValue:
		m_Low = P_FloorHeight(sec) - height;
		m_Status = DPlat::middown;
		PlayPlatSound();
		break;

	case DPlat::platDownWaitUpStay:
		m_Low = P_FindLowestFloorSurrounding(sec) + lip;

		if (m_Low > P_FloorHeight(sec))
			m_Low = P_FloorHeight(sec);

		m_High = P_FloorHeight(sec);
		m_Status = DPlat::down;
		PlayPlatSound();
		break;

	case DPlat::platUpWaitDownStay:
		m_High = P_FindHighestFloorSurrounding(sec);

		if (m_High < P_FloorHeight(sec))
			m_High = P_FloorHeight(sec);

		m_Status = DPlat::up;
		PlayPlatSound();
		break;

	case DPlat::platPerpetualRaise:
		m_Low = P_FindLowestFloorSurrounding(sec) + lip;

		if (m_Low > P_FloorHeight(sec))
			m_Low = P_FloorHeight(sec);

		m_High = P_FindHighestFloorSurrounding (sec);

		if (m_High < P_FloorHeight(sec))
			m_High = P_FloorHeight(sec);

		m_Status = P_Random () & 1 ? DPlat::down : DPlat::up;

		PlayPlatSound();
		break;

	case DPlat::platToggle:	//jff 3/14/98 add new type to support instant toggle
		m_Crush = false;	//jff 3/14/98 crush anything in the way

		// set up toggling between ceiling, floor inclusive
		m_Low = P_CeilingHeight(sec);
		m_High = P_FloorHeight(sec);
		m_Status = DPlat::down;
// 			SN_StartSequence (sec, "Silence");
		break;

	case DPlat::platDownToNearestFloor:
		m_Low = P_FindNextLowestFloor(sec) + lip;
		m_Status = DPlat::down;
		m_High = P_FloorHeight(sec);
		PlayPlatSound();
		break;

	case DPlat::platDownToLowestCeiling:
	    m_Low = P_FindLowestCeilingSurrounding (sec);
		m_High = P_FloorHeight(sec);

		if (m_Low > P_FloorHeight(sec))
			m_Low = P_FloorHeight(sec);

		m_Status = DPlat::down;
		PlayPlatSound();
		break;

	default:
		break;
	}
}

// Clones a DPlat and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DPlat* DPlat::Clone(sector_t* sec) const
{
	DPlat* plat = new DPlat(*this);

	plat->Orphan();
	plat->m_Sector = sec;

	return plat;
}

//
// Do Platforms
//	[RH] Changed amount to height and added delay,
//		 lip, change, tag, and speed parameters.
//
BOOL EV_DoPlat (int tag, line_t *line, DPlat::EPlatType type, fixed_t height,
				int speed, int delay, fixed_t lip, int change)
{
	DPlat *plat;
	int secnum;
	sector_t *sec;
	int rtn = false;
	BOOL manual = false;

	// [RH] If tag is zero, use the sector on the back side
	//		of the activating line (if any).
	if (co_boomphys && tag == 0)
	{
		if (!line || !(sec = line->backsector))
			return false;
		secnum = sec - sectors;
		manual = true;
		goto manual_plat;
	}

	//	Activate all <type> plats that are in_stasis
	switch (type)
	{
	case DPlat::platToggle:
		rtn = true;
	case DPlat::platPerpetualRaise:
		P_ActivateInStasis (tag);
		break;

	default:
		break;
	}

	secnum = -1;
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

manual_plat:
		if (sec->floordata)
		{
			if (co_boomphys && manual)
				return false;
			else
				continue;
		}

		// Find lowest & highest floors around sector
		rtn = true;
		plat = new DPlat(sec,type, height, speed, delay, lip);
		P_AddMovingFloor(sec);

		plat->m_Tag = tag;

		if (change)
		{
			if (line)
				sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
			if (change == 1)
				sec->special = 0;	// Stop damage and other stuff, if any
			if (serverside)
				SV_BroadcastSector(secnum);
		}

		if (manual)
			return rtn;
	}
	return rtn;
}

void DPlat::Reactivate ()
{
	if (m_Type == platToggle)	//jff 3/14/98 reactivate toggle type
		m_Status = m_OldStatus == up ? down : up;
	else
		m_Status = m_OldStatus;
}

void DPlat::Stop ()
{
	m_OldStatus = m_Status;
	m_Status = in_stasis;
}

void EV_StopPlat (int tag)
{
	DPlat *scan;
	TThinkerIterator<DPlat> iterator;

	while ( (scan = iterator.Next ()) )
	{
		if (scan->m_Status != DPlat::in_stasis && scan->m_Tag == tag)
			scan->Stop ();
	}
}


VERSION_CONTROL (p_plats_cpp, "$Id$")

