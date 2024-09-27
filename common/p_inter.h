// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	The Give commands (?)
//
//-----------------------------------------------------------------------------

// [AM] Seems like this header should be "p_interaction.h"...

#pragma once

#include "d_player.h"

#define BONUSADD 6

bool P_GiveFrags(player_t* player, int num);
bool P_GiveKills(player_t* player, int num);
bool P_GiveDeaths(player_t* player, int num);
bool P_GiveMonsterDamage(player_t* player, int num);
bool P_GiveTeamPoints(player_t* player, int num);
bool P_GiveLives(player_t* player, int num);
int P_GetFragCount(const player_t* player);
int P_GetPointCount(const player_t* player);
int P_GetDeathCount(const player_t* player);
ItemEquipVal P_GiveAmmo(player_t* player, ammotype_t ammotype, float num);
ItemEquipVal P_GiveWeapon(player_t* player, weapontype_t weapon, BOOL dropped);
ItemEquipVal P_GiveArmor(player_t* player, int armortype);
ItemEquipVal P_GiveCard(player_t* player, card_t card);
ItemEquipVal P_GivePower(player_t* player, int /*powertype_t*/ power);
void P_KillMobj(AActor* source, AActor* target, AActor* inflictor, bool joinkill);
void P_HealMobj(AActor* mo, int num);
