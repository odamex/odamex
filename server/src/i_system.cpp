// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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

#include <sstream>
#include <limits>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN

#include <conio.h>
#include <io.h>
#include <process.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#include <direct.h> // SoM: I don't know HOW this has been overlooked until now...
#endif

#ifdef UNIX
#define HAVE_PWD_H

#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pwd.h>
#include <unistd.h>
#include <limits.h>

#endif

#include "errors.h"

#include "doomtype.h"
#include "version.h"
#include "doomdef.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "m_misc.h"

#include "d_main.h"
#include "d_net.h"
#include "g_game.h"
#include "i_system.h"
#include "i_net.h"
#include "c_dispatch.h"
#include "sv_main.h"

#ifdef WIN32
UINT TimerPeriod;
#endif

QWORD (*I_GetTime) (void);
QWORD (*I_WaitForTic) (QWORD);

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


#ifdef WIN32
// denis - use this unless you want your program
// to get confused every 49 days due to DWORD limit
QWORD I_UnwrapTime(DWORD now32)
{
	static QWORD last = 0;
	QWORD now = now32;
	static QWORD max = std::numeric_limits<DWORD>::max();

	if(now < last%max)
		last += (max-(last%max)) + now;
	else
		last = now;

	return last;
}

// [RH] Returns time in milliseconds
QWORD I_MSTime (void)
{
	static QWORD basetime = 0;
	QWORD now = I_UnwrapTime(timeGetTime());

	if(!basetime)
		basetime = now;

	return now - basetime;
}
#else
// [RH] Returns time in milliseconds
QWORD I_MSTime (void)
{
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);

	static QWORD basetime = 0;
	static QWORD last = 0;
	QWORD now = (QWORD)(tv.tv_sec)*1000 + (QWORD)(tv.tv_usec)/1000;

	// CPhipps - believe it or not, it is possible with consecutive calls to
	// gettimeofday to receive times out of order, e.g you query the time twice and
	// the second time is earlier than the first. Cheap'n'cheerful fix here.
	// NOTE: only occurs with bad kernel drivers loaded, e.g. pc speaker drv
	if(last > now)
		now = last;
	else
		last = now;

	if (!basetime)
		basetime = now;

	return now;
}
#endif

//
// I_GetTime
// returns time in 1/35th second tics
//
QWORD I_GetTimePolled (void)
{
	return (I_MSTime()*TICRATE)/1000;
}

QWORD I_WaitForTicPolled (QWORD prevtic)
{
    QWORD time;

    while ((time = I_GetTimePolled()) <= prevtic)I_Yield();

    return time;
}

void I_Yield(void)
{
	#ifdef WIN32
	Sleep(1);
	#else
	usleep(1000);
	#endif
}

void I_WaitVBL (int count)
{
    // I_WaitVBL is never used to actually synchronize to the
    // vertical blank. Instead, it's used for delay purposes.
    #ifdef WIN32
    Sleep (1000 * count / 70);
    #else
    usleep (1000000 * count / 70);
    #endif
}

//
// SubsetLanguageIDs
//
#ifdef WIN32
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

//
// SetLanguageIDs
//
static const char *langids[] = {
	"auto",
	"enu",
	"fr",
	"it"
};

EXTERN_CVAR (language)

void SetLanguageIDs ()
{
	unsigned int langid = language.asInt();

	if (langid == 0 || langid > 3)
	{
    #ifdef WIN32
		memset (LanguageIDs, 0, sizeof(LanguageIDs));
		SubsetLanguageIDs (LOCALE_USER_DEFAULT, LOCALE_ILANGUAGE, 0);
		SubsetLanguageIDs (LOCALE_USER_DEFAULT, LOCALE_IDEFAULTLANGUAGE, 1);
		SubsetLanguageIDs (LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, 2);
		SubsetLanguageIDs (LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTLANGUAGE, 3);
    #else
        langid = 1;     // Default to US English on non-windows systems
    #endif
	}
	else
	{
		DWORD lang = 0;
		const char *langtag = langids[langid];

		((BYTE *)&lang)[0] = (langtag)[0];
		((BYTE *)&lang)[1] = (langtag)[1];
		((BYTE *)&lang)[2] = (langtag)[2];
		LanguageIDs[0] = lang;
		LanguageIDs[1] = lang;
		LanguageIDs[2] = lang;
		LanguageIDs[3] = lang;
	}
}

//
// I_Init
//
void I_Init (void)
{
	I_GetTime = I_GetTimePolled;
	I_WaitForTic = I_WaitForTicPolled;
}

std::string I_GetCWD()
{
	char tmp[4096] = {0};
	std::string ret = "./";

	const char *cwd = getcwd(tmp, sizeof(tmp));
	if(cwd)
		ret = cwd;

	FixPathSeparator(ret);

	return ret;
}

#ifdef UNIX
std::string I_GetHomeDir(std::string user = "")
{
	const char *envhome = getenv("HOME");
	std::string home = envhome ? envhome : "";

	if (!home.length())
	{
#ifdef HAVE_PWD_H
		// try the uid way
		passwd *p = user.length() ? getpwnam(user.c_str()) : getpwuid(getuid());
		if(p && p->pw_dir)
			home = p->pw_dir;
#endif

		if (!home.length())
			I_FatalError ("Please set your HOME variable");
	}

	if(home[home.length() - 1] != PATHSEPCHAR)
		home += PATHSEP;

	return home;
}
#endif

std::string I_GetUserFileName (const char *file)
{
#ifdef UNIX
	std::string path = I_GetHomeDir();

	if(path[path.length() - 1] != PATHSEPCHAR)
		path += PATHSEP;

	path += ".odamex";

	struct stat info;
	if (stat (path.c_str(), &info) == -1)
	{
		if (mkdir (path.c_str(), S_IRUSR | S_IWUSR | S_IXUSR) == -1)
		{
			I_FatalError ("Failed to create %s directory:\n%s",
						  path.c_str(), strerror (errno));
		}
	}
	else
	{
		if (!S_ISDIR(info.st_mode))
		{
			I_FatalError ("%s must be a directory", path.c_str());
		}
	}

	path += PATHSEP;
	path += file;
#endif

#ifdef WIN32
	std::string path = I_GetBinaryDir();

	if(path[path.length() - 1] != PATHSEPCHAR)
		path += PATHSEP;

	path += file;
#endif

	return path;
}

void I_ExpandHomeDir (std::string &path)
{
#ifdef UNIX
	if(!path.length())
		return;

	if(path[0] != '~')
		return;

	std::string user;

	size_t slash_pos = path.find_first_of(PATHSEPCHAR);
	size_t end_pos = path.length();

	if(slash_pos == std::string::npos)
		slash_pos = end_pos;

	if(path.length() != 1 && slash_pos != 1)
		user = path.substr(1, slash_pos - 1);

	if(slash_pos != end_pos)
		slash_pos++;

	path = I_GetHomeDir(user) + path.substr(slash_pos, end_pos - slash_pos);
#endif
}

std::string I_GetBinaryDir()
{
	std::string ret;

#ifdef WIN32
	char tmp[MAX_PATH]; // denis - todo - make separate function
	GetModuleFileName (NULL, tmp, sizeof(tmp));
	ret = tmp;
#else
	if(!Args[0])
		return "./";

	char realp[PATH_MAX];
	if(realpath(Args[0], realp))
		ret = realp;
	else
	{
		// search through $PATH
		const char *path = getenv("PATH");
		if(path)
		{
			std::stringstream ss(path);
			std::string segment;

			while(ss)
			{
				std::getline(ss, segment, ':');

				if(!segment.length())
					continue;

				if(segment[segment.length() - 1] != PATHSEPCHAR)
					segment += PATHSEP;
				segment += Args[0];

				if(realpath(segment.c_str(), realp))
				{
					ret = realp;
					break;
				}
			}
		}
	}
#endif

	FixPathSeparator(ret);

	size_t slash = ret.find_last_of(PATHSEPCHAR);
	if(slash == std::string::npos)
		return "";
	else
		return ret.substr(0, slash);
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

    #ifdef WIN32
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

void STACK_ARGS I_FatalError (const char *error, ...)
{
    static BOOL alreadyThrown = false;
    gameisdead = true;

    if (!alreadyThrown)         // ignore all but the first message -- killough
    {
                alreadyThrown = true;
                char errortext[MAX_ERRORTEXT];
                int index;
                va_list argptr;
                va_start (argptr, error);
                index = vsprintf (errortext, error, argptr);
                #ifdef WIN32
                sprintf (errortext + index, "\nGetLastError = %ld", GetLastError());
				#endif
                va_end (argptr);

                throw CFatalError (errortext);
    }

    if (!has_exited)    // If it hasn't exited yet, exit now -- killough
    {
        has_exited = 1; // Prevent infinitely recursive exits -- killough

        call_terms();

        exit(EXIT_FAILURE);
    }
}

void STACK_ARGS I_Error (const char *error, ...)
{
    va_list argptr;
    char errortext[MAX_ERRORTEXT];

    va_start (argptr, error);
    vsprintf (errortext, error, argptr);
    va_end (argptr);

    throw CRecoverableError (errortext);
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
#ifdef WIN32
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
    static char     text[1024] = {0};
    int             len;

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

