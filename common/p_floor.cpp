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
//		Floor animation: raising stairs.
//
//-----------------------------------------------------------------------------


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "doomstat.h"
#include "r_state.h"
#include "tables.h"

EXTERN_CVAR(co_boomphys)

extern bool predicting;

// ============================================================================
// FLOORS
// ============================================================================

void P_SetFloorDestroy(DFloor *floor)
{
	if (!floor)
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
			<< m_NewSpecial
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
			>> m_NewSpecial
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

	EResult res = MoveFloor (m_Speed, m_FloorDestHeight, m_Crush, m_Direction);

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
	}
}

DFloor::DFloor (sector_t *sec)
	: DMovingFloor (sec), m_Status(init)
{
}

DFloor::DFloor(sector_t *sec, DFloor::EFloor floortype, line_t *line,
			   fixed_t speed, fixed_t height, bool crush, int change)
	: DMovingFloor (sec), m_Status(init)
{
	int secnum = sec - sectors;

	fixed_t floorheight = P_FloorHeight(sec);
	fixed_t ceilingheight = P_CeilingHeight(sec);

	m_Type = floortype;
	m_Crush = false;
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
		m_FloorDestHeight = floorheight -
			P_FindShortestTextureAround (secnum);
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
			P_FindShortestTextureAround (secnum);
		break;

	case DFloor::floorRaiseAndChange:
		m_Direction = 1;
		m_FloorDestHeight = floorheight + height;
		if (line)
		{
			sec->floorpic = line->frontsector->floorpic;
			sec->special = line->frontsector->special;
		}
		break;

	case DFloor::floorLowerAndChange:
		m_Direction = -1;
		m_FloorDestHeight =
			P_FindLowestFloorSurrounding (sec);
		m_Texture = sec->floorpic;
		// jff 1/24/98 make sure m_NewSpecial gets initialized
		// in case no surrounding sector is at floordestheight
		// --> should not affect compatibility <--
		m_NewSpecial = sec->special;

		//jff 5/23/98 use model subroutine to unify fixes and handling
		sec = P_FindModelFloorSector (m_FloorDestHeight,sec-sectors);
		if (sec)
		{
			m_Texture = sec->floorpic;
			m_NewSpecial = sec->special;
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
			sector_t *sec;

			sec = (floortype == DFloor::floorRaiseToLowestCeiling ||
				   floortype == DFloor::floorLowerToLowestCeiling ||
				   floortype == DFloor::floorRaiseToCeiling ||
				   floortype == DFloor::floorLowerToCeiling) ?
				  P_FindModelCeilingSector (m_FloorDestHeight, secnum) :
				  P_FindModelFloorSector (m_FloorDestHeight, secnum);

			if (sec) {
				m_Texture = sec->floorpic;
				switch (change & 3) {
					case 1:
						m_NewSpecial = 0;
						m_Type = DFloor::genFloorChg0;
						break;
					case 2:
						m_NewSpecial = sec->special;
						m_Type = DFloor::genFloorChgT;
						break;
					case 3:
						m_Type = DFloor::genFloorChg;
						break;
				}
			}
		} else if (line) {
			// Trigger model change
			m_Texture = line->frontsector->floorpic;

			switch (change & 3) {
				case 1:
					m_NewSpecial = 0;
					m_Type = DFloor::genFloorChg0;
					break;
				case 2:
					m_NewSpecial = sec->special;
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
		if (sec->floordata)
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
			secm = P_FindModelFloorSector(P_FloorHeight(sec), secnum);
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

		floor->m_Crush = (!usespecials && speed == 4*FRACUNIT); //jff 2/27/98 fix uninitialized crush field

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
				floor->m_Crush = (!usespecials && speed == 4*FRACUNIT);
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

// [RH] Added pillarspeed and slimespeed parameters
int EV_DoDonut (int tag, fixed_t pillarspeed, fixed_t slimespeed)
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

			floor->m_Type = DFloor::donutRaise;
			floor->m_Crush = false;
			floor->m_Direction = 1;
			floor->m_Sector = s2;
			floor->m_Speed = slimespeed;
			floor->m_Texture = s3->floorpic;
			floor->m_NewSpecial = 0;
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
	if (!elevator)
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
	int			secnum;
	BOOL		rtn;
	sector_t*	sec;
	DElevator*	elevator;

	if (!line && (elevtype == DElevator::elevateCurrent))
		return false;

	secnum = -1;
	rtn = false;
	// act on all sectors with the same tag as the triggering linedef
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
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

		if (sec->floordata || sec->ceilingdata) //jff 2/22/98
			continue;

		fixed_t floorheight = P_FloorHeight(sec);
		fixed_t ceilingheight = P_CeilingHeight(sec);

		// create and initialize new elevator thinker
		rtn = true;
		elevator = new DElevator (sec);

		// [SL] 2012-04-19 - Elevators have both moving ceilings and floors.
		// Consider them as moving ceilings for consistency sake.
		P_AddMovingCeiling(sec);

		elevator->m_Type = elevtype;
		elevator->m_Speed = speed;
		elevator->PlayElevatorSound();

        sec->floordata = sec->ceilingdata = elevator;

		// set up the fields according to the type of elevator action
		switch (elevtype)
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
			elevator->m_Direction =
				elevator->m_FloorDestHeight > floorheight ? 1 : -1;
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

VERSION_CONTROL (p_floor_cpp, "$Id$")

