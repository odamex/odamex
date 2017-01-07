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
//	Each function returns true if it caused something to happen
//	or false if it couldn't perform the desired action.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "g_level.h"
#include "v_palette.h"
#include "tables.h"
#include "i_system.h"

#define FUNC(a) static BOOL a (line_t *ln, AActor *it, int arg0, int arg1, \
							   int arg2, int arg3, int arg4)

#define SPEED(a)		((a)*(FRACUNIT/8))
#define TICS(a)			(((a)*TICRATE)/35)
#define OCTICS(a)		(((a)*TICRATE)/8)
#define	BYTEANGLE(a)	((angle_t)((a)<<24))

// Used by the teleporters to know if they were
// activated by walking across the backside of a line.
int TeleportSide;
extern bool HasBehavior;

// Set true if this special was activated from inside a script.
BOOL InScript;

// 9/11/10: Add poly action definitions here, even though they're in p_local...
// Why are these needed here?  Linux won't compile without these definitions??
//
BOOL EV_MovePoly (line_t *line, int polyNum, int speed, angle_t angle, fixed_t dist, BOOL overRide);
BOOL EV_OpenPolyDoor (line_t *line, int polyNum, int speed, angle_t angle, int delay, int distance, podoortype_t type);
BOOL EV_RotatePoly (line_t *line, int polyNum, int speed, int byteAngle, int direction, BOOL overRide);

//
// P_LineSpecialMovesSector
//
// Returns true if the special for line will cause a DMovingFloor or
// DMovingCeiling object to be created.
//
bool P_LineSpecialMovesSector(line_t *line)
{
	if (!line)
		return false;

	static bool initialized = false;
	static bool specials[256];

	if (!initialized)
	{
		// generate a lookup table for line specials
		initialized = true;
		memset(specials, 0, sizeof(specials));

		specials[Door_Close]					= true;		// 10
		specials[Door_Open]						= true;		// 11
		specials[Door_Raise]					= true;		// 12
		specials[Door_LockedRaise]				= true;		// 13
		specials[Floor_LowerByValue]			= true;		// 20
		specials[Floor_LowerToLowest]			= true;		// 21
		specials[Floor_LowerToNearest]			= true;		// 22
		specials[Floor_RaiseByValue]			= true;		// 23
		specials[Floor_RaiseToHighest]			= true;		// 24
		specials[Floor_RaiseToNearest]			= true;		// 25
		specials[Stairs_BuildDown]				= true;		// 26
		specials[Stairs_BuildUp]				= true;		// 27
		specials[Floor_RaiseAndCrush]			= true;		// 28
		specials[Pillar_Build]					= true;		// 29
		specials[Pillar_Open]					= true;		// 30
		specials[Stairs_BuildDownSync]			= true;		// 31
		specials[Stairs_BuildUpSync]			= true;		// 32
		specials[Floor_RaiseByValueTimes8]		= true;		// 35
		specials[Floor_LowerByValueTimes8]		= true;		// 36
		specials[Ceiling_LowerByValue]			= true;		// 40
		specials[Ceiling_RaiseByValue]			= true;		// 41
		specials[Ceiling_CrushAndRaise]			= true;		// 42
		specials[Ceiling_LowerAndCrush]			= true;		// 43
		specials[Ceiling_CrushStop]				= true;		// 44
		specials[Ceiling_CrushRaiseAndStay]		= true;		// 45
		specials[Floor_CrushStop]				= true;		// 46
		specials[Plat_PerpetualRaise]			= true;		// 60
		specials[Plat_Stop]						= true;		// 61
		specials[Plat_DownWaitUpStay]			= true;		// 62
		specials[Plat_DownByValue]				= true;		// 63
		specials[Plat_UpWaitDownStay]			= true;		// 64
		specials[Plat_UpByValue]				= true;		// 65
		specials[Floor_LowerInstant]			= true;		// 66
		specials[Floor_RaiseInstant]			= true;		// 67
		specials[Floor_MoveToValueTimes8]		= true;		// 68
		specials[Ceiling_MoveToValueTimes8]		= true;		// 69
		specials[Pillar_BuildAndCrush]			= true;		// 94
		specials[FloorAndCeiling_LowerByValue]	= true;		// 95
		specials[FloorAndCeiling_RaiseByValue]	= true;		// 96
		specials[Ceiling_LowerToHighestFloor]	= true;		// 192
		specials[Ceiling_LowerInstant]			= true;		// 193
		specials[Ceiling_RaiseInstant]			= true;		// 194
		specials[Ceiling_CrushRaiseAndStayA]	= true;		// 195
		specials[Ceiling_CrushAndRaiseA]		= true;		// 196
		specials[Ceiling_CrushAndRaiseSilentA]	= true;		// 197
		specials[Ceiling_RaiseByValueTimes8]	= true;		// 198
		specials[Ceiling_LowerByValueTimes8]	= true;		// 199
		specials[Generic_Floor]					= true;		// 200
		specials[Generic_Ceiling]				= true;		// 201
		specials[Generic_Door]					= true;		// 202
		specials[Generic_Lift]					= true;		// 203
		specials[Generic_Stairs]				= true;		// 204
		specials[Generic_Crusher]				= true;		// 205
		specials[Plat_DownWaitUpStayLip]		= true;		// 206
		specials[Plat_PerpetualRaiseLip]		= true;		// 207
		specials[Stairs_BuildUpDoom]			= true;		// 217
		specials[Plat_RaiseAndStayTx0]			= true;		// 228
		specials[Plat_UpByValueStayTx]			= true;		// 230
		specials[Plat_ToggleCeiling]			= true;		// 231
		specials[Floor_RaiseToLowestCeiling]	= true;		// 238
		specials[Floor_RaiseByValueTxTy]		= true;		// 239
		specials[Floor_RaiseByTexture]			= true;		// 240
		specials[Floor_LowerToLowestTxTy]		= true;		// 241
		specials[Floor_LowerToHighest]			= true;		// 242
		specials[Elevator_RaiseToNearest]		= true;		// 245
		specials[Elevator_MoveToFloor]			= true;		// 246
		specials[Elevator_LowerToNearest]		= true;		// 247
		specials[Door_CloseWaitOpen]			= true;		// 249
		specials[Floor_Donut]					= true;		// 250
		specials[FloorAndCeiling_LowerRaise]	= true;		// 251
		specials[Ceiling_RaiseToNearest]		= true;		// 252
		specials[Ceiling_LowerToLowest]			= true;		// 253
		specials[Ceiling_LowerToFloor]			= true;		// 254
		specials[Ceiling_CrushRaiseAndStaySilA]	= true;		// 255
	}

	return specials[line->special];
}

EXTERN_CVAR (cl_predictsectors)

bool P_CanActivateSpecials(AActor* mo, line_t* line)
{
	// Server can always activate specials
	if (serverside)
		return true;

	if (cl_predictsectors)
	{
		// Always predict sectors if set to 1, only predict sectors activated
		// by the local player if set to 2.
		if (cl_predictsectors == 1.0f ||
		    (mo->player == &consoleplayer() && cl_predictsectors == 2.0f))
			return true;
	}

	// Predict sectors that don't actually create floor or ceiling thinkers.
	if (!P_LineSpecialMovesSector(line))
		return true;

	return false;
}

FUNC(LS_NOP)
{
	return false;
}

FUNC(LS_Polyobj_RotateLeft)
// Polyobj_RotateLeft (po, speed, angle)
{
	return EV_RotatePoly (ln, arg0, arg1, arg2, 1, false);
}

FUNC(LS_Polyobj_RotateRight)
// Polyobj_rotateRight (po, speed, angle)
{
	return EV_RotatePoly (ln, arg0, arg1, arg2, -1, false);
}

FUNC(LS_Polyobj_Move)
// Polyobj_Move (po, speed, angle, distance)
{
	return EV_MovePoly (ln, arg0, SPEED(arg1), BYTEANGLE(arg2), arg3 * FRACUNIT, false);
}

FUNC(LS_Polyobj_MoveTimes8)
// Polyobj_MoveTimes8 (po, speed, angle, distance)
{
	return EV_MovePoly (ln, arg0, SPEED(arg1), BYTEANGLE(arg2), arg3 * FRACUNIT * 8, false);
}

FUNC(LS_Polyobj_DoorSwing)
// Polyobj_DoorSwing (po, speed, angle, delay)
{
	return EV_OpenPolyDoor (ln, arg0, arg1, BYTEANGLE(arg2), arg3, 0, PODOOR_SWING);
}

FUNC(LS_Polyobj_DoorSlide)
// Polyobj_DoorSlide (po, speed, angle, distance, delay)
{
	return EV_OpenPolyDoor (ln, arg0, SPEED(arg1), BYTEANGLE(arg2), arg4, arg3*FRACUNIT, PODOOR_SLIDE);
}

FUNC(LS_Polyobj_OR_RotateLeft)
// Polyobj_OR_RotateLeft (po, speed, angle)
{
	return EV_RotatePoly (ln, arg0, arg1, arg2, 1, true);
}

FUNC(LS_Polyobj_OR_RotateRight)
// Polyobj_OR_RotateRight (po, speed, angle)
{
	return EV_RotatePoly (ln, arg0, arg1, arg2, -1, true);
}

FUNC(LS_Polyobj_OR_Move)
// Polyobj_OR_Move (po, speed, angle, distance)
{
	return EV_MovePoly (ln, arg0, SPEED(arg1), BYTEANGLE(arg2), arg3 * FRACUNIT, true);
}

FUNC(LS_Polyobj_OR_MoveTimes8)
// Polyobj_OR_MoveTimes8 (po, speed, angle, distance)
{
	return EV_MovePoly (ln, arg0, SPEED(arg1), BYTEANGLE(arg2), arg3 * FRACUNIT * 8, true);
}

FUNC(LS_Door_Close)
// Door_Close (tag, speed)
{
	return EV_DoDoor (DDoor::doorClose, ln, it, arg0, SPEED(arg1), 0, NoKey);
}

FUNC(LS_Door_Open)
// Door_Open (tag, speed)
{
	return EV_DoDoor (DDoor::doorOpen, ln, it, arg0, SPEED(arg1), 0, NoKey);
}

FUNC(LS_Door_Raise)
// Door_Raise (tag, speed, delay)
{
	return EV_DoDoor (DDoor::doorRaise, ln, it, arg0, SPEED(arg1), TICS(arg2), NoKey);
}

FUNC(LS_Door_LockedRaise)
// Door_LockedRaise (tag, speed, delay, lock)
{
	return EV_DoDoor (arg2 ? DDoor::doorRaise : DDoor::doorOpen, ln, it,
                          arg0, SPEED(arg1), TICS(arg2), (card_t)arg3);
}

FUNC(LS_Door_CloseWaitOpen)
// Door_CloseWaitOpen (tag, speed, delay)
{
	return EV_DoDoor (DDoor::doorCloseWaitOpen, ln, it, arg0, SPEED(arg1), OCTICS(arg2), NoKey);
}

FUNC(LS_Generic_Door)
// Generic_Door (tag, speed, kind, delay, lock)
{
	DDoor::EVlDoor type;

	switch (arg2)
	{
		case 0: type = DDoor::doorRaise;			break;
		case 1: type = DDoor::doorOpen;				break;
		case 2: type = DDoor::doorCloseWaitOpen;	break;
		case 3: type = DDoor::doorClose;			break;
		default: return false;
	}
	return EV_DoDoor (type, ln, it, arg0, SPEED(arg1), OCTICS(arg3), (card_t)arg4);
}

FUNC(LS_Floor_LowerByValue)
// Floor_LowerByValue (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorLowerByValue, ln, arg0, SPEED(arg1), FRACUNIT*arg2, 0, 0);
}

FUNC(LS_Floor_LowerToLowest)
// Floor_LowerToLowest (tag, speed)
{
	return EV_DoFloor (DFloor::floorLowerToLowest, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Floor_LowerToHighest)
// Floor_LowerToHighest (tag, speed, adjust)
{
	return EV_DoFloor (DFloor::floorLowerToHighest, ln, arg0, SPEED(arg1), (arg2-128)*FRACUNIT, 0, 0);
}

FUNC(LS_Floor_LowerToNearest)
// Floor_LowerToNearest (tag, speed)
{
	return EV_DoFloor (DFloor::floorLowerToNearest, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Floor_RaiseByValue)
// Floor_RaiseByValue (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorRaiseByValue, ln, arg0, SPEED(arg1), FRACUNIT*arg2, 0, 0);
}

FUNC(LS_Floor_RaiseToHighest)
// Floor_RaiseToHighest (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseToHighest, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Floor_RaiseToNearest)
// Floor_RaiseToNearest (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseToNearest, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Floor_RaiseAndCrush)
// Floor_RaiseAndCrush (tag, speed, crush)
{
	return EV_DoFloor (DFloor::floorRaiseAndCrush, ln, arg0, SPEED(arg1), 0, (arg2 != 0), 0);
}

FUNC(LS_Floor_RaiseByValueTimes8)
// FLoor_RaiseByValueTimes8 (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorRaiseByValue, ln, arg0, SPEED(arg1), FRACUNIT*arg2*8, 0, 0);
}

FUNC(LS_Floor_LowerByValueTimes8)
// Floor_LowerByValueTimes8 (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorLowerByValue, ln, arg0, SPEED(arg1), FRACUNIT*arg2*8, 0, 0);
}

FUNC(LS_Floor_CrushStop)
// Floor_CrushStop (tag)
{
//	return EV_FloorCrushStop (arg0);
	return 0;
}

FUNC(LS_Floor_LowerInstant)
// Floor_LowerInstant (tag, unused, height)
{
	return EV_DoFloor (DFloor::floorLowerInstant, ln, arg0, 0, arg2*FRACUNIT*8, 0, 0);
}

FUNC(LS_Floor_RaiseInstant)
// Floor_RaiseInstant (tag, unused, height)
{
	return EV_DoFloor (DFloor::floorRaiseInstant, ln, arg0, 0, arg2*FRACUNIT*8, 0, 0);
}

FUNC(LS_Floor_MoveToValueTimes8)
// Floor_MoveToValueTimes8 (tag, speed, height, negative)
{
	return EV_DoFloor (DFloor::floorMoveToValue, ln, arg0, SPEED(arg1),
					   arg2*FRACUNIT*8*(arg3?-1:1), 0, 0);
}

FUNC(LS_Floor_RaiseToLowestCeiling)
// Floor_RaiseToLowestCeiling (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseToLowestCeiling, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Floor_RaiseByTexture)
// Floor_RaiseByTexture (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseByTexture, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Floor_RaiseByValueTxTy)
// Floor_RaiseByValueTxTy (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorRaiseAndChange, ln, arg0, SPEED(arg1), arg2*FRACUNIT, 0, 0);
}

FUNC(LS_Floor_LowerToLowestTxTy)
// Floor_LowerToLowestTxTy (tag, speed)
{
	return EV_DoFloor (DFloor::floorLowerAndChange, ln, arg0, SPEED(arg1), arg2*FRACUNIT, 0, 0);
}

FUNC(LS_Floor_Waggle)
// Floor_Waggle (tag, amplitude, frequency, delay, time)
{
//	return EV_StartFloorWaggle (arg0, arg1, arg2, arg3, arg4);
	return 0;
}

FUNC(LS_Floor_TransferTrigger)
// Floor_TransferTrigger (tag)
{
	return EV_DoChange (ln, trigChangeOnly, arg0);
}

FUNC(LS_Floor_TransferNumeric)
// Floor_TransferNumeric (tag)
{
	return EV_DoChange (ln, numChangeOnly, arg0);
}

FUNC(LS_Floor_Donut)
// Floor_Donut (pillartag, pillarspeed, slimespeed)
{
	return EV_DoDonut (arg0, SPEED(arg1), SPEED(arg2));
}

FUNC(LS_Generic_Floor)
// Generic_Floor (tag, speed, height, target, change/model/direct/crush)
{
	DFloor::EFloor type;

	if (arg4 & 8)
	{
		switch (arg3)
		{
			case 1: type = DFloor::floorRaiseToHighest;			break;
			case 2: type = DFloor::floorRaiseToLowest;			break;
			case 3: type = DFloor::floorRaiseToNearest;			break;
			case 4: type = DFloor::floorRaiseToLowestCeiling;	break;
			case 5: type = DFloor::floorRaiseToCeiling;			break;
			case 6: type = DFloor::floorRaiseByTexture;			break;
			default:type = DFloor::floorRaiseByValue;			break;
		}
	}
	else
	{
		switch (arg3)
		{
			case 1: type = DFloor::floorLowerToHighest;			break;
			case 2: type = DFloor::floorLowerToLowest;			break;
			case 3: type = DFloor::floorLowerToNearest;			break;
			case 4: type = DFloor::floorLowerToLowestCeiling;	break;
			case 5: type = DFloor::floorLowerToCeiling;			break;
			case 6: type = DFloor::floorLowerByTexture;			break;
			default:type = DFloor::floorLowerByValue;			break;
		}
	}

	return EV_DoFloor (type, ln, arg0, SPEED(arg1), arg2*FRACUNIT,
					   (arg4 & 16) ? 20 : -1, arg4 & 7);

}

FUNC(LS_Stairs_BuildDown)
// Stair_BuildDown (tag, speed, height, delay, reset)
{
	return EV_BuildStairs (arg0, DFloor::buildDown, ln,
						   arg2 * FRACUNIT, SPEED(arg1), TICS(arg3), arg4, 0, 1);
}

FUNC(LS_Stairs_BuildUp)
// Stairs_BuildUp (tag, speed, height, delay, reset)
{
	return EV_BuildStairs (arg0, DFloor::buildUp, ln,
						   arg2 * FRACUNIT, SPEED(arg1), TICS(arg3), arg4, 0, 1);
}

FUNC(LS_Stairs_BuildDownSync)
// Stairs_BuildDownSync (tag, speed, height, reset)
{
	return EV_BuildStairs (arg0, DFloor::buildDown, ln,
						   arg2 * FRACUNIT, SPEED(arg1), 0, arg3, 0, 2);
}

FUNC(LS_Stairs_BuildUpSync)
// Stairs_BuildUpSync (tag, speed, height, reset)
{
	return EV_BuildStairs (arg0, DFloor::buildUp, ln,
						   arg2 * FRACUNIT, SPEED(arg1), 0, arg3, 0, 2);
}

FUNC(LS_Stairs_BuildUpDoom)
// Stairs_BuildUpDoom (tag, speed, height, delay, reset)
{
	return EV_BuildStairs (arg0, DFloor::buildUp, ln,
						   arg2 * FRACUNIT, SPEED(arg1), TICS(arg3), arg4, 0, 0);
}

FUNC(LS_Generic_Stairs)
// Generic_Stairs (tag, speed, step, dir/igntxt, reset)
{
	DFloor::EStair type = (arg3 & 1) ? DFloor::buildUp : DFloor::buildDown;
	BOOL res = EV_BuildStairs (arg0, type, ln,
							   arg2 * FRACUNIT, SPEED(arg1), 0, arg4, arg3 & 2, 0);

	if (res && ln && (ln->flags & ML_REPEAT_SPECIAL) && ln->special == Generic_Stairs)
		// Toggle direction of next activation of repeatable stairs
		arg3 ^= 1;

	return res;
}

FUNC(LS_Pillar_Build)
// Pillar_Build (tag, speed, height)
{
	return EV_DoPillar (DPillar::pillarBuild, arg0, SPEED(arg1), arg2*FRACUNIT, 0, -1);
}

FUNC(LS_Pillar_Open)
// Pillar_Open (tag, speed, f_height, c_height)
{
	return EV_DoPillar (DPillar::pillarOpen, arg0, SPEED(arg1), arg2*FRACUNIT, arg3*FRACUNIT, -1);
}

FUNC(LS_Ceiling_LowerByValue)
// Ceiling_LowerByValue (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilLowerByValue, ln, arg0, SPEED(arg1), 0, arg2*FRACUNIT, 0, 0, 0);
}

FUNC(LS_Ceiling_RaiseByValue)
// Ceiling_RaiseByValue (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilRaiseByValue, ln, arg0, SPEED(arg1), 0, arg2*FRACUNIT, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerByValueTimes8)
// Ceiling_LowerByValueTimes8 (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilLowerByValue, ln, arg0, SPEED(arg1), 0, arg2*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_RaiseByValueTimes8)
// Ceiling_RaiseByValueTimes8 (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilRaiseByValue, ln, arg0, SPEED(arg1), 0, arg2*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_CrushAndRaise)
// Ceiling_CrushAndRaise (tag, speed, crush)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, arg0, SPEED(arg1), SPEED(arg1)/2, 0, (arg2 != 0), 0, 0);
}

FUNC(LS_Ceiling_LowerAndCrush)
// Ceiling_LowerAndCrush (tag, speed, crush)
{
	return EV_DoCeiling (DCeiling::ceilLowerAndCrush, ln, arg0, SPEED(arg1), SPEED(arg1)/2, 0, (arg2 != 0), 0, 0);
}

FUNC(LS_Ceiling_CrushStop)
// Ceiling_CrushStop (tag)
{
	return EV_CeilingCrushStop (arg0);
}

FUNC(LS_Ceiling_CrushRaiseAndStay)
// Ceiling_CrushRaiseAndStay (tag, speed, crush)
{
	return EV_DoCeiling (DCeiling::ceilCrushRaiseAndStay, ln, arg0, SPEED(arg1), SPEED(arg1)/2, 0, (arg2 != 0), 0, 0);
}

FUNC(LS_Ceiling_MoveToValueTimes8)
// Ceiling_MoveToValueTimes8 (tag, speed, height, negative)
{
	return EV_DoCeiling (DCeiling::ceilMoveToValue, ln, arg0, SPEED(arg1), 0,
						 arg2*FRACUNIT*8*((arg3) ? -1 : 1), 0, 0, 0);
}

FUNC(LS_Ceiling_LowerToHighestFloor)
// Ceiling_LowerToHighestFloor (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilLowerToHighestFloor, ln, arg0, SPEED(arg1), 0, 0, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerInstant)
// Ceiling_LowerInstant (tag, unused, height)
{
	return EV_DoCeiling (DCeiling::ceilLowerInstant, ln, arg0, 0, 0, arg2*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_RaiseInstant)
// Ceiling_RaiseInstant (tag, unused, height)
{
	return EV_DoCeiling (DCeiling::ceilRaiseInstant, ln, arg0, 0, 0, arg2*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_CrushRaiseAndStayA)
// Ceiling_CrushRaiseAndStayA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushRaiseAndStay, ln, arg0, SPEED(arg1), SPEED(arg2), 0, (arg3 != 0), 0, 0);
}

FUNC(LS_Ceiling_CrushRaiseAndStaySilA)
// Ceiling_CrushRaiseAndStaySilA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushRaiseAndStay, ln, arg0, SPEED(arg1), SPEED(arg2), 0, (arg3 != 0), 1, 0);
}

FUNC(LS_Ceiling_CrushAndRaiseA)
// Ceiling_CrushAndRaiseA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, arg0, SPEED(arg1), SPEED(arg2), 0, (arg3 != 0), 0, 0);
}

FUNC(LS_Ceiling_CrushAndRaiseSilentA)
// Ceiling_CrushAndRaiseSilentA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, arg0, SPEED(arg1), SPEED(arg2), 0, (arg3 != 0), 1, 0);
}

FUNC(LS_Ceiling_RaiseToNearest)
// Ceiling_RaiseToNearest (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilRaiseToNearest, ln, arg0, SPEED(arg1), 0, 0, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerToLowest)
// Ceiling_LowerToLowest (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilLowerToLowest, ln, arg0, SPEED(arg1), 0, 0, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerToFloor)
// Ceiling_LowerToFloor (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilLowerToFloor, ln, arg0, SPEED(arg1), 0, 0, 0, 0, 0);
}

FUNC(LS_Generic_Ceiling)
// Generic_Ceiling (tag, speed, height, target, change/model/direct/crush)
{
	DCeiling::ECeiling type;

	if (arg4 & 8) {
		switch (arg3) {
			case 1:  type = DCeiling::ceilRaiseToHighest;		break;
			case 2:  type = DCeiling::ceilRaiseToLowest;		break;
			case 3:  type = DCeiling::ceilRaiseToNearest;		break;
			case 4:  type = DCeiling::ceilRaiseToHighestFloor;	break;
			case 5:  type = DCeiling::ceilRaiseToFloor;			break;
			case 6:  type = DCeiling::ceilRaiseByTexture;		break;
			default: type = DCeiling::ceilRaiseByValue;			break;
		}
	} else {
		switch (arg3) {
			case 1:  type = DCeiling::ceilLowerToHighest;		break;
			case 2:  type = DCeiling::ceilLowerToLowest;		break;
			case 3:  type = DCeiling::ceilLowerToNearest;		break;
			case 4:  type = DCeiling::ceilLowerToHighestFloor;	break;
			case 5:  type = DCeiling::ceilLowerToFloor;			break;
			case 6:  type = DCeiling::ceilLowerByTexture;		break;
			default: type = DCeiling::ceilLowerByValue;			break;
		}
	}

	return EV_DoCeiling (type, ln, arg0, SPEED(arg1), SPEED(arg1), arg2*FRACUNIT,
						 (arg4 & 16) ? 20 : -1, 0, arg4 & 7);
	return false;
}

FUNC(LS_Generic_Crusher)
// Generic_Crusher (tag, dnspeed, upspeed, silent, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, arg0, SPEED(arg1),
						 SPEED(arg2), 0, (arg4 != 0), arg3 ? 2 : 0, 0);
}

FUNC(LS_Plat_PerpetualRaise)
// Plat_PerpetualRaise (tag, speed, delay)
{
	return EV_DoPlat (arg0, ln, DPlat::platPerpetualRaise, 0, SPEED(arg1), TICS(arg2), 8*FRACUNIT, 0);
}

FUNC(LS_Plat_PerpetualRaiseLip)
// Plat_PerpetualRaiseLip (tag, speed, delay, lip)
{
	return EV_DoPlat (arg0, ln, DPlat::platPerpetualRaise, 0, SPEED(arg1), TICS(arg2), arg3*FRACUNIT, 0);
}

FUNC(LS_Plat_Stop)
// Plat_Stop (tag)
{
	EV_StopPlat (arg0);
	return true;
}

FUNC(LS_Plat_DownWaitUpStay)
// Plat_DownWaitUpStay (tag, speed, delay)
{
	return EV_DoPlat (arg0, ln, DPlat::platDownWaitUpStay, 0, SPEED(arg1), TICS(arg2), 8*FRACUNIT, 0);
}

FUNC(LS_Plat_DownWaitUpStayLip)
// Plat_DownWaitUpStayLip (tag, speed, delay, lip)
{
	return EV_DoPlat (arg0, ln, DPlat::platDownWaitUpStay, 0, SPEED(arg1), TICS(arg2), arg3*FRACUNIT, 0);
}

FUNC(LS_Plat_DownByValue)
// Plat_DownByValue (tag, speed, delay, height)
{
	return EV_DoPlat (arg0, ln, DPlat::platDownByValue, FRACUNIT*arg3*8, SPEED(arg1), TICS(arg2), 0, 0);
}

FUNC(LS_Plat_UpByValue)
// Plat_UpByValue (tag, speed, delay, height)
{
	return EV_DoPlat (arg0, ln, DPlat::platUpByValue, FRACUNIT*arg3*8, SPEED(arg1), TICS(arg2), 0, 0);
}

FUNC(LS_Plat_UpWaitDownStay)
// Plat_UpWaitDownStay (tag, speed, delay)
{
	return EV_DoPlat (arg0, ln, DPlat::platUpWaitDownStay, 0, SPEED(arg1), TICS(arg2), 0, 0);
}

FUNC(LS_Plat_RaiseAndStayTx0)
// Plat_RaiseAndStayTx0 (tag, speed)
{
	return EV_DoPlat (arg0, ln, DPlat::platRaiseAndStay, 0, SPEED(arg1), 0, 0, 1);
}

FUNC(LS_Plat_UpByValueStayTx)
// Plat_UpByValueStayTx (tag, speed, height)
{
	return EV_DoPlat (arg0, ln, DPlat::platUpByValueStay, FRACUNIT*arg2*8, SPEED(arg1), 0, 0, 2);
}

FUNC(LS_Plat_ToggleCeiling)
// Plat_ToggleCeiling (tag)
{
	return EV_DoPlat (arg0, ln, DPlat::platToggle, 0, 0, 0, 0, 0);
}

FUNC(LS_Generic_Lift)
// Generic_Lift (tag, speed, delay, target, height)
{
	DPlat::EPlatType type;

	switch (arg3)
	{
		case 1:
			type = DPlat::platDownWaitUpStay;
			break;
		case 2:
			type = DPlat::platDownToNearestFloor;
			break;
		case 3:
			type = DPlat::platDownToLowestCeiling;
			break;
		case 4:
			type = DPlat::platPerpetualRaise;
			break;
		default:
			type = DPlat::platUpByValue;
			break;
	}

	return EV_DoPlat (arg0, ln, type, arg4*8*FRACUNIT, SPEED(arg1), OCTICS(arg2), 0, 0);
}

FUNC(LS_Exit_Normal)
// Exit_Normal (position)
{
	if (it && CheckIfExitIsGood (it))
	{
		G_ExitLevel (0, 1);
		return true;
	}
	return false;
}

FUNC(LS_Exit_Secret)
// Exit_Secret (position)
{
	if (it && CheckIfExitIsGood (it))
	{
		G_SecretExitLevel (0, 1);
		return true;
	}
	return false;
}

FUNC(LS_Teleport_NewMap)
// Teleport_NewMap (map, position)
{
   	if (!TeleportSide)
	{
        level_info_t *info = FindLevelByNum (arg0);

        if (it && (info && CheckIfExitIsGood (it)))
        {
            strncpy (level.nextmap, info->mapname, 8);
            G_ExitLevel (arg1, 1);
            return true;
        }
	}
	return false;
}

FUNC(LS_Teleport)
// Teleport (tid, tag)
{
	if(!it) return false;
	BOOL result;

	if (HasBehavior)
		// [AM] Use ZDoom-style teleport for Hexen-format maps
		result = EV_Teleport(arg0, arg1, TeleportSide, it);
	else
		// [AM] Use Vanilla-style teleport for Doom-format maps
		result = EV_LineTeleport(ln, TeleportSide, it);

	return result;
}

FUNC(LS_Teleport_NoFog)
// Teleport_NoFog (tid)
{
	if(!it) return false;
	return EV_SilentTeleport (arg0, ln, TeleportSide, it);
}

FUNC(LS_Teleport_EndGame)
// Teleport_EndGame ()
{
	if (!TeleportSide && it && CheckIfExitIsGood (it))
	{
		strncpy (level.nextmap, "EndGameC", 8);
		G_ExitLevel (0, 1);
		return true;
	}
	return false;
}

FUNC(LS_Teleport_Line)
// Teleport_Line (thisid, destid, reversed)
{
	if(!it) return false;
	return EV_SilentLineTeleport (ln, TeleportSide, it, arg1, arg2);
}

FUNC(LS_ThrustThing)
// ThrustThing (angle, force)
{
	if(!it) return false;

	angle_t angle = BYTEANGLE(arg0) >> ANGLETOFINESHIFT;

	it->momx = arg1 * finecosine[angle];
	it->momy = arg1 * finesine[angle];

	return true;
}

FUNC(LS_ThrustThingZ)
// ThrustThingZ (tid, zthrust, down/up, set)
{
	AActor *victim;
	fixed_t thrust = arg1*FRACUNIT/4;

	// [BC] Up is default
	if (arg2)
		thrust = -thrust;

	if (arg0 != 0)
	{
		FActorIterator iterator (arg0);

		while ( (victim = iterator.Next ()) )
		{
			if (!arg3)
				victim->momz = thrust;
			else
				victim->momz += thrust;
		}
		return true;
	}
	else if (it)
	{
		if (!arg3)
			it->momz = thrust;
		else
			it->momz += thrust;
		return true;
	}
	return false;
}

FUNC(LS_DamageThing)
// DamageThing (damage)
{
	if(!it) return false;

	if (arg0)
		P_DamageMobj (it, NULL, NULL, arg0, MOD_UNKNOWN);
	else
		P_DamageMobj (it, NULL, NULL, 10000, MOD_UNKNOWN);

	return true;
}

BOOL P_GiveBody (player_t *, int);

FUNC(LS_HealThing)
// HealThing (amount)
{
	if(!it) return false;

	if (it->player)
	{
		P_GiveBody (it->player, arg0);
	}
	else
	{
		it->health += arg0;
		if (mobjinfo[it->type].spawnhealth > it->health)
			it->health = mobjinfo[it->type].spawnhealth;
	}

	return true;
}

FUNC(LS_Thing_Activate)
// Thing_Activate (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, arg0);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (arg0);
		P_ActivateMobj (mobj, it);
		mobj = temp;
	}

	return true;
}

FUNC(LS_Thing_Deactivate)
// Thing_Deactivate (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, arg0);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (arg0);
		P_DeactivateMobj (mobj);
		mobj = temp;
	}

	return true;
}

FUNC(LS_Thing_Remove)
// Thing_Remove (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, arg0);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (arg0);
		mobj->Destroy ();
		mobj = temp;
	}

	return true;
}

FUNC(LS_Thing_Destroy)
// Thing_Destroy (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, arg0);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (arg0);

		if (mobj->flags & MF_SHOOTABLE)
			P_DamageMobj (mobj, NULL, it, mobj->health, MOD_UNKNOWN);

		mobj = temp;
	}

	return true;
}

FUNC(LS_Thing_Projectile)
// Thing_Projectile (tid, type, angle, speed, vspeed)
{
	return P_Thing_Projectile (arg0, arg1, BYTEANGLE(arg2), arg3<<(FRACBITS-3),
		arg4<<(FRACBITS-3), false);
}

FUNC(LS_Thing_ProjectileGravity)
// Thing_ProjectileGravity (tid, type, angle, speed, vspeed)
{
	return P_Thing_Projectile (arg0, arg1, BYTEANGLE(arg2), arg3<<(FRACBITS-3),
		arg4<<(FRACBITS-3), true);
}

FUNC(LS_Thing_Spawn)
// Thing_Spawn (tid, type, angle)
{
	return P_Thing_Spawn (arg0, arg1, BYTEANGLE(arg2), true);
}

FUNC(LS_Thing_SpawnNoFog)
// Thing_SpawnNoFog (tid, type, angle)
{
	return P_Thing_Spawn (arg0, arg1, BYTEANGLE(arg2), false);
}

FUNC(LS_Thing_SetGoal)
// Thing_SetGoal (tid, goal, delay)
{
	AActor *self = AActor::FindByTID (NULL, arg0);
	AActor *goal = AActor::FindGoal (NULL, arg1, MT_PATHNODE);

	while (self)
	{
		if (goal && (self->flags & MF_SHOOTABLE))
		{
			self->goal = goal->ptr();
			if (!self->target)
				self->reactiontime = arg2 * TICRATE;
		}
		self = self->FindByTID (arg0);
	}

	return true;
}

FUNC(LS_ACS_Execute)
// ACS_Execute (script, map, s_arg1, s_arg2, s_arg3)
{
	level_info_t *info;

	if ( (arg1 == 0) || !(info = FindLevelByNum (arg1)) )
		return P_StartScript (it, ln, arg0, level.mapname, TeleportSide, arg2, arg3, arg4, 0);
	else
		return P_StartScript (it, ln, arg0, info->mapname, TeleportSide, arg2, arg3, arg4, 0);
}

FUNC(LS_ACS_ExecuteAlways)
// ACS_ExecuteAlways (script, map, s_arg1, s_arg2, s_arg3)
{
	level_info_t *info;

	if ( (arg1 == 0) || !(info = FindLevelByNum (arg1)) )
		return P_StartScript (it, ln, arg0, level.mapname, TeleportSide, arg2, arg3, arg4, 1);
	else
		return P_StartScript (it, ln, arg0, info->mapname, TeleportSide, arg2, arg3, arg4, 1);
}

FUNC(LS_ACS_LockedExecute)
// ACS_LockedExecute (script, map, s_arg1, s_arg2, lock)
{
	if (arg4 && !P_CheckKeys (it->player, (card_t)arg4, 1))
		return false;
	else
		return LS_ACS_Execute (ln, it, arg0, arg1, arg2, arg3, 0);
}

FUNC(LS_ACS_Suspend)
// ACS_Suspend (script, map)
{
	level_info_t *info;

	if ( (arg1 == 0) || !(info = FindLevelByNum (arg1)) )
		P_SuspendScript (arg0, level.mapname);
	else
		P_SuspendScript (arg0, info->mapname);

	return true;
}

FUNC(LS_ACS_Terminate)
// ACS_Terminate (script, map)
{
	level_info_t *info;

	if ( (arg1 == 0) || !(info = FindLevelByNum (arg1)) )
		P_TerminateScript (arg0, level.mapname);
	else
		P_TerminateScript (arg0, info->mapname);

	return true;
}

FUNC(LS_FloorAndCeiling_LowerByValue)
// FloorAndCeiling_LowerByValue (tag, speed, height)
{
	return EV_DoElevator (ln, DElevator::elevateLower, SPEED(arg1), arg2*FRACUNIT, arg0);
}

FUNC(LS_FloorAndCeiling_RaiseByValue)
// FloorAndCeiling_RaiseByValue (tag, speed, height)
{
	return EV_DoElevator (ln, DElevator::elevateRaise, SPEED(arg1), arg2*FRACUNIT, arg0);
}

FUNC(LS_FloorAndCeiling_LowerRaise)
// FloorAndCeiling_LowerRaise (tag, fspeed, cspeed)
{
	return EV_DoCeiling (DCeiling::ceilRaiseToHighest, ln, arg0, SPEED(arg2), 0, 0, 0, 0, 0) ||
		EV_DoFloor (DFloor::floorLowerToLowest, ln, arg0, SPEED(arg1), 0, 0, 0);
}

FUNC(LS_Elevator_MoveToFloor)
// Elevator_MoveToFloor (tag, speed)
{
	return EV_DoElevator (ln, DElevator::elevateCurrent, SPEED(arg1), 0, arg0);
}

FUNC(LS_Elevator_RaiseToNearest)
// Elevator_RaiseToNearest (tag, speed)
{
	return EV_DoElevator (ln, DElevator::elevateUp, SPEED(arg1), 0, arg0);
}

FUNC(LS_Elevator_LowerToNearest)
// Elevator_LowerToNearest (tag, speed)
{
	return EV_DoElevator (ln, DElevator::elevateDown, SPEED(arg1), 0, arg0);
}

FUNC(LS_Light_ForceLightning)
// Light_ForceLightning (tag)
{
	return false;
}

FUNC(LS_Light_RaiseByValue)
// Light_RaiseByValue (tag, value)
{
	EV_LightChange (arg0, arg1);
	return true;
}

FUNC(LS_Light_LowerByValue)
// Light_LowerByValue (tag, value)
{
	EV_LightChange (arg0, -arg1);
	return true;
}

FUNC(LS_Light_ChangeToValue)
// Light_ChangeToValue (tag, value)
{
	EV_LightTurnOn (arg0, arg1);
	return true;
}

FUNC(LS_Light_Fade)
// Light_Fade (tag, value, tics);
{
	EV_StartLightFading (arg0, arg1, TICS(arg2));
	return true;
}

FUNC(LS_Light_Glow)
// Light_Glow (tag, upper, lower, tics)
{
	EV_StartLightGlowing (arg0, arg1, arg2, TICS(arg3));
	return true;
}

FUNC(LS_Light_Flicker)
// Light_Flicker (tag, upper, lower)
{
	EV_StartLightFlickering (arg0, arg1, arg2);
	return true;
}

FUNC(LS_Light_Strobe)
// Light_Strobe (tag, upper, lower, u-tics, l-tics)
{
	EV_StartLightStrobing (arg0, arg1, arg2, TICS(arg3), TICS(arg4));
	return true;
}

FUNC(LS_Light_StrobeDoom)
// Light_StrobeDoom (tag, u-tics, l-tics)
{
	EV_StartLightStrobing (arg0, TICS(arg1), TICS(arg2));
	return true;
}

FUNC(LS_Light_MinNeighbor)
// Light_MinNeighbor (tag)
{
	EV_TurnTagLightsOff (arg0);
	return true;
}

FUNC(LS_Light_MaxNeighbor)
// Light_MaxNeighbor (tag)
{
	EV_LightTurnOn (arg0, -1);
	return true;
}

FUNC(LS_Radius_Quake)
// Radius_Quake (intensity, duration, damrad, tremrad, tid)
{
	return P_StartQuake (arg4, arg0, arg1, arg2, arg3);
}

FUNC(LS_UsePuzzleItem)
// UsePuzzleItem (item, script)
{
	return false;
}

FUNC(LS_Sector_ChangeSound)
// Sector_ChangeSound (tag, sound)
{
	return false;
}

struct FThinkerCollection
{
	int RefNum;
	DThinker *Obj;
};

static TArray<FThinkerCollection> Collection;

void AdjustPusher (int tag, int magnitude, int angle, DPusher::EPusher type)
{
	// Find pushers already attached to the sector, and change their parameters.
	{
		TThinkerIterator<DPusher> iterator;
		FThinkerCollection collect;

		while ( (collect.Obj = iterator.Next ()) )
		{
			if ((collect.RefNum = ((DPusher *)collect.Obj)->CheckForSectorMatch (type, tag)) >= 0)
			{
				((DPusher *)collect.Obj)->ChangeValues (magnitude, angle);
				Collection.Push (collect);
			}
		}
	}

	int numcollected = Collection.Size ();
	int secnum = -1;

	// Now create pushers for any sectors that don't already have them.
	while ((secnum = P_FindSectorFromTag (tag, secnum)) >= 0)
	{
		int i;
		for (i = 0; i < numcollected; i++)
		{
			if (Collection[i].RefNum == sectors[secnum].tag)
				break;
		}
		if (i == numcollected)
		{
			new DPusher (type, NULL, magnitude, angle, NULL, secnum);
		}
	}
	Collection.Clear ();
}

FUNC(LS_Sector_SetWind)
// Sector_SetWind (tag, amount, angle)
{
	if (ln || arg3)
		return false;

	AdjustPusher (arg0, arg1, arg2, DPusher::p_wind);
	return true;
}

FUNC(LS_Sector_SetCurrent)
// Sector_SetCurrent (tag, amount, angle)
{
	if (ln || arg3)
		return false;

	AdjustPusher (arg0, arg1, arg2, DPusher::p_current);
	return true;
}

FUNC(LS_Sector_SetFriction)
// Sector_SetFriction ()
{
	return false;
}

FUNC(LS_Scroll_Texture_Both)
// Scroll_Texture_Both (id, left, right, up, down)
{
	if (arg0 == 0)
		return false;

	fixed_t dx = (arg1 - arg2) * (FRACUNIT/64);
	fixed_t dy = (arg4 - arg3) * (FRACUNIT/64);
	int sidechoice;

	if (arg0 < 0)
	{
		sidechoice = 1;
		arg0 = -arg0;
	}
	else
	{
		sidechoice = 0;
	}

	if (dx == 0 && dy == 0)
	{
		// Special case: Remove the scroller, because the deltas are both 0.
		TThinkerIterator<DScroller> iterator;
		DScroller *scroller;

		while ( (scroller = iterator.Next ()) )
		{
			int wallnum = scroller->GetWallNum ();

			if (wallnum >= 0 && lines[sides[wallnum].linenum].id == arg0 &&
				lines[sides[wallnum].linenum].sidenum[sidechoice] == wallnum)
			{
				scroller->Destroy ();
			}
		}
	}
	else
	{
		// Find scrollers already attached to the matching walls, and change
		// their rates.
		{
			TThinkerIterator<DScroller> iterator;
			FThinkerCollection collect;

			while ( (collect.Obj = iterator.Next ()) )
			{
				if ((collect.RefNum = ((DScroller *)collect.Obj)->GetWallNum ()) != -1 &&
					lines[sides[collect.RefNum].linenum].id == arg0 &&
					lines[sides[collect.RefNum].linenum].sidenum[sidechoice] == collect.RefNum)
				{
					((DScroller *)collect.Obj)->SetRate (dx, dy);
					Collection.Push (collect);
				}
			}
		}

		int numcollected = Collection.Size ();
		int linenum = -1;

		// Now create scrollers for any walls that don't already have them.
		while ((linenum = P_FindLineFromID (arg0, linenum)) >= 0)
		{
			int i;
			for (i = 0; i < numcollected; i++)
			{
				if (Collection[i].RefNum == lines[linenum].sidenum[sidechoice])
					break;
			}
			if (i == numcollected)
			{
				new DScroller (DScroller::sc_side, dx, dy, -1, lines[linenum].sidenum[sidechoice], 0);
			}
		}
		Collection.Clear ();
	}

	return true;
}

FUNC(LS_Scroll_Floor)
{
	return false;
}

FUNC(LS_Scroll_Ceiling)
{
	return false;
}

FUNC(LS_PointPush_SetForce)
// PointPush_SetForce ()
{
	return false;
}

FUNC(LS_Sector_SetDamage)
// Sector_SetDamage (tag, amount, mod)
{
	int secnum = -1;
	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0) {
		sectors[secnum].damage = arg1;
		sectors[secnum].mod = arg2;
	}
	return true;
}

FUNC(LS_Sector_SetGravity)
// Sector_SetGravity (tag, intpart, fracpart)
{
	int secnum = -1;
	float gravity;

	if (arg2 > 99)
		arg2 = 99;
	gravity = (float)arg1 + (float)arg2 * 0.01f;

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
		sectors[secnum].gravity = gravity;

	return true;
}

FUNC(LS_Sector_SetColor)
// Sector_SetColor (tag, r, g, b)
{
	int secnum = -1;

	if (clientside)
	{
		while ((secnum = P_FindSectorFromTag(arg0, secnum)) >= 0)
		{
			sectors[secnum].colormap = GetSpecialLights(arg1, arg2, arg3,
					sectors[secnum].colormap->fade.getr(),
					sectors[secnum].colormap->fade.getg(),
					sectors[secnum].colormap->fade.getb());
		}
	}
	return true;
}

FUNC(LS_Sector_SetFade)
// Sector_SetFade (tag, r, g, b)
{
	int secnum = -1;

	if (clientside)
	{
		while ((secnum = P_FindSectorFromTag(arg0, secnum)) >= 0)
		{
			sectors[secnum].colormap = GetSpecialLights(
					sectors[secnum].colormap->color.getr(),
					sectors[secnum].colormap->color.getg(),
					sectors[secnum].colormap->color.getb(),
					arg1, arg2, arg3);
		}
	}
	return true;
}

FUNC(LS_Sector_SetCeilingPanning)
// Sector_SetCeilingPanning (tag, x-int, x-frac, y-int, y-frac)
{
	int secnum = -1;
	fixed_t xofs = arg1 * FRACUNIT + arg2 * (FRACUNIT/100);
	fixed_t yofs = arg3 * FRACUNIT + arg4 * (FRACUNIT/100);

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
	{
		sectors[secnum].ceiling_xoffs = xofs;
		sectors[secnum].ceiling_yoffs = yofs;
	}
	return true;
}

FUNC(LS_Sector_SetFloorPanning)
// Sector_SetCeilingPanning (tag, x-int, x-frac, y-int, y-frac)
{
	int secnum = -1;
	fixed_t xofs = arg1 * FRACUNIT + arg2 * (FRACUNIT/100);
	fixed_t yofs = arg3 * FRACUNIT + arg4 * (FRACUNIT/100);

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
	{
		sectors[secnum].floor_xoffs = xofs;
		sectors[secnum].floor_yoffs = yofs;
	}
	return true;
}

FUNC(LS_Sector_SetCeilingScale)
// Sector_SetCeilingScale (tag, x-int, x-frac, y-int, y-frac)
{
	int secnum = -1;
	fixed_t xscale = arg1 * FRACUNIT + arg2 * (FRACUNIT/100);
	fixed_t yscale = arg3 * FRACUNIT + arg4 * (FRACUNIT/100);

	if (xscale)
		xscale = FixedDiv (FRACUNIT, xscale);
	if (yscale)
		yscale = FixedDiv (FRACUNIT, yscale);

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
	{
		if (xscale)
			sectors[secnum].ceiling_xscale = xscale;
		if (yscale)
			sectors[secnum].ceiling_yscale = yscale;
	}
	return true;
}

FUNC(LS_Sector_SetFloorScale)
// Sector_SetFloorScale (tag, x-int, x-frac, y-int, y-frac)
{
	int secnum = -1;
	fixed_t xscale = arg1 * FRACUNIT + arg2 * (FRACUNIT/100);
	fixed_t yscale = arg3 * FRACUNIT + arg4 * (FRACUNIT/100);

	if (xscale)
		xscale = FixedDiv (FRACUNIT, xscale);
	if (yscale)
		yscale = FixedDiv (FRACUNIT, yscale);

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
	{
		if (xscale)
			sectors[secnum].floor_xscale = xscale;
		if (yscale)
			sectors[secnum].floor_yscale = yscale;
	}
	return true;
}

FUNC(LS_Sector_SetRotation)
// Sector_SetRotation (tag, floor-angle, ceiling-angle)
{
	int secnum = -1;
	angle_t ceiling = arg2 * ANG(1);
	angle_t floor = arg1 * ANG(1);

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
	{
		sectors[secnum].floor_angle = floor;
		sectors[secnum].ceiling_angle = ceiling;
	}
	return true;
}

FUNC(LS_Line_AlignCeiling)
// Line_AlignCeiling (lineid, side)
{
	int line = P_FindLineFromID (arg0, -1);
	BOOL ret = 0;

	if (line < 0)
		I_Error ("Sector_AlignCeiling: Lineid %d is undefined", arg0);
	do
	{
		ret |= R_AlignFlat (line, !!arg1, 1);
	} while ( (line = P_FindLineFromID (arg0, line)) >= 0);
	return ret;
}

FUNC(LS_Line_AlignFloor)
// Line_AlignFloor (lineid, side)
{
	int line = P_FindLineFromID (arg0, -1);
	BOOL ret = 0;

	if (line < 0)
		I_Error ("Sector_AlignFloor: Lineid %d is undefined", arg0);
	do
	{
		ret |= R_AlignFlat (line, !!arg1, 0);
	} while ( (line = P_FindLineFromID (arg0, line)) >= 0);
	return ret;
}

FUNC(LS_ChangeCamera)
// ChangeCamera (tid, who, revert?)
{
	AActor *camera = AActor::FindGoal (NULL, arg0, MT_CAMERA);

	if (!it || !it->player || arg1)
	{
		for (Players::iterator itr = players.begin();itr != players.end();++itr)
		{
			if (!(itr->ingame()))
				continue;

			if (camera)
			{
				itr->camera = camera->ptr();
				if (arg2)
					itr->cheats |= CF_REVERTPLEASE;
			}
			else
			{
				itr->camera = itr->mo;
				itr->cheats &= ~CF_REVERTPLEASE;
			}
		}
	}
	else
	{
		if (camera) {
			it->player->camera = camera->ptr();
			if (arg2)
				it->player->cheats |= CF_REVERTPLEASE;
		} else {
			it->player->camera = it->ptr();
			it->player->cheats &= ~CF_REVERTPLEASE;
		}
	}

	return true;
}

FUNC(LS_SetPlayerProperty)
// SetPlayerProperty (who, set, which)
{
#define PROP_FROZEN		0
#define PROP_NOTARGET	1

	if(!it) return false;

	int mask = 0;

	if (!it->player && !arg0)
		return false;

	switch (arg2)
	{
		case PROP_FROZEN:
			mask = CF_FROZEN;
			break;
		case PROP_NOTARGET:
			mask = CF_NOTARGET;
			break;
	}

	if (arg1 == 0)
	{
		if (arg1)
			it->player->cheats |= mask;
		else
			it->player->cheats &= ~mask;
	}
	else
	{
		for (Players::iterator itr = players.begin();itr != players.end();++itr)
		{
			if (!(itr->ingame()))
				continue;

			if (arg1)
				itr->cheats |= mask;
			else
				itr->cheats &= ~mask;
		}
	}

	return !!mask;
}

FUNC(LS_TranslucentLine)
// TranslucentLine (id, amount)
{
	if (ln)
		return false;

	int linenum = -1;
	while ((linenum = P_FindLineFromID (arg0, linenum)) >= 0)
	{
		lines[linenum].lucency = arg1 & 255;
	}

	return true;
}

lnSpecFunc LineSpecials[256] =
{
	LS_NOP,
	LS_NOP,		// Polyobj_StartLine,
	LS_Polyobj_RotateLeft,
	LS_Polyobj_RotateRight,
	LS_Polyobj_Move,
	LS_NOP,		// Polyobj_ExplicitLine
	LS_Polyobj_MoveTimes8,
	LS_Polyobj_DoorSwing,
	LS_Polyobj_DoorSlide,
	LS_NOP,		// 9
	LS_Door_Close,
	LS_Door_Open,
	LS_Door_Raise,
	LS_Door_LockedRaise,
	LS_NOP,		// 14
	LS_NOP,		// 15
	LS_NOP,		// 16
	LS_NOP,		// 17
	LS_NOP,		// 18
	LS_NOP,		// 19
	LS_Floor_LowerByValue,
	LS_Floor_LowerToLowest,
	LS_Floor_LowerToNearest,
	LS_Floor_RaiseByValue,
	LS_Floor_RaiseToHighest,
	LS_Floor_RaiseToNearest,
	LS_Stairs_BuildDown,
	LS_Stairs_BuildUp,
	LS_Floor_RaiseAndCrush,
	LS_Pillar_Build,
	LS_Pillar_Open,
	LS_Stairs_BuildDownSync,
	LS_Stairs_BuildUpSync,
	LS_NOP,		// 33
	LS_NOP,		// 34
	LS_Floor_RaiseByValueTimes8,
	LS_Floor_LowerByValueTimes8,
	LS_NOP,		// 37
	LS_NOP,		// 38
	LS_NOP,		// 39
	LS_Ceiling_LowerByValue,
	LS_Ceiling_RaiseByValue,
	LS_Ceiling_CrushAndRaise,
	LS_Ceiling_LowerAndCrush,
	LS_Ceiling_CrushStop,
	LS_Ceiling_CrushRaiseAndStay,
	LS_Floor_CrushStop,
	LS_NOP,		// 47
	LS_NOP,		// 48
	LS_NOP,		// 49
	LS_NOP,		// 50
	LS_NOP,		// 51
	LS_NOP,		// 52
	LS_NOP,		// 53
	LS_NOP,		// 54
	LS_NOP,		// 55
	LS_NOP,		// 56
	LS_NOP,		// 57
	LS_NOP,		// 58
	LS_NOP,		// 59
	LS_Plat_PerpetualRaise,
	LS_Plat_Stop,
	LS_Plat_DownWaitUpStay,
	LS_Plat_DownByValue,
	LS_Plat_UpWaitDownStay,
	LS_Plat_UpByValue,
	LS_Floor_LowerInstant,
	LS_Floor_RaiseInstant,
	LS_Floor_MoveToValueTimes8,
	LS_Ceiling_MoveToValueTimes8,
	LS_Teleport,
	LS_Teleport_NoFog,
	LS_ThrustThing,
	LS_DamageThing,
	LS_Teleport_NewMap,
	LS_Teleport_EndGame,
	LS_NOP,		// 76
	LS_NOP,		// 77
	LS_NOP,		// 78
	LS_NOP,		// 79
	LS_ACS_Execute,
	LS_ACS_Suspend,
	LS_ACS_Terminate,
	LS_ACS_LockedExecute,
	LS_NOP,		// 84
	LS_NOP,		// 85
	LS_NOP,		// 86
	LS_NOP,		// 87
	LS_NOP,		// 88
	LS_NOP,		// 89
	LS_Polyobj_OR_RotateLeft,
	LS_Polyobj_OR_RotateRight,
	LS_Polyobj_OR_Move,
	LS_Polyobj_OR_MoveTimes8,
	LS_NOP,
	LS_FloorAndCeiling_LowerByValue,
	LS_FloorAndCeiling_RaiseByValue,
	LS_NOP,		// 97
	LS_NOP,		// 98
	LS_NOP,		// 99
	LS_NOP,		// Scroll_Texture_Left
	LS_NOP,		// Scroll_Texture_Right
	LS_NOP,		// Scroll_Texture_Up
	LS_NOP,		// Scroll_Texture_Down
	LS_NOP,		// 104
	LS_NOP,		// 105
	LS_NOP,		// 106
	LS_NOP,		// 107
	LS_NOP,		// 108
	LS_Light_ForceLightning,
	LS_Light_RaiseByValue,
	LS_Light_LowerByValue,
	LS_Light_ChangeToValue,
	LS_Light_Fade,
	LS_Light_Glow,
	LS_Light_Flicker,
	LS_Light_Strobe,
	LS_NOP,		// 117
	LS_NOP,		// 118
	LS_NOP,		// 119
	LS_Radius_Quake,
	LS_NOP,		// Line_SetIdentification
	LS_NOP,		// 122
	LS_NOP,		// 123
	LS_NOP,		// 124
	LS_NOP,		// 125
	LS_NOP,		// 126
	LS_NOP,		// 127
	LS_ThrustThingZ,		// 128
	LS_UsePuzzleItem,
	LS_Thing_Activate,
	LS_Thing_Deactivate,
	LS_Thing_Remove,
	LS_Thing_Destroy,
	LS_Thing_Projectile,
	LS_Thing_Spawn,
	LS_Thing_ProjectileGravity,
	LS_Thing_SpawnNoFog,
	LS_Floor_Waggle,
	LS_NOP,		// 139
	LS_Sector_ChangeSound,
	LS_NOP,		// 141
	LS_NOP,		// 142
	LS_NOP,		// 143
	LS_NOP,		// 144
	LS_NOP,		// 145
	LS_NOP,		// 146
	LS_NOP,		// 147
	LS_NOP,		// 148
	LS_NOP,		// 149
	LS_NOP,		// 150
	LS_NOP,		// 151
	LS_NOP,		// 152
	LS_NOP,		// 153
	LS_NOP,		// 154
	LS_NOP,		// 155
	LS_NOP,		// 156
	LS_NOP,		// 157
	LS_NOP,		// 158
	LS_NOP,		// 159
	LS_NOP,		// 160
	LS_NOP,		// 161
	LS_NOP,		// 162
	LS_NOP,		// 163
	LS_NOP,		// 164
	LS_NOP,		// 165
	LS_NOP,		// 166
	LS_NOP,		// 167
	LS_NOP,		// 168
	LS_NOP,		// 169
	LS_NOP,		// 170
	LS_NOP,		// 171
	LS_NOP,		// 172
	LS_NOP,		// 173
	LS_NOP,		// 174
	LS_NOP,		// 175
	LS_NOP,		// 176
	LS_NOP,		// 177
	LS_NOP,		// 178
	LS_NOP,		// 179
	LS_NOP,		// 180
	LS_NOP,		// 181
	LS_NOP,		// 182
	LS_Line_AlignCeiling,
	LS_Line_AlignFloor,
	LS_Sector_SetRotation,
	LS_Sector_SetCeilingPanning,
	LS_Sector_SetFloorPanning,
	LS_Sector_SetCeilingScale,
	LS_Sector_SetFloorScale,
	LS_NOP,		// Static_Init
	LS_SetPlayerProperty,
	LS_Ceiling_LowerToHighestFloor,
	LS_Ceiling_LowerInstant,
	LS_Ceiling_RaiseInstant,
	LS_Ceiling_CrushRaiseAndStayA,
	LS_Ceiling_CrushAndRaiseA,
	LS_Ceiling_CrushAndRaiseSilentA,
	LS_Ceiling_RaiseByValueTimes8,
	LS_Ceiling_LowerByValueTimes8,
	LS_Generic_Floor,
	LS_Generic_Ceiling,
	LS_Generic_Door,
	LS_Generic_Lift,
	LS_Generic_Stairs,
	LS_Generic_Crusher,
	LS_Plat_DownWaitUpStayLip,
	LS_Plat_PerpetualRaiseLip,
	LS_TranslucentLine,
	LS_NOP,		// Transfer_Heights
	LS_NOP,		// Transfer_FloorLight
	LS_NOP,		// Transfer_CeilingLight
	LS_Sector_SetColor,
	LS_Sector_SetFade,
	LS_Sector_SetDamage,
	LS_Teleport_Line,
	LS_Sector_SetGravity,
	LS_Stairs_BuildUpDoom,
	LS_Sector_SetWind,
	LS_Sector_SetFriction,
	LS_Sector_SetCurrent,
	LS_Scroll_Texture_Both,
	LS_NOP,		// Scroll_Texture_Model
	LS_Scroll_Floor,
	LS_Scroll_Ceiling,
	LS_NOP,		// Scroll_Texture_Offsets
	LS_ACS_ExecuteAlways,
	LS_PointPush_SetForce,
	LS_Plat_RaiseAndStayTx0,
	LS_Thing_SetGoal,
	LS_Plat_UpByValueStayTx,
	LS_Plat_ToggleCeiling,
	LS_Light_StrobeDoom,
	LS_Light_MinNeighbor,
	LS_Light_MaxNeighbor,
	LS_Floor_TransferTrigger,
	LS_Floor_TransferNumeric,
	LS_ChangeCamera,
	LS_Floor_RaiseToLowestCeiling,
	LS_Floor_RaiseByValueTxTy,
	LS_Floor_RaiseByTexture,
	LS_Floor_LowerToLowestTxTy,
	LS_Floor_LowerToHighest,
	LS_Exit_Normal,
	LS_Exit_Secret,
	LS_Elevator_RaiseToNearest,
	LS_Elevator_MoveToFloor,
	LS_Elevator_LowerToNearest,
	LS_HealThing,
	LS_Door_CloseWaitOpen,
	LS_Floor_Donut,
	LS_FloorAndCeiling_LowerRaise,
	LS_Ceiling_RaiseToNearest,
	LS_Ceiling_LowerToLowest,
	LS_Ceiling_LowerToFloor,
	LS_Ceiling_CrushRaiseAndStaySilA
};


EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_fragexitswitch)

BOOL CheckIfExitIsGood (AActor *self)
{
	if (self == NULL)
		return false;

	// [Toke - dmflags] Old location of DF_NO_EXIT

	if (sv_gametype != GM_COOP && self)
	{
        if (!sv_allowexit)
        {
			if (sv_fragexitswitch && serverside)
				P_DamageMobj(self, NULL, NULL, 10000, MOD_SUICIDE);

			return false;
		}
	}

	if (self->player && multiplayer)
		Printf (PRINT_HIGH, "%s exited the level.\n", self->player->userinfo.netname.c_str());

	return true;
}

VERSION_CONTROL (p_lnspec_cpp, "$Id$")

