// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: 
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//   Episode data for defining new episodes.
// 
//-----------------------------------------------------------------------------

#ifndef __G_EPISODE__
#define __G_EPISODE__

#include "doomdef.h"

#define MAX_EPISODES 8

struct EpisodeInfo
{
	std::string name;
	char key;
	bool fulltext;
	bool noskillmenu;
	bool optional;
	bool extended;
};

extern char EpisodeMaps[MAX_EPISODES][8];
extern EpisodeInfo EpisodeInfos[MAX_EPISODES];
extern byte episodenum;

#endif