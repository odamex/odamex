// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
#include "oscanner.h"

#include <memory>
#include <unordered_map>

static std::unordered_map<OLumpName, std::unique_ptr<interlevel_t>> interlevelstorage;

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
		output.emplace_back();
		jsonlumpresult_t res = parse(arrayelem, output.back());
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
	output.param1 = param.asInt();
	output.param2 = 0;

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
	output.imagelumpnum = W_CheckNumForName(output.imagelump.c_str());
	if (output.imagelumpnum < 0)
	{
		// TNT1A0 used for transparent by Legacy of Rust
		output.imagelumpnum = W_GetNumForName(output.imagelump.c_str(), ns_sprites);
	}
	output.altimagelump = altimage.asString();
	output.altimagelumpnum = W_GetNumForName(output.altimagelump.c_str());
	output.type = static_cast<interlevelframe_t::frametype_t>(type.asInt());
	output.duration = (int)(duration.asDouble() * TICRATE);
	output.maxduration = (int)(maxduration.asDouble() * TICRATE);

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

interlevel_t* WI_GetInterlevel(const char* lumpname)
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
		I_Error("R_GetInterlevel: Interlevel JSON error in lump %s: %s", lumpname, M_JSONLumpResultToString(result));
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
	int splatnum;
	OLumpName ptr1, ptr2;
	int ptr1num, ptr2num;
	std::vector<std::tuple<OLumpName, int, int>> spots;
};

int ValidateMapName(const OLumpName& mapname)
{
	// Check if the given map name can be expressed as a gameepisode/gamemap pair and be
	// reconstructed from it.
	char lumpname[9];
	int epi = -1, map = -1;

	if (gamemode != commercial)
	{
		if (sscanf(mapname.c_str(), "E%dM%d", &epi, &map) != 2)
			return 0;
		snprintf(lumpname, 9, "E%dM%d", epi, map);
	}
	else
	{
		if (sscanf(mapname.c_str(), "MAP%d", &map) != 1)
			return 0;
		snprintf(lumpname, 9, "MAP%02d", map);
		epi = 1;
	}
	return !strcmp(mapname.c_str(), lumpname);
}

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
	int picnum = W_GetNumForName(picname.c_str());
	if (!twoanims && cond2.condition != animcondition_t::None)
		anims.emplace_back(std::vector<interlevelframe_t>{interlevelframe_t{picname, picnum, "", -1, interlevelframe_t::DurationInf, 0, 0}}, std::vector<interlevelcond_t>{cond1, cond2}, x, y);
	else
		anims.emplace_back(std::vector<interlevelframe_t>{interlevelframe_t{picname, picnum, "", -1, interlevelframe_t::DurationInf, 0, 0}}, std::vector<interlevelcond_t>{cond1}, x, y);
	if ((cond2.condition != animcondition_t::None) && twoanims)
		anims.emplace_back(std::vector<interlevelframe_t>{interlevelframe_t{picname, picnum, "", -1, interlevelframe_t::DurationInf, 0, 0}}, std::vector<interlevelcond_t>{cond2}, x, y);
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
			os.error("Expected identifier, got \"%s\".", os.getToken().c_str());
		}
		OLumpName framename = os.getToken();
		int framenum = W_GetNumForName(framename.c_str());
		interlevelframe_t::frametype_t type = (i == 0 ?
			static_cast<interlevelframe_t::frametype_t>(interlevelframe_t::DurationFixed | interlevelframe_t::RandomStart) :
			interlevelframe_t::DurationFixed);
		anim.frames.emplace_back(framename, framenum, "", -1, type, duration, 0);
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

interlevel_t* WI_GetIntermissionScript(const char* lumpname)
{
	auto found = interlevelstorage.find(lumpname);
	if (found != interlevelstorage.end())
	{
		return found->second.get();
	}

	const int lumpnum = W_CheckNumForName(lumpname);
	if (lumpnum == -1)
		return nullptr;

	std::unique_ptr<interlevel_t> output = std::make_unique<interlevel_t>();
	output->layers.emplace_back();
	output->layers.emplace_back();
	output->layers.emplace_back();
	std::vector<interlevelanim_t>& anims = output->layers[0].anims;
	std::vector<interlevelanim_t>& splats = output->layers[1].anims;
	std::vector<interlevelanim_t>& pointers = output->layers[2].anims;
	output->layers[1].conditions.emplace_back(animcondition_t::OnEnteringScreen, 0, 0);
	output->layers[2].conditions.emplace_back(animcondition_t::OnEnteringScreen, 0, 0);
	LevelInfos& levels = getLevelInfos();
	intermissionscript_t intermissionscript{};
	const char* buffer = static_cast<char*>(W_CacheLumpNum(lumpnum, PU_STATIC));

	const OScannerConfig config = {
	    lumpname, // lumpName
	    false,    // semiComments
	    false,     // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lumpnum));

	while (os.scan())
	{
		if (!os.isIdentifier())
		{
			os.error("Expected identifier, got \"%s\".", os.getToken().c_str());
		}

		std::string name = os.getToken();
		if (!strnicmp(name.c_str(), "noautostartmap", 14))
		{
			intermissionscript.autostart = false;
		}
		else if (!strnicmp(name.c_str(), "tilebackground", 14))
		{
			intermissionscript.tilebackground = true;
		}
		else if (!strnicmp(name.c_str(), "screensize", 13))
		{
			os.mustScanInt();
			intermissionscript.screenx = os.getTokenInt();
			os.mustScanInt();
			intermissionscript.screeny = os.getTokenInt();
		}
		else if (!strnicmp(name.c_str(), "background", 10))
		{
			os.mustScan(8);
			output->backgroundlump = os.getToken();
		}
		else if (!strnicmp(name.c_str(), "splat", 5))
		{
			os.mustScan(8);
			intermissionscript.splat = os.getToken();
			intermissionscript.splatnum = W_GetNumForName(intermissionscript.splat.c_str());
		}
		else if (!strnicmp(name.c_str(), "pointer", 7))
		{
			os.mustScan(8);
			intermissionscript.ptr1 = os.getToken();
			intermissionscript.ptr1num = W_GetNumForName(intermissionscript.ptr1.c_str());

			os.mustScan(8);
			intermissionscript.ptr2 = os.getToken();
			intermissionscript.ptr2num = W_GetNumForName(intermissionscript.ptr2.c_str());

		}
		else if (!strnicmp(name.c_str(), "spots", 5))
		{
			os.mustScan();
			os.assertTokenNoCaseIs("{");
			os.scan();
			while (!os.compareToken("}"))
			{
				OLumpName mapname = os.getToken();
				if (!ValidateMapName(mapname))
					os.error("Invalid map name %s", mapname.c_str());
				os.mustScanInt();
				int x = os.getTokenInt();
				os.mustScanInt();
				int y = os.getTokenInt();
				intermissionscript.spots.emplace_back(mapname, x, y);
				os.mustScan();
			}
		}
		else if (!strnicmp(name.c_str(), "pic", 3))
		{
			WI_ParseZDoomPic(os, anims);
		}
		else if (!strnicmp(name.c_str(), "animation", 9))
		{
			WI_ParseZDoomAnim(os, anims);
		}
		else if (!strnicmp(name.c_str(), "ifentering", 10))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapnum, 0});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapnum, 0});
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "ifnotentering", 13))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapnum, 0}, true);
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapnum, 0}, true);
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "ifleaving", 10))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapnum, 0});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnFinishedScreen, 0, 0}, {animcondition_t::CurrMapEqual, mapnum, 0});
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "ifnotleaving", 13))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapnum, 0}, true);
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::OnEnteringScreen, 0, 0}, {animcondition_t::CurrMapNotEqual, mapnum, 0}, true);
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "ifvisited", 13))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::MapVisited, mapnum, 0});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::MapVisited, mapnum, 0});
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "ifnotvisited", 13))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::MapNotVisited, mapnum, 0});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::MapNotVisited, mapnum, 0});
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "iftraveling", 13))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan(8);
			OLumpName mapname2 = os.getToken();
			int mapnum2 = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::TravelingBetween, mapnum, mapnum2});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::TravelingBetween, mapnum, mapnum2});
			else
				os.error("Unknown command %s", os.getToken());
		}
		else if (!strnicmp(name.c_str(), "ifnottraveling", 13))
		{
			os.mustScan(8);
			OLumpName mapname = os.getToken();
			int mapnum = levels.findByName(mapname).levelnum;
			os.mustScan(8);
			OLumpName mapname2 = os.getToken();
			int mapnum2 = levels.findByName(mapname).levelnum;
			os.mustScan();
			if (os.compareTokenNoCase("animation"))
				WI_ParseZDoomAnim(os, anims, {animcondition_t::NotTravelingBetween, mapnum, mapnum2});
			else if (os.compareTokenNoCase("pic"))
				WI_ParseZDoomPic(os, anims, {animcondition_t::NotTravelingBetween, mapnum, mapnum2});
			else
				os.error("Unknown command %s", os.getToken());
		}
		else
		{
			os.error("Unknown command %s", name.c_str());
		}
	}

	int tnt1 = W_GetNumForName("TNT1A0", ns_sprites);
	// for (const auto& [map, x, y] : intermissionscript.spots) // uncomment for c++17
	for (const auto& spot : intermissionscript.spots)           // delete for c++17
	{
		OLumpName map = std::get<0>(spot);                      // delete for c++17
		int x = std::get<1>(spot);                              // delete for c++17
		int y = std::get<2>(spot);                              // delete for c++17
		int mapnum = levels.findByName(map).levelnum;
		splats.emplace_back(
			std::vector<interlevelframe_t>{{intermissionscript.splat, intermissionscript.splatnum, "", -1, interlevelframe_t::DurationInf, 0, 0}},
			std::vector<interlevelcond_t>{{animcondition_t::MapVisited, mapnum, 0}},
			x, y
		);
		pointers.emplace_back(
			std::vector<interlevelframe_t>{
				{intermissionscript.ptr1, intermissionscript.ptr1num, intermissionscript.ptr2, intermissionscript.ptr2num, interlevelframe_t::DurationFixed, 20, 0},
				{"TNT1A0", tnt1, "", -1, interlevelframe_t::DurationFixed, 12, 0}
			},
			std::vector<interlevelcond_t>{{animcondition_t::CurrMapEqual, mapnum, 0}},
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