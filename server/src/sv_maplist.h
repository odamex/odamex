// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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
//  Serverside maplist-related functionality.
//
//-----------------------------------------------------------------------------

#ifndef __SV_MAPLIST__
#define __SV_MAPLIST__

#include <c_maplist.h>
#include <d_player.h>

#include <map>
#include <string>
#include <vector>

// Serverside maplist structure
class Maplist {
private:
	bool entered_once;
	std::string error;
	size_t index;
	bool in_maplist;
	std::vector<maplist_entry_t> maplist;
	bool shuffled;
	size_t s_index;
	std::vector<size_t> s_maplist;
	std::map<int, QWORD> timeout;
	byte version;
	void shuffle(void);
	void update_shuffle_index(void);
public:
	Maplist() : entered_once(false), error(""), index(0),
				in_maplist(false), shuffled(false), s_index(0), version(0) { };
	static Maplist& instance(void);
	// Modifiers
	bool add(maplist_entry_t &maplist_entry);
	bool insert(const size_t &position, maplist_entry_t &maplist_entry);
	bool remove(const size_t &position);
	bool clear(void);
	// Elements
	bool empty(void);
	std::string get_error(void);
	bool get_map_by_index(const size_t &index, maplist_entry_t &maplist_entry);
	bool get_next_index(size_t &index);
	bool get_this_index(size_t &index);
	byte get_version(void);
	bool query(std::vector<std::pair<size_t, maplist_entry_t*> > &result);
	bool query(const std::vector<std::string> &query,
			   std::vector<std::pair<size_t, maplist_entry_t*> > &result);
	// Settings
	bool set_index(const size_t &index);
	void set_shuffle(const bool setting);
	// Timeout
	bool pid_timeout(const int index);
	bool pid_cached(const int index);
	void set_timeout(const int index);
	void clear_timeout(const int index);
};

void SV_Maplist(player_t &player);
void SV_MaplistUpdate(player_t &player);

void Maplist_Disconnect(player_t &player);

bool CMD_Randmap(std::string &error);

#endif
