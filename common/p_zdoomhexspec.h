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
//  [Blair] Define the ZDoom (Doom in Hexen) format doom map spec.
//  Includes sector specials, linedef types, line crossing.
//
//-----------------------------------------------------------------------------

#pragma once

void OnChangedSwitchTexture(line_t* line, int useAgain);
void SV_OnActivatedLine(line_t* line, AActor* mo, const int side,
                        const LineActivationType activationType, const bool bossaction);
BOOL EV_DoZDoomDoor(DDoor::EVlDoor type, line_t* line, AActor* mo, byte tag,
                    byte speed_byte, int topwait, zdoom_lock_t lock, byte lightTag,
                    bool boomgen, int topcountdown);
bool EV_DoZDoomPillar(DPillar::EPillar type, line_t* line, int tag, fixed_t speed,
                      fixed_t floordist, fixed_t ceilingdist, int crush, bool hexencrush);
bool EV_DoZDoomElevator(line_t* line, DElevator::EElevator type, fixed_t speed,
                        fixed_t height, int tag);
BOOL EV_DoZDoomDonut(int tag, line_t* line, fixed_t pillarspeed, fixed_t slimespeed);
BOOL EV_ZDoomCeilingCrushStop(int tag, bool remove);
void P_HealMobj(AActor* mo, int num);
void EV_LightSetMinNeighbor(int tag);
void EV_LightSetMaxNeighbor(int tag);
void P_ApplySectorFriction(int tag, int value, bool use_thinker);
void P_SetupSectorDamage(sector_t* sector, short amount, byte interval, byte leakrate,
                         unsigned int flags);
void P_AddSectorSecret(sector_t* sector);
void P_SpawnLightFlash(sector_t* sector);
void P_SpawnStrobeFlash(sector_t* sector, int utics, int ltics, bool inSync);
void P_SpawnFireFlicker(sector_t* sector);
bool P_CrossZDoomSpecialLine(line_t* line, int side, AActor* thing,
                                     bool bossaction);
bool P_ActivateZDoomLine(line_t* line, AActor* mo, int side,
                                 unsigned int activationType);
void P_CollectSecretZDoom(sector_t* sector, player_t* player);
bool P_TestActivateZDoomLine(line_t* line, AActor* mo, int side,
                             unsigned int activationType);
bool P_ExecuteZDoomLineSpecial(int special, short* args, line_t* line, int side,
                               AActor* mo);
BOOL EV_DoZDoomFloor(DFloor::EFloor floortype, line_t* line, int tag, fixed_t speed,
                     fixed_t height, int crush, int change, bool hexencrush,
                     bool hereticlower);
BOOL EV_DoZDoomCeiling(DCeiling::ECeiling type, line_t* line, byte tag, fixed_t speed,
                       fixed_t speed2, fixed_t height, int crush, byte silent, int change,
                       crushmode_e crushmode);
void P_SetTransferHeightBlends(side_t* sd, const mapsidedef_t* msd);
void SetTextureNoErr(short* texture, unsigned int* color, char* name);

int P_Random(AActor* actor);
LineActivationType P_LineActivationTypeForSPACFlag(
    const unsigned int activationType);
void P_SpawnPhasedLight(sector_t* sector, int base, int index);
void P_SpawnLightSequence(sector_t* sector);
AActor* P_GetPushThing(int s);
void P_PostProcessZDoomLinedefSpecial(line_t* line);
