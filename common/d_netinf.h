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
//	Multiplayer properties (?)
//
//-----------------------------------------------------------------------------


#ifndef __D_NETINFO_H__
#define __D_NETINFO_H__

#include "c_cvars.h"

#define MAXPLAYERNAME	15

enum gender_t
{
	GENDER_MALE,
	GENDER_FEMALE,
	GENDER_NEUTER,
	
	NUMGENDER
};

// [Toke - Teams]
// denis - for both teamplay and ctfmode
enum team_t
{
	TEAM_BLUE,
	TEAM_RED,
	
	NUMTEAMS,
	
	TEAM_NONE
};

struct userinfo_s
{
	char			netname[MAXPLAYERNAME+1];
	team_t			team; // [Toke - Teams] 
	fixed_t			aimdist;
	bool			unlag;
	int				color;
	unsigned int	skin;
	gender_t		gender;

	userinfo_s() : team(TEAM_NONE), aimdist(0), unlag(true), color(0), skin(0), gender(GENDER_MALE) { *netname = 0; }
};
typedef userinfo_s userinfo_t;

FArchive &operator<< (FArchive &arc, userinfo_t &info);
FArchive &operator>> (FArchive &arc, userinfo_t &info);

void D_SetupUserInfo (void);

void D_UserInfoChanged (cvar_t *info);

void D_SendServerInfoChange (const cvar_t *cvar, const char *value);
void D_DoServerInfoChange (byte **stream);

void D_WriteUserInfoStrings (int player, byte **stream, bool compact=false);
void D_ReadUserInfoStrings (int player, byte **stream, bool update);

#endif //__D_NETINFO_H__


