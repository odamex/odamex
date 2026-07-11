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
//		ID24 intermission screens.
//
//-----------------------------------------------------------------------------

#include "wi_interlevel.h"

#include "i_system.h"
#include "m_jsonlump.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"
#include "oscanner.h"
#include "hashtable.h"

#include <memory>

static OHashTable<OLumpName, std::unique_ptr<interlevel_t>> interlevelstorage;

template<typename T>
jsonlumpresult_t WI_ParseInterlevelArray(const Json::Value& array, std::vector<T>& output,
                                         std::function<jsonlumpresult_t(const Json::Value&, T&)>&& parse)
{
	if (!array.isArray())
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	for (const auto& arrayelem : array)
	{
		jsonlumpresult_t res = parse(arrayelem, output.emplace_back());
		if(res != jsonlumpresult_t::SUCCESS)
			return res;
	}

	return jsonlumpresult_t::SUCCESS;
}

jsonlumpresult_t WI_ParseInterlevelCondition(const Json::Value& condition, interlevelcond_t& output)
{
	const Json::Value& animcondition = condition["condition"];
	const Json::Value& param = condition["param"];

	if (!animcondition.isNumeric()
		|| !param.isNumeric())
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	output.condition = static_cast<animcondition_t>(animcondition.asInt());
	output.param = param.asInt();

	if (output.condition < animcondition_t::None || output.condition >= animcondition_t::ID24Max)
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	return jsonlumpresult_t::SUCCESS;
}

jsonlumpresult_t WI_ParseInterlevelFrame(const Json::Value& frame, interlevelframe_t& output)
{
	const Json::Value& image = frame["image"];
	const Json::Value& altimage = frame["altimage"]; // nonstandard - should only be used internally by lumps in odamex.wad
	const Json::Value& type = frame["type"];
	const Json::Value& duration = frame["duration"];
	const Json::Value& maxduration = frame["maxduration"];

	if (!image.isString()
		|| !type.isNumeric()
		|| !duration.isNumeric()
		|| !maxduration.isNumeric())
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	output.imagelump = image.asString();
	output.imageresourceid = Res_GetTextureResourceId(
	    OStringToUpper(output.imagelump.c_str()), GRAPHICS, false);
	if (output.imageresourceid == ResourceId::INVALID_ID)
	{
		// TNT1A0 used for transparent by Legacy of Rust
		output.imageresourceid = Res_GetTextureResourceId(
		    OStringToUpper(output.imagelump.c_str()), SPRITE);
	}
	output.altimagelump = altimage.asString();
	output.altimageresourceid = Res_GetTextureResourceId(
	    OStringToUpper(output.altimagelump.c_str()), GRAPHICS, false);
	output.type = static_cast<interlevelframe_t::frametype_t>(type.asInt());
	output.duration = static_cast<int>(duration.asDouble() * TICRATE);
	output.maxduration = static_cast<int>(maxduration.asDouble() * TICRATE);

	if(output.type != 0 && (output.type & ~interlevelframe_t::Valid) != 0)
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	return jsonlumpresult_t::SUCCESS;
}

jsonlumpresult_t WI_ParseInterlevelAnim(const Json::Value& anim, interlevelanim_t& output)
{
	const Json::Value& xpos = anim["x"];
	const Json::Value& ypos = anim["y"];
	const Json::Value& frames = anim["frames"];
	const Json::Value& conditions = anim["conditions"];

	if (!xpos.isNumeric()
		|| !ypos.isNumeric()
		|| !(conditions.isArray() || conditions.isNull())
		|| !frames.isArray())
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	output.xpos = xpos.asInt();
	output.ypos = ypos.asInt();
	jsonlumpresult_t res = WI_ParseInterlevelArray<interlevelframe_t>(frames, output.frames, WI_ParseInterlevelFrame);
	if (res != jsonlumpresult_t::SUCCESS)
		return res;

	if (!conditions.isNull())
	{
		res = WI_ParseInterlevelArray<interlevelcond_t>(conditions, output.conditions, WI_ParseInterlevelCondition);
		if (res != jsonlumpresult_t::SUCCESS)
			return res;
	}

	return jsonlumpresult_t::SUCCESS;
}

jsonlumpresult_t WI_ParseInterlevelLayer(const Json::Value& anim, interlevellayer_t& output)
{
	const Json::Value& anims = anim["anims"];
	const Json::Value& conditions = anim["conditions"];

	jsonlumpresult_t res = WI_ParseInterlevelArray<interlevelanim_t>(anims, output.anims, WI_ParseInterlevelAnim);
	if (res != jsonlumpresult_t::SUCCESS)
		return res;

	if (!conditions.isNull())
	{
		res = WI_ParseInterlevelArray<interlevelcond_t>(conditions, output.conditions, WI_ParseInterlevelCondition);
		if(res != jsonlumpresult_t::SUCCESS)
			return res;
	}

	return jsonlumpresult_t::SUCCESS;
}

interlevel_t* WI_GetInterlevel(const OLumpName& lumpname)
{
	auto found = interlevelstorage.find(lumpname);
	if (found != interlevelstorage.end())
	{
		return found->second.get();
	}

	std::unique_ptr<interlevel_t> output = nullptr;
	auto ParseInterlevel = [&output]( const Json::Value& elem, const JSONLumpVersion& version ) -> jsonlumpresult_t
	{
		const Json::Value& music = elem["music"];
		const Json::Value& backgroundimage = elem["backgroundimage"];
		const Json::Value& layers = elem["layers"];

		if (!music.isString() || !backgroundimage.isString())
		{
			return jsonlumpresult_t::PARSEERROR;
		}

		output = std::make_unique<interlevel_t>();
		output->musiclump = music.asString();
		output->backgroundlump = backgroundimage.asString();
        output->layers = std::vector<interlevellayer_t>();
		jsonlumpresult_t res = jsonlumpresult_t::SUCCESS;
		if(!layers.isNull())
		{
			res = WI_ParseInterlevelArray<interlevellayer_t>(layers, output->layers, WI_ParseInterlevelLayer);
		}

		return res;
	};

	jsonlumpresult_t result =  M_ParseJSONLump(lumpname, "interlevel", { 1, 0, 0 }, ParseInterlevel);
	if (result != jsonlumpresult_t::SUCCESS)
	{
		I_Error("R_GetInterlevel: Interlevel JSON error in lump {}: {}", lumpname, M_JSONLumpResultToString(result));
		return nullptr;
	}

	interlevel_t* ret = output.get();
	interlevelstorage[lumpname] = std::move(output);

	return ret;
}

struct intermissionscript_t
{
	// unsupported vvv
	int screenx, screeny;
	bool autostart = true;
	bool tilebackground;
	// unsupported ^^^
	OLumpName splat;
	ResourceId splatresourceid;
	OLumpName ptr1, ptr2;
	ResourceId ptr1resourceid, ptr2resourceid;
	std::vector<std::tuple<OLumpName, int, int>> spots;
};

// some of the zdoom intermission conditions are the same as the logical OR of 2 id24 conditions
// id24 offers no way to do this directly (only AND), but we can just make one animation with each condition
// this is the purpose of the twoanims argument
void WI_ParseZDoomPic(OScanner& os, std::vector<interlevelanim_t>& anims, interlevelcond_t cond1 = {}, interlevelcond_t cond2 = {}, bool twoanims = false)
{
	os.mustScanInt();
	int x = os.getTokenInt();
	os.mustScanInt();
	int y = os.getTokenInt();
	os.mustScan(8);
	OLumpName picname = os.getToken();
	ResourceId picnum = Res_GetTextureResourceId(OStringToUpper(picname.c_str()), GRAPHICS);
	if (!twoanims && cond2.condition != animcondition_t::None)
		anims.emplace_back(std::vector<interlevelframe_t>{interlevelframe_t{picname, picnum, "", ResourceId::INVALID_ID, interlevelframe_t::DurationInf, 0, 0}}, std::vector<interlevelcond_t>{cond1, cond2}, x, y);
	else
		anims.emplace_back(std::vector<interlevelframe_t>{interlevelframe_t{picname, picnum, "", ResourceId::INVALID_ID, interlevelframe_t::DurationInf, 0, 0}}, std::vector<interlevelcond_t>{cond1}, x, y);
	if ((cond2.condition != animcondition_t::None) && twoanims)
		anims.emplace_back(std::vector<interlevelframe_t>{interlevelframe_t{picname, picnum, "", ResourceId::INVALID_ID, interlevelframe_t::DurationInf, 0, 0}}, std::vector<interlevelcond_t>{cond2}, x, y);
}

void WI_ParseZDoomAnim(OScanner& os, std::vector<interlevelanim_t>& anims, interlevelcond_t cond1 = {}, interlevelcond_t cond2 = {}, bool twoanims = false)
{
	os.mustScanInt();
	int x = os.getTokenInt();
	os.mustScanInt();
	int y = os.getTokenInt();
	os.mustScanInt();
	int duration = os.getTokenInt();
	os.scan();
	bool once = os.compareTokenNoCase("once");
	os.assertTokenNoCaseIs("{");
	os.scan();
	interlevelanim_t anim;
	if (!twoanims && cond2.condition != animcondition_t::None)
		anim = {{}, {cond1, cond2}, x, y};
	else
		anim = {{}, {cond1}, x, y};
	int i = 0;
	while (!os.compareToken("}"))
	{
		if (!os.isIdentifier())
		{
			os.error("Expected identifier, got \"{}\".", os.getToken());
		}
		OLumpName framename = os.getToken();
		ResourceId frameresourceid = Res_GetTextureResourceId(OStringToUpper(framename.c_str()), GRAPHICS);
		interlevelframe_t::frametype_t type = (i == 0 ?
			static_cast<interlevelframe_t::frametype_t>(interlevelframe_t::DurationFixed | interlevelframe_t::RandomStart) :
			interlevelframe_t::DurationFixed);
		anim.frames.emplace_back(framename, frameresourceid, "", ResourceId::INVALID_ID, type, duration, 0);
		if (++i >= 20)
			os.error("More than 20 frames in animation.");
		os.mustScan();
	}
	if (once)
		anim.frames.back().type = interlevelframe_t::DurationInf;
	anims.push_back(anim);
	if ((cond2.condition != animcondition_t::None) && twoanims)
	{
		anim.conditions = {cond2};
		anims.push_back(anim);
	}
}

interlevel_t* WI_GetIntermissionScript(const OLumpName& lumpname)
{
	auto found = interlevelstorage.find(lumpname);
	if (found != interlevelstorage.end())
	{
		return found->second.get();
	}

	const ResourceId script_res_id =
	    Res_GetResourceId(OStringToUpper(lumpname.c_str()), NS_GLOBAL);
	if (!Res_CheckResource(script_res_id))
		return nullptr;

	std::unique_ptr<interlevel_t> output = std::make_unique<interlevel_t>();
	output->layers.emplace_back();
	output->layers.emplace_back();
	output->layers.emplace_back();
	std::vector<interlevelanim_t>& anims = output->layers[0].anims;
	std::vector<interlevelanim_t>& splats = output->layers[1].anims;
	std::vector<interlevelanim_t>& pointers = output->layers[2].anims;
	output->layers[1].conditions.emplace_back(animcondition_t::OnEnteringScreen, 0);
	output->layers[2].conditions.emplace_back(animcondition_t::OnEnteringScreen, 0);
	LevelInfos& levels = getLevelInfos();
	intermissionscript_t intermissionscript{};
	const char* buffer = Res_LoadResource<char>(script_res_id, PU_STATIC);

	const OScannerConfig config = {
	    lumpname, // lumpName
	    false,    // semiComments
	    false,    // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + Res_GetResourceSize(script_res_id));

	while (os.scan())
	{
		if (!os.isIdentifier())
		{
			os.error("Expected identifier, got \"{}\".", os.getToken());
		}

		std::string name = os.getToken();
		if (iequals(name, "noautostartmap"))
		{
			intermissionscript.autostart = false;
		}
		else if (iequals(name, "tilebackground"))
		{
			intermissionscript.tilebackground = true;
		}
		else if (iequals(name, "screensize"))
		{
			os.mustScanInt();
			intermissionscript.screenx = os.getTokenInt();
			os.mustScanInt();
			intermissionscript.screeny = os.getTokenInt();
		}
		else if (iequals(name, "background"))
		{
			os.mustScan(8);
			output->backgroundlump = os.getToken();
		}
		else if (iequals(name, "splat"))
		{
			os.mustScan(8);
			intermissionscript.splat = os.getToken();
			intermissionscript.splatresourceid = Res_GetTextureResourceId(OStringToUpper(intermissionscript.splat.c_str()), GRAPHICS);
		}
		else if (iequals(name, "pointer"))
		{
			os.mustScan(8);
			intermissionscript.ptr1 = os.getToken();
			intermissionscript.ptr1resourceid = Res_GetTextureResourceId(OStringToUpper(intermissionscript.ptr1.c_str()), GRAPHICS);

			os.mustScan(8);
			intermissionscript.ptr2 = os.getToken();
			intermissionscript.ptr2resourceid = Res_GetTextureResourceId(OStringToUpper(intermissionscript.ptr2.c_str()), GRAPHICS);

		}
		else if (iequals(name, "spots"))
		{
			os.mustScan();
			os.assertTokenNoCaseIs("{");
			os.scan();
			while (!os.compareToken("}"))
			{
				OLumpName mapname = os.getToken();
				if (!levels.findByName(mapname).exists())
					os.error("Map {} does not exist", mapname);

				os.mustScanInt();
				int x = os.getTokenInt();
				os.mustScanInt();
				int y = os.getTokenInt();
				intermissionscript.spots.emplace_back(mapname, x, y);
				os.mustScan();
			}
		}
		else if (iequals(name, "pic"))
		{
			WI_ParseZDoomPic(os, anims);
		}
		else if (iequals(name, "animation"))
		{
			WI_ParseZDoomAnim(os, anims);
		}
		else if (iequals(name, "ifentering"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapname});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapname});
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "ifnotentering"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapname}, true);
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapname}, true);
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "ifleaving"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapname});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapname});
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "ifnotleaving"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapname}, true);
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapname}, true);
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "ifvisited"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::MapVisited, mapname});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::MapVisited, mapname});
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "ifnotvisited"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::MapNotVisited, mapname});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::MapNotVisited, mapname});
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "iftraveling"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan(8);
			OLumpName mapname2 = os.getToken();
			if (!levels.findByName(mapname2).exists())
				os.error("Map {} does not exist", mapname2);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::TravelingBetween, mapname, mapname2});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::TravelingBetween, mapname, mapname2});
			else
				os.error("Unknown command {}", os.getToken());
		}
		else if (iequals(name, "ifnottraveling"))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			if (!levels.findByName(mapname).exists())
				os.error("Map {} does not exist", mapname);

			os.mustScan(8);
			OLumpName mapname2 = os.getToken();
			if (!levels.findByName(mapname2).exists())
				os.error("Map {} does not exist", mapname2);

			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::NotTravelingBetween, mapname, mapname2});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::NotTravelingBetween, mapname, mapname2});
			else
				os.error("Unknown command {}", os.getToken());
		}
		else
		{
			os.error("Unknown command {}", name);
		}
	}

	ResourceId tnt1 = Res_GetTextureResourceId("TNT1A0", SPRITE);
	for (const auto& [map, x, y] : intermissionscript.spots)
	{
		splats.emplace_back(
			std::vector<interlevelframe_t>{{intermissionscript.splat, intermissionscript.splatresourceid, "", ResourceId::INVALID_ID, interlevelframe_t::DurationInf, 0, 0}},
			std::vector<interlevelcond_t>{{animcondition_t::MapVisited, map}},
			x, y
		);
		pointers.emplace_back(
			std::vector<interlevelframe_t>{
				{intermissionscript.ptr1, intermissionscript.ptr1resourceid, intermissionscript.ptr2, intermissionscript.ptr2resourceid, interlevelframe_t::DurationFixed, 20, 0},
				{"TNT1A0", tnt1, "", ResourceId::INVALID_ID, interlevelframe_t::DurationFixed, 12, 0}
			},
			std::vector<interlevelcond_t>{{animcondition_t::CurrMapEqual, map}},
			x, y
		);
	}

	interlevel_t* ret = output.get();
	interlevelstorage[lumpname] = std::move(output);

	return ret;
}

void WI_ClearInterlevels()
{
    interlevelstorage.clear();
}
