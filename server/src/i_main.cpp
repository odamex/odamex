// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------

#include <stack>
#include <iostream>
#include <map>
#include <string>

#include "win32inc.h"
#ifdef _WIN32
    #include "resource.h"
	#include "mmsystem.h"
#endif

#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>

#include "i_crash.h"
#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "c_console.h"
#include "z_zone.h"
#include "errors.h"
#include "i_net.h"
#include "sv_main.h"
#include "m_ostring.h"

using namespace std;

void AddCommandString(std::string cmd);

DArgs Args;

#ifdef _WIN32
extern UINT TimerPeriod;
#endif

// functions to be called at shutdown are stored in this stack
typedef void (STACK_ARGS *term_func_t)(void);
std::stack< std::pair<term_func_t, std::string> > TermFuncs;

void addterm (void (STACK_ARGS *func) (), const char *name)
{
	TermFuncs.push(std::pair<term_func_t, std::string>(func, name));
}

void STACK_ARGS call_terms (void)
{
	while (!TermFuncs.empty())
		TermFuncs.top().first(), TermFuncs.pop();
}

int PrintString(int printlevel, char const* str)
{
	std::string sanitized_str(str);
	StripColorCodes(sanitized_str);

	printf("%s", sanitized_str.c_str());
	fflush(stdout);

	return sanitized_str.length();
}

#ifdef _WIN32
static HANDLE hEvent;

int ShutdownNow()
{
    return (WaitForSingleObject(hEvent, 1) == WAIT_OBJECT_0);
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType)
{
    SetEvent(hEvent);
    return TRUE;
}

int __cdecl main(int argc, char *argv[])
{
	// [AM] Set crash callbacks, so we get something useful from crashes.
	I_SetCrashCallbacks();

    try
    {
        // Handle ctrl-c, close box, shutdown and logoff events
        if (!(hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)))
            throw CDoomError("Could not create console control event!\n");

        if (!SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE))
            throw CDoomError("Could not set console control handler!\n");

        #ifdef _WIN32
        // Fixes icon not showing in titlebar and alt-tab menu under windows 7
        HANDLE hIcon;

        hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));

        if(hIcon)
        {
            SendMessage(GetConsoleWindow(), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(GetConsoleWindow(), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }
        #endif

		// [ML] 2007/9/3: From Eternity (originally chocolate Doom) Thanks SoM & fraggle!
		Args.SetArgs (argc, argv);

		const char *CON_FILE = Args.CheckValue("-confile");
		if(CON_FILE)CON.open(CON_FILE, std::ios::in);

		// Set the timer to be as accurate as possible
		TIMECAPS tc;
		if (timeGetDevCaps (&tc, sizeof(tc) != TIMERR_NOERROR))
			TimerPeriod = 1;	// Assume minimum resolution of 1 ms
		else
			TimerPeriod = tc.wPeriodMin;

		timeBeginPeriod (TimerPeriod);

        // Don't call this on windows!
		//atexit (call_terms);

		Z_Init();

		atterm (I_Quit);
		atterm (DObject::StaticShutdown);

		progdir = I_GetBinaryDir();
		startdir = I_GetCWD();

		C_InitConsole();

		D_DoomMain();
    }
    catch (CDoomError &error)
    {
		if (LOG.is_open())
        {
            LOG << error.GetMsg() << std::endl;
            LOG << std::endl;
        }
        else
        {
            MessageBox(NULL, error.GetMsg().c_str(), "Odasrv Error", MB_OK);
        }

		exit(EXIT_FAILURE);
    }
    catch (...)
    {
		call_terms ();
		throw;
    }
    return 0;
}
#else

// cleanup handling -- killough:
static void handler (int s)
{
    char buf[64];

    signal(s,SIG_IGN);  // Ignore future instances of this signal.

    strcpy(buf,
		   s==SIGSEGV ? "Segmentation Violation" :
		   s==SIGINT  ? "Interrupted by User" :
		   s==SIGILL  ? "Illegal Instruction" :
		   s==SIGFPE  ? "Floating Point Exception" :
		   s==SIGTERM ? "Killed" : "Terminated by signal %s");

    I_FatalError (buf, s);
}

//
// daemon_init
//
void daemon_init(void)
{
    int     pid;
    FILE   *fpid;
    string  pidfile;

    Printf(PRINT_HIGH, "Launched into the background\n");

    if ((pid = fork()) != 0)
    {
    	call_terms();
    	exit(EXIT_SUCCESS);
    }

	const char *forkargs = Args.CheckValue("-fork");
	if (forkargs)
		pidfile = string(forkargs);

    if(!pidfile.size() || pidfile[0] == '-')
    	pidfile = "doomsv.pid";

    pid = getpid();
    fpid = fopen(pidfile.c_str(), "w");
    fprintf(fpid, "%d\n", pid);
    fclose(fpid);
}

int main (int argc, char **argv)
{
	// [AM] Set crash callbacks, so we get something useful from crashes.
	I_SetCrashCallbacks();

    try
    {
		if(!getuid() || !geteuid())
			I_FatalError("root user detected, quitting odamex immediately");

	    seteuid (getuid ());

		Args.SetArgs (argc, argv);

		const char *CON_FILE = Args.CheckValue("-confile");
		if(CON_FILE)CON.open(CON_FILE, std::ios::in);

		/*
		  killough 1/98:

		  This fixes some problems with exit handling
		  during abnormal situations.

		  The old code called I_Quit() to end program,
		  while now I_Quit() is installed as an exit
		  handler and exit() is called to exit, either
		  normally or abnormally. Seg faults are caught
		  and the error handler is used, to prevent
		  being left in graphics mode or having very
		  loud SFX noise because the sound card is
		  left in an unstable state.
		*/

        // Don't use this on other platforms either
		//atexit (call_terms);
		Z_Init();					// 1/18/98 killough: start up memory stuff first

		atterm (I_Quit);
		atterm (DObject::StaticShutdown);

		signal(SIGSEGV, handler);
		signal(SIGTERM, handler);
		signal(SIGILL,  handler);
		signal(SIGFPE,  handler);
		signal(SIGINT,  handler);	// killough 3/6/98: allow CTRL-BRK during init
		signal(SIGABRT, handler);

		progdir = I_GetBinaryDir();

		C_InitConsole();

		D_DoomMain();
    }
    catch (CDoomError &error)
    {
	fprintf (stderr, "%s\n", error.GetMsg().c_str());

	if (LOG.is_open())
        {
            LOG << error.GetMsg() << std::endl;
            LOG << std::endl;
        }

	call_terms();
	exit(EXIT_FAILURE);
    }
    catch (...)
    {
		call_terms ();
		throw;
    }
    return 0;
}

#endif

VERSION_CONTROL (i_main_cpp, "$Id$")

