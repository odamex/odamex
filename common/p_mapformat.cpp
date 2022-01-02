// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Determine map format and handle it accordingly.
//  (Props to DSDA-Doom for the inspiration.)
//
//-----------------------------------------------------------------------------
#include "doomstat.h"
#include "doomdata.h"
#include "lprintf.h"
#include "m_argv.h"
#include "actor.h"
#include "p_lnspec.h"
#include "r_state.h"
#include "w_wad.h"

#include "p_boomfspec.h"
#include "p_zdoomhexspec.h"

#include "p_mapformat.h"

map_format_s map_format;

enum door_type_t
{
	door_type_none = -1,
	door_type_red,
	door_type_blue,
	door_type_yellow,
	door_type_unknown = door_type_yellow,
	door_type_multiple
};

int P_DoorType(int index)
{
	int special = lines[index].special;

	if (map_format.hexen)
	{
		if (special == 13 || special == 83)
			return door_type_unknown;

		return door_type_none;
	}

	if (GenLockedBase <= special && special < GenDoorBase)
	{
		special -= GenLockedBase;
		special = (special & LockedKey) >> LockedKeyShift;
		if (!special || special == 7)
			return door_type_multiple;
		else
			return (special - 1) % 3;
	}

	switch (special)
	{
	case 26:
	case 32:
	case 99:
	case 133:
		return door_type_blue;
	case 27:
	case 34:
	case 136:
	case 137:
		return door_type_yellow;
	case 28:
	case 33:
	case 134:
	case 135:
		return door_type_red;
	default:
		return door_type_none;
	}
}

bool P_IsExitLine(int index)
{
	int special = lines[index].special;

	if (map_format.hexen)
		return special == 74 || special == 75;

	return special == 11 || special == 52 || special == 197 || special == 51 ||
	       special == 124 || special == 198;
}

bool P_IsTeleportLine(int index)
{
	int special = lines[index].special;

	if (map_format.hexen)
		return special == 70 || special == 71;

	return special == 39 || special == 97 || special == 125 || special == 126;
}

// Migrate some non-hexen data to hexen format
static void P_MigrateActorInfo(void)
{
	int i;
	static bool migrated = false;

	if (map_format.zdoom && !migrated)
	{
		migrated = true;

		for (i = 0; i < NUMMOBJTYPES; ++i)
		{
			if (mobjinfo[i].flags & MF_COUNTKILL)
				mobjinfo[i].flags2 |= MF2_MCROSS | MF2_PUSHWALL;

			if (mobjinfo[i].flags & MF_MISSILE)
				mobjinfo[i].flags2 |= MF2_PCROSS | MF2_IMPACT;
		}

		mobjinfo[MT_SKULL].flags2 |= MF2_MCROSS | MF2_PUSHWALL;
		mobjinfo[MT_PLAYER].flags2 |= MF2_WINDTHRUST | MF2_PUSHWALL;
	}
	else if (!map_format.zdoom && migrated)
	{
		migrated = false;

		for (i = 0; i < NUMMOBJTYPES; ++i)
		{
			if (mobjinfo[i].flags & MF_COUNTKILL)
				mobjinfo[i].flags2 &= ~(MF2_MCROSS | MF2_PUSHWALL);

			if (mobjinfo[i].flags & MF_MISSILE)
				mobjinfo[i].flags2 &= ~(MF2_PCROSS | MF2_IMPACT);
		}

		mobjinfo[MT_SKULL].flags2 &= ~(MF2_MCROSS | MF2_PUSHWALL);
		mobjinfo[MT_PLAYER].flags2 &= ~(MF2_WINDTHRUST | MF2_PUSHWALL);
	}
}

extern void P_SpawnCompatibleSectorSpecial(sector_t* sector, int i);
void P_SpawnZDoomSectorSpecials(sector_t* sector);

extern void P_PlayerInCompatibleSector(player_t* player, sector_t* sector);
extern void P_PlayerInZDoomSector(player_t* player, sector_t* sector);

extern void P_SpawnCompatibleScroller(line_t* l, int i);
extern void P_SpawnZDoomScroller(line_t* l, int i);

extern void P_SpawnCompatibleFriction(line_t* l);
extern void P_SpawnZDoomFriction(line_t* l);

extern void P_SpawnCompatiblePusher(line_t* l);
extern void P_SpawnZDoomPusher(line_t* l);

extern void P_SpawnCompatibleExtra(line_t* l, int i);
extern void P_SpawnZDoomExtra(line_t* l, int i);

extern void P_CrossCompatibleSpecialLine(line_t* line, int side, AActor* thing,
                                         bool bossaction);
extern void P_CrossZDoomSpecialLine(line_t* line, int side, AActor* thing,
                                    bool bossaction);

extern void P_ShootCompatibleSpecialLine(AActor* thing, line_t* line);
extern void P_ShootHexenSpecialLine(AActor* thing, line_t* line);

extern bool P_TestActivateZDoomLine(line_t* line, AActor* mo, int side,
                                        unsigned int activationType);

extern void P_PostProcessCompatibleLineSpecial(line_t* ld);
extern void P_PostProcessZDoomLineSpecial(line_t* ld);
extern void P_PostProcessZDoomLineSectorSpecial(int line);

extern void P_PostProcessCompatibleSidedefSpecial(side_t* sd, const mapsidedef_t* msd,
                                                  sector_t* sec, int i);
extern void P_PostProcessZDoomSidedefSpecial(side_t* sd, const mapsidedef_t* msd,
                                             sector_t* sec, int i);

extern void P_AnimateCompatibleSurfaces(void);
extern void P_AnimateZDoomSurfaces(void);

extern void P_CheckCompatibleImpact(AActor*);
extern void P_CheckZDoomImpact(AActor*);

extern void P_TranslateZDoomLineFlags(unsigned int*);
extern void P_TranslateCompatibleLineFlags(unsigned int*);

extern void P_ApplyCompatibleSectorMovementSpecial(AActor*, int);
extern void P_ApplyHereticSectorMovementSpecial(AActor*, int);

extern bool P_ActorInCompatibleSector(AActor*);
extern bool P_ActorInZDoomSector(AActor*);

extern void P_CompatiblePlayerThrust(player_t* player, angle_t angle, fixed_t move);

extern bool P_ExecuteZDoomLineSpecial(int special, byte* args, line_t* line, int side,
                                          AActor* mo);
extern bool P_ExecuteHexenLineSpecial(int special, byte* args, line_t* line, int side,
                                          AActor* mo);

extern void T_VerticalCompatibleDoor(vldoor_t* door);
extern void T_VerticalHexenDoor(vldoor_t* door);

extern void T_MoveCompatibleFloor(floormove_t*);

void T_MoveCompatibleCeiling(ceiling_t* ceiling);

int EV_CompatibleTeleport(int tag, line_t* line, int side, AActor* thing, int flags);

static const map_format_s zdoom_in_hexen_map_format = {
    true, //zdoom
    true, //hexen
    false, //polyobjs (true when detected)
    false, //acs (true when detected)
    false, // mapinfo (true when detected)
    false, // sndseq (true when detected)
    false, // sndinfo (true when detected)
    false, // animdefs (true when detected)
    false, // doublesky (true when detected)
    false, // map99 (true when detected)
    true, // lax_monster_activation
    ~0xff, // generalized_mask
    ML_SPAC_USE | ML_SPAC_IMPACT | ML_SPAC_PUSH, // switch_activation
    P_SpawnZDoomSectorSpecial, // init_sector_special
    .player_in_special_sector = P_PlayerInZDoomSector,
    .actor_in_special_sector = P_ActorInZDoomSector,
    .spawn_scroller = P_SpawnZDoomScroller,
    .spawn_friction = P_SpawnZDoomFriction,
    .spawn_pusher = P_SpawnZDoomPusher,
    .spawn_extra = P_SpawnZDoomExtra,
    .cross_special_line = P_CrossZDoomSpecialLine,
    .shoot_special_line = P_ShootHexenSpecialLine,
    .test_activate_line = P_TestActivateZDoomLine,
    .execute_line_special = P_ExecuteZDoomLineSpecial,
    .post_process_line_special = P_PostProcessZDoomLineSpecial,
    .post_process_sidedef_special = P_PostProcessZDoomSidedefSpecial,
    .animate_surfaces = P_AnimateZDoomSurfaces,
    .check_impact = P_CheckZDoomImpact,
    .translate_line_flags = P_TranslateZDoomLineFlags,
    .apply_sector_movement_special = P_ApplyHereticSectorMovementSpecial,
    .mt_push = MT_PUSH,
    .mt_pull = MT_PULL,
};

static const map_format_s doom_map_format = {
    .zdoom = false,
    .hexen = false,
    .polyobjs = false,
    .acs = false,
    .mapinfo = false,
    .sndseq = false,
    .sndinfo = false,
    .animdefs = false,
    .doublesky = false,
    .map99 = false,
    .lax_monster_activation = true,
    .generalized_mask = ~31,
    .switch_activation = 0, // not used
    .init_sector_special = P_SpawnCompatibleSectorSpecial,
    .actor_in_special_sector = P_ActorInCompatibleSectorSpecial,
    .player_in_special_sector = P_PlayerInCompatibleSector,
    .mobj_in_special_sector = P_ActorInCompatibleSector,
    .spawn_scroller = P_SpawnCompatibleScroller,
    .spawn_friction = P_SpawnCompatibleFriction,
    .spawn_pusher = P_SpawnCompatiblePusher,
    .spawn_extra = P_SpawnCompatibleExtra,
    .cross_special_line = P_CrossCompatibleSpecialLine,
    .shoot_special_line = P_ShootCompatibleSpecialLine,
    .post_process_line_special = P_PostProcessCompatibleLineSpecial,
    .post_process_sidedef_special = P_PostProcessCompatibleSidedefSpecial,
    .animate_surfaces = P_AnimateCompatibleSurfaces,
    .check_impact = P_CheckCompatibleImpact,
    .translate_line_flags = P_TranslateCompatibleLineFlags,
    .apply_sector_movement_special = P_ApplyCompatibleSectorMovementSpecial,
    .mt_push = MT_PUSH,
    .mt_pull = MT_PULL,
};

void P_ApplyZDoomMapFormat(void)
{
	map_format = zdoom_in_hexen_map_format;

	P_MigrateActorInfo();
}

void P_ApplyDefaultMapFormat(void)
{
	map_format = doom_map_format;

	P_MigrateActorInfo();
}