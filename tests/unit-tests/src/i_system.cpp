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
	#ifndef _XBOX
		#include <winsock2.h>
	#endif  // !_XBOX
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

void I_BeginRead(void) {}
void I_EndRead(void) {}

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
void I_Init (void) {}

void I_FinishClockCalibration ()
{
    ///    Printf (PRINT_HIGH, "CPU Frequency: ~%f MHz\n", CyclesPerSecond / 1e6);
}

//
// I_Quit
//
static int has_exited;

void STACK_ARGS I_Quit (void) {}


//
// I_Error
//
bool gameisdead;

#define MAX_ERRORTEXT	1024

void I_BaseError(const std::string& errortext)
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

		exit(EXIT_FAILURE);
	}
}

void I_SetTitleString (const char *title) { return; }

void I_PrintStr (int xp, const char *cp, int count, bool scroll)
{
        char string[4096];

        memcpy (string, cp, count);
        if (scroll)
                string[count++] = '\n';
        string[count] = 0;

        fputs (string, stdout);
        fflush (stdout);
}

long I_FindFirst (char *filespec, findstate_t *fileinfo) { return 0; }
int I_FindNext (long handle, findstate_t *fileinfo) { return 0; }
int I_FindClose (long handle) { return 0; }
int I_FindAttr (findstate_t *fileinfo) { return 0; }

std::string I_ConsoleInput (void) { return ""; }

VERSION_CONTROL (i_system_cpp, "$Id$")
