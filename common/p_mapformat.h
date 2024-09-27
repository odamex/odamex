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
//	Determine map format and handle it accordingly.
//  (Props to DSDA-Doom for the inspiration.)
//
//-----------------------------------------------------------------------------

#pragma once

#include "p_local.h"

class MapFormat
{
  public:
	void P_ApplyZDoomMapFormat(void);
	void P_ApplyDefaultMapFormat(void);

	bool getZDoom(void);
	bool getHexen(void);
	short getGeneralizedMask(void);

	void init_sector_special(sector_t*);
	void player_in_special_sector(player_t*);
	bool actor_in_special_sector(AActor*);
	void spawn_scroller(line_t*, int);
	void spawn_friction(line_t*);
	void spawn_pusher(line_t*);
	void spawn_extra(int);
	bool cross_special_line(line_t*, int, AActor*, bool);
	void post_process_sidedef_special(side_t*, mapsidedef_t*, sector_t*, int);
	void post_process_linedef_special(line_t* line);

  protected:
	bool zdoom;
	bool hexen;
	short generalized_mask;
};

extern MapFormat map_format;

void P_SpawnZDoomSectorSpecial(sector_t*);
void P_SpawnCompatibleSectorSpecial(sector_t*);

bool P_ActorInZDoomSector(AActor*);
bool P_ActorInCompatibleSector(AActor* actor);

void P_PlayerInCompatibleSector(player_t* player);
void P_PlayerInZDoomSector(player_t* player);

void P_SpawnZDoomScroller(line_t*, int);
void P_SpawnCompatibleScroller(line_t*, int);

void P_SpawnZDoomFriction(line_t* l);
void P_SpawnCompatibleFriction(line_t* l);

void P_SpawnCompatiblePusher(line_t* l);
void P_SpawnZDoomPusher(line_t* l);

void P_SpawnCompatibleExtra(int i);
void P_SpawnZDoomExtra(int i);

void P_RecordCompatibleLineSpecial(line_t* ld, maplinedef_t* mld);
void P_RecordZDoomLineSpecial(line_t* ld, maplinedef_t* mld);

bool P_CrossZDoomSpecialLine(line_t* line, int side, AActor* thing,
                                     bool bossaction);
bool P_CrossCompatibleSpecialLine(line_t* line, int side, AActor* thing,
                                          bool bossaction);

void P_PostProcessZDoomSidedefSpecial(side_t* sd, mapsidedef_t* msd, sector_t* sec,
                                      int i);
void P_PostProcessCompatibleSidedefSpecial(side_t* sd, mapsidedef_t* msd,
                                           sector_t* sec, int i);

void P_PostProcessZDoomLinedefSpecial(line_t* line);
void P_PostProcessCompatibleLinedefSpecial(line_t* line);

bool P_IsSpecialBoomRepeatable(const short special);
bool P_IsExitLine(const short special);
bool P_IsTeleportLine(const short special);
bool P_IsThingTeleportLine(const short special);
bool P_IsThingNoFogTeleportLine(const short special);
bool P_IsCompatibleLockedDoorLine(const short special);
bool P_IsCompatibleBlueDoorLine(const short special);
bool P_IsCompatibleRedDoorLine(const short special);
bool P_IsCompatibleYellowDoorLine(const short special);
bool P_IsLightTagDoorType(const short special);
