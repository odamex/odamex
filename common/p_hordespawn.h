// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  "Horde" game mode spawn selection.
//
//-----------------------------------------------------------------------------

#pragma once

#include "actor.h"
#include "p_hordedefine.h"

#define TTYPE_HORDE_SMALLMONSTER (5300)
#define TTYPE_HORDE_ITEM (5301)
#define TTYPE_HORDE_MONSTER (5302)
#define TTYPE_HORDE_BOSS (5303)
#define TTYPE_HORDE_FLYING (5304)
#define TTYPE_HORDE_SMALLSNIPER (5305)
#define TTYPE_HORDE_SNIPER (5306)
#define TTYPE_HORDE_POWERUP (5307)
#define TTYPE_HORDE_SMALLBOSS (5308)

static inline const char* HordeThingStr(const int ttype)
{
	switch (ttype)
	{
	case TTYPE_HORDE_SMALLMONSTER:
		return "Small Monster";
	case TTYPE_HORDE_ITEM:
		return "Item";
	case TTYPE_HORDE_MONSTER:
		return "Monster";
	case TTYPE_HORDE_BOSS:
		return "Boss";
	case TTYPE_HORDE_FLYING:
		return "Flying";
	case TTYPE_HORDE_SMALLSNIPER:
		return "Small Sniper";
	case TTYPE_HORDE_SNIPER:
		return "Sniper";
	case TTYPE_HORDE_POWERUP:
		return "Powerup";
	case TTYPE_HORDE_SMALLBOSS:
		return "Small Boss";
	default:
		return "Unknown";
	}
}

struct hordeSpawn_t
{
	AActor::AActorPtr mo;
	int type;
};
typedef std::vector<hordeSpawn_t> hordeSpawns_t;

void P_HordeAddSpawns();
bool P_HordeHasSpawns();
bool P_HordeHasRequiredMonsterSpawns();
void P_HordeClearSpawns();
hordeSpawn_t* P_HordeSpawnPoint(const hordeRecipe_t& recipe);
AActors P_HordeSpawn(hordeSpawn_t& spawn, const hordeRecipe_t& recipe);
void P_HordeSpawnItem();
void P_HordeSpawnPowerup(const mobjtype_t pw);
