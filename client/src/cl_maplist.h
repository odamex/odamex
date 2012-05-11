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

#ifndef __CL_MAPLIST__
#define __CL_MAPLIST__

#include <map>
#include <string>

#include "c_maplist.h"
#include "doomtype.h"

// Callbacks for deffered queries
typedef void (*query_callback_t)(const query_result_t&);
typedef void (*query_errback_t)(const std::string&);

typedef struct {
	std::vector<std::string> query;
	query_callback_t callback;
	query_errback_t errback;
} deferred_query_t;

// Clientside maplist cache
class MaplistCache {
private:
	std::vector<deferred_query_t> deferred_queries;
	std::string error;
	size_t index;
	std::map<size_t, maplist_entry_t> maplist;
	size_t next_index;
	size_t size;
	maplist_status_t status;
	QWORD timeout;
	byte valid_indexes;
	void check_complete(void);
	void invalidate(void);
	bool query(query_result_t &result);
	bool query(const std::vector<std::string> &query, query_result_t &result);
public:
	MaplistCache(void) : error(""), index(0), next_index(0), size(0),
						 status(MAPLIST_EMPTY), timeout(0),
						 valid_indexes(0) { };
	static MaplistCache& instance(void);
	// Events
	void ev_tic(void);
	// Getters
	const std::string& get_error(void);
	bool get_this_index(size_t &index);
	bool get_next_index(size_t &index);
	maplist_status_t get_status(void);
	// Queries
	void defer_query(query_callback_t query_callback,
					 query_errback_t query_errback);
	void defer_query(const std::vector<std::string> &query,
					 query_callback_t query_callback,
					 query_errback_t query_errback);
	// Packet handlers
	void status_handler(maplist_status_t status);
	bool update_status_handler(maplist_status_t status);
	// Cache modification
	void set_this_index(const size_t index);
	void unset_this_index(void);
	void set_next_index(const size_t index);
	void set_size(const size_t index);
	void set_cache_entry(const size_t index, const maplist_entry_t &maplist_entry);
};

void CL_Maplist(void);
void CL_MaplistIndex(void);
void CL_MaplistUpdate(void);

void Maplist_Runtic(void);

#endif
