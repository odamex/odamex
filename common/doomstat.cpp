// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2020 by The Odamex Team.
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
//      Put all global state variables here.
//
//-----------------------------------------------------------------------------


#include "stringtable.h"
#include "doomstat.h"
#include "c_cvars.h"
#include "i_system.h"
#include "p_acs.h"

// Localizable strings
FStringTable	GStrings;

// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t		gamemode = undetermined;
GameMission_t	gamemission = doom;

static const char* TeamNames[NUMTEAMS] = { "BLUE", "RED", "GREEN" };

// Language.
CVAR_FUNC_IMPL (language)
{
	SetLanguageIDs ();
	if (level.behavior != NULL)
	{
		level.behavior->PrepLocale (LanguageIDs[0], LanguageIDs[1],
			LanguageIDs[2], LanguageIDs[3]);
	}
    
	GStrings.ResetStrings ();
	GStrings.Compact ();
	G_SetLevelStrings ();
}

// Set if homebrew PWAD stuff has been added.
BOOL			modifiedgame;

bool IsGameModeDuel()
{
	return sv_gametype == GM_DM && sv_maxplayers == 2;
}

bool IsGameModeFFA()
{
	return sv_gametype == GM_DM && sv_maxplayers > 2;
}

const char* GetGameModeString()
{
	if (sv_gametype == GM_COOP)
		return "COOPERATIVE";
	else if (IsGameModeDuel())
		return "DUEL";
	else if (sv_gametype == GM_DM)
		return "DEATHMATCH";
	else if (sv_gametype == GM_TEAMDM)
		return "TEAM DEATHMATCH";
	else if (sv_gametype == GM_CTF)
		return "CAPTURE THE FLAG";

	return "";
}

const char* GetShortGameModeString()
{
	if (sv_gametype == GM_COOP)
		return multiplayer ? "COOP" : "SOLO";
	else if (IsGameModeDuel())
		return "DUEL";
	else if (sv_gametype == GM_DM)
		return "DM";
	else if (sv_gametype == GM_TEAMDM)
		return "TDM";
	else if (sv_gametype == GM_CTF)
		return "CTF";

	return "";
}

#ifdef CLIENT_APP
EColorRange GetTeamTextColor(team_t team)
{
	static const EColorRange TeamColors[NUMTEAMS] = { CR_BLUE, CR_RED, CR_GREEN };
	if (team < NUMTEAMS)
		return TeamColors[team];

	return CR_GREY;
}
#endif

argb_t GetTeamColor(team_t team)
{
	switch (team)
	{
	case TEAM_BLUE:
		return argb_t(255, 0, 0, 255);
	case TEAM_RED:
		return argb_t(255, 255, 0, 0);
	default:
		return argb_t(255, 0, 255, 0);
	}
}

const char* GetTeamName(team_t team)
{
	static const char* TeamNames[NUMTEAMS] = { "BLUE TEAM", "RED TEAM", "GREEN TEAM" };

	if (team < NUMTEAMS)
		return TeamNames[team];

	return "NO TEAM";
}

const char* GetTeamColorString(team_t team)
{
	if (team < NUMTEAMS)
		return TeamNames[team];

	return "NONE";
}

const char* GetTeamColorStringCase(team_t team)
{
	static const char* TeamNamesCase[NUMTEAMS] = { "Blue", "Red", "Green" };

	if (team < NUMTEAMS)
		return TeamNamesCase[team];

	return "";
}

const char** GetTeamColorStrings()
{
	return TeamNames;
}

VERSION_CONTROL (doomstat_cpp, "$Id$")

