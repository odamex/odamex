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

#include <algorithm>
#include <sstream>

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "g_level.h"
#include "m_fileio.h"
#include "sv_maplist.h"
#include "sv_vote.h"
#include "w_wad.h"

// Remove me if you figure out how to remove unnatural_map_progression
#include "sv_main.h"

namespace sv {

// Return a singleton reference for the class
Maplist& Maplist::instance() {
	static Maplist singleton;
	return singleton;
}

// Clear the maplist
void Maplist::clear() {
	maplist.clear();
	nextmap_index = 0;
	sv::Voting::instance().event_clearmaplist();
}

// Shuffle the order of the maplist
void Maplist::random_shuffle() {
	std::random_shuffle(maplist.begin(), maplist.end());
	nextmap_index = 0;
}

// Takes a vector of strings and returns a sequence of maplist indexes that
// match all of the passed strings.
std::vector<size_t> Maplist::expand(const std::vector<std::string> &arguments) {
	std::vector<size_t> maps;

	// If the maplist is empty we have nothing to look through.
	if (maplist.size() == 0) {
		return maps;
	}

	// In the case of a single argument, we need to check to see if that argument
	// is actually a maplist number.  Note that maplist numbers are the actual index
	// of the map + 1, since we want the user-facing maplist to be 1-indexed.
	if (arguments.size() == 1) {
		size_t map_index;
		std::istringstream buffer(arguments[0]);
		buffer >> map_index;

		// If the stream evaluates to true, the conversion was successful.
		if (buffer) {
			// Check to see if we've got a valid map index.
			if (map_index > 0 && map_index <= maplist.size()) {
				maps.push_back(map_index - 1);
			}

			// If we have a valid map index, we return that.  Otherwise, we
			// return an empty mapset.
			return maps;
		}
	}

	for (std::vector<std::string>::const_iterator it = arguments.begin();it != arguments.end();++it) {
		std::string pattern = "*" + (*it) + "*";
		if (it == arguments.begin()) {
			// Check the entire maplist for a match
			for (std::vector<maplist_entry_t>::size_type i = 0;
				 i < maplist.size();i++) {
				bool f_map = CheckWildcards(
					pattern.c_str(), maplist[i].map.c_str());
				bool f_wad = CheckWildcards(
					pattern.c_str(), maplist[i].wads.c_str());
				if (f_map || f_wad) {
					maps.push_back(i);
				}
			}
		} else {
			// Discard any map that doesn't match
			std::vector<std::vector<maplist_entry_t>::size_type>::iterator itr;
			for (itr = maps.begin();itr != maps.end();) {
				bool f_map = CheckWildcards(
					pattern.c_str(), maplist[*itr].map.c_str());
				bool f_wad = CheckWildcards(
					pattern.c_str(), maplist[*itr].wads.c_str());
				if (f_map || f_wad) {
					++itr;
				} else {
					itr = maps.erase(itr);
				}
			}
		}

		if (maps.size() == 0){
			// No indexes?  We're done.
			return maps;
		}
	}

	// Return the remaining list of indexes.
	return maps;
}

/**
 * Checks passed arguments for switching to a specific spot on the maplist.
 *
 * @param arguments any arguments to expand.
 * @param response If the function has a message to pass back, this variable will contain it on return.
 * @param index If the function finds a single map, this variable will contain it on return.
 * @return true if the function completed successfully, otherwise false.
 */
bool Maplist::gotomap_check(const std::vector<std::string> &arguments,
							std::vector<std::string> &response, size_t &index) {
	// Does the maplist even exist?
	if (maplist.size() == 0) {
		response.push_back("Maplist is empty.");
		return false;
	}

	// maplist::expand needs to resolve to exactly one map.
	std::vector<size_t> map = expand(arguments);
	if (map.size() == 0) {
		response.push_back("Map or maplist position does not exist.");
		return false;
	} else if (map.size() > 1) {
		response.push_back("Map is ambiguous.");
		return false;
	}

	index = map[0];

	return true;
}

// Switch to a specifi index in the maplist
void Maplist::gotomap(const size_t &index) {
	nextmap_index = index;
	G_ChangeMap();
}

// Return a textual representation of the maplist as a vector of strings.
void Maplist::print_maplist(const std::vector<std::string> &arguments,
							std::vector<std::string> &response) {
	// Does the maplist even exist?
	if (maplist.size() == 0) {
		response.push_back("Maplist is empty.");
		return;
	}

	// If we passed any arguments, use them to truncate the list of maps
	std::vector<std::vector<maplist_entry_t>::size_type> subset;
	if (arguments.size() > 0) {
		subset = expand(arguments);

		if (subset.size() == 0) {
			response.push_back("No maps found.");
			return;
		}
	}

	response.push_back(" MAPLIST:");

	// Iterate through the maplist and add each entry to the response.
	for (size_t i = 0;
		 i < maplist.size();i++) {

		if (subset.size() != 0) {
			if (!std::binary_search(subset.begin(), subset.end(), i)) {
				continue;
			}
		}

		std::ostringstream entry;

		if (i != maplist_index) {
			entry << "  ";
		} else {
			entry << "> ";
		}

		entry << (i + 1) << ". ";

		if (maplist[i].wads.empty()) {
			entry << "-";
		} else {
			entry << maplist[i].wads;
		}

		entry << " " << maplist[i].map;
		response.push_back(entry.str());
	}
}

// Check to see it's okay to select a random map
bool Maplist::randmap_check(std::string &error) {
	if (maplist.empty()) {
		error = "Map list is empty.";
		return false;
	}

	return true;
}

// Switch to a random map
void Maplist::randmap() {
	size_t index = (size_t)std::rand() % maplist.size();
	gotomap(index);
}

}

// Adds a map and optional wad files to be loaded into the map list
BEGIN_COMMAND(addmap) {
	sv::maplist_entry_t maplist_entry;

	if (argc < 2) {
		Printf(PRINT_HIGH, "Usage: addmap mapname [wads]\n");
		return;
	}

	if (sv::Maplist::instance().maplist.size() == -1) {
		Printf(PRINT_HIGH, "Maplist is full.\n");
		return;
	}

	maplist_entry.map = argv[1];

	if (argc > 2) {
		// Add any wads if they were specified
		for (unsigned int i = 2;i < argc;++i) {
			maplist_entry.wads += argv[i];
			maplist_entry.wads += " ";
		}
	} else if (sv::Maplist::instance().maplist.size()) {
		// Or.. check if there are maps already in the list, if so,
		// get the last one and copy them for this one
		maplist_entry.wads = sv::Maplist::instance().maplist.back().wads;
	} else {
		// Otherwise use the list of loaded wads instead (since the
		// wad ccmd can change the wads at runtime, we want to begin
		// consistent with the maplist)
		std::string Filename;

		// i = 1, so we bypass loading odamex.wad (or displaying it
		// with the maplist ccmd)
		for (size_t i = 1; i < wadfiles.size(); ++i) {
			M_ExtractFileName(wadfiles[i], Filename);

			maplist_entry.wads += Filename;
			maplist_entry.wads += " ";
		}

		// Nix the extra space at the end
		if (maplist_entry.wads.size() > 0) {
			maplist_entry.wads.resize(maplist_entry.wads.size() - 1);
		}
	}

	// Is our next map back at the beginning of the list?
	// If so, we must be at the end, so we should change the
	// next map to the map entry we're about to add.
	if (sv::Maplist::instance().nextmap_index == 0 &&
		sv::Maplist::instance().maplist_index != -1) {
		sv::Maplist::instance().nextmap_index =
			sv::Maplist::instance().maplist.size();
	}

	// Push the new entry on to the end of the vector
	sv::Maplist::instance().maplist.push_back(maplist_entry);
} END_COMMAND(addmap)

// Clear all maps in the map list
BEGIN_COMMAND(clearmaplist) {
	if (sv::Maplist::instance().maplist.empty()) {
		Printf(PRINT_HIGH, "Map list is already empty.\n");
		return;
	}

	sv::Maplist::instance().maplist.clear();
	sv::Maplist::instance().maplist_index = -1;
	sv::Maplist::instance().nextmap_index = 0;
} END_COMMAND(clearmaplist)

// Forcefully go to next map without intermission screen
BEGIN_COMMAND(forcenextmap) {
	if (sv::Maplist::instance().maplist.empty()) {
		Printf(PRINT_HIGH, "Map list is empty.\n" );
		return;
	}

	G_ChangeMap();
} END_COMMAND(forcenextmap)

// Switch to a specific map in the maplist
BEGIN_COMMAND(gotomap) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::vector<std::string> response;
	size_t index;

	// Run the check
	bool ok = sv::Maplist::instance().gotomap_check(arguments, response, index);

	if (!ok) {
		// Called by the server, so print the response to the server console.
		for (std::vector<std::string>::size_type i = 0;i < response.size();i++) {
			Printf(PRINT_HIGH, (response[i] + "\n").c_str());
		}
	} else {
		// If the check completed successfully, run the command.
		sv::Maplist::instance().gotomap(index);
	}
} END_COMMAND(gotomap)

// Serverside maplist command
BEGIN_COMMAND(maplist) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::vector<std::string> response;

	sv::Maplist::instance().print_maplist(arguments, response);

	// Called by the server, so print the response to the server console.
	for (std::vector<std::string>::size_type i = 0;
		 i < response.size();i++) {
		Printf(PRINT_HIGH, (response[i] + "\n").c_str());
	}
} END_COMMAND(maplist)

// Jump to next map in the list, with intermission
BEGIN_COMMAND(nextmap) {
	if (sv::Maplist::instance().maplist.empty()) {
		Printf(PRINT_HIGH, "Map list is empty.\n" );
		return;
	}

	G_ExitLevel(0, 1);
} END_COMMAND(nextmap)

// Pick a random map in the maplist
BEGIN_COMMAND(randmap) {
	std::string error;
	bool valid = sv::Maplist::instance().randmap_check(error);

	if (!valid) {
		Printf(PRINT_HIGH, "%s\n", error.c_str());
		return;
	}

	sv::Maplist::instance().randmap();
} END_COMMAND(randmap)

// CVAR function for shuffling the maplist
// TODO: This is a naive implementation, which doesn't return the maplist to
//       its original state when set to 0.
CVAR_FUNC_IMPL(sv_shufflemaplist) {
	sv::Maplist::instance().random_shuffle();
}
