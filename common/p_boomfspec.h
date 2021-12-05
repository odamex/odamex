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
//  [Blair] Define the Doom (Doom in Doom) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//  "Attempts" to be Boom compatible, hence the name.
//
//-----------------------------------------------------------------------------

#ifndef __P_BOOMFSPEC__
#define __P_BOOMFSPEC__

#include "r_defs.h"
#include "actor.h"
#include "doomdef.h"
#include "doomtype.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "p_spec.h"
#include "d_player.h"

void P_CrossCompatibleSpecialLine(line_t* line, int side, AActor* thing, bool bossaction);
void P_ApplyGeneralizedSectorDamage(player_t* player, int bits);
void P_CollectSecretBoom(sector_t* sector, player_t* player);
void P_PlayerInCompatibleSector(player_t* player);
bool P_ActorInCompatibleSector(AActor* actor);
bool P_UseCompatibleSpecialLine(AActor* thing, line_t* line, int side, bool bossaction);

#endif
