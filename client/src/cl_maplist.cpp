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
//  Clientside maplist-related functionality.
//
//-----------------------------------------------------------------------------

#include <map>
#include <sstream>
#include <string>

#include "c_maplist.h"
#include "cl_maplist.h"
#include "cl_main.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "i_net.h"
#include "i_system.h"

//////// MAPLIST CACHE METHODS (Private) ////////

// Go from OUTDATED to OK status if the maplist cache is complete.
void MaplistCache::check_complete() {
	if (this->status != MAPLIST_OUTDATED) {
		return;
	}

	if (this->maplist.size() > 0 &&
		(this->size == this->maplist.size()) &&
		(this->valid_indexes > 0)) {
		this->status = MAPLIST_OK;
	}
}

// Invalidate the cache.
void MaplistCache::invalidate() {
	this->maplist.clear();
	this->valid_indexes = 0;
}

// Return the entire maplist.
bool MaplistCache::query(query_result_t &result) {
	// Check to see if the query should fail and pass an error.
	if (this->status != MAPLIST_OK) {
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
bool MaplistCache::query(const std::vector<std::string> &query,
						 query_result_t &result) {
	// Check to see if the query should fail and pass an error.
	if (this->status != MAPLIST_OK) {
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

//////// MAPLIST CACHE METHODS (Public) ////////

// Return a singleton reference for the class.
MaplistCache& MaplistCache::instance() {
	static MaplistCache singleton;
	return singleton;
}

// Tic-by-tic handling of the Maplist Cache.
void MaplistCache::ev_tic() {
	// No query callbacks to worry about
	if (this->deferred_queries.empty()) {
		return;
	}

	if (!connected) {
		// If we're not connected to a server, our maplist cache is useless.
		this->invalidate();
		this->status = MAPLIST_EMPTY;

		// Our deferred queries are similarly useless.
		this->error = "You are not connected to a server.";
		for (std::vector<deferred_query_t>::iterator it = this->deferred_queries.begin();
			 it != this->deferred_queries.end();++it) {
			it->errback(this->error);
		}
		this->deferred_queries.clear();
		return;
	}

	// Handle a potential timeout condition.
	if (this->timeout < I_MSTime()) {
		this->status = MAPLIST_TIMEOUT;
	}

	switch (this->status) {
	case MAPLIST_OK:
		// If we have an "OK" maplist status, we ought to run
		// our callbacks and get things over with.
		for (std::vector<deferred_query_t>::iterator it = this->deferred_queries.begin();
			 it != this->deferred_queries.end();++it) {
			query_result_t query_result;
			this->query(it->query, query_result);
			it->callback(query_result);
		}
		this->deferred_queries.clear();
		return;
	case MAPLIST_EMPTY:
		this->error = "Maplist is empty.";
		break;
	case MAPLIST_TIMEOUT:
		this->error = "Maplist update timed out.";
		DPrintf("MaplistCache::ev_tic: Maplist Cache Update Timeout.\n");
		DPrintf("- Successfully Cached Maps: %d\n", this->maplist.size());
		DPrintf("- Destionation Maplist Size: %d\n", this->size);
		DPrintf("- Valid Indexes: %d\n", this->valid_indexes);
		break;
	case MAPLIST_THROTTLED:
		this->error = "Server refused to send the maplist.";
		break;
	default:
		// We're still waiting for results (MAPLIST_WAIT or MAPLIST_OUTDATED).
		// Bail out until the next tic.
		return;
	}

	// Handle our error conditions by running our errbacks.
	for (std::vector<deferred_query_t>::iterator it = this->deferred_queries.begin();
		 it != this->deferred_queries.end();++it) {
		it->errback(this->error);
	}
	this->deferred_queries.clear();
}

// Error getter
const std::string& MaplistCache::get_error() {
	return this->error;
}

bool MaplistCache::get_this_index(size_t &index) {
	if (this->status != MAPLIST_OK || this->valid_indexes < 2) {
		return false;
	}

	index = this->index;
	return true;
}

bool MaplistCache::get_next_index(size_t &index) {
	if (this->status != MAPLIST_OK || this->valid_indexes < 1) {
		return false;
	}

	index = this->next_index;
	return true;
}

// Returns the current cache status.
maplist_status_t MaplistCache::get_status() {
	return this->status;
}

// A deferred query with no params.
void MaplistCache::defer_query(query_callback_t query_callback,
							   query_errback_t query_errback) {
	std::vector<std::string> query;
	this->defer_query(query, query_callback, query_errback);
}

// A deferred query.
void MaplistCache::defer_query(const std::vector<std::string> &query,
							   query_callback_t query_callback,
							   query_errback_t query_errback) {
	if (this->deferred_queries.empty()) {
		// Only send out a maplist status packet if we don't already have a
		// deferred query in progress.
		MSG_WriteMarker(&net_buffer, clc_maplist);
		MSG_WriteByte(&net_buffer, this->status);
		this->status = MAPLIST_WAIT;
		this->timeout = I_MSTime() + (1000 * 3);
	}

	deferred_query_t deferred_query;
	deferred_query.query = query;
	deferred_query.callback = query_callback;
	deferred_query.errback = query_errback;
	this->deferred_queries.push_back(deferred_query);
}

// Handle the maplist status response
void MaplistCache::status_handler(maplist_status_t status) {
	switch (status) {
	case MAPLIST_OUTDATED:
		// If our cache is out-of-date and we are able to request
		// an updated maplist, request one.
		MSG_WriteMarker(&net_buffer, clc_maplist_update);
	case MAPLIST_EMPTY:
	case MAPLIST_THROTTLED:
		// If our cache is out-of-date or the maplist on the other end
		// is empty, invalidate the local cache.
		this->invalidate();
	case MAPLIST_OK:
		// No matter what, we ought to set the correct status.
		this->status = status;
		break;
	default:
		DPrintf("MaplistCache::status_handler: Unknown status %d from server.\n", status);
		return;
	}
}

// Handle the maplist update status response.  Pass false if the update
// packet read should be abandoned.
bool MaplistCache::update_status_handler(maplist_status_t status) {
	switch (status) {
	case MAPLIST_EMPTY:
	case MAPLIST_THROTTLED:
		// Our cache is useless.
		this->invalidate();
		this->status = status;
		return false;
	case MAPLIST_OUTDATED:
		return true;
	default:
		DPrintf("MaplistCache::status_handler: Unknown status %d from server.\n", status);
		return true;
	}
}

// Sets the current map index
void MaplistCache::set_this_index(const size_t index) {
	if (this->valid_indexes < 1 || this->status == MAPLIST_EMPTY) {
		return;
	}

	this->valid_indexes = 2;
	this->index = index;
	this->check_complete();
}

// Unset the current map index
void MaplistCache::unset_this_index() {
	if (this->valid_indexes < 2 || this->status == MAPLIST_EMPTY) {
		return;
	}

	this->valid_indexes = 1;
	this->check_complete();
}

// Sets the next map index
void MaplistCache::set_next_index(const size_t index) {
	if (this->status == MAPLIST_EMPTY) {
		return;
	}

	if (this->valid_indexes < 2) {
		this->valid_indexes = 1;
	}
	this->next_index = index;
	this->check_complete();
}

// Sets the desired maplist cache size
void MaplistCache::set_size(const size_t size) {
	if (this->status != MAPLIST_OUTDATED) {
		return;
	}

	this->size = size;
	this->check_complete();
}

// Updates an entry in the cache.
void MaplistCache::set_cache_entry(const size_t index, const maplist_entry_t &maplist_entry) {
	if (this->status != MAPLIST_OUTDATED) {
		return;
	}

	this->maplist[index] = maplist_entry;
	this->check_complete();
}

//////// SERVER COMMANDS ////////

// Got a packet that contains the maplist status
void CL_Maplist() {
	maplist_status_t status = (maplist_status_t)MSG_ReadByte();
	DPrintf("CL_Maplist: Status %d\n", status);
	MaplistCache::instance().status_handler(status);
}

// Got a packet that contains the next and current index.
void CL_MaplistIndex(void) {
	byte count = MSG_ReadByte();
	DPrintf("CL_MaplistIndex: Count %d\n", count);
	if (count > 0) {
		MaplistCache::instance().set_next_index(MSG_ReadShort());
		if (count > 1) {
			MaplistCache::instance().set_this_index(MSG_ReadShort());
		} else {
			MaplistCache::instance().unset_this_index();
		}
	}
}

// Got a packet that contains a chunk of the maplist.
void CL_MaplistUpdate(void) {
	// The update status might require us to bail out.
	maplist_status_t status = (maplist_status_t)MSG_ReadByte();
	DPrintf("CL_MaplistUpdate: Status %d\n", status);
	if (!MaplistCache::instance().update_status_handler(status)) {
		return;
	}

	size_t maplist_size = MSG_ReadShort();
	DPrintf("CL_MaplistUpdate: Size %d\n", maplist_size);

	// If we've gotten this far, set the destination maplist size.
	MaplistCache::instance().set_size(maplist_size);

	size_t starting_index = MSG_ReadShort();
	for (size_t i = starting_index;i < maplist_size;i++) {
		maplist_entry_t maplist_entry;
		maplist_entry.map = MSG_ReadString();

		size_t wads_size = MSG_ReadShort();
		for (size_t j = 0;j < wads_size;j++) {
			maplist_entry.wads.push_back(MSG_ReadString());
		}

		MaplistCache::instance().set_cache_entry(i, maplist_entry);

		if (!MSG_ReadBool()) {
			// We have reached the end of the packet.
			break;
		}
	}
}

// Handle tic-by-tic maintenance of the various maplist functionality.
void Maplist_Runtic() {
	MaplistCache::instance().ev_tic();
}

//////// CONSOLE COMMANDS ////////

// Clientside maplist query callback.
void CMD_MaplistCallback(const query_result_t &result) {
	// Rip through the result set and print it
	size_t this_index = 0, next_index = 0;
	bool show_this_map = MaplistCache::instance().get_this_index(this_index);
	MaplistCache::instance().get_next_index(next_index);
	for (query_result_t::const_iterator it = result.begin();it != result.end();++it) {
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
}

// Clientside maplist query errback.
void CMD_MaplistErrback(const std::string &error) {
	Printf(PRINT_HIGH, "%s\n", error.c_str());
}

// Clientside maplist query.
BEGIN_COMMAND (maplist) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	MaplistCache::instance().defer_query(arguments, &CMD_MaplistCallback,
										 &CMD_MaplistErrback);
} END_COMMAND (maplist)
