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
//  Adapted from Rum and Raisin Doom m_jsonlump.h
//
//-----------------------------------------------------------------------------


#pragma once

#include "odamex.h"

#include "doomtype.h"
#include "w_wad.h"

#include <functional>

#include <json/json.h>

struct JSONLumpVersion
{
	int32_t major;
	int32_t minor;
	int32_t revision;

	constexpr bool operator > (const JSONLumpVersion& rhs) const
	{
		return major > rhs.major
			|| minor > rhs.minor
			|| revision > rhs.revision;
	}

	constexpr bool operator >= (const JSONLumpVersion& rhs) const
	{
		return major >= rhs.major
			|| minor >= rhs.minor
			|| revision >= rhs.revision;
	}

	constexpr bool operator < (const JSONLumpVersion& rhs) const
	{
		return major < rhs.major
			|| minor < rhs.minor
			|| revision < rhs.revision;
	}

	constexpr bool operator <= (const JSONLumpVersion& rhs) const
	{
		return major <= rhs.major
			|| minor <= rhs.minor
			|| revision <= rhs.revision;
	}
};

enum class jsonlumpresult_t
{
	SUCCESS,
	NOTFOUND,
	MALFORMEDROOT,
	TYPEMISMATCH,
	BADVERSIONFORMATTING,
	VERSIONMISMATCH,
	PARSEERROR
};

static constexpr const char* JSON_LUMP_RESULT_ERROR_STRINGS[] =
{
	"JSON Lump Parsing Successful",
	"JSON Lump Not Found",
	"JSON Lump Malformed Root",
	"JSON Lump Type Mismatch",
	"JSON Lump Bad Version Formatting",
	"JSON Lump Version Mismatch",
	"JSON Lump Parse Error"
};

constexpr const char* M_JSONLumpResultToString(jsonlumpresult_t jlr)
{
	return JSON_LUMP_RESULT_ERROR_STRINGS[static_cast<int>(jlr)];
}

using JSONLumpFunc = std::function<jsonlumpresult_t(const Json::Value& elem, const JSONLumpVersion& version)>;

jsonlumpresult_t M_ParseJSONLump(int lumpindex, const char* lumptype, const JSONLumpVersion& maxversion, const JSONLumpFunc& parsefunc);

inline jsonlumpresult_t M_ParseJSONLump(int lumpindex, const char* lumptype, const JSONLumpVersion& maxversion, JSONLumpFunc&& parsefunc)
{
	return M_ParseJSONLump(lumpindex, lumptype, maxversion, parsefunc);
}

inline jsonlumpresult_t M_ParseJSONLump(const char* lumpname, const char* lumptype, const JSONLumpVersion& maxversion, JSONLumpFunc&& parsefunc)
{
	return M_ParseJSONLump(W_CheckNumForName(lumpname), lumptype, maxversion, parsefunc);
}
