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
//     Defines needed by the WDL stats logger.
//
//-----------------------------------------------------------------------------

#ifndef __WDLSTATS_H__
#define __WDLSTATS_H__

#include "d_player.h"

enum WDLEvents {
	WDL_EVENT_DAMAGE,
	WDL_EVENT_CARRIERDAMAGE,
	WDL_EVENT_KILL,
	WDL_EVENT_CARRIERKILL,
	WDL_EVENT_ENVIRODAMAGE,
	WDL_EVENT_ENVIROCARRIERDAMAGE,
	WDL_EVENT_ENVIROKILL,
	WDL_EVENT_ENVIROCARRIERKILL,
	WDL_EVENT_TOUCH,
	WDL_EVENT_PICKUPTOUCH,
	WDL_EVENT_CAPTURE,
	WDL_EVENT_PICKUPCAPTURE,
	WDL_EVENT_ASSIST,
	WDL_EVENT_RETURNFLAG,
	WDL_EVENT_POWERPICKUP,
	WDL_EVENT_ACCURACY,
};

enum WDLPowerups {
	WDL_PICKUP_SOULSPHERE,
	WDL_PICKUP_MEGASPHERE,
	WDL_PICKUP_BLUEARMOR,
	WDL_PICKUP_GREENARMOR,
	WDL_PICKUP_BERSERK,
	WDL_PICKUP_STIMPACK,
	WDL_PICKUP_MEDKIT,
	WDL_PICKUP_HEALTHBONUS,
	WDL_PICKUP_ARMORBONUS,
};

/**
 * Weapons used by the Accuracy event.
 * 
 * There's probably an equivalent enum for these elsewhere.
 */
enum WDLWeapons {
	WDL_WEAPON_FIST,
	WDL_WEAPON_PISTOL,
	WDL_WEAPON_SHOTGUN,
	WDL_WEAPON_CHAINGUN,
	WDL_WEAPON_MISSLE,
	WDL_WEAPON_PLASMA,
	WDL_WEAPON_BFG,
	WDL_WEAPON_CHAINSAW,
	WDL_WEAPON_SSG,
};

void M_StartWDLLog();
void M_LogWDLEvent(
	WDLEvents event, player_t* activator, player_t* target,
	int arg0, int arg1, int arg2
);
void M_LogActorWDLEvent(
	WDLEvents event, AActor* activator, AActor* target,
	int arg0, int arg1, int arg2
);
void M_MaybeLogWDLAccuracyMiss(player_t* activator, int arg0, int arg1);
weapontype_t M_MODToWeapon(int mod);
void M_CommitWDLLog();

#endif
