// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2011 by Randy Heit (ZDoom 1.22).
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
//	GSTRINGS Define
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "gstrings.h"

#include "c_dispatch.h"

// Localizable strings
StringTable	GStrings;

static void StringinfoHelp()
{
	Printf(PRINT_HIGH,
		"stringinfo - Looks up internal information about strings\n\n"
		"Usage:\n"
		"  ] stringinfo name <STRINGNAME>\n"
		"  Looks up a string by name STRINGNAME.\n\n"
		"  ] stringinfo index <INDEX>\n"
		"  Looks up a string by index INDEX.\n\n"
		"  ] stringinfo size\n"
		"  Return the size of the internal stringtable.\n\n"
		"  ] stringinfo dump\n"
		"  Dumps all strings in the stringtable.  Sometimes a blunt instrument is appropriate.\n");
}

BEGIN_COMMAND(stringinfo)
{
	if (argc < 2)
	{
		StringinfoHelp();
		return;
	}

	if (stricmp(argv[1], "size") == 0)
	{
		Printf("%zu strings found\n", GStrings.size());
		return;
	}
	else if (stricmp(argv[1], "dump") == 0)
	{
		GStrings.dumpStrings();
		return;
	}

	if (argc < 3)
	{
		StringinfoHelp();
		return;
	}

	if (stricmp(argv[1], "name") == 0)
	{
		Printf(PRINT_HIGH, "%s = \"%s\"\n", argv[2], GStrings(argv[2]));
		return;
	}
	else if (stricmp(argv[1], "index") == 0)
	{
		int index = atoi(argv[2]);
		Printf(PRINT_HIGH, "%s = \"%s\"\n", argv[2], GStrings.getIndex(index));
		return;
	}

	StringinfoHelp();
}
END_COMMAND(stringinfo)
