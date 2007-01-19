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
// DESCRIPTION:
//	Main program, simply calls D_DoomMain high level loop.
//
//-----------------------------------------------------------------------------


#ifdef UNIX

#include <iostream>

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/param.h>
#include <time.h>

#include "m_argv.h"
#include "d_main.h"
#include "i_system.h"
#include "c_console.h"
#include "z_zone.h"
#include "errors.h"
#include "i_net.h"
#include "sv_main.h"

DArgs Args;

#define MAX_TERMS	16
void (STACK_ARGS *TermFuncs[MAX_TERMS]) ();
const char *TermNames[MAX_TERMS];
static int NumTerms;

void addterm (void (STACK_ARGS *func) (), const char *name)
{
    if (NumTerms == MAX_TERMS)
	{
		func ();
		I_FatalError (
			"Too many exit functions registered.\n"
			"Increase MAX_TERMS in i_main.cpp");
	}
	TermNames[NumTerms] = name;
    TermFuncs[NumTerms++] = func;
}

void popterm ()
{
	if (NumTerms)
		NumTerms--;
}

void STACK_ARGS call_terms ()
{
    while (NumTerms > 0)
	{
//		Printf ("term %d - %s\n", NumTerms, TermNames[NumTerms-1]);
		TermFuncs[--NumTerms] ();
	}
}

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

int PrintString (int printlevel, char const *outline)
{
	return printf("%s", outline);
}

//
// daemon_init
//
void daemon_init(void)
{
    int   pid;
    FILE *fpid;

    Printf(PRINT_HIGH, "Launched into the background\n");

    if ((pid = fork()) != 0)
	exit(0);

    pid = getpid();
    fpid = fopen("doomsv.pid", "w");
    fprintf(fpid, "%d\n", pid);
    fclose(fpid);
}

int main (int argc, char **argv)
{
    try
    {
		if(!getuid() || !geteuid())
			I_FatalError("root user detected, quitting odamex immediately");

	    seteuid (getuid ());

		Args.SetArgs (argc, argv);

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

		atexit (call_terms);
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

		C_InitConsole (80*8, 25*8, false);
		D_DoomMain ();
    }
    catch (CDoomError &error)
    {
		fprintf (stderr, "%s\n", error.GetMessage().c_str());
		exit (-1);
    }
    catch (...)
    {
		call_terms ();
		throw;
    }
    return 0;
}

VERSION_CONTROL (i_main_cpp, "$Id$")

#endif

