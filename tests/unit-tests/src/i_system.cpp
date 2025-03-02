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

/* [Russell] - Modified to accomodate a minimal allowable heap size */
// These values are in megabytes
size_t def_heapsize = 64;
const size_t min_heapsize = 8;

// The size we got back from I_ZoneBase in megabytes
size_t got_heapsize = 0;

DWORD LanguageIDs[4];

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

//
// I_ZoneBase
//
// Allocates a portion of system memory for the Zone Memory Allocator, returns
// the 'size' of what it could allocate in its parameter
void *I_ZoneBase (size_t *size)
{
	void *zone = NULL;

	if (def_heapsize < min_heapsize)
		def_heapsize = min_heapsize;

	// Set the size
	*size = I_MegabytesToBytes(def_heapsize);

	// Allocate the def_heapsize, otherwise try to allocate a smaller amount
	while ((zone == NULL) && (*size >= I_MegabytesToBytes(min_heapsize)))
	{
		zone = malloc (*size);

		if (zone != NULL)
			break;

		*size -= I_MegabytesToBytes(1);
	}

	// Our heap size we received
	got_heapsize = I_BytesToMegabytes(*size);

	// Die if the system has insufficient memory
	if (got_heapsize < min_heapsize)
		I_FatalError("I_ZoneBase: Insufficient memory available! Minimum size "
					 "is %lu MB but got %lu MB instead",
					 min_heapsize,
					 got_heapsize);

	return zone;
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

//
// I_GetTime
//
// [SL] Retrieve an arbitrarily-based time from a high-resolution timer with
// nanosecond accuracy.
//
dtime_t I_GetTime()
{
#if defined OSX
	clock_serv_t cclock;
	mach_timespec_t mts;

	host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
	clock_get_time(cclock, &mts);
	mach_port_deallocate(mach_task_self(), cclock);
	return mts.tv_sec * 1000LL * 1000LL * 1000LL + mts.tv_nsec;

#elif defined UNIX
	timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000LL * 1000LL * 1000LL + ts.tv_nsec;

#elif defined WIN32 && !defined _XBOX
	static bool initialized = false;
	static LARGE_INTEGER initial_count;
	static double nanoseconds_per_count;
	static LARGE_INTEGER last_count;

	if (!initialized)
	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		nanoseconds_per_count = 1000.0 * 1000.0 * 1000.0 / double(freq.QuadPart);

        QueryPerformanceCounter(&initial_count);
        last_count = initial_count;

		initialized = true;
	}

	LARGE_INTEGER current_count;
	QueryPerformanceCounter(&current_count);

	// [SL] ensure current_count is a sane value
	// AMD dual-core CPUs and buggy BIOSes sometimes cause QPC
	// to return different values from different CPU cores,
	// which ruins our timing. We check that the new value is
	// at least equal to the last value and that the new value
	// isn't too far past the last value (1 frame at 10fps).

	const int64_t min_count = last_count.QuadPart;
	const int64_t max_count = last_count.QuadPart +
			nanoseconds_per_count * I_ConvertTimeFromMs(100);

	if (current_count.QuadPart < min_count || current_count.QuadPart > max_count)
		current_count = last_count;

	last_count = current_count;

	return nanoseconds_per_count * (current_count.QuadPart - initial_count.QuadPart);
#endif
}

dtime_t I_MSTime()
{
	return I_ConvertTimeToMs(I_GetTime());
}

dtime_t I_ConvertTimeToMs(dtime_t value)
{
	return value / 1000000LL;
}

dtime_t I_ConvertTimeFromMs(dtime_t value)
{
	return value * 1000000LL;
}

//
// I_Sleep
//
// Sleeps for the specified number of nanoseconds, yielding control to the
// operating system. In actuality, the highest resolution availible with
// the select() function is 1 microsecond, but the nanosecond parameter
// is used for consistency with I_GetTime().
//
void I_Sleep(dtime_t sleep_time)
{
#if defined UNIX
	usleep(sleep_time / 1000LL);

#elif defined(WIN32) && !defined(_XBOX)
	Sleep(sleep_time / 1000000LL);

#else
	SDL_Delay(sleep_time / 1000000LL);

#endif
}


//
// I_Yield
//
// Sleeps for 1 millisecond
//
void I_Yield()
{
	I_Sleep(1000LL * 1000LL);		// sleep for 1ms
}

//
// I_WaitVBL
//
// I_WaitVBL is never used to actually synchronize to the
// vertical blank. Instead, it's used for delay purposes.
//
void I_WaitVBL(int count)
{
	I_Sleep(1000000LL * 1000LL * count / 70);
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
