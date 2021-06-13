// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//  A class that "indexes" a string with a unique ID  Mostly used to compress
//  text tokens that are often repeated.
//
//-----------------------------------------------------------------------------

#include "m_strindex.h"

#include "cmdlib.h"

/**
 * @brief Create indexer with default dictionary containing common maplist
 *        tokens.
 */
OStringIndexer OStringIndexer::maplistFactory()
{
	std::string buf;
	OStringIndexer stridx;

	// The big two IWADs.
	const char* IWADS[] = {"DOOM.WAD", "DOOM2.WAD"};
	for (size_t i = 0; i < ARRAY_LENGTH(IWADS); i++)
	{
		stridx.getIndex(IWADS[i]);
	}

	// 36 for Ultimate DOOM.
	for (int e = 1; e <= 4; e++)
	{
		for (int m = 1; m <= 9; m++)
		{
			StrFormat(buf, "E%dM%d", e, m);
			stridx.getIndex(buf);
		}
	}

	// 32 for DOOM II/Final Doom.
	for (int i = 1; i <= 32; i++)
	{
		StrFormat(buf, "MAP%02d", i);
		stridx.getIndex(buf);
	}

	// Any other string/index combinations are up for grabs.
	stridx.setTransmit();

	return stridx;
}
