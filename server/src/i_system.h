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
//
//-----------------------------------------------------------------------------

#pragma once

#ifdef UNIX
#include <dirent.h>
#endif

#include "d_ticcmd.h"
#include "d_event.h"

/*extern "C"
{
    extern byte CPUFamily, CPUModel, CPUStepping;
}*/

// Index values into the LanguageIDs array
enum
{
	LANGIDX_UserPreferred,
	LANGIDX_UserDefault,
	LANGIDX_SysPreferred,
	LANGIDX_SysDefault
};
extern DWORD LanguageIDs[4];
extern void SetLanguageIDs ();

void I_BeginRead (void);
void I_EndRead (void);

// Called by DoomMain.
void I_Init (void);

// Called by startup code
// to get the ammount of memory to malloc
// for the zone management.
void *I_ZoneBase (size_t *size);


dtime_t I_GetTime();
dtime_t I_ConvertTimeToMs(dtime_t value);
dtime_t I_ConvertTimeFromMs(dtime_t value);
void I_Sleep(dtime_t sleep_time);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t *I_BaseTiccmd (void);


// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void STACK_ARGS I_Quit (void);

void I_Error(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(void, I_Error, fmt::CStringRef)
NORETURN void I_FatalError(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(void, I_FatalError, fmt::CStringRef)

void addterm (void (STACK_ARGS *func)(void), const char *name);
#define atterm(t) addterm (t, #t)

// Print a console string
void I_PrintStr (int x, const char *str, int count, BOOL scroll);

// Set the title string of the startup window
void I_SetTitleString (const char *title);

std::string I_ConsoleInput (void);

// [RH] Returns millisecond-accurate time
dtime_t I_MSTime (void);

void I_Yield(void);

// [RH] Title string to display at bottom of console during startup
extern char DoomStartupTitle[256];

void I_FinishClockCalibration ();

// Directory searching routines

typedef struct
{
    int count;
    struct dirent **namelist;
    int current;
} findstate_t;

long I_FindFirst (char *filespec, findstate_t *fileinfo);
int I_FindNext (long handle, findstate_t *fileinfo);
int I_FindClose (long handle);
int I_FindAttr (findstate_t *fileinfo);

#define I_FindName(a)	((a)->namelist[(a)->current]->d_name)

#define FA_RDONLY	1
#define FA_HIDDEN	2
#define FA_SYSTEM	4
#define FA_DIREC	8
#define FA_ARCH		16
