// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
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
//
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------


// denis - todo - remove
#ifdef _WIN32
#include <windows.h>
#undef GetMessage
#endif

#ifdef UNIX
// for getuid and geteuid
#include <unistd.h>
#include <sys/types.h>
#endif

#include <new>
#include <map>
#include <stack>
#include <iostream>

#include <SDL.h>

#include "errors.h"
#include "hardware.h"

#include "doomtype.h"
#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "c_console.h"
#include "z_zone.h"
#include "version.h"
#include "i_video.h"
#include "i_sound.h"

DArgs Args;

extern float mb_used;

// functions to be called at shutdown are stored in this stack
typedef void (STACK_ARGS *term_func_t)(void);
std::stack< std::pair<term_func_t, std::string> > TermFuncs;

void addterm (void (STACK_ARGS *func) (), const char *name)
{
	TermFuncs.push(std::pair<term_func_t, std::string>(func, name));
}

static void STACK_ARGS call_terms (void)
{
	while (!TermFuncs.empty())
		TermFuncs.top().first(), TermFuncs.pop();
}

FILE *errout;

static void openlog(void)
{
#ifdef WIN32
   if(!(errout = fopen("odamex.out", "wb")))
   {
		fprintf (stderr, "Could not open log file for output.");
      exit(-1);
   }
#else
   errout = stderr;
#endif
}


void STACK_ARGS closelog(void)
{
#ifdef WIN32
   if(errout)
      fclose(errout);
#endif
}


int main(int argc, char *argv[])
{
	try
	{
#ifdef UNIX
		if(!getuid() || !geteuid())
			I_FatalError("root user detected, quitting odamex immediately");
#endif

// [ML] 2007/9/3: From Eternity (originally chocolate Doom) Thanks SoM & fraggle!
		Args.SetArgs (argc, argv);
   
#ifdef WIN32
    	// Allow -gdi as a shortcut for using the windib driver.

   		//!
    	// @category video 
    	// @platform windows
    	//
    	// Use the Windows GDI driver instead of DirectX.
    	//

		const char *sdlv = getenv("SDL_VIDEODRIVER");

    	// From the SDL 1.2.10 release notes: 
    	//
    	// > The "windib" video driver is the default now, to prevent 
    	// > problems with certain laptops, 64-bit Windows, and Windows 
    	// > Vista. 
    	//
    	// The hell with that.
   
   		// SoM: the gdi interface is much faster for windowed modes which are more
   		// commonly used. Thus, GDI is default.
     	if (Args.CheckParm ("-directx"))
        	putenv("SDL_VIDEODRIVER=directx");     
    	else if (getenv("SDL_VIDEODRIVER") == NULL || Args.CheckParm ("-gdi") > 0)
        	putenv("SDL_VIDEODRIVER=windib");
#endif
		
		openlog();

		atterm(closelog);
		
		if (SDL_Init (SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) == -1)
		{
			fprintf (errout, "Could not initialize SDL:\n%s\n", SDL_GetError());
			exit(-1);
		}
		atterm (SDL_Quit);
		
		/*
		killough 1/98:
		
		  This fixes some problems with exit handling
		  during abnormal situations.
		  
			The old code called I_Quit() to end program,
			while now I_Quit() is installed as an exit
			handler and exit() is called to exit, either
			normally or abnormally.
		*/
		
		atexit (call_terms);
		Z_Init ();					// 1/18/98 killough: start up memory stuff first

		atterm (I_Quit);
		atterm (DObject::StaticShutdown);
		
		// Figure out what directory the program resides in.
		progdir = I_GetBinaryDir();
		startdir = I_GetCWD();
		
		// init console
		C_InitConsole (80 * 8, 25 * 8, false);
		//Printf (PRINT_HIGH, "Heapsize: %g megabytes\n", mb_used);
		D_DoomMain ();
	}
	catch (CDoomError &error)
	{
		fprintf (errout, "%s\n", error.GetMessage().c_str());
#ifdef WIN32
		MessageBox(NULL, error.GetMessage().c_str(), "Odamex Error", MB_OK);
#endif
		exit (-1);
	}
#ifndef _DEBUG
	catch (...)
	{
		// If an exception is thrown, be sure to do a proper shutdown.
		// This is especially important if we are in fullscreen mode,
		// because the OS will only show the alert box if we are in
		// windowed mode. Graphics gets shut down first in case something
		// goes wrong calling the cleanup functions.
		call_terms ();
		// Now let somebody who understands the exception deal with it.
		throw;
	}
#endif
	return 0;
}


VERSION_CONTROL (i_main_cpp, "$Id$")

