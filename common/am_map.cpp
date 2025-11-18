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
//  AutoMap module.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "am_map.h"

#include "w_wad.h"
#include "oscanner.h"

//
static bool ScanAndCompareString(OScanner& os, std::string cmp)
{
	os.scan();
	if (!os.compareToken(cmp.c_str()))
	{
		os.warning("Expected \"{}\", got \"{}\". Aborting parsing", cmp, os.getToken());
		return false;
	}

	return true;
}

//
static bool ScanAndSetRealNum(OScanner& os, fixed64_t& num)
{
	os.scan();
	if (!IsRealNum(os.getToken().c_str()))
	{
		os.warning("Expected number, got \"{}\". Aborting parsing", os.getToken());
		return false;
	}
	num = FLOAT2FIXED64(os.getTokenFloat());

	return true;
}

// Scans through and interprets a file of lines
nonstd::expected<std::vector<mline_t>, am_lump_parse_error_t> AM_ParseVectorLump(const OLumpName& name)
{
	const int lump = W_CheckNumForName(name, 0);
	if (lump == -1)
		return nonstd::make_unexpected(am_lump_parse_error_t::LUMP_NOT_FOUND);


	const char* buffer = static_cast<char*>(W_CacheLumpNum(lump, PU_STATIC));

	const OScannerConfig config = {
	    name,  // lumpName
	    false, // semiComments
	    true,  // cComments
	};
	OScanner os = OScanner::openBuffer(config, buffer, buffer + W_LumpLength(lump));

	std::vector<mline_t> lines;

	while (os.scan())
	{
		os.unScan();
		mline_t ml;

		if (!ScanAndCompareString(os, "(")) break;
		if (!ScanAndSetRealNum(os, ml.a.x)) break;
		if (!ScanAndCompareString(os, ",")) break;
		if (!ScanAndSetRealNum(os, ml.a.y)) break;
		if (!ScanAndCompareString(os, ")")) break;
		if (!ScanAndCompareString(os, ",")) break;
		if (!ScanAndCompareString(os, "(")) break;
		if (!ScanAndSetRealNum(os, ml.b.x)) break;
		if (!ScanAndCompareString(os, ",")) break;
		if (!ScanAndSetRealNum(os, ml.b.y)) break;
		if (!ScanAndCompareString(os, ")")) break;

		lines.push_back(ml);
	}

	return lines;
}