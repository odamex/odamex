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
//	Door animation code (opening/closing)
//	[RH] Removed sliding door code and simplified for Hexen-ish specials
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "r_state.h"
#include "p_lnspec.h"

#include "p_spec.h"

#include "p_mapformat.h"

EXTERN_CVAR(co_zdoomsound)

extern bool predicting;

void P_SetDoorDestroy(DDoor *door)
{
	if (!door || predicting)
		return;

	door->m_Status = DDoor::destroy;

	if (clientside && door->m_Sector)
	{
		door->m_Sector->ceilingdata = NULL;
		door->Destroy();
	}
}

IMPLEMENT_SERIAL (DDoor, DMovingCeiling)

DDoor::DDoor () :
	m_Status(init),	m_Line(NULL)
{
}

void DDoor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_Status
			<< m_TopHeight
			<< m_Speed
			<< m_TopWait
			<< m_TopCountdown
			<< m_LightTag;
	}
	else
	{
		arc >> m_Type
			>> m_Status
			>> m_TopHeight
			>> m_Speed
			>> m_TopWait
			>> m_TopCountdown
			>> m_LightTag;
	}
}

//
// VERTICAL DOORS
//

//
// T_VerticalDoor
//
void DDoor::RunThink ()
{
	fixed_t ceilingheight = P_CeilingHeight(m_Sector);
	fixed_t floorheight = P_FloorHeight(m_Sector);
	
	EResult res;
		
	switch (m_Status)
	{
	case finished:

		if (m_Type != doorOpen)
			PlayDoorSound();
		// fall through
	case destroy:
		P_SetDoorDestroy(this);
		return;
		
	case waiting:
		// WAITING
		if (!--m_TopCountdown)
		{
			switch (m_Type)
			{
			case doorRaise:
			case genRaise:
			case blazeRaise:
			case genBlazeRaise:
				// time to go back down
				m_Status = closing;
				PlayDoorSound();
				break;
				
			case doorCloseWaitOpen:
			case close30ThenOpen:
			case genCdO:
			case genBlazeCdO:
				m_Status = opening;
				PlayDoorSound();
				break;
				
			default:
				break;
			}
		}
		break;
		
	case init:
		//	INITIAL WAIT
		if (!--m_TopCountdown)
		{
			switch (m_Type)
			{
			case doorRaiseIn5Mins:
			case waitRaiseDoor:
				m_Type = doorRaise;
				m_Status = opening;
				PlayDoorSound();
				break;

			case waitCloseDoor:
				m_Type = doorClose;
				m_Status = closing;
				PlayDoorSound();
				break;

			default:
				break;
			}
		}
		break;
		
	case closing:
		res = MoveCeiling(m_Speed, floorheight, -1);
		
        if (m_LightTag)
        {
			EV_LightTurnOnPartway(m_LightTag,
                FixedDiv(ceilingheight - floorheight, m_TopHeight - floorheight));
        }
		if (res == pastdest)
		{
			//S_StopSound (m_Sector->soundorg);
			SN_StopSequence (m_Sector);
			switch (m_Type)
			{
			case doorRaise:
			case doorClose:
			case blazeRaise:
			case blazeClose:
			case genRaise:
			case genClose:
			case genBlazeRaise:
			case genBlazeClose:
				m_Status = finished;
				P_SetDoorDestroy(this);	// Destroy the door immediately, not 1 tick after!
				return;
				
			case doorCloseWaitOpen:
			case genCdO:
			case genBlazeCdO:
				m_TopCountdown = m_TopWait;
				m_Status = waiting;
				break;
				
			default:
				break;
			}
			if (m_LightTag && m_TopHeight - m_Sector->floorheight)
            {
				EV_LightTurnOnPartway(m_LightTag, 0);
            }
		}
		else if (res == crushed)
		{
			switch (m_Type)
			{
			case doorClose:				// DO NOT GO BACK UP!
			case genClose:
			case blazeClose:
			case genBlazeClose:
				break;
				
			default:
				m_Status = reopening;
				PlayDoorSound();
				break;
			}
		}
		break;
		
	case reopening:
	case opening:
		res = MoveCeiling(m_Speed, m_TopHeight, 1);
		
        if (m_LightTag && m_TopHeight - floorheight)
        {
			EV_LightTurnOnPartway(m_LightTag,
                FixedDiv(ceilingheight - floorheight, m_TopHeight - floorheight));
        }
		if (res == pastdest)
		{
			//S_StopSound (m_Sector->soundorg);
			SN_StopSequence (m_Sector);
			switch (m_Type)
			{
			case doorRaise:
			case blazeRaise:
			case genRaise:
			case genBlazeRaise:
				// wait at top
				m_TopCountdown = m_TopWait;
				m_Status = waiting;
				break;
				
			case doorCloseWaitOpen:
			case doorOpen:
			case blazeOpen:
			case genBlazeOpen:
			case genOpen:
			case genCdO:
			case genBlazeCdO:
				m_Status = finished;
				return;
				
			default:
				break;
			}
			if (m_LightTag && m_TopHeight - floorheight)
            {
				EV_LightTurnOnPartway(m_LightTag, FRACUNIT);
            }
		}
		break;
	default:
		break;
	}
}

//
// IsBlazingDoor
//
// Returns true if the door is a fast door that should use
// the dr2_open/dr2_close sounds
//
static bool IsBlazingDoor(DDoor *door)
{
	if (!door)
		return false;
	
	return (door->m_Speed >= (8 << FRACBITS));
}

// [RH] PlayDoorSound: Plays door sound depending on direction and speed
void DDoor::PlayDoorSound ()
{
	if (predicting)
		return;
		
	if (m_Sector->seqType >= 0)
	{
		SN_StartSequence (m_Sector, m_Sector->seqType, SEQ_DOOR);
		return;
	}

	const char *snd = NULL;	
	switch(m_Status)
	{
	case opening:
		if (IsBlazingDoor(this))
			snd = "doors/dr2_open";
		else
			snd = "doors/dr1_open";
		break;
	case reopening:
		// [SL] 2011-06-10 - emulate vanilla Doom bug that plays the
		// slower door opening sound when a door is forced to reopen
		// due to a player standing underneath it when it's closing.
		snd = "doors/dr1_open";
		break;
	case closing:
		if (IsBlazingDoor(this))
			snd = "doors/dr2_clos";
		else
			snd = "doors/dr1_clos";
		break;
	case finished:
		// [SL] 2011-06-10 - emulate vanilla Doom bug that plays the
		// blazing-door sound twice: when the door begins to close
		// and when the door finishes closing.
		if (!IsBlazingDoor(this) || co_zdoomsound)
			return;
		snd = "doors/dr2_clos";
		break;
	default:
		return;
	}

	S_Sound (m_Sector->soundorg, CHAN_BODY, snd, 1, ATTN_NORM);
}

DDoor::DDoor (sector_t *sector)
	: DMovingCeiling(sector), m_Status(init), m_Line(NULL)
{
}

// [RH] Merged EV_VerticalDoor and EV_DoLockedDoor into EV_DoDoor
//		and made them more general to support the new specials.

// [RH] SpawnDoor: Helper function for EV_DoDoor
DDoor::DDoor (sector_t *sec, line_t *ln, EVlDoor type, fixed_t speed, int delay)
	: DMovingCeiling (sec), m_Status(init)
{
	m_Type = type;
	m_TopWait = delay;
	m_TopCountdown = -1;
	m_Speed = speed;
    m_Line = ln;
	/* killough 10/98: use gradual lighting changes if nonzero tag given */
	//door->lighttag = comp[comp_doorlight] ? 0 : line->tag;
	// [Blair] Only certain door types are allowed to have LightTags.
	if (m_Line && P_IsLightTagDoorType(m_Line->special))
	{
		m_LightTag = m_Line->id;
	}
	else
	{
		m_LightTag = 0; /* killough 10/98: no light effects with tagged doors */
	}

	fixed_t ceilingheight = P_CeilingHeight(sec);
	
	switch (type)
	{
	case doorClose:
		m_Status = closing;
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
		PlayDoorSound();
		break;

	case doorOpen:
	case doorRaise:
		m_Status = opening;
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4*FRACUNIT;
		if (m_TopHeight != ceilingheight)
			PlayDoorSound();
		break;

	case doorCloseWaitOpen:
		m_TopHeight = ceilingheight;
		m_Status = closing;
		PlayDoorSound();
		break;

	case doorRaiseIn5Mins: // denis - fixme - does this need code?
		break;
	}
}

// [Blair] Boom Generic locked Door Type
DDoor::DDoor(sector_t* sec, line_t* ln, int kind, int trigger, int speed)
    : DMovingCeiling(sec), m_Status(init)
{
	m_Line = ln;
	m_TopWait = VDOORWAIT;
	m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
	m_Status = opening;
	m_TopCountdown = -1;

	fixed_t ceilingheight = P_CeilingHeight(sec);

	switch (speed)
	{
	default:
	case SpeedSlow:
		m_Type = kind ? genOpen : genRaise;
		m_Speed = VDOORSPEED;
		break;
	case SpeedNormal:
		m_Type = kind ? genOpen : genRaise;
		m_Speed = VDOORSPEED * 2;
		break;
	case SpeedFast:
		m_Type = kind ? genBlazeOpen : genBlazeRaise;
		m_Speed = VDOORSPEED * 4;
		break;
	case SpeedTurbo:
		m_Type = kind ? genBlazeOpen : genBlazeRaise;
		m_Speed = VDOORSPEED * 8;
		break;
	}
	/* killough 10/98: implement gradual lighting */
	m_LightTag = (ln->special & 6) == 6 && ln->special > GenLockedBase ? ln->id : 0;

	if (m_TopHeight != ceilingheight)
		PlayDoorSound();
}

// [Blair] Boom Generic Door Type
DDoor::DDoor(sector_t* sec, line_t* ln, int delay, int kind, int trigger, int speed)
    : DMovingCeiling(sec), m_Status(init)
{
	m_Line = ln;
	m_TopCountdown = -1;

	// setup delay for door remaining open/closed
	switch (delay)
	{
	default:
	case 0:
		m_TopWait = 35;
		break;
	case 1:
		m_TopWait = VDOORWAIT;
		break;
	case 2:
		m_TopWait = 2 * VDOORWAIT;
		break;
	case 3:
		m_TopWait = 7 * VDOORWAIT;
		break;
	}

	// setup speed of door motion
	switch (speed)
	{
	default:
	case SpeedSlow:
		m_Speed = VDOORSPEED;
		break;
	case SpeedNormal:
		m_Speed = VDOORSPEED * 2;
		break;
	case SpeedFast:
		m_Speed = VDOORSPEED * 4;
		break;
	case SpeedTurbo:
		m_Speed = VDOORSPEED * 8;
		break;
	}
	/* killough 10/98: implement gradual lighting */
	m_LightTag =
	    (ln->special & 6) == 6 && ln->special > GenLockedBase
	        ? ln->id
	        : 0;

	// set kind of door, whether it opens then close, opens, closes etc.
	// assign target heights accordingly
	switch (kind)
	{
	case OdCDoor:
		m_Status = opening;
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - (4 * FRACUNIT);
		if (m_TopHeight != sec->ceilingheight)
			PlayDoorSound();
		m_Type = speed >= SpeedFast ? genBlazeRaise : genRaise;
		break;
	case ODoor:
		m_Status = opening;
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
		if (m_TopHeight != sec->ceilingheight)
			PlayDoorSound();
		m_Type = speed >= SpeedFast ? genBlazeOpen : genOpen;
		break;
	case CdODoor:
		m_TopHeight = sec->ceilingheight;
		m_Status = closing;
		PlayDoorSound();
		m_Type = speed >= SpeedFast ? genBlazeCdO : genCdO;
		break;
	case CDoor:
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
		m_Status = closing;
		PlayDoorSound();
		m_Type = speed >= SpeedFast ? genBlazeClose : genClose;
		break;
	default:
		break;
	}
}

// [Blair] ZDoom-compatible door type
DDoor::DDoor(sector_t* sec, line_t* ln, EVlDoor type, fixed_t speed, int topwait, byte lighttag, int topcountdown)
    : DMovingCeiling(sec), m_Status(init)
{
	m_Type = type;
	m_TopWait = topwait;
	m_TopCountdown = topcountdown;
	m_Speed = speed;
	m_Line = ln;
	m_LightTag = lighttag;

	fixed_t ceilingheight = P_CeilingHeight(sec);

	switch (type)
	{
	case doorClose:
		m_Status = closing;
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
		PlayDoorSound();
		break;

	case genCdO:
		m_TopHeight = ceilingheight;
		m_Status = closing;
		m_TopWait = topwait;
		PlayDoorSound();
		break;
	case doorOpen:
	case doorRaise:
		m_Status = opening;
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
		if (m_TopHeight != ceilingheight)
			PlayDoorSound();
		break;

	case waitRaiseDoor:
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
		m_Status = opening;
		break;

	case waitCloseDoor:
		m_TopHeight = P_FindLowestCeilingSurrounding(sec) - 4 * FRACUNIT;
		m_Status = opening;
		break;
	default:
		break;
	}
}

// Clones a DDoor and returns a pointer to that clone.
//
// The caller owns the pointer, and it must be deleted with `delete`.
DDoor* DDoor::Clone(sector_t* sec) const
{
	DDoor* door = new DDoor(*this);

	door->Orphan();
	door->m_Sector = sec;

	return door;
}

BOOL EV_DoDoor (DDoor::EVlDoor type, line_t *line, AActor *thing,
                int tag, int speed, int delay, card_t lock)
{
	BOOL		rtn = false;
	int 		secnum;
	sector_t*	sec;
    DDoor *door;

	if (lock && thing && !P_CheckKeys (thing->player, lock, tag))
		return false;

	if (tag == 0)
	{		// [RH] manual door
		if (!line)
			return false;

		// if the wrong side of door is pushed, give oof sound
		if (line->sidenum[1]==R_NOSIDE)				// killough
		{
			UV_SoundAvoidPlayer (thing, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
			return false;
		}

		// get the sector on the second side of activating linedef
		sec = sides[line->sidenum[1]].sector;
		secnum = sec-sectors;

		if (sec->ceilingdata && P_MovingCeilingCompleted(sec))
		{
			sec->ceilingdata->Destroy();
			sec->ceilingdata = NULL;
		}
		
		// if door already has a thinker, use it
		door = static_cast<DDoor *>(sec->ceilingdata);
		// cph 2001/04/05 -
		// Original Doom didn't distinguish floor/lighting/ceiling
		// actions, so we need to do the same in demo compatibility mode.
		//   [SL] 2011-06-20 - Credit to PrBoom for the fix
		if (!door)
			door = static_cast<DDoor *>(sec->floordata);
		if (!door)
			door = static_cast<DDoor *>(sec->lightingdata);

		if (door)
		{
			// ONLY FOR "RAISE" DOORS, NOT "OPEN"s
			if (door->m_Type == DDoor::doorRaise && type == DDoor::doorRaise)
			{
				if (sec->ceilingdata && sec->ceilingdata->IsKindOf (RUNTIME_CLASS(DDoor)))
				{
					if (door->m_Status == DDoor::closing)
					{
						// go back up
						door->m_Status = DDoor::reopening;
						door->PlayDoorSound();
						return true;
					}
					else if (map_format.getZDoom() && (line->flags & ML_SPAC_PUSH))
					{
						// [RH] activate push doors don't go back down when you
						// run into them (otherwise opening them would be
						// a real pain).
						door->m_Line = line;
						return true;	
					}
					else if (thing && thing->player)
					{
						// go back down
						door->m_Status = DDoor::closing;
						door->PlayDoorSound();
						return true;
					}
				}
				return false;
			}
		}
        else
        {
            door = new DDoor(sec, line, type, speed, delay);
            P_AddMovingCeiling(sec);
        }
		if (door)
        {
			rtn = true;
        }
	}
	else
	{	// [RH] Remote door
		secnum = -1;
		while ((secnum = P_FindSectorFromTag (tag,secnum)) >= 0)
		{
			sec = &sectors[secnum];
			// if the ceiling already moving, don't start the door action
			if (sec->ceilingdata)
				continue;

			door = new DDoor(sec, line, type, speed, delay);
			P_AddMovingCeiling(sec);
			
			if (door)
				rtn = true;
		}
	}
	
	return rtn;
}


//
// Spawn a door that closes after 30 seconds
//
void P_SpawnDoorCloseIn30 (sector_t *sec)
{
	DDoor *door = new DDoor (sec);
	P_AddMovingCeiling(sec);
	specialdoors.push_back(sec);

	sec->special = 0;

	door->m_Sector = sec;
	door->m_Type = DDoor::doorRaise;
	door->m_Speed = FRACUNIT*2;
	door->m_TopCountdown = 30 * TICRATE;
	door->m_Status = DDoor::waiting;
}

//
// Spawn a door that opens after 5 minutes
//
void P_SpawnDoorRaiseIn5Mins (sector_t *sec)
{
	DDoor *door = new DDoor (sec);
	P_AddMovingCeiling(sec);
	specialdoors.push_back(sec);

	sec->special = 0;

	door->m_Type = DDoor::doorRaiseIn5Mins;
	door->m_Speed = FRACUNIT * 2;
	door->m_TopHeight = P_FindLowestCeilingSurrounding (sec);
	door->m_TopHeight -= 4*FRACUNIT;
	door->m_TopWait = (150*TICRATE)/35;
	door->m_TopCountdown = 5 * 60 * TICRATE;
	door->m_Status = DDoor::init;
}

BOOL EV_DoZDoomDoor(DDoor::EVlDoor type, line_t* line, AActor* mo, byte tag,
                   byte speed_byte, int topwait, zdoom_lock_t lock, byte lightTag,
                   bool boomgen, int topcountdown)
{
	sector_t* sec;
	fixed_t speed;
	DDoor* door;

	speed = (fixed_t)speed_byte * FRACUNIT / 8;

	if (lock && !P_CanUnlockZDoomDoor(mo->player, lock, tag))
		return false;

	if (!tag)
	{
		if (!line)
			return false;

		// if the wrong side of door is pushed, give oof sound
		if (line->sidenum[1] == NO_INDEX)
		{
			if (mo->player) // is this check necessary?
				UV_SoundAvoidPlayer(mo, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
			return false;
		}

		// get the sector on the second side of activating linedef
		sec = sides[line->sidenum[1]].sector;

		if (sec->ceilingdata && P_MovingCeilingCompleted(sec))
		{
			sec->ceilingdata->Destroy();
			sec->ceilingdata = NULL;
		}

		// if door already has a thinker, use it
		door = static_cast<DDoor*>(sec->ceilingdata);

		if (door)
		{
			// Boom used remote door logic for generalized doors, even if they are manual
			if (boomgen)
				return false;

			if (sec->ceilingdata && sec->ceilingdata->IsKindOf(RUNTIME_CLASS(DDoor)))
			{
				// ONLY FOR "RAISE" DOORS, NOT "OPEN"s
				if (door->m_Type == DDoor::doorRaise && type == DDoor::doorRaise)
				{
					if (door->m_Status == DDoor::closing)
					{
						door->m_Status = DDoor::reopening;
						door->PlayDoorSound();
						return true;
					}
					else if (!(line->flags & ML_SPAC_PUSH))
					// [RH] activate push doors don't go back down when you
					//    run into them (otherwise opening them would be
					//    a real pain).
					{
						if (!mo->player)
							return false; // JDC: bad guys never close doors

						door->m_Status = DDoor::closing; // start going down immediately
						door->PlayDoorSound();
						return true;
					}
					else
					{
						return false;
					}
				}
			}

			return false;
		}

		new DDoor(sec, line, type, speed, topwait, lightTag, topcountdown);
		P_AddMovingCeiling(sec);
		return true;
	}
	else
	{
		int secnum = -1;
		BOOL retcode = false;

		while ((secnum = P_FindSectorFromTag(tag, secnum)) >= 0)
		{
			sec = &sectors[secnum];
			if (sec->ceilingdata)
			{
				continue;
			}
			retcode = true;
			new DDoor(sec, line, type, speed, topwait, lightTag, topcountdown);
			P_AddMovingCeiling(sec);
		}

		return retcode;
	}
}

//
// EV_DoGenDoor()
//
// Handle generalized door types
//
// Passed the linedef activating the generalized door
// Returns true if a thinker created
//
BOOL EV_DoGenDoor(line_t* line)
{
	int secnum;
	bool rtn;
	sector_t* sec;
	bool manual;
	unsigned value = (unsigned)line->special - GenDoorBase;

	// parse the bit fields in the line's special type

	int Dely = (value & DoorDelay) >> DoorDelayShift;
	int Kind = (value & DoorKind) >> DoorKindShift;
	int Sped = (value & DoorSpeed) >> DoorSpeedShift;
	int Trig = (value & TriggerType) >> TriggerTypeShift;

	rtn = 0;

	if (line->id == 0)
	{
		if (!(sec = line->backsector))
			return true;
		secnum = sec - sectors;
		manual = true;
		goto manual_gendoor;
	}; // e6y
	// check if a manual trigger, if so do just the sector on the backside
	manual = false;
	if (Trig == PushOnce || Trig == PushMany)
	{
		if (!(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_gendoor;
	}

	secnum = -1;
	rtn = false;

	// if not manual do all sectors tagged the same as the line
	while ((secnum = P_FindSectorFromTagOrLine(line->id, line, secnum)) >= 0)
	{
		sec = &sectors[secnum];
manual_gendoor:
		// Do not start another function if ceiling already moving
		if (sec->ceilingdata) // jff 2/22/98
		{
			if (!manual)
				continue;
			else
				return rtn;
		}

		// new door thinker
		new DDoor(sec, line, Dely, Kind, Trig, Sped);
		rtn = true;
		P_AddMovingCeiling(sec);

		if (manual)
			return rtn;
	}
	return rtn;
}

//
// EV_DoGenLockedDoor()
//
// Handle generalized locked door types
//
// Passed the linedef activating the generalized locked door
// Returns true if a thinker created
//
BOOL EV_DoGenLockedDoor(line_t* line)
{
	int secnum;
	bool rtn;
	sector_t* sec;
	bool manual;
	unsigned value = (unsigned)line->special - GenLockedBase;

	// parse the bit fields in the line's special type

	int Kind = (value & LockedKind) >> LockedKindShift;
	int Sped = (value & LockedSpeed) >> LockedSpeedShift;
	int Trig = (value & TriggerType) >> TriggerTypeShift;

	rtn = 0;

	if (line->id == 0)
	{
		if (!(sec = line->backsector))
			return true;
		secnum = sec - sectors;
		manual = true;
		goto manual_genlocked;
	}; // e6y
	// check if a manual trigger, if so do just the sector on the backside
	manual = false;
	if (Trig == PushOnce || Trig == PushMany)
	{
		if (!(sec = line->backsector))
			return rtn;
		secnum = sec - sectors;
		manual = true;
		goto manual_genlocked;
	}

	secnum = -1;
	rtn = false;

	// if not manual do all sectors tagged the same as the line
	while ((secnum = P_FindSectorFromLineTag(line, secnum)) >= 0)
	{
		sec = &sectors[secnum];
manual_genlocked:
		// Do not start another function if ceiling already moving
		if (P_CeilingActive(sec)) // jff 2/22/98
		{
			if (!manual)
				continue;
			else
				return rtn;
		}

		// new door thinker
		new DDoor(sec, line, Kind, Trig, Sped);
		rtn = true;
		P_AddMovingCeiling(sec);

		if (manual)
			return rtn;
	}
	return rtn;
}


VERSION_CONTROL (p_doors_cpp, "$Id$")
