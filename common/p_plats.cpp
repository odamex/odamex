// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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

extern bool predicting;

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
			<< m_Type;
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
			>> m_Type;
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
						if (serverside)
							m_Status = destroy;	
						else
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
						if (serverside)
							m_Status = destroy;
						else
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
				if (serverside)
					m_Status = destroy;
				else
					m_Status = finished;
				break;
			default:
				break;
		}

		break;
		
	case waiting:
		if (!--m_Count)
		{
			if (P_FloorHeight(m_Sector) == m_Low)
				m_Status = up;
			else
				m_Status = down;
			/*
			if (m_Type == platToggle)
				SN_StartSequence (m_Sector, "Silence");
			else
				PlayPlatSound ("Platform");
			*/
			PlayPlatSound();
		}
		break;
	case in_stasis:
		break;
	default:
		break;
	}

	if (clientside && m_Status == destroy)
	{
		if (serverside) // single player game
		{
			// make sure we play the finished sound because it doesn't
			// get called for servers otherwise
			m_Status = finished;
			PlayPlatSound();
			m_Status = destroy;
		}

		m_Sector->floordata = NULL;
		Destroy();
		return;
	}
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

//
// Do Platforms
//	[RH] Changed amount to height and added delay,
//		 lip, change, tag, and speed parameters.
//
BOOL EV_DoPlat (int tag, line_t *line, DPlat::EPlatType type, int height,
				int speed, int delay, int lip, int change)
{
	DPlat *plat;
	int secnum;
	sector_t *sec;
	int rtn = false;
	BOOL manual = false;

	// [RH] If tag is zero, use the sector on the back side
	//		of the activating line (if any).
	if (!tag)
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
			continue;
		
		// Find lowest & highest floors around sector
		rtn = true;
		plat = new DPlat (sec);
		P_AddMovingFloor(sec);

		plat->m_Type = type;
		plat->m_Crush = false;
		plat->m_Tag = tag;
		plat->m_Speed = speed;
		plat->m_Wait = delay;

		//jff 1/26/98 Avoid raise plat bouncing a head off a ceiling and then
		//going down forever -- default lower to plat height when triggered
		plat->m_Low = P_FloorHeight(sec);

		if (change)
		{
			if (line)
				sec->floorpic = sides[line->sidenum[0]].sector->floorpic;
			if (change == 1)
				sec->special = 0;	// Stop damage and other stuff, if any
		}

		switch (type)
		{
		case DPlat::platRaiseAndStay:
			plat->m_High = P_FindNextHighestFloor(sec);
			plat->m_Status = DPlat::midup;
			plat->PlayPlatSound();
			break;

		case DPlat::platUpByValue:
		case DPlat::platUpByValueStay:
			plat->m_High = P_FloorHeight(sec) + height;
			plat->m_Status = DPlat::midup;
			plat->PlayPlatSound();
			break;
		
		case DPlat::platDownByValue:
			plat->m_Low = P_FloorHeight(sec) - height;
			plat->m_Status = DPlat::middown;
			plat->PlayPlatSound();
			break;

		case DPlat::platDownWaitUpStay:
			plat->m_Low = P_FindLowestFloorSurrounding (sec) + lip*FRACUNIT;

			if (plat->m_Low > P_FloorHeight(sec))
				plat->m_Low = P_FloorHeight(sec);

			plat->m_High = P_FloorHeight(sec);
			plat->m_Status = DPlat::down;
			plat->PlayPlatSound();
			break;
		
		case DPlat::platUpWaitDownStay:
			plat->m_High = P_FindHighestFloorSurrounding (sec);

			if (plat->m_High < P_FloorHeight(sec))
				plat->m_High = P_FloorHeight(sec);

			plat->m_Status = DPlat::up;
			plat->PlayPlatSound();
			break;

		case DPlat::platPerpetualRaise:
			plat->m_Low = P_FindLowestFloorSurrounding (sec) + lip*FRACUNIT;

			if (plat->m_Low > P_FloorHeight(sec))
				plat->m_Low = P_FloorHeight(sec);

			plat->m_High = P_FindHighestFloorSurrounding (sec);

			if (plat->m_High < P_FloorHeight(sec))
				plat->m_High = P_FloorHeight(sec);

			plat->m_Status = P_Random () & 1 ? DPlat::down : DPlat::up;

			plat->PlayPlatSound();
			break;

		case DPlat::platToggle:	//jff 3/14/98 add new type to support instant toggle
			plat->m_Crush = false;	//jff 3/14/98 crush anything in the way

			// set up toggling between ceiling, floor inclusive
			plat->m_Low = P_CeilingHeight(sec);
			plat->m_High = P_FloorHeight(sec);
			plat->m_Status = DPlat::down;
// 			SN_StartSequence (sec, "Silence");
			break;

		case DPlat::platDownToNearestFloor:
			plat->m_Low = P_FindNextLowestFloor(sec) + lip*FRACUNIT;
			plat->m_Status = DPlat::down;
			plat->m_High = P_FloorHeight(sec);
			plat->PlayPlatSound();
			break;

		case DPlat::platDownToLowestCeiling:
		    plat->m_Low = P_FindLowestCeilingSurrounding (sec);
			plat->m_High = P_FloorHeight(sec);

			if (plat->m_Low > P_FloorHeight(sec))
				plat->m_Low = P_FloorHeight(sec);

			plat->m_Status = DPlat::down;
			plat->PlayPlatSound();
			break;

		default:
			break;
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

