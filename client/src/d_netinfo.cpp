// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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


EXTERN_CVAR (cl_autoaim)
EXTERN_CVAR (cl_name)
EXTERN_CVAR (cl_color)
EXTERN_CVAR (cl_gender)
EXTERN_CVAR (cl_skin)
EXTERN_CVAR (cl_team)
EXTERN_CVAR (cl_unlag)
EXTERN_CVAR (cl_updaterate)
EXTERN_CVAR (cl_switchweapon)
EXTERN_CVAR (cl_weaponpref1)
EXTERN_CVAR (cl_weaponpref2)
EXTERN_CVAR (cl_weaponpref3)
EXTERN_CVAR (cl_weaponpref4)
EXTERN_CVAR (cl_weaponpref5)
EXTERN_CVAR (cl_weaponpref6)
EXTERN_CVAR (cl_weaponpref7)
EXTERN_CVAR (cl_weaponpref8)
EXTERN_CVAR (cl_weaponpref9)

void CL_RefreshWeaponPreferenceCvars();
void CL_PrepareWeaponPreferenceUserInfo();
void CL_SetWeaponPreferenceCvar(int slot, weapontype_t weapon);

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

void D_SetupUserInfo(void)
{
	userinfo_t *coninfo = &consoleplayer().userinfo;

	// Save the previous weapon preferences
	weapontype_t backup_weapon_prefs[NUMWEAPONS];
	memcpy(backup_weapon_prefs, coninfo->weapon_prefs, sizeof(backup_weapon_prefs));

	memset (&consoleplayer().userinfo, 0, sizeof(userinfo_t));
	
	strncpy (coninfo->netname, cl_name.cstring(), MAXPLAYERNAME);
	coninfo->team			= D_TeamByName (cl_team.cstring()); // [Toke - Teams]
	coninfo->color			= V_GetColorFromString (NULL, cl_color.cstring());
	coninfo->skin			= R_FindSkin (cl_skin.cstring());
	coninfo->gender			= D_GenderByName (cl_gender.cstring());
	coninfo->aimdist		= (fixed_t)(cl_autoaim * 16384.0);
	coninfo->unlag			= (cl_unlag != 0);
	coninfo->update_rate	= cl_updaterate;
	coninfo->switchweapon	= (weaponswitch_t)cl_switchweapon.asInt();

	// Copies the updated cl_weaponpref* cvars to coninfo->weapon_prefs[]
	CL_PrepareWeaponPreferenceUserInfo();

	// Find which weapon preference slot was changed
	for (size_t i = 0; i < NUMWEAPONS; i++)
	{
		if (backup_weapon_prefs[i] != coninfo->weapon_prefs[i])
		{
			// slot i was changed
			weapontype_t oldweapon = backup_weapon_prefs[i];
			weapontype_t newweapon = coninfo->weapon_prefs[i];
			
			// swap the weapon in slot i with whatever slot already has newweapon
			for (size_t j = 0; j < NUMWEAPONS; j++)
			{
				if (coninfo->weapon_prefs[j] == newweapon &&  j != i)
				{
					coninfo->weapon_prefs[j] = oldweapon;
					CL_SetWeaponPreferenceCvar(j, oldweapon);
					break;
				}
			}

			break;
		}
	}
}

void D_UserInfoChanged (cvar_t *cvar)
{
	if (gamestate != GS_STARTUP && !connected)
		D_SetupUserInfo();

	if (connected)
		CL_SendUserInfo();
}

FArchive &operator<< (FArchive &arc, userinfo_t &info)
{
	arc.Write (&info.netname, sizeof(info.netname));
	arc.Write (&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Write (&info.gender, sizeof(info.gender));
	arc << info.aimdist << info.color << info.skin << info.unlag
		<< info.update_rate << (byte)info.switchweapon;
	arc.Write (&info.weapon_prefs, sizeof(info.weapon_prefs));
 	arc << 0;

	return arc;
}

FArchive &operator>> (FArchive &arc, userinfo_t &info)
{
	int dummy;
	byte switchweapon;

	arc.Read (&info.netname, sizeof(info.netname));
	arc.Read (&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Read (&info.gender, sizeof(info.gender));
	arc >> info.aimdist >> info.color >> info.skin >> info.unlag
		>> info.update_rate >> switchweapon;
	info.switchweapon = (weaponswitch_t)switchweapon;
	arc.Read (&info.weapon_prefs, sizeof(info.weapon_prefs));
	arc >> dummy;

	return arc;
}

VERSION_CONTROL (d_netinfo_cpp, "$Id$")

