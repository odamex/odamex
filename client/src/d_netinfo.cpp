// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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
//	D_NETINFO
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "doomtype.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_netinf.h"
#include "d_net.h"
#include "d_protocol.h"
#include "v_palette.h"
#include "v_video.h"
#include "i_system.h"
#include "r_draw.h"
#include "r_state.h"
#include "st_stuff.h"
#include "cl_main.h"

#include "p_ctf.h"

// The default preference ordering when the player runs out of one type of ammo.
// Vanilla Doom compatible.
const byte UserInfo::weapon_prefs_default[NUMWEAPONS] = {
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

EXTERN_CVAR (cl_autoaim)
EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_gender)
EXTERN_CVAR (cl_skin)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_unlag)
EXTERN_CVAR (cl_updaterate)
EXTERN_CVAR (cl_switchweapon)
EXTERN_CVAR (cl_weaponpref_fst)
EXTERN_CVAR (cl_weaponpref_csw)
EXTERN_CVAR (cl_weaponpref_pis)
EXTERN_CVAR (cl_weaponpref_sg)
EXTERN_CVAR (cl_weaponpref_ssg)
EXTERN_CVAR (cl_weaponpref_cg)
EXTERN_CVAR (cl_weaponpref_rl)
EXTERN_CVAR (cl_weaponpref_pls)
EXTERN_CVAR (cl_weaponpref_bfg)
EXTERN_CVAR (cl_predictweapons)

enum
{
	INFO_Name,
	INFO_Autoaim,
	INFO_Color,
	INFO_Skin,
	INFO_Team,
	INFO_Gender,
    INFO_Unlag
};




gender_t D_GenderByName (const char *gender)
{
	if (!stricmp (gender, "female"))
		return GENDER_FEMALE;
	else if (!stricmp (gender, "cyborg"))
		return GENDER_NEUTER;
	else
		return GENDER_MALE;
}

//
//	[Toke - Teams] D_TeamToInt
//	Convert team string to team integer
//
team_t D_TeamByName (const char *team)
{
	if (!stricmp (team, "blue"))
		return TEAM_BLUE;

	else if (!stricmp (team, "red"))
		return TEAM_RED;

	else return TEAM_NONE;
}


static cvar_t *weaponpref_cvar_map[NUMWEAPONS] = {
	&cl_weaponpref_fst, &cl_weaponpref_pis, &cl_weaponpref_sg, &cl_weaponpref_cg,
	&cl_weaponpref_rl, &cl_weaponpref_pls, &cl_weaponpref_bfg, &cl_weaponpref_csw,
	&cl_weaponpref_ssg };

//
// D_PrepareWeaponPreferenceUserInfo
//
// Copies the weapon order preferences from the cl_weaponpref_* cvars
// to the userinfo struct for the consoleplayer.
//
void D_PrepareWeaponPreferenceUserInfo()
{
	byte *prefs = consoleplayer().userinfo.weapon_prefs;

	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		// sanitize the weapon preferences
		if (weaponpref_cvar_map[i]->asInt() < 0)
			weaponpref_cvar_map[i]->ForceSet(0.0f);
		if (weaponpref_cvar_map[i]->asInt() >= NUMWEAPONS)
			weaponpref_cvar_map[i]->ForceSet(float(NUMWEAPONS - 1));

		prefs[i] = weaponpref_cvar_map[i]->asInt();
	}
}

void D_SetupUserInfo(void)
{
	UserInfo* coninfo = &consoleplayer().userinfo;

	std::string netname(cl_name.str());
	if (netname.length() > MAXPLAYERNAME)
		netname.erase(MAXPLAYERNAME);

	coninfo->netname			= netname;
	coninfo->team				= D_TeamByName (cl_team.cstring()); // [Toke - Teams]
	coninfo->color				= V_GetColorFromString (NULL, cl_color.cstring());
	coninfo->skin				= R_FindSkin (cl_skin.cstring());
	coninfo->gender				= D_GenderByName (cl_gender.cstring());
	coninfo->aimdist			= (fixed_t)(cl_autoaim * 16384.0);
	coninfo->unlag				= (cl_unlag != 0);
	coninfo->update_rate		= cl_updaterate;
	coninfo->predict_weapons	= (cl_predictweapons != 0);

	// sanitize the weapon switching choice
	if (cl_switchweapon < 0 || cl_switchweapon > 2)
		cl_switchweapon.ForceSet(float(WPSW_ALWAYS));
	coninfo->switchweapon	= (weaponswitch_t)cl_switchweapon.asInt();

	// Copies the updated cl_weaponpref* cvars to coninfo->weapon_prefs[]
	D_PrepareWeaponPreferenceUserInfo();
}

void D_UserInfoChanged (cvar_t *cvar)
{
	if (gamestate != GS_STARTUP && !connected)
		D_SetupUserInfo();

	if (connected)
		CL_SendUserInfo();
}

FArchive &operator<< (FArchive &arc, UserInfo &info)
{
	arc.Write(info.netname.c_str(), MAXPLAYERNAME);
	arc << byte(0);		// ensure the string is properly terminated

	arc.Write(&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Write(&info.gender, sizeof(info.gender));
	arc << info.aimdist << info.color << info.skin << info.unlag
		<< info.update_rate;
	arc.Write(&info.switchweapon, sizeof(info.switchweapon));
	arc.Write(info.weapon_prefs, sizeof(info.weapon_prefs));
 	arc << 0;

	return arc;
}

FArchive &operator>> (FArchive &arc, UserInfo &info)
{
	int dummy;

	char netname[MAXPLAYERNAME+1];
	arc.Read(netname, sizeof(netname));
	info.netname = netname;

	arc.Read(&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Read(&info.gender, sizeof(info.gender));
	arc >> info.aimdist >> info.color >> info.skin >> info.unlag
		>> info.update_rate;
	arc.Read(&info.switchweapon, sizeof(info.switchweapon));
	arc.Read(info.weapon_prefs, sizeof(info.weapon_prefs));
	arc >> dummy;

	return arc;
}

VERSION_CONTROL (d_netinfo_cpp, "$Id$")

