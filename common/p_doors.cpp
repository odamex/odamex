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
//	Door animation code (opening/closing)
//	[RH] Removed sliding door code and simplified for Hexen-ish specials
//
//-----------------------------------------------------------------------------


#include "z_zone.h"
#include "doomdef.h"
#include "p_local.h"
#include "s_sound.h"
#include "s_sndseq.h"
#include "doomstat.h"
#include "r_state.h"
#include "dstrings.h"
#include "c_console.h"

#include "p_spec.h"

IMPLEMENT_SERIAL (DDoor, DMovingCeiling)

DDoor::DDoor ()
{
	m_Line = NULL;
	m_Status = init;
	memset(m_PlayedSound, false, sizeof(m_PlayedSound));
}

void DDoor::Serialize (FArchive &arc)
{
	Super::Serialize (arc);
	if (arc.IsStoring ())
	{
		arc << m_Type
			<< m_TopHeight
			<< m_Speed
			<< m_Direction
			<< m_TopWait
			<< m_TopCountdown
			<< m_Status;
	}
	else
	{
		arc >> m_Type
			>> m_TopHeight
			>> m_Speed
			>> m_Direction
			>> m_TopWait
			>> m_TopCountdown
			>> m_Status;
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
	if (clientside && m_Status == destroy)
	{
		if (serverside)	// single player game
		{
			// make sure we play the finished sound because it doesn't
			// get called for servers otherwise
			m_Status = finished;
			PlayDoorSound();
			m_Status = destroy;
		}

		m_Sector->ceilingdata = NULL;
		Destroy();
	}

	EResult res;
		
	switch (m_Direction)
	{
	case 0:
		// WAITING
		if (!--m_TopCountdown)
		{
			switch (m_Type)
			{
			case doorRaise:
				m_Direction = -1; // time to go back down
				m_Status = closing;
				PlayDoorSound();
				break;
				
			case doorCloseWaitOpen:
				m_Direction = 1;
				m_Status = opening;
				PlayDoorSound();
				break;
				
			default:
				break;
			}
		}
		break;
		
	case 2:
		//	INITIAL WAIT
		if (!--m_TopCountdown)
		{
			switch (m_Type)
			{
			case doorRaiseIn5Mins:
				m_Direction = 1;
				m_Type = doorRaise;
				m_Status = opening;
				PlayDoorSound();
				break;

			default:
				break;
			}
		}
		break;
		
    case -1:
		// DOWN
        res = MoveCeiling (m_Speed, m_Sector->floorheight, false, m_Direction);
        if (m_Line && m_Line->id)
        {
            EV_LightTurnOnPartway(m_Line->id,
                FixedDiv(
                    m_Sector->ceilingheight - m_Sector->floorheight,
                    m_TopHeight - m_Sector->floorheight
                )
            );
        }
		if (res == pastdest)
		{
			//S_StopSound (m_Sector->soundorg);
			SN_StopSequence (m_Sector);
			switch (m_Type)
			{
			case doorRaise:
			case doorClose:
				if (serverside)
					m_Status = destroy;
				else
				{
					// if the server sends the ceiling height of the door at the
					// floor but prediction code says we're at the top, ignore it
					if (!m_PlayedSound[closing] == opening)
						break;	
					// we really have finished
					m_Status = finished;
					PlayDoorSound();
					break;
				}
				break;
				
			case doorCloseWaitOpen:
				m_Direction = 0;
				m_TopCountdown = m_TopWait;
				m_Status = waiting;
				break;
				
			default:
				break;
			}
            if (m_Line && m_Line->id)
            {
                EV_LightTurnOnPartway(m_Line->id, 0);
            }
		}
		else if (res == crushed)
		{
			switch (m_Type)
			{
			case doorClose:				// DO NOT GO BACK UP!
				break;
				
			default:
				m_Direction = 1;
				m_Status = reopening;
				PlayDoorSound();
				break;
			}
		}
		break;
		
	case 1:
		// UP
		res = MoveCeiling (m_Speed, m_TopHeight, false, m_Direction);
		
        if (m_Line && m_Line->id)
        {
            EV_LightTurnOnPartway(m_Line->id,
                FixedDiv(
                    m_Sector->ceilingheight - m_Sector->floorheight,
                    m_TopHeight - m_Sector->floorheight
                )
            );
        }
		if (res == pastdest)
		{
			//S_StopSound (m_Sector->soundorg);
			SN_StopSequence (m_Sector);
			switch (m_Type)
			{
			case doorRaise:
				m_Direction = 0; // wait at top
				m_TopCountdown = m_TopWait;
				m_Status = waiting;
				break;
				
			case doorCloseWaitOpen:
			case doorOpen:
				if (serverside)
					m_Status = destroy;
				else
				{
					m_Status = finished;
					PlayDoorSound();
					return;
				}
				break;
				
			default:
				break;
			}
            if (m_Line && m_Line->id)
            {
                EV_LightTurnOnPartway(m_Line->id, FRACUNIT);
            }
		}
		break;
	}
}

// [RH] PlayDoorSound: Plays door sound depending on direction and speed
void DDoor::PlayDoorSound ()
{
	// We've already played this sound so don't repeat it
	if (m_PlayedSound[m_Status])
		return;
	m_PlayedSound[m_Status] = true;	

	const char *snd;
	if (m_Sector->seqType >= 0)
	{
		SN_StartSequence (m_Sector, m_Sector->seqType, SEQ_DOOR);
	}
	else
	{
		switch(m_Status)
		{
		case opening:
			if((m_Speed >= FRACUNIT*8))
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
			m_PlayedSound[opening] = false;
			m_PlayedSound[reopening] = false;
			if((m_Speed >= FRACUNIT*8))
				snd = "doors/dr2_clos";
			else
				snd = "doors/dr1_clos";
			break;
		case finished:
			// [SL] 2011-06-10 - emulate vanilla Doom bug that plays the
			// blazing-door sound twice: when the door begins to close
			// and when the door finishes closing.
			if ((m_Speed >= FRACUNIT*8))
				snd = "doors/dr2_clos";
			else
				return;
			break;
		default:
			return;
		}

		S_Sound (m_Sector->soundorg, CHAN_BODY, snd, 1, ATTN_NORM);
	}
}

DDoor::DDoor (sector_t *sector)
	: DMovingCeiling (sector)
{
	m_Line = NULL;
	m_Status = init;
	memset(m_PlayedSound, false, sizeof(m_PlayedSound));
}

// [RH] Merged EV_VerticalDoor and EV_DoLockedDoor into EV_DoDoor
//		and made them more general to support the new specials.

// [RH] SpawnDoor: Helper function for EV_DoDoor
DDoor::DDoor (sector_t *sec, line_t *ln, EVlDoor type, fixed_t speed, int delay)
	: DMovingCeiling (sec)
{
	m_Type = type;
	m_TopWait = delay;
	m_TopCountdown = -1;
	m_Speed = speed;
    m_Line = ln;
	memset(m_PlayedSound, false, sizeof(m_PlayedSound));
	m_Status = init;

	switch (type)
	{
	case doorClose:
		m_TopHeight = P_FindLowestCeilingSurrounding (sec) - 4*FRACUNIT;
		//m_TopHeight -= 4*FRACUNIT;
		m_Direction = -1;
		m_Status = closing;
		PlayDoorSound();
		break;

	case doorOpen:
	case doorRaise:
		m_Direction = 1;
		m_Status = opening;
		m_TopHeight = P_FindLowestCeilingSurrounding (sec) - 4*FRACUNIT;
		if (m_TopHeight != sec->ceilingheight)
		{
			PlayDoorSound();
		}
		break;

	case doorCloseWaitOpen:
		m_TopHeight = sec->ceilingheight;
		m_Direction = -1;
		m_Status = closing;
		PlayDoorSound();
		break;

	case doorRaiseIn5Mins: // denis - fixme - does this need code?
		break;
	}
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
		if (line->sidenum[1]==-1)				// killough
		{
			UV_SoundAvoidPlayer (thing, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
			return false;
		}

		// get the sector on the second side of activating linedef
		sec = sides[line->sidenum[1]].sector;
		secnum = sec-sectors;

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
					if (door->m_Direction == -1)
					{
						door->m_Direction = 1;	// go back up
						door->m_Status = DDoor::opening;
						door->PlayDoorSound();
						return true;
					}
					else if (thing && thing->player)
					{
						door->m_Direction = -1;	// go back down
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

			if (new DDoor (sec, line, type, speed, delay))
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

	sec->special = 0;

	door->m_Sector = sec;
	door->m_Direction = 0;
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

	sec->special = 0;

	door->m_Direction = 2;
	door->m_Type = DDoor::doorRaiseIn5Mins;
	door->m_Speed = FRACUNIT * 2;
	door->m_TopHeight = P_FindLowestCeilingSurrounding (sec);
	door->m_TopHeight -= 4*FRACUNIT;
	door->m_TopWait = (150*TICRATE)/35;
	door->m_TopCountdown = 5 * 60 * TICRATE;
	door->m_Status = DDoor::waiting;
}

VERSION_CONTROL (p_doors_cpp, "$Id$")

