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
//	[Odamex] Fitted to work with SDL
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <limits>

#include "i_sdl.h" 
#include <stdlib.h>

#ifdef OSX
	#include <mach/clock.h>
	#include <mach/mach.h>
	#include <Carbon/Carbon.h>
	#include <CoreFoundation/CoreFoundation.h>
#endif

#include "win32inc.h"
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>

#ifdef _XBOX
#include <xtl.h>
#else
#include <shlwapi.h>
#include <winsock2.h>
#include <mmsystem.h>
#include <shlobj.h>
#endif // !_XBOX
#endif // WIN32

#ifdef UNIX
// for getuid and geteuid
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#endif

#include <sstream>

#include <stdarg.h>
#include <sys/types.h>

#ifndef __OpenBSD__
	#include <sys/timeb.h>
#endif

#include <sys/stat.h>


#include "w_wad.h"
#include "cmdlib.h"
#include "m_argv.h"
#include "m_misc.h"
#include "i_video.h"
#include "v_video.h"
#include "i_sound.h"

#include "d_main.h"
#include "g_game.h"
#include "i_system.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "m_fileio.h"

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

EXTERN_CVAR (r_loadicon)
EXTERN_CVAR (r_showendoom)

ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

/* [Russell] - Modified to accomodate a minimal allowable heap size */
// These values are in megabytes
#if defined(GCONSOLE) && !defined(__SWITCH__)
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

void I_BeginRead()
{
	if (r_loadicon)
		I_DrawLoadingIcon();
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

#elif defined UNIX && !defined GEKKO
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

#else
	// [SL] use SDL_GetTicks, but account for the fact that after
	// 49 days, it wraps around since it returns a 32-bit int
	static const uint64_t mask = 0xFFFFFFFFLL;
	static uint64_t last_time = 0LL;
	uint64_t current_time = SDL_GetTicks();

	if (current_time < (last_time & mask))      // just wrapped around
		last_time += mask + 1 - (last_time & mask) + current_time;
	else
		last_time = current_time;

	return last_time * 1000000LL;

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
#if defined _WIN32 && !defined _XBOX
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

void SetLanguageIDs()
{
	const char* langid = language.cstring();

	if (strcmp(langid, "auto") == 0)
	{
#if defined _WIN32 && !defined _XBOX
		memset(LanguageIDs, 0, sizeof(LanguageIDs));
		SubsetLanguageIDs(LOCALE_USER_DEFAULT, LOCALE_ILANGUAGE, 0);
		SubsetLanguageIDs(LOCALE_USER_DEFAULT, LOCALE_IDEFAULTLANGUAGE, 1);
		SubsetLanguageIDs(LOCALE_SYSTEM_DEFAULT, LOCALE_ILANGUAGE, 2);
		SubsetLanguageIDs(LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTLANGUAGE, 3);
#else
		// Default to US English on non-windows systems
		// FIXME: Use SDL Locale support if available.
		langid = "enu";
#endif
	}
	else
	{
		char slang[4] = {'\0', '\0', '\0', '\0'};
		strncpy(slang, langid, ARRAY_LENGTH(slang) - 1);
		uint32_t lang = MAKE_ID(slang[0], slang[1], slang[2], slang[3]);
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
	I_InitSound ();
	I_InitHardware ();
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

    if (!r_showendoom || Args.CheckParm ("-novideo"))
        return;

    // Hack to stop crash with disk icon
    in_endoom = true;

	endoom_data = (unsigned char *)W_CacheLumpName("ENDOOM", PU_STATIC);

	// Set up text mode screen

	TXT_Init();

	I_SetWindowCaption(D_GetTitleString());
    I_SetWindowIcon();

	// Write the data to the screen memory

	screendata = TXT_GetScreenData();

    if(NULL != screendata)
    {
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

	CL_QuitNetGame(NQ_SILENT);

	M_SaveDefaults();

	//I_ShutdownHardware();

    CloseNetwork();

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
	std::string errortext;
	std::string messagetext;

	static BOOL alreadyThrown = false;
	gameisdead = true;

	if (!alreadyThrown) // ignore all but the first message -- killough
	{
		alreadyThrown = true;
		errortext = fmt::sprintf(format, args);
		if (SDL_GetError()[0] != '\0')
		{
			messagetext =
			    fmt::sprintf("%s\nLast SDL Error:\n%s\n", errortext, SDL_GetError());
			SDL_ClearError();
		}
		else
		{
			messagetext = fmt::sprintf("%s\n", errortext);
		}

		throw CFatalError(messagetext);
	}

	if (!has_exited) // If it hasn't exited yet, exit now -- killough
	{
		has_exited = 1; // Prevent infinitely recursive exits -- killough

		call_terms();

		exit(EXIT_FAILURE);
	}

	// Recursive atterm, we've used up all our chances.
	errortext = fmt::sprintf(format, args);
	if (SDL_GetError()[0] != '\0')
	{
		messagetext = fmt::sprintf(
		    "Error while shutting down, aborting:\n%s\nLast SDL Error:\n%s\n", errortext,
		    SDL_GetError());
	}
	else
	{
		messagetext =
		    fmt::sprintf("Error while shutting down, aborting:\n%s\n", errortext);
	}

	I_ErrorMessageBox(messagetext.c_str());

	abort();
}

void I_Error(fmt::CStringRef format, fmt::ArgList args)
{
	std::string errortext = fmt::sprintf(format, args);
	std::string messagetext;

	if (!has_exited)
	{
		throw CRecoverableError(errortext);
	}

	// Recursive atterm, we've used up all our chances.
	if (SDL_GetError()[0] != '\0')
	{
		messagetext = fmt::sprintf(
		    "Error while shutting down, aborting:\n%s\nLast SDL Error:\n%s\n", errortext,
		    SDL_GetError());
	}
	else
	{
		messagetext =
		    fmt::sprintf("Error while shutting down, aborting:\n%s\n", errortext);
	}

	I_ErrorMessageBox(messagetext.c_str());

	abort();
}

void I_Warning(fmt::CStringRef format, fmt::ArgList args)
{
	std::string warningtext = fmt::sprintf(format, args);
	Printf(PRINT_WARNING, "\n%s\n", warningtext);
}

char DoomStartupTitle[256] = { 0 };

void I_SetTitleString (const char *title)
{
	int i;

	for (i = 0; title[i]; i++)
		DoomStartupTitle[i] = title[i] | 0x80;
}


#ifdef X11
	#include <X11/Xlib.h>
	#include <X11/Xatom.h>
#endif

//
// I_GetClipboardText
//
// by Denis Lukianov - 20 Mar 2006
// Cross-platform clipboard functionality
//
std::string I_GetClipboardText()
{
#ifdef X11
	std::string ret;

	Display *dis = XOpenDisplay(NULL);
	int screen = DefaultScreen(dis);

	if (!dis)
	{
		Printf(PRINT_HIGH, "I_GetClipboardText: XOpenDisplay failed");
		return "";
	}

	XLockDisplay(dis);

	Window WindowEvents = XCreateSimpleWindow(dis, RootWindow(dis, screen), 0, 0, 1, 1, 0,
										BlackPixel(dis, screen), BlackPixel(dis, screen));

	if (XGetSelectionOwner(dis, XA_PRIMARY) != None)
	{
		if (!XConvertSelection(dis, XA_PRIMARY, XA_STRING, XA_PRIMARY, WindowEvents, CurrentTime))
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

		result = XGetWindowProperty(dis, WindowEvents, XA_PRIMARY, 0, 0, False, AnyPropertyType,
							&type, &format, &len, &bytes_left, &data);
		if (result != Success)
		{
			XDestroyWindow(dis, WindowEvents);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			Printf(PRINT_HIGH, "I_GetClipboardText: XGetWindowProperty failed(1)");
			return "";
		}

		if (!bytes_left)
		{
			XDestroyWindow(dis, WindowEvents);
			DPrintf("I_GetClipboardText: Len was: %d", len);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			return "";
		}

		result = XGetWindowProperty(dis, WindowEvents, XA_PRIMARY, 0, bytes_left, False, AnyPropertyType,
							&type, &format, &len, &temp, &data);
		if (result != Success)
		{
			XDestroyWindow(dis, WindowEvents);
			XUnlockDisplay(dis);
			XCloseDisplay(dis);
			Printf(PRINT_HIGH, "I_GetClipboardText: XGetWindowProperty failed(2)");
			return "";
		}

		ret = std::string((const char*)data, len);
		XFree(data);
	}

	XDestroyWindow(dis, WindowEvents);
	XUnlockDisplay(dis);
	XCloseDisplay(dis);

	return ret;
#endif

#if defined _WIN32 && !defined _XBOX
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

#if defined(OSX) && (MAC_OS_X_VERSION_MIN_REQUIRED < 1050)
	ScrapRef scrap;
	Size size;

	int err = GetCurrentScrap(&scrap);

	if (err)
	{
		Printf(PRINT_HIGH, "GetCurrentScrap error: %d", err);
		return "";
	}

	err = GetScrapFlavorSize(scrap, FOUR_CHAR_CODE('TEXT'), &size);

	if (err)
	{
		Printf(PRINT_HIGH, "GetScrapFlavorSize error: %d", err);
		return "";
	}

	char* data = new char[size+1];

	err = GetScrapFlavorData(scrap, FOUR_CHAR_CODE('TEXT'), &size, data);
	data[size] = 0;

	if(err)
	{
		Printf(PRINT_HIGH, "GetScrapFlavorData error: %d", err);
		delete[] data;

		return "";
	}

	std::string ret(data);

	delete [] data;

	return ret;
#endif	// OSX < 1050

#ifdef SDL20
    char* textp = SDL_GetClipboardText();

    if(NULL == textp)
    {
        Printf(PRINT_HIGH, "SDL_GetClipboardText error: %s", SDL_GetError());
        return "";
    }

    std::string clipText(textp);
    SDL_free(textp);

	return clipText;
#endif  // SDL20

	return "";
}

void I_PrintStr (int xp, const char *cp, int count, BOOL scroll)
{
	// used in the DOS version
}

#ifdef _WIN32 // denis - fixme - make this work on POSIX

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
#ifdef _WIN32
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


//
// I_IsHeadless
//
// Returns true if no application window will be created.
//
bool I_IsHeadless()
{
	static bool headless;
	static bool initialized = false;
	if (!initialized)
	{
		headless = Args.CheckParm("-novideo") || Args.CheckParm("+demotest");
		initialized = true;
	}

	return headless;
}

const char* ODAMEX_ERROR_TITLE = "Odamex " DOTVERSIONSTR " Fatal Error";

#if defined(SDL20)

void I_ErrorMessageBox(const char* message)
{
	SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, ODAMEX_ERROR_TITLE, message, NULL);
}

#elif defined(WIN32) && !defined(_XBOX)

void I_ErrorMessageBox(const char* message)
{
	MessageBoxA(NULL, message, ODAMEX_ERROR_TITLE, MB_OK);
}

#elif OSX

void I_ErrorMessageBox(const char* message)
{
	CFStringRef macErrorMessage =
	    CFStringCreateWithCString(NULL, message, kCFStringEncodingMacRoman);
	CFUserNotificationDisplayAlert(0, 0, NULL, NULL, NULL, CFSTR(ODAMEX_ERROR_TITLE),
	                               macErrorMessage, CFSTR("OK"), NULL, NULL, NULL);
	CFRelease(macErrorMessage);
}

#else

void I_ErrorMessageBox(const char* message)
{
	fprintf(stderr, "%s\n%s\n", ODAMEX_ERROR_TITLE, message);
}

#endif

#if defined(_DEBUG)

BEGIN_COMMAND(debug_userfilename)
{
	if (argc < 2)
	{
		Printf("debug_userfilename: needs a path to check.\n");
		return;
	}

	std::string userfile = M_GetUserFileName(argv[1]);
	Printf("Resolved to: %s\n", userfile.c_str());
}
END_COMMAND(debug_userfilename)

#endif

VERSION_CONTROL (i_system_cpp, "$Id$")
