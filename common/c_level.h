// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Common level routines
//
//-----------------------------------------------------------------------------

#ifndef __C_LEVEL_H__
#define __C_LEVEL_H__

#include <vector>
#include "g_level.h"

extern bool unnatural_level_progression;

void P_RemoveDefereds (void);
int FindWadLevelInfo (char *name);
int FindWadClusterInfo (int cluster);

level_info_t *FindDefLevelInfo (char *mapname);
cluster_info_t *FindDefClusterInfo (int cluster);

bool G_LoadWad(	const std::vector<std::string> &newwadfiles,
				const std::vector<std::string> &newpatchfiles,
				const std::vector<std::string> &newwadhashes = std::vector<std::string>(),
				const std::vector<std::string> &newpatchhashes = std::vector<std::string>(),
				const std::string &mapname = "");

bool G_LoadWad(const std::string &str, const std::string &mapname = "");

#endif // __C_LEVEL_H__
