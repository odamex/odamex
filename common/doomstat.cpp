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


#include "gstrings.h"
#include "doomstat.h"
#include "c_cvars.h"
#include "i_system.h"
#include "p_acs.h"
#include "d_dehacked.h"

// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t		gamemode = undetermined;
GameMission_t	gamemission = doom;

// Language.
CVAR_FUNC_IMPL (language)
{
	SetLanguageIDs();
	if (level.behavior != NULL)
	{
		level.behavior->PrepLocale (LanguageIDs[0], LanguageIDs[1],
			LanguageIDs[2], LanguageIDs[3]);
	}

	// Reload LANGUAGE strings.
	GStrings.loadStrings();

	// Reapply DeHackEd patches on top of these strings.
	// FIXME: This only handles PWAD patches for now.
	DoDehPatch(NULL, true);

	// Set default level strings based on those DeHackEd patches.
	G_SetLevelStrings();

	// MAPINFO comes last, because it overrides default level strings.
	G_ParseMapInfo();
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

VERSION_CONTROL (doomstat_cpp, "$Id$")
