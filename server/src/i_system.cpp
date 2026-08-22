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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <sstream>
#include <limits>

#include <stdlib.h>
#include <stdarg.h>

#ifdef OSX
#include <mach/clock.h>
#include <mach/mach.h>
#endif

#include "win32inc.h"
#ifdef _WIN32
    #include <conio.h>
    #include <io.h>
    #include <process.h>
    #include <mmsystem.h>
    #include <direct.h> // SoM: I don't know HOW this has been overlooked until now...
	#include <winsock2.h>
#endif

#ifdef UNIX
#include <sys/stat.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <limits.h>
#endif


#include "cmdlib.h"
#include "m_argv.h"

#include "d_main.h"
#include "i_system.h"
#include "i_net.h"
#include "c_dispatch.h"
#include "sv_main.h"
#include "m_consolecommandstream.h"

#ifdef _WIN32
UINT TimerPeriod;
#endif

ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

uint32_t LanguageIDs[4];

//
// I_MegabytesToBytes
//
// Returns the megabyte value of size in bytes
size_t I_MegabytesToBytes (size_t Megabytes)
{
	return (Megabytes*1024*1024);
}

//
// I_BytesToMegabytes
//
// Returns the byte value of size in megabytes
size_t I_BytesToMegabytes (size_t Bytes)
{
	if (!Bytes)
        return 0;

    return (Bytes/1024/1024);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

//
// SubsetLanguageIDs
//
#ifdef _WIN32
static void SubsetLanguageIDs (LCID id, LCTYPE type, int idx)
{
	char buf[8];
	LCID langid;
	char *idp;

	if (!GetLocaleInfo (id, type, buf, 8))
		return;
	langid = MAKELCID (strtoul(buf, NULL, 16), SORT_DEFAULT);
	if (!GetLocaleInfo (langid, LOCALE_SABBREVLANGNAME, buf, 8))
		return;
	idp = (char *)(&LanguageIDs[idx]);
	memset (idp, 0, 4);
	idp[0] = tolower(buf[0]);
	idp[1] = tolower(buf[1]);
	idp[2] = tolower(buf[2]);
	idp[3] = 0;
}
#endif

EXTERN_CVAR(language)

// Force the language to English (default)
void SetLanguageIDs()
{
	uint32_t lang = MAKE_ID('*', '*', '\0', '\0');
	LanguageIDs[0] = lang;
	LanguageIDs[1] = lang;
	LanguageIDs[2] = lang;
	LanguageIDs[3] = lang;
}

//
// I_Init
//
void I_Init (void)
{
}

void I_FinishClockCalibration ()
{
    ///    Printf (PRINT_HIGH, "CPU Frequency: ~%f MHz\n", CyclesPerSecond / 1e6);
}

//
// I_Quit
//
static int has_exited;

void I_Quit()
{
    has_exited = 1;             /* Prevent infinitely recursive exits -- killough */

    #ifdef _WIN32
    timeEndPeriod (TimerPeriod);
    #endif

    G_ClearSnapshots ();
    SV_SendAndFlushDisconnectSignal();

    CloseNetwork ();

	DConsoleAlias::DestroyAll();
}


//
// I_Error
//
bool gameisdead;

void call_terms();

[[noreturn]] void I_BaseError(const std::string& errortext)
{
	throw CRecoverableError(errortext);
}

[[noreturn]] void I_BaseFatalError(const std::string& errortext)
{
	static bool alreadyThrown = false;
	gameisdead = true;

	if (!alreadyThrown) // ignore all but the first message -- killough
	{
		alreadyThrown = true;
		std::string error = errortext;
#ifdef _WIN32
		error += fmt::format("\nGetLastError = {}", GetLastError());
#endif
		throw CFatalError(error);
	}

	if (!has_exited) // If it hasn't exited yet, exit now -- killough
	{
		has_exited = 1; // Prevent infinitely recursive exits -- killough

		call_terms();

		exit(EXIT_FAILURE);
	}
}

#ifdef _WIN32
int ShutdownNow();
#endif

bool I_ConsoleUseColor()
{
#ifdef _WIN32
	static const bool usecolor = []{
		const auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);
		if (GetFileType(hOut) != FILE_TYPE_CHAR)
			return false;

		bool out = true;
		// enable ansi color escape processing
		DWORD consoleMode = 0;
		if (!GetConsoleMode(hOut, &consoleMode))
			throw CDoomError("GetConsoleMode (output) failed!");

		const DWORD originalOutputMode = consoleMode;
		consoleMode |= ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

		if (!SetConsoleMode(hOut, consoleMode))
		{
			out = consoleMode & ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			// we might just be on a version of windows before support was added, try setting mode back
			if (!SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), originalOutputMode))
				throw CDoomError("SetConsoleMode (output) failed!");
		}
		return out;
	}();
#else
	static const bool usecolor = []{
		if (!isatty(STDOUT_FILENO))
			return false;

		if (getenv("NO_COLOR"))
			return false;

		const char* term = getenv("TERM");
		if (!term || strcmp(term, "dumb") == 0)
			return false;

		return true;
	}();
#endif
	return usecolor;
}

std::string I_ConsoleInput()
{
#ifdef _WIN32
    if (ShutdownNow())
        return "quit";
#endif

    return M_ConsoleInput();
}

VERSION_CONTROL (i_system_cpp, "$Id$")
