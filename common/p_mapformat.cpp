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
#include "m_argv.h"
#include "actor.h"
#include "p_lnspec.h"
#include "r_state.h"
#include "w_wad.h"

#include "p_boomfspec.h"
#include "p_zdoomhexspec.h"

#include "p_mapformat.h"

MapFormat map_format;

enum door_type_t
{
	door_type_none = -1,
	door_type_red,
	door_type_blue,
	door_type_yellow,
	door_type_unknown = door_type_yellow,
	door_type_multiple
};

// Migrate some non-hexen data to hexen format
void P_MigrateActorInfo(void)
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

void MapFormat::init_sector_special(sector_t* sector)
{
	if (map_format.zdoom)
		P_SpawnZDoomSectorSpecial(sector);
	else
		P_SpawnCompatibleSectorSpecial(sector);
}

void MapFormat::player_in_special_sector(player_t* player)
{
	if (map_format.zdoom)
		P_PlayerInZDoomSector(player);
	else
		P_PlayerInCompatibleSector(player);
}

bool MapFormat::actor_in_special_sector(AActor* actor)
{
	if (map_format.zdoom)
		P_ActorInZDoomSector(actor);
	else
		P_ActorInCompatibleSector(actor);
}

void MapFormat::spawn_scroller(line_t* line, int i)
{
	if (map_format.zdoom)
		P_SpawnZDoomScroller(line, i);
	else
		P_SpawnCompatibleScroller(line, i);
}

void MapFormat::spawn_friction(line_t* line)
{
	if (map_format.zdoom)
		P_SpawnZDoomFriction(line);
	else
		P_SpawnCompatibleFriction(line);
}

void MapFormat::spawn_pusher(line_t* line)
{
	if (map_format.zdoom)
		P_SpawnZDoomPusher(line);
	else
		P_SpawnCompatiblePusher(line);
}

void MapFormat::spawn_extra(int i)
{
	if (map_format.zdoom)
		P_SpawnZDoomExtra(i);
	else
		P_SpawnCompatibleExtra(i);
}

void MapFormat::cross_special_line(line_t* line, int side, AActor* thing, bool bossaction)
{
	if (map_format.zdoom)
		P_CrossZDoomSpecialLine(line, side, thing, bossaction);
	else
		P_CrossCompatibleSpecialLine(line, side, thing, bossaction);
}

void MapFormat::post_process_sidedef_special(side_t* sd, mapsidedef_t* msd,
                                             sector_t* sec, int i)
{
	if (map_format.zdoom)
		P_PostProcessZDoomSidedefSpecial(sd, msd, sec, i);
	else
		P_PostProcessCompatibleSidedefSpecial(sd, msd, sec, i);
}

void MapFormat::P_ApplyZDoomMapFormat(void)
{
	map_format.zdoom = true;
	map_format.hexen = true;
	map_format.polyobjs = false;
	map_format.acs = false;
	map_format.mapinfo = false;
	map_format.sndseq = false;
	map_format.sndinfo = false;
	map_format.animdefs = false;
	map_format.doublesky = false;
	map_format.map99 = false;
	map_format.generalized_mask = ~0xff;
	map_format.switch_activation = ML_SPAC_USE | ML_SPAC_IMPACT | ML_SPAC_PUSH;

	P_MigrateActorInfo();
}

void MapFormat::P_ApplyDefaultMapFormat(void)
{
	map_format.zdoom = false;
	map_format.hexen = false;
	map_format.polyobjs = false;
	map_format.acs = false;
	map_format.mapinfo = false;
	map_format.sndseq = false;
	map_format.sndinfo = false;
	map_format.animdefs = false;
	map_format.doublesky = false;
	map_format.map99 = false;
	map_format.generalized_mask = ~31;
	map_format.switch_activation = 0; // not used

	P_MigrateActorInfo();
}