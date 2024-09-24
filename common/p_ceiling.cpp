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
//	Ceiling aninmation (lowering, crushing, raising)
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "r_state.h"
#include "p_mapformat.h"

EXTERN_CVAR(co_boomphys)

extern bool predicting;

void P_ResetTransferSpecial(newspecial_s* newspecial);
unsigned int P_ResetSectorTransferFlags(const unsigned int flags);

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
			if (m_Silent <= 1)
			{
				S_StopSound(m_Sector->soundorg);
				S_Sound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_stop", 1, ATTN_NORM);
			}

			switch (m_Type)
			{
			case ceilRaiseToHighest:
			case genCeiling:
				Destroy();
				break;
			// movers with texture change, change the texture then get removed
			case genCeilingChgT:
			case genCeilingChg0:
				m_Sector->special = m_NewSpecial;
				m_Sector->damageamount = m_NewDamageRate;
				m_Sector->damageinterval = m_NewDmgInterval;
				m_Sector->leakrate = m_NewLeakRate;
				m_Sector->flags = m_NewFlags;
			case genCeilingChg:
				m_Sector->ceilingpic = m_Texture;
				Destroy();
				break;
			case silentCrushAndRaise:
			case genSilentCrusher:
			case genCrusher:
			case fastCrushAndRaise:
			case crushAndRaise:
				m_Direction = -1;
				break;
			case ceilCrushAndRaise:
				m_Direction = -1;
				m_Speed = m_Speed1;
				PlayCeilingSound();
				break;
			default:
				Destroy ();
				break;
			}

		}
		break;

	case -1:
		// DOWN
		res = MoveCeiling(m_Speed, m_BottomHeight, m_Crush, m_Direction,
		                  m_CrushMode == crushHexen);

		if (res == pastdest)
		{
			if (m_Silent <= 1)
			{
				S_StopSound(m_Sector->soundorg);
				S_Sound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_stop", 1, ATTN_NORM);
			}

			switch (m_Type)
			{
			// 02/09/98 jff change slow crushers' speed back to normal
			// start back up
			case genSilentCrusher:
			case genCrusher:
				if (m_Speed < CEILSPEED * 3)
					m_Speed = m_Speed2;
				m_Direction = 1; // jff 2/22/98 make it go back up!
				break;

			// make platform stop at bottom of all crusher strokes
			// except generalized ones, reset speed, start back up
			case silentCrushAndRaise:
			case crushAndRaise:
				m_Speed = CEILSPEED;
			case fastCrushAndRaise:
				m_Speed = m_Speed2;
				m_Direction = 1;
				PlayCeilingSound ();
				break;

			// in the case of ceiling mover/changer, change the texture
			// then remove the active ceiling
			case genCeilingChgT:
			case genCeilingChg0:
				m_Sector->special = m_NewSpecial;
				m_Sector->damageamount = m_NewDamageRate;
				m_Sector->damageinterval = m_NewDmgInterval;
				m_Sector->leakrate = m_NewLeakRate;
				m_Sector->flags = m_NewFlags;
			case genCeilingChg:
				m_Sector->ceilingpic = m_Texture;
				Destroy();
				break;
			case lowerAndCrush:
			case lowerToFloor:
			case lowerToLowest:
			case lowerToMaxFloor:
			case genCeiling:
				Destroy();
				break;

			case ceilCrushAndRaise:
			case ceilCrushRaiseAndStay:
				m_Speed = m_Speed2;
				m_Direction = 1;
				if (m_Silent == 1)
					PlayCeilingSound();
				break;
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
				// jff 02/08/98 slow down slow crushers on obstacle
				case genCrusher:
				case genSilentCrusher:
					if (m_Speed1 < CEILSPEED * 3)
						m_Speed = CEILSPEED / 8;
					break;
				case silentCrushAndRaise:
				case crushAndRaise:
				case lowerAndCrush:
					m_Speed = CEILSPEED / 8;
					break;
				case ceilCrushAndRaise:
				case ceilLowerAndCrush:
				case ceilCrushRaiseAndStay:
					if (m_CrushMode == crushSlowdown)
						m_Speed = CEILSPEED / 8;
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
	m_Crush = NO_CRUSH;
	m_Speed = m_Speed1 = speed1;
	m_Speed2 = speed2;
	m_Silent = silent;
}

DCeiling::DCeiling(sector_t* sec, line_t* line, int silent, int speed)
    : DMovingCeiling(sec), m_Status(init)
{
	m_Type = silent ? genSilentCrusher : genCrusher;
	m_Crush = DOOM_CRUSH;
	m_CrushMode = crushDoom;
	m_Direction = -1;
	m_Sector = sec;
	m_Texture = sec->ceilingpic;
	m_NewSpecial = sec->special;
	m_NewDamageRate = sec->damageamount;
	m_NewDmgInterval = sec->damageinterval;
	m_NewLeakRate = sec->leakrate;
	m_NewFlags = sec->flags;
	m_Tag = sec->tag;
	m_Silent = m_Type == genSilentCrusher ? 2 : 1;
	m_TopHeight = sec->ceilingheight;
	m_BottomHeight = sec->floorheight + (8 * FRACUNIT);

	// setup ceiling motion speed
	switch (speed)
	{
	case SpeedSlow:
		m_Speed = m_Speed1 = CEILSPEED;
		break;
	case SpeedNormal:
		m_Speed = m_Speed1 = CEILSPEED * 2;
		break;
	case SpeedFast:
		m_Speed = m_Speed1 = CEILSPEED * 4;
		break;
	case SpeedTurbo:
		m_Speed = m_Speed1 = CEILSPEED * 8;
		break;
	default:
		break;
	}
	m_Speed2 = m_Speed;
}

DCeiling::DCeiling(sector_t* sec, line_t* line, int speed,
                   int target, int crush, int change, int direction, int model)
    : DMovingCeiling(sec), m_Status(init)
{
	fixed_t targheight;

	m_Type = genCeiling;
	m_Crush = (crush ? DOOM_CRUSH : NO_CRUSH);
	m_CrushMode = crushDoom;
	m_Direction = direction ? 1 : -1;
	m_Sector = sec;
	m_Texture = sec->ceilingpic;
	m_NewSpecial = sec->special;
	m_NewDamageRate = sec->damageamount;
	m_NewDmgInterval = sec->damageinterval;
	m_NewLeakRate = sec->leakrate;
	m_NewFlags = sec->flags;
	m_Tag = sec->tag;

	// set speed of motion
	switch (speed)
	{
	case SpeedSlow:
		m_Speed = CEILSPEED;
		break;
	case SpeedNormal:
		m_Speed = CEILSPEED * 2;
		break;
	case SpeedFast:
		m_Speed = CEILSPEED * 4;
		break;
	case SpeedTurbo:
		m_Speed = CEILSPEED * 8;
		break;
	default:
		break;
	}

	// set destination target height
	targheight = sec->ceilingheight;
	switch (target)
	{
	case CtoHnC:
		targheight = P_FindHighestCeilingSurrounding(sec);
		break;
	case CtoLnC:
		targheight = P_FindLowestCeilingSurrounding(sec);
		break;
	case CtoNnC:
		targheight =
		    direction ? P_FindNextHighestCeiling(sec)
		                  : P_FindNextLowestCeiling(sec);
		break;
	case CtoHnF:
		targheight = P_FindHighestFloorSurrounding(sec);
		break;
	case CtoF:
		targheight = sec->floorheight;
		break;
	case CbyST:
		targheight =
		    (sec->ceilingheight >> FRACBITS) +
		             m_Direction * (P_FindShortestUpperAround(sec) >> FRACBITS);
		if (targheight > 32000)     // jff 3/13/98 prevent overflow
			targheight = 32000; // wraparound in ceiling height
		if (targheight < -32000)
			targheight = -32000;
		targheight <<= FRACBITS;
		break;
	case Cby24:
		targheight =
		    sec->ceilingheight + m_Direction * 24 * FRACUNIT;
		break;
	case Cby32:
		targheight =
		   sec->ceilingheight + m_Direction * 32 * FRACUNIT;
		break;
	default:
		break;
	}
	if (direction)
		m_TopHeight = targheight;
	else
		m_BottomHeight = targheight;

	// set texture/type change properties
	if (change) // if a texture change is indicated
	{
		if (model) // if a numeric model change
		{
			sector_t* chgsec = NULL;

			// jff 5/23/98 find model with floor at target height if target
			// is a floor type
			chgsec = (target == CtoHnF || target == CtoF)
			          ? P_FindModelFloorSector(targheight, sec)
			          : P_FindModelCeilingSector(targheight, sec);
			if (chgsec)
			{
				m_Texture = chgsec->ceilingpic;
				m_NewSpecial = chgsec->special;
				m_NewDamageRate = chgsec->damageamount;
				m_NewDmgInterval = chgsec->damageinterval;
				m_NewLeakRate = chgsec->leakrate;
				m_NewFlags = chgsec->flags;
				switch (change)
				{
				case CChgZero: // type is zeroed
					newspecial_s ns;
					P_ResetTransferSpecial(&ns);
					m_NewSpecial = ns.special;
					m_NewDamageRate = ns.damageamount;
					m_NewDmgInterval = ns.damageinterval;
					m_NewLeakRate = ns.damageleakrate;
					m_NewFlags = P_ResetSectorTransferFlags(chgsec->flags);
					m_Type = genCeilingChg0;
					break;
				case CChgTyp: // type is copied
					m_NewSpecial = line->frontsector->special;
					m_NewDamageRate = line->frontsector->damageamount;
					m_NewDmgInterval = line->frontsector->damageinterval;
					m_NewLeakRate = line->frontsector->leakrate;
					m_NewFlags = line->frontsector->flags;
					m_Type = genCeilingChgT;
					break;
				case CChgTxt: // type is left alone
					m_Type = genCeilingChg;
					break;
				default:
					break;
				}
			}
		}
		else // else if a trigger model change
		{
			m_Texture = line->frontsector->ceilingpic;
			m_NewSpecial = line->frontsector->special;
			m_NewDamageRate = line->frontsector->damageamount;
			m_NewDmgInterval = line->frontsector->damageinterval;
			m_NewLeakRate = line->frontsector->leakrate;
			m_NewFlags = line->frontsector->flags;
			switch (change)
			{
			case CChgZero: // type is zeroed
				newspecial_s ns;
				P_ResetTransferSpecial(&ns);
				m_NewSpecial = ns.special;
				m_NewDamageRate = ns.damageamount;
				m_NewDmgInterval = ns.damageinterval;
				m_NewLeakRate = ns.damageleakrate;
				m_NewFlags = P_ResetSectorTransferFlags(line->frontsector->flags);
				m_Type = genCeilingChg0;
				break;
			case CChgTyp: // type is copied
				m_NewSpecial = line->frontsector->special;
				m_NewDamageRate = line->frontsector->damageamount;
				m_NewDmgInterval = line->frontsector->damageinterval;
				m_NewLeakRate = line->frontsector->leakrate;
				m_NewFlags = line->frontsector->flags;
				m_Type = genCeilingChgT;
				break;
			case CChgTxt: // type is left alone
				m_Type = genCeilingChg;
				break;
			default:
				break;
			}
		}
	}
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

BOOL EV_ZDoomCeilingCrushStop(int tag, bool remove)
{
	BOOL rtn = false;
	DCeiling* scan;
	TThinkerIterator<DCeiling> iterator;

	while ((scan = iterator.Next()))
	{
		if (scan->m_Tag == tag && scan->m_Direction != 0)
		{
			S_StopSound(scan->m_Sector->soundorg);
			if (!remove)
			{
				scan->m_OldDirection = scan->m_Direction;
				scan->m_Direction = 0; // in-stasis;
			}
			else
			{
				P_SetCeilingDestroy(scan);
			}
			rtn = true;
		}
	}

	return rtn;
}

BOOL P_SpawnZDoomCeiling(DCeiling::ECeiling, line_t*, int, fixed_t,
                         fixed_t, fixed_t, int, int,
                         int, crushmode_e);

BOOL EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t* line, byte tag, fixed_t speed,
                       fixed_t speed2, fixed_t height, int crush, byte silent, int change,
                       crushmode_e crushmode)
{
		return P_SpawnZDoomCeiling(type, line, tag, speed, speed2, height, crush, silent,
		                    change, crushmode);
}

//
// P_SpawnZDoomCeiling
// Move a ceiling up/down and all around!
//
BOOL P_SpawnZDoomCeiling(DCeiling::ECeiling type, line_t* line, int tag, fixed_t speed,
                  fixed_t speed2, fixed_t height, int crush, int silent, int change, crushmode_e crushmode)
{
	int secnum;
	BOOL rtn;
	sector_t* sec;
	DCeiling* ceiling;
	BOOL manual = false;
	fixed_t targheight = 0;

	height *= FRACUNIT;

	rtn = false;

	// check if a manual trigger, if so do just the sector on the backside
	//
	if (co_boomphys && tag == 0)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		// [RH] Hack to let manual crushers be retriggerable, too
		tag ^= secnum | 0x1000000;
		P_ActivateInStasisCeiling(tag);
		goto manual_ceiling;
	}

	//	Reactivate in-stasis ceilings...for certain types.
	// This restarts a crusher after it has been stopped
	if (type == DCeiling::ceilCrushAndRaise)
	{
		P_ActivateInStasisCeiling(tag);
	}

	secnum = -1;
	// affects all sectors with the same tag as the linedef
	while ((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];
	manual_ceiling:
		// if ceiling already moving, don't start a second function on it
		if (P_CeilingActive(sec))
		{
			if (co_boomphys && manual)
				return false;
			else
				continue;
		}

		fixed_t ceilingheight = P_CeilingHeight(sec);
		fixed_t floorheight = P_FloorHeight(sec);

		// new door thinker
		rtn = true;
		ceiling = new DCeiling(sec, speed, speed2, silent);
		ceiling->m_Texture = NO_TEXTURE;
		P_AddMovingCeiling(sec);

		switch (type)
		{
		case DCeiling::ceilCrushAndRaise:
		case DCeiling::ceilCrushRaiseAndStay:
			ceiling->m_TopHeight = ceilingheight;
		case DCeiling::ceilLowerAndCrush:
			targheight = ceiling->m_BottomHeight = floorheight + height;
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToHighest:
			targheight = ceiling->m_TopHeight = P_FindHighestCeilingSurrounding(sec);
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

		case DCeiling::ceilMoveToValue: {
			int diff = height - ceilingheight;

			targheight = height;
			if (diff < 0)
			{
				targheight = ceiling->m_BottomHeight = height;
				ceiling->m_Direction = -1;
			}
			else
			{
				targheight = ceiling->m_TopHeight = height;
				ceiling->m_Direction = 1;
			}
		}
		break;

		case DCeiling::ceilLowerToHighestFloor:
			targheight = ceiling->m_BottomHeight = P_FindHighestFloorSurrounding(sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToHighestFloor:
			targheight = ceiling->m_TopHeight = P_FindHighestFloorSurrounding(sec);
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
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToNearest:
			targheight = ceiling->m_TopHeight = P_FindNextHighestCeiling(sec);
			ceiling->m_Direction = 1;
			break;

		case DCeiling::ceilLowerToLowest:
			targheight = ceiling->m_BottomHeight = P_FindLowestCeilingSurrounding(sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseToLowest:
			targheight = ceiling->m_TopHeight = P_FindLowestCeilingSurrounding(sec);
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
			targheight = ceiling->m_BottomHeight = P_FindHighestCeilingSurrounding(sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilLowerByTexture:
			targheight = ceiling->m_BottomHeight =
			    ceilingheight - P_FindShortestUpperAround(sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseByTexture:
			targheight = ceiling->m_TopHeight =
			    ceilingheight + P_FindShortestUpperAround(sec);
			ceiling->m_Direction = 1;
			break;
		}

		ceiling->m_Tag = tag;
		ceiling->m_Type = type;
		ceiling->m_Crush = crush;
		ceiling->m_CrushMode = crushmode;

		// Don't make noise for instant movement ceilings
		if (ceiling->m_Direction < 0)
		{
			if (ceiling->m_Speed >= sec->ceilingheight - ceiling->m_BottomHeight)
				if (silent & 4)
					ceiling->m_Silent = 2;
		}
		else
		{
			if (ceiling->m_Speed >= ceiling->m_TopHeight - sec->ceilingheight)
				if (silent & 4)
					ceiling->m_Silent = 2;
		}

		// set texture/type change properties
		if (change & 3) // if a texture change is indicated
		{
			if (change & 4) // if a numeric model change
			{
				// jff 5/23/98 find model with floor at target height if target
				// is a floor type
				sector_t* found = NULL;
				if (type == DCeiling::ceilRaiseToFloor ||
				    type == DCeiling::ceilLowerToFloor)
				{
					found = P_FindModelFloorSector(targheight, sec);
				}
				else
				{
					found = P_FindModelCeilingSector(targheight, sec);
				}

				if (found != NULL)
				{
					ceiling->m_Texture = found->ceilingpic;
					switch (change & 3)
					{
					case 1: // type is zeroed
						ceiling->m_NewSpecial = 0;
						ceiling->m_Type = DCeiling::genCeilingChg0;
						break;
					case 2: // type is copied
						ceiling->m_NewSpecial = found->special;
						ceiling->m_Type = DCeiling::genCeilingChgT;
						break;
					case 3: // type is left alone
						ceiling->m_Type = DCeiling::genCeilingChg;
						break;
					}
				}
			}
			else if (line != NULL) // else if a trigger model change
			{
				ceiling->m_Texture = line->frontsector->ceilingpic;
				switch (change & 3)
				{
				case 1: // type is zeroed
					ceiling->m_NewSpecial = 0;
					ceiling->m_Type = DCeiling::genCeilingChg0;
					break;
				case 2: // type is copied
					ceiling->m_NewSpecial = line->frontsector->special;
					ceiling->m_Type = DCeiling::genCeilingChgT;
					break;
				case 3: // type is left alone
					ceiling->m_Type = DCeiling::genCeilingChg;
					break;
				}
			}
		}

		ceiling->PlayCeilingSound();
		P_AddMovingCeiling(sec);

		if (manual)
			return rtn;
	}
	return rtn;
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
	if (type == DCeiling::crushAndRaise)
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
		case DCeiling::fastCrushAndRaise:
		case DCeiling::crushAndRaise:
		case DCeiling::silentCrushAndRaise:
		case DCeiling::ceilCrushRaiseAndStay:
			ceiling->m_TopHeight = ceilingheight;
		case DCeiling::lowerAndCrush:
			ceiling->m_Crush = crush ? DOOM_CRUSH : NO_CRUSH;
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
			ceiling->m_Direction = -1;
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
				ceilingheight - P_FindShortestUpperAround (sec);
			ceiling->m_Direction = -1;
			break;

		case DCeiling::ceilRaiseByTexture:
			targheight = ceiling->m_TopHeight =
				ceilingheight + P_FindShortestUpperAround (sec);
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
			if (change & 4) // if a numeric model change
			{
				// jff 5/23/98 find model with floor at target height if target
				// is a floor type
				sector_t* found = NULL;
				if (type == DCeiling::ceilRaiseToFloor ||
				    type == DCeiling::ceilLowerToFloor)
				{
					found = P_FindModelFloorSector(targheight, sec);
				}
				else
				{
					found = P_FindModelCeilingSector(targheight, sec);
				}

				if (found != NULL)
				{
					ceiling->m_Texture = found->ceilingpic;
					switch (change & 3)
					{
					case 1: // type is zeroed
						ceiling->m_NewSpecial = 0;
						ceiling->m_Type = DCeiling::genCeilingChg0;
						break;
					case 2: // type is copied
						ceiling->m_NewSpecial = found->special;
						ceiling->m_Type = DCeiling::genCeilingChgT;
						break;
					case 3: // type is left alone
						ceiling->m_Type = DCeiling::genCeilingChg;
						break;
					}
				}
			}
			else if (line != NULL) // else if a trigger model change
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
// EV_DoGenCeiling()
//
// Handle generalized ceiling types
//
// Passed the linedef activating the ceiling function
// Returns true if a thinker created
//
// jff 02/04/98 Added this routine (and file) to handle generalized
// floor movers using bit fields in the line special type.
//
BOOL EV_DoGenCeiling(line_t* line)
{
	int secnum;
	BOOL rtn;
	BOOL manual;
	sector_t* sec;
	unsigned value = (unsigned)line->special - GenCeilingBase;

	// parse the bit fields in the line's special type

	int Crsh = (value & CeilingCrush) >> CeilingCrushShift;
	int ChgT = (value & CeilingChange) >> CeilingChangeShift;
	int Targ = (value & CeilingTarget) >> CeilingTargetShift;
	int Dirn = (value & CeilingDirection) >> CeilingDirectionShift;
	int ChgM = (value & CeilingModel) >> CeilingModelShift;
	int Sped = (value & CeilingSpeed) >> CeilingSpeedShift;
	int Trig = (value & TriggerType) >> TriggerTypeShift;

	rtn = false;

	// check if a manual trigger, if so do just the sector on the backside
	manual = false;
	if (Trig == PushOnce || Trig == PushMany)
	{
		if (!(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_genceiling;
	}

	secnum = -1;
	// if not manual do all sectors tagged the same as the line
	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
	manual_genceiling:
		sec = &sectors[secnum];
		// Do not start another function if ceiling already moving
		if (sec->ceilingdata) // jff 2/22/98
		{
			if (!manual)
				continue;
			else
				return rtn;
		}

		// new ceiling thinker
		rtn = true;

		new DCeiling(sec, line, Sped, Targ, Crsh, ChgT, Dirn, ChgM);
		P_AddMovingCeiling(sec); // add this ceiling to the active list
		if (manual)
			return rtn;
	}
	return rtn;
}

//
// EV_DoGenCrusher()
//
// Handle generalized crusher types
//
// Passed the linedef activating the crusher function
// Returns true if a thinker created
//
// jff 02/04/98 Added this routine (and file) to handle generalized
// floor movers using bit fields in the line special type.
//
BOOL EV_DoGenCrusher(line_t* line)
{
	int secnum;
	BOOL rtn;
	BOOL manual;
	sector_t* sec;
	unsigned value = (unsigned)line->special - GenCrusherBase;

	// parse the bit fields in the line's special type

	int Slnt = (value & CrusherSilent) >> CrusherSilentShift;
	int Sped = (value & CrusherSpeed) >> CrusherSpeedShift;
	int Trig = (value & TriggerType) >> TriggerTypeShift;

	rtn = false;

	P_ActivateInStasisCeiling(line->id);

	// check if a manual trigger, if so do just the sector on the backside
	manual = false;
	if (Trig == PushOnce || Trig == PushMany)
	{
		if (!(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_gencrusher;
	}

	secnum = -1;
	// if not manual do all sectors tagged the same as the line
	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
	manual_gencrusher:
		sec = &sectors[secnum];
		// Do not start another function if ceiling already moving
		if (sec->ceilingdata) // jff 2/22/98
		{
			if (!manual)
				continue;
			else
				return rtn;
		}

		// new ceiling thinker
		rtn = true;

		new DCeiling(sec, line, Slnt, Sped);
		P_AddMovingCeiling(sec); // add this ceiling to the active list
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
