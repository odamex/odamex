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

#pragma once

void OnChangedSwitchTexture(line_t* line, int useAgain);
void G_SecretExitLevel(int position, int drawscores, bool resetinv);
void P_DamageMobj(AActor* target, AActor* inflictor, AActor* source, int damage, int mod,
                  int flags);
bool P_CrossCompatibleSpecialLine(line_t* line, int side, AActor* thing,
                                          bool bossaction);
unsigned int P_TranslateCompatibleLineFlags(const unsigned int flags, const bool reserved);
void P_ApplyGeneralizedSectorDamage(player_t* player, int bits);
void P_CollectSecretBoom(sector_t* sector, player_t* player);
void P_PlayerInCompatibleSector(player_t* player);
bool P_ActorInCompatibleSector(AActor* actor);
bool P_UseCompatibleSpecialLine(AActor* thing, line_t* line, int side,
                                        bool bossaction);
bool P_ShootCompatibleSpecialLine(AActor* thing, line_t* line);
BOOL EV_DoGenDoor(line_t* line);
BOOL EV_DoGenFloor(line_t* line);
BOOL EV_DoGenCeiling(line_t* line);
BOOL EV_DoGenLift(line_t* line);
BOOL EV_DoGenStairs(line_t* line);
bool P_CanUnlockGenDoor(line_t* line, player_t* player);
BOOL EV_DoGenLockedDoor(line_t* line);
BOOL EV_DoGenCrusher(line_t* line);
int EV_DoDonut(line_t* line);
void P_CollectSecretVanilla(sector_t* sector, player_t* player);
void EV_StartLightStrobing(int tag, int upper, int lower, int utics, int ltics);
void EV_StartLightStrobing(int tag, int utics, int ltics);
void P_SetTransferHeightBlends(side_t* sd, const mapsidedef_t* msd);
void SetTextureNoErr(short* texture, unsigned int* color, char* name);
void P_AddSectorSecret(sector_t* sector);
void P_SpawnLightFlash(sector_t* sector);
void P_SpawnStrobeFlash(sector_t* sector, int utics, int ltics, bool inSync);
void P_SpawnFireFlicker(sector_t* sector);
AActor* P_GetPushThing(int);
void P_PostProcessCompatibleLinedefSpecial(line_t* line);
bool P_IsTeleportLine(const short special);

extern BOOL demoplayback;
