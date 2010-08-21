// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2009 by The Odamex Team.
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

#define FUNC(a) static BOOL a (line_t *ln, AActor *it)

#define SPEED(a)		((a)*(FRACUNIT/8))
#define TICS(a)			(((a)*TICRATE)/35)
#define OCTICS(a)		(((a)*TICRATE)/8)
#define	BYTEANGLE(a)	((angle_t)((a)<<24))

// Set true if this special was activated from inside a script.
BOOL InScript;

FUNC(LS_NOP)
{
	return false;
}

FUNC(LS_Door_Close)
// Door_Close (tag, speed)
{
	return EV_DoDoor (DDoor::doorClose, ln, it, ln->args[0], SPEED(ln->args[1]), 0, NoKey);
}

FUNC(LS_Door_Open)
// Door_Open (tag, speed)
{
	return EV_DoDoor (DDoor::doorOpen, ln, it, ln->args[0], SPEED(ln->args[1]), 0, NoKey);
}

FUNC(LS_Door_Raise)
// Door_Raise (tag, speed, delay)
{
	return EV_DoDoor (DDoor::doorRaise, ln, it, ln->args[0], SPEED(ln->args[1]), TICS(ln->args[2]), NoKey);
}

FUNC(LS_Door_LockedRaise)
// Door_LockedRaise (tag, speed, delay, lock)
{
	return EV_DoDoor (ln->args[2] ? DDoor::doorRaise : DDoor::doorOpen, ln, it,
                          ln->args[0], SPEED(ln->args[1]), TICS(ln->args[2]), (card_t)ln->args[3]);
}

FUNC(LS_Door_CloseWaitOpen)
// Door_CloseWaitOpen (tag, speed, delay)
{
	return EV_DoDoor (DDoor::doorCloseWaitOpen, ln, it, ln->args[0], SPEED(ln->args[1]), OCTICS(ln->args[2]), NoKey);
}

FUNC(LS_Generic_Door)
// Generic_Door (tag, speed, kind, delay, lock)
{
	DDoor::EVlDoor type;

	switch (ln->args[2])
	{
		case 0: type = DDoor::doorRaise;			break;
		case 1: type = DDoor::doorOpen;				break;
		case 2: type = DDoor::doorCloseWaitOpen;	break;
		case 3: type = DDoor::doorClose;			break;
		default: return false;
	}
        return EV_DoDoor (type, ln, it, ln->args[0], SPEED(ln->args[1]), OCTICS(ln->args[3]), (card_t)ln->args[4]);
}

FUNC(LS_Floor_LowerByValue)
// Floor_LowerByValue (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorLowerByValue, ln, ln->args[0], SPEED(ln->args[1]), FRACUNIT*ln->args[2], 0, 0);
}

FUNC(LS_Floor_LowerToLowest)
// Floor_LowerToLowest (tag, speed)
{
	return EV_DoFloor (DFloor::floorLowerToLowest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Floor_LowerToHighest)
// Floor_LowerToHighest (tag, speed, adjust)
{
	return EV_DoFloor (DFloor::floorLowerToHighest, ln, ln->args[0], SPEED(ln->args[1]), (ln->args[2]-128)*FRACUNIT, 0, 0);
}

FUNC(LS_Floor_LowerToNearest)
// Floor_LowerToNearest (tag, speed)
{
	return EV_DoFloor (DFloor::floorLowerToNearest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Floor_RaiseByValue)
// Floor_RaiseByValue (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorRaiseByValue, ln, ln->args[0], SPEED(ln->args[1]), FRACUNIT*ln->args[2], 0, 0);
}

FUNC(LS_Floor_RaiseToHighest)
// Floor_RaiseToHighest (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseToHighest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Floor_RaiseToNearest)
// Floor_RaiseToNearest (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseToNearest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Floor_RaiseAndCrush)
// Floor_RaiseAndCrush (tag, speed, crush)
{
	return EV_DoFloor (DFloor::floorRaiseAndCrush, ln, ln->args[0], SPEED(ln->args[1]), 0, ln->args[2], 0);
}

FUNC(LS_Floor_RaiseByValueTimes8)
// FLoor_RaiseByValueTimes8 (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorRaiseByValue, ln, ln->args[0], SPEED(ln->args[1]), FRACUNIT*ln->args[2]*8, 0, 0);
}

FUNC(LS_Floor_LowerByValueTimes8)
// Floor_LowerByValueTimes8 (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorLowerByValue, ln, ln->args[0], SPEED(ln->args[1]), FRACUNIT*ln->args[2]*8, 0, 0);
}

FUNC(LS_Floor_CrushStop)
// Floor_CrushStop (tag)
{
//	return EV_FloorCrushStop (ln->args[0]);
	return 0;
}

FUNC(LS_Floor_LowerInstant)
// Floor_LowerInstant (tag, unused, height)
{
	return EV_DoFloor (DFloor::floorLowerInstant, ln, ln->args[0], 0, ln->args[2]*FRACUNIT*8, 0, 0);
}

FUNC(LS_Floor_RaiseInstant)
// Floor_RaiseInstant (tag, unused, height)
{
	return EV_DoFloor (DFloor::floorRaiseInstant, ln, ln->args[0], 0, ln->args[2]*FRACUNIT*8, 0, 0);
}

FUNC(LS_Floor_MoveToValueTimes8)
// Floor_MoveToValueTimes8 (tag, speed, height, negative)
{
	return EV_DoFloor (DFloor::floorMoveToValue, ln, ln->args[0], SPEED(ln->args[1]),
					   ln->args[2]*FRACUNIT*8*(ln->args[3]?-1:1), 0, 0);
}

FUNC(LS_Floor_RaiseToLowestCeiling)
// Floor_RaiseToLowestCeiling (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseToLowestCeiling, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Floor_RaiseByTexture)
// Floor_RaiseByTexture (tag, speed)
{
	return EV_DoFloor (DFloor::floorRaiseByTexture, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Floor_RaiseByValueTxTy)
// Floor_RaiseByValueTxTy (tag, speed, height)
{
	return EV_DoFloor (DFloor::floorRaiseAndChange, ln, ln->args[0], SPEED(ln->args[1]), ln->args[2]*FRACUNIT, 0, 0);
}

FUNC(LS_Floor_LowerToLowestTxTy)
// Floor_LowerToLowestTxTy (tag, speed)
{
	return EV_DoFloor (DFloor::floorLowerAndChange, ln, ln->args[0], SPEED(ln->args[1]), ln->args[2]*FRACUNIT, 0, 0);
}

FUNC(LS_Floor_Waggle)
// Floor_Waggle (tag, amplitude, frequency, delay, time)
{
//	return EV_StartFloorWaggle (ln->args[0], ln->args[1], ln->args[2], ln->args[3], ln->args[4]);
	return 0;
}

FUNC(LS_Floor_TransferTrigger)
// Floor_TransferTrigger (tag)
{
	return EV_DoChange (ln, trigChangeOnly, ln->args[0]);
}

FUNC(LS_Floor_TransferNumeric)
// Floor_TransferNumeric (tag)
{
	return EV_DoChange (ln, numChangeOnly, ln->args[0]);
}

FUNC(LS_Floor_Donut)
// Floor_Donut (pillartag, pillarspeed, slimespeed)
{
	return EV_DoDonut (ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[2]));
}

FUNC(LS_Generic_Floor)
// Generic_Floor (tag, speed, height, target, change/model/direct/crush)
{
	DFloor::EFloor type;

	if (ln->args[4] & 8)
	{
		switch (ln->args[3])
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
		switch (ln->args[3])
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

	return EV_DoFloor (type, ln, ln->args[0], SPEED(ln->args[1]), ln->args[2]*FRACUNIT,
					   (ln->args[4] & 16) ? 20 : -1, ln->args[4] & 7);

}

FUNC(LS_Stairs_BuildDown)
// Stair_BuildDown (tag, speed, height, delay, reset)
{
	return EV_BuildStairs (ln->args[0], DFloor::buildDown, ln,
						   ln->args[2] * FRACUNIT, SPEED(ln->args[1]), TICS(ln->args[3]), ln->args[4], 0, 1);
}

FUNC(LS_Stairs_BuildUp)
// Stairs_BuildUp (tag, speed, height, delay, reset)
{
	return EV_BuildStairs (ln->args[0], DFloor::buildUp, ln,
						   ln->args[2] * FRACUNIT, SPEED(ln->args[1]), TICS(ln->args[3]), ln->args[4], 0, 1);
}

FUNC(LS_Stairs_BuildDownSync)
// Stairs_BuildDownSync (tag, speed, height, reset)
{
	return EV_BuildStairs (ln->args[0], DFloor::buildDown, ln,
						   ln->args[2] * FRACUNIT, SPEED(ln->args[1]), 0, ln->args[3], 0, 2);
}

FUNC(LS_Stairs_BuildUpSync)
// Stairs_BuildUpSync (tag, speed, height, reset)
{
	return EV_BuildStairs (ln->args[0], DFloor::buildUp, ln,
						   ln->args[2] * FRACUNIT, SPEED(ln->args[1]), 0, ln->args[3], 0, 2);
}

FUNC(LS_Stairs_BuildUpDoom)
// Stairs_BuildUpDoom (tag, speed, height, delay, reset)
{
	return EV_BuildStairs (ln->args[0], DFloor::buildUp, ln,
						   ln->args[2] * FRACUNIT, SPEED(ln->args[1]), TICS(ln->args[3]), ln->args[4], 0, 0);
}

FUNC(LS_Generic_Stairs)
// Generic_Stairs (tag, speed, step, dir/igntxt, reset)
{
	DFloor::EStair type = (ln->args[3] & 1) ? DFloor::buildUp : DFloor::buildDown;
	BOOL res = EV_BuildStairs (ln->args[0], type, ln,
							   ln->args[2] * FRACUNIT, SPEED(ln->args[1]), 0, ln->args[4], ln->args[3] & 2, 0);

	if (res && ln && (ln->flags & ML_REPEAT_SPECIAL) && ln->special == Generic_Stairs)
		// Toggle direction of next activation of repeatable stairs
		ln->args[3] ^= 1;

	return res;
}

FUNC(LS_Pillar_Build)
// Pillar_Build (tag, speed, height)
{
	return EV_DoPillar (DPillar::pillarBuild, ln->args[0], SPEED(ln->args[1]), ln->args[2]*FRACUNIT, 0, -1);
}

FUNC(LS_Pillar_Open)
// Pillar_Open (tag, speed, f_height, c_height)
{
	return EV_DoPillar (DPillar::pillarOpen, ln->args[0], SPEED(ln->args[1]), ln->args[2]*FRACUNIT, ln->args[3]*FRACUNIT, -1);
}

FUNC(LS_Ceiling_LowerByValue)
// Ceiling_LowerByValue (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilLowerByValue, ln, ln->args[0], SPEED(ln->args[1]), 0, ln->args[2]*FRACUNIT, 0, 0, 0);
}

FUNC(LS_Ceiling_RaiseByValue)
// Ceiling_RaiseByValue (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilRaiseByValue, ln, ln->args[0], SPEED(ln->args[1]), 0, ln->args[2]*FRACUNIT, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerByValueTimes8)
// Ceiling_LowerByValueTimes8 (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilLowerByValue, ln, ln->args[0], SPEED(ln->args[1]), 0, ln->args[2]*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_RaiseByValueTimes8)
// Ceiling_RaiseByValueTimes8 (tag, speed, height)
{
	return EV_DoCeiling (DCeiling::ceilRaiseByValue, ln, ln->args[0], SPEED(ln->args[1]), 0, ln->args[2]*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_CrushAndRaise)
// Ceiling_CrushAndRaise (tag, speed, crush)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[1])/2, 0, ln->args[2], 0, 0);
}

FUNC(LS_Ceiling_LowerAndCrush)
// Ceiling_LowerAndCrush (tag, speed, crush)
{
	return EV_DoCeiling (DCeiling::ceilLowerAndCrush, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[1])/2, 0, ln->args[2], 0, 0);
}

FUNC(LS_Ceiling_CrushStop)
// Ceiling_CrushStop (tag)
{
	return EV_CeilingCrushStop (ln->args[0]);
}

FUNC(LS_Ceiling_CrushRaiseAndStay)
// Ceiling_CrushRaiseAndStay (tag, speed, crush)
{
	return EV_DoCeiling (DCeiling::ceilCrushRaiseAndStay, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[1])/2, 0, ln->args[2], 0, 0);
}

FUNC(LS_Ceiling_MoveToValueTimes8)
// Ceiling_MoveToValueTimes8 (tag, speed, height, negative)
{
	return EV_DoCeiling (DCeiling::ceilMoveToValue, ln, ln->args[0], SPEED(ln->args[1]), 0,
						 ln->args[2]*FRACUNIT*8*((ln->args[3]) ? -1 : 1), 0, 0, 0);
}

FUNC(LS_Ceiling_LowerToHighestFloor)
// Ceiling_LowerToHighestFloor (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilLowerToHighestFloor, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerInstant)
// Ceiling_LowerInstant (tag, unused, height)
{
	return EV_DoCeiling (DCeiling::ceilLowerInstant, ln, ln->args[0], 0, 0, ln->args[2]*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_RaiseInstant)
// Ceiling_RaiseInstant (tag, unused, height)
{
	return EV_DoCeiling (DCeiling::ceilRaiseInstant, ln, ln->args[0], 0, 0, ln->args[2]*FRACUNIT*8, 0, 0, 0);
}

FUNC(LS_Ceiling_CrushRaiseAndStayA)
// Ceiling_CrushRaiseAndStayA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushRaiseAndStay, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[2]), 0, ln->args[3], 0, 0);
}

FUNC(LS_Ceiling_CrushRaiseAndStaySilA)
// Ceiling_CrushRaiseAndStaySilA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushRaiseAndStay, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[2]), 0, ln->args[3], 1, 0);
}

FUNC(LS_Ceiling_CrushAndRaiseA)
// Ceiling_CrushAndRaiseA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[2]), 0, ln->args[3], 0, 0);
}

FUNC(LS_Ceiling_CrushAndRaiseSilentA)
// Ceiling_CrushAndRaiseSilentA (tag, dnspeed, upspeed, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[2]), 0, ln->args[3], 1, 0);
}

FUNC(LS_Ceiling_RaiseToNearest)
// Ceiling_RaiseToNearest (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilRaiseToNearest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerToLowest)
// Ceiling_LowerToLowest (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilLowerToLowest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0, 0, 0);
}

FUNC(LS_Ceiling_LowerToFloor)
// Ceiling_LowerToFloor (tag, speed)
{
	return EV_DoCeiling (DCeiling::ceilLowerToFloor, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0, 0, 0);
}

FUNC(LS_Generic_Ceiling)
// Generic_Ceiling (tag, speed, height, target, change/model/direct/crush)
{
	DCeiling::ECeiling type;

	if (ln->args[4] & 8) {
		switch (ln->args[3]) {
			case 1:  type = DCeiling::ceilRaiseToHighest;		break;
			case 2:  type = DCeiling::ceilRaiseToLowest;		break;
			case 3:  type = DCeiling::ceilRaiseToNearest;		break;
			case 4:  type = DCeiling::ceilRaiseToHighestFloor;	break;
			case 5:  type = DCeiling::ceilRaiseToFloor;			break;
			case 6:  type = DCeiling::ceilRaiseByTexture;		break;
			default: type = DCeiling::ceilRaiseByValue;			break;
		}
	} else {
		switch (ln->args[3]) {
			case 1:  type = DCeiling::ceilLowerToHighest;		break;
			case 2:  type = DCeiling::ceilLowerToLowest;		break;
			case 3:  type = DCeiling::ceilLowerToNearest;		break;
			case 4:  type = DCeiling::ceilLowerToHighestFloor;	break;
			case 5:  type = DCeiling::ceilLowerToFloor;			break;
			case 6:  type = DCeiling::ceilLowerByTexture;		break;
			default: type = DCeiling::ceilLowerByValue;			break;
		}
	}

	return EV_DoCeiling (type, ln, ln->args[0], SPEED(ln->args[1]), SPEED(ln->args[1]), ln->args[2]*FRACUNIT,
						 (ln->args[4] & 16) ? 20 : -1, 0, ln->args[4] & 7);
	return false;
}

FUNC(LS_Generic_Crusher)
// Generic_Crusher (tag, dnspeed, upspeed, silent, damage)
{
	return EV_DoCeiling (DCeiling::ceilCrushAndRaise, ln, ln->args[0], SPEED(ln->args[1]),
						 SPEED(ln->args[2]), 0, ln->args[4], ln->args[3] ? 2 : 0, 0);
}

FUNC(LS_Plat_PerpetualRaise)
// Plat_PerpetualRaise (tag, speed, delay)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platPerpetualRaise, 0, SPEED(ln->args[1]), TICS(ln->args[2]), 8, 0);
}

FUNC(LS_Plat_PerpetualRaiseLip)
// Plat_PerpetualRaiseLip (tag, speed, delay, lip)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platPerpetualRaise, 0, SPEED(ln->args[1]), TICS(ln->args[2]), ln->args[3], 0);
}

FUNC(LS_Plat_Stop)
// Plat_Stop (tag)
{
	EV_StopPlat (ln->args[0]);
	return true;
}

FUNC(LS_Plat_DownWaitUpStay)
// Plat_DownWaitUpStay (tag, speed, delay)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platDownWaitUpStay, 0, SPEED(ln->args[1]), TICS(ln->args[2]), 8, 0);
}

FUNC(LS_Plat_DownWaitUpStayLip)
// Plat_DownWaitUpStayLip (tag, speed, delay, lip)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platDownWaitUpStay, 0, SPEED(ln->args[1]), TICS(ln->args[2]), ln->args[3], 0);
}

FUNC(LS_Plat_DownByValue)
// Plat_DownByValue (tag, speed, delay, height)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platDownByValue, FRACUNIT*ln->args[3]*8, SPEED(ln->args[1]), TICS(ln->args[2]), 0, 0);
}

FUNC(LS_Plat_UpByValue)
// Plat_UpByValue (tag, speed, delay, height)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platUpByValue, FRACUNIT*ln->args[3]*8, SPEED(ln->args[1]), TICS(ln->args[2]), 0, 0);
}

FUNC(LS_Plat_UpWaitDownStay)
// Plat_UpWaitDownStay (tag, speed, delay)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platUpWaitDownStay, 0, SPEED(ln->args[1]), TICS(ln->args[2]), 0, 0);
}

FUNC(LS_Plat_RaiseAndStayTx0)
// Plat_RaiseAndStayTx0 (tag, speed)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platRaiseAndStay, 0, SPEED(ln->args[1]), 0, 0, 1);
}

FUNC(LS_Plat_UpByValueStayTx)
// Plat_UpByValueStayTx (tag, speed, height)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platUpByValueStay, FRACUNIT*ln->args[2]*8, SPEED(ln->args[1]), 0, 0, 2);
}

FUNC(LS_Plat_ToggleCeiling)
// Plat_ToggleCeiling (tag)
{
	return EV_DoPlat (ln->args[0], ln, DPlat::platToggle, 0, 0, 0, 0, 0);
}

FUNC(LS_Generic_Lift)
// Generic_Lift (tag, speed, delay, target, height)
{
	DPlat::EPlatType type;

	switch (ln->args[3])
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

	return EV_DoPlat (ln->args[0], ln, type, ln->args[4]*8*FRACUNIT, SPEED(ln->args[1]), OCTICS(ln->args[2]), 0, 0);
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
	level_info_t *info = FindLevelByNum (ln->args[0]);

	if (it && (info && CheckIfExitIsGood (it)))
	{
		strncpy (level.nextmap, info->mapname, 8);
		G_ExitLevel (ln->args[1], 1);
		return true;
	}

	return false;
}

FUNC(LS_Teleport)
// Teleport (tid)
{
	if(!it) return false;
	return EV_Teleport (ln->args[0], it);
}

FUNC(LS_Teleport_NoFog)
// Teleport_NoFog (tid)
{
	if(!it) return false;
	return EV_SilentTeleport (ln->args[0], ln, it);
}

FUNC(LS_Teleport_EndGame)
// Teleport_EndGame ()
{
	if (it && CheckIfExitIsGood (it))
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
	return EV_SilentLineTeleport (ln, it, ln->args[1], ln->args[2]);
}

FUNC(LS_ThrustThing)
// ThrustThing (angle, force)
{
	if(!it) return false;

	angle_t angle = BYTEANGLE(ln->args[0]) >> ANGLETOFINESHIFT;

	it->momx = ln->args[1] * finecosine[angle];
	it->momy = ln->args[1] * finesine[angle];

	return true;
}

FUNC(LS_DamageThing)
// DamageThing (damage)
{
	if(!it) return false;

	if (ln->args[0])
		P_DamageMobj (it, NULL, NULL, ln->args[0], MOD_UNKNOWN);
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
		P_GiveBody (it->player, ln->args[0]);
	}
	else
	{
		it->health += ln->args[0];
		if (mobjinfo[it->type].spawnhealth > it->health)
			it->health = mobjinfo[it->type].spawnhealth;
	}

	return true;
}
/*
FUNC(LS_Thing_Activate)
// Thing_Activate (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, ln->args[0]);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (ln->args[0]);
		P_ActivateMobj (mobj, it);
		mobj = temp;
	}

	return true;
}

FUNC(LS_Thing_Deactivate)
// Thing_Deactivate (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, ln->args[0]);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (ln->args[0]);
		P_DeactivateMobj (mobj);
		mobj = temp;
	}

	return true;
}
*/
FUNC(LS_Thing_Remove)
// Thing_Remove (tid)
{
	AActor *mobj = AActor::FindByTID (NULL, ln->args[0]);

	while (mobj)
	{
		AActor *temp = mobj->FindByTID (ln->args[0]);
		mobj->Destroy ();
		mobj = temp;
	}

	return true;
}

FUNC(LS_Thing_SetGoal)
// Thing_SetGoal (tid, goal, delay)
{
	AActor *self = AActor::FindByTID (NULL, ln->args[0]);
	AActor *goal = AActor::FindGoal (NULL, ln->args[1], MT_PATHNODE);

	while (self)
	{
		if (goal && (self->flags & MF_SHOOTABLE))
		{
			self->goal = goal->ptr();
			if (!self->target)
				self->reactiontime = ln->args[2] * TICRATE;
		}
		self = self->FindByTID (ln->args[0]);
	}

	return true;
}

FUNC(LS_FloorAndCeiling_LowerByValue)
// FloorAndCeiling_LowerByValue (tag, speed, height)
{
	return EV_DoElevator (ln, DElevator::elevateLower, SPEED(ln->args[1]), ln->args[2]*FRACUNIT, ln->args[0]);
}

FUNC(LS_FloorAndCeiling_RaiseByValue)
// FloorAndCeiling_RaiseByValue (tag, speed, height)
{
	return EV_DoElevator (ln, DElevator::elevateRaise, SPEED(ln->args[1]), ln->args[2]*FRACUNIT, ln->args[0]);
}

FUNC(LS_FloorAndCeiling_LowerRaise)
// FloorAndCeiling_LowerRaise (tag, fspeed, cspeed)
{
	return EV_DoCeiling (DCeiling::ceilRaiseToHighest, ln, ln->args[0], SPEED(ln->args[2]), 0, 0, 0, 0, 0) ||
		EV_DoFloor (DFloor::floorLowerToLowest, ln, ln->args[0], SPEED(ln->args[1]), 0, 0, 0);
}

FUNC(LS_Elevator_MoveToFloor)
// Elevator_MoveToFloor (tag, speed)
{
	return EV_DoElevator (ln, DElevator::elevateCurrent, SPEED(ln->args[1]), 0, ln->args[0]);
}

FUNC(LS_Elevator_RaiseToNearest)
// Elevator_RaiseToNearest (tag, speed)
{
	return EV_DoElevator (ln, DElevator::elevateUp, SPEED(ln->args[1]), 0, ln->args[0]);
}

FUNC(LS_Elevator_LowerToNearest)
// Elevator_LowerToNearest (tag, speed)
{
	return EV_DoElevator (ln, DElevator::elevateDown, SPEED(ln->args[1]), 0, ln->args[0]);
}

FUNC(LS_Light_ForceLightning)
// Light_ForceLightning (tag)
{
	return false;
}

FUNC(LS_Light_RaiseByValue)
// Light_RaiseByValue (tag, value)
{
	EV_LightChange (ln->args[0], ln->args[1]);
	return true;
}

FUNC(LS_Light_LowerByValue)
// Light_LowerByValue (tag, value)
{
	EV_LightChange (ln->args[0], -ln->args[1]);
	return true;
}

FUNC(LS_Light_ChangeToValue)
// Light_ChangeToValue (tag, value)
{
	EV_LightTurnOn (ln->args[0], ln->args[1]);
	return true;
}

FUNC(LS_Light_Fade)
// Light_Fade (tag, value, tics);
{
	EV_StartLightFading (ln->args[0], ln->args[1], TICS(ln->args[2]));
	return true;
}

FUNC(LS_Light_Glow)
// Light_Glow (tag, upper, lower, tics)
{
	EV_StartLightGlowing (ln->args[0], ln->args[1], ln->args[2], TICS(ln->args[3]));
	return true;
}

FUNC(LS_Light_Flicker)
// Light_Flicker (tag, upper, lower)
{
	EV_StartLightFlickering (ln->args[0], ln->args[1], ln->args[2]);
	return true;
}

FUNC(LS_Light_Strobe)
// Light_Strobe (tag, upper, lower, u-tics, l-tics)
{
	EV_StartLightStrobing (ln->args[0], ln->args[1], ln->args[2], TICS(ln->args[3]), TICS(ln->args[4]));
	return true;
}

FUNC(LS_Light_StrobeDoom)
// Light_StrobeDoom (tag, u-tics, l-tics)
{
	EV_StartLightStrobing (ln->args[0], TICS(ln->args[1]), TICS(ln->args[2]));
	return true;
}

FUNC(LS_Light_MinNeighbor)
// Light_MinNeighbor (tag)
{
	EV_TurnTagLightsOff (ln->args[0]);
	return true;
}

FUNC(LS_Light_MaxNeighbor)
// Light_MaxNeighbor (tag)
{
	EV_LightTurnOn (ln->args[0], -1);
	return true;
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
	if (ln || ln->args[3])
		return false;

	AdjustPusher (ln->args[0], ln->args[1], ln->args[2], DPusher::p_wind);
	return true;
}

FUNC(LS_Sector_SetCurrent)
// Sector_SetCurrent (tag, amount, angle)
{
	if (ln || ln->args[3])
		return false;

	AdjustPusher (ln->args[0], ln->args[1], ln->args[2], DPusher::p_current);
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
	if (ln->args[0] == 0)
		return false;

	fixed_t dx = (ln->args[1] - ln->args[2]) * (FRACUNIT/64);
	fixed_t dy = (ln->args[4] - ln->args[3]) * (FRACUNIT/64);
	int sidechoice;

	if (ln->args[0] < 0)
	{
		sidechoice = 1;
		ln->args[0] = -ln->args[0];
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

			if (wallnum >= 0 && lines[sides[wallnum].linenum].id == ln->args[0] &&
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
					lines[sides[collect.RefNum].linenum].id == ln->args[0] &&
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
		while ((linenum = P_FindLineFromID (ln->args[0], linenum)) >= 0)
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
	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0) {
		sectors[secnum].damage = ln->args[1];
		sectors[secnum].mod = ln->args[2];
	}
	return true;
}

FUNC(LS_Sector_SetGravity)
// Sector_SetGravity (tag, intpart, fracpart)
{
	int secnum = -1;
	float gravity;

	if (ln->args[2] > 99)
		ln->args[2] = 99;
	gravity = (float)ln->args[1] + (float)ln->args[2] * 0.01f;

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
		sectors[secnum].gravity = gravity;

	return true;
}

FUNC(LS_Sector_SetColor)
// Sector_SetColor (tag, r, g, b)
{
	int secnum = -1;

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
	{
		sectors[secnum].floorcolormap = GetSpecialLights (ln->args[1], ln->args[2], ln->args[3],
			RPART(sectors[secnum].floorcolormap->fade),
			GPART(sectors[secnum].floorcolormap->fade),
			BPART(sectors[secnum].floorcolormap->fade));
		sectors[secnum].ceilingcolormap = GetSpecialLights (ln->args[1], ln->args[2], ln->args[3],
			RPART(sectors[secnum].ceilingcolormap->fade),
			GPART(sectors[secnum].ceilingcolormap->fade),
			BPART(sectors[secnum].ceilingcolormap->fade));
	}

	return true;
}

FUNC(LS_Sector_SetFade)
// Sector_SetFade (tag, r, g, b)
{
	int secnum = -1;

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
	{
		sectors[secnum].floorcolormap = GetSpecialLights (
			RPART(sectors[secnum].floorcolormap->color),
			GPART(sectors[secnum].floorcolormap->color),
			BPART(sectors[secnum].floorcolormap->color),
			ln->args[1], ln->args[2], ln->args[3]);
		sectors[secnum].ceilingcolormap = GetSpecialLights (
			RPART(sectors[secnum].ceilingcolormap->color),
			GPART(sectors[secnum].ceilingcolormap->color),
			BPART(sectors[secnum].ceilingcolormap->color),
			ln->args[1], ln->args[2], ln->args[3]);
	}
	return true;
}

FUNC(LS_Sector_SetCeilingPanning)
// Sector_SetCeilingPanning (tag, x-int, x-frac, y-int, y-frac)
{
	int secnum = -1;
	fixed_t xofs = ln->args[1] * FRACUNIT + ln->args[2] * (FRACUNIT/100);
	fixed_t yofs = ln->args[3] * FRACUNIT + ln->args[4] * (FRACUNIT/100);

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
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
	fixed_t xofs = ln->args[1] * FRACUNIT + ln->args[2] * (FRACUNIT/100);
	fixed_t yofs = ln->args[3] * FRACUNIT + ln->args[4] * (FRACUNIT/100);

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
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
	fixed_t xscale = ln->args[1] * FRACUNIT + ln->args[2] * (FRACUNIT/100);
	fixed_t yscale = ln->args[3] * FRACUNIT + ln->args[4] * (FRACUNIT/100);

	if (xscale)
		xscale = FixedDiv (FRACUNIT, xscale);
	if (yscale)
		yscale = FixedDiv (FRACUNIT, yscale);

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
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
	fixed_t xscale = ln->args[1] * FRACUNIT + ln->args[2] * (FRACUNIT/100);
	fixed_t yscale = ln->args[3] * FRACUNIT + ln->args[4] * (FRACUNIT/100);

	if (xscale)
		xscale = FixedDiv (FRACUNIT, xscale);
	if (yscale)
		yscale = FixedDiv (FRACUNIT, yscale);

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
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
	angle_t ceiling = ln->args[2] * ANG(1);
	angle_t floor = ln->args[1] * ANG(1);

	while ((secnum = P_FindSectorFromTag (ln->args[0], secnum)) >= 0)
	{
		sectors[secnum].floor_angle = floor;
		sectors[secnum].ceiling_angle = ceiling;
	}
	return true;
}

FUNC(LS_Line_AlignCeiling)
// Line_AlignCeiling (lineid, side)
{
	int line = P_FindLineFromID (ln->args[0], -1);
	BOOL ret = 0;

	if (line < 0)
		I_Error ("Sector_AlignCeiling: Lineid %d is undefined", ln->args[0]);
	do
	{
		ret |= R_AlignFlat (line, !!ln->args[1], 1);
	} while ( (line = P_FindLineFromID (ln->args[0], line)) >= 0);
	return ret;
}

FUNC(LS_Line_AlignFloor)
// Line_AlignFloor (lineid, side)
{
	int line = P_FindLineFromID (ln->args[0], -1);
	BOOL ret = 0;

	if (line < 0)
		I_Error ("Sector_AlignFloor: Lineid %d is undefined", ln->args[0]);
	do
	{
		ret |= R_AlignFlat (line, !!ln->args[1], 0);
	} while ( (line = P_FindLineFromID (ln->args[0], line)) >= 0);
	return ret;
}

FUNC(LS_ChangeCamera)
// ChangeCamera (tid, who, revert?)
{
	AActor *camera = AActor::FindGoal (NULL, ln->args[0], MT_CAMERA);

	if (!it || !it->player || ln->args[1])
	{
		for (size_t i = 0; i < players.size(); i++)
		{
			if (!players[i].ingame())
				continue;

			if (camera)
			{
				players[i].camera = camera->ptr();
				if (ln->args[2])
					players[i].cheats |= CF_REVERTPLEASE;
			}
			else
			{
				players[i].camera = players[i].mo;
				players[i].cheats &= ~CF_REVERTPLEASE;
			}
		}
	}
	else
	{
		if (camera) {
			it->player->camera = camera->ptr();
			if (ln->args[2])
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

	if (!it->player && !ln->args[0])
		return false;

	switch (ln->args[2])
	{
		case PROP_FROZEN:
			mask = CF_FROZEN;
			break;
		case PROP_NOTARGET:
			mask = CF_NOTARGET;
			break;
	}

	if (ln->args[1] == 0)
	{
		if (ln->args[1])
			it->player->cheats |= mask;
		else
			it->player->cheats &= ~mask;
	}
	else
	{
		for (size_t i = 0; i < players.size(); i++)
		{
			if (!players[i].ingame())
				continue;

			if (ln->args[1])
				players[i].cheats |= mask;
			else
				players[i].cheats &= ~mask;
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
	while ((linenum = P_FindLineFromID (ln->args[0], linenum)) >= 0)
	{
		lines[linenum].lucency = ln->args[1] & 255;
	}

	return true;
}

lnSpecFunc LineSpecials[256] =
{
	LS_NOP,
	LS_NOP,		// Polyobj_StartLine,
	LS_NOP,
	LS_NOP,
	LS_NOP,
	LS_NOP,		// Polyobj_ExplicitLine
	LS_NOP,
	LS_NOP,
	LS_NOP,
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
	LS_NOP,
	LS_NOP,
	LS_NOP,
	LS_NOP,
	LS_NOP,		// 84
	LS_NOP,		// 85
	LS_NOP,		// 86
	LS_NOP,		// 87
	LS_NOP,		// 88
	LS_NOP,		// 89
	LS_NOP,
	LS_NOP,
	LS_NOP,
	LS_NOP,
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
	LS_NOP,
	LS_NOP,		// Line_SetIdentification
	LS_NOP,		// 122
	LS_NOP,		// 123
	LS_NOP,		// 124
	LS_NOP,		// 125
	LS_NOP,		// 126
	LS_NOP,		// 127
	LS_NOP,		// 128
	LS_UsePuzzleItem,
	LS_NOP,
	LS_NOP,
	LS_Thing_Remove,
	LS_NOP,
	LS_NOP,
	LS_NOP,
	LS_NOP,
	LS_NOP,
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
	LS_NOP,
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

VERSION_CONTROL (p_lnspec_cpp, "$Id$")

