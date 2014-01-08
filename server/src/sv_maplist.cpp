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

#include <algorithm>
#include <sstream>

#include "c_maplist.h"
#include "sv_maplist.h"

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "g_level.h"
#include "i_system.h"
#include "m_fileio.h"
#include "sv_main.h"
#include "sv_vote.h"
#include "w_wad.h"

//////// MAPLIST METHODS ////////

// Shuffle the maplist.
void Maplist::shuffle() {
	// There's no maplist to shuffle.  Save ourselves some CPU time.
	if (this->maplist.empty()) {
		return;
	}

	// Create a shuffled list of indexes
	this->s_maplist.clear();
	for (size_t i = 0;i < maplist.size();++i) {
		this->s_maplist.push_back(i);
	}
	std::random_shuffle(this->s_maplist.begin(), this->s_maplist.end());

	// Update s_index based on our newly-shuffled maplist.
	this->update_shuffle_index();
}

// Update s_index based on the current map index
void Maplist::update_shuffle_index() {
	// There's no shuffled maplist.  Save ourselves some CPU time.
	if (!this->shuffled) {
		return;
	}

	// Find our current maplist position in the shuffled maplist
	// and update the shuffled index.
	for (size_t i = 0;i < this->s_maplist.size();++i) {
		if (this->index == this->s_maplist[i]) {
			this->s_index = i;
			break;
		}
	}
}

// Return a singleton reference for the class.
Maplist& Maplist::instance() {
	static Maplist singleton;
	return singleton;
}

// Adds a map to the end of the maplist.
bool Maplist::add(maplist_entry_t &maplist_entry) {
	return this->insert(this->maplist.size(), maplist_entry);
}

// Inserts a map into the maplist at the specified position.
bool Maplist::insert(const size_t &position, maplist_entry_t &maplist_entry) {
	// We send the maplist to clients using a short int, so we don't want
	// more maps in the list than the short int can handle.
	if (this->maplist.size() > (unsigned short)-1) {
		this->error = "Maplist is full.";
		return false;
	}

	// Desired position is outside of the maplist
	if (position > this->maplist.size()) {
		std::ostringstream buffer;
		buffer << "Index " << position + 1 << " out of range.";
		this->error = buffer.str();
		return false;
	}

	// If we have not been passed custom WAD files,
	// we need to fill them in on our own.
	if (maplist_entry.wads.empty()) {
		if (position == 0) {
			// Nothing is 'above us' to yoink from, so just use the
			// currently loaded WAD files.  Add one to the beginning
			// of wadfiles, since position 0 stores odamex.wad.
			maplist_entry.wads.resize(wadfiles.size() - 1);
			std::copy(wadfiles.begin() + 1, wadfiles.end(),
					  maplist_entry.wads.begin());
		} else {
			maplist_entry.wads.resize(this->maplist[position - 1].wads.size());
			std::copy(this->maplist[position - 1].wads.begin(),
					  this->maplist[position - 1].wads.end(),
					  maplist_entry.wads.begin());
		}
	}

	// Puts the map into its proper place
	this->maplist.insert(this->maplist.begin() + position, maplist_entry);

	// If we haven't actually entered the map rotation yet, don't increase
	// indexes because this results in the initial map switch going to the end
	// of the maplist, which probably isn't what we want.
	if (!this->entered_once) {
		// If either of our indexes are after the inserted map, fix them
		if (this->index >= position) {
			this->index += 1;
		}
	}

	// Regenerate the 'shuffled maplist'
	if (this->shuffled) {
		this->shuffle();
	}

	// New version of the maplist.
	this->version += 1;

	// Clear the timeouts, since we've modified the maplist.
	this->timeout.clear();
	return true;
}

// Removes a map from the maplist at the specified position.
bool Maplist::remove(const size_t &index) {
	// There's no maplist to delete an entry from.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	// Desired index is outside of the maplist
	if (index >= this->maplist.size()) {
		std::ostringstream buffer;
		buffer << "Index " << index + 1 << " out of range.";
		this->error = buffer.str();
		return false;
	}

	// Removes the specified map
	this->maplist.erase(this->maplist.begin() + index);

	// If deleteing a map from the maplist leaves us with an
	// empty maplist we're done here.
	if (maplist.empty()) {
		this->in_maplist = false;
		this->index = 0;
		return true;
	}

	// Don't mess with indexes if we haven't actually entered the maplist.
	if (!this->entered_once) {
		// If we delete our current map, we do not have a determined
		// position in the maplist.
		if (this->index == index) {
			this->in_maplist = false;
		}
		// If we delete a map that is less than our current index, we need to
		// keep the index consistent.
		if (this->index >= index) {
			if (this->index == 0) {
				this->index = this->maplist.size() - 1;
			} else {
				this->index -= 1;
			}
		}
	}

	// Regenerate the 'shuffled maplist'.
	if (this->shuffled) {
		this->shuffle();
	}

	// New version of the maplist.
	this->version += 1;

	// Clear the timeouts, since we've modified the maplist.
	this->timeout.clear();

	return true;
}

// Clear the entire maplist.
bool Maplist::clear(void) {
	// There's no maplist to clear.
	if (this->maplist.empty()) {
		this->error = "Maplist is already empty.";
		return false;
	}

	this->in_maplist = false;
	this->index = 0;
	this->maplist.clear();
	this->version += 1;
	return true;
}

// Is the maplist empty?
bool Maplist::empty(void) {
	return this->maplist.empty();
}

// Error getter.
std::string Maplist::get_error() {
	return this->error;
}

// Return the map associated with the given index
bool Maplist::get_map_by_index(const size_t &index, maplist_entry_t &maplist_entry) {
	// There's no maplist to return a map for.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	// Desired index is outside of the maplist
	if (index >= this->maplist.size()) {
		std::ostringstream buffer;
		buffer << "Index " << index + 1 << " out of range.";
		this->error = buffer.str();
		return false;
	}

	maplist_entry = this->maplist[index];
	return true;
}

// Return the current map index.
bool Maplist::get_this_index(size_t &index) {
	// There's no maplist to return a 'current map' on.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	// We're not in the maplist
	if (!this->in_maplist) {
		this->error = "This map is not in the maplist.";
		return false;
	}

	index = this->index;
	return true;
}

// Return what the next map index ought to be based on the current index.
bool Maplist::get_next_index(size_t &index) {
	// There's no maplist to return a 'next map' on.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	if (this->shuffled) {
		if (this->s_index + 1 >= this->s_maplist.size() || !entered_once) {
			index = this->s_maplist[0];
		} else {
			index = this->s_maplist[this->s_index + 1];
		}
	} else {
		if (this->index + 1 >= this->maplist.size() || !entered_once) {
			index = 0;
		} else {
			index = this->index + 1;
		}
	}

	return true;
}

// Maplist version getter
byte Maplist::get_version() {
	return this->version;
}

// Run a query on the maplist without any actual query.  Used for grabbing
// the entire mapset at once.
bool Maplist::query(std::vector<std::pair<size_t, maplist_entry_t*> > &result) {
	// There's no maplist to query.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	// Return everything
	result.reserve(this->maplist.size());
	for (size_t i = 0;i < maplist.size();i++) {
		result.push_back(std::pair<size_t, maplist_entry_t*>(i, &(this->maplist[i])));
	}
	return true;
}

// Run a query on the maplist and return a list of all matching entries.
bool Maplist::query(const std::vector<std::string> &query,
					std::vector<std::pair<size_t, maplist_entry_t*> > &result) {
	// There's no maplist to query.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	// If we passed an empty query, return everything.
	if (query.empty()) {
		return this->query(result);
	}

	// If we passed a single entry that is a number, return that single entry
	if (query.size() == 1) {
		size_t index;
		std::istringstream buffer(query[0]);
		buffer >> index;

		if (buffer) {
			if (query[0][0] == '-' || index == 0) {
				this->error = "Index must be a positive number.";
				return false;
			}

			if (index > this->maplist.size()) {
				std::ostringstream error_ss;
				error_ss << "Index " << index << " out of range.";
				this->error = error_ss.str();
				return false;
			}
			index -= 1;
			result.push_back(std::pair<size_t, maplist_entry_t*>(index, &(this->maplist[index])));
			return true;
		}
	}

	for (std::vector<std::string>::const_iterator it = query.begin();it != query.end();++it) {
		std::string pattern = "*" + (*it) + "*";
		if (it == query.begin()) {
			// Check the entire maplist for a match
			for (size_t i = 0;i < this->maplist.size();i++) {
				bool f_map = CheckWildcards(pattern.c_str(), this->maplist[i].map.c_str());
				bool f_wad = CheckWildcards(pattern.c_str(), JoinStrings(this->maplist[i].wads).c_str());
				if (f_map || f_wad) {
					result.push_back(std::pair<size_t, maplist_entry_t*>(i, &(this->maplist[i])));
				}
			}
		} else {
			// Discard any map that doesn't match
			std::vector<std::pair<size_t, maplist_entry_t*> >::iterator itr;
			for (itr = result.begin();itr != result.end();) {
				bool f_map = CheckWildcards(pattern.c_str(), this->maplist[itr->first].map.c_str());
				bool f_wad = CheckWildcards(pattern.c_str(), JoinStrings(this->maplist[itr->first].wads).c_str());
				if (f_map || f_wad) {
					++itr;
				} else {
					itr = result.erase(itr);
				}
			}
		}

		if (result.empty()){
			// Result set is empty? We're done!
			return true;
		}
	}

	// We're done with every passed string.
	return true;
}

// Set the map index.
bool Maplist::set_index(const size_t &index) {
	// There's no maplist.
	if (this->maplist.empty()) {
		this->error = "Maplist is empty.";
		return false;
	}

	// Desired position is outside of the maplist
	if (index >= this->maplist.size()) {
		std::ostringstream buffer;
		buffer << "Index " << index + 1 << " out of range.";
		this->error = buffer.str();
		return false;
	}

	this->entered_once = true;
	this->in_maplist = true;
	this->index = index;
	this->update_shuffle_index();
	return true;
}

// Set or clear map shuffling.
void Maplist::set_shuffle(const bool setting) {
	if (this->shuffled == false && setting == true) {
		this->shuffle();
	}
	this->shuffled = setting;
}

// Check the timeout for a specific player id, return true if their time
// is up or if they have no timeout.
bool Maplist::pid_timeout(const int index) {
	if (this->timeout.find(index) == this->timeout.end()) {
		return true;
	}

	return (this->timeout[index] > I_MSTime());
}

// Return true if we've already passed our maplist to the player id.  This
// method doesn't perform any comparisons, it just sees if the timeout
// exists at all.
bool Maplist::pid_cached(const int index) {
	return (this->timeout.find(index) != this->timeout.end());
}

// Set the timeout for a player.
void Maplist::set_timeout(const int index) {
	// Set timeout to 60 seconds from now.
	this->timeout[index] = I_MSTime() + (1000 * 60);
}

// Clear the timeout for a disconnecting player.
void Maplist::clear_timeout(const int index) {
	this->timeout.erase(index);
}

//////// COMMANDS TO CLIENT ////////

// Clue the client in on if the maplist is outdated or not and if they're
// allowed to retrieve the entire maplist yet.
void SVC_Maplist(player_t &player, maplist_status_t status) {
	client_t *cl = &player.client;

	switch (status) {
	case MAPLIST_EMPTY:
	case MAPLIST_OUTDATED:
	case MAPLIST_OK:
		// Valid statuses
		DPrintf("SVC_Maplist: Sending status %d to pid %d\n", status, player.id);
		MSG_WriteMarker(&cl->reliablebuf, svc_maplist);
		MSG_WriteByte(&cl->reliablebuf, status);
	default:
		// Invalid statuses
		return;
	}
}

// Send the player the next map index and current map index if it exists.
void SVC_MaplistIndex(player_t &player) {
	client_t *cl = &player.client;

	// count = 0: No indexes.
	// count = 1: Next map index.
	// count = 2: Next map & this map indexes.
	byte count = 0;
	size_t this_index, next_index;
	if (Maplist::instance().get_next_index(next_index)) {
		count += 1;
		if (Maplist::instance().get_this_index(this_index)) {
			count += 1;
		}
	}

	MSG_WriteMarker(&cl->reliablebuf, svc_maplist_index);
	MSG_WriteByte(&cl->reliablebuf, count);

	if (count > 0) {
		MSG_WriteShort(&cl->reliablebuf, next_index);
		if (count > 1) {
			MSG_WriteShort(&cl->reliablebuf, this_index);
		}
	}
}

// Send a full maplist update to a specific player
void SVC_MaplistUpdate(player_t &player, maplist_status_t status) {
	client_t *cl = &player.client;

	switch (status) {
	case MAPLIST_EMPTY:
	case MAPLIST_TIMEOUT:
		// Valid statuses that don't require the packet logic
		DPrintf("SVC_MaplistUpdate: Sending status %d to pid %d\n", status, player.id);
		MSG_WriteMarker(&cl->reliablebuf, svc_maplist_update);
		MSG_WriteByte(&cl->reliablebuf, status);
		return;
	case MAPLIST_OUTDATED:
		// Valid statuses that need the packet logic
		DPrintf("SVC_MaplistUpdate: Sending status %d to pid %d\n", status, player.id);
		break;
	default:
		// Invalid statuses
		return;
	}

	std::vector<std::pair<size_t, maplist_entry_t*> > maplist;
	Maplist::instance().query(maplist);

	MSG_WriteMarker(&cl->reliablebuf, svc_maplist_update); // new packet
	MSG_WriteByte(&cl->reliablebuf, MAPLIST_OUTDATED);
	MSG_WriteShort(&cl->reliablebuf, maplist.size()); // total size
	MSG_WriteShort(&cl->reliablebuf, 0); // starting index
	for (std::vector<std::pair<size_t, maplist_entry_t*> >::iterator it = maplist.begin();
		 it != maplist.end();++it) {
		MSG_WriteString(&cl->reliablebuf, it->second->map.c_str());
		MSG_WriteShort(&cl->reliablebuf, it->second->wads.size());
		for (std::vector<std::string>::iterator itr = it->second->wads.begin();
			 itr != it->second->wads.end();++itr) {
			MSG_WriteString(&cl->reliablebuf, itr->c_str());
		}

		// If we're at the end of the maplist, we don't want to continue the
		// message any longer and we don't need to start a new packet, so we
		// need this check before the fragmenting logic below.
		if (it == maplist.end() - 1) {
			MSG_WriteBool(&cl->reliablebuf, false); // end packet for good
			break;
		}

		// This message could get huge, so send it
		// when it grows to a specific size.
		if (cl->reliablebuf.cursize >= 1024) {
			MSG_WriteBool(&cl->reliablebuf, false); // end packet
			SV_SendPacket(player);
			MSG_WriteMarker(&cl->reliablebuf, svc_maplist_update); // new packet
			MSG_WriteByte(&cl->reliablebuf, MAPLIST_OUTDATED);
			MSG_WriteShort(&cl->reliablebuf, maplist.size()); // total size
			MSG_WriteShort(&cl->reliablebuf, it->first + 1); // next index
		} else {
			MSG_WriteBool(&cl->reliablebuf, true); // continue packet
		}
	}

	// Update the timeout to ensure the player doesn't abuse the server
	Maplist::instance().set_timeout(player.id);
}

//////// CLIENT COMMANDS ////////

// Client wants to know the status of the maplist.
void SV_Maplist(player_t &player) {
	maplist_status_t status = (maplist_status_t)MSG_ReadByte();

	// If the maplist is empty, say so
	if (Maplist::instance().empty()) {
		SVC_Maplist(player, MAPLIST_EMPTY);
		return;
	}

	// If they haven't been sent the maplist, they're obviously out of date.
	if (!Maplist::instance().pid_cached(player.id)) {
		SVC_Maplist(player, MAPLIST_OUTDATED);
		return;
	}

	// If they have been sent the maplist, check to see what the status
	// of their maplist cache is.
	switch (status) {
	case MAPLIST_OK:
		// They think their maplist is okay, and we haven't changed it,
		// so send back proper maplist indexes and an "OK" message.
		SVC_MaplistIndex(player);
		SVC_Maplist(player, MAPLIST_OK);
		return;
	case MAPLIST_EMPTY:
	case MAPLIST_TIMEOUT:
	case MAPLIST_THROTTLED:
		// Their maplist is clearly wrong, so we need to fall through
		// to the rest of the function to see if we can respond.
		break;
	default:
		// We don't know what status they sent us.  Ignore it.
		DPrintf("SV_Maplist: Unexpected status %d from player %d\n",
				status, player.id);
		return;
	}

	// Check to see if their timeout has passed.
	if (Maplist::instance().pid_timeout(player.id)) {
		SVC_Maplist(player, MAPLIST_OUTDATED);
		return;
	}

	// They have been sent the maplist, but their timeout has not expired yet.
	SVC_Maplist(player, MAPLIST_THROTTLED);
}

// Client wants the full list of maps.
void SV_MaplistUpdate(player_t &player) {
	// Send them the current maplist indexes.
	SVC_MaplistIndex(player);

	// If the maplist is empty, say so
	if (Maplist::instance().empty()) {
		SVC_MaplistUpdate(player, MAPLIST_EMPTY);
		return;
	}

	// If the player requested the maplist update too soon, say so
	if (!Maplist::instance().pid_timeout(player.id)) {
		SVC_MaplistUpdate(player, MAPLIST_THROTTLED);
		return;
	}

	// Send them an update
	SVC_MaplistUpdate(player, MAPLIST_OUTDATED);
}

//////// EVENTS ////////

void Maplist_Disconnect(player_t &player) {
	Maplist::instance().clear_timeout(player.id);
}

//////// CONSOLE COMMANDS ////////

BEGIN_COMMAND (maplist) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// Query the maplist
	std::vector<std::pair<size_t, maplist_entry_t*> > result;
	if (!Maplist::instance().query(arguments, result)) {
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
		return;
	}

	// Rip through the result set and print it
	size_t this_index, next_index;
	bool show_this_map = Maplist::instance().get_this_index(this_index);
	Maplist::instance().get_next_index(next_index);
	for (std::vector<std::pair<size_t, maplist_entry_t*> >::iterator it = result.begin();
		 it != result.end();++it) {
		char flag = ' ';
		if (show_this_map && it->first == this_index) {
			flag = '*';
		} else if (it->first == next_index) {
			flag = '+';
		}
		Printf(PRINT_HIGH, "%c%d. %s %s\n", flag, it->first + 1,
			   JoinStrings(it->second->wads, " ").c_str(),
			   it->second->map.c_str());
	}
} END_COMMAND (maplist)

BEGIN_COMMAND (addmap) {
	if (argc < 2) {
		Printf(PRINT_HIGH, "Usage: addmap <map lump> [wad name] [...]\n");
		return;
	}

	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// Grab the map lump.
	maplist_entry_t maplist_entry;
	maplist_entry.map = arguments[0];

	// If we specified any WAD files, grab them too.
	if (arguments.size() > 1) {
		maplist_entry.wads.resize(arguments.size() - 1);
		std::copy(arguments.begin() + 1, arguments.end(), maplist_entry.wads.begin());
	}

	if (!Maplist::instance().add(maplist_entry)) {
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
	}
} END_COMMAND(addmap)

BEGIN_COMMAND(insertmap) {
	if (argc < 3) {
		Printf(PRINT_HIGH, "Usage: insertmap <maplist position> <map lump> [wad name] [...]\n");
	}

	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// Grab the position, make sure it's greater than zero.
	size_t maplist_position;
	std::istringstream buffer(arguments[0]);
	buffer >> maplist_position;
	if (maplist_position == 0 || arguments[0][0] == '-') {
		Printf(PRINT_HIGH, "Position must be a positive number.\n");
		return;
	}

	// Grab the map lump.
	maplist_entry_t maplist_entry;
	maplist_entry.map = arguments[1];

	// If we specified any WAD files, grab them too.
	if (arguments.size() > 2) {
		maplist_entry.wads.resize(arguments.size() - 2);
		std::copy(arguments.begin() + 2, arguments.end(), maplist_entry.wads.begin());
	}

	if (!Maplist::instance().insert(maplist_position - 1, maplist_entry)) {
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
	}
} END_COMMAND(insertmap)

BEGIN_COMMAND(delmap) {
	if (argc < 2) {
		Printf(PRINT_HIGH, "Usage: delmap <maplist index>\n");
	}

	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// Grab the position, make sure it's greater than zero.
	size_t maplist_index;
	std::istringstream buffer(arguments[0]);
	buffer >> maplist_index;
	if (maplist_index == 0 || arguments[0][0] == '-') {
		Printf(PRINT_HIGH, "Index must be a positive number.\n");
		return;
	}

	if (!Maplist::instance().remove(maplist_index - 1)) {
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
	}
} END_COMMAND(delmap)

BEGIN_COMMAND(clearmaplist) {
	if (!Maplist::instance().clear()) {
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
	}
} END_COMMAND(clearmaplist)

CVAR_FUNC_IMPL(sv_shufflemaplist) {
	bool setting = var.cstring()[0] != '0';
	Maplist::instance().set_shuffle(setting);
}

BEGIN_COMMAND (gotomap) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);

	if (arguments.empty()) {
		Printf(PRINT_HIGH, "Usage: gotomap <map index or unambiguous map name>\n");
		return;
	}

	// Query the maplist
	query_result_t result;
	if (!Maplist::instance().query(arguments, result)) {
		Printf(PRINT_HIGH, "%s\n", Maplist::instance().get_error().c_str());
		return;
	}

	// If we got back an empty response, complain.
	if (result.empty()) {
		Printf(PRINT_HIGH, "Map not found.\n");
		return;
	}

	// If we got back more than one response, complain.
	if (result.size() > 1) {
		Printf(PRINT_HIGH, "Map is ambiguous.\n");
		return;
	}

	// Change to the map.
	G_ChangeMap(result[0].first);
} END_COMMAND (gotomap)

bool CMD_Randmap(std::string &error) {
	query_result_t result;
	if (!Maplist::instance().query(result)) {
		error = Maplist::instance().get_error();
		return false;
	}

	// Change to the map.
	size_t index = rand() % result.size();
	G_ChangeMap(index);
	return true;
}

BEGIN_COMMAND (randmap) {
	std::string error;
	if (!CMD_Randmap(error)) {
		Printf(PRINT_HIGH, "%s\n", error.c_str());
	}
} END_COMMAND (randmap)
