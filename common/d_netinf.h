// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
	GENDER_CYBORG,
	GENDER_OTHER,

	NUMGENDER
};

inline auto format_as(gender_t eGender)
{
	return fmt::underlying(eGender);
}

enum colorpreset_t // [Acts 19 quiz] The order must match m_menu.cpp.
{
	COLOR_GREEN,
	COLOR_INDIGO,
	COLOR_BROWN,
	COLOR_RED,
	COLOR_BLUE,
	COLOR_ORANGE,
	COLOR_GOLD,
	NUMVANILLACOLOR = COLOR_GOLD, // [Acts 19 quiz] The first non-indexed color.
	COLOR_JUNGLEGREEN,
	COLOR_PURPLE,
	COLOR_WHITE,
	COLOR_BLACK,
	COLOR_CUSTOM,

	NUMCOLOR
};

// [Acts 19 quiz] The playerinfo prints in cl_main.cpp and sv_main.cpp are dependent
// on this.

inline auto format_as(colorpreset_t eColorpreset)
{
	return fmt::underlying(eColorpreset);
}

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
	colorpreset_t	colorpreset;
	byte			color[4];
	gender_t		gender;
	weaponswitch_t	switchweapon;
	byte			weapon_prefs[NUMWEAPONS];

	// The default preference ordering when the player runs out of one type of ammo.
	// Vanilla Doom compatible.
	static constexpr byte weapon_prefs_default[NUMWEAPONS] = {
		0, // wp_fist
		4, // wp_pistol
		5, // wp_shotgun
		6, // wp_chaingun
		1, // wp_missile
		8, // wp_plasma
		2, // wp_bfg
		3, // wp_chainsaw
		7  // wp_supershotgun
	};

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
