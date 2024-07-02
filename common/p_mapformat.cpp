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
#include "odamex.h"

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

enum triggertype
{
	WalkOnce,
	WalkMany,
	SwitchOnce,
	SwitchMany,
	GunOnce,
	GunMany,
	PushOnce,
	PushMany
};

// Migrate some non-hexen data to hexen format, and other misc flags.
void P_MigrateActorInfo(void)
{
	int i;
	static bool migrated = false;

	// Set MF2_PASSMOBJ on dehacked monsters
	// because we don't expose ZDoom's Bits2 BEX extension (yet...)
	// which is the normal way MF2_PASSMOBJ gets set.
	for (i = 0; i < ::num_mobjinfo_types; ++i)
	{
		if (mobjinfo[i].flags & MF_COUNTKILL)
		{
			if (P_AllowPassover())
			{
				if (mobjinfo[i].flags & MF_COUNTKILL)
					mobjinfo[i].flags2 |= MF2_PASSMOBJ;
			}
			else
			{
				if (mobjinfo[i].flags & MF_COUNTKILL)
					mobjinfo[i].flags2 &= ~MF2_PASSMOBJ;
			}
		}
	}

	// Don't forget about lost souls!
	if (P_AllowPassover())
	{
		mobjinfo[MT_SKULL].flags2 |= MF2_PASSMOBJ;
	}
	else
	{
		mobjinfo[MT_SKULL].flags2 &= ~MF2_PASSMOBJ;
	}

	if (map_format.getZDoom() && !migrated)
	{
		migrated = true;

		for (i = 0; i < ::num_mobjinfo_types; ++i)
		{
			if (mobjinfo[i].flags & MF_COUNTKILL)
				mobjinfo[i].flags2 |= MF2_MCROSS | MF2_PUSHWALL;

			if (mobjinfo[i].flags & MF_MISSILE)
				mobjinfo[i].flags2 |= MF2_PCROSS | MF2_IMPACT;
		}

		mobjinfo[MT_SKULL].flags2 |= MF2_MCROSS | MF2_PUSHWALL;
		mobjinfo[MT_PLAYER].flags2 |= MF2_WINDTHRUST | MF2_PUSHWALL;
	}
	else if (!map_format.getZDoom() && migrated)
	{
		migrated = false;

		for (i = 0; i < ::num_mobjinfo_types; ++i)
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
		return P_ActorInZDoomSector(actor);
	else
		return P_ActorInCompatibleSector(actor);
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

bool MapFormat::cross_special_line(line_t* line, int side, AActor* thing,
                                           bool bossaction)
{
	if (map_format.zdoom)
		return P_CrossZDoomSpecialLine(line, side, thing, bossaction);
	else
		return P_CrossCompatibleSpecialLine(line, side, thing, bossaction);
}

void MapFormat::post_process_sidedef_special(side_t* sd, mapsidedef_t* msd,
                                             sector_t* sec, int i)
{
	if (map_format.zdoom)
		P_PostProcessZDoomSidedefSpecial(sd, msd, sec, i);
	else
		P_PostProcessCompatibleSidedefSpecial(sd, msd, sec, i);
}

void MapFormat::post_process_linedef_special(line_t* line)
{
	if (map_format.zdoom)
		P_PostProcessZDoomLinedefSpecial(line);
	else
		P_PostProcessCompatibleLinedefSpecial(line);
}

void MapFormat::P_ApplyZDoomMapFormat(void)
{
	map_format.zdoom = true;
	map_format.hexen = true;
	map_format.generalized_mask = ~0xff;

	P_MigrateActorInfo();
}

void MapFormat::P_ApplyDefaultMapFormat(void)
{
	map_format.zdoom = false;
	map_format.hexen = false;
	map_format.generalized_mask = ~31;

	P_MigrateActorInfo();
}

bool MapFormat::getZDoom(void)
{
	return map_format.zdoom;
}

bool MapFormat::getHexen(void)
{
	return map_format.hexen;
}

short MapFormat::getGeneralizedMask(void)
{
	return map_format.generalized_mask;
}

bool P_IsSpecialBoomRepeatable(const short special)
{
	switch (special)
	{
	case 1:
	case 26:
	case 27:
	case 28:
	case 42:
	case 43:
	case 45:
	case 46:
	case 60:
	case 61:
	case 62:
	case 63:
	case 64:
	case 65:
	case 66:
	case 67:
	case 68:
	case 69:
	case 70:
	case 72:
	case 73:
	case 74:
	case 75:
	case 76:
	case 77:
	case 78:
	case 79:
	case 80:
	case 81:
	case 82:
	case 83:
	case 84:
	case 86:
	case 87:
	case 88:
	case 89:
	case 90:
	case 91:
	case 92:
	case 93:
	case 94:
	case 95:
	case 96:
	case 97:
	case 98:
	case 99:
	case 105:
	case 106:
	case 107:
	case 114:
	case 115:
	case 116:
	case 117:
	case 120:
	case 123:
	case 126:
	case 128:
	case 129:
	case 132:
	case 134:
	case 136:
	case 138:
	case 139:
	case 147:
	case 148:
	case 149:
	case 150:
	case 151:
	case 152:
	case 154:
	case 155:
	case 156:
	case 157:
	case 176:
	case 177:
	case 178:
	case 179:
	case 180:
	case 181:
	case 182:
	case 183:
	case 184:
	case 185:
	case 186:
	case 187:
	case 188:
	case 190:
	case 191:
	case 192:
	case 193:
	case 194:
	case 195:
	case 196:
	case 201:
	case 202:
	case 205:
	case 206:
	case 208:
	case 210:
	case 211:
	case 212:
	case 220:
	case 222:
	case 228:
	case 230:
	case 232:
	case 234:
	case 236:
	case 238:
	case 240:
	case 244:
	case 256:
	case 257:
	case 258:
	case 259:
	case 263:
	case 265:
	case 267:
	case 269:
		return true;
		break;
	}

	if (special >= GenCrusherBase && special <= GenEnd)
	{
		switch ((special & TriggerType) >> TriggerTypeShift)
		{
		case PushOnce:
		case SwitchOnce:
		case WalkOnce:
		case GunOnce:
			return false;
		case PushMany:
		case SwitchMany:
		case WalkMany:
		case GunMany:
			return true;
		}
	}

	return false;
}

bool P_IsExitLine(const short special)
{
	if (map_format.getZDoom())
		return special == 74 || special == 75 || special == 244 || special == 243;

	return special == 11 || special == 52 || special == 197 || special == 51 ||
	       special == 124 || special == 198;
}

bool P_IsTeleportLine(const short special)
{
	if (map_format.getZDoom())
		return special == 70 || special == 71 || special == 154 || special == 215;

	return special == 39 || special == 97 || special == 125 || special == 126 ||
	       special == 174 || special == 195 || special == 207 || special == 208 ||
	       special == 209 || special == 210 || special == 244 || special == 268 ||
	       special == 269;
}

bool P_IsThingTeleportLine(const short special)
{
	if (map_format.getZDoom())
		return false;

	return special == 39 || special == 97 || special == 125 || special == 126 ||
	       special == 174 || special == 195 || special == 208 || special == 243;
}

bool P_IsThingNoFogTeleportLine(const short special)
{
	if (map_format.getZDoom())
		return false;

	return special == 207 || special == 208 || special == 209 || special == 210 ||
	       special == 268 || special == 269;
}

bool P_IsCompatibleLockedDoorLine(const short special)
{
	if (map_format.getZDoom())
		return false;

	return special == 26 || special == 27 || special == 28 || special == 32 ||
	       special == 33 || special == 34;
}

bool P_IsCompatibleBlueDoorLine(const short special)
{
	if (map_format.getZDoom())
		return false;

	int lock = (special & LockedKey) >> LockedKeyShift;
	bool genericlock = false;

	if (lock == BCard || lock == BSkull)
		genericlock = true;

	return special == 26 || special == 32;
}

bool P_IsCompatibleRedDoorLine(const short special)
{
	if (map_format.getZDoom())
		return false;

	int lock = (special & LockedKey) >> LockedKeyShift;
	bool genericlock = false;

	if (lock == RCard || lock == RSkull)
		genericlock = true;

	return special == 28 || special == 33;
}

bool P_IsCompatibleYellowDoorLine(const short special)
{
	if (map_format.getZDoom())
		return false;

	int lock = (special & LockedKey) >> LockedKeyShift;
	bool genericlock = false;

	if (lock == YCard || lock == YSkull)
		genericlock = true;

	return special == 27 || special == 34;
}

bool P_IsLightTagDoorType(const short special)
{
	switch (special)
	{
	case 1:  // Vertical Door
	case 26: // Blue Door/Locked
	case 27: // Yellow Door /Locked
	case 28: // Red Door /Locked

	case 31: // Manual door open
	case 32: // Blue locked door open
	case 33: // Red locked door open
	case 34: // Yellow locked door open

	case 117: // Blazing door raise
	case 118: // Blazing door open
		return true;
	}
	return false;
}