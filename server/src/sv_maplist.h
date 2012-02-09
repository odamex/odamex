// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Maplist-related functionality.
//
//-----------------------------------------------------------------------------

#ifndef __SV_MAPLIST__
#define __SV_MAPLIST__

#include <string>
#include <vector>

namespace sv {

// Map list entry structure
typedef struct {
	std::string map;
	std::string wads;
} maplist_entry_t;

class Maplist {
public:
	// The map list itself
	std::vector<maplist_entry_t> maplist;
	// The current map in the rotation.  Setting this to -1 means that the very
	// first time we load up odamex the "pointer" in the maplist console
	// command won't appear unless you have size_t maps in the maplist.
	size_t maplist_index;
	// The next map in the rotation, incrementally.
	size_t nextmap_index;

	static Maplist& instance(void);
	void clear(void);
	void random_shuffle(void);
	bool gotomap_check(const std::vector<std::string> &arguments,
					   std::vector<std::string> &response, size_t &index);
	void gotomap(const size_t &index);
	void print_maplist(const std::vector<std::string> &arguments,
				 std::vector<std::string> &response);
	bool randmap_check(std::string &error);
	void randmap(void);
private:
	Maplist() : maplist_index(-1), nextmap_index(0) { };

	std::vector<size_t> expand(const std::vector<std::string> &arguments);
};

}

#endif
