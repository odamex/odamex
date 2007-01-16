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
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------


#include <SDL.h>
#include <stdlib.h>

#ifdef OSX
#include <Carbon/Carbon.h>
#endif

#ifdef WIN32
#include <io.h>
#include <direct.h>
#include <process.h>
#include <windows.h>
#endif

#ifdef UNIX
#define HAVE_PWD_H
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include <sstream>

#include <stdarg.h>
#include <sys/types.h>
#include <sys/timeb.h>

#include <sys/stat.h>

#include "hardware.h"
#include "errors.h"
#include <math.h>

#include "doomtype.h"
#include "version.h"
#include "doomdef.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_main.h"
#include "d_net.h"
#include "g_game.h"
#include "i_input.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "cl_main.h"

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
	void *zone;

	const char *p = Args.CheckValue ("-heapsize");
	if (p)
		mb_used = (float)atof (p);
	*size = (size_t)(mb_used*1024*1024);

	while (NULL == (zone = Malloc (*size)) && *size >= 2*1024*1024)
		*size -= 1024*1024;

	return (byte *)zone;
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

	mem = (byte *)Malloc (length);
	if (mem) {
		memset (mem,0,length);
	}
	return mem;
}

// denis - use this unless you want your program
// to get confused every 49 days due to DWORD limit
QWORD I_UnwrapTime(DWORD now32)
{
	static QWORD last = 0;
	QWORD now = now32;

	if(now < last%UINT_MAX)
		last += (UINT_MAX-(last%UINT_MAX)) + now;
	else
		last = now;

	return last;
}

// [RH] Returns time in milliseconds
QWORD I_MSTime (void)
{
   return I_UnwrapTime(SDL_GetTicks());
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

	do
	{
		I_Yield();
	}while ((time = I_GetTimePolled()) <= prevtic);

	return time;
}

void I_Yield(void)
{
	SDL_Delay(1);
}

void I_WaitVBL (int count)
{
	// I_WaitVBL is never used to actually synchronize to the
	// vertical blank. Instead, it's used for delay purposes.
	SDL_Delay (1000 * count / 70);
}


//
// I_Init
//
void I_Init (void)
{
	// TODO: CPU identification here (Does SDL have that? Doesn't look like it does.)
	
	I_GetTime = I_GetTimePolled;
	I_WaitForTic = I_WaitForTicPolled;
	
	I_InitSound ();
	I_InitInput ();
	I_InitHardware ();
}

std::string I_GetCWD ()
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

	if(path[path.length() - 1] != '/')
		path += "/";

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
}

//
// I_Quit
//
static int has_exited;

void STACK_ARGS I_Quit (void)
{
	has_exited = 1;		/* Prevent infinitely recursive exits -- killough */

	G_ClearSnapshots ();
	
	CL_QuitNetGame();

	M_SaveDefaults();
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

	if (!alreadyThrown)		// ignore all but the first message -- killough
	{
		char errortext[MAX_ERRORTEXT];
		int index;
		va_list argptr;
		va_start (argptr, error);
		index = vsprintf (errortext, error, argptr);
		sprintf (errortext + index, "\nSDL_GetError = %s", SDL_GetError());
		va_end (argptr);

		// Record error to log (if logging)
		if (Logfile)
			fprintf (Logfile, "\n**** DIED WITH FATAL ERROR:\n%s\n", errortext);

		throw CFatalError (errortext);
	}

	if (!has_exited)	// If it hasn't exited yet, exit now -- killough
	{
		has_exited = 1;	// Prevent infinitely recursive exits -- killough
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

#ifdef LINUX
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#define X11
#endif

//
// I_GetClipboardText
//
// by Denis Lukianov - 20 Mar 2006
// Cross-platform clipboard functionality
//
std::string I_GetClipboardText (void)
{
#ifdef X11
	std::string ret;
	
	Display *dis = XOpenDisplay(NULL);
	int screen = DefaultScreen(dis);
	
	if(!dis)
	{
		Printf(PRINT_HIGH, "I_GetClipboardText: XOpenDisplay failed");
		return "";
	}
	
	XLockDisplay(dis);
	
	Window WindowEvents = XCreateSimpleWindow(dis, RootWindow(dis, screen), 0, 0, 1, 1, 0, BlackPixel(dis, screen), BlackPixel(dis, screen));

	if(XGetSelectionOwner(dis, XA_PRIMARY) != None)
	{
		if(!XConvertSelection(dis, XA_PRIMARY, XA_STRING, XA_PRIMARY, WindowEvents, CurrentTime))
		{
			XDestroyWindow(dis, WindowEvents);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			Printf(PRINT_HIGH, "I_GetClipboardText: XConvertSelection failed");
			return "";
		}
		
		XFlush (dis);
		
		// Wait for the reply
		for(;;)
		{
			XEvent e;
			XNextEvent(dis, &e);

			if(e.type == SelectionNotify)
				break;
		}
		
		Atom type;
		int format, result;
		u_long len, bytes_left, temp;
		u_char *data;

		result = XGetWindowProperty(dis, WindowEvents, XA_PRIMARY, 0, 0, False, AnyPropertyType, &type, &format, &len, &bytes_left, &data);
		if(result != Success)
		{
			XDestroyWindow(dis, WindowEvents);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			Printf(PRINT_HIGH, "I_GetClipboardText: XGetWindowProperty failed(1)");
			return "";
		}
		
		if(!bytes_left)
		{
			XDestroyWindow(dis, WindowEvents);
			Printf(PRINT_HIGH, "I_GetClipboardText: Len was: %d", len);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			return "";
		}
		
		result = XGetWindowProperty(dis, WindowEvents, XA_PRIMARY, 0, bytes_left, False, AnyPropertyType, &type, &format, &len, &temp, &data);
		if(result != Success)
		{
			XDestroyWindow(dis, WindowEvents);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			Printf(PRINT_HIGH, "I_GetClipboardText: XGetWindowProperty failed(2)");
			return "";
		}
	
		ret = std::string((const char *)data, len);
		XFree(data);
	}

	XDestroyWindow(dis, WindowEvents);
	XUnlockDisplay(dis);
	XCloseDisplay(dis);
	
	return ret;
#endif

#ifdef WIN32
	std::string ret;

	if(!IsClipboardFormatAvailable(CF_TEXT))
		return "";

	if(!OpenClipboard(NULL))
		return "";
	
	HANDLE hClipboardData = GetClipboardData(CF_TEXT);
		
	if(!hClipboardData)
	{
		CloseClipboard();
		return "";
	}
	
	const char *cData = reinterpret_cast<const char *>(GlobalLock(hClipboardData));
	u_int uiSize = static_cast<u_int>(GlobalSize(hClipboardData));
			
	if(cData && uiSize)
	{
		for(size_t i = 0; i < uiSize; i++)
		{
			if(!cData[i])
			{
				uiSize = i;
				break;
			}
		}

		ret = std::string(cData, uiSize);
	}
	
	GlobalUnlock(hClipboardData);

	CloseClipboard();
	
	return ret;
#endif
	
#ifdef OSX
	ScrapRef scrap;
	Size size;
	
	int err = GetCurrentScrap(&scrap);
	
	if(err)
	{
		Printf(PRINT_HIGH, "GetCurrentScrap error: %d", err);
		return "";
	}
	
	err = GetScrapFlavorSize(scrap, FOUR_CHAR_CODE('TEXT'), &size);
	
	if(err)
	{
		Printf(PRINT_HIGH, "GetScrapFlavorSize error: %d", err);
		return "";
	}
	
	char *data = new char[size+1];
	
	err = GetScrapFlavorData(scrap, FOUR_CHAR_CODE('TEXT'), &size, data);
	data[size] = 0;
	
	if(err)
	{
		Printf(PRINT_HIGH, "GetScrapFlavorData error: %d", err);
		delete[] data;
	}
	
	std::string ret(data);
	
	delete[] data;
	
	return ret;
#endif
	
	return "";
}

void I_PrintStr (int xp, const char *cp, int count, BOOL scroll)
{
	// used in the DOS version
}

#ifdef WIN32 // denis - fixme - make this work on POSIX

long I_FindFirst (char *filespec, findstate_t *fileinfo)
{
	//return _findfirst (filespec, fileinfo);
	return 0;
}

int I_FindNext (long handle, findstate_t *fileinfo)
{
	//return _findnext (handle, fileinfo);
	return 0;
}

int I_FindClose (long handle)
{
	//return _findclose (handle);
	return 0;
}

#else

long I_FindFirst (char *filespec, findstate_t *fileinfo) {return 0;}
int I_FindNext (long handle, findstate_t *fileinfo) {return 0;}
int I_FindClose (long handle) {return 0;}

#endif
