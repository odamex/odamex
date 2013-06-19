// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

#include <limits>

#include <SDL.h>
#include <stdlib.h>

#ifdef OSX
#include <Carbon/Carbon.h>
#endif

#include "win32inc.h"
#ifdef WIN32
    #include <io.h>
    #include <direct.h>
    #include <process.h>

    #ifdef _XBOX
        #include <xtl.h>
    #else
        #include <shlwapi.h>
    #endif // !_XBOX
#endif // WIN32

#ifdef UNIX
#define HAVE_PWD_H
// for getuid and geteuid
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
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
#include "w_wad.h"
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

#ifdef _XBOX
#include "i_xbox.h"
#endif

#ifdef GEKKO
#include "i_wii.h"
#endif

#ifndef GCONSOLE // I will add this back later -- Hyper_Eye
// For libtextscreen to link properly
extern "C"
{
#include "txt_main.h"
}
#define ENDOOM_W 80
#define ENDOOM_H 25
#endif // _XBOX

EXTERN_CVAR (r_showendoom)

QWORD (*I_GetTime) (void);

QWORD tic_start_time = 0;

ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

/* [Russell] - Modified to accomodate a minimal allowable heap size */
// These values are in megabytes
#ifdef GCONSOLE
size_t def_heapsize = 16;
#else
size_t def_heapsize = 128;
#endif
const size_t min_heapsize = 8;

// The size we got back from I_ZoneBase in megabytes
size_t got_heapsize = 0;

DWORD LanguageIDs[4];

// Endoom screen is showing
bool in_endoom = false;

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
	// NOTE(jsd): This is called before V_Palette is set causing crash in 32bpp mode.
#if 0
	patch_t *diskpatch = W_CachePatch("STDISK");

	if (!screen || !diskpatch || in_endoom)
		return;

	screen->Lock();

	int scale = MIN(CleanXfac, CleanYfac);
	int w = diskpatch->width() * scale;
	int h = diskpatch->height() * scale;
	// offset x and y for the lower right corner of the screen
	int ofsx = screen->width - w + (scale * diskpatch->leftoffset());
	int ofsy = screen->height - h + (scale * diskpatch->topoffset());

	screen->DrawPatchStretched(diskpatch, ofsx, ofsy, w, h);

	screen->Unlock();
#endif
}

void I_EndRead(void)
{
}

void I_StartTicTimer()
{
	tic_start_time = I_MSTime();
}

void I_Sleep(int milliseconds)
{
	SDL_Delay(milliseconds);
}

void I_SleepUntilNextTic()
{
	QWORD current_time = I_MSTime();
	QWORD next_tic_start_time = tic_start_time + 1000 / TICRATE;

	if (current_time < next_tic_start_time)
		I_Sleep(next_tic_start_time - current_time);
}


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

void I_WaitVBL (int count)
{
	// I_WaitVBL is never used to actually synchronize to the
	// vertical blank. Instead, it's used for delay purposes.
	SDL_Delay (1000 * count / 70);
}

//
// SubsetLanguageIDs
//
#if defined WIN32 && !defined _XBOX
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
    #if defined WIN32 && !defined _XBOX
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

	I_InitSound ();
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

#if defined(UNIX) && !defined(GEKKO)
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
#if defined(UNIX) && !defined(GEKKO)
	// return absolute or explicitly relative pathnames unmodified,
	// so launchers or CLI/console users have control over netdemo placement
	if (file &&
		(file[0] == PATHSEPCHAR || // /path/to/file
		(file[0] == '.' && file[1] == PATHSEPCHAR) || // ./file
		(file[0] == '.' && file[1] == '.' && file[2] == PATHSEPCHAR))) // ../file
		return std::string (file);

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
#elif defined(_XBOX)
	std::string path = "T:";

	path += PATHSEP;
	path += file;
#else
	if (!PathIsRelative(file))
		return std::string (file);

	std::string path = I_GetBinaryDir();

	if(path[path.length() - 1] != PATHSEPCHAR)
		path += PATHSEP;

	path += file;
#endif

	FixPathSeparator(path);

	return path;
}

void I_ExpandHomeDir (std::string &path)
{
#if defined(UNIX) && !defined(GEKKO)
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

#ifdef _XBOX
	// D:\ always corresponds to the binary path whether running from DVD or HDD.
	ret = "D:\\";
#elif defined GEKKO
	ret = "sd:/";
#elif defined WIN32
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
}

//
// Displays the text mode ending screen after the game quits
//

void I_Endoom(void)
{
#ifndef GCONSOLE // I will return to this -- Hyper_Eye
	unsigned char *endoom_data;
	unsigned char *screendata;
	int y;
	int indent;

    // Hack to stop crash with disk icon
    in_endoom = true;

	endoom_data = (unsigned char *)W_CacheLumpName("ENDOOM", PU_STATIC);

	// Set up text mode screen

	TXT_Init();

    I_SetWindowCaption();
    I_SetWindowIcon();

	// Write the data to the screen memory

	screendata = TXT_GetScreenData();

	indent = (ENDOOM_W - TXT_SCREEN_W) / 2;

	for (y=0; y<TXT_SCREEN_H; ++y)
	{
		memcpy(screendata + (y * TXT_SCREEN_W * 2),
				endoom_data + (y * ENDOOM_W + indent) * 2,
				TXT_SCREEN_W * 2);
	}

	// Wait for a keypress
	while (true)
	{
		TXT_UpdateScreen();

		if (TXT_GetChar() > 0)
            break;

        TXT_Sleep(0);
	}

	// Shut down text mode screen

	TXT_Shutdown();

	in_endoom = false;
#endif // Hyper_Eye
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

	//I_ShutdownHardware();

    CloseNetwork();

	DConsoleAlias::DestroyAll();

	try
	{
		if (r_showendoom && !Args.CheckParm ("-novideo"))
			I_Endoom();
	}
	catch (CRecoverableError &error)
	{
		// [AM] ENDOOM does not exist, but at this point we don't care.
	}
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

	if (!alreadyThrown)		// ignore all but the first message -- killough
	{
		char errortext[MAX_ERRORTEXT];
		int index;
		va_list argptr;
		va_start (argptr, error);
		index = vsprintf (errortext, error, argptr);
		sprintf (errortext + index, "\nSDL_GetError = \"%s\"", SDL_GetError());
		va_end (argptr);

		throw CFatalError (errortext);
	}

	if (!has_exited)	// If it hasn't exited yet, exit now -- killough
	{
		has_exited = 1;	// Prevent infinitely recursive exits -- killough

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

void STACK_ARGS I_Warning(const char *warning, ...)
{
	va_list argptr;
	char warningtext[MAX_ERRORTEXT];

	va_start (argptr, warning);
	vsprintf (warningtext, warning, argptr);
	va_end (argptr);

	Printf_Bold ("\n%s\n", warningtext);
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
			DPrintf("I_GetClipboardText: Len was: %d", len);
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

#if defined WIN32 && !defined _XBOX
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
	SIZE_T uiSize = static_cast<SIZE_T>(GlobalSize(hClipboardData));

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

		return "";
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

//
// I_ConsoleInput
//
#ifdef WIN32
std::string I_ConsoleInput (void)
{
	// denis - todo - implement this properly!!!
	/* denis - this probably won't work for a gui sdl app. if it does work, please uncomment!
    static char     text[1024] = {0};
    static char     buffer[1024] = {0};
    unsigned int    len = strlen(buffer);

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

	if(len && buffer[len - 1] == '\n' || buffer[len - 1] == '\r')
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
*/
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

