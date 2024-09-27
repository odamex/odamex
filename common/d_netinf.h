// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Multiplayer properties (?)
//
//-----------------------------------------------------------------------------

#pragma once

#include "teaminfo.h"

#define MAXPLAYERNAME	15

enum gender_t
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTER,
	
	NUMGENDER
};

enum colorpreset_t	// Acts 19 quiz the order must match m_menu.cpp.
{
	COLOR_CUSTOM,
	COLOR_BLUE,
	COLOR_INDIGO,
	COLOR_GREEN,
	COLOR_BROWN,
	COLOR_RED,
	COLOR_GOLD,
	COLOR_JUNGLEGREEN,
	COLOR_PURPLE,
	COLOR_WHITE,
	COLOR_BLACK,

	NUMCOLOR
};

enum weaponswitch_t
{
	WPSW_NEVER,
	WPSW_ALWAYS,
	WPSW_PWO,
	WPSW_PWO_ALT,	// PWO but never switch if holding +attack

	WPSW_NUMTYPES
};

struct UserInfo
{
	std::string		netname;
	team_t			team; // [Toke - Teams]
	fixed_t			aimdist;
	bool			predict_weapons;
	byte			color[4];
	gender_t		gender;
	weaponswitch_t	switchweapon;
	byte			weapon_prefs[NUMWEAPONS];

	static const byte weapon_prefs_default[NUMWEAPONS];

	UserInfo() : team(TEAM_NONE), aimdist(0),
	             predict_weapons(true),
	             gender(GENDER_MALE), switchweapon(WPSW_ALWAYS)
	{
		// default doom weapon ordering when player runs out of ammo
		memcpy(weapon_prefs, UserInfo::weapon_prefs_default, sizeof(weapon_prefs));
		memset(color, 0, 4);
	}
};

FArchive &operator<< (FArchive &arc, UserInfo &info);
FArchive &operator>> (FArchive &arc, UserInfo &info);

void D_SetupUserInfo (void);

void D_UserInfoChanged (cvar_t *info);

void D_SendServerInfoChange (const cvar_t *cvar, const char *value);
void D_DoServerInfoChange (byte **stream);

void D_WriteUserInfoStrings (int player, byte **stream, bool compact=false);
void D_ReadUserInfoStrings (int player, byte **stream, bool update);
