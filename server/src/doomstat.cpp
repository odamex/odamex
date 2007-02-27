// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Put all global state variables here.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "c_cvars.h"


// Game Mode - identify IWAD as shareware, retail etc.
GameMode_t		gamemode = undetermined;
GameMission_t	gamemission = doom;

// Language.
Language_t		language = english;

// Set if homebrew PWAD stuff has been added.
BOOL			modifiedgame;

// Show developer messages if true.
CVAR (developer, "0", 0)

// [RH] Feature control cvars
CVAR (var_friction, "1", CVAR_NULL) // removeme 
CVAR (var_pushers, "1", CVAR_NULL) // removeme 


VERSION_CONTROL (doomstat_cpp, "$Id$")

