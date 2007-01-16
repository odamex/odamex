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

#include <new>
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

#define MAX_TERMS	16
void (STACK_ARGS *TermFuncs[MAX_TERMS])(void);
static int NumTerms;

void atterm (void (STACK_ARGS *func)(void))
{
	if (NumTerms == MAX_TERMS)
		I_FatalError ("Too many exit functions registered.\nIncrease MAX_TERMS in i_main.cpp");
	TermFuncs[NumTerms++] = func;
}

void popterm ()
{
	if (NumTerms)
		NumTerms--;
}

static void STACK_ARGS call_terms (void)
{
	while (NumTerms > 0)
	{
		TermFuncs[--NumTerms]();
	}
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
		
		openlog();

		atterm(closelog);
		
		// [Russell] - A basic version string that will eventually get replaced
		//             better than "Odamex SDL Alpha Build 001" or something :P
		
		std::string title = "Odamex - v";
		title += DOTVERSIONSTR;
		
		std::cerr << title << "\n";
		
		if (SDL_Init (SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE) == -1)
		{
			fprintf (errout, "Could not initialize SDL:\n%s\n", SDL_GetError());
			exit(-1);
		}
		atterm (SDL_Quit);
		
		// [Russell] - Update window caption with name
		SDL_WM_SetCaption (title.c_str(), title.c_str());
		
		Args.SetArgs (argc, argv);
		
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


VERSION_CONTROL (i_main_cpp, "$Id:$")

