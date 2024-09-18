// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2024 by The Odamex Team.
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
//  JSON lump parsing for ID24
//  Adapted from Rum and Raisin Doom m_jsonlump.cpp
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "m_jsonlump.h"

#include "i_system.h"

#include <string>
#include <regex>

constexpr const char* TypeMatchRegexString = "^[a-z0-9_-]+$";
static std::regex TypeMatchRegex = std::regex(TypeMatchRegexString);

constexpr const char* VersionMatchRegexString = "^(\\d+)\\.(\\d+)\\.(\\d+)$";
static std::regex VersionMatchRegex = std::regex(VersionMatchRegexString);

jsonlumpresult_t M_ParseJSONLump(int lumpindex, const char* lumptype, const JSONLumpVersion& maxversion, const JSONLumpFunc& parsefunc)
{
	if(lumpindex < 0 || W_LumpLength(lumpindex) <= 0)
	{
		return jsonlumpresult_t::NOTFOUND;
	}

	const char* jsondata = (const char*)W_CacheLumpNum(lumpindex, PU_STATIC);

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    Json::CharReader* reader = builder.newCharReader();
    Json::Value root;
    std::string errs;

    if(!reader->parse(jsondata, jsondata + W_LumpLength(lumpindex), &root, &errs))
    {
        delete reader;
        I_Error("M_ParseJSONLump: JSON parsing error in lump %s:\n%s", W_LumpName(lumpindex), errs.c_str());
        return jsonlumpresult_t::PARSEERROR;
    }

    delete reader;

	const Json::Value& type     = root["type"];
	const Json::Value& version  = root["version"];
	const Json::Value& metadata = root["metadata"];
	const Json::Value& data     = root["data"];

	std::smatch versionmatch;
	std::string versionstr = version.asString();

	if(root.size() != 4
	   || !type.isString()
	   || !version.isString()
	   || !metadata.isObject()
	   || !data.isObject())
	{
		return jsonlumpresult_t::MALFORMEDROOT;
	}

	if(!std::regex_match(lumptype, TypeMatchRegex)
	   || type.asString() != lumptype)
	{
		return jsonlumpresult_t::TYPEMISMATCH;
	}

	if(!std::regex_search(versionstr, versionmatch, VersionMatchRegex))
	{
		return jsonlumpresult_t::BADVERSIONFORMATTING;
	}

	JSONLumpVersion versiondata =
	{
		stoi(versionmatch[1].str()),
		stoi(versionmatch[2].str()),
		stoi(versionmatch[3].str())
	};

	if(versiondata > maxversion)
	{
		return jsonlumpresult_t::VERSIONMISMATCH;
	}

	return parsefunc(data, versiondata);
}
