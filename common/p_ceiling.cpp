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
//	Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------


#include "m_alloc.h"
#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "doomstat.h"
#include "r_state.h"

EXTERN_CVAR(co_boomphys)

extern bool predicting;

//
// CEILINGS
//

void P_SetCeilingDestroy(DCeiling *ceiling)
{
	if (!ceiling)
		return;

	ceiling->m_Status = DCeiling::destroy;

	if (clientside && ceiling->m_Sector)
	{
		ceiling->m_Sector->ceilingdata = NULL;
		ceiling->Destroy();
	}
}

IMPLEMENT_SERIAL (DCeiling, DMovingCeiling)

DCeiling::DCeiling ()
{
}

void DCeiling::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_Status
			<< m_BottomHeight
			<< m_TopHeight
			<< m_Speed
			<< m_Speed1
			<< m_Speed2
			<< m_Crush
			<< m_Silent
			<< m_Direction
			<< m_Texture
			<< m_NewSpecial
			<< m_Tag
			<< m_OldDirection;
	}
	else
	{
		arc >> m_Type
			>> m_Status
			>> m_BottomHeight
			>> m_TopHeight
			>> m_Speed
			>> m_Speed1
			>> m_Speed2
			>> m_Crush
			>> m_Silent
			>> m_Direction
			>> m_Texture
			>> m_NewSpecial
			>> m_Tag
			>> m_OldDirection;
	}
}

void DCeiling::PlayCeilingSound ()
{
	if (predicting || !m_Sector)
		return;

	if (m_Sector->seqType >= 0)
	{
		SN_StartSequence (m_Sector, m_Sector->seqType, SEQ_PLATFORM);
	}
	else
	{
		if (m_Silent == 2)
			SN_StartSequence (m_Sector, "Silence");
		else if (m_Silent == 1)
			SN_StartSequence (m_Sector, "CeilingSemiSilent");
		else
			S_LoopedSound (m_Sector->soundorg, CHAN_BODY, "plats/pt1_mid", 1, ATTN_NORM);
	}
}

//
// T_MoveCeiling
//
void DCeiling::RunThink ()
{
	EResult res;

	switch (m_Direction)
	{
	case 0:
		// IN STASIS
		break;
	case 1:
		// UP
		res = MoveCeiling (m_Speed, m_TopHeight, m_Direction);

		if (res == pastdest)
		{
			S_StopSound (m_Sector->soundorg);
			S_Sound (m_Sector->soundorg, CHAN_BODY, "plats/pt1_stop", 1, ATTN_NORM);

			switch (m_Type)
			{
			case ceilCrushAndRaise:
				m_Direction = -1;
				m_Speed = m_Speed1;
				PlayCeilingSound ();
				break;

			// movers with texture change, change the texture then get removed
			case genCeilingChgT:
			case genCeilingChg0:
				m_Sector->special = m_NewSpecial;
			case genCeilingChg:
				m_Sector->ceilingpic = m_Texture;
				// fall through
			default:
				Destroy ();
				break;
			}

		}
		break;

	case -1:
		// DOWN
		res = MoveCeiling (m_Speed, m_BottomHeight, m_Crush, m_Direction);

		if (res == pastdest)
		{
			S_StopSound (m_Sector->soundorg);
			S_Sound (m_Sector->soundorg, CHAN_BODY, "plats/pt1_stop", 1, ATTN_NORM);

			switch (m_Type)
			{
			case ceilCrushAndRaise:
			case ceilCrushRaiseAndStay:
				m_Speed = m_Speed2;
				m_Direction = 1;
				PlayCeilingSound ();
				break;

			// in the case of ceiling mover/changer, change the texture
			// then remove the active ceiling
			case genCeilingChgT:
			case genCeilingChg0:
				m_Sector->special = m_NewSpecial;
			case genCeilingChg:
				m_Sector->ceilingpic = m_Texture;
				// fall through
			default:
				Destroy ();
				break;
			}
		}
		else // ( res != pastdest )
		{
			if (res == crushed)
			{
				switch (m_Type)
				{
				case ceilCrushAndRaise:
				case ceilLowerAndCrush:
					if (m_Speed1 == FRACUNIT && m_Speed2 == FRACUNIT)
						m_Speed = FRACUNIT / 8;
						break;

				default:
					break;
				}
			}
		}
		break;
	}
}

DCeiling::DCeiling (sector_t *sec)
	: DMovingCeiling (sec), m_Status(init)
{
}

DCeiling::DCeiling (sector_t *sec, fixed_t speed1, fixed_t speed2, int silent)
	: DMovingCeiling (sec), m_Status(init)
{
	m_Crush = false;
	m_Speed = m_Speed1 = speed1;
	m_Speed2 = speed2;
	m_Silent = silent;
}

// Clones a DCeiling and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DCeiling* DCeiling::Clone(sector_t* sec) const
{
	DCeiling* ceiling = new DCeiling(*this);

	ceiling->Orphan();
	ceiling->m_Sector = sec;

	return ceiling;
}

//
// Restart a ceiling that's in-stasis
// [RH] Passed a tag instead of a line and rewritten to use list
//
void P_ActivateInStasisCeiling (int tag)
{
	DCeiling *scan;
	TThinkerIterator<DCeiling> iterator;

	while ( (scan = iterator.Next ()) )
	{
		if (scan->m_Tag == tag && scan->m_Direction == 0)
		{
			scan->m_Direction = scan->m_OldDirection;
			scan->PlayCeilingSound ();
		}
	}
}

//
// EV_DoCeiling
// Move a ceiling up/down and all around!
//
// [RH] Added tag, speed, speed2, height, crush, silent, change params
BOOL EV_DoCeiling (DCeiling::ECeiling type, line_t *line,
				   int tag, fixed_t speed, fixed_t speed2, fixed_t height,
				   bool crush, int silent, int change)
{
	int 		secnum;
	BOOL 		rtn;
	sector_t*	sec;
	DCeiling*	ceiling;
	BOOL		manual = false;
	fixed_t		targheight = 0;

	rtn = false;

	// check if a manual trigger, if so do just the sector on the backside
	//
	if (co_boomphys && tag == 0)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec-sectors;
		manual = true;
		// [RH] Hack to let manual crushers be retriggerable, too
		tag ^= secnum | 0x1000000;
		P_ActivateInStasisCeiling (tag);
		goto manual_ceiling;
	}

	//	Reactivate in-stasis ceilings...for certain types.
	// This restarts a crusher after it has been stopped
	if (type == DCeiling::ceilCrushAndRaise)
	{
		P_ActivateInStasisCeiling (tag);
	}

	secnum = -1;
	// affects all sectors with the same tag as the linedef
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];
manual_ceiling:
		// if ceiling already moving, don't start a second function on it
		if (sec->ceilingdata)
		{
			if (co_boomphys && manual)
				return false;
			else
				continue;
		}

		fixed_t ceilingheight = P_CeilingHeight(sec);
		fixed_t floorheight = P_FloorHeight(sec);

		// new door thinker
		rtn = 1;
		ceiling = new DCeiling (sec, speed, speed2, silent);
		P_AddMovingCeiling(sec);

		switch (type)
		{
		case DCeiling::ceilCrushAndRaise:
		case DCeiling::ceilCrushRaiseAndStay:
			ceiling->m_TopHeight = ceilingheight;
		case DCeiling::ceilLowerAndCrush:
			ceiling->m_Crush = crush;
			targheight = ceiling->m_BottomHeight = floorheight + 8*FRACUNIT;
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToHighest:
			targheight = ceiling->m_TopHeight = P_FindHighestCeilingSurrounding (sec);
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilLowerByValue:
			targheight = ceiling->m_BottomHeight = ceilingheight - height;
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseByValue:
			targheight = ceiling->m_TopHeight = ceilingheight + height;
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilMoveToValue:
			{
			  int diff = height - ceilingheight;

			  if (diff < 0) {
				  targheight = ceiling->m_BottomHeight = height;
				  ceiling->m_Direction = -1;
			  } else {
				  targheight = ceiling->m_TopHeight = height;
				  ceiling->m_Direction = 1;
			  }
			}
			break;

		case DCeiling::ceilLowerToHighestFloor:
			targheight = ceiling->m_BottomHeight = P_FindHighestFloorSurrounding (sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToHighestFloor:
			targheight = ceiling->m_TopHeight = P_FindHighestFloorSurrounding (sec);
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilLowerInstant:
			targheight = ceiling->m_BottomHeight = P_CeilingHeight(sec) - height;
			ceiling->m_Direction = -1;
			ceiling->m_Speed = height;
			break;

		case DCeiling::ceilRaiseInstant:
			targheight = ceiling->m_TopHeight = ceilingheight + height;
			ceiling->m_Direction = 1;
			ceiling->m_Speed = height;
			break;

		case DCeiling::ceilLowerToNearest:
			targheight = ceiling->m_BottomHeight = P_FindNextLowestCeiling(sec);
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilRaiseToNearest:
			targheight = ceiling->m_TopHeight = P_FindNextHighestCeiling(sec);
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilLowerToLowest:
			targheight = ceiling->m_BottomHeight = P_FindLowestCeilingSurrounding (sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToLowest:
			targheight = ceiling->m_TopHeight = P_FindLowestCeilingSurrounding (sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilLowerToFloor:
			targheight = ceiling->m_BottomHeight = floorheight;
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToFloor:
			targheight = ceiling->m_TopHeight = floorheight;
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilLowerToHighest:
			targheight = ceiling->m_BottomHeight = P_FindHighestCeilingSurrounding (sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilLowerByTexture:
			targheight = ceiling->m_BottomHeight =
				ceilingheight - P_FindShortestUpperAround (secnum);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseByTexture:
			targheight = ceiling->m_TopHeight =
				ceilingheight + P_FindShortestUpperAround (secnum);
			ceiling->m_Direction = 1;
			break;

		case DCeiling::genCeilingChg0: // denis - fixme - will these need code?
		case DCeiling::genCeilingChgT:
		case DCeiling::genCeilingChg:
			break;
		}

		ceiling->m_Tag = tag;
		ceiling->m_Type = type;

		// set texture/type change properties
		if (change & 3)		// if a texture change is indicated
		{
			if (change & 4)	// if a numeric model change
			{
				sector_t *sec;

				//jff 5/23/98 find model with floor at target height if target
				//is a floor type
				sec = (type == DCeiling::ceilRaiseToHighest ||
					   type == DCeiling::ceilRaiseToFloor ||
					   type == DCeiling::ceilLowerToHighest ||
					   type == DCeiling::ceilLowerToFloor) ?
					P_FindModelFloorSector (targheight, secnum) :
					P_FindModelCeilingSector (targheight, secnum);
				if (sec)
				{
					ceiling->m_Texture = sec->ceilingpic;
					switch (change & 3)
					{
						case 1:		// type is zeroed
							ceiling->m_NewSpecial = 0;
							ceiling->m_Type = DCeiling::genCeilingChg0;
							break;
						case 2:		// type is copied
							ceiling->m_NewSpecial = sec->special;
							ceiling->m_Type = DCeiling::genCeilingChgT;
							break;
						case 3:		// type is left alone
							ceiling->m_Type = DCeiling::genCeilingChg;
							break;
					}
				}
			}
			else if (line)	// else if a trigger model change
			{

				ceiling->m_Texture = line->frontsector->ceilingpic;
				switch (change & 3)
				{
					case 1:		// type is zeroed
						ceiling->m_NewSpecial = 0;
						ceiling->m_Type = DCeiling::genCeilingChg0;
						break;
					case 2:		// type is copied
						ceiling->m_NewSpecial = line->frontsector->special;
						ceiling->m_Type = DCeiling::genCeilingChgT;
						break;
					case 3:		// type is left alone
						ceiling->m_Type = DCeiling::genCeilingChg;
						break;
				}
			}
		}

		ceiling->PlayCeilingSound ();

		if (manual)
			return rtn;
	}
	return rtn;
}

//
// EV_CeilingCrushStop
// Stop a ceiling from crushing!
// [RH] Passed a tag instead of a line and rewritten to use list
//
BOOL EV_CeilingCrushStop (int tag)
{
	BOOL rtn = false;
	DCeiling *scan;
	TThinkerIterator<DCeiling> iterator;

	while ( (scan = iterator.Next ()) )
	{
		if (scan->m_Tag == tag && scan->m_Direction != 0)
		{
			S_StopSound (scan->m_Sector->soundorg);
			scan->m_OldDirection = scan->m_Direction;
			scan->m_Direction = 0;		// in-stasis;
			rtn = true;
		}
	}

	return rtn;
}

VERSION_CONTROL (p_ceiling_cpp, "$Id$")

