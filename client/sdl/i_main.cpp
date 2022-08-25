// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
// Copyright (C) 2022-2022 by DoomBattle.Zone.
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

// denis - todo - remove
#include "win32inc.h"
#ifdef _WIN32
    #ifndef _XBOX
        #undef GetMessage
        typedef BOOL (WINAPI *SetAffinityFunc)(HANDLE hProcess, DWORD mask);
    #endif // !_XBOX
#else
    #include <sched.h>
#endif // WIN32

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

#ifdef _XBOX
#include "i_xbox.h"
#endif

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

std::string parse_uri(char const* uri, char const* protocol, bool return_ticket = false)
{
	std::string location = uri + strlen(protocol);

	// check and parse query parameters
	size_t query_start = location.find_first_of('?');
	if (query_start != std::string::npos)
	{
		std::string parameters = location.substr(query_start + 1);

		if (!parameters.empty())
		{
			size_t param_start = 0;
			size_t param_end = 0;

			do
			{
				param_end = parameters.find_first_of('&', param_start);

				std::string parameter = parameters.substr(param_start, param_end - param_start);
				size_t key_end = parameter.find_first_of('=');

				std::string key = parameter.substr(0, key_end);
				std::string value = key_end != std::string::npos ? parameter.substr(key_end + 1) : "";

				if (!key.empty())
				{
					char const first_char = key.at(0);
					bool const has_qualifier = first_char == '-' || first_char == '+' || first_char == '@';

					Args.AppendArg(has_qualifier ? key.c_str() : ("-" + key).c_str());
				}

				if (!value.empty())
				{
					Args.AppendArg(value.c_str());
					if (return_ticket && key == "ticket")
						return value;
				}

				param_start = param_end + 1;
			} while (param_end != std::string::npos);
		}

		location = location.substr(0, query_start);
	}


	// if no ticket found, return empty string
	if (return_ticket)
		return "";

	size_t term = location.find_first_of('/');

	if (term == std::string::npos)
		term = location.length();

	return location.substr(0, term);
}

std::string parse_scheme(int argc, char* argv[], const char* protocol)
{
	if (argc == 2 && argv && argv[1])
	{
		const char* uri = argv[1];

		if (uri && strncmp(uri, protocol, strlen(protocol)) == 0)
			return parse_uri(uri, protocol);
	}

	return std::string();
}

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
			printf("Odamex %s\n", NiceVersion());
#endif
			exit(EXIT_SUCCESS);
		}

		// denis - if argv[1] starts with "odamex://"
		std::string host = parse_scheme(argc, argv, "odamex://");
		if (!host.empty())
		{
			Args.AppendArg("-connect");
			Args.AppendArg(host.c_str());
		}

		// doombattlezone://doombattle.zone/play/?ticket=<ticket>
		char const* dbz_protocol = "doombattlezone://";
		host = parse_scheme(argc, argv, dbz_protocol);
		if (!host.empty())
		{
			Args.AppendArg("-battle");
			Args.AppendArg(argv[1]);
		}
		else
		{
			char const* battle_value = Args.CheckValue("-battle");
			if (battle_value)
			{
				if (strncmp(battle_value, dbz_protocol, strlen(dbz_protocol)) == 0)
					parse_uri(battle_value, dbz_protocol);
				else
					throw CDoomError("Invalid scheme for battle");
			}
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

#if defined(SDL12)
        // [Russell] - No more double-tapping of capslock to enable autorun
        SDL_putenv((char*)"SDL_DISABLE_LOCK_KEYS=1");

		// Set SDL video centering
		SDL_putenv((char*)"SDL_VIDEO_WINDOW_POS=center");
		SDL_putenv((char*)"SDL_VIDEO_CENTERED=1");
#endif

#if defined _WIN32 && !defined _XBOX

	#if defined(SDL12)
    	// From the SDL 1.2.10 release notes:
    	//
    	// > The "windib" video driver is the default now, to prevent
    	// > problems with certain laptops, 64-bit Windows, and Windows
    	// > Vista.
    	//
    	// The hell with that.

   		// SoM: the gdi interface is much faster for windowed modes which are more
   		// commonly used. Thus, GDI is default.
		//
		// GDI mouse issues fill many users with great sadness. We are going back
		// to directx as defulat for now and the people will rejoice. --Hyper_Eye
     	if (Args.CheckParm ("-gdi"))
        	putenv((char*)"SDL_VIDEODRIVER=windib");
    	else
        	putenv((char*)"SDL_VIDEODRIVER=directx");
	#endif	// SDL12

	
	#if defined(SDL20)
        // FIXME: Remove this when SDL gets it shit together, see 
        // https://bugzilla.libsdl.org/show_bug.cgi?id=2089
        // ...
        // Disable thread naming on windows, with SDL 2.0.5 and GDB > 7.8.1
        // RaiseException will be thrown and will crash under the debugger with symbols
        // loaded or not
        SDL_SetHint(SDL_HINT_WINDOWS_DISABLE_THREAD_NAMING, "1");
	#endif // SDL20

        // Set the process affinity mask to 1 on Windows, so that all threads
        // run on the same processor.  This is a workaround for a bug in
        // SDL_mixer that causes occasional crashes.  Thanks to entryway and fraggle for this.
        //
        // [ML] 8/6/10: Updated to match prboom+'s I_SetAffinityMask.  We don't do everything
        // you might find in there but we do enough for now.
        HMODULE kernel32_dll = LoadLibrary("kernel32.dll");

        if (kernel32_dll)
        {
            SetAffinityFunc SetAffinity = (SetAffinityFunc)GetProcAddress(kernel32_dll, "SetProcessAffinityMask");

            if (SetAffinity)
            {
                if (!SetAffinity(GetCurrentProcess(), 1))
                    LOG << "Failed to set process affinity mask: " << GetLastError() << std::endl;
            }
        }
#endif	// _WIN32 && !_XBOX

#ifdef X11
	#if defined(SDL12)
		// [SL] 2011-12-21 - Ensure we're getting raw DGA mouse input from X11,
		// bypassing X11's mouse acceleration
		putenv((char*)"SDL_VIDEO_X11_DGAMOUSE=1");
	#endif	// SDL12
#endif	// X11

		unsigned int sdl_flags = SDL_INIT_TIMER;

#ifdef _MSC_VER
		// [SL] per the SDL documentation, SDL's parachute, used to cleanup
		// after a crash, causes the MSVC debugger to be unusable
		sdl_flags |= SDL_INIT_NOPARACHUTE;
#endif

		if (SDL_Init(sdl_flags) == -1)
			I_FatalError("Could not initialize SDL:\n%s\n", SDL_GetError());

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
