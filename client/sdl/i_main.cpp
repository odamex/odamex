// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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


#include "odamex.h"

#ifdef UNIX
// for getuid and geteuid
#include <unistd.h>
#include <sys/types.h>
#ifdef __SWITCH__
#include "nx_system.h"
#endif
#endif

#include <new>
#include <stack>
#include <iostream>

#include "i_sdl.h"
#include "i_crash.h"
// [Russell] - Don't need SDLmain library
#ifdef _WIN32
#undef main
#endif // WIN32


#include "m_argv.h"
#include "m_fileio.h"
#include "d_main.h"
#include "i_system.h"
#include "c_console.h"
#include "z_zone.h"

// Use main() on windows for msvc
#if defined(_MSC_VER) && !defined(GCONSOLE)
#    pragma comment(linker, "/subsystem:windows /ENTRY:mainCRTStartup")
#endif

EXTERN_CVAR (r_centerwindow)

DArgs Args;

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

#ifdef __SWITCH__
void STACK_ARGS nx_early_init (void)
{
	socketInitializeDefault();
#ifdef ODAMEX_DEBUG
	nxlinkStdio();
#endif
}
void STACK_ARGS nx_early_deinit (void)
{
	socketExit();
}
#endif


#if defined GCONSOLE && !defined __SWITCH__
int I_Main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	// [AM] Set crash callbacks, so we get something useful from crashes.
#ifdef NDEBUG
	I_SetCrashCallbacks();
#endif

	try
	{

#if defined(__SWITCH__)
		nx_early_init();
		atterm(nx_early_deinit);
#endif

#if defined(UNIX) && !defined(GCONSOLE)
		if(!getuid() || !geteuid())
			I_FatalError("root user detected, quitting odamex immediately");
#endif

		// [ML] 2007/9/3: From Eternity (originally chocolate Doom) Thanks SoM & fraggle!
		::Args.SetArgs(argc, argv);

		if (::Args.CheckParm("--version"))
		{
#ifdef _WIN32
			FILE* fh = fopen("odamex-version.txt", "w");
			if (!fh)
				exit(EXIT_FAILURE);

			const int ok = fprintf(fh, "Odamex %s\n", NiceVersion());
			if (!ok)
				exit(EXIT_FAILURE);

			fclose(fh);
#else
			fmt::print("Odamex {}\n", NiceVersion());
#endif
			exit(EXIT_SUCCESS);
		}

		const char* crashdir = ::Args.CheckValue("-crashdir");
		if (crashdir)
		{
			I_SetCrashDir(crashdir);
		}
		else
		{
			std::string writedir = M_GetWriteDir();
			I_SetCrashDir(writedir.c_str());
		}

		const char* CON_FILE = ::Args.CheckValue("-confile");
		if (CON_FILE)
		{
			CON.open(CON_FILE, std::ios::in);
		}

		// denis - if argv[1] starts with "odamex://"
		if(argc == 2 && argv && argv[1])
		{
			static constexpr std::string_view protocol = "odamex://";
			std::string_view uri = argv[1];

			if(uri.substr(0, protocol.length()) == protocol)
			{
				std::string_view location = uri.substr(protocol.length());
				size_t term = location.find_first_of('/');

				if(term == std::string::npos)
					term = location.length();

				Args.AppendArg("-connect");
				Args.AppendArg(std::string(location.substr(0, term)).c_str());
			}
		}

		unsigned int sdl_flags = SDL_INIT_TIMER;

#ifdef _MSC_VER
		// [SL] per the SDL documentation, SDL's parachute, used to cleanup
		// after a crash, causes the MSVC debugger to be unusable
		sdl_flags |= SDL_INIT_NOPARACHUTE;
#endif

		if (SDL_Init(sdl_flags) == -1)
			I_FatalError("Could not initialize SDL:\n{}\n", SDL_GetError());

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

        // But avoid calling this on windows!
        // Good on some platforms, useless on others
//		#ifndef _WIN32
//		atexit (call_terms);
//		#endif

		atterm (I_Quit);
		atterm (DObject::StaticShutdown);

		D_DoomMain(); // Usually does not return

		// If D_DoomMain does return (as is the case with the +demotest parameter)
		// proper termination needs to occur -- Hyper_Eye
		call_terms ();
	}
	catch (CDoomError &error)
	{
		if (LOG.is_open())
		{
			LOG << "=== ERROR: " << error.GetMsg() << " ===\n\n";
		}

		I_ErrorMessageBox(error.GetMsg().c_str());

		call_terms();
		exit(EXIT_FAILURE);
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
