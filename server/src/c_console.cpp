// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------


#include <stdarg.h>

#include "m_memio.h"
#include "version.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "v_palette.h"
#include "sv_main.h"
#include "doomstat.h"
#include "gi.h"

#include <string>

static const int MAX_LINE_LENGTH = 8192;

struct History
{
	struct History *Older;
	struct History *Newer;
	char String[1];
};

// CmdLine[0]  = # of chars on command line
// CmdLine[1]  = cursor position
// CmdLine[2+] = command line (max 255 chars + NULL)
// CmdLine[259]= offset from beginning of cmdline to display
//static byte CmdLine[260];

static byte printxormask;

static struct History *HistTail = NULL;

#define PRINTLEVELS 5

EXTERN_CVAR (log_fulltimestamps)

char *TimeStamp()
{
	static char stamp[38];

	time_t ti = time(NULL);
	struct tm *lt = localtime(&ti);

	if(lt)
	{
		if (log_fulltimestamps)
		{
            sprintf (stamp,
                     "[%.2d/%.2d/%.2d %.2d:%.2d:%.2d]",
                     lt->tm_mday,
                     lt->tm_mon + 1,	// localtime returns 0-based month
                     lt->tm_year + 1900,
                     lt->tm_hour,
                     lt->tm_min,
                     lt->tm_sec);
		}
		else
		{
            sprintf (stamp,
                     "[%.2d:%.2d:%.2d]",
                     lt->tm_hour,
                     lt->tm_min,
                     lt->tm_sec);
		}
	}
	else
		*stamp = 0;

	return stamp;
}

/* Provide our own Printf() that is sensitive of the
 * console status (in or out of game)
 */
extern int PrintString (int printlevel, const char *outline);

extern BOOL gameisdead;

int VPrintf(int printlevel, const char* format, va_list parms)
{
	char outline[MAX_LINE_LENGTH];

	if (gameisdead)
		return 0;

	vsnprintf(outline, ARRAY_LENGTH(outline), format, parms);

	// denis - 0x07 is a system beep, which can DoS the console (lol)
	size_t len = strlen(outline);
	for(size_t i = 0; i < len; i++)
		if (outline[i] == 0x07)
			outline[i] = '.';

	std::string str(TimeStamp());
	str.append(" ");
	str.append(outline);

	if (str[str.length() - 1] != '\n')
		str += '\n';

	// send to any rcon players
	for (Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		client_t* cl = &(it->client);

		if (cl->allow_rcon)
		{
			MSG_WriteMarker(&cl->reliablebuf, svc_print);
			MSG_WriteByte(&cl->reliablebuf, PRINT_MEDIUM);
			MSG_WriteString(&cl->reliablebuf, (char*)str.c_str());
		}
	}

	if (LOG.is_open()) {
		LOG << str;
		LOG.flush();
	}

	return PrintString(printlevel, str.c_str());
}

FORMAT_PRINTF(2, 3) int STACK_ARGS Printf(int printlevel, const char* format, ...)
{
	va_list argptr;
	int count;

	va_start (argptr, format);
	count = VPrintf (printlevel, format, argptr);
	va_end (argptr);

	return count;
}

FORMAT_PRINTF(1, 2) int STACK_ARGS Printf_Bold(const char* format, ...)
{
	va_list argptr;
	int count;

	printxormask = 0x80;
	va_start (argptr, format);
	count = VPrintf (PRINT_HIGH, format, argptr);
	va_end (argptr);

	return count;
}

FORMAT_PRINTF(1, 2) int STACK_ARGS DPrintf(const char* format, ...)
{
	va_list argptr;
	int count;

	if (developer || devparm)
	{
		va_start (argptr, format);
		count = VPrintf (PRINT_HIGH, format, argptr);
		va_end (argptr);
		return count;
	}
	else
	{
		return 0;
	}
}

BEGIN_COMMAND (history)
{
	struct History *hist = HistTail;

	while (hist)
	{
		Printf (PRINT_HIGH, "   %s\n", hist->String);
		hist = hist->Newer;
	}
}
END_COMMAND (history)

BEGIN_COMMAND (echo)
{
	if (argc > 1)
	{
		std::string text = C_ArgCombine(argc - 1, (const char **)(argv + 1));
		Printf(PRINT_HIGH, "%s\n", text.c_str());
	}
}
END_COMMAND (echo)

void C_MidPrint (const char *msg, player_t *p, int msgtime)
{
    if (p == NULL)
        return;

    SV_MidPrint(msg, p, msgtime);
}

/****** Tab completion code ******/

typedef std::map<std::string, size_t> tabcommand_map_t; // name, use count
tabcommand_map_t &TabCommands()
{
	static tabcommand_map_t _TabCommands;
	return _TabCommands;
}

void C_AddTabCommand (const char *name)
{
	tabcommand_map_t::iterator i = TabCommands().find(name);

	if(i != TabCommands().end())
		TabCommands()[name]++;
	else
		TabCommands()[name] = 1;
}

void C_RemoveTabCommand (const char *name)
{
	tabcommand_map_t::iterator i = TabCommands().find(name);

	if(i != TabCommands().end())
		if(!--i->second)
			TabCommands().erase(i);
}

VERSION_CONTROL (c_console_cpp, "$Id$")
