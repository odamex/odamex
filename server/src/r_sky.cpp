// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
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
//	Sky rendering.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "m_jsonlump.h"
#include "r_sky.h"
#include "r_data.h"
#include "w_wad.h"
#include "i_system.h"

#include <unordered_set>

std::unordered_set<int32_t> skyflatlookup;

void R_InitSkyDefs()
{
	auto ParseSkydef = [](const Json::Value& elem, const JSONLumpVersion& version) -> jsonlumpresult_t
	{
		const Json::Value& skyarray = elem["skies"];
		const Json::Value& flatmappings = elem["flatmapping"];

		if (!(skyarray.isArray() || skyarray.isNull())) return jsonlumpresult_t::PARSEERROR;
		if (!(flatmappings.isArray() || flatmappings.isNull())) return jsonlumpresult_t::PARSEERROR;

		for (const Json::Value& flatentry : flatmappings)
		{
			const Json::Value& flatelem = flatentry["flat"];

			OLumpName flatname = flatelem.asString();
			int32_t flatnum = R_FlatNumForName(flatname);
			if(flatnum < 0 || flatnum >= ::numflats) return jsonlumpresult_t::PARSEERROR;

			skyflatlookup.insert(flatnum);
		}

		return jsonlumpresult_t::SUCCESS;
	};

	jsonlumpresult_t result =  M_ParseJSONLump("SKYDEFS", "skydefs", { 1, 0, 0 }, ParseSkydef);
	if (result != jsonlumpresult_t::SUCCESS && result != jsonlumpresult_t::NOTFOUND)
		I_Error("R_InitSkyDefs: SKYDEFS JSON error: {}", M_JSONLumpResultToString(result));
}

bool R_IsSkyFlat(int flatnum)
{
    return flatnum == skyflatnum || skyflatlookup.count(flatnum);
}

void R_ClearSkyDefs()
{
	skyflatlookup.clear();
}

VERSION_CONTROL (r_sky_cpp, "$Id$")