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
//	D_NETINFO
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include "cmdlib.h"
#include "d_netinf.h"
#include "v_video.h"
#include "r_draw.h"
#include "r_state.h"
#include "cl_main.h"

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
EXTERN_CVAR (cl_team)
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
	INFO_Team,
	INFO_Gender,
    INFO_Unlag
};


CVAR_FUNC_IMPL(cl_name)
{
	std::string newname(var.str());
	StripColorCodes(newname);

	if (var.str().compare(newname) != 0)
		var.Set(newname.c_str());
}



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
	for (int i = 0; i < NUMTEAMS; i++)
	{
		TeamInfo* teamInfo = GetTeamInfo((team_t)i);
		if (stricmp(team, teamInfo->ColorString.c_str()) == 0)
			return (team_t)i;
	}

	if (strcmp(team, "0") == 0)
		return TEAM_BLUE;
	else if (strcmp(team, "1") == 0)
		return TEAM_RED;
	else if (strcmp(team, "2") == 0)
		return TEAM_GREEN;

	return TEAM_NONE;
}

colorpreset_t D_ColorPreset (const char *colorpreset)
{
	if (!stricmp(colorpreset, "blue"))
		return COLOR_BLUE;
	else if (!stricmp(colorpreset, "indigo"))
		return COLOR_INDIGO;
	else if (!stricmp(colorpreset, "green"))
		return COLOR_GREEN;
	else if (!stricmp(colorpreset, "brown"))
		return COLOR_BROWN;
	else if (!stricmp(colorpreset, "red"))
		return COLOR_RED;
	else if (!stricmp(colorpreset, "gold"))
		return COLOR_GOLD;
	else if (!stricmp(colorpreset, "jungle green"))
		return COLOR_JUNGLEGREEN;
	else if (!stricmp(colorpreset, "purple"))
		return COLOR_PURPLE;
	else if (!stricmp(colorpreset, "white"))
		return COLOR_WHITE;
	else if (!stricmp(colorpreset, "black"))
		return COLOR_BLACK;
	else
		return COLOR_CUSTOM;
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
	StripColorCodes(netname);
	
	if (netname.length() > MAXPLAYERNAME)
		netname.erase(MAXPLAYERNAME);

	team_t newteam = D_TeamByName(cl_team.cstring());
	if (newteam == TEAM_NONE){
		cl_team.RestoreDefault();
		newteam = TEAM_BLUE;
	}


	coninfo->netname			= netname;
	coninfo->team				= newteam; // [Toke - Teams]
	coninfo->gender				= D_GenderByName (cl_gender.cstring());
	coninfo->aimdist			= (fixed_t)(cl_autoaim * 16384.0);
	coninfo->predict_weapons	= (cl_predictweapons != 0);

	// sanitize the weapon switching choice
	if (cl_switchweapon >= WPSW_NUMTYPES || cl_switchweapon < 0)
		cl_switchweapon.ForceSet(WPSW_ALWAYS);
	coninfo->switchweapon	= (weaponswitch_t)cl_switchweapon.asInt();

	// Copies the updated cl_weaponpref* cvars to coninfo->weapon_prefs[]
	D_PrepareWeaponPreferenceUserInfo();

	// set the color in a pixel-format neutral way
	argb_t color = V_GetColorFromString(cl_color);
	coninfo->color[0] = color.geta();
	coninfo->color[1] = color.getr();
	coninfo->color[2] = color.getg(); 
	coninfo->color[3] = color.getb(); 

	// update color translation
	if (!demoplayback && !connected)
		R_BuildPlayerTranslation(consoleplayer_id, color);
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
	char netname[MAXPLAYERNAME + 1];
	memset(netname, 0, MAXPLAYERNAME + 1);
	strncpy(netname, info.netname.c_str(), MAXPLAYERNAME);
	arc.Write(netname, MAXPLAYERNAME + 1);

	arc.Write(&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Write(&info.gender, sizeof(info.gender));

	arc << info.aimdist;
	arc << info.color[0] << info.color[1] << info.color[2] << info.color[3];

	// [SL] use place-holders for deprecated client options
	// so old save games and netdemos continue to function
	unsigned int skin = 0;
	bool unlag = true;
	byte update_rate = 0;
	arc << skin;
	arc << unlag;
	arc << update_rate;

	arc.Write(&info.switchweapon, sizeof(info.switchweapon));
	arc.Write(info.weapon_prefs, sizeof(info.weapon_prefs));

	int terminator = 0;
 	arc << terminator;

	return arc;
}

FArchive &operator>> (FArchive &arc, UserInfo &info)
{
	char netname[MAXPLAYERNAME + 1];
	arc.Read(netname, MAXPLAYERNAME + 1);
	info.netname = netname;

	arc.Read(&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Read(&info.gender, sizeof(info.gender));

	arc >> info.aimdist;
	arc >> info.color[0] >> info.color[1] >> info.color[2] >> info.color[3];

	// [SL] use place-holders for deprecated client options
	// so old save games and netdemos continue to function.
	unsigned int skin;
	bool unlag;
	byte update_rate;
	arc >> skin;
	arc >> unlag;
	arc >> update_rate;

	arc.Read(&info.switchweapon, sizeof(info.switchweapon));
	arc.Read(info.weapon_prefs, sizeof(info.weapon_prefs));

	int terminator;
	arc >> terminator;	// 0

	return arc;
}

VERSION_CONTROL (d_netinfo_cpp, "$Id$")
