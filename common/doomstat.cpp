// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2014 by The Odamex Team.
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


#include <stdio.h>
#include <stdlib.h>
#include "stringtable.h"
#include "stringenums.h"

#include "doomstat.h"
#include "c_cvars.h"
#include "i_system.h"
#include "p_acs.h"
#include "res_main.h"


// Localizable strings
FStringTable	GStrings;

// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t		gamemode = undetermined;
GameMission_t	gamemission = doom;

// Language.
CVAR_FUNC_IMPL(language)
{
	SetLanguageIDs ();
	if (level.behavior != NULL)
		level.behavior->PrepLocale(LanguageIDs[0], LanguageIDs[1], LanguageIDs[2], LanguageIDs[3]);

	ResourceId language_res_id = Res_GetResourceId("LANGUAGE");
	size_t language_length = Res_GetLumpLength(language_res_id);
	byte* language_data = new byte[language_length];
	Res_ReadLump(language_res_id, language_data);

	GStrings.LoadStrings(language_data, language_length, STRING_TABLE_SIZE, false);
	GStrings.Compact();

	delete [] language_data;

	G_SetLevelStrings();
}

// Set if homebrew PWAD stuff has been added.
BOOL			modifiedgame;

VERSION_CONTROL (doomstat_cpp, "$Id$")

