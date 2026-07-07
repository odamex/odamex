// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Sky rendering (serverside stub).  Tracks which flats represent skies,
//	including ID24 SKYDEFS flat mappings, for gameplay logic.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "g_level.h"
#include "i_system.h"
#include "m_jsonlump.h"
#include "r_data.h"
#include "g_mapinfo.h"

#include "resources/res_resourceid.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

#include <unordered_set>

static ResourceId sky_flat_resource_id = ResourceId::INVALID_ID;

static std::unordered_set<uint32_t> skyflatlookup;

//
// R_ResourceIdIsSkyFlat
//
// Returns true if the given ResourceId represents the sky.
//
bool R_ResourceIdIsSkyFlat(const ResourceId res_id)
{
	if (res_id == sky_flat_resource_id)
		return true;
	return skyflatlookup.contains(static_cast<uint32_t>(res_id));
}


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
			const ResourceId flat_res_id =
			    Res_GetTextureResourceId(OStringToUpper(flatname.c_str()), FLOOR);
			if (!Res_CheckResource(flat_res_id)) return jsonlumpresult_t::PARSEERROR;

			skyflatlookup.insert(static_cast<uint32_t>(flat_res_id));
		}

		return jsonlumpresult_t::SUCCESS;
	};

	jsonlumpresult_t result =  M_ParseJSONLump("SKYDEFS", "skydefs", { 1, 0, 0 }, ParseSkydef);
	if (result != jsonlumpresult_t::SUCCESS && result != jsonlumpresult_t::NOTFOUND)
		I_Error("R_InitSkyDefs: SKYDEFS JSON error: {}", M_JSONLumpResultToString(result));
}

void R_ClearSkyDefs()
{
	skyflatlookup.clear();
}


//
// R_InitSkyMap
//
void R_InitSkyMap()
{
}


//
// R_SetSkyTextures
//
// Loads the sky textures and re-initializes the sky map lookup tables.
//
void R_SetSkyTextures(const char* sky1_name, const char* sky2_name)
{
	// [SL] 2011-11-30 - Don't run if we don't know what sky texture to use
	if (gamestate != GS_LEVEL)
		return;

	const ResourceId res_id = Res_GetTextureResourceId(OStringToUpper(sky1_name, 8), WALL);
	if (!Res_CheckResource(res_id))
		I_Error("Invalid sky1 texture \"{}\"", OStringToUpper(sky1_name, 8));

	if (HexenHack)
		sky_flat_resource_id = Res_GetTextureResourceId("F_SKY", FLOOR);
	else
		sky_flat_resource_id = Res_GetTextureResourceId("F_SKY1", FLOOR);

	R_InitSkyMap();
}


VERSION_CONTROL (r_sky_cpp, "$Id$")
