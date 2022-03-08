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
#include "sv_main.h"

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

	// User wanted a different default size
	const char *p = Args.CheckValue ("-heapsize");

	if (p)
		def_heapsize = atoi(p);

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

void STACK_ARGS I_Quit (void)
{
    has_exited = 1;             /* Prevent infinitely recursive exits -- killough */

    #ifdef _WIN32
    timeEndPeriod (TimerPeriod);
    #endif

    G_ClearSnapshots ();
    SV_SendDisconnectSignal();

    CloseNetwork ();

	DConsoleAlias::DestroyAll();
}


//
// I_Error
//
BOOL gameisdead;

#define MAX_ERRORTEXT	1024

void STACK_ARGS call_terms (void);

NORETURN void I_FatalError(fmt::CStringRef format, fmt::ArgList args)
{
	static BOOL alreadyThrown = false;
	gameisdead = true;

	if (!alreadyThrown) // ignore all but the first message -- killough
	{
		alreadyThrown = true;
		std::string errortext = fmt::sprintf(format, args);
#ifdef _WIN32
		errortext += fmt::format("\nGetLastError = {}", GetLastError());
#endif
		throw CFatalError(errortext);
	}

	if (!has_exited) // If it hasn't exited yet, exit now -- killough
	{
		has_exited = 1; // Prevent infinitely recursive exits -- killough

		call_terms();

		exit(EXIT_FAILURE);
	}
}

void I_Error(fmt::CStringRef format, fmt::ArgList args)
{
	const std::string errortext = fmt::sprintf(format, args);
	throw CRecoverableError(errortext);
}

char DoomStartupTitle[256] = { 0 };

void I_SetTitleString (const char *title)
{
    int i;

    for (i = 0; title[i]; i++)
                DoomStartupTitle[i] = title[i] | 0x80;
}

void I_PrintStr (int xp, const char *cp, int count, BOOL scroll)
{
        char string[4096];

        memcpy (string, cp, count);
        if (scroll)
                string[count++] = '\n';
        string[count] = 0;

        fputs (string, stdout);
        fflush (stdout);
}

//static const char *pattern; // [DL] todo - remove
//static findstate_t *findstates[8]; // [DL] todo - remove

long I_FindFirst (char *filespec, findstate_t *fileinfo)
{
    return 0;
}

int I_FindNext (long handle, findstate_t *fileinfo)
{
    return 0;
}

int I_FindClose (long handle)
{
    return 0;
}

int I_FindAttr (findstate_t *fileinfo)
{
    return 0;
}

//
// I_ConsoleInput
//
#ifdef _WIN32
int ShutdownNow();

std::string I_ConsoleInput (void)
{
	// denis - todo - implement this properly!!!
    static char     text[1024] = {0};
    static char     buffer[1024] = {0};
    unsigned int    len = strlen(buffer);

    if (ShutdownNow())
        return "quit";

	while(kbhit() && len < sizeof(text))
	{
		char ch = (char)getch();

		// input the character
		if(ch == '\b' && len)
		{
			buffer[--len] = 0;
			// john - backspace hack
			fwrite(&ch, 1, 1, stdout);
			ch = ' ';
			fwrite(&ch, 1, 1, stdout);
			ch = '\b';
		}
		else
			buffer[len++] = ch;
		buffer[len] = 0;

		// recalculate length
		len = strlen(buffer);

		// echo character back to user
		fwrite(&ch, 1, 1, stdout);
		fflush(stdout);
	}

	if(len && (buffer[len - 1] == '\n' || buffer[len - 1] == '\r'))
	{
		// echo newline back to user
		char ch = '\n';
		fwrite(&ch, 1, 1, stdout);
		fflush(stdout);

		strcpy(text, buffer);
		text[len-1] = 0; // rip off the /n and terminate
		buffer[0] = 0;
		len = 0;

		return text;
	}

	return "";
}

#else

std::string I_ConsoleInput (void)
{
	std::string ret;
	static char	 text[1024] = {0};
	int			 len;

	fd_set fdr;
	FD_ZERO(&fdr);
	FD_SET(0, &fdr);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if (select(1, &fdr, NULL, NULL, &tv) <= 0)
		return "";

	len = read (0, text + strlen(text), sizeof(text) - strlen(text)); // denis - fixme - make it read until the next linebreak instead

	if (len < 1)
		return "";

	len = strlen(text);

	if (strlen(text) >= sizeof(text))
	{
		if(text[len-1] == '\n' || text[len-1] == '\r')
			text[len-1] = 0; // rip off the /n and terminate

		ret = text;
		memset(text, 0, sizeof(text));
		return ret;
	}

	if(text[len-1] == '\n' || text[len-1] == '\r')
	{
		text[len-1] = 0;

		ret = text;
		memset(text, 0, sizeof(text));
		return ret;
	}

	return "";
}
#endif

VERSION_CONTROL (i_system_cpp, "$Id$")
