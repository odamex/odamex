// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
// Team definitions
//
//-----------------------------------------------------------------------------

#pragma once

enum team_t
{
	TEAM_BLUE,
	TEAM_RED,
	TEAM_GREEN,

	NUMTEAMS,

	TEAM_NONE
};

// flags can only be in one of these states
enum flag_state_t
{
	flag_home,
	flag_dropped,
	flag_carried,

	NUMFLAGSTATES
};
