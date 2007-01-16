// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#ifdef UNIX

#include <sstream>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fnmatch.h>
#include <sys/stat.h>

#ifdef UNIX
#define HAVE_PWD_H
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef OSF1
#define _XOPEN_SOURCE_EXTENDED
#endif
#include <unistd.h>
#ifdef OSF1
#undef _XOPEN_SOURCE_EXTENDED
#endif

#include <stdarg.h>
#include <sys/types.h>
#include <sys/time.h>

#include "errors.h"
#include <math.h>

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

float mb_used = 32.0;

QWORD (*I_GetTime) (void);
QWORD (*I_WaitForTic) (QWORD);

ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
    return &emptycmd;
}

int I_GetHeapSize (void)
{
    return (int)(mb_used*1024*1024);
}

byte *I_ZoneBase (size_t *size)
{
    const char *p = Args.CheckValue ("-heapsize");
    if (p)
                mb_used = (float)atof (p);
    *size = (int)(mb_used*1024*1024);
    return (byte *) malloc (*size);
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte *I_AllocLow(int length)
{
    byte *mem;

    mem = (byte *)malloc (length);
    if (mem) {
                memset (mem,0,length);
    }
    return mem;
}


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
	usleep(1000);
}

void I_WaitVBL (int count)
{
    // I_WaitVBL is never used to actually synchronize to the
    // vertical blank. Instead, it's used for delay purposes.
    usleep (1000000 * count / 70);
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
	
	if(home[home.length() - 1] != '/')
		home += "/";

	return home;
}
#endif

std::string I_GetUserFileName (const char *file)
{
#ifdef UNIX
	std::string path = I_GetHomeDir();

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

	path += "/";
	path += file;
#endif

#ifdef WIN32
	char tmp[MAX_PATH*2];
	std::string path = I_GetBinaryDir();

	if(path[path.length() - 1] != '/')
		path += "/";
	
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

	size_t slash_pos = path.find_first_of('/');
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

				if(segment[segment.length() - 1] != '/')
					segment += "/";
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

	size_t slash = ret.find_last_of('/');
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

    G_ClearSnapshots ();
    SV_SendDisconnectSignal();
    
    CloseNetwork ();
}


//
// I_Error
//
extern FILE *Logfile;
BOOL gameisdead;

#define MAX_ERRORTEXT	1024

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
                va_end (argptr);

                // Record error to log (if logging)
                if (Logfile)
                        fprintf (Logfile, "\n**** DIED WITH FATAL ERROR:\n%s\n", errortext);
                throw CFatalError (errortext);
    }

    if (!has_exited)    // If it hasn't exited yet, exit now -- killough
    {
                has_exited = 1; // Prevent infinitely recursive exits -- killough
                exit(-1);
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

    if (!select(1, &fdr, NULL, NULL, &tv))
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
