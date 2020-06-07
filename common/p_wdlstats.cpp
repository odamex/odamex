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
//     Defines needed by the WDL stats logger.
//
//-----------------------------------------------------------------------------

#include "p_wdlstats.h"

#include <string>
#include <vector>

#include "c_dispatch.h"

// A single event.
struct WDLEvent
{
	WDLEvents ev;
	int arg0;
	int arg1;
	int arg2;
};

// Events that we're keeping track of.
static std::vector<WDLEvent> events;

// WDL Stats dir - if not empty, we are logging.
static std::string wdlstatdir;

// Generate a log filename based on the current time.
static std::string GenerateLogFilename()
{
	time_t ti = time(NULL);
	struct tm *lt = localtime(&ti);

	char buffer[128];
	if (!strftime(buffer, ARRAY_LENGTH(buffer), "wdl_%Y.%m.%d.%H.%M.%S.log", lt))
		return "";

	return std::string(buffer, ARRAY_LENGTH(buffer));
}

static void WDLStatsHelp()
{
	Printf(PRINT_HIGH,
		"wdlstats - Starts logging WDL statistics to the given directory.  Unless "
		"you are running a WDL server, you probably are not interested in this.\n\n"
		"Usage:\n"
		"  ] wdlstats <DIRNAME>\n"
		"  Starts logging WDL statistics in the directory DIRNAME.\n");
}

BEGIN_COMMAND(wdlstats)
{
	if (argc < 2)
	{
		WDLStatsHelp();
		return;
	}
}
END_COMMAND(wdlstats)

void P_StartWDLLog()
{
	if (::wdlstatdir.empty())
		return;

	// TODO: Clear the globals.
}

void P_LogWDLEvent(WDLEvents event, int arg0, int arg1, int arg2)
{
	if (::wdlstatdir.empty())
		return;

	WDLEvent ev = { event, arg0, arg1, arg2 };
	::events.push_back(ev);
}

void P_CommitWDLLog()
{
	if (::wdlstatdir.empty())
		return;

	GenerateLogFilename();

	// TODO: Actually write the log.
}
