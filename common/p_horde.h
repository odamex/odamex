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
//  "Horde" game mode.
//
//-----------------------------------------------------------------------------

#ifndef __P_HORDE__
#define __P_HORDE__

#include "actor.h"
#include "doomdata.h"

static const int TTYPE_HORDE_MONSTER = 5301;
static const int TTYPE_HORDE_ITEM = 5302;
static const int TTYPE_HORDE_BOSS = 5303;
static const int TTYPE_HORDE_FLYING = 5304;
static const int TTYPE_HORDE_SNIPER = 5305;

enum hordeState_e
{
	HS_STARTING,
	HS_PRESSURE,
	HS_RELAX,
	HS_BOSS,
};

struct hordeInfo_t
{
	hordeState_e state;
	int round;
	int spawned;
	int killed;
	int goal;
};

hordeInfo_t P_HordeInfo();
void P_ClearHordeSpawnPoints();
void P_AddHordeSpawnPoint(AActor* mo, const int type);
void P_AddHealthPool(AActor* mo);
void P_RemoveHealthPool(AActor* mo);

void P_RunHordeTics();
bool P_IsHordeMode();
bool P_IsHordeThing(const int type);

#endif // __G_DEATHWISH__
