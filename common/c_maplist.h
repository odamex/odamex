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
//  Common maplist-related functionality.
//
//-----------------------------------------------------------------------------

#pragma once

#include <nonstd/expected.hpp>

// Maplist statuses
enum maplist_status_t
{
	// The maplist cache is waiting on a status update from the server.
	MAPLIST_WAIT,
	// The maplist cache is out-of-date and is currently being updated.
	MAPLIST_OUTDATED,
	// The maplist cache is up-to-date.
	MAPLIST_OK,
	// The maplist cache is empty due to there not being any serverside
	// maplist at all.
	MAPLIST_EMPTY,
	// The maplist cache might be out-of-date and was waiting for a response
	// from the server but ran out of time.
	MAPLIST_TIMEOUT,
	// The maplist cache tried to update but was throttled by the server.
	MAPLIST_THROTTLED
};
#define NUM_MAPLIST_STATUS (MAPLIST_THROTTLED + 1)

inline auto format_as(maplist_status_t eStatus)
{
	return fmt::underlying(eStatus);
}

struct maplist_lastmaps_t {
	std::vector<std::pair<std::string, std::string>> entries;

	bool empty() const {
		return entries.empty();
	}

	static nonstd::expected<maplist_lastmaps_t, std::string> parse(const std::string& lastmaps);
};

// Map list entry structure
struct maplist_entry_t {
	std::string map;
	// FIXME: there's some lastmap duplication here because of this being in common
	// and requiring compatibility with 11.x
	// rework this for 12.0.0
	std::string lastmap;
	maplist_lastmaps_t lastmaps;
	std::vector<std::string> wads;
};

inline auto format_as(const maplist_lastmaps_t& lastmaps)
{
	std::string out;
	bool first = true;
	for (const auto& [from, to] : lastmaps.entries)
	{
		if (!first)
			out += ",";

		if (!to.empty())
			out += fmt::format("{}->{}", from, to);
		else
			out += from.c_str();

		first = false;
	}
	return out;
}

// Query result
typedef std::pair<size_t, maplist_entry_t*> maplist_qrow_t;
typedef std::vector<maplist_qrow_t> maplist_qrows_t;
