// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//  [Blair] Define the Doom (Doom in Doom) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//  "Attempts" to be Boom compatible, hence the name.
//
//-----------------------------------------------------------------------------

#include "odamex.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "m_wdlstats.h"
#include "c_cvars.h"
#include "d_player.h"
#include "p_boomfspec.h"

EXTERN_CVAR(sv_allowexit)

//
// P_CrossCompatibleSpecialLine - Walkover Trigger Dispatcher
//
// Called every time a thing origin is about
//  to cross a line with a non 0 special, whether a walkover type or not.
//
// jff 02/12/98 all W1 lines were fixed to check the result from the EV_
//  function before clearing the special. This avoids losing the function
//  of the line, should the sector already be active when the line is
//  crossed. Change is qualified by demo_compatibility.
//
// CPhipps - take a line_t pointer instead of a line number, as in MBF

bool P_CrossCompatibleSpecialLine(line_t* line, int side, AActor* thing,
                                          bool bossaction)
{
	int ok;

	//  Things that should never trigger lines
	//
	// e6y: Improved support for Doom v1.2
	/*
	if (compatibility_level == doom_12_compatibility)
	{
	    if (line->special > 98 && line->special != 104)
	    {
	        return;
	    }
	}
	*/
	if (!thing->player && thing->type != MT_AVATAR && !bossaction)
	{
		// Things that should NOT trigger specials...
		switch (thing->type)
		{
		case MT_ROCKET:
		case MT_PLASMA:
		case MT_BFG:
		case MT_TROOPSHOT:
		case MT_HEADSHOT:
		case MT_BRUISERSHOT:
			return false;
			break;

		default:
			break;
		}
	}

	// jff 02/04/98 add check here for generalized lindef types
	if (!demoplayback) // generalized types not recognized if old demo
	{
		// pointer to line function is NULL by default, set non-null if
		// line special is walkover generalized linedef type
		BOOL (*linefunc)(line_t * line) = NULL;

		// check each range of generalized linedefs
		if ((unsigned)line->special >= GenEnd)
		{
			// Out of range for GenFloors
		}
		else if ((unsigned)line->special >= GenFloorBase)
		{
			if (!thing->player && thing->type != MT_AVATAR && !bossaction)
				if ((line->special & FloorChange) || !(line->special & FloorModel))
					return false; // FloorModel is "Allow Monsters" if FloorChange is 0
			linefunc = EV_DoGenFloor;
		}
		else if ((unsigned)line->special >= GenCeilingBase)
		{
			if (!thing->player && thing->type != MT_AVATAR && !bossaction)
				if ((line->special & CeilingChange) || !(line->special & CeilingModel))
					return false; // CeilingModel is "Allow Monsters" if CeilingChange is
					               // 0
			linefunc = EV_DoGenCeiling;
		}
		else if ((unsigned)line->special >= GenDoorBase)
		{
			if (!thing->player && thing->type != MT_AVATAR && !bossaction)
			{
				if (!(line->special & DoorMonster))
					return false;            // monsters disallowed from this door
				if (line->flags & ML_SECRET) // they can't open secret doors either
					return false;
			}
			linefunc = EV_DoGenDoor;
		}
		else if ((unsigned)line->special >= GenLockedBase)
		{
			if ((!thing->player && thing->type != MT_AVATAR) ||
			    bossaction)    // boss actions can't handle locked doors
				return false;  // monsters disallowed from unlocking doors
			if (((line->special & TriggerType) == WalkOnce) ||
			    ((line->special & TriggerType) == WalkMany))
			{ // jff 4/1/98 check for being a walk type before reporting door type
				if (!P_CanUnlockGenDoor(line, thing->player))
					return false;
			}
			else
				return false;
			linefunc = EV_DoGenLockedDoor;
		}
		else if ((unsigned)line->special >= GenLiftBase)
		{
			if (!thing->player && thing->type != MT_AVATAR && !bossaction)
				if (!(line->special & LiftMonster))
					return false; // monsters disallowed
			linefunc = EV_DoGenLift;
		}
		else if ((unsigned)line->special >= GenStairsBase)
		{
			if (!thing->player && thing->type != MT_AVATAR && !bossaction)
				if (!(line->special & StairMonster))
					return false; // monsters disallowed
			linefunc = EV_DoGenStairs;
		}
		else if ((unsigned)line->special >= GenCrusherBase)
		{
			// haleyjd 06/09/09: This was completely forgotten in BOOM, disabling
			// all generalized walk-over crusher types!
			if (!thing->player && thing->type != MT_AVATAR && !bossaction)
				if (!(line->special & StairMonster))
					return false; // monsters disallowed
			linefunc = EV_DoGenCrusher;
		}

		if (linefunc) // if it was a valid generalized type
			switch ((line->special & TriggerType) >> TriggerTypeShift)
			{
			case WalkOnce:
				if (linefunc(line))
				{
					return true;
				}
			case WalkMany:
				linefunc(line);
				return true;
			default: // if not a walk type, do nothing here
				return false;
			}
	}

	if ((!thing->player && thing->type != MT_AVATAR) || bossaction)
	{
		ok = 0;
		switch (line->special)
		{
		// teleporters are blocked for boss actions.
		case 39:  // teleport trigger
		case 97:  // teleport retrigger
		case 125: // teleport monsteronly trigger
		case 126: // teleport monsteronly retrigger
		          // jff 3/5/98 add ability of monsters etc. to use teleporters
		case 208: // silent thing teleporters
		case 207:
		case 243: // silent line-line teleporter
		case 244: // jff 3/6/98 make fit within DCK's 256 linedef types
		case 262: // jff 4/14/98 add monster only
		case 263: // jff 4/14/98 silent thing,line,line rev types
		case 264: // jff 4/14/98 plus player/monster silent line
		case 265: //            reversed types
		case 266:
		case 267:
		case 268:
		case 269:
			if (bossaction)
				return false;

		case 4:  // raise door
		case 10: // plat down-wait-up-stay trigger
		case 88: // plat down-wait-up-stay retrigger
			ok = 1;
			break;
		}
		if (!ok && !bossaction) // Bossactions can use any linedef except teleports.
			return false;
	}

	if (!P_CheckTag(line)) // jff 2/27/98 disallow zero tag on some types
		return false;

	// Dispatch on the line special value to the line's action routine
	// If a once only function, and successful, clear the line special

	// Do not teleport on the wrong side
	if (side)
	{
		if (P_IsTeleportLine(line->special))
		{
			return false;
		}
	}

	switch (line->special)
	{
		// Regular walk once triggers

	case 2:
		// Open Door
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_SLOW), 0, NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 3:
		// Close Door
		if (EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_SLOW), 0, NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 4:
		// Raise Door
		if (EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_SLOW),
		              TICS(VDOORWAIT), NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 5:
		// Raise Floor
		if (EV_DoFloor(DFloor::floorRaiseToLowestCeiling, line, line->id, SPEED(F_SLOW),
		               0, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 6:
		// Fast Ceiling Crush & Raise
		if (EV_DoCeiling(DCeiling::fastCrushAndRaise, line, line->id, SPEED(C_NORMAL),
		                 SPEED(C_NORMAL), 0, true, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 8:
		// Build Stairs
		if (EV_BuildStairs(line->id, DFloor::buildUp, line, 8 * FRACUNIT, SPEED(S_SLOW),
		                   TICS(0), 0, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 10:
		// PlatDownWaitUp
		if (EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_FAST),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 12:
		// Light Turn On - brightest near
		EV_LightTurnOn(line->id, -1);
		return true;
		//line->special = 0;
		break;

	case 13:
		// Light Turn On 255
		EV_LightTurnOn(line->id, 255);
		return true;
		//line->special = 0;
		break;

	case 16:
		// Close Door 30
		if (EV_DoDoor(DDoor::doorCloseWaitOpen, line, thing, line->id, SPEED(D_SLOW),
		              OCTICS(240), NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 17:
		// Start Light Strobing
		EV_StartLightStrobing(line->id, TICS(5), TICS(35));
		return true;
		//line->special = 0;
		break;

	case 19:
		// Lower Floor
		if (EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_SLOW),
		               (128 - 128) * FRACUNIT, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 22:
		// Raise floor to nearest height and change texture
		if (EV_DoPlat(line->id, line, DPlat::platRaiseAndStay, 0, SPEED(P_SLOW / 2), 0, 0,
		              1))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 25:
		// Ceiling Crush and Raise
		if (EV_DoCeiling(DCeiling::crushAndRaise, line, line->id, SPEED(C_SLOW),
		                 SPEED(C_SLOW), 0, true, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 30:
		// Raise floor to shortest texture height
		//  on either side of lines.
		if (EV_DoFloor(DFloor::floorRaiseByTexture, line, line->id, SPEED(F_SLOW), 0, 0,
		               0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 35:
		// Lights Very Dark
		EV_LightTurnOn(line->id, 35);
		return true;
		//line->special = 0;
		break;

	case 36:
		// Lower Floor (TURBO)
		if (EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_FAST),
		               (136 - 128) * FRACUNIT, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 37:
		// LowerAndChange
		if (EV_DoFloor(DFloor::floorLowerAndChange, line, line->id, SPEED(F_SLOW),
		               0 * FRACUNIT, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 38:
		// Lower Floor To Lowest
		if (EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0, 0,
		               0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 39:
		// TELEPORT! //jff 02/09/98 fix using up with wrong side crossing
		if (EV_LineTeleport(line, side, thing))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 40:
		// RaiseCeilingLowerFloor
		EV_DoCeiling(DCeiling::ceilRaiseToHighest, line, line->id, SPEED(C_SLOW), 0, 0, 0,
		             0, 0);
		EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0, 0,
		           0); // jff 02/12/98 doesn't work
		return true;
		//line->special = 0;
		break;

	case 44:
		// Ceiling Crush
		if (EV_DoCeiling(DCeiling::lowerAndCrush, line, line->id, SPEED(C_SLOW),
		                 SPEED(C_SLOW) / 2, 0, true, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 52:
		// EXIT!
		// killough 10/98: prevent zombies from exiting levels
		if (bossaction || ((!(thing->player && thing->player->health <= 0)) &&
		                   CheckIfExitIsGood(thing)))
		{
			G_ExitLevel(0, 1);
			return true;
		}
		break;

	case 53:
		// Perpetual Platform Raise
		if (EV_DoPlat(line->id, line, DPlat::platPerpetualRaise, 0, SPEED(P_SLOW),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 54:
		// Platform Stop
		EV_StopPlat(line->id);
		return true;
		//line->special = 0;
		break;

	case 56:
		// Raise Floor Crush
		if (EV_DoFloor(DFloor::floorRaiseAndCrush, line, line->id, SPEED(F_SLOW), 0, true,
		               0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 57:
		// Ceiling Crush Stop
		if (EV_CeilingCrushStop(line->id))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 58:
		// Raise Floor 24
		if (EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
		               FRACUNIT * 24, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 59:
		// Raise Floor 24 And Change
		if (EV_DoFloor(DFloor::floorRaiseAndChange, line, line->id, SPEED(F_SLOW),
		               24 * FRACUNIT, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 100:
		// Build Stairs Turbo 16
		if (EV_BuildStairs(line->id, DFloor::buildUp, line, 16 * FRACUNIT, SPEED(S_TURBO),
		                   TICS(0), 0, 0, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 104:
		// Turn lights off in sector(tag)
		EV_TurnTagLightsOff(line->id);
		return true;
		//line->special = 0;
		break;

	case 108:
		// Blazing Door Raise (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_FAST),
		              TICS(VDOORWAIT), NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 109:
		// Blazing Door Open (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), 0, NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 110:
		// Blazing Door Close (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_FAST), 0, NoKey))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 119:
		// Raise floor to nearest surr. floor
		if (EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_SLOW), 0, 0,
		               0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 121:
		// Blazing PlatDownWaitUpStay
		if (EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_TURBO),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 124:
		// Secret EXIT
		// killough 10/98: prevent zombies from exiting levels
		// CPhipps - change for lxdoom's compatibility handling
		if (bossaction || ((!(thing->player && thing->player->health <= 0)) &&
		                   CheckIfExitIsGood(thing)))
		{
			G_SecretExitLevel(0, 1);
			return true;
		}
		break;

	case 125:
		// TELEPORT MonsterONLY
		if (!thing->player && thing->type != MT_AVATAR &&
		    (EV_LineTeleport(line, side, thing)))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 130:
		// Raise Floor Turbo
		if (EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_FAST), 0, 0,
		               0))
		{
			return true;
			//line->special = 0;
		}
		break;

	case 141:
		// Silent Ceiling Crush & Raise
		if (EV_DoCeiling(DCeiling::silentCrushAndRaise, line, line->id, SPEED(C_SLOW),
		                 SPEED(C_SLOW), 0, true, 1, 0))
		{
			return true;
			//line->special = 0;
		}
		break;

		// Regular walk many retriggerable

	case 72:
		// Ceiling Crush
		EV_DoCeiling(DCeiling::lowerAndCrush, line, line->id, SPEED(C_SLOW),
		             SPEED(C_SLOW) / 2, 0, true, 0, 0);
		return true;

	case 73:
		// Ceiling Crush and Raise
		EV_DoCeiling(DCeiling::crushAndRaise, line, line->id, SPEED(C_SLOW),
		             SPEED(C_SLOW), 0, true, 0, 0);
		return true;

	case 74:
		// Ceiling Crush Stop
		EV_CeilingCrushStop(line->id);
		return true;

	case 75:
		// Close Door
		EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_SLOW), 0, NoKey);
		return true;

	case 76:
		// Close Door 30
		EV_DoDoor(DDoor::doorCloseWaitOpen, line, thing, line->id, SPEED(D_SLOW),
		          OCTICS(240), NoKey);
		return true;

	case 77:
		// Fast Ceiling Crush & Raise
		EV_DoCeiling(DCeiling::fastCrushAndRaise, line, line->id, SPEED(C_NORMAL),
		             SPEED(C_NORMAL), 0, true, 0, 0);
		return true;

	case 79:
		// Lights Very Dark
		EV_LightTurnOn(line->id, 35);
		return true;

	case 80:
		// Light Turn On - brightest near
		EV_LightTurnOn(line->id, -1);
		return true;

	case 81:
		// Light Turn On 255
		EV_LightTurnOn(line->id, 255);
		return true;

	case 82:
		// Lower Floor To Lowest
		EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0, 0, 0);
		return true;

	case 83:
		// Lower Floor
		EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_SLOW),
		           (128 - 128) * FRACUNIT, 0, 0);
		return true;

	case 84:
		// LowerAndChange
		EV_DoFloor(DFloor::floorLowerAndChange, line, line->id, SPEED(F_SLOW),
		           0 * FRACUNIT, 0, 0);
		return true;

	case 86:
		// Open Door
		EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_SLOW), 0, NoKey);
		return true;

	case 87:
		// Perpetual Platform Raise
		EV_DoPlat(line->id, line, DPlat::platPerpetualRaise, 0, SPEED(P_SLOW),
		          TICS(PLATWAIT), 0 * FRACUNIT, 0);
		return true;

	case 88:
		// PlatDownWaitUp
		EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_FAST),
		          TICS(PLATWAIT), 0 * FRACUNIT, 0);
		return true;

	case 89:
		// Platform Stop
		EV_StopPlat(line->id);
		return true;

	case 90:
		// Raise Door
		EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_SLOW), TICS(VDOORWAIT),
		          NoKey);
		return true;

	case 91:
		// Raise Floor
		EV_DoFloor(DFloor::floorRaiseToLowestCeiling, line, line->id, SPEED(F_SLOW), 0, 0,
		           0);
		return true;

	case 92:
		// Raise Floor 24
		EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
		           FRACUNIT * 24, 0, 0);
		return true;

	case 93:
		// Raise Floor 24 And Change
		EV_DoFloor(DFloor::floorRaiseAndChange, line, line->id, SPEED(F_SLOW),
		           24 * FRACUNIT, 0, 0);
		return true;

	case 94:
		// Raise Floor Crush
		EV_DoFloor(DFloor::floorRaiseAndCrush, line, line->id, SPEED(F_SLOW), 0, true, 0);
		return true;

	case 95:
		// Raise floor to nearest height
		// and change texture.
		EV_DoPlat(line->id, line, DPlat::platRaiseAndStay, 0, SPEED(P_SLOW / 2), 0, 0, 1);
		return true;

	case 96:
		// Raise floor to shortest texture height
		// on either side of lines.
		EV_DoFloor(DFloor::floorRaiseByTexture, line, line->id, SPEED(F_SLOW), 0, 0, 0);
		return true;

	case 97:
		// TELEPORT!
		EV_LineTeleport(line, side, thing);
		return true;

	case 98:
		// Lower Floor (TURBO)
		EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_FAST),
		           (136 - 128) * FRACUNIT, 0, 0);
		return true;

	case 105:
		// Blazing Door Raise (faster than TURBO!)
		EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_FAST), TICS(VDOORWAIT),
		          NoKey);
		return true;

	case 106:
		// Blazing Door Open (faster than TURBO!)
		EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), 0, NoKey);
		return true;

	case 107:
		// Blazing Door Close (faster than TURBO!)
		EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_FAST), 0, NoKey);
		return true;

	case 120:
		// Blazing PlatDownWaitUpStay.
		EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_TURBO),
		          TICS(PLATWAIT), 0 * FRACUNIT, 0);
		return true;

	case 126:
		// TELEPORT MonsterONLY.
		if (!thing->player && thing->type != MT_AVATAR)
		{
			EV_LineTeleport(line, side, thing);
			return true;
		}
		break;

	case 128:
		// Raise To Nearest Floor
		EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_SLOW), 0, 0, 0);
		return true;

	case 129:
		// Raise Floor Turbo
		EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_FAST), 0, 0, 0);
		return true;

		// Extended walk triggers

		// jff 1/29/98 added new linedef types to fill all functions out so that
		// all have varieties SR, S1, WR, W1

		// killough 1/31/98: "factor out" compatibility test, by
		// adding inner switch qualified by compatibility flag.

		// killough 2/16/98: Fix problems with W1 types being cleared too early

	default:
		switch (line->special)
		{
			// Extended walk once triggers

		case 142:
			// Raise Floor 512
			// 142 W1  EV_DoFloor(raiseFloor512)
			if (EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
			               FRACUNIT * 64 * 8, 0, 0))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 143:
			// Raise Floor 24 and change
			// 143 W1  EV_DoPlat(raiseAndChange,24)
			if (EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 3 * 8,
			              SPEED(P_SLOW / 2), 0, 0, 2))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 144:
			// Raise Floor 32 and change
			// 144 W1  EV_DoPlat(raiseAndChange,32)
			if (EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 4 * 8,
			              SPEED(P_SLOW / 2), 0, 0, 2))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 145:
			// Lower Ceiling to Floor
			// 145 W1  EV_DoCeiling(lowerToFloor)
			if (EV_DoCeiling(DCeiling::ceilLowerToFloor, line, line->id, SPEED(C_SLOW), 0,
			                 0, 0, 0, 0))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 146:
			// Lower Pillar, Raise Donut
			// 146 W1  EV_DoDonut()
			if (EV_DoDonut(line))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 199:
			// Lower ceiling to lowest surrounding ceiling
			// 199 W1 EV_DoCeiling(lowerToLowest)
			if (EV_DoCeiling(DCeiling::ceilLowerToLowest, line, line->id, SPEED(C_SLOW),
			                 0, 0, 0, 0, 0))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 200:
			// Lower ceiling to highest surrounding floor
			// 200 W1 EV_DoCeiling(lowerToMaxFloor)
			if (EV_DoCeiling(DCeiling::ceilLowerToHighestFloor, line, line->id,
			                 SPEED(C_SLOW), 0, 0, 0, 0, 0))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 207:
			// killough 2/16/98: W1 silent teleporter (normal kind)
			if (EV_SilentTeleport(line->args[0], 0, line->args[2], 0, line, side, thing))
			{
				return true;
				//line->special = 0;
			}
			break;

			// jff 3/16/98 renumber 215->153
		case 153: // jff 3/15/98 create texture change no motion type
			// Texture/Type Change Only (Trig)
			// 153 W1 Change Texture/Type Only
			if (EV_DoChange(line, trigChangeOnly, line->id))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 239: // jff 3/15/98 create texture change no motion type
			// Texture/Type Change Only (Numeric)
			// 239 W1 Change Texture/Type Only
			if (EV_DoChange(line, numChangeOnly, line->id))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 219:
			// Lower floor to next lower neighbor
			// 219 W1 Lower Floor Next Lower Neighbor
			if (EV_DoFloor(DFloor::floorLowerToNearest, line, line->id, SPEED(F_SLOW), 0,
			               0, 0))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 227:
			// Raise elevator next floor
			// 227 W1 Raise Elevator next floor
			if (EV_DoElevator(line, DElevator::elevateUp, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 231:
			// Lower elevator next floor
			// 231 W1 Lower Elevator next floor
			if (EV_DoElevator(line, DElevator::elevateDown, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 235:
			// Elevator to current floor
			// 235 W1 Elevator to current floor
			if (EV_DoElevator(line, DElevator::elevateCurrent, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 243: // jff 3/6/98 make fit within DCK's 256 linedef types
			// killough 2/16/98: W1 silent teleporter (linedef-linedef kind)
			if (thing && EV_SilentLineTeleport(line, side, thing, line->id, false))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 262: // jff 4/14/98 add silent line-line reversed
			if (thing && EV_SilentLineTeleport(line, side, thing, line->id, true))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 264: // jff 4/14/98 add monster-only silent line-line reversed
			if (!thing->player && thing->type != MT_AVATAR &&
			    EV_SilentLineTeleport(line, side, thing, line->id, true))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 266: // jff 4/14/98 add monster-only silent line-line
			if (!thing->player && thing->type != MT_AVATAR &&
			    EV_SilentLineTeleport(line, side, thing, line->id, false))
			{
				return true;
				//line->special = 0;
			}
			break;

		case 268: // jff 4/14/98 add monster-only silent
			if (!thing->player && thing->type != MT_AVATAR &&
			    EV_SilentTeleport(line->args[0], 0, line->args[2], 0, line, side, thing))
			{
				return true;
				//line->special = 0;
			}
			break;

			// jff 1/29/98 end of added W1 linedef types

			// Extended walk many retriggerable

			// jff 1/29/98 added new linedef types to fill all functions
			// out so that all have varieties SR, S1, WR, W1

		case 147:
			// Raise Floor 512
			// 147 WR  EV_DoFloor(raiseFloor512)
			EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
			           FRACUNIT * 64 * 8, 0, 0);
			return true;

		case 148:
			// Raise Floor 24 and Change
			// 148 WR  EV_DoPlat(raiseAndChange,24)
			EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 3 * 8,
			          SPEED(P_SLOW / 2), 0, 0, 2);
			return true;

		case 149:
			// Raise Floor 32 and Change
			// 149 WR  EV_DoPlat(raiseAndChange,32)
			EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 4 * 8,
			          SPEED(P_SLOW / 2), 0, 0, 2);
			return true;

		case 150:
			// Start slow silent crusher
			// 150 WR  EV_DoCeiling(silentCrushAndRaise)
			EV_DoCeiling(DCeiling::silentCrushAndRaise, line, line->id, SPEED(C_SLOW),
			             SPEED(C_SLOW), 0, true, 1, 0);
			return true;

		case 151:
			// RaiseCeilingLowerFloor
			// 151 WR  EV_DoCeiling(raiseToHighest),
			//         EV_DoFloor(lowerFloortoLowest)
			EV_DoCeiling(DCeiling::ceilRaiseToHighest, line, line->id, SPEED(C_SLOW), 0,
			             0, 0, 0, 0);
			EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0, 0,
			           0);
			return true;

		case 152:
			// Lower Ceiling to Floor
			// 152 WR  EV_DoCeiling(lowerToFloor)
			EV_DoCeiling(DCeiling::ceilLowerToFloor, line, line->id, SPEED(C_SLOW), 0, 0,
			             0, 0, 0);
			return true;

			// jff 3/16/98 renumber 153->256
		case 256:
			// Build stairs, step 8
			// 256 WR EV_BuildStairs(build8)
			EV_BuildStairs(line->id, DFloor::buildUp, line, 8 * FRACUNIT, SPEED(S_SLOW),
			               0, 0, 0, 0);
			return true;

			// jff 3/16/98 renumber 154->257
		case 257:
			// Build stairs, step 16
			// 257 WR EV_BuildStairs(turbo16)
			EV_BuildStairs(line->id, DFloor::buildUp, line, 16 * FRACUNIT, SPEED(S_TURBO),
			               0, 0, 0, 0);
			return true;

		case 155:
			// Lower Pillar, Raise Donut
			// 155 WR  EV_DoDonut()
			EV_DoDonut(line);
			return true;

		case 156:
			// Start lights strobing
			// 156 WR Lights EV_StartLightStrobing()
			EV_StartLightStrobing(line->id, TICS(5), TICS(35));
			return true;

		case 157:
			// Lights to dimmest near
			// 157 WR Lights EV_TurnTagLightsOff()
			EV_TurnTagLightsOff(line->id);
			return true;

		case 201:
			// Lower ceiling to lowest surrounding ceiling
			// 201 WR EV_DoCeiling(lowerToLowest)
			EV_DoCeiling(DCeiling::ceilLowerToLowest, line, line->id, SPEED(C_SLOW), 0, 0,
			             0, 0, 0);
			return true;

		case 202:
			// Lower ceiling to highest surrounding floor
			// 202 WR EV_DoCeiling(lowerToMaxFloor)
			EV_DoCeiling(DCeiling::ceilLowerToHighestFloor, line, line->id, SPEED(C_SLOW),
			             0, 0, 0, 0, 0);
			return true;

		case 208:
			// killough 2/16/98: WR silent teleporter (normal kind)
			EV_SilentTeleport(line->args[0], 0, line->args[2], 0, line, side, thing);
			return true;

		case 212: // jff 3/14/98 create instant toggle floor type
			// Toggle floor between C and F instantly
			// 212 WR Instant Toggle Floor
			EV_DoPlat(line->id, line, DPlat::platToggle, 0, 0, 0, 0, 0);
			return true;

		// jff 3/16/98 renumber 216->154
		case 154: // jff 3/15/98 create texture change no motion type
			// Texture/Type Change Only (Trigger)
			// 154 WR Change Texture/Type Only
			EV_DoChange(line, trigChangeOnly, line->id);
			return true;

		case 240: // jff 3/15/98 create texture change no motion type
			// Texture/Type Change Only (Numeric)
			// 240 WR Change Texture/Type Only
			EV_DoChange(line, numChangeOnly, line->id);
			return true;

		case 220:
			// Lower floor to next lower neighbor
			// 220 WR Lower Floor Next Lower Neighbor
			EV_DoFloor(DFloor::floorLowerToNearest, line, line->id, SPEED(F_SLOW), 0, 0,
			           0);
			return true;

		case 228:
			// Raise elevator next floor
			// 228 WR Raise Elevator next floor
			EV_DoElevator(line, DElevator::elevateUp, SPEED(ELEVATORSPEED), 0, line->id);
			return true;

		case 232:
			// Lower elevator next floor
			// 232 WR Lower Elevator next floor
			EV_DoElevator(line, DElevator::elevateDown, SPEED(ELEVATORSPEED), 0,
			              line->id);
			return true;

		case 236:
			// Elevator to current floor
			// 236 WR Elevator to current floor
			EV_DoElevator(line, DElevator::elevateCurrent, SPEED(ELEVATORSPEED), 0,
			              line->id);
			return true;

		case 244: // jff 3/6/98 make fit within DCK's 256 linedef types
			// killough 2/16/98: WR silent teleporter (linedef-linedef kind)
			if (thing)
			{
				EV_SilentLineTeleport(line, side, thing, line->id, false);
				return true;
			}
			break;

		case 263: // jff 4/14/98 add silent line-line reversed
			if (thing)
			{
				EV_SilentLineTeleport(line, side, thing, line->id, true);
				return true;
			}
			break;

		case 265: // jff 4/14/98 add monster-only silent line-line reversed
			if (!thing->player && thing->type != MT_AVATAR)
			{
				EV_SilentLineTeleport(line, side, thing, line->id, true);
				return true;
			}
			break;

		case 267: // jff 4/14/98 add monster-only silent line-line
			if (!thing->player && thing->type != MT_AVATAR)
			{
				EV_SilentLineTeleport(line, side, thing, line->id, false);
				return true;
			}
			break;

		case 269: // jff 4/14/98 add monster-only silent
			if (!thing->player)
			{
				EV_SilentTeleport(line->args[0], 0, line->args[2], 0, line, side, thing);
				return true;
			}
			break;

			// jff 1/29/98 end of added WR linedef types
		}
		break;
	}
	return false;
}

void P_ApplyGeneralizedSectorDamage(player_t* player, int bits)
{
	switch (bits & 3)
	{
	case 0:
		break;
	case 1:
		P_ApplySectorDamage(player, 5, 0);
		break;
	case 2:
		P_ApplySectorDamage(player, 10, 0);
		break;
	case 3:
		P_ApplySectorDamage(player, 20, 5);
		break;
	}
}

void P_CollectSecretBoom(sector_t* sector, player_t* player)
{
	sector->special &= ~SECRET_MASK;

	if (sector->special < 32) // if all extended bits clear,
		sector->special = 0;  // sector is not special anymore

	P_CollectSecretCommon(sector, player);
}

void P_PlayerInCompatibleSector(player_t* player)
{
	// Spectators should not be affected by special sectors
	if (player->spectator)
		return;

	// Falling, not all the way down yet?
	if (player->mo->z != P_FloorHeight(player->mo) && !player->mo->waterlevel)
		return;

	sector_t* sector = player->mo->subsector->sector;
	if (sector->special == 0 && sector->damageamount > 0) // Odamex Static Init Damage
	{
		if (sector->damageamount < 20)
		{
			P_ApplySectorDamageNoRandom(player, sector->damageamount, MOD_UNKNOWN);
		}
		else if (sector->damageamount < 50)
		{
			P_ApplySectorDamage(player, sector->damageamount, 5, MOD_UNKNOWN);
		}
		else
		{
			P_ApplySectorDamageNoWait(player, sector->damageamount, MOD_UNKNOWN);
		}
	}
	// jff add if to handle old vs generalized types
	else if (sector->special < 32) // regular sector specials
	{
		switch (sector->special)
		{
		case 5:
			P_ApplySectorDamage(player, 10, 0, MOD_SLIME);
			break;
		case 7:
			P_ApplySectorDamage(player, 5, 0, MOD_SLIME);
			break;
		case 16:
		case 4:
			P_ApplySectorDamage(player, 20, 5, MOD_SLIME);
			break;
		case 9:
			P_CollectSecretVanilla(sector, player);
			break;
		case 11:
			P_ApplySectorDamageEndLevel(player);
			break;
		default:
			break;
		}
	}
	else // jff 3/14/98 handle extended sector damage
	{
		if (sector->special & DEATH_MASK)
		{
			switch ((sector->special & DAMAGE_MASK) >> DAMAGE_SHIFT)
			{
			case 0: // Kill player unless invuln or rad suit or IDDQD
				if (!player->powers[pw_invulnerability] && !player->powers[pw_ironfeet] && !(player->cheats & CF_GODMODE))
				{
					P_DamageMobj(player->mo, NULL, NULL, 999, MOD_UNKNOWN); // 999 so BUDDHA can survive
				}
				break;
			case 1: // Kill player with no scruples unless IDDQD
				if(!(player->cheats & CF_GODMODE))
				{
					P_DamageMobj(player->mo, NULL, NULL, 10000, MOD_UNKNOWN);
				}
				break;
			case 2: // Kill all players and exit. There's no delay here so it may confuse
			        // some players. Do NOT kill players with IDDQD.
				if (serverside)
				{
					if (sv_allowexit)
					{
						for (Players::iterator it = ::players.begin();
						     it != ::players.end(); ++it)
						{
							if (player->ingame() && player->health > 0 && !(player->cheats & CF_GODMODE))
							{
								P_DamageMobj((*it).mo, NULL, NULL, 10000, MOD_EXIT);
							}
						}
						G_ExitLevel(0, 1);
					}
					else if (!(player->cheats & CF_GODMODE)) // Do NOT kill players with IDDQD.
					{
						P_DamageMobj(
						    player->mo, NULL, NULL, 10000,
						    MOD_EXIT); // Exiting not allowed, kill only activator here
						               // even if fragexitswitch = 0
					}
				}
				break;
			case 3: // Kill all players and secret exit. There's no delay here so it may
			        // confuse some players. Do NOT kill players with IDDQD.
				if (serverside)
				{
					if (sv_allowexit)
					{
						for (Players::iterator it = ::players.begin();
						     it != ::players.end(); ++it)
						{
							if (player->ingame() && player->health > 0 && !(player->cheats & CF_GODMODE))
							{
								P_DamageMobj((*it).mo, NULL, NULL, 10000, MOD_EXIT);
							}
						}
						G_SecretExitLevel(0, 1);
					}
					else if (!(player->cheats & CF_GODMODE)) // Do NOT kill players with IDDQD.
					{
						P_DamageMobj(
						    player->mo, NULL, NULL, 10000,
						    MOD_EXIT); // Exiting not allowed, kill only activator here
						               // even if fragexitswitch = 0
					}
				}
				break;
			}
		}
		else if (!(player->cheats & CF_GODMODE)) // Do NOT damage players with IDDQD.
		{
			P_ApplyGeneralizedSectorDamage(player, (sector->special & DAMAGE_MASK) >>
			                                           DAMAGE_SHIFT);
		}
	}

	if (sector->flags & SECF_SECRET)
	{
		P_CollectSecretBoom(sector, player);
	}
}

//
// P_ActorInCompatibleSector
// Really only used for one function -- kill land monsters sector action.
//
bool P_ActorInCompatibleSector(AActor* actor)
{
	if (!actor)
		return false;

	sector_t* sector = actor->subsector->sector;

	if (sector && sector->special & KILL_MONSTERS_MASK && actor->z == actor->floorz &&
	    !actor->player && actor->flags & MF_SHOOTABLE && !(actor->flags & MF_FLOAT))
	{
		P_DamageMobj(actor, NULL, NULL, 10000);

		// must have been killed
		if (actor->health <= 0)
			return true;
	}

	return false;
}

void P_PostProcessCompatibleSidedefSpecial(side_t* sd, mapsidedef_t* msd,
                                           sector_t* sec, int i)
{
	switch (sd->special)
	{
	case 242: // variable colormap via 242 linedef
	                       // [RH] The colormap num we get here isn't really a colormap,
	                       //	  but a packed ARGB word for blending, so we also allow
	                       //	  the blend to be specified directly by the texture names
	                       //	  instead of figuring something out from the colormap.
		P_SetTransferHeightBlends(sd, msd);
		break;

	case OdamexStaticInits + 1: // init_color
		// [RH] Set sector color and fog
		// upper "texture" is light color
		// lower "texture" is fog color
		{
			unsigned int color = 0xffffff, fog = 0x000000;

			SetTextureNoErr(&sd->bottomtexture, &fog, msd->bottomtexture);
			SetTextureNoErr(&sd->toptexture, &color, msd->toptexture);
			sd->midtexture = R_TextureNumForName(msd->midtexture);

			if (fog != 0x000000 || color != 0xffffff)
			{
				dyncolormap_t* colormap =
				    GetSpecialLights(((argb_t)color).getr(), ((argb_t)color).getg(),
				                     ((argb_t)color).getb(), ((argb_t)fog).getr(),
				                     ((argb_t)fog).getg(), ((argb_t)fog).getb());

				for (int s = 0; s < numsectors; s++)
				{
					if (sectors[s].tag == sd->tag)
						sectors[s].colormap = colormap;
				}
			}
		}
		break;

		/*
		case 260:	// killough 4/11/98: apply translucency to 2s
		   normal texture sd->midtexture = strncasecmp("TRANMAP", msd->midtexture, 8) ?
		                (sd->special = W_CheckNumForName(msd->midtexture)) < 0 ||
		                W_LumpLength(sd->special) != 65536 ?
		                sd->special=0, R_TextureNumForName(msd->midtexture) :
		                    (sd->special++, 0) : (sd->special=0);
		            sd->toptexture = R_TextureNumForName(msd->toptexture);
		            sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		            break;
		*/
	default: // normal cases
		sd->midtexture = R_TextureNumForName(msd->midtexture);
		sd->toptexture = R_TextureNumForName(msd->toptexture);
		sd->bottomtexture = R_TextureNumForName(msd->bottomtexture);
		break;
	}
}

void P_SpawnCompatibleExtra(int i)
{
	int s;
	sector_t* sec;
	float grav;
	int damage;

	switch (lines[i].special)
	{
	// killough 3/7/98:
	// support for drawn heights coming from different sector
	case 242:
		sec = sides[*lines[i].sidenum].sector;
		for (s = -1; (s = P_FindSectorFromTag(lines[i].id, s)) >= 0;)
			sectors[s].heightsec = sec;
		break;

	// killough 3/16/98: Add support for setting
	// floor lighting independently (e.g. lava)
	case 213:
		sec = sides[*lines[i].sidenum].sector;
		for (s = -1; (s = P_FindSectorFromTag(lines[i].id, s)) >= 0;)
			sectors[s].floorlightsec = sec;
		break;

	// killough 4/11/98: Add support for setting
	// ceiling lighting independently
	case 261:
		sec = sides[*lines[i].sidenum].sector;
		for (s = -1; (s = P_FindSectorFromTag(lines[i].id, s)) >= 0;)
			sectors[s].ceilinglightsec = sec;
		break;

		// killough 10/98:
		//
		// Support for sky textures being transferred from sidedefs.
		// Allows scrolling and other effects (but if scrolling is
		// used, then the same sector tag needs to be used for the
		// sky sector, the sky-transfer linedef, and the scroll-effect
		// linedef). Still requires user to use F_SKY1 for the floor
		// or ceiling texture, to distinguish floor and ceiling sky.

	case 271: // Regular sky
	case 272: // Same, only flipped
		for (s = -1; (s = P_FindSectorFromTag(lines[i].id, s)) >= 0;)
			sectors[s].sky = (i + 1) | PL_SKYFLAT;
		break;

	case OdamexStaticInits: // Gravity
		grav =
		    ((float)P_AproxDistance(lines[i].dx, lines[i].dy)) / (FRACUNIT * 100.0f);
		for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
			sectors[s].gravity = grav;
		break;

	case OdamexStaticInits + 2: // Damage
		damage = P_AproxDistance(lines[i].dx, lines[i].dy) >> FRACBITS;
		for (s = -1; (s = P_FindSectorFromTag(lines[i].args[0], s)) >= 0;)
		{
			sectors[s].damageamount = damage;
			sectors[s].damageinterval = 32;
			sectors[s].leakrate = 0;
			sectors[s].mod = MOD_UNKNOWN;
		}
		break;
	}
}

//
// P_SpawnCompatibleSectorSpecial
// After the map has been loaded, scan for specials
// that spawn thinkers.
// Only runs for maps in Doom format.
//

void P_SpawnCompatibleSectorSpecial(sector_t* sector)
{
	if (sector->special & SECRET_MASK) // jff 3/15/98 count extended
		P_AddSectorSecret(sector);

	if (sector->special & FRICTION_MASK)
		sector->flags |= SECF_FRICTION;

	if (sector->special & PUSH_MASK)
		sector->flags |= SECF_PUSH;

	switch (sector->special & 31)
	{
	case 1:
		if (IgnoreSpecial)
			break;
		// random off
		P_SpawnLightFlash(sector);
		break;

	case 2:
		if (IgnoreSpecial)
			break;
		// strobe fast
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, false);
		break;

	case 3:
		if (IgnoreSpecial)
			break;
		// strobe slow
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, SLOWDARK, false);
		break;

	case 4:
		if (IgnoreSpecial)
			break;
		// strobe fast/death slime
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, false);
		sector->special |= 3 << DAMAGE_SHIFT; // jff 3/14/98 put damage bits in
		break;

	case 8:
		if (IgnoreSpecial)
			break;
		// glowing light
		P_SpawnGlowingLight(sector);
		break;
	case 9:
		// secret sector
		if (sector->special < 32) // jff 3/14/98 bits don't count unless not
			P_AddSectorSecret(sector);
		break;

	case 10:
		if (!serverside)
			break;
		// door close in 30 seconds
		P_SpawnDoorCloseIn30(sector);
		break;

	case 12:
		if (IgnoreSpecial)
			break;
		// sync strobe slow
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, SLOWDARK, true);
		break;

	case 13:
		if (IgnoreSpecial)
			break;
		// sync strobe fast
		P_SpawnStrobeFlash(sector, STROBEBRIGHT, FASTDARK, true);
		break;

	case 14:
		if (!serverside)
			break;
		// door raise in 5 minutes
		P_SpawnDoorRaiseIn5Mins(sector);
		break;

	case 17:
		if (IgnoreSpecial)
			break;
		// fire flickering
		P_SpawnFireFlicker(sector);
		break;
	}
}

void P_SpawnCompatibleScroller(line_t* l, int i)
{
	// [Blair] don't run scrolling on clients to prevent desyncs
	if (IgnoreSpecial)
		return;

	fixed_t dx = l->dx >> SCROLL_SHIFT; // direction and speed of scrolling
	fixed_t dy = l->dy >> SCROLL_SHIFT;
	int control = -1, accel = 0; // no control sector or acceleration
	int special = l->special;

	if (demoplayback && special != 48) // demo compat
		return;                        // e6y

	// killough 3/7/98: Types 245-249 are same as 250-254 except that the
	// first side's sector's heights cause scrolling when they change, and
	// this linedef controls the direction and speed of the scrolling. The
	// most complicated linedef since donuts, but powerful :)
	//
	// killough 3/15/98: Add acceleration. Types 214-218 are the same but
	// are accelerative.

	if (special >= 245 && special <= 249) // displacement scrollers
	{
		special += 250 - 245;
		control = sides[*l->sidenum].sector - sectors;
	}
	else if (special >= 214 && special <= 218) // accelerative scrollers
	{
		accel = 1;
		special += 250 - 214;
		control = sides[*l->sidenum].sector - sectors;
	}

	switch (special)
	{
		int s;

	case 250: // scroll effect ceiling
		for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
			new DScroller(DScroller::sc_ceiling, -dx, dy, control, s, accel);
		break;

	case 251: // scroll effect floor
	case 253: // scroll and carry objects on floor
		for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
			new DScroller(DScroller::sc_floor, -dx, dy, control, s, accel);
		if (special != 253)
			break;
		// fallthrough

	case 252: // carry objects on floor
		dx = FixedMul(dx, CARRYFACTOR);
		dy = FixedMul(dy, CARRYFACTOR);
		for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
			new DScroller(DScroller::sc_carry, dx, dy, control, s, accel);
		break;

		// killough 3/1/98: scroll wall according to linedef
		// (same direction and speed as scrolling floors)
	case 254:
		if (l->id == 0)
		{
			new DScroller(dx, dy, l, control, accel);
		}
		else
		{
			for (s = -1; (s = P_FindLineFromLineTag(l, s)) >= 0;)
				if (s != i)
					new DScroller(dx, dy, lines + s, control, accel);
		}
		break;

	case 255: // killough 3/2/98: scroll according to sidedef offsets
		s = lines[i].sidenum[0];
		new DScroller(DScroller::sc_side, -sides[s].textureoffset, sides[s].rowoffset, -1,
		              s, accel);
		break;

	case 1024: // special 255 with tag control
	case 1025:
	case 1026:
		if (l->id == 0)
			Printf(PRINT_HIGH, "Line %d is missing a tag!", i);

		if (special > 1024)
			control = sides[*l->sidenum].sector - sectors;

		if (special == 1026)
			accel = 1;

		s = lines[i].sidenum[0];
		dx = -sides[s].textureoffset / 8;
		dy = sides[s].rowoffset / 8;
		for (s = -1; (s = P_FindLineFromLineTag(l, s)) >= 0;)
			if (s != i)
				new DScroller(DScroller::sc_side, dx, dy, control, lines[s].sidenum[0],
				              accel);
		break;

	case 48: // scroll first side
		new DScroller(DScroller::sc_side, FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
		break;

	case 85: // jff 1/30/98 2-way scroll
		new DScroller(DScroller::sc_side, -FRACUNIT, 0, -1, lines[i].sidenum[0], accel);
		break;
	}
}

// killough 3/7/98 -- end generalized scroll effects

void P_SpawnCompatibleFriction(line_t* l)
{
	if (l->special == 223)
	{
		int value;
		bool use_thinker;

		value = P_AproxDistance(l->dx, l->dy) >> FRACBITS;
		use_thinker = !demoplayback;

		P_ApplySectorFriction(l->id, value, use_thinker);
	}
}

void P_SpawnCompatiblePusher(line_t* l)
{
	int s;
	AActor* thing;

	switch (l->special)
	{
	case 224: // wind
		for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
			new DPusher(DPusher::p_wind, l, 0, 0, NULL, s);
		break;
	case 225: // current
		for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
			new DPusher(DPusher::p_current, l, 0, 0, NULL, s);
		break;
	case 226: // push/pull
		for (s = -1; (s = P_FindSectorFromLineTag(l, s)) >= 0;)
		{
			thing = P_GetPushThing(s);
			if (thing) // No MT_P* means no effect
				new DPusher(DPusher::p_push, l, 0, 0, thing, s);
		}
		break;
	}
}

//
// P_UseCompatibleSpecialLine
//
//
// Called when a thing uses (pushes) a special line.
// Only the front sides of lines are usable.
// Dispatches to the appropriate linedef function handler.
//
// Passed the thing using the line, the line being used, and the side used
// Returns true if a thinker was created
//
bool P_UseCompatibleSpecialLine(AActor* thing, line_t* line, int side,
                                        bool bossaction)
{
	bool reuse = false;
	bool trigger = false; // used for bossactions
	// e6y
	// b.m. side test was broken in boom201
	if (side) // jff 6/1/98 fix inadvertent deletion of side test
		return false;

	// jff 02/04/98 add check here for generalized floor/ceil mover

	// pointer to line function is NULL by default, set non-null if
	// line special is push or switch generalized linedef type
	int (*linefunc)(line_t * line) = NULL;

	// check each range of generalized linedefs
	if ((unsigned)line->special >= GenEnd)
	{
		// Out of range for GenFloors
	}
	else if ((unsigned)line->special >= GenFloorBase)
	{
		if (!thing->player && thing->type != MT_AVATAR && !bossaction)
			if ((line->special & FloorChange) || !(line->special & FloorModel))
				return false; // FloorModel is "Allow Monsters" if FloorChange is 0
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 2/27/98 all non-manual
			return false;                            // generalized types require tag
		linefunc = EV_DoGenFloor;
	}
	else if ((unsigned)line->special >= GenCeilingBase)
	{
		if (!thing->player && thing->type != MT_AVATAR && !bossaction)
			if ((line->special & CeilingChange) || !(line->special & CeilingModel))
				return false; // CeilingModel is "Allow Monsters" if CeilingChange is
					            // 0
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 2/27/98 all non-manual
			return false;                            // generalized types require tag
		linefunc = EV_DoGenCeiling;
	}
	else if ((unsigned)line->special >= GenDoorBase)
	{
		if (!thing->player && thing->type != MT_AVATAR && !bossaction)
		{
			if (!(line->special & DoorMonster))
				return false;            // monsters disallowed from this door
			if (line->flags & ML_SECRET) // they can't open secret doors either
				return false;
		}
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 3/2/98 all non-manual
			return false;                            // generalized types require tag
		linefunc = EV_DoGenDoor;
	}
	else if ((unsigned)line->special >= GenLockedBase)
	{
		if ((!thing->player && thing->type != MT_AVATAR) || bossaction)
			return false; // monsters disallowed from unlocking doors
		if (!P_CanUnlockGenDoor(line, thing->player))
			return false;
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 2/27/98 all non-manual
			return false;                            // generalized types require tag

		linefunc = EV_DoGenLockedDoor;
	}
	else if ((unsigned)line->special >= GenLiftBase)
	{
		if (!thing->player && thing->type != MT_AVATAR && !bossaction)
			if (!(line->special & LiftMonster))
				return false;                        // monsters disallowed
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 2/27/98 all non-manual
			return false;                            // generalized types require tag
		linefunc = EV_DoGenLift;
	}
	else if ((unsigned)line->special >= GenStairsBase)
	{
		if (!thing->player && thing->type != MT_AVATAR && !bossaction)
			if (!(line->special & StairMonster))
				return false;                        // monsters disallowed
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 2/27/98 all non-manual
			return false;                            // generalized types require tag
		linefunc = EV_DoGenStairs;
	}
	else if ((unsigned)line->special >= GenCrusherBase)
	{
		if (!thing->player && thing->type != MT_AVATAR && !bossaction)
			if (!(line->special & CrusherMonster))
				return false;                        // monsters disallowed
		if (!line->id &&
			((line->special & 6) != 6)) // e6y //jff 2/27/98 all non-manual
			return false;                            // generalized types require tag
		linefunc = EV_DoGenCrusher;
	}

	if (linefunc)
		switch ((line->special & TriggerType) >> TriggerTypeShift)
		{
		case PushOnce:
			if (!side)
			{
				if (linefunc(line))
				{
					reuse = false;
					trigger = true;
				}
			}
			break;
		case PushMany:
			if (!side)
			{
				linefunc(line);
				reuse = true;
				trigger = true;
			}
			break;
		case SwitchOnce:
			if (linefunc(line))
			{
				reuse = false;
				trigger = true;
			}
			break;
		case SwitchMany:
			if (linefunc(line))
			{
				reuse = true;
				trigger = true;
			}
			break;
		default: // if not a switch/push type, do nothing here
			return false;
		}

	// Switches that other things can activate.
	if (thing && !thing->player && thing->type != MT_AVATAR && !bossaction)
	{
		// never open secret doors
		if (line->flags & ML_SECRET)
			return false;

		switch (line->special)
		{
		case 1:  // MANUAL DOOR RAISE
		case 32: // MANUAL BLUE
		case 33: // MANUAL RED
		case 34: // MANUAL YELLOW
		// jff 3/5/98 add ability to use teleporters for monsters
		case 195: // switch teleporters
		case 174:
		case 210: // silent switch teleporters
		case 209:
			break;

		default:
			return false;
			break;
		}
	}

	if (bossaction)
	{
		switch (line->special)
		{
			// 0-tag specials, locked switches and teleporters need to be blocked for boss
			// actions.
		case 1:   // MANUAL DOOR RAISE
		case 32:  // MANUAL BLUE
		case 33:  // MANUAL RED
		case 34:  // MANUAL YELLOW
		case 117: // Blazing door raise
		case 118: // Blazing door open
		case 133: // BlzOpenDoor BLUE
		case 135: // BlzOpenDoor RED
		case 137: // BlzOpenDoor YEL

		case 99:  // BlzOpenDoor BLUE
		case 134: // BlzOpenDoor RED
		case 136: // BlzOpenDoor YELLOW

			// jff 3/5/98 add ability to use teleporters for monsters
		case 195: // switch teleporters
		case 174:
		case 210: // silent switch teleporters
		case 209:
			return false;
			break;
		}
	}

	if (!P_CheckTag(line)) // jff 2/27/98 disallow zero tag on some types
		return false;

	// Dispatch to handler according to linedef type
	switch (line->special)
	{
	// Manual doors, push type with no tag
	case 1:  // Vertical Door
		if (EV_DoDoor(DDoor::doorRaise, line, thing, 0, SPEED(D_SLOW),
		                                TICS(VDOORWAIT), NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 26: // Blue Door/Locked
		if (EV_DoDoor(DDoor::doorRaise, line, thing, 0, SPEED(D_SLOW),
		                                TICS(VDOORWAIT), (card_t)(BCard | CardIsSkull)))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 27: // Yellow Door /Locked
		if (EV_DoDoor(DDoor::doorRaise, line, thing, 0, SPEED(D_SLOW),
		                                TICS(VDOORWAIT), (card_t)(YCard | CardIsSkull)))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 28: // Red Door /Locked
		if (EV_DoDoor(DDoor::doorRaise, line, thing, 0, SPEED(D_SLOW),
		                                TICS(VDOORWAIT), (card_t)(RCard | CardIsSkull)))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 31: // Manual door open
		if (EV_DoDoor(DDoor::doorOpen, line, thing, 0, SPEED(D_SLOW), 0, NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;
	case 32: // Blue locked door open
		if (EV_DoDoor(DDoor::doorOpen, line, thing, 0, SPEED(D_SLOW),
			0, (card_t)(BCard | CardIsSkull)))
		{
			reuse = false;
			trigger = true;
		}
		break;
	case 33: // Red locked door open
		if (EV_DoDoor(DDoor::doorOpen, line, thing, 0, SPEED(D_SLOW), 0,
		                                (card_t)(RCard | CardIsSkull)))
		{
			reuse = false;
			trigger = true;
		}
		break;
	case 34: // Yellow locked door open
		if (EV_DoDoor(DDoor::doorOpen, line, thing, 0, SPEED(D_SLOW), 0,
		                                (card_t)(YCard | CardIsSkull)))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 117: // Blazing door raise
		if (EV_DoDoor(DDoor::doorRaise, line, thing, 0, SPEED(D_FAST),
		                                TICS(VDOORWAIT), NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 118: // Blazing door open
		if (EV_DoDoor(DDoor::doorOpen, line, thing, 0, SPEED(D_FAST), 0, NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	// Switches (non-retriggerable)
	case 7:
		// Build Stairs
		if (EV_BuildStairs(line->id, DFloor::buildUp, line, 8 * FRACUNIT, SPEED(S_SLOW),
		                   0, 0, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 9:
		// Change Donut
		if (EV_DoDonut(line))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 11:
		/* Exit level
		 * killough 10/98: prevent zombies from exiting levels
		 */
		if (!bossaction && thing && thing->player && thing->player->health <= 0)
		{
			return false;
		}

		if (thing && CheckIfExitIsGood(thing))
		{
			reuse = false;
			trigger = true;
			G_ExitLevel(0, 1);
		}
		break;

	case 14:
		// Raise Floor 32 and change texture
		if (EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 4 * 8,
		              SPEED(P_SLOW / 2), 0, 0, 2))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 15:
		// Raise Floor 24 and change texture
		if (EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 3 * 8,
		              SPEED(P_SLOW / 2), 0, 0, 2))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 18:
		// Raise Floor to next highest floor
		if (EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_SLOW), 0, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 20:
		// Raise Plat next highest floor and change texture
		if (EV_DoPlat(line->id, line, DPlat::platRaiseAndStay, 0, SPEED(P_SLOW / 2), 0, 0, 1))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 21:
		// PlatDownWaitUpStay
		if (EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_FAST),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 23:
		// Lower Floor to Lowest
		if (EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0, 0,
		               0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 29:
		// Raise Door
		if (EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_SLOW),
		              TICS(VDOORWAIT), NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 41:
		// Lower Ceiling to Floor
		if (EV_DoCeiling(DCeiling::ceilLowerToFloor, line, line->id, SPEED(C_SLOW), 0, 0,
		                 0, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 71:
		// Turbo Lower Floor
		if (EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_FAST),
		               (136 - 128) * FRACUNIT, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 49:
		// Ceiling Crush And Raise
		if (EV_DoCeiling(DCeiling::crushAndRaise, line, line->id, SPEED(C_SLOW),
		                 SPEED(C_SLOW), 0, true, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 50:
		// Close Door
		if (EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_SLOW), 0, NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 51:
		/* Secret EXIT
		 * killough 10/98: prevent zombies from exiting levels
		 */
		if (!bossaction && thing && thing->player && thing->player->health <= 0)
		{
			return false;
		}

		if (thing && CheckIfExitIsGood(thing))
		{
			reuse = false;
			trigger = true;
			G_SecretExitLevel(0, 1);
		}
		break;

	case 55:
		// Raise Floor Crush
		if (EV_DoFloor(DFloor::floorRaiseAndCrush, line, line->id, SPEED(F_SLOW), 0, true,
		               0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 101:
		// Raise Floor
		if (EV_DoFloor (DFloor::floorRaiseToLowestCeiling, line, line->id, SPEED(F_SLOW), 0, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 102:
		// Lower Floor to Surrounding floor height
		if (EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_SLOW),
		               (128 - 128) * FRACUNIT, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 103:
		// Open Door
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_SLOW), 0, NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 111:
		// Blazing Door Raise (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_FAST),
		              TICS(VDOORWAIT), NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 112:
		// Blazing Door Open (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), 0, NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 113:
		// Blazing Door Close (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_FAST), 0, NoKey))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 122:
		// Blazing PlatDownWaitUpStay
		if (EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_TURBO),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 127:
		// Build Stairs Turbo 16
		if (EV_BuildStairs(line->id, DFloor::buildUp, line, 16 * FRACUNIT, SPEED(S_TURBO),
		                   TICS(0), 0, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 131:
		// Raise Floor Turbo
		if (EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_FAST), 0, 0,
		               0))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 133:
		// BlzOpenDoor BLUE
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id,
		              SPEED(D_FAST), TICS(0), (card_t)(BCard | CardIsSkull)))
		{
			reuse = false;
			trigger = true;
		}
		break;
	case 135:
		// BlzOpenDoor RED
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), TICS(0),
		              (card_t)(RCard | CardIsSkull)))
		{
			reuse = false;
			trigger = true;
		}
		break;
	case 137:
		// BlzOpenDoor YELLOW
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), TICS(0),
		              (card_t)(YCard | CardIsSkull)))
		{
			reuse = false;
			trigger = true;
		}
		break;

	case 140:
		// Raise Floor 512
		if (EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
		               FRACUNIT * 64 * 8, 0, 0))
		{
			reuse = false;
			trigger = true;
		}
		break;

		// killough 1/31/98: factored out compatibility check;
		// added inner switch, relaxed check to demo_compatibility

	default:
		switch (line->special)
		{
			// jff 1/29/98 added linedef types to fill all functions out so that
			// all possess SR, S1, WR, W1 types

		case 158:
			// Raise Floor to shortest lower texture
			// 158 S1  EV_DoFloor(raiseToTexture), CSW(0)
			if (EV_DoFloor(DFloor::floorRaiseByTexture, line, line->id, SPEED(F_SLOW), 0,
			               false, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 159:
			// Raise Floor to shortest lower texture
			// 159 S1  EV_DoFloor(lowerAndChange)
			if (EV_DoFloor(DFloor::floorLowerAndChange, line, line->id, SPEED(F_SLOW),
			               0 * FRACUNIT, false, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 160:
			// Raise Floor 24 and change
			// 160 S1  EV_DoFloor(raiseFloor24AndChange)
			if (EV_DoFloor(DFloor::floorRaiseAndChange, line, line->id, SPEED(F_SLOW),
			               24 * FRACUNIT, 0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 161:
			// Raise Floor 24
			// 161 S1  EV_DoFloor(raiseFloor24)
			if (EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
			               FRACUNIT * 24, 0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 162:
			// Moving floor min n to max n
			// 162 S1  EV_DoPlat(perpetualRaise,0)
			if (EV_DoPlat(line->id, line, DPlat::platPerpetualRaise, 0, SPEED(F_SLOW),
			              TICS(PLATWAIT), 0 * FRACUNIT, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 163:
			// Stop Moving floor
			// 163 S1  EV_DoPlat(perpetualRaise,0)
			EV_StopPlat(line->id);
			reuse = false;
			trigger = true;

			break;

		case 164:
			// Start fast crusher
			// 164 S1  EV_DoCeiling(fastCrushAndRaise)
			if (EV_DoCeiling(DCeiling::fastCrushAndRaise, line, line->id, SPEED(C_NORMAL),
			                 SPEED(C_NORMAL), 0, true, 0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 165:
			// Start slow silent crusher
			// 165 S1  EV_DoCeiling(silentCrushAndRaise)
			if (EV_DoCeiling(DCeiling::silentCrushAndRaise, line, line->id, SPEED(C_SLOW),
			                 SPEED(C_SLOW), 0, true, 1, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 166:
			// Raise ceiling, Lower floor
			// 166 S1 EV_DoCeiling(raiseToHighest), EV_DoFloor(lowerFloortoLowest)
			if (EV_DoCeiling(DCeiling::ceilRaiseToHighest, line, line->id, SPEED(C_SLOW),
			                 0, 0, 0, 0, 0) ||
			    EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0,
			               0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 167:
			// Lower ceiling and Crush
			// 167 S1 EV_DoCeiling(lowerAndCrush)
			if (EV_DoCeiling(DCeiling::lowerAndCrush, line, line->id, SPEED(C_SLOW),
			                 SPEED(C_SLOW) / 2, 0, true, 0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 168:
			// Stop crusher
			// 168 S1 EV_CeilingCrushStop()
			if (EV_CeilingCrushStop(line->id))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 169:
			// Lights to brightest neighbor sector
			// 169 S1  EV_LightTurnOn(0)
			EV_LightTurnOn(line->id, -1);
			reuse = false;
			trigger = true;

			break;

		case 170:
			// Lights to near dark
			// 170 S1  EV_LightTurnOn(35)
			EV_LightTurnOn(line->id, 35);
			reuse = false;
			trigger = true;

			break;

		case 171:
			// Lights on full
			// 171 S1  EV_LightTurnOn(255)
			EV_LightTurnOn(line->id, 255);
			reuse = false;
			trigger = true;

			break;

		case 172:
			// Start Lights Strobing
			// 172 S1  EV_StartLightStrobing()
			EV_StartLightStrobing(line->id, TICS(5), TICS(35));
			reuse = false;
			trigger = true;

			break;

		case 173:
			// Lights to Dimmest Near
			// 173 S1  EV_TurnTagLightsOff()
			EV_TurnTagLightsOff(line->id);
			reuse = false;
			trigger = true;

			break;

		case 174:
			// Teleport
			// 174 S1  Teleport(side,thing)
			if (EV_LineTeleport(line, side, thing))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 175:
			// Close Door, Open in 30 secs
			// 175 S1  EV_DoDoor(close30ThenOpen)
			if (EV_DoDoor(DDoor::doorCloseWaitOpen, line, thing, line->id, SPEED(F_SLOW),
			              OCTICS(240), NoKey))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 189: // jff 3/15/98 create texture change no motion type
			// Texture Change Only (Trigger)
			// 189 S1 Change Texture/Type Only
			if (EV_DoChange(line, trigChangeOnly, line->id))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 203:
			// Lower ceiling to lowest surrounding ceiling
			// 203 S1 EV_DoCeiling(lowerToLowest)
			if (EV_DoCeiling(DCeiling::ceilLowerToLowest, line, line->id, SPEED(C_SLOW),
			                 0, 0, 0, 0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 204:
			// Lower ceiling to highest surrounding floor
			// 204 S1 EV_DoCeiling(lowerToMaxFloor)
			if (EV_DoCeiling(DCeiling::ceilLowerToHighestFloor, line, line->id,
			                 SPEED(C_SLOW), 0, 0, 0, 0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 209:
			// killough 1/31/98: silent teleporter
			// jff 209 S1 SilentTeleport
			if (EV_SilentTeleport(line->args[0], 0, line->args[2], 0, line, side, thing))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 241: // jff 3/15/98 create texture change no motion type
			// Texture Change Only (Numeric)
			// 241 S1 Change Texture/Type Only
			if (EV_DoChange(line, numChangeOnly, line->id))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 221:
			// Lower floor to next lowest floor
			// 221 S1 Lower Floor To Nearest Floor
			if (EV_DoFloor(DFloor::floorLowerToNearest, line, line->id, SPEED(F_SLOW), 0,
			               0, 0))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 229:
			// Raise elevator next floor
			// 229 S1 Raise Elevator next floor
			if (EV_DoElevator(line, DElevator::elevateUp, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 233:
			// Lower elevator next floor
			// 233 S1 Lower Elevator next floor
			if (EV_DoElevator(line, DElevator::elevateDown, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				reuse = false;
				trigger = true;
			}
			break;

		case 237:
			// Elevator to current floor
			// 237 S1 Elevator to current floor
			if (EV_DoElevator(line, DElevator::elevateCurrent, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				reuse = false;
				trigger = true;
			}
			break;

			// jff 1/29/98 end of added S1 linedef types

			// jff 1/29/98 added linedef types to fill all functions out so that
			// all possess SR, S1, WR, W1 types

		case 78: // jff 3/15/98 create texture change no motion type
			// Texture Change Only (Numeric)
			// 78 SR Change Texture/Type Only
			if (EV_DoChange(line, numChangeOnly, line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 176:
			// Raise Floor to shortest lower texture
			// 176 SR  EV_DoFloor(raiseToTexture), CSW(1)
			if (EV_DoChange(line, numChangeOnly, line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 177:
			// Raise Floor to shortest lower texture
			// 177 SR  EV_DoFloor(lowerAndChange)
			if (EV_DoFloor(DFloor::floorLowerAndChange, line, line->id, SPEED(F_SLOW),
			               0 * FRACUNIT, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 178:
			// Raise Floor 512
			// 178 SR  EV_DoFloor(raiseFloor512)
			if (EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
			               FRACUNIT * 64 * 8, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 179:
			// Raise Floor 24 and change
			// 179 SR  EV_DoFloor(raiseFloor24AndChange)
			if (EV_DoFloor(DFloor::floorRaiseAndChange, line, line->id, SPEED(F_SLOW),
			               24 * FRACUNIT, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 180:
			// Raise Floor 24
			// 180 SR  EV_DoFloor(raiseFloor24)
			if (EV_DoFloor(DFloor::floorRaiseByValue, line, line->id, SPEED(F_SLOW),
			               FRACUNIT * 24, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 181:
			// Moving floor min n to max n
			// 181 SR  EV_DoPlat(perpetualRaise,0)

			EV_DoPlat(line->id, line, DPlat::platPerpetualRaise, 0, SPEED(F_SLOW),
			          TICS(PLATWAIT), 0 * FRACUNIT, 0);
			reuse = true;
			trigger = true;

			break;

		case 182:
			// Stop Moving floor
			// 182 SR  EV_DoPlat(perpetualRaise,0)
			EV_StopPlat(line->id);
			reuse = true;
			trigger = true;

			break;

		case 183:
			// Start fast crusher
			// 183 SR  EV_DoCeiling(fastCrushAndRaise)
			if (EV_DoCeiling(DCeiling::fastCrushAndRaise, line, line->id, SPEED(C_NORMAL),
			                 SPEED(C_NORMAL), 0, true, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 184:
			// Start slow crusher
			// 184 SR  EV_DoCeiling(crushAndRaise)
			if (EV_DoCeiling(DCeiling::crushAndRaise, line, line->id, SPEED(C_SLOW),
			                 SPEED(C_SLOW), 0, true, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 185:
			// Start slow silent crusher
			// 185 SR  EV_DoCeiling(silentCrushAndRaise)
			if (EV_DoCeiling(DCeiling::silentCrushAndRaise, line, line->id, SPEED(C_SLOW),
			                 SPEED(C_SLOW), 0, true, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 186:
			// Raise ceiling, Lower floor
			// 186 SR EV_DoCeiling(raiseToHighest), EV_DoFloor(lowerFloortoLowest)
			if (EV_DoCeiling(DCeiling::ceilRaiseToHighest, line, line->id, SPEED(C_SLOW),
			                 0, 0, 0, 0, 0) ||
			    EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0,
			               0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 187:
			// Lower ceiling and Crush
			// 187 SR EV_DoCeiling(lowerAndCrush)
			if (EV_DoCeiling(DCeiling::lowerAndCrush, line, line->id, SPEED(C_SLOW),
			                 SPEED(C_SLOW) / 2, 0, true, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 188:
			// Stop crusher
			// 188 SR EV_CeilingCrushStop()
			if (EV_CeilingCrushStop(line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 190: // jff 3/15/98 create texture change no motion type
			// Texture Change Only (Trigger)
			// 190 SR Change Texture/Type Only
			if (EV_DoChange(line, trigChangeOnly, line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 191:
			// Lower Pillar, Raise Donut
			// 191 SR  EV_DoDonut()
			if (EV_DoDonut(line))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 192:
			// Lights to brightest neighbor sector
			// 192 SR  EV_LightTurnOn(0)
			EV_LightTurnOn(line->id, -1);
			reuse = true;
			trigger = true;

			break;

		case 193:
			// Start Lights Strobing
			// 193 SR  EV_StartLightStrobing()
			EV_StartLightStrobing(line->id, TICS(5), TICS(35));
			reuse = true;
			trigger = true;

			break;

		case 194:
			// Lights to Dimmest Near
			// 194 SR  EV_TurnTagLightsOff()
			EV_TurnTagLightsOff(line->id);
			reuse = true;
			trigger = true;

			break;

		case 195:
			// Teleport
			// 195 SR  Teleport(side,thing)
			if (EV_LineTeleport(line, side, thing))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 196:
			// Close Door, Open in 30 secs
			// 196 SR  EV_DoDoor(close30ThenOpen)
			if (EV_DoDoor(DDoor::doorCloseWaitOpen, line, thing, line->id, SPEED(D_SLOW),
			              OCTICS(240), NoKey))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 205:
			// Lower ceiling to lowest surrounding ceiling
			// 205 SR EV_DoCeiling(lowerToLowest)
			if (EV_DoCeiling(DCeiling::ceilLowerToLowest, line, line->id, SPEED(C_SLOW),
			                 0, 0, 0, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 206:
			// Lower ceiling to highest surrounding floor
			// 206 SR EV_DoCeiling(lowerToMaxFloor)
			if (EV_DoCeiling(DCeiling::ceilLowerToHighestFloor, line, line->id,
			                 SPEED(C_SLOW), 0, 0, 0, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 210:
			// killough 1/31/98: silent teleporter
			// jff 210 SR SilentTeleport
			if (EV_SilentTeleport(line->args[0], 0, line->args[2], 0, line, side, thing))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 211: // jff 3/14/98 create instant toggle floor type
			// Toggle Floor Between C and F Instantly
			// 211 SR Toggle Floor Instant
			if (EV_DoPlat(line->id, line, DPlat::platToggle, 0, 0, 0, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 222:
			// Lower floor to next lowest floor
			// 222 SR Lower Floor To Nearest Floor
			if (EV_DoFloor(DFloor::floorLowerToNearest, line, line->id, SPEED(F_SLOW), 0,
			               0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 230:
			// Raise elevator next floor
			// 230 SR Raise Elevator next floor
			if (EV_DoElevator(line, DElevator::elevateUp, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 234:
			// Lower elevator next floor
			// 234 SR Lower Elevator next floor
			if (EV_DoElevator(line, DElevator::elevateDown, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 238:
			// Elevator to current floor
			// 238 SR Elevator to current floor
			if (EV_DoElevator(line, DElevator::elevateCurrent, SPEED(ELEVATORSPEED), 0,
			                  line->id))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 258:
			// Build stairs, step 8
			// 258 SR EV_BuildStairs(build8)
			if (EV_BuildStairs(line->id, DFloor::buildUp, line, 8 * FRACUNIT,
			                   SPEED(S_SLOW), 0, 0, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

		case 259:
			// Build stairs, step 16
			// 259 SR EV_BuildStairs(turbo16)
			if (EV_BuildStairs(line->id, DFloor::buildUp, line, 16 * FRACUNIT,
			                   SPEED(S_TURBO), 0, 0, 0, 0))
			{
				reuse = true;
				trigger = true;
			}
			break;

			// 1/29/98 jff end of added SR linedef types
		}
	break;

	// Buttons (retriggerable switches)
	case 42:
		// Close Door
		if (EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_SLOW), 0, NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 43:
		// Lower Ceiling to Floor
		if (EV_DoCeiling(DCeiling::ceilLowerToFloor, line, line->id, SPEED(C_SLOW), 0, 0,
		                 false, 0, 0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 45:
		// Lower Floor to Surrounding floor height
		if (EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_SLOW),
		               (128 - 128) * FRACUNIT, 0, 0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 60:
		// Lower Floor to Lowest
		if (EV_DoFloor(DFloor::floorLowerToLowest, line, line->id, SPEED(F_SLOW), 0, 0,
		               0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 61:
		// Open Door
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_SLOW), 0, NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 62:
		// PlatDownWaitUpStay
		if (EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_FAST),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 63:
		// Raise Door
		if (EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_SLOW),
		              TICS(VDOORWAIT), NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 64:
		// Raise Floor to ceiling
		if (EV_DoFloor(DFloor::floorRaiseToLowestCeiling, line, line->id, SPEED(F_SLOW),
		               0, 0, 0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 66:
		// Raise Floor 24 and change texture
		if (EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 3 * 8,
		              SPEED(P_SLOW / 2), 0, 0, 2))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 67:
		// Raise Floor 32 and change texture
		if (EV_DoPlat(line->id, line, DPlat::platUpByValueStay, FRACUNIT * 4 * 8,
		              SPEED(P_SLOW / 2), 0, 0, 2))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 65:
		// Raise Floor Crush
		if (EV_DoFloor(DFloor::floorRaiseAndCrush, line, line->id, SPEED(F_SLOW), 0, true,
		               0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 68:
		// Raise Plat to next highest floor and change texture
		if (EV_DoPlat(line->id, line, DPlat::platRaiseAndStay, 0, SPEED(P_SLOW / 2), 0, 0,
		              1))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 69:
		// Raise Floor to next highest floor
		if (EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_SLOW), 0, 0,
		               0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 70:
		// Turbo Lower Floor
		if (EV_DoFloor(DFloor::floorLowerToHighest, line, line->id, SPEED(F_FAST),
		               (136 - 128) * FRACUNIT, 0, 0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 114:
		// Blazing Door Raise (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorRaise, line, thing, line->id, SPEED(D_FAST),
		              TICS(VDOORWAIT), NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 115:
		// Blazing Door Open (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), 0, NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 116:
		// Blazing Door Close (faster than TURBO!)
		if (EV_DoDoor(DDoor::doorClose, line, thing, line->id, SPEED(D_FAST), 0, NoKey))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 123:
		// Blazing PlatDownWaitUpStay
		if (EV_DoPlat(line->id, line, DPlat::platDownWaitUpStay, 0, SPEED(P_TURBO),
		              TICS(PLATWAIT), 0 * FRACUNIT, 0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 132:
		// Raise Floor Turbo
		if (EV_DoFloor(DFloor::floorRaiseToNearest, line, line->id, SPEED(F_FAST), 0, 0,
		               0))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 99:
		// BlzOpenDoor BLUE
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), TICS(0),
		              (card_t)(BCard | CardIsSkull)))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 134:
		// BlzOpenDoor RED
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), TICS(0),
		              (card_t)(RCard | CardIsSkull)))
		{
			reuse = true;
			trigger = true;
		}
		break;
	case 136:
		// BlzOpenDoor YELLOW
		if (EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_FAST), TICS(0),
		              (card_t)(YCard | CardIsSkull)))
		{
			reuse = true;
			trigger = true;
		}
		break;

	case 138:
		// Light Turn On
		EV_LightTurnOn(line->id, 255);
		reuse = true;
		trigger = true;

		break;

	case 139:
		// Light Turn Off
		EV_LightTurnOn(line->id, 35);
		reuse = true;
		trigger = true;

		break;
	}

	if (trigger && !bossaction)
	{
		if (serverside)
		{
			P_ChangeSwitchTexture(line, reuse, true);
			OnChangedSwitchTexture(line, reuse);
		}
	}

	if (!trigger && bossaction)
	{
		return false;
	}

	return true;
}

//
// P_ShootSpecialLine - Gun trigger special dispatcher
//
// Called when a thing shoots a special line with bullet, shell, saw, or fist.
//
// jff 02/12/98 all G1 lines were fixed to check the result from the EV_
// function before clearing the special. This avoids losing the function
// of the line, should the sector already be in motion when the line is
// impacted. Change is qualified by demo_compatibility.
//

bool P_ShootCompatibleSpecialLine(AActor* thing, line_t* line)
{
	// pointer to line function is NULL by default, set non-null if
	// line special is gun triggered generalized linedef type
	int (*linefunc)(line_t * line) = NULL;

	// check each range of generalized linedefs
	if ((unsigned)line->special >= GenEnd)
	{
		// Out of range for GenFloors
	}
	else if ((unsigned)line->special >= GenFloorBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
			if ((line->special & FloorChange) || !(line->special & FloorModel))
				return false; // FloorModel is "Allow Monsters" if FloorChange is 0
		linefunc = EV_DoGenFloor;
	}
	else if ((unsigned)line->special >= GenCeilingBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
			if ((line->special & CeilingChange) || !(line->special & CeilingModel))
				return false; // CeilingModel is "Allow Monsters" if CeilingChange is 0
		linefunc = EV_DoGenCeiling;
	}
	else if ((unsigned)line->special >= GenDoorBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
		{
			if (!(line->special & DoorMonster))
				return false;            // monsters disallowed from this door
			if (line->flags & ML_SECRET) // they can't open secret doors either
				return false;
		}
		linefunc = EV_DoGenDoor;
	}
	else if ((unsigned)line->special >= GenLockedBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
			return false; // monsters disallowed from unlocking doors
		if (((line->special & TriggerType) == GunOnce) ||
		    ((line->special & TriggerType) == GunMany))
		{ // jff 4/1/98 check for being a gun type before reporting door type
			if (!P_CanUnlockGenDoor(line, thing->player))
				return false;
		}
		else
			return false;
		linefunc = EV_DoGenLockedDoor;
	}
	else if ((unsigned)line->special >= GenLiftBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
			if (!(line->special & LiftMonster))
				return false; // monsters disallowed
		linefunc = EV_DoGenLift;
	}
	else if ((unsigned)line->special >= GenStairsBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
			if (!(line->special & StairMonster))
				return false; // monsters disallowed
		linefunc = EV_DoGenStairs;
	}
	else if ((unsigned)line->special >= GenCrusherBase)
	{
		if (!thing->player && thing->type != MT_AVATAR)
			if (!(line->special & StairMonster))
				return false; // monsters disallowed
		linefunc = EV_DoGenCrusher;
	}

	if (linefunc)
		switch ((line->special & TriggerType) >> TriggerTypeShift)
		{
		case GunOnce:
			if (linefunc(line))
			{
				return true;
			}
			return false;
		case GunMany:
			if (linefunc(line))
			{
				return true;
			}
			return false;
		default: // if not a gun type, do nothing here
			return false;
		}

	// Impacts that other things can activate.
	if (thing && !thing->player && thing->type != MT_AVATAR)
	{
		int ok = 0;
		switch (line->special)
		{
		case 46:
			// 46 GR Open door on impact weapon is monster activatable
			ok = 1;
			break;
		}
		if (!ok)
			return false;
	}

	if (!P_CheckTag(line)) // jff 2/27/98 disallow zero tag on some types
		return false;

	switch (line->special)
	{
	case 24:
		// 24 G1 raise floor to highest adjacent
		if (EV_DoFloor(DFloor::floorRaiseToLowestCeiling, line, line->id, SPEED(F_SLOW),
		               0, 0, 0) ||
		    demoplayback)
		{
			return true;
		}
		break;

	case 46:
		// 46 GR open door, stay open
		EV_DoDoor(DDoor::doorOpen, line, thing, line->id, SPEED(D_SLOW), 0, NoKey);
		return true;

	case 47:
		// 47 G1 raise floor to nearest and change texture and type
		if (EV_DoPlat(line->id, line, DPlat::platRaiseAndStay, 0, SPEED(D_SLOW), 0, 0,
		              1) ||
		    demoplayback)
		{
			return true;
		}
		break;

		// jff 1/30/98 added new gun linedefs here
		// killough 1/31/98: added demo_compatibility check, added inner switch

	default:
		switch (line->special)
		{
		case 197:
			// Exit to next level
			// killough 10/98: prevent zombies from exiting levels
			if (thing && thing->player && thing->player->health <= 0)
				break;
			if (thing && CheckIfExitIsGood(thing))
			{
				G_ExitLevel(0, 1);
				return true;
			}
			break;

		case 198:
			// Exit to secret level
			// killough 10/98: prevent zombies from exiting levels
			if (thing && thing->player && thing->player->health <= 0)
				break;
			if (thing && CheckIfExitIsGood(thing))
			{
				G_SecretExitLevel(0, 1);
				return true;
			}
			break;
			// jff end addition of new gun linedefs
		}
		break;
	}
	return false;
}

const unsigned int P_TranslateCompatibleLineFlags(const unsigned int flags, const bool reserved)
{
	/*
	if (mbf21)
		const unsigned int filter = (flags & ML_RESERVED && comp[comp_reservedlineflag]) ? 0x01ff : 0x3fff;
	else
		const unsigned int filter = 0x03ff;
	*/

	unsigned int filter;

	if (demoplayback || reserved)
		filter = 0x01ff;
	else
		filter = 0x3fff;

	return (flags & filter);
}

void P_PostProcessCompatibleLinedefSpecial(line_t* line)
{
	switch (line->special)
	{ // killough 4/11/98: handle special types
		int j;
	case 260: // killough 4/11/98: translucent 2s textures
#if 0
				lump = sides[*ld->sidenum].special;		// translucency from sidedef
				if (!ld->tag)							// if tag==0,
					ld->tranlump = lump;				// affect this linedef only
				else
					for (j=0;j<numlines;j++)			// if tag!=0,
						if (lines[j].tag == ld->tag)	// affect all matching linedefs
							lines[j].tranlump = lump;
#else
	          // [RH] Second arg controls how opaque it is.
		if (line->id == 0)
			line->lucency = (byte)128;
		else
			for (j = 0; j < numlines; j++)
				if (lines[j].id == line->id)
					lines[j].lucency = (byte)128;
#endif
		line->special = 0;
		break;
	}
}
