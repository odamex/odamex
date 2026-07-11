// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	Compatibility hacks for older maps that won't be updated
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "g_level.h"

#include "p_compdb.h"

template <>
struct std::hash<fhfprint_t>
{
	constexpr size_t operator()(const fhfprint_t& f) const noexcept
	{
		const size_t halfhash = (size_t)(f.fingerprint[0]) |
		                        (size_t)(f.fingerprint[1]) << 8 |
		                        (size_t)(f.fingerprint[2]) << 16 |
		                        (size_t)(f.fingerprint[3]) << 24 |
		                        (size_t)(f.fingerprint[4]) << 32 |
		                        (size_t)(f.fingerprint[5]) << 40 |
		                        (size_t)(f.fingerprint[6]) << 48 |
		                        (size_t)(f.fingerprint[7]) << 56;
		return halfhash;
	}
};

static const std::unordered_map<fhfprint_t, levelcompdata_t> compdata = {
	{
		// Congestion 1024 MAP23
		fhfprint_t::fromString("b0b0b3a99c2ed6780a5a79b325dcc5ca"),
		{
			// TODO C++20: use designated initializers for readability here
			true // reservedLineFlag
		}
	},
	{
		// UDMX MAP32
		fhfprint_t::fromString("c13f47bbcca3fc2d5013c17a604af645"),
		{
			true // reservedLineFlag
		}
	},
};

const levelcompdata_t& P_GetLevelCompData(fhfprint_t fingerprint)
{
	const auto it = compdata.find(fingerprint);
	if (it != compdata.end())
		return it->second;

	static constexpr levelcompdata_t defaultData{};
	return defaultData;
}

VERSION_CONTROL (p_compdb_cpp, "$Id$")