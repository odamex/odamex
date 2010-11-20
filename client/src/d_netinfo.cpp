// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2010 by The Odamex Team.
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

enum
{
	INFO_Name,
	INFO_Autoaim,
	INFO_Color,
	INFO_Skin,
	INFO_Team,
	INFO_Gender
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

void D_UserInfoChanged (cvar_t *cvar)
{
//	if (gamestate != GS_STARTUP)
//		D_SetupUserInfo();

	if (connected)
		CL_SendUserInfo();
}

FArchive &operator<< (FArchive &arc, userinfo_t &info)
{
	arc.Write (&info.netname, sizeof(info.netname));
	arc.Write (&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Write (&info.gender, sizeof(info.gender));
	arc << info.aimdist << info.color << info.skin << 0;
	return arc;
}

FArchive &operator>> (FArchive &arc, userinfo_t &info)
{
	arc.Read (&info.netname, sizeof(info.netname));
	arc.Read (&info.team, sizeof(info.team));  // [Toke - Teams]
	arc.Read (&info.gender, sizeof(info.gender));
	int neverswitch;
	arc >> info.aimdist >> info.color >> info.skin >> neverswitch;
	return arc;
}

VERSION_CONTROL (d_netinfo_cpp, "$Id$")

