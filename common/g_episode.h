// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:
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
//   Episode data for defining new episodes.
//
//-----------------------------------------------------------------------------

#pragma once

#define MAX_EPISODES 10

struct EpisodeInfo
{
	OLumpName pic_name;
	std::string menu_name;
	char key;
	bool fulltext;
	bool noskillmenu;

	EpisodeInfo() : pic_name(""), menu_name(""), key('\0'), fulltext(false), noskillmenu(false) {}
};

inline OLumpName EpisodeMaps[MAX_EPISODES];
inline EpisodeInfo EpisodeInfos[MAX_EPISODES];
inline byte episodenum;
inline bool episodes_modified; // Used by UMAPINFO only
