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
//		Intermission screens.
//
//-----------------------------------------------------------------------------

#include "wi_interlevel.h"

#include "i_system.h"
#include "m_jsonlump.h"

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
	output.param = param.asInt();

	if (output.condition < animcondition_t::None || output.condition >= animcondition_t::Max)
	{
		return jsonlumpresult_t::PARSEERROR;
	}

	return jsonlumpresult_t::SUCCESS;
}

jsonlumpresult_t WI_ParseInterlevelFrame(const Json::Value& frame, interlevelframe_t& output)
{
    const Json::Value& image = frame["image"];
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
	output.type = static_cast<interlevelframe_t::frametype_t>(type.asInt());
	output.duration = duration.asDouble() * TICRATE;
	output.maxduration = maxduration.asDouble() * TICRATE;
	// output.lumpname_animindex = 0;
	// output.lumpname_animframe = 0;

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
		output->music_lump = music.asString();
		output->background_lump = backgroundimage.asString();
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

void WI_ClearInterlevels()
{
    interlevelstorage.clear();
}