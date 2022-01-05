// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Each function returns true if it caused something to happen
//	or false if it couldn't perform the desired action.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "p_lnspec.h"
#include "v_palette.h"
#include "tables.h"
#include "i_system.h"
#include "m_wdlstats.h"

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
extern bool s_SpecialFromServer;

// Set true if this special was activated from inside a script.
BOOL InScript;

// 9/11/10: Add poly action definitions here, even though they're in p_local...
// Why are these needed here?  Linux won't compile without these definitions??
//
BOOL EV_MovePoly (line_t *line, int polyNum, int speed, angle_t angle, fixed_t dist, BOOL overRide);
BOOL EV_OpenPolyDoor (line_t *line, int polyNum, int speed, angle_t angle, int delay, int distance, podoortype_t type);
BOOL EV_RotatePoly (line_t *line, int polyNum, int speed, int byteAngle, int direction, BOOL overRide);
BOOL EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t* line, byte tag, fixed_t speed,
                       fixed_t speed2, fixed_t height, int crush, byte silent, int change,
                       crushmode_e crushmode);

//
// P_LineSpecialMovesSector
//
// Returns true if the special for line will cause a DMovingFloor or
// DMovingCeiling object to be created.
//
bool P_LineSpecialMovesSector(byte special)
{
	static bool initialized = false;
	static bool zdoomspecials[283];

	if (!initialized)
	{
		// generate a lookup table for line zdoomspecials
		initialized = true;
		memset(zdoomspecials, 0, sizeof(zdoomspecials));

		zdoomspecials[Door_Close]					= true;		// 10
		zdoomspecials[Door_Open]					= true;		// 11
		zdoomspecials[Door_Raise]					= true;		// 12
		zdoomspecials[Door_LockedRaise]				= true;		// 13
		zdoomspecials[Floor_LowerByValue]			= true;		// 20
		zdoomspecials[Floor_LowerToLowest]			= true;		// 21
		zdoomspecials[Floor_LowerToNearest]			= true;		// 22
		zdoomspecials[Floor_RaiseByValue]			= true;		// 23
		zdoomspecials[Floor_RaiseToHighest]			= true;		// 24
		zdoomspecials[Floor_RaiseToNearest]			= true;		// 25
		zdoomspecials[Stairs_BuildDown]				= true;		// 26
		zdoomspecials[Stairs_BuildUp]				= true;		// 27
		zdoomspecials[Floor_RaiseAndCrush]			= true;		// 28
		zdoomspecials[Pillar_Build]					= true;		// 29
		zdoomspecials[Pillar_Open]					= true;		// 30
		zdoomspecials[Stairs_BuildDownSync]			= true;		// 31
		zdoomspecials[Stairs_BuildUpSync]			= true;		// 32
		zdoomspecials[Floor_RaiseByValueTimes8]		= true;		// 35
		zdoomspecials[Floor_LowerByValueTimes8]		= true;		// 36
		zdoomspecials[Ceiling_LowerByValue]			= true;		// 40
		zdoomspecials[Ceiling_RaiseByValue]			= true;		// 41
		zdoomspecials[Ceiling_CrushAndRaise]		= true;		// 42
		zdoomspecials[Ceiling_LowerAndCrush]		= true;		// 43
		zdoomspecials[Ceiling_CrushStop]			= true;		// 44
		zdoomspecials[Ceiling_CrushRaiseAndStay]	= true;		// 45
		zdoomspecials[Floor_CrushStop]				= true;		// 46
		zdoomspecials[Plat_PerpetualRaise]			= true;		// 60
		zdoomspecials[Plat_Stop]					= true;		// 61
		zdoomspecials[Plat_DownWaitUpStay]			= true;		// 62
		zdoomspecials[Plat_DownByValue]				= true;		// 63
		zdoomspecials[Plat_UpWaitDownStay]			= true;		// 64
		zdoomspecials[Plat_UpByValue]				= true;		// 65
		zdoomspecials[Floor_LowerInstant]			= true;		// 66
		zdoomspecials[Floor_RaiseInstant]			= true;		// 67
		zdoomspecials[Floor_MoveToValueTimes8]		= true;		// 68
		zdoomspecials[Ceiling_MoveToValueTimes8]	= true;		// 69
		zdoomspecials[Pillar_BuildAndCrush]			= true;		// 94
		zdoomspecials[FloorAndCeiling_LowerByValue]	= true;		// 95
		zdoomspecials[FloorAndCeiling_RaiseByValue]	= true;		// 96
		zdoomspecials[Ceiling_LowerToHighestFloor]	= true;		// 192
		zdoomspecials[Ceiling_LowerInstant]			= true;		// 193
		zdoomspecials[Ceiling_RaiseInstant]			= true;		// 194
		zdoomspecials[Ceiling_CrushRaiseAndStayA]	= true;		// 195
		zdoomspecials[Ceiling_CrushAndRaiseA]		= true;		// 196
		zdoomspecials[Ceiling_CrushAndRaiseSilentA]	= true;		// 197
		zdoomspecials[Ceiling_RaiseByValueTimes8]	= true;		// 198
		zdoomspecials[Ceiling_LowerByValueTimes8]	= true;		// 199
		zdoomspecials[Generic_Floor]				= true;		// 200
		zdoomspecials[Generic_Ceiling]				= true;		// 201
		zdoomspecials[Generic_Door]					= true;		// 202
		zdoomspecials[Generic_Lift]					= true;		// 203
		zdoomspecials[Generic_Stairs]				= true;		// 204
		zdoomspecials[Generic_Crusher]				= true;		// 205
		zdoomspecials[Plat_DownWaitUpStayLip]		= true;		// 206
		zdoomspecials[Plat_PerpetualRaiseLip]		= true;		// 207
		zdoomspecials[Stairs_BuildUpDoom]			= true;		// 217
		zdoomspecials[Plat_RaiseAndStayTx0]			= true;		// 228
		zdoomspecials[Plat_UpByValueStayTx]			= true;		// 230
		zdoomspecials[Plat_ToggleCeiling]			= true;		// 231
		zdoomspecials[Floor_RaiseToLowestCeiling]	= true;		// 238
		zdoomspecials[Floor_RaiseByValueTxTy]		= true;		// 239
		zdoomspecials[Floor_RaiseByTexture]			= true;		// 240
		zdoomspecials[Floor_LowerToLowestTxTy]		= true;		// 241
		zdoomspecials[Floor_LowerToHighest]			= true;		// 242
		zdoomspecials[Elevator_RaiseToNearest]		= true;		// 245
		zdoomspecials[Elevator_MoveToFloor]			= true;		// 246
		zdoomspecials[Elevator_LowerToNearest]		= true;		// 247
		zdoomspecials[Door_CloseWaitOpen]			= true;		// 249
		zdoomspecials[Floor_Donut]					= true;		// 250
		zdoomspecials[FloorAndCeiling_LowerRaise]	= true;		// 251
		zdoomspecials[Ceiling_RaiseToNearest]		= true;		// 252
		zdoomspecials[Ceiling_LowerToLowest]		= true;		// 253
		zdoomspecials[Ceiling_LowerToFloor]			= true;		// 254
		zdoomspecials[Ceiling_CrushRaiseAndStaySilA]= true;		// 255
	}

	return zdoomspecials[special];
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
		if (cl_predictsectors == 1.0f)
		{
			return true;
		}
		else if (cl_predictsectors == 2.0f && mo != NULL && mo->player == &consoleplayer())
		{
			return true;
		}
	}

	// Predict sectors that don't actually create floor or ceiling thinkers.
	if (line && !P_LineSpecialMovesSector(line->special))
		return true;

	return false;
}

FUNC(LS_NOP)
{
	return false;
}

FUNC(LS_NOTIMP)
{
	Printf(PRINT_HIGH, "Line special not implemented yet: special number %d", ln->special);
	return false;
}

// Begin ZDoom specials
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
	return EV_DoZDoomDoor(DDoor::doorClose, ln, it, arg0, arg1, 0, zk_none, arg2,
	                      false, 0);
}

FUNC(LS_Door_Open)
// Door_Open (tag, speed)
{
	return EV_DoZDoomDoor(DDoor::doorOpen, ln, it, arg0, arg1, 0, zk_none, arg2, false,
	                      0);
}

FUNC(LS_Door_Raise)
// Door_Raise (tag, speed, delay)
{
	return EV_DoZDoomDoor(DDoor::doorRaise, ln, it, arg0, arg1, arg2, zk_none,
	                      arg3, false, 0);
}

FUNC(LS_Door_LockedRaise)
// Door_LockedRaise (tag, speed, delay, lock)
{
	return EV_DoZDoomDoor(arg2 ? DDoor::doorRaise : DDoor::doorOpen, ln, it, arg0,
	                      arg1, arg2, (zdoom_lock_t)arg3, arg4, false, 0);
}

FUNC(LS_Door_CloseWaitOpen)
// Door_CloseWaitOpen (tag, speed, delay)
{
	return EV_DoZDoomDoor(DDoor::genCdO, ln, it, arg0, arg1, (int)arg2 * 35 / 8, zk_none,
	                      arg3, false, 0);
}

FUNC(LS_Door_WaitRaise)
// Door_WaitRaise (tag, speed, delay, wait, lighttag)
{
	return EV_DoZDoomDoor(DDoor::waitRaiseDoor, ln, it, arg0, arg1, arg2, zk_none,
	                      arg4, false, arg3);
}

FUNC(LS_Door_WaitClose)
// Door_WaitClose (tag, speed, wait, lighttag)
{
	return EV_DoZDoomDoor(DDoor::waitCloseDoor, ln, it, arg0, arg1, 0, zk_none, arg3,
	                      false, arg2);
}

FUNC(LS_Generic_Door)
// Generic_Door (tag, speed, kind, delay, lock)
{
	byte tag, lightTag;
	DDoor::EVlDoor type;
	bool boomgen = false;

	switch (arg2 & 63)
	{
	case 0:
		type = DDoor::doorRaise;
		break;
	case 1:
		type = DDoor::doorOpen;
		break;
	case 2:
		type = DDoor::genCdO;
		break;
	case 3:
		type = DDoor::doorClose;
		break;
	default:
		return 0;
	}

	// Boom doesn't allow manual generalized doors to be activated while they move
	if (arg2 & 64)
		boomgen = true;

	if (arg2 & 128)
	{
		tag = 0;
		lightTag = arg0;
	}
	else
	{
		tag = arg0;
		lightTag = 0;
	}

	return EV_DoZDoomDoor(type, ln, it, tag, arg1, (int)arg3 * 35 / 8,
	                               (zdoom_lock_t)arg4, lightTag, boomgen, 0);
}

FUNC(LS_Thing_Stop)
// Thing_Stop (tid)
{
	AActor * target;

	if (arg0 != 0)
	{
		FActorIterator iterator (arg0);

		while ((target = iterator.Next()))
		{
			target->momx = target->momy = target->momz = 0;
			if (target->player != NULL)
				target->momx = target->momy = 0;
			
			return true;
		}
	}
	else if (it)
	{
		it->momx = it->momy = it->momz = 0;
		if (it->player != NULL)
			it->momx = it->momy = 0;

		return true;
	}
	return false;
}

FUNC(LS_Floor_LowerByValue)
// Floor_LowerByValue (tag, speed, height)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerByValue, ln, arg0, arg1,
	                       arg2, NO_CRUSH, P_ArgToChange(arg3), false, false);
}

FUNC(LS_Floor_LowerToLowest)
// Floor_LowerToLowest (tag, speed)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowest, ln, arg0, arg1, 0,
	                       NO_CRUSH, P_ArgToChange(arg2), false, false);
}

FUNC(LS_Floor_LowerToLowestCeiling)
// Floor_LowerToLowestCeiling (tag, speed)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowestCeiling, ln, arg0, arg1, 0,
	                       NO_CRUSH, P_ArgToChange(arg2), false, false);
}

FUNC(LS_Floor_LowerToHighest)
// Floor_LowerToHighest (tag, speed, adjust)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerToHighest, ln, arg0, arg1,
	                       (int)arg2 - 128, NO_CRUSH, 0, false, arg3 == 1);
}

FUNC(LS_Floor_LowerToHighestEE)
// Floor_LowerToHighestEE (tag, speed, adjust)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerToHighest, ln, arg0, arg1, 0,
	                       NO_CRUSH, P_ArgToChange(arg2), false, false);
}

FUNC(LS_Floor_LowerToNearest)
// Floor_LowerToNearest (tag, speed)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerToNearest, ln, arg0, arg1, 0,
	                       NO_CRUSH, P_ArgToChange(arg2), false, false);
}

FUNC(LS_Floor_RaiseByValue)
// Floor_RaiseByValue (tag, speed, height)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseByValue, ln, arg0, arg1,
	                       arg2, P_ArgToCrush(arg4), P_ArgToChange(arg3), true,
	                       false);
}

FUNC(LS_Floor_RaiseToHighest)
// Floor_RaiseToHighest (tag, speed)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToHighest, ln, arg0, arg1, 0,
	                       P_ArgToCrush(arg3), P_ArgToChange(arg2), true, false);
}

FUNC(LS_Floor_RaiseToNearest)
// Floor_RaiseToNearest (tag, speed)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToNearest, ln, arg0, arg1, 0,
	                       P_ArgToCrush(arg3), P_ArgToChange(arg2), true, false);
}

FUNC(LS_Floor_RaiseAndCrush)
// Floor_RaiseAndCrush (tag, speed, crush, crushmode)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseAndCrush, ln, arg0, arg1, 0,
	                       arg2, 0, P_ArgToCrushType(arg3), false);
}

FUNC(LS_Floor_RaiseAndCrushDoom)
// Floor_RaiseAndCrushDoom (tag, speed, crush, crushmode)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseAndCrushDoom, ln, arg0, arg1,
	                       0, arg2, 0, P_ArgToCrushType(arg3), false);
}

FUNC(LS_Floor_RaiseByValueTimes8)
// FLoor_RaiseByValueTimes8 (tag, speed, height)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseByValue, ln, arg0, arg1,
	                       (int)arg2 * 8, P_ArgToCrush(arg4),
	                       P_ArgToChange(arg3), true, false);
}

FUNC(LS_Floor_LowerByValueTimes8)
// Floor_LowerByValueTimes8 (tag, speed, height)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerByValue, ln, arg0, arg1,
	                       (int)arg2 * 8, NO_CRUSH, P_ArgToChange(arg3), false, false);
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
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerInstant, ln, arg0, 0,
	                       (int)arg2 * 8, NO_CRUSH, P_ArgToChange(arg3), false,
	                       false);
}

FUNC(LS_Floor_RaiseInstant)
// Floor_RaiseInstant (tag, unused, height, crush)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseInstant, ln, arg0, 0,
	                       (int)arg2 * 8, P_ArgToCrush(arg4),
	                       P_ArgToChange(arg3), true, false);
}

FUNC(LS_Floor_MoveToValue)
// Floor_MoveToValue (tag, speed, height, negative)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorMoveToValue, ln, arg0, arg1,
	                       (int)arg2 * (arg3 ? -1 : 1), NO_CRUSH,
	                       P_ArgToChange(arg4), false, false);
}

FUNC(LS_Floor_MoveToValueTimes8)
// Floor_MoveToValueTimes8 (tag, speed, height, negative)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorMoveToValue, ln, arg0, arg1,
	                       (int)arg2 * 8 * (arg3 ? -1 : 1), NO_CRUSH,
	                       P_ArgToChange(arg4), false, false);
}

FUNC(LS_Floor_RaiseToLowest)
// Floor_RaiseToLowest (tag, change, crush)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToLowest, ln, arg0, 2, 0,
	                       P_ArgToCrush(arg3), P_ArgToChange(arg2), true, false);
}

FUNC(LS_Floor_ToCeilingInstant)
// Floor_ToCeilingInstant (tag, change, crush, gap)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToLowest, ln, arg0, 2, 0,
	                       P_ArgToCrush(arg3), P_ArgToChange(arg2), true, false);
}

FUNC(LS_Floor_RaiseToLowestCeiling)
// Floor_RaiseToLowestCeiling (tag, speed, change, crush)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToLowestCeiling, ln, arg0,
	                       arg1, 0, P_ArgToCrush(arg3), P_ArgToChange(arg2),
	                       true, false);
}

FUNC(LS_Floor_RaiseToCeiling)
// Floor_RaiseToLowestCeiling (tag, speed, change, crush, gap)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseToCeiling, ln, arg0, arg1,
	                       arg4, P_ArgToCrush(arg3), P_ArgToChange(arg2), true,
	                       false);
}

FUNC(LS_Floor_RaiseByTexture)
// Floor_RaiseByTexture (tag, speed, change, crush)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseByTexture, ln, arg0, arg1, 0,
	                       P_ArgToCrush(arg3), P_ArgToChange(arg2), true, false);
}

FUNC(LS_Floor_LowerByTexture)
// Floor_LowerByTexture (tag, speed, change)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerByTexture, ln, arg0, arg1, 0,
	                       NO_CRUSH, P_ArgToChange(arg2), true, false);
}

FUNC(LS_Floor_RaiseByValueTxTy)
// Floor_RaiseByValueTxTy (tag, speed, height)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorRaiseAndChange, ln, arg0, arg1,
	                       arg2, NO_CRUSH, 0, false, false);
}

FUNC(LS_Floor_LowerToLowestTxTy)
// Floor_LowerToLowestTxTy (tag, speed)
{
	return EV_DoZDoomFloor(DFloor::EFloor::floorLowerAndChange, ln, arg0, arg1,
	                       0, NO_CRUSH, 0, false, false);
}

FUNC(LS_Floor_Waggle)
// Floor_Waggle (tag, amplitude, frequency, delay, time)
{
	return EV_StartPlaneWaggle(arg0, ln, arg1, arg2, arg3, arg4, false);
}

FUNC(LS_Ceiling_Waggle)
// Ceiling_Waggle (tag, amplitude, frequency, delay, time)
{
	return EV_StartPlaneWaggle(arg0, ln, arg1, arg2, arg3, arg4, true);
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
	return EV_DoZDoomDonut(arg0, ln, P_ArgToSpeed(arg1), P_ArgToSpeed(arg2));
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

	return EV_DoZDoomFloor(type, ln, arg0, arg1, arg2, (arg4 & 16) ? 20 : NO_CRUSH,
	                arg4 & 7, false, false);

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
	return EV_BuildStairs(arg0, DFloor::EStair::buildUp, ln, arg2 * FRACUNIT, SPEED(arg1),
	                      arg3,
	                      arg4, 0, 0);
}

FUNC(LS_Stairs_BuildDownDoom)
// Stairs_BuildDownDoom (tag, speed, height, delay, reset)
{
	return EV_BuildStairs(arg0, DFloor::EStair::buildDown, ln, arg2 * FRACUNIT,
	                      SPEED(arg1), arg3,
	                      arg4, 0, 0);
}

FUNC(LS_Stairs_BuildDownDoomSync)
// Stairs_BuildDownDoomSync (tag, speed, height, reset)
{
	return EV_BuildStairs(arg0, DFloor::EStair::buildDown, ln, arg2 * FRACUNIT,
	                      SPEED(arg1), 0, arg3,
	                      0, 2);
}

FUNC(LS_Stairs_BuildUpDoomSync)
// Stairs_BuildUpDoomSync (tag, speed, height, reset)
{
	return EV_BuildStairs(arg0, DFloor::EStair::buildUp, ln, arg2 * FRACUNIT, SPEED(arg1),
	                      0, arg3, 0, 2);
}

FUNC(LS_Stairs_BuildUpDoomCrush)
// Stairs_BuildUpDoomCrush (tag, speed, height, delay, reset)
{
	return EV_BuildStairs(arg0, DFloor::EStair::buildUp, ln, arg2 * FRACUNIT, SPEED(arg1),
	                      arg3, arg4, 0, 1);
}

FUNC(LS_Generic_Stairs)
// Generic_Stairs (tag, speed, step, dir/igntxt, reset)
{
	DFloor::EStair type = (arg3 & 1) ? DFloor::buildUp : DFloor::buildDown;
	BOOL res = EV_BuildStairs (arg0, type, ln,
							   arg2 * FRACUNIT, SPEED(arg1), 0, arg4, arg3 & 2, 0);

	if (res && ln && (ln->flags & ML_REPEATSPECIAL) && ln->special == Generic_Stairs)
		// Toggle direction of next activation of repeatable stairs
		arg3 ^= 1;

	return res;
}

FUNC(LS_Pillar_Build)
// Pillar_Build (tag, speed, height)
{
	return EV_DoPillar (DPillar::pillarBuild, arg0, SPEED(arg1), arg2*FRACUNIT, 0, -1);
}

FUNC(LS_Pillar_BuildAndCrush)
// Pillar_Build (tag, speed, height)
{
	return EV_DoZDoomPillar(DPillar::EPillar::pillarBuild, ln, arg0,
	                        P_ArgToSpeed(arg1), arg2, 0, arg3,
	                        P_ArgToCrushType(arg4));
}

FUNC(LS_Pillar_Open)
// Pillar_Open (tag, speed, f_height, c_height)
{
	return EV_DoZDoomPillar(DPillar::EPillar::pillarOpen, ln, arg0,
	                        P_ArgToSpeed(arg1), arg2, arg3, NO_CRUSH, false);
}

FUNC(LS_Ceiling_LowerByValue)
// Ceiling_LowerByValue (tag, speed, height)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerByValue, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, arg2, NO_CRUSH, 0,
	                         0, crushDoom);
}

FUNC(LS_Ceiling_RaiseByValue)
// Ceiling_RaiseByValue (tag, speed, height)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseByValue, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, arg2, NO_CRUSH, 0, 0, crushDoom);
}

FUNC(LS_Ceiling_LowerByValueTimes8)
// Ceiling_LowerByValueTimes8 (tag, speed, height)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerByValue, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, (int)arg2 * 8, NO_CRUSH, 0, 0,
	                         crushDoom);
}

FUNC(LS_Ceiling_RaiseByValueTimes8)
// Ceiling_RaiseByValueTimes8 (tag, speed, height)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseByValue, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, (int)arg2 * 8, NO_CRUSH, 0, 0,
	                         crushDoom);
}

FUNC(LS_Ceiling_CrushAndRaise)
// Ceiling_CrushAndRaise (tag, speed, damage [, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0,
	                  P_ArgToSpeed(arg1), P_ArgToSpeed(arg1) / 2, 8, arg2, 0, 0,
	                  (crushmode_e)P_ArgToCrushMode(arg3, false));
}

FUNC(LS_Ceiling_CrushAndRaiseDist)
// Ceiling_CrushAndRaiseDist (tag, dist, speed, damage [, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0,
	                  P_ArgToSpeed(arg2), P_ArgToSpeed(arg2), arg1, arg3, 0,
	                  0, (crushmode_e)P_ArgToCrushMode(arg4, arg2 == 8));
}

FUNC(LS_Ceiling_LowerAndCrush)
// Ceiling_LowerAndCrush (tag, speed, crush [, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerAndCrush, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg1), 8, arg2, 0,
	                         0, (crushmode_e)P_ArgToCrushMode(arg3, arg1 == 8));
}

FUNC(LS_Ceiling_LowerAndCrushDist)
// Ceiling_LowerAndCrushDist (tag, speed, crush [, dist[, crushmode]])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerAndCrush, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg1), arg3,
	                         arg2, 0, 0, (crushmode_e)P_ArgToCrushMode(arg4, arg1 == 8));
}

FUNC(LS_Ceiling_CrushStop)
// Ceiling_CrushStop (tag)
{
	return EV_ZDoomCeilingCrushStop(arg0, false);
}

FUNC(LS_Ceiling_CrushRaiseAndStay)
// Ceiling_CrushRaiseAndStay (tag, speed, crush [, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushRaiseAndStay, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg1) / 2, 8, arg2,
	                         0, 0, (crushmode_e)P_ArgToCrushMode(arg3, false));
}

FUNC(LS_Ceiling_MoveToValueTimes8)
// Ceiling_MoveToValueTimes8 (tag, speed, height, negative)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilMoveToValue, ln, arg0,
	                         P_ArgToSpeed(arg1), 0,
	                         (int)arg2 * 8 * (arg3 ? -1 : 1), NO_CRUSH, 0,
	                         0, crushDoom);
}

FUNC(LS_Ceiling_MoveToValue)
// Ceiling_MoveToValue (tag, speed, height, negative)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilMoveToValue, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, (int)arg2 * (arg3 ? -1 : 1),
	                         NO_CRUSH, 0, 0, crushDoom);
}

FUNC(LS_Ceiling_LowerToHighestFloor)
// Ceiling_LowerToHighestFloor (tag, speed)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToHighestFloor, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, arg4, NO_CRUSH, 0, 0, crushDoom);
}

FUNC(LS_Ceiling_LowerInstant)
// Ceiling_LowerInstant (tag, unused, height)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerInstant, ln, arg0, 0, 0,
	                         (int)arg2 * 8, NO_CRUSH, 0, 0,
	                         crushDoom);
}

FUNC(LS_Ceiling_RaiseInstant)
// Ceiling_RaiseInstant (tag, unused, height)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseInstant, ln, arg0, 0, 0,
	                         (int)arg2 * 8, NO_CRUSH, 0, 0, crushDoom);
}

FUNC(LS_Ceiling_CrushRaiseAndStayA)
// Ceiling_CrushRaiseAndStayA (tag, dnspeed, upspeed, damage[, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushRaiseAndStay, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg2), 0, arg3, 0, 0,
	                         (crushmode_e)P_ArgToCrushMode(arg4, false));
}

FUNC(LS_Ceiling_CrushRaiseAndStaySilA)
// Ceiling_CrushRaiseAndStaySilA (tag, dnspeed, upspeed, damage[, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushRaiseAndStay, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg2), 0, arg3, 1,
	                         0, (crushmode_e)P_ArgToCrushMode(arg4, false));
}

FUNC(LS_Ceiling_CrushAndRaiseA)
// Ceiling_CrushAndRaiseA (tag, dnspeed, upspeed, damage[, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg2), 0, arg3, 0, 0,
	                         (crushmode_e)P_ArgToCrushMode(arg4, arg1 == 8 && arg2 == 8));
}

FUNC(LS_Ceiling_CrushAndRaiseSilentA)
// Ceiling_CrushAndRaiseSilentA (tag, dnspeed, upspeed, damage[, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg2), 0, arg3, 1, 0,
	                         (crushmode_e)P_ArgToCrushMode(arg4, arg1 == 8 && arg2 == 8));
}

FUNC(LS_Ceiling_CrushAndRaiseSilentDist)
// Ceiling_CrushAndRaiseSilentDist (tag, dist, speed, damage[, crushmode])
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg2), 0, arg3, 1, 0,
	                         (crushmode_e)P_ArgToCrushMode(arg4, arg1 == 8 && arg2 == 8));
}

FUNC(LS_Ceiling_RaiseToNearest)
// Ceiling_RaiseToNearest (tag, speed)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToNearest, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, NO_CRUSH,
	                         0, 0, crushDoom);
}

FUNC(LS_Ceiling_RaiseToHighest)
// Ceiling_RaiseToHighest (tag, speed, change)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighest, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, NO_CRUSH, 0, P_ArgToChange(arg2),
	                         crushDoom);
}

FUNC(LS_Ceiling_RaiseToHighestFloor)
// Ceiling_RaiseToHighestFloor (tag, speed, change)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighestFloor, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, NO_CRUSH, 0, P_ArgToChange(arg2),
	                         crushDoom);
}

FUNC(LS_Ceiling_RaiseToLowest)
// Ceiling_RaiseToLowest (tag, speed, change)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToLowest, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, NO_CRUSH, 0, P_ArgToChange(arg2),
	                         crushDoom);
}

FUNC(LS_Ceiling_RaiseByTexture)
// Ceiling_RaiseByTexture (tag, speed, change)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseByTexture, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, NO_CRUSH, P_ArgToChange(arg2), 0,
	                         crushDoom);
}

FUNC(LS_Ceiling_LowerToLowest)
// Ceiling_LowerToLowest (tag, speed)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToLowest, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, NO_CRUSH, 0,
	                         0, crushDoom);
}

FUNC(LS_Ceiling_ToHighestInstant)
// Ceiling_ToHighestInstant (tag, change, crush)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToHighest, ln, arg0,
	                         2 * FRACUNIT, 0, 0, P_ArgToCrush(arg2), 0,
	                         P_ArgToChange(arg1), crushDoom);
}

FUNC(LS_Ceiling_ToFloorInstant)
// Ceiling_ToFloorInstant (tag, change, crush, gap)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToHighest, ln, arg0,
	                         2 * FRACUNIT, 0, arg3, P_ArgToCrush(arg2), 0,
	                         P_ArgToChange(arg1), crushDoom);
}

FUNC(LS_Ceiling_LowerToNearest)
// Ceiling_LowerToNearest (tag, speed, change, crush)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToNearest, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, P_ArgToCrush(arg3), 0,
	                         P_ArgToChange(arg2), crushDoom);
}

FUNC(LS_Ceiling_LowerToFloor)
// Ceiling_LowerToFloor (tag, speed)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerToFloor, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, NO_CRUSH, 0, 0,
	                         0, crushDoom);
}

FUNC(LS_Ceiling_LowerByTexture)
// Ceiling_LowerByTexture (tag, speed, change, crush)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilLowerByTexture, ln, arg0,
	                         P_ArgToSpeed(arg1), 0, 0, P_ArgToCrush(arg3), 0,
	                         P_ArgToChange(arg4), crushDoom);
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

	return EV_DoZDoomCeiling(type, ln, arg0, P_ArgToSpeed(arg1),
	                         P_ArgToSpeed(arg1), arg2,
	                         (arg4 & 16) ? 20 : NO_CRUSH, 0, arg4 & 7, crushDoom);
}

FUNC(LS_Generic_Crusher)
// Generic_Crusher (tag, dnspeed, upspeed, silent, damage)
{
	return EV_DoZDoomCeiling(
	    DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0, P_ArgToSpeed(arg1),
	    P_ArgToSpeed(arg2), 0, arg4, arg3 ? 2 : 0, 0,
	    (arg1 <= 24 && arg2 <= 24) ? crushSlowdown : crushDoom);
}

FUNC(LS_Generic_Crusher2)
// Generic_Crusher2 (tag, dnspeed, upspeed, silent, damage)
{
	return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilCrushAndRaise, ln, arg0,
	                         P_ArgToSpeed(arg1), P_ArgToSpeed(arg2), 0, arg4,
	                         arg3 ? 2 : 0, 0, crushHexen);
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

FUNC(LS_Plat_UpNearestWaitDownStay)
// Plat_UpNearestWaitDownStay (tag, speed, delay)
{
	return EV_DoPlat(arg0, ln, DPlat::platUpNearestWaitDownStay, 0, SPEED(arg1),
	                 TICS(arg2), 0, 0);
}

FUNC(LS_Plat_RaiseAndStayTx0)
// Plat_RaiseAndStayTx0 (tag, speed, lockout)
{
	DPlat::EPlatType type;

	switch (arg3)
	{
	case 1:
		type = DPlat::EPlatType::platRaiseAndStay;
		break;
	case 2:
		type = DPlat::EPlatType::platRaiseAndStayLockout;
		break;
	default:
		type = DPlat::EPlatType::platRaiseAndStay;
		break;
	}

	return EV_DoPlat(arg0, ln, type, 0, P_ArgToSpeed(arg1), 0, 0, 1);
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

FUNC(LS_Line_SetBlocking)
// Line_SetBlocking (tag, setflags, clearflags)
{
	if (arg0)
	{
		int i, s;
		static const int flags[] = {ML_BLOCKING,
		                            ML_BLOCKMONSTERS,
		                            ML_BLOCKPLAYERS,
		                            0, // block floaters (not supported)
		                            0, // block projectiles (not supported)
		                            ML_BLOCKEVERYTHING,
		                            0, // railing (not supported)
		                            0, // block use (not supported)
		                            0, // block sight (not supported)
		                            0, // block hitscan (not supported)
		                            ML_SOUNDBLOCK,
		                            -1};

		int setflags = 0;
		int clearflags = 0;

		for (i = 0; flags[i] != -1; i++, arg1 >>= 1, arg2 >>= 1)
		{
			if (arg1 & 1)
				setflags |= flags[i];
			if (arg2 & 1)
				clearflags |= flags[i];
		}

		for (s = -1; (s = P_FindLineFromTag(arg0, s)) >= 0;)
		{
			lines[s].flags = (lines[s].flags & ~clearflags) | setflags;
		}

		return true;
	}
	return false;
}

FUNC(LS_Scroll_Wall)
// Scroll_Wall (lineid, x, y, side, flags)
{
	if (arg4)
	{
		Printf(PRINT_HIGH,
		       "Warning: Odamex can only scroll entire sidedefs (special 52)");
	}
	if (arg0)
	{
		int s;
		int side = !!arg3;

		for (s = -1; (s = P_FindLineFromTag(arg0, s)) >= 0;)
		{
			new DScroller(DScroller::EScrollType::sc_side, arg1, arg2, -1,
			              lines[s].sidenum[side], 0);
		}

		return true;
	}
	return false;
}

FUNC(LS_Line_SetTextureOffset)
// Line_SetTextureOffset (lineid, x, y, side, flags)
{
	if (arg4 & 7)
	{
		Printf(PRINT_HIGH,
		       "Warning: Odamex can only offset entire sidedefs (special 53)");
	}
	if (arg0 && arg3 <= 1)
	{
		int s;
		int sidenum = !!arg3;

		for (s = -1; (s = P_FindLineFromTag(arg0, s)) >= 0;)
		{
			side_t* side = &sides[lines[s].sidenum[sidenum]];
			side->textureoffset = arg1;
			side->rowoffset = arg2;
		}

		return true;
	}
	return false;
}

FUNC(LS_Noise_Alert)
// Noise_Alert (target, emitter)
{
	AActor *target, *emitter;
	bool noise = false;

	if (!arg0)
	{
		target = it;
	}
	else
	{
		// not supported yet
		target = NULL;
	}

	if (!arg1)
	{
		emitter = it;
	}
	else
	{
		// not supported yet
		emitter = NULL;
	}

	if (emitter)
	{
		P_NoiseAlert(target, emitter);
		noise = true;
	}

	return noise;
}

FUNC(LS_Sector_SetDamage)
// Sector_SetDamage (tag, amount, mod, interval, leaky)
{
	int s = -1;
	bool unblockable = false;

	if (arg3 == 0)
	{
		if (arg1 < 20)
		{
			arg4 = 0;
			arg3 = 32;
		}
		else if (arg1 < 50)
		{
			arg4 = 5;
			arg3 = 32;
		}
		else
		{
			unblockable = true;
			arg4 = 0;
			arg3 = 1;
		}
	}

	while ((s = P_FindSectorFromTag(arg0, s)) >= 0)
	{
		sectors[s].damage->amount = arg1;
		sectors[s].damage->interval = arg3;
		sectors[s].damage->leakrate = arg4;
		if (unblockable)
			sectors[s].flags |= SECF_DMGUNBLOCKABLE;
		else
			sectors[s].flags &= ~SECF_DMGUNBLOCKABLE;
	}
	return true;
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
		LevelInfos& levels = getLevelInfos();
		level_pwad_info_t info = levels.findByNum(arg0);

		if (it && (info.levelnum != 0 && CheckIfExitIsGood(it)))
		{
			level.nextmap = info.mapname;
			G_ExitLevel(arg1, 1);
			return true;
		}
	}
	return false;
}

FUNC(LS_Teleport)
// Teleport (tid, tag, nosourcefog)
{
	if(!it) return false;
	BOOL result;

	int flags = TELF_DESTFOG;

	if (!arg2)
		flags |= TELF_SOURCEFOG;

	result = EV_CompatibleTeleport(arg1, ln, TeleportSide, it, flags);

	return result;
}

FUNC(LS_Teleport_NoStop)
// Teleport_NoStop(tid, tag, nofog)
{
	if (!it) return false;
	int flags = TELF_DESTFOG | TELF_KEEPVELOCITY;

	if (!arg2)
		flags |= TELF_SOURCEFOG;

	return EV_CompatibleTeleport(arg1, ln, TeleportSide, it, flags);
}

FUNC(LS_Teleport_NoFog)
// Teleport_NoFog (tid, useangle, tag, keepheight)
{
	if(!it)
		return false;

	int flags = 0;

	switch (arg1)
	{
	case 0:
		flags |= TELF_KEEPORIENTATION;
		break;

	case 2:
		if (ln)
			flags |= TELF_KEEPORIENTATION | TELF_ROTATEBOOM;
		break;

	case 3:
		if (ln)
			flags |= TELF_KEEPORIENTATION | TELF_ROTATEBOOMINVERSE;
		break;

	default:
		break;
	}

	if (arg3)
		flags |= TELF_KEEPHEIGHT;

	return EV_CompatibleTeleport(arg2, ln, TeleportSide, it, flags);
}

FUNC(LS_Teleport_EndGame)
// Teleport_EndGame ()
{
	if (!TeleportSide && it && CheckIfExitIsGood (it))
	{
		M_CommitWDLLog();
		level.nextmap = "EndGameC";
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
// DamageThing (damage, mod)
{
	if(!it) return false;

	if (arg0)
		P_DamageMobj (it, NULL, NULL, arg0, arg1);
	else
		P_DamageMobj (it, NULL, NULL, 10000, arg1);

	return true;
}

FUNC(LS_Thing_Damage)
// Thing_Damage (tid, amount, mod)
{
	if (!arg0)
		return false;

	TThinkerIterator<AActor> iterator;
	AActor* actor;
	while ((actor = iterator.Next()))
	{
		if (actor->tid == arg0)
		{
			if (arg1)
			{
				P_DamageMobj(it, NULL, NULL, arg1, arg2);
			}
			else
			{
				P_DamageMobj(it, NULL, NULL, 10000, arg2);
			}
		}
	}

	return true;
}

FUNC(LS_ForceField)
// ForceField (damage)
{
	if (!it)
		return false;

	P_DamageMobj(it, NULL, NULL, 16);
	P_ThrustMobj(it, ANG180 + it->angle, 2048 * 250);

	return true;
}

FUNC(LS_Clear_ForceField)
// ForceField (damage)
{
	int s = -1;
	bool clear = false;

	while ((s = P_FindSectorFromTag(arg0, s)) >= 0)
	{
		int i;
		line_t* line;

		for (i = 0; i < sectors[s].linecount; i++)
		{
			line_t* line = sectors[s].lines[i];

			clear = true;

			if (line->backsector && line->special == ForceField)
			{
				line->flags &= ~(ML_BLOCKING | ML_BLOCKEVERYTHING);
				line->special = 0;
				sides[line->sidenum[0]].midtexture = NO_TEXTURE;
				sides[line->sidenum[1]].midtexture = NO_TEXTURE;
			}
		}
	}

	return clear;
}

ItemEquipVal P_GiveBody (player_t *, int);

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
	if (!serverside)
		return false;

	LevelInfos& levels = getLevelInfos();
	level_pwad_info_t& info = levels.findByNum(arg1);

	if (arg1 == 0 || !info.exists())
		return P_StartScript(it, ln, arg0, ::level.mapname.c_str(), TeleportSide, arg2, arg3, arg4, 0);

	return P_StartScript(it, ln, arg0, info.mapname.c_str(), TeleportSide, arg2, arg3, arg4, 0);
}

FUNC(LS_ACS_ExecuteAlways)
// ACS_ExecuteAlways (script, map, s_arg1, s_arg2, s_arg3)
{
	if (!serverside)
		return false;

	LevelInfos& levels = getLevelInfos();
	level_pwad_info_t& info = levels.findByNum(arg1);

	if (arg1 == 0 || !info.exists())
		return P_StartScript(it, ln, arg0, ::level.mapname.c_str(), TeleportSide, arg2, arg3, arg4, 1);

	return P_StartScript(it, ln, arg0, info.mapname.c_str(), TeleportSide, arg2, arg3, arg4, 1);
}

FUNC(LS_ACS_LockedExecute)
// ACS_LockedExecute (script, map, s_arg1, s_arg2, lock)
{
	if (!serverside)
		return false;

	if (arg4 && !P_CheckKeys (it->player, (card_t)arg4, 1))
		return false;
	else
		return LS_ACS_Execute (ln, it, arg0, arg1, arg2, arg3, 0);
}

FUNC(LS_ACS_Suspend)
// ACS_Suspend (script, map)
{
	if (!serverside)
		return false;

	LevelInfos& levels = getLevelInfos();
	level_pwad_info_t& info = levels.findByNum(arg1);

	if (arg1 == 0 || !info.exists())
		P_SuspendScript(arg0, ::level.mapname.c_str());
	else
		P_SuspendScript(arg0, info.mapname.c_str());

	return true;
}

FUNC(LS_ACS_Terminate)
// ACS_Terminate (script, map)
{
	if (!serverside)
		return false;

	level_pwad_info_t& info = getLevelInfos().findByNum(arg1);

	if (arg1 == 0 || !info.exists())
		P_TerminateScript(arg0, ::level.mapname.c_str());
	else
		P_TerminateScript(arg0, info.mapname.c_str());

	return true;
}

FUNC(LS_FloorAndCeiling_LowerByValue)
// FloorAndCeiling_LowerByValue (tag, speed, height)
{
	return EV_DoZDoomElevator(ln, DElevator::EElevator::elevateLower,
	                          P_ArgToSpeed(arg1), arg2, arg0);
}

FUNC(LS_FloorAndCeiling_RaiseByValue)
// FloorAndCeiling_RaiseByValue (tag, speed, height)
{
	return EV_DoZDoomElevator(ln, DElevator::EElevator::elevateRaise,
	                          P_ArgToSpeed(arg1), arg2, arg0);
}

FUNC(LS_FloorAndCeiling_LowerRaise)
// FloorAndCeiling_LowerRaise (tag, fspeed, cspeed, boomemu)
{
	if (arg3 == 1998)
	{
		return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighest, ln, arg0,
		                         P_ArgToSpeed(arg2), 0, 0, 0, 0, 0, crushDoom) ||
		       EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowest, ln, arg0, arg1, 0,
		                       NO_CRUSH, 0, false, crushDoom);
	}
	else
	{
		return EV_DoZDoomCeiling(DCeiling::ECeiling::ceilRaiseToHighest, ln, arg0,
		                         P_ArgToSpeed(arg2), 0, 0, 0, 0, 0, crushDoom) &&
		       EV_DoZDoomFloor(DFloor::EFloor::floorLowerToLowest, ln, arg0, arg1, 0,
		                       NO_CRUSH, 0, false, crushDoom);
	}
}

FUNC(LS_Elevator_MoveToFloor)
// Elevator_MoveToFloor (tag, speed)
{
	return EV_DoZDoomElevator(ln, DElevator::EElevator::elevateCurrent,
	                          P_ArgToSpeed(arg1), 0, arg0);
}

FUNC(LS_Elevator_RaiseToNearest)
// Elevator_RaiseToNearest (tag, speed)
{
	return EV_DoZDoomElevator(ln, DElevator::EElevator::elevateUp,
	                          P_ArgToSpeed(arg1), 0, arg0);
}

FUNC(LS_Elevator_LowerToNearest)
// Elevator_LowerToNearest (tag, speed)
{
	return EV_DoZDoomElevator(ln, DElevator::EElevator::elevateDown,
	                          P_ArgToSpeed(arg1), 0, arg0);
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
	EV_LightSetMinNeighbor(arg0);
	return true;
}

FUNC(LS_Light_MaxNeighbor)
// Light_MaxNeighbor (tag)
{
	EV_LightSetMaxNeighbor(arg0);
	return true;
}

FUNC(LS_Light_Stop)
// Light_Stop (tag)
{
	EV_TurnTagLightsOff(arg0);
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
	if (!serverside && !s_SpecialFromServer)
		return false;

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

static void SetScroller(int tag, DScroller::EScrollType type, fixed_t dx, fixed_t dy)
{
	TThinkerIterator<DScroller> iterator;
	DScroller *scroller;
	int i;

	// Check if there is already a scroller for this tag
	// If at least one sector with this tag is scrolling, then they all are.
	// If the deltas are both 0, we don't remove the scroller, because a
	// displacement/accelerative scroller might have been set up, and there's
	// no way to create one after the level is fully loaded.
	i = 0;
	while ((scroller = iterator.Next()))
	{
		if (scroller->IsType(type))
		{
			if (sectors[scroller->GetAffectee()].tag == tag)
			{
				i++;
				scroller->SetRate(dx, dy);
			}
		}
	}

	if (i > 0 || (dx | dy) == 0)
	{
		return;
	}

	// Need to create scrollers for the sector(s)
	for (i = -1; (i = P_FindSectorFromTag(tag, i)) >= 0; )
	{
		new DScroller(type, dx, dy, -1, i, 0);
	}
}

FUNC(LS_Scroll_Floor)
{
	if (IgnoreSpecial)
		return false;

	fixed_t dx = arg1 * FRACUNIT / 32;
	fixed_t dy = arg2 * FRACUNIT / 32;

	if (arg3 == 0 || arg3 == 2)
	{
		SetScroller(arg0, DScroller::sc_floor, -dx, dy);
	}
	else
	{
		SetScroller(arg0, DScroller::sc_floor, 0, 0);
	}
	if (arg3 > 0)
	{
		SetScroller(arg0, DScroller::sc_carry, dx, dy);
	}
	else
	{
		SetScroller(arg0, DScroller::sc_carry, 0, 0);
	}
	return true;
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

FUNC(LS_Sector_SetGravity)
// Sector_SetGravity (tag, intpart, fracpart)
{
	int secnum = -1;
	float gravity;

	if (arg2 > 99)
		arg2 = 99;
	gravity = (float)arg1 + (float)arg2 * 0.01f;

	while ((secnum = P_FindSectorFromTag(arg0, secnum)) >= 0)
	{
		sectors[secnum].gravity = gravity;
		sectors[secnum].SectorChanges |= SPC_Gravity;
	}

	return true;
}

FUNC(LS_Sector_SetColor)
// Sector_SetColor (tag, r, g, b)
{
	int secnum = -1;
	while ((secnum = P_FindSectorFromTag(arg0, secnum)) >= 0)
	{
		sectors[secnum].colormap = GetSpecialLights(arg1, arg2, arg3,
				sectors[secnum].colormap->fade.getr(),
				sectors[secnum].colormap->fade.getg(),
				sectors[secnum].colormap->fade.getb());
		sectors[secnum].SectorChanges |= SPC_Color;
	}
	return true;
}

FUNC(LS_Sector_SetFade)
// Sector_SetFade (tag, r, g, b)
{
	int secnum = -1;
	while ((secnum = P_FindSectorFromTag(arg0, secnum)) >= 0)
	{
		sectors[secnum].colormap = GetSpecialLights(
				sectors[secnum].colormap->color.getr(),
				sectors[secnum].colormap->color.getg(),
				sectors[secnum].colormap->color.getb(),
				arg1, arg2, arg3);
		byte r = sectors[secnum].colormap->fade.getr();
		byte g = sectors[secnum].colormap->fade.getg();
		byte b = sectors[secnum].colormap->fade.getb();
		sectors[secnum].SectorChanges |= SPC_Fade;
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
		sectors[secnum].SectorChanges |= SPC_Panning;
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
		sectors[secnum].SectorChanges |= SPC_Panning;
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
		sectors[secnum].SectorChanges |= SPC_Scale;
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
		sectors[secnum].SectorChanges |= SPC_Scale;
	}
	return true;
}

FUNC(LS_Sector_SetRotation)
// Sector_SetRotation (tag, floor-angle, ceiling-angle)
{
	int secnum = -1;
	angle_t ceiling = ANG(arg2);
	angle_t floor = ANG(arg1);

	while ((secnum = P_FindSectorFromTag (arg0, secnum)) >= 0)
	{
		sectors[secnum].floor_angle = floor;
		sectors[secnum].ceiling_angle = ceiling;
		sectors[secnum].SectorChanges |= SPC_Rotation;
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
	if (IgnoreSpecial)
		return false;

	int linenum = -1;
	while ((linenum = P_FindLineFromID (arg0, linenum)) >= 0)
	{
		lines[linenum].lucency = arg1 & 255;
		lines[linenum].PropertiesChanged = true;
	}

	return true;
}

// ZDoom line specials
lnSpecFunc LineSpecials[283] =
{
	LS_NOP,
	LS_NOP,		// Polyobj_StartLine,
	LS_Polyobj_RotateLeft,
	LS_Polyobj_RotateRight,
	LS_Polyobj_Move,
	LS_NOTIMP,	// Polyobj_ExplicitLine
	LS_Polyobj_MoveTimes8,
	LS_Polyobj_DoorSwing,
	LS_Polyobj_DoorSlide,
	LS_NOP,		// 9 Line Horizon (handled elsewhere)
	LS_Door_Close,
	LS_Door_Open,
	LS_Door_Raise,
	LS_Door_LockedRaise,
    LS_NOTIMP, // 14 Door Animated (Not Supported)
    LS_NOTIMP, // 15 Autosave (Not Supported)
    LS_NOTIMP, // 16 Transfer_WallLight (Not Supported)
    LS_NOTIMP, // 17 Thing_Raise (Not Supported)
    LS_NOTIMP, // 18 StartConversation (Not Supported)
	LS_Thing_Stop,
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
	LS_ForceField,
	LS_Clear_ForceField,
	LS_Floor_RaiseByValueTimes8,
	LS_Floor_LowerByValueTimes8,
	LS_Floor_MoveToValue,
	LS_Ceiling_Waggle,
    LS_NOTIMP, // 39 Teleport_ZombieChanger (not supported)
	LS_Ceiling_LowerByValue,
	LS_Ceiling_RaiseByValue,
	LS_Ceiling_CrushAndRaise,
	LS_Ceiling_LowerAndCrush,
	LS_Ceiling_CrushStop,
	LS_Ceiling_CrushRaiseAndStay,
	LS_Floor_CrushStop,
	LS_Ceiling_MoveToValue,
    LS_NOTIMP, // 48 Sector_Attach3dMidtex (not supported)
    LS_NOTIMP, // 49 GlassBreak (not supported)
    LS_NOTIMP, // 50 ExtraFloor_LightOnly (not supported)
    LS_NOTIMP, // 51 Sector_SetLink (not supported)
	LS_Scroll_Wall,
	LS_Line_SetTextureOffset,
    LS_NOTIMP, // 54 Sector_ChangeFlags (not supported)
	LS_Line_SetBlocking,
    LS_NOTIMP, // 56 Line_SetTextureScale (not supported)
    LS_NOTIMP, // 57 Sector_SetPortal (not supported)
    LS_NOTIMP, // 58 Sector_CopyScroller (not supported)
    LS_NOTIMP, // 59 Polyobj_OR_MoveToSpot (not supported)
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
    LS_NOTIMP, // 76 Teleport_Other (not supported)
    LS_NOTIMP, // 77 Teleport_Group (not supported)
    LS_NOTIMP, // 78 Teleport_InSector (not supported)
    LS_NOTIMP, // 79 Thing_SetConversation (not supported)
	LS_ACS_Execute,
	LS_ACS_Suspend,
	LS_ACS_Terminate,
	LS_ACS_LockedExecute,
    LS_NOTIMP, // 84 ACS_ExecuteWithResult (not supported)
    LS_NOTIMP, // 85 ACS_LockedExecuteDoor (not supported)
    LS_NOTIMP, // 86 Polyobj_MoveToSpot (not supported)
    LS_NOTIMP, // 87 Polyobj_Stop (not supported)
    LS_NOTIMP, // 88 Polyobj_MoveTo (not supported)
    LS_NOTIMP, // 89 Polyobj_OR_MoveTo (not supported)
	LS_Polyobj_OR_RotateLeft,
	LS_Polyobj_OR_RotateRight,
	LS_Polyobj_OR_Move,
	LS_Polyobj_OR_MoveTimes8,
	LS_Pillar_BuildAndCrush,
	LS_FloorAndCeiling_LowerByValue,
	LS_FloorAndCeiling_RaiseByValue,
	LS_Ceiling_LowerAndCrushDist,
	LS_NOP,		// 98 Sector_SetTranslucent (not supported/handled elsewhere?)
    LS_Floor_RaiseAndCrushDoom,
    LS_NOP, // 100 Scroll_Texture_Left (handled elsewhere)
	LS_NOP, // 101 Scroll_Texture_Right (handled elsewhere)
    LS_NOP, // 102 Scroll_Texture_Up (handled elsewhere)
    LS_NOP, // 103 Scroll_Texture_Down (handled elsewhere)
    LS_Ceiling_CrushAndRaiseSilentDist,
    LS_Door_WaitRaise,
    LS_Door_WaitClose,
    LS_NOTIMP,  // 107 Line_SetPortalTarget (not supported)
	LS_NOP,		// 108 (unused)
    LS_NOTIMP,  // 109 Light_ForceLightning (not supported)
	LS_Light_RaiseByValue,
	LS_Light_LowerByValue,
	LS_Light_ChangeToValue,
	LS_Light_Fade,
	LS_Light_Glow,
	LS_Light_Flicker,
	LS_Light_Strobe,
	LS_Light_Stop,
	LS_NOP,		// 118 Plane_Copy (handled elsewhere/not supported)
    LS_Thing_Damage,
	LS_Radius_Quake,
	LS_NOP,		// 123 Set_LineIdentification (handled elsewhere)
	LS_NOP,		// 122 (unused)
	LS_NOP,		// 123 (unused)
	LS_NOP,		// 124 (unused)
    LS_NOTIMP,  // 125 Thing_Move (not supported)
	LS_NOP,		// 126 (unused)
    LS_NOTIMP,  // 127 Thing_SetSpecial (not supported)
	LS_ThrustThingZ,
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
    LS_NOTIMP,  // 139 Thing_SpawnFacing (not supported)
	LS_Sector_ChangeSound,
	LS_NOP,		// 141 (unused)
	LS_NOP,		// 142 (unused)
	LS_NOP,		// 143 (unused)
	LS_NOP,		// 144 (unused)
    LS_NOTIMP,  // 145 Player_SetTeam (not supported)
	LS_NOP,		// 146 (unused)
	LS_NOP,		// 147 (unused)
	LS_NOP,		// 148 (unused)
	LS_NOP,		// 149 (unused)
	LS_NOP,		// 150 (unused)
	LS_NOP,		// 151 (unused)
    LS_NOTIMP,  // 152 Team_Score (not supported)
    LS_NOTIMP,  // 153 Team_GivePoints (not supported)
	LS_Teleport_NoStop,
	LS_NOP,		// 155 (unused)
    LS_NOTIMP,  // 156 Line_SetPortal (not supported)
    LS_NOTIMP,  // 157 Set_GlobalFogParameter (not supported)
    LS_NOTIMP,  // 158 FS_Execute (not supported)
    LS_NOTIMP,  // 159 Sector_SetPlaneReflection (not supported)
    LS_NOTIMP,  // 160 Sector_Set3dFloor (not supported)
    LS_NOTIMP,  // 161 Sector_SetContents (not supported)
	LS_NOP,		// 162 (unused)
	LS_NOP,		// 163 (unused)
	LS_NOP,		// 164 (unused)
	LS_NOP,		// 165 (unused)
	LS_NOP,		// 166 (unused)
	LS_NOP,		// 167 (unused)
    LS_Ceiling_CrushAndRaiseDist,
    LS_Generic_Crusher2,
    LS_NOTIMP, // 170 Sector_SetCeilingScale2 (not supported)
    LS_NOTIMP, // 171 Sector_SetFloorScale2 (not supported)
    LS_Plat_UpNearestWaitDownStay,
    LS_Noise_Alert,
    LS_NOTIMP, // 174 SendToCommunicator (not supported)
    LS_NOTIMP, // 175 Thing_ProjectileIntercept (not supported)
    LS_NOTIMP, // 176 Thing_ChangeTID (not supported)
    LS_NOTIMP, // 177 Thing_Hate (not supported)
    LS_NOTIMP, // 178 Thing_ProjectileAimed (not supported)
    LS_NOTIMP, // 179 Change_Skill (not supported)
    LS_NOTIMP, // 180 Thing_SetTranslation (not supported)
    LS_NOP,    // 181 Plane_Align (handled elsewhere)
    LS_NOTIMP, // 182 Line_Mirror (not supported)
	LS_Line_AlignCeiling,
	LS_Line_AlignFloor,
	LS_Sector_SetRotation,
	LS_Sector_SetCeilingPanning,
	LS_Sector_SetFloorPanning,
	LS_Sector_SetCeilingScale,
	LS_Sector_SetFloorScale,
	LS_NOP,		// 190 Static_Init
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
	LS_NOP,		// 209 Transfer_Heights (handled elsewhere)
	LS_NOP,		// 210 Transfer_FloorLight (handled elsewhere)
	LS_NOP,		// 211 Transfer_CeilingLight (handled elsewhere)
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
    LS_NOP, //222 Scroll_Texture_Model (handled elsewhere)
	LS_Scroll_Floor,
	LS_Scroll_Ceiling,
    LS_NOP, // 225 Scroll_TextureOffsets (handled elsewhere)
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
	LS_Ceiling_CrushRaiseAndStaySilA,
    LS_Floor_LowerToHighestEE,
    LS_Floor_RaiseToLowest,
    LS_Floor_LowerToLowestCeiling,
    LS_Floor_RaiseToCeiling,
    LS_Floor_ToCeilingInstant,
    LS_Floor_LowerByTexture,
    LS_Ceiling_RaiseToHighest,
    LS_Ceiling_ToHighestInstant,
    LS_Ceiling_LowerToNearest,
    LS_Ceiling_RaiseToLowest,
    LS_Ceiling_RaiseToHighestFloor,
    LS_Ceiling_ToFloorInstant,
    LS_Ceiling_RaiseByTexture,
    LS_Ceiling_LowerByTexture,
    LS_Stairs_BuildDownDoom,
    LS_Stairs_BuildUpDoomSync,
    LS_Stairs_BuildDownDoomSync,
	LS_Stairs_BuildUpDoomCrush,
    LS_NOTIMP, // 274 Door_AnimatedClose (not supported)
    LS_NOTIMP, // 275 Floor_Stop (not supported)
    LS_NOTIMP, // 276 Ceiling_Stop (not supported)
    LS_NOTIMP, // 277 Sector_SetFloorGlow (not supported)
    LS_NOTIMP, // 278 Sector_SetCeilingGlow (not supported)
    LS_NOTIMP, // 279 Floor_MoveToValueAndCrush (not supported)
    LS_NOTIMP, // 280 Ceiling_MoveToValueAndCrush (not supported)
    LS_NOTIMP, // 281 Line_SetAutomapFlags (not supported)
    LS_NOTIMP, // 282 Line_SetAutomapStyle (not supported)
};


EXTERN_CVAR (sv_fraglimit)
EXTERN_CVAR (sv_allowexit)
EXTERN_CVAR (sv_fragexitswitch)

BOOL CheckIfExitIsGood (AActor *self)
{
	if (self == NULL || !serverside)
		return false;

	// Bypass the exit restrictions if we're on a lobby.
	if (level.flags & LEVEL_LOBBYSPECIAL)
		return true;	

	// [Toke - dmflags] Old location of DF_NO_EXIT
	if (sv_gametype != GM_COOP && self)
	{
        if (!sv_allowexit)
        {
			if (sv_fragexitswitch && serverside)
				P_DamageMobj(self, NULL, NULL, 10000, MOD_EXIT);

			return false;
		}
	}

	if (self->player && multiplayer)
	{
		OTimespan tspan;
		TicsToTime(tspan, ::level.time);

		std::string tstr;
		if (tspan.hours)
		{
			StrFormat(tstr, "%02d:%02d:%02d.%02d", tspan.hours, tspan.minutes,
			          tspan.seconds, tspan.csecs);
		}
		else
		{
			StrFormat(tstr, "%02d:%02d.%02d", tspan.minutes, tspan.seconds, tspan.csecs);
		}

		SV_BroadcastPrintf("%s exited the level in %s.\n",
		                   self->player->userinfo.netname.c_str(), tstr.c_str());
	}

	M_CommitWDLLog();
	return true;
}

VERSION_CONTROL (p_lnspec_cpp, "$Id$")
