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
//		Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "r_state.h"
#include "tables.h"

void P_ResetTransferSpecial(newspecial_s* newspecial);
unsigned int P_ResetSectorTransferFlags(const unsigned int flags);

EXTERN_CVAR(co_boomphys)

extern bool predicting;

// ============================================================================
// FLOORS
// ============================================================================

void P_SetFloorDestroy(DFloor *floor)
{
	if (!floor || predicting)
		return;

	floor->m_Status = DFloor::destroy;

	if (clientside && floor->m_Sector)
	{
		floor->m_Sector->floordata = NULL;
		floor->Destroy();
	}
}

IMPLEMENT_SERIAL (DFloor, DMovingFloor)

DFloor::DFloor () :
	m_Status(init)
{
}

void DFloor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_Status
			<< m_Crush
			<< m_Direction
			<< m_HexenCrush
			<< m_NewSpecial
			<< m_NewDamageRate
			<< m_NewDmgInterval
			<< m_NewFlags
			<< m_NewLeakRate
			<< m_Texture
			<< m_FloorDestHeight
			<< m_Speed
			<< m_ResetCount
			<< m_OrgHeight
			<< m_Delay
			<< m_PauseTime
			<< m_StepTime
			<< m_PerStepTime;
	}
	else
	{
		arc >> m_Type
			>> m_Status
			>> m_Crush
			>> m_Direction
			>> m_HexenCrush
			>> m_NewSpecial
			>> m_NewDamageRate
			>> m_NewDmgInterval
			>> m_NewFlags
			>> m_NewLeakRate
			>> m_Texture
			>> m_FloorDestHeight
			>> m_Speed
			>> m_ResetCount
			>> m_OrgHeight
			>> m_Delay
			>> m_PauseTime
			>> m_StepTime
			>> m_PerStepTime;
	}
}

void DFloor::PlayFloorSound()
{
	if (predicting || !m_Sector)
		return;

	S_StopSound(m_Sector->soundorg);

	if (m_Status == init)
		S_LoopedSound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_mid", 1, ATTN_NORM);
	if (m_Status == finished)
		S_Sound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_stop", 1, ATTN_NORM);
}

//
// MOVE A FLOOR TO IT'S DESTINATION (UP OR DOWN)
//
void DFloor::RunThink ()
{
	if (m_Status == finished)
	{
		PlayFloorSound();
		P_SetFloorDestroy(this);
	}

	if (m_Status == destroy)
		return;

	// [RH] Handle resetting stairs
	if (m_Type == buildStair || m_Type == waitStair)
	{
		if (m_ResetCount)
		{
			if (--m_ResetCount == 0)
			{
				m_Type = resetStair;
				m_Direction = (m_Direction > 0) ? -1 : 1;
				m_FloorDestHeight = m_OrgHeight;
			}
		}
		if (m_PauseTime)
		{
			m_PauseTime--;
			return;
		}
		else if (m_StepTime)
		{
			if (--m_StepTime == 0)
			{
				m_PauseTime = m_Delay;
				m_StepTime = m_PerStepTime;
			}
		}
	}

	if (m_Type == waitStair)
		return;

	EResult res = MoveFloor (m_Speed, m_FloorDestHeight, m_Crush, m_Direction, m_HexenCrush);

	if (res == pastdest)
	{
		m_Status = finished;
		PlayFloorSound();

		if (m_Type == buildStair)
			m_Type = waitStair;

		if (m_Type != waitStair || m_ResetCount == 0)
		{
			if (m_Direction == 1)
			{
				switch (m_Type)
				{
				case donutRaise:
				case genFloorChgT:
				case genFloorChg0:
					m_Sector->special = m_NewSpecial;
					m_Sector->flags = m_NewFlags;
					m_Sector->damageamount = m_NewDamageRate;
					m_Sector->damageinterval = m_NewDmgInterval;
					m_Sector->leakrate = m_NewLeakRate;
					//fall thru
				case genFloorChg:
					m_Sector->floorpic = m_Texture;
					break;
				default:
					break;
				}
			}
			else if (m_Direction == -1)
			{
				switch(m_Type)
				{
				case floorLowerAndChange:
				case genFloorChgT:
				case genFloorChg0:
					m_Sector->special = m_NewSpecial;
					m_Sector->flags = m_NewFlags;
					m_Sector->damageamount = m_NewDamageRate;
					m_Sector->damageinterval = m_NewDmgInterval;
					m_Sector->leakrate = m_NewLeakRate;
					//fall thru
				case genFloorChg:
					m_Sector->floorpic = m_Texture;
					break;
				default:
					break;
				}
			}

			//jff 2/26/98 implement stair retrigger lockout while still building
			// note this only applies to the retriggerable generalized stairs

			if (m_Sector->stairlock == -2)		// if this sector is stairlocked
			{
				sector_t *sec = m_Sector;
				sec->stairlock = -1;				// thinker done, promote lock to -1

				while (sec->prevsec != -1 && sectors[sec->prevsec].stairlock != -2)
					sec = &sectors[sec->prevsec];	// search for a non-done thinker
				if (sec->prevsec == -1)				// if all thinkers previous are done
				{
					sec = m_Sector;			// search forward
					while (sec->nextsec != -1 && sectors[sec->nextsec].stairlock != -2)
						sec = &sectors[sec->nextsec];
					if (sec->nextsec == -1)			// if all thinkers ahead are done too
					{
						while (sec->prevsec != -1)	// clear all locks
						{
							sec->stairlock = 0;
							sec = &sectors[sec->prevsec];
						}
						sec->stairlock = 0;
					}
				}
			}
		}

		P_SetFloorDestroy(this);
	}
}

DFloor::DFloor (sector_t *sec)
	: DMovingFloor (sec), m_Status(init)
{
}

DFloor::DFloor(sector_t* sec, DFloor::EFloor floortype, line_t* line, fixed_t speed,
               fixed_t height, int crush, int change, bool hexencrush, bool hereticlower)
    : DMovingFloor(sec), m_Status(init)
{
	fixed_t floorheight = P_FloorHeight(sec);
	fixed_t ceilingheight = P_CeilingHeight(sec);

	m_Type = floortype;
	m_Crush = crush;
	m_HexenCrush = hexencrush;
	m_Speed = speed;
	m_ResetCount = 0; // [RH]
	m_Direction = 1;
	m_OrgHeight = floorheight;
	m_Height = height;
	m_Change = change;
	m_Line = line;

	PlayFloorSound();

	switch (floortype)
	{
	case DFloor::floorLowerToHighest:
		m_Direction = -1;
		m_FloorDestHeight = P_FindHighestFloorSurrounding(sec);
		// [RH] DOOM's turboLower type did this. I've just extended it
		//		to be applicable to all LowerToHighest types.
		if (hereticlower || m_FloorDestHeight != floorheight)
			m_FloorDestHeight += height;
		break;

	case DFloor::floorLowerToLowest:
		m_Direction = -1;
		m_FloorDestHeight = P_FindLowestFloorSurrounding(sec);
		break;

	case DFloor::floorLowerToNearest:
		// jff 02/03/30 support lowering floor to next lowest floor
		m_Direction = -1;
		m_FloorDestHeight = P_FindNextLowestFloor(sec);
		break;

	case DFloor::floorLowerInstant:
		m_Speed = height;
	case DFloor::floorLowerByValue:
		m_Direction = -1;
		m_FloorDestHeight = floorheight - height;
		break;

	case DFloor::floorRaiseInstant:
		m_Speed = height;
	case DFloor::floorRaiseByValue:
		m_Direction = 1;
		m_FloorDestHeight = floorheight + height;
		break;

	case DFloor::floorMoveToValue:
		m_FloorDestHeight = height;
		m_Direction = (m_FloorDestHeight > floorheight) ? 1 : -1;
		break;

	case DFloor::floorRaiseAndCrushDoom:
		height = 8 * FRACUNIT;
	case DFloor::floorRaiseToLowestCeiling:
		m_Direction = 1;
		m_FloorDestHeight = P_FindLowestCeilingSurrounding(sec);
		if (m_FloorDestHeight > ceilingheight)
			m_FloorDestHeight = ceilingheight - height;
		break;

	case DFloor::floorRaiseToHighest:
		m_Direction = 1;
		m_FloorDestHeight = P_FindHighestFloorSurrounding(sec);
		break;

	case DFloor::floorRaiseToNearest:
		m_Direction = 1;
		m_FloorDestHeight = P_FindNextHighestFloor(sec);
		break;

	case DFloor::floorRaiseToLowest:
		m_Direction = 1;
		m_FloorDestHeight = P_FindLowestFloorSurrounding(sec);
		break;

	case DFloor::floorRaiseAndCrush:
		height = 8 * FRACUNIT;
	case DFloor::floorRaiseToCeiling:
		m_Direction = 1;
		m_FloorDestHeight = ceilingheight - height;
		break;

	case DFloor::floorLowerToLowestCeiling:
		m_Direction = -1;
		m_FloorDestHeight = P_FindLowestCeilingSurrounding(sec);
		break;

	case DFloor::floorLowerByTexture:
		m_Direction = -1;
		m_FloorDestHeight = floorheight - P_FindShortestTextureAround(sec);
		break;

	case DFloor::floorLowerToCeiling:
		// [RH] Essentially instantly raises the floor to the ceiling
		m_Direction = -1;
		m_FloorDestHeight = ceilingheight - height;
		break;

	case DFloor::floorRaiseByTexture:
		m_Direction = 1;
		// [RH] Use P_FindShortestTextureAround from BOOM to do this
		//		since the code is identical to what was here. (Oddly
		//		enough, BOOM preserved the code here even though it
		//		also had this function.)
		m_FloorDestHeight = floorheight + P_FindShortestTextureAround(sec);
		break;

	case DFloor::floorRaiseAndChange:
		m_Direction = 1;
		m_FloorDestHeight = floorheight + height;
		if (line)
		{
			sec->floorpic = line->frontsector->floorpic;
			P_CopySectorSpecial(sec, line->frontsector);
		}
		else
		{
			P_ResetSectorSpecial(sec);
		}
		break;

	case DFloor::floorLowerAndChange:
		m_Direction = -1;
		m_FloorDestHeight = P_FindLowestFloorSurrounding(sec);
		m_Texture = sec->floorpic;
		// jff 1/24/98 make sure m_NewSpecial gets initialized
		// in case no surrounding sector is at floordestheight
		// --> should not affect compatibility <--
		m_NewSpecial = sec->special;
		m_NewDamageRate = sec->damageamount;
		m_NewDmgInterval = sec->damageinterval;
		m_NewLeakRate = sec->leakrate;
		m_NewFlags = sec->flags;

		// jff 5/23/98 use model subroutine to unify fixes and handling
		sec = P_FindModelFloorSector(m_FloorDestHeight, sec);
		if (sec)
		{
			m_Texture = sec->floorpic;
			m_NewSpecial = sec->special;
			m_NewDamageRate = sec->damageamount;
			m_NewDmgInterval = sec->damageinterval;
			m_NewLeakRate = sec->leakrate;
			m_NewFlags = sec->flags;
		}
		break;

	default:
		break;
	}

	if (m_Direction == 1)
		m_Status = up;
	else if (m_Direction == -1)
		m_Status = down;

	if (change & 3)
	{
		// [RH] Need to do some transferring
		if (change & 4)
		{
			// Numeric model change
			sector_t* found = NULL;
			if (floortype == DFloor::floorRaiseToLowestCeiling ||
			    floortype == DFloor::floorLowerToLowestCeiling ||
			    floortype == DFloor::floorRaiseToCeiling ||
			    floortype == DFloor::floorLowerToCeiling)
			{
				found = P_FindModelCeilingSector(m_FloorDestHeight, sec);
			}
			else
			{
				found = P_FindModelFloorSector(m_FloorDestHeight, sec);
			}

			if (found != NULL)
			{
				m_Texture = found->floorpic;
				switch (change & 3)
				{
				case 1:
					newspecial_s ns;
					P_ResetTransferSpecial(&ns);
					m_NewSpecial = ns.special;
					m_NewDamageRate = ns.damageamount;
					m_NewDmgInterval = ns.damageinterval;
					m_NewLeakRate = ns.damageleakrate;
					m_NewFlags = P_ResetSectorTransferFlags(found->flags);
					m_Type = DFloor::genFloorChg0;
					break;
				case 2:
					m_Type = DFloor::genFloorChg;
					break;
				case 3:
					m_NewSpecial = found->special;
					m_NewDamageRate = found->damageamount;
					m_NewDmgInterval = found->damageinterval;
					m_NewLeakRate = found->leakrate;
					m_NewFlags = found->flags;
					m_Type = DFloor::genFloorChgT;
					break;
				}
			}
		}
		else if (line != NULL)
		{
			// Trigger model change
			m_Texture = line->frontsector->floorpic;

			switch (change & 3)
			{
			case 1:
				newspecial_s ns;
				P_ResetTransferSpecial(&ns);
				m_NewSpecial = ns.special;
				m_NewDamageRate = ns.damageamount;
				m_NewDmgInterval = ns.damageinterval;
				m_NewLeakRate = ns.damageleakrate;
				m_NewFlags = P_ResetSectorTransferFlags(line->frontsector->flags);
				m_Type = DFloor::genFloorChg0;
				break;
			case 2:
				m_Type = DFloor::genFloorChg;
				break;
			case 3:
				m_NewSpecial = line->frontsector->special;
				m_NewDamageRate = line->frontsector->damageamount;
				m_NewDmgInterval = line->frontsector->damageinterval;
				m_NewLeakRate = line->frontsector->leakrate;
				m_NewFlags = line->frontsector->flags;
				m_Type = DFloor::genFloorChgT;
				break;
			}
		}
	}
}

// Generalized floor init
DFloor::DFloor(sector_t* sec, line_t* line, int speed,
               int target, int crush, int change, int direction, int model)
    : DMovingFloor(sec), m_Status(init)
{
	fixed_t floorheight = P_FloorHeight(sec);

	m_Type = genFloor;
	m_Crush = crush ? DOOM_CRUSH : NO_CRUSH;
	m_HexenCrush = false;
	m_ResetCount = 0; // [RH]
	m_Sector = sec;
	m_OrgHeight = floorheight;
	m_Height = floorheight;
	m_Line = line;
	m_Direction = direction ? 1 : -1;
	m_NewSpecial = sec->special;
	m_NewDamageRate = sec->damageamount;
	m_NewDmgInterval = sec->damageinterval;
	m_NewLeakRate = sec->leakrate;
	m_NewFlags = sec->flags;

	if (m_Direction == 1)
		m_Status = up;
	else if (m_Direction == -1)
		m_Status = down;

	PlayFloorSound();

	switch (speed)
	{
	case SpeedSlow:
		m_Speed = FLOORSPEED;
		break;
	case SpeedNormal:
		m_Speed = FLOORSPEED * 2;
		break;
	case SpeedFast:
		m_Speed = FLOORSPEED * 4;
		break;
	case SpeedTurbo:
		m_Speed = FLOORSPEED * 8;
		break;
	default:
		break;
	}

	// set the destination height
	switch (target)
	{
	case FtoHnF:
		m_FloorDestHeight = P_FindHighestFloorSurrounding(sec);
		break;
	case FtoLnF:
		m_FloorDestHeight = P_FindLowestFloorSurrounding(sec);
		break;
	case FtoNnF:
		m_FloorDestHeight =
		    direction ? P_FindNextHighestFloor(sec) : P_FindNextLowestFloor(sec);
		break;
	case FtoLnC:
		m_FloorDestHeight = P_FindLowestCeilingSurrounding(sec);
		break;
	case FtoC:
		m_FloorDestHeight = sec->ceilingheight;
		break;
	case FbyST:
		m_FloorDestHeight =
		    (sec->floorheight >> FRACBITS) +
		    m_Direction * (P_FindShortestTextureAround(sec) >> FRACBITS);
		if (m_FloorDestHeight > 32000)      // jff 3/13/98 prevent overflow
			m_FloorDestHeight = 32000; // wraparound in floor height
		if (m_FloorDestHeight < -32000)
			m_FloorDestHeight = -32000;
		m_FloorDestHeight <<= FRACBITS;
		break;
	case Fby24:
		m_FloorDestHeight =
		    sec->floorheight + m_Direction * 24 * FRACUNIT;
		break;
	case Fby32:
		m_FloorDestHeight = sec->floorheight + m_Direction * 32 * FRACUNIT;
		break;
	default:
		break;
	}

	// set texture/type change properties
	if (change) // if a texture change is indicated
	{
		if (model) // if a numeric model change
		{
			sector_t* chgsec = NULL;

			// jff 5/23/98 find model with ceiling at target height if target
			// is a ceiling type
			chgsec = (target == FtoLnC || target == FtoC)
			          ? P_FindModelCeilingSector(m_FloorDestHeight, sec)
			          : P_FindModelFloorSector(m_FloorDestHeight, sec);
			if (chgsec)
			{
				m_Texture = chgsec->floorpic;
				m_NewSpecial = chgsec->special;
				m_NewDamageRate = chgsec->damageamount;
				m_NewDmgInterval = chgsec->damageinterval;
				m_NewLeakRate = chgsec->leakrate;
				m_NewFlags = chgsec->flags;
				switch (change)
				{
				case FChgZero: // zero type
					newspecial_s ns;
					P_ResetTransferSpecial(&ns);
					m_NewSpecial = ns.special;
					m_NewDamageRate = ns.damageamount;
					m_NewDmgInterval = ns.damageinterval;
					m_NewLeakRate = ns.damageleakrate;
					m_NewFlags = P_ResetSectorTransferFlags(chgsec->flags);
					m_Type = genFloorChg0;
					break;
				case FChgTyp: // copy type
					m_NewSpecial = chgsec->special;
					m_NewDamageRate = chgsec->damageamount;
					m_NewDmgInterval = chgsec->damageinterval;
					m_NewLeakRate = chgsec->leakrate;
					m_NewFlags = chgsec->flags;
					m_Type = genFloorChgT;
					break;
				case FChgTxt: // leave type be
					m_Type = genFloorChg;
					break;
				default:
					break;
				}
			}
		}
		else // else if a trigger model change
		{
			m_Texture = line->frontsector->floorpic;
			m_NewSpecial = line->frontsector->special;
			m_NewDamageRate = line->frontsector->damageamount;
			m_NewDmgInterval = line->frontsector->damageinterval;
			m_NewLeakRate = line->frontsector->leakrate;
			m_NewFlags = line->frontsector->flags;
			switch (change)
			{
			case FChgZero: // zero type
				newspecial_s ns;
				P_ResetTransferSpecial(&ns);
				m_NewSpecial = ns.special;
				m_NewDamageRate = ns.damageamount;
				m_NewDmgInterval = ns.damageinterval;
				m_NewLeakRate = ns.damageleakrate;
				m_NewFlags = P_ResetSectorTransferFlags(line->frontsector->flags);
				m_Type = genFloorChg0;
				break;
			case FChgTyp: // copy type
				m_NewSpecial = line->frontsector->special;
				m_NewDamageRate = line->frontsector->damageamount;
				m_NewDmgInterval = line->frontsector->damageinterval;
				m_NewLeakRate = line->frontsector->leakrate;
				m_NewFlags = line->frontsector->flags;
				m_Type = genFloorChgT;
				break;
			case FChgTxt: // leave type be
				m_Type = genFloorChg;
			default:
				break;
			}
		}
	}
}

DFloor::DFloor(sector_t *sec, DFloor::EFloor floortype, line_t *line,
			   fixed_t speed, fixed_t height, bool crush, int change)
	: DMovingFloor (sec), m_Status(init)
{
	fixed_t floorheight = P_FloorHeight(sec);
	fixed_t ceilingheight = P_CeilingHeight(sec);

	m_Type = floortype;
	m_Crush = crush ? DOOM_CRUSH : NO_CRUSH;
	m_HexenCrush = false;
	m_Speed = speed;
	m_ResetCount = 0;				// [RH]
	m_Direction = 1;
	m_OrgHeight = floorheight;
	m_Height = height;
	m_Change = change;
	m_Line = line;

	PlayFloorSound();

	switch (floortype)
	{
	case DFloor::floorLowerToHighest:
		m_Direction = -1;
		m_FloorDestHeight = P_FindHighestFloorSurrounding(sec);
		// [RH] DOOM's turboLower type did this. I've just extended it
		//		to be applicable to all LowerToHighest types.
		if (m_FloorDestHeight != floorheight)
			m_FloorDestHeight += height;
		break;

	case DFloor::floorLowerToLowest:
		m_Direction = -1;
		m_FloorDestHeight = P_FindLowestFloorSurrounding(sec);
		break;

	case DFloor::floorLowerToNearest:
		//jff 02/03/30 support lowering floor to next lowest floor
		m_Direction = -1;
		m_FloorDestHeight = P_FindNextLowestFloor(sec);
		break;

	case DFloor::floorLowerInstant:
		m_Speed = height;
	case DFloor::floorLowerByValue:
		m_Direction = -1;
		m_FloorDestHeight = floorheight - height;
		break;

	case DFloor::floorRaiseInstant:
		m_Speed = height;
	case DFloor::floorRaiseByValue:
		m_Direction = 1;
		m_FloorDestHeight = floorheight + height;
		break;

	case DFloor::floorMoveToValue:
		m_Direction = (height - floorheight) > 0 ? 1 : -1;
		m_FloorDestHeight = height;
		break;

	case DFloor::floorRaiseAndCrush:
		m_Crush = crush;
	case DFloor::floorRaiseToLowestCeiling:
		m_Direction = 1;
		m_FloorDestHeight =
			P_FindLowestCeilingSurrounding(sec);
		if (m_FloorDestHeight > ceilingheight)
			m_FloorDestHeight = ceilingheight;
		if (floortype == DFloor::floorRaiseAndCrush)
			m_FloorDestHeight -= 8 * FRACUNIT;
		break;

	case DFloor::floorRaiseToHighest:
		m_Direction = 1;
		m_FloorDestHeight = P_FindHighestFloorSurrounding(sec);
		break;

	case DFloor::floorRaiseToNearest:
		m_Direction = 1;
		m_FloorDestHeight = P_FindNextHighestFloor(sec);
		break;

	case DFloor::floorRaiseToLowest:
		m_Direction = 1;
		m_FloorDestHeight = P_FindLowestFloorSurrounding(sec);
		break;

	case DFloor::floorRaiseToCeiling:
		m_Direction = 1;
		m_FloorDestHeight = ceilingheight;
		break;

	case DFloor::floorLowerToLowestCeiling:
		m_Direction = -1;
		m_FloorDestHeight = P_FindLowestCeilingSurrounding(sec);
		break;

	case DFloor::floorLowerByTexture:
		m_Direction = -1;
		m_FloorDestHeight = floorheight - P_FindShortestTextureAround (sec);
		break;

	case DFloor::floorLowerToCeiling:
		// [RH] Essentially instantly raises the floor to the ceiling
		m_Direction = -1;
		m_FloorDestHeight = ceilingheight;
		break;

	case DFloor::floorRaiseByTexture:
		m_Direction = 1;
		// [RH] Use P_FindShortestTextureAround from BOOM to do this
		//		since the code is identical to what was here. (Oddly
		//		enough, BOOM preserved the code here even though it
		//		also had this function.)
		m_FloorDestHeight = floorheight +
			P_FindShortestTextureAround (sec);
		break;

	case DFloor::floorRaiseAndChange:
		m_Direction = 1;
		m_FloorDestHeight = floorheight + height;
		if (line)
		{
			sec->floorpic = line->frontsector->floorpic;
			sec->special = line->frontsector->special;
			sec->flags = line->frontsector->flags;
			sec->damageinterval = line->frontsector->damageinterval;
			sec->leakrate = line->frontsector->leakrate;
			sec->damageamount = line->frontsector->damageamount;
		}
		break;

	case DFloor::floorLowerAndChange:
		m_Direction = -1;
		m_FloorDestHeight = P_FindLowestFloorSurrounding (sec);
		m_Texture = sec->floorpic;
		// jff 1/24/98 make sure m_NewSpecial gets initialized
		// in case no surrounding sector is at floordestheight
		// --> should not affect compatibility <--
		m_NewSpecial = sec->special;
		m_NewFlags = sec->flags;
		m_NewDmgInterval = sec->damageinterval;
		m_NewLeakRate = sec->leakrate;
		m_NewDamageRate = sec->damageamount;

		//jff 5/23/98 use model subroutine to unify fixes and handling
		sec = P_FindModelFloorSector (m_FloorDestHeight,sec);
		if (sec)
		{
			m_Texture = sec->floorpic;
			m_NewSpecial = sec->special;
			m_NewFlags = sec->flags;
			m_NewDmgInterval = sec->damageinterval;
			m_NewLeakRate = sec->leakrate;
			m_NewDamageRate = sec->damageamount;
		}
		break;

	  default:
		break;
	}

	if (m_Direction == 1)
		m_Status = up;
	else if (m_Direction == -1)
		m_Status = down;

	if (change & 3)
	{
		// [RH] Need to do some transferring
		if (change & 4)
		{
			// Numeric model change
			sector_t* found = NULL;
			if (floortype == DFloor::floorRaiseToLowestCeiling ||
			    floortype == DFloor::floorLowerToLowestCeiling ||
			    floortype == DFloor::floorRaiseToCeiling ||
			    floortype == DFloor::floorLowerToCeiling)
			{
				found = P_FindModelCeilingSector(m_FloorDestHeight, sec);
			}
			else
			{
				found = P_FindModelFloorSector(m_FloorDestHeight, sec);
			}

			if (found != NULL)
			{
				m_Texture = found->floorpic;
				switch (change & 3)
				{
				case 1:
					m_NewSpecial = 0;
					m_NewFlags = 0;
					m_NewDmgInterval = 0;
					m_NewLeakRate = 0;
					m_NewDamageRate = 0;
					m_Type = DFloor::genFloorChg0;
					break;
				case 2:
					m_NewSpecial = found->special;
					m_NewFlags = found->flags;
					m_NewDmgInterval = found->damageinterval;
					m_NewLeakRate = found->leakrate;
					m_NewDamageRate = found->damageamount;
					m_Type = DFloor::genFloorChgT;
					break;
				case 3:
					m_Type = DFloor::genFloorChg;
					break;
				}
			}
		}
		else if (line != NULL)
		{
			// Trigger model change
			m_Texture = line->frontsector->floorpic;

			switch (change & 3) {
				case 1:
					m_NewSpecial = 0;
				    m_NewFlags = 0;
					m_Type = DFloor::genFloorChg0;
					break;
				case 2:
				    m_NewSpecial = line->frontsector->special;
				    m_NewFlags = line->frontsector->flags;
				    m_NewDmgInterval = line->frontsector->damageinterval;
				    m_NewLeakRate = line->frontsector->leakrate;
				    m_NewDamageRate = line->frontsector->damageamount;
					m_Type = DFloor::genFloorChgT;
					break;
				case 3:
					m_Type = DFloor::genFloorChg;
					break;
			}
		}
	}
}

// Clones a DFloor and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DFloor* DFloor::Clone(sector_t* sec) const
{
	DFloor* dfloor = new DFloor(*this);

	dfloor->Orphan();
	dfloor->m_Sector = sec;

	return dfloor;
}

//
// HANDLE FLOOR TYPES
// [RH] Added tag, speed, height, crush, change params.
//
BOOL EV_DoFloor (DFloor::EFloor floortype, line_t *line, int tag,
				 fixed_t speed, fixed_t height, bool crush, int change)
{
	int 				secnum;
	BOOL 				rtn = false;
	sector_t*			sec;
	BOOL				manual = false;

	// check if a manual trigger; if so do just the sector on the backside
	if (co_boomphys && tag == 0)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec-sectors;
		manual = true;
		goto manual_floor;
	}

	secnum = -1;
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

manual_floor:
		// ALREADY MOVING?	IF SO, KEEP GOING...
		if (sec->floordata || (demoplayback && sec->ceilingdata))
		{
			if (co_boomphys && manual)
				return false;
			else
				continue;
		}

		// new floor thinker
		rtn = true;
		new DFloor(sec, floortype, line, speed, height, crush, change);
		P_AddMovingFloor(sec);

		if (manual)
			return rtn;
	}
	return rtn;
}

//
// EV_DoGenFloor()
//
// Handle generalized floor types
//
// Passed the line activating the generalized floor function
// Returns true if a thinker is created
//
// jff 02/04/98 Added this routine (and file) to handle generalized
// floor movers using bit fields in the line special type.
//
BOOL EV_DoGenFloor(line_t* line)
{
	int secnum;
	BOOL rtn;
	bool manual;
	sector_t* sec;
	unsigned value = (unsigned)line->special - GenFloorBase;

	// parse the bit fields in the line's special type

	int Crsh = (value & FloorCrush) >> FloorCrushShift;
	int ChgT = (value & FloorChange) >> FloorChangeShift;
	int Targ = (value & FloorTarget) >> FloorTargetShift;
	int Dirn = (value & FloorDirection) >> FloorDirectionShift;
	int ChgM = (value & FloorModel) >> FloorModelShift;
	int Sped = (value & FloorSpeed) >> FloorSpeedShift;
	int Trig = (value & TriggerType) >> TriggerTypeShift;

	rtn = false;
	manual = false;

	// check if a manual trigger; if so do just the sector on the backside
	if (Trig == PushOnce || Trig == PushMany)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_genfloor;
	}

	secnum = -1;
	while ((secnum = P_FindSectorFromTag(line->id, secnum)) >= 0)
	{
	manual_genfloor:
		sec = &sectors[secnum];
		// ALREADY MOVING?	IF SO, KEEP GOING...
		if (sec->floordata)
		{
			if (co_boomphys && manual)
				return false;
			else
				continue;
		}

		// new floor thinker
		rtn = true;
		new DFloor(sec, line, Sped, Targ, Crsh, ChgT, Dirn, ChgM);
		P_AddMovingFloor(sec);

		if (manual)
			return rtn;
	}
	return rtn;
}

BOOL EV_DoZDoomFloor(DFloor::EFloor floortype, line_t* line, int tag, fixed_t speed,
                fixed_t height, int crush, int change, bool hexencrush, bool hereticlower)
{
	int secnum;
	BOOL rtn = false;
	sector_t* sec;
	bool manual = false;

	height *= FRACUNIT;

	// check if a manual trigger; if so do just the sector on the backside
	if (co_boomphys && tag == 0)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_floor;
	}

	secnum = -1;
	while ((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

	manual_floor:
		// ALREADY MOVING?	IF SO, KEEP GOING...
		if (sec->floordata)
		{
			if (co_boomphys && manual)
				return false;
			else
				continue;
		}

		// new floor thinker
		rtn = true;
		new DFloor(sec, floortype, line, speed, height, crush, change, hexencrush, hereticlower);
		P_AddMovingFloor(sec);

		if (manual)
			return rtn;
	}
	return rtn;
}

//
// EV_DoChange()
//
// Handle pure change types. These change floor texture and sector type
// by trigger or numeric model without moving the floor.
//
// The linedef causing the change and the type of change is passed
// Returns true if any sector changes
//
// jff 3/15/98 added to better support generalized sector types
// [RH] Added tag parameter.
//
BOOL EV_DoChange (line_t *line, EChange changetype, int tag)
{
	int			secnum;
	BOOL		rtn;
	sector_t	*sec;
	sector_t	*secm;

	secnum = -1;
	rtn = false;
	// change all sectors with the same tag as the linedef
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		rtn = true;

		// handle trigger or numeric change type
		switch(changetype)
		{
		case trigChangeOnly:
			if (line) { // [RH] if no line, no change
				sec->floorpic = line->frontsector->floorpic;
				sec->special = line->frontsector->special;
			}
			break;
		case numChangeOnly:
			secm = P_FindModelFloorSector(P_FloorHeight(sec), sec);
			if (secm) // if no model, no change
			{
				sec->floorpic = secm->floorpic;
				sec->special = secm->special;
			}
			break;
		default:
			break;
		}
	}
	return rtn;
}

//
// [Blair] Generic staircase building
//
BOOL EV_DoGenStairs(line_t* line)
{
	int secnum;
	int osecnum; // jff 3/4/98 save old loop index
	int height;
	int i;
	int newsecnum = 0;
	int texture;
	int ok;
	bool rtn = false;

	sector_t* sec = NULL;
	sector_t* tsec = NULL;

	DFloor* floor = NULL;
	bool manual = false;

	fixed_t stairsize;
	fixed_t speed;

	unsigned value = (unsigned)line->special - GenStairsBase;

	  // parse the bit fields in the line's special type

	int Igno = (value & StairIgnore) >> StairIgnoreShift;
	int Dirn = (value & StairDirection) >> StairDirectionShift;
	int Step = (value & StairStep) >> StairStepShift;
	int Sped = (value & StairSpeed) >> StairSpeedShift;
	int Trig = (value & TriggerType) >> TriggerTypeShift;

	// check if a manual trigger, if so do just the sector on the backside
	if (line->id == 0)
	{
		secnum = sec - sectors;
		manual = true;
		goto manual_genstair;
	}

	// check if a manual trigger, if so do just the sector on the backside
	if (Trig == PushOnce || Trig == PushMany)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_genstair;
	}

	secnum = -1;
	while ((secnum = P_FindSectorFromTag(line->id, secnum)) >= 0)
	{
	manual_genstair:
		sec = &sectors[secnum];
		// ALREADY MOVING?	IF SO, KEEP GOING...
		// jff 2/26/98 add special lockout condition to wait for entire
		// staircase to build before retriggering
		if (sec->floordata || sec->stairlock)
		{
			if (!manual)
				continue;
			else
				return rtn;
		}

		fixed_t floorheight = P_FloorHeight(sec);

		// new floor thinker
		rtn = true;
		floor = new DFloor(sec);
		P_AddMovingFloor(sec);

		floor->m_Direction = Dirn ? 1 : -1;
		floor->m_OrgHeight = floorheight;   // [RH] Height to reset to
		floor->m_PauseTime = 0;

		// setup speed of stair building
		switch (Sped)
		{
		default:
		case SpeedSlow:
			floor->m_Speed = FLOORSPEED / 4;
			break;
		case SpeedNormal:
			floor->m_Speed = FLOORSPEED / 2;
			break;
		case SpeedFast:
			floor->m_Speed = FLOORSPEED * 2;
			break;
		case SpeedTurbo:
			floor->m_Speed = FLOORSPEED * 4;
			break;
		}

		// setup stepsize for stairs
		switch (Step)
		{
		default:
		case 0:
			stairsize = 4 * FRACUNIT;
			break;
		case 1:
			stairsize = 8 * FRACUNIT;
			break;
		case 2:
			stairsize = 16 * FRACUNIT;
			break;
		case 3:
			stairsize = 24 * FRACUNIT;
			break;
		}

		speed = floor->m_Speed;
		height = sec->floorheight + floor->m_Direction * stairsize;
		floor->m_FloorDestHeight = height;
		texture = sec->floorpic;
		floor->m_Crush = NO_CRUSH;
		floor->m_Type = DFloor::genBuildStair; // jff 3/31/98 do not leave uninited

		sec->stairlock = -2;
		sec->nextsec = -1;
		sec->prevsec = -1;

		osecnum = secnum; // jff 3/4/98 preserve loop index

		// Find next sector to raise
		// 1.	Find 2-sided line with same sector side[0] (lowest numbered)
		// 2.	Other side is the next sector to raise
		// 3.	Unless already moving, or different texture, then stop building
		do
		{
			ok = 0;
			for (i = 0; i < sec->linecount; i++)
			{
				if (!((sec->lines[i])->backsector))
					continue;

				tsec = (sec->lines[i])->frontsector;
				newsecnum = tsec - sectors;

				if (secnum != newsecnum)
					continue;

				tsec = (sec->lines[i])->backsector;
				newsecnum = tsec - sectors;

				if (!Igno && tsec->floorpic != texture)
					continue;

				/* jff 6/19/98 prevent double stepsize */
				//if (compatibility_level < boom_202_compatibility)
				//height += floor->m_Direction * stairsize;

				// if sector's floor already moving, look for another
				// jff 2/26/98 special lockout condition for retriggering
				if (tsec->floordata || tsec->stairlock)
					continue;

				height += floor->m_Direction * stairsize;

				ok = true;
				break;
			}

			if (ok)
			{
				// jff 2/26/98
				// link the stair chain in both directions
				// lock the stair sector until building complete
				sec->nextsec = newsecnum; // link step to next
				tsec->prevsec = secnum;   // link next back
				tsec->nextsec = -1;       // set next forward link as end
				tsec->stairlock = -2;     // lock the step

				sec = tsec;
				secnum = newsecnum;

				// create and initialize a thinker for the next step
				floor = new DFloor(sec);
				P_AddMovingFloor(sec);

				floor->PlayFloorSound();
				floor->m_Direction = Dirn ? 1 : -1;
				floor->m_FloorDestHeight = height;
				// [RH] Set up delay values
				floor->m_Delay = 0;
				floor->m_PauseTime = 0;

				floor->m_Speed = speed;
				floor->m_Type =
				    DFloor::genBuildStair; // jff 3/31/98 do not leave uninited
				// jff 2/27/98 fix uninitialized crush field
				floor->m_Crush = NO_CRUSH;
				floor->m_ResetCount = 0;      // [RH] Tics until reset (0 if never)
				floor->m_OrgHeight = floorheight; // [RH] Height to reset to
			}
		} while (ok);

		if (rtn)
			line->special ^= StairDirection; // alternate dir on succ activations

		if (manual)
			return rtn;
		secnum = osecnum; // jff 3/4/98 restore loop index
	}
	return rtn;
}

//
// BUILD A STAIRCASE!
// [RH] Added stairsize, srcspeed, delay, reset, igntxt, usespecials parameters
//		If usespecials is non-zero, then each sector in a stair is determined
//		by its special. If usespecials is 2, each sector stays in "sync" with
//		the others.
//
BOOL EV_BuildStairs (int tag, DFloor::EStair type, line_t *line,
					 fixed_t stairsize, fixed_t speed, int delay, int reset, int igntxt,
					 int usespecials)
{
	int 				secnum;
	int					osecnum;	//jff 3/4/98 save old loop index
	int 				height;
	int 				i;
	int 				newsecnum = 0;
	int 				texture;
	int 				ok;
	int					persteptime;
	BOOL 				rtn = false;

	sector_t*			sec = NULL;
	sector_t*			tsec = NULL;
	sector_t*			prev = NULL;

	DFloor*				floor = NULL;
	BOOL				manual = false;

	if (speed == 0)
		return false;

	persteptime = FixedDiv (stairsize, speed) >> FRACBITS;

	// check if a manual trigger, if so do just the sector on the backside
	if (tag == 0)
	{
		if (!line || !(sec = line->backsector))
			return rtn;
		secnum = sec-sectors;
		manual = true;
		goto manual_stair;
	}

	secnum = -1;
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

manual_stair:
		// ALREADY MOVING?	IF SO, KEEP GOING...
		//jff 2/26/98 add special lockout condition to wait for entire
		//staircase to build before retriggering
		if (sec->floordata || sec->stairlock)
		{
			if (!manual)
				continue;
			else
				return rtn;
		}

		fixed_t floorheight = P_FloorHeight(sec);

		// new floor thinker
		rtn = true;
		floor = new DFloor (sec);
		P_AddMovingFloor(sec);

		floor->m_Direction = (type == DFloor::buildUp) ? 1 : -1;
		floor->m_Type = DFloor::buildStair;	//jff 3/31/98 do not leave uninited
		floor->m_ResetCount = reset;	// [RH] Tics until reset (0 if never)
		floor->m_OrgHeight = floorheight;	// [RH] Height to reset to
		// [RH] Set up delay values
		floor->m_Delay = delay;
		floor->m_PauseTime = 0;
		floor->m_StepTime = floor->m_PerStepTime = persteptime;

		floor->m_Crush = (!usespecials && speed == 4*FRACUNIT) ? DOOM_CRUSH : NO_CRUSH; //jff 2/27/98 fix uninitialized crush field

		floor->m_Speed = speed;
		height = floorheight + stairsize * floor->m_Direction;
		floor->m_FloorDestHeight = height;

		texture = sec->floorpic;
		osecnum = secnum;				//jff 3/4/98 preserve loop index

		// Find next sector to raise
		// 1.	Find 2-sided line with same sector side[0] (lowest numbered)
		// 2.	Other side is the next sector to raise
		// 3.	Unless already moving, or different texture, then stop building
		do
		{
			ok = 0;

			if (usespecials)
			{
				// [RH] Find the next sector by scanning for Stairs_Special?
				tsec = P_NextSpecialSector (sec,
						(sec->special & 0xff) == Stairs_Special1 ?
							Stairs_Special2 : Stairs_Special1, prev);

				if ( (ok = (tsec != NULL)) )
				{
					height += floor->m_Direction * stairsize;

					// if sector's floor already moving, look for another
					//jff 2/26/98 special lockout condition for retriggering
					if (tsec->floordata || tsec->stairlock)
					{
						prev = sec;
						sec = tsec;
						continue;
					}
				}
				newsecnum = tsec - sectors;
			}
			else
			{
				for (i = 0; i < sec->linecount; i++)
				{
					if ( !((sec->lines[i])->flags & ML_TWOSIDED) )
						continue;

					tsec = (sec->lines[i])->frontsector;
					if (!tsec) continue;	//jff 5/7/98 if no backside, continue
					newsecnum = tsec-sectors;

					if (secnum != newsecnum)
						continue;

					tsec = (sec->lines[i])->backsector;
					newsecnum = tsec - sectors;

					if (!igntxt && tsec->floorpic != texture)
						continue;

					height += floor->m_Direction * stairsize;

					// if sector's floor already moving, look for another
					//jff 2/26/98 special lockout condition for retriggering
					if (tsec->floordata || tsec->stairlock)
						continue;

					ok = true;
					break;
				}
			}

			if (ok)
			{
				// jff 2/26/98
				// link the stair chain in both directions
				// lock the stair sector until building complete
				sec->nextsec = newsecnum; // link step to next
				tsec->prevsec = secnum;   // link next back
				tsec->nextsec = -1;       // set next forward link as end
				tsec->stairlock = -2;     // lock the step

				prev = sec;
				sec = tsec;
				secnum = newsecnum;

				// create and initialize a thinker for the next step
				floor = new DFloor (sec);
				P_AddMovingFloor(sec);

				floor->PlayFloorSound();
				floor->m_Direction = (type == DFloor::buildUp) ? 1 : -1;
				floor->m_FloorDestHeight = height;
				// [RH] Set up delay values
				floor->m_Delay = delay;
				floor->m_PauseTime = 0;
				floor->m_StepTime = floor->m_PerStepTime = persteptime;

				if (usespecials == 2)
				{
					// [RH]
					fixed_t rise = (floor->m_FloorDestHeight - floorheight)
									* floor->m_Direction;
					floor->m_Speed = FixedDiv (FixedMul (speed, rise), stairsize);
				}
				else
				{
					floor->m_Speed = speed;
				}
				floor->m_Type = DFloor::buildStair;	//jff 3/31/98 do not leave uninited
				//jff 2/27/98 fix uninitialized crush field
				floor->m_Crush =
				    (!usespecials && speed == 4 * FRACUNIT) ? DOOM_CRUSH : NO_CRUSH;
				floor->m_ResetCount = reset;	// [RH] Tics until reset (0 if never)
				floor->m_OrgHeight = floorheight;	// [RH] Height to reset to
			}
		} while(ok);
		if (manual)
			return rtn;
		secnum = osecnum;	//jff 3/4/98 restore loop index
	}
	return rtn;
}

int P_SpawnDonut(int, line_t*, fixed_t, fixed_t);

BOOL EV_DoZDoomDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed)
{
	int rtn = 0;

	rtn = P_SpawnDonut(line->id, line, pillarspeed, slimespeed);

	return rtn;
}

int EV_DoDonut(line_t* line)
{
	int rtn = 0;

	rtn = P_SpawnDonut(line->id, line, FLOORSPEED / 2, FLOORSPEED / 2);

	return rtn;
}

// [RH] Added pillarspeed and slimespeed parameters
int P_SpawnDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed)
{
	sector_t*			s1;
	sector_t*			s2;
	sector_t*			s3;
	int 				secnum;
	int 				rtn;
	int 				i;
	DFloor*				floor;

	secnum = -1;
	rtn = 0;
	while ((secnum = P_FindSectorFromTag(tag,secnum)) >= 0)
	{
		s1 = &sectors[secnum];					// s1 is pillar's sector

		// ALREADY MOVING?	IF SO, KEEP GOING...
		if (s1->floordata)
			continue;

		rtn = 1;
		s2 = getNextSector (s1->lines[0], s1);	// s2 is pool's sector
		if (!s2)								// note lowest numbered line around
			continue;							// pillar must be two-sided

		for (i = 0; i < s2->linecount; i++)
		{
			if (!(s2->lines[i]->flags & ML_TWOSIDED) ||
				(s2->lines[i]->backsector == s1))
				continue;
			s3 = s2->lines[i]->backsector;

			//	Spawn rising slime
			floor = new DFloor (s2);
			P_AddMovingFloor(s2);

			newspecial_s newspec;
			P_ResetTransferSpecial(&newspec);

			floor->m_Type = DFloor::donutRaise;
			floor->m_Crush = false;
			floor->m_HexenCrush = false;
			floor->m_Direction = 1;
			floor->m_Sector = s2;
			floor->m_Speed = slimespeed;
			floor->m_Texture = s3->floorpic;
			floor->m_NewSpecial = newspec.special;
			floor->m_NewDamageRate = newspec.damageamount;
			floor->m_NewFlags = newspec.flags;
			floor->m_NewDmgInterval = newspec.damageinterval;
			floor->m_NewLeakRate = newspec.damageleakrate;
			floor->m_FloorDestHeight = P_FloorHeight(s3);
			floor->m_Change = 0;
			floor->m_Height = 0;
			floor->m_Line = NULL;
			floor->PlayFloorSound();

			//	Spawn lowering donut-hole
			floor = new DFloor (s1);
			P_AddMovingFloor(s1);

			floor->m_Type = DFloor::floorLowerToNearest;
			floor->m_Crush = false;
			floor->m_HexenCrush = false;
			floor->m_Direction = -1;
			floor->m_Sector = s1;
			floor->m_Speed = pillarspeed;
			floor->m_FloorDestHeight = P_FloorHeight(s3);
			floor->m_Change = 0;
			floor->m_Height = 0;
			floor->m_Line = NULL;
			floor->PlayFloorSound();
			break;
		}
	}
	return rtn;
}

// ============================================================================
// ELEVATORS
// ============================================================================

void P_SetElevatorDestroy(DElevator *elevator)
{
	if (!elevator || predicting)
		return;

	elevator->m_Status = DElevator::destroy;

	if (clientside && elevator->m_Sector)
	{
		elevator->m_Sector->ceilingdata = NULL;
		elevator->m_Sector->floordata = NULL;
		elevator->Destroy();
	}
}

IMPLEMENT_SERIAL (DElevator, DMover)

DElevator::DElevator () :
	m_Status(init)
{
}

void DElevator::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_Status
			<< m_Direction
			<< m_FloorDestHeight
			<< m_CeilingDestHeight
			<< m_Speed;
	}
	else
	{
		arc >> m_Type
			>> m_Status
			>> m_Direction
			>> m_FloorDestHeight
			>> m_CeilingDestHeight
			>> m_Speed;
	}
}

void DElevator::PlayElevatorSound()
{
	if (predicting || !m_Sector)
		return;

	S_StopSound(m_Sector->soundorg);

	if (m_Status == init)
		S_LoopedSound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_mid", 1, ATTN_NORM);
	else
		S_Sound(m_Sector->soundorg, CHAN_BODY, "plats/pt1_stop", 1, ATTN_NORM);
}

//
// T_MoveElevator()
//
// Move an elevator to it's destination (up or down)
// Called once per tick for each moving floor.
//
// Passed an elevator_t structure that contains all pertinent info about the
// move. See P_SPEC.H for fields.
// No return.
//
// jff 02/22/98 added to support parallel floor/ceiling motion
//
void DElevator::RunThink ()
{
	if (m_Status == destroy)
		return;

	EResult res;

	if (m_Direction < 0)	// moving down
	{
		res = MoveCeiling (m_Speed, m_CeilingDestHeight, m_Direction);
		if (res == ok || res == pastdest)
			MoveFloor (m_Speed, m_FloorDestHeight, m_Direction);
	}
	else // up
	{
		res = MoveFloor (m_Speed, m_FloorDestHeight, m_Direction);
		if (res == ok || res == pastdest)
			MoveCeiling (m_Speed, m_CeilingDestHeight, m_Direction);
	}

	if (res == pastdest)	// if destination height acheived
	{
		m_Status = finished;
		// make floor stop sound
		PlayElevatorSound();

		P_SetElevatorDestroy(this);
	}
}

DElevator::DElevator (sector_t *sec)
	: Super (sec)
{
}

// Clones a DElevator and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DElevator* DElevator::Clone(sector_t* sec) const
{
	DElevator* ele = new DElevator(*this);

	ele->Orphan();
	ele->m_Sector = sec;

	return ele;
}

bool SpawnCommonElevator(line_t*, DElevator::EElevator, fixed_t,
                         fixed_t, int);

//
// EV_DoElevator
//
// Handle elevator linedef types
//
// Passed the linedef that triggered the elevator and the elevator action
//
// jff 2/22/98 new type to move floor and ceiling in parallel
// [RH] Added speed, tag, and height parameters and new types.
//
BOOL EV_DoElevator (line_t *line, DElevator::EElevator elevtype,
					fixed_t speed, fixed_t height, int tag)
{
	bool rtn = SpawnCommonElevator(line, elevtype, speed, height, tag);
	return rtn;
}

bool SpawnCommonElevator(line_t* line, DElevator::EElevator type, fixed_t speed,
                       fixed_t height, int tag)
{
	int secnum;
	BOOL rtn;
	sector_t* sec;
	DElevator* elevator;

	if (!line && (type == DElevator::elevateCurrent))
		return false;

	secnum = -1;
	rtn = false;
	// act on all sectors with the same tag as the triggering linedef
	while ((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
	{
		sec = &sectors[secnum];

		// If either floor or ceiling is already activated, skip it
		if (sec->ceilingdata && P_MovingCeilingCompleted(sec))
		{
			sec->ceilingdata->Destroy();
			sec->ceilingdata = NULL;
		}
		if (sec->floordata && P_MovingFloorCompleted(sec))
		{
			sec->floordata->Destroy();
			sec->floordata = NULL;
		}

		if (sec->floordata || sec->ceilingdata) // jff 2/22/98
			continue;

		fixed_t floorheight = P_FloorHeight(sec);
		fixed_t ceilingheight = P_CeilingHeight(sec);

		// create and initialize new elevator thinker
		rtn = true;
		elevator = new DElevator(sec);

		// [SL] 2012-04-19 - Elevators have both moving ceilings and floors.
		// Consider them as moving ceilings for consistency sake.
		P_AddMovingCeiling(sec);

		elevator->m_Type = type;
		elevator->m_Speed = speed;
		elevator->m_Status = DElevator::init;
		elevator->PlayElevatorSound();

		sec->floordata = sec->ceilingdata = elevator;

		// set up the fields according to the type of elevator action
		switch (type)
		{
		// elevator down to next floor
		case DElevator::elevateDown:
			elevator->m_Direction = -1;
			elevator->m_FloorDestHeight = P_FindNextLowestFloor(sec);
			elevator->m_CeilingDestHeight =
			    elevator->m_FloorDestHeight + ceilingheight - floorheight;
			break;

		// elevator up to next floor
		case DElevator::elevateUp:
			elevator->m_Direction = 1;
			elevator->m_FloorDestHeight = P_FindNextHighestFloor(sec);
			elevator->m_CeilingDestHeight =
			    elevator->m_FloorDestHeight + ceilingheight - floorheight;
			break;

		// elevator to floor height of activating switch's front sector
		case DElevator::elevateCurrent:
			elevator->m_FloorDestHeight = P_FloorHeight(line->frontsector);
			elevator->m_CeilingDestHeight =
			    elevator->m_FloorDestHeight + ceilingheight - floorheight;
			elevator->m_Direction = elevator->m_FloorDestHeight > floorheight ? 1 : -1;
			break;

		// [RH] elevate up by a specific amount
		case DElevator::elevateRaise:
			elevator->m_Direction = 1;
			elevator->m_FloorDestHeight = floorheight + height;
			elevator->m_CeilingDestHeight = ceilingheight + height;
			break;

		// [RH] elevate down by a specific amount
		case DElevator::elevateLower:
			elevator->m_Direction = -1;
			elevator->m_FloorDestHeight = floorheight - height;
			elevator->m_CeilingDestHeight = ceilingheight - height;
			break;
		}
	}

	return rtn;
}

// Almost identical to the above, but height is multiplied by FRACUNIT
bool EV_DoZDoomElevator(line_t* line, DElevator::EElevator type, fixed_t speed,
                       fixed_t height, int tag)
{
	height *= FRACUNIT;

	bool rtn = SpawnCommonElevator(line, type, speed, height, tag);

	return rtn;
}

///////////////////////////////////////
/// Waggle
///////////////////////////////////////

static const fixed_t FloatBobOffsets[64] = {
    0,       51389,   102283,  152192,  200636,  247147,  291278,  332604,
    370727,  405280,  435929,  462380,  484378,  501712,  514213,  521763,
    524287,  521763,  514213,  501712,  484378,  462380,  435929,  405280,
    370727,  332604,  291278,  247147,  200636,  152192,  102283,  51389,
    -1,      -51390,  -102284, -152193, -200637, -247148, -291279, -332605,
    -370728, -405281, -435930, -462381, -484380, -501713, -514215, -521764,
    -524288, -521764, -514214, -501713, -484379, -462381, -435930, -405280,
    -370728, -332605, -291279, -247148, -200637, -152193, -102284, -51389};
/*
BOOL EV_StartPlaneWaggle(int tag, line_t* line, int height, int speed, int offset,
                             int timer, bool ceiling)
{
	int sectorIndex;
	sector_t* sector;
	bool retCode;

	retCode = false;
	sectorIndex = -1;
	while ((sectorIndex = P_FindSectorFromTagOrLine(tag, line, sectorIndex)) >= 0)
	{
		sector = &sectors[sectorIndex];
		if (ceiling ? P_CeilingActive(sector) : P_FloorActive(sector))
		{ // Already busy with another thinker
			continue;
		}
		retCode = true;
		new DWaggle(sector, height, speed, offset, timer, ceiling);

		if (ceiling)
		{
			P_AddMovingCeiling(sector);
		}
		else
		{
			P_AddMovingFloor(sector);
		}
	}

	return retCode;
}

IMPLEMENT_SERIAL(DWaggle, DMover)

void DWaggle::Serialize(FArchive& arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_OriginalHeight
			<< m_Accumulator
			<< m_AccDelta
			<< m_TargetScale
			<< m_Scale
		    << m_ScaleDelta
			<< m_Ticker
			<< m_State
			<< m_Ceiling
			<< m_StartTic;
	}
	else
	{
		arc >> m_OriginalHeight
			>> m_Accumulator
			>> m_AccDelta
			>> m_TargetScale
			>> m_Scale
			>> m_ScaleDelta
			>> m_Ticker
			>> m_State
			>> m_Ceiling
			>> m_StartTic;
	}
}

DWaggle::DWaggle()
{

}

DWaggle::DWaggle(sector_t* sector, int height, int speed, int offset, int timer,
                 bool ceiling)
{
	if (ceiling)
	{
		sector->ceilingdata = this;
		m_OriginalHeight = sector->ceilingheight;
	}
	else
	{
		sector->floordata = this;
		m_OriginalHeight = sector->floorheight;
	}
	m_Sector = sector;
	m_Accumulator = offset * FRACUNIT;
	m_AccDelta = speed << 10;
	m_Scale = 0;
	m_TargetScale = height << 10;
	m_ScaleDelta = m_TargetScale / (35 + ((3 * 35) * height) / 255);
	m_Ticker = timer ? timer * 35 : -1;
	m_State = init;
	m_Ceiling = ceiling;
	m_StartTic = ::level.time; // [Blair] Used for client side synchronization
}

void DWaggle::RunThink()
{
	switch (m_State)
	{
	case finished:
		Destroy();
		m_State = destroy;
	case destroy:
		return;
		break;
	case init:
		m_State = expand;
		// fall thru
	case expand:
		if ((m_Scale += m_ScaleDelta) >= m_TargetScale)
		{
			m_Scale = m_TargetScale;
			m_State = stable;
		}
		break;
	case reduce:
		if ((m_Scale -= m_ScaleDelta) <= 0)
		{ // Remove
			if (m_Ceiling)
			{
				P_SetCeilingHeight(m_Sector, m_OriginalHeight);
			}
			else
			{
				P_SetFloorHeight(m_Sector, m_OriginalHeight);
			}
			P_ChangeSector(m_Sector, DOOM_CRUSH);
			m_State = finished;
			return;
		}
		break;
	case stable:
		if (m_Ticker != -1)
		{
			if (!--m_Ticker)
			{
				m_State = reduce;
			}
		}
		break;
	}

	m_Accumulator += m_AccDelta;
	fixed_t changeamount = m_OriginalHeight + FixedMul(FloatBobOffsets[(m_Accumulator >> FRACBITS) & 63], m_Scale);
	
	if (m_Ceiling)
	{
		P_SetCeilingHeight(m_Sector, changeamount);
	}
	else
	{
		P_SetFloorHeight(m_Sector, changeamount);
	}
	P_ChangeSector(m_Sector, DOOM_CRUSH);
}

DWaggle::DWaggle(sector_t* sec) : Super(sec) { }

// Clones a DWaggle and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DWaggle* DWaggle::Clone(sector_t* sec) const
{
	DWaggle* ele = new DWaggle(*this);

	ele->Orphan();
	ele->m_Sector = sec;

	return ele;
}
*/
VERSION_CONTROL (p_floor_cpp, "$Id$")
