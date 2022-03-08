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


#pragma once

#ifdef _WIN32
#include <io.h>
#endif

#include "d_ticcmd.h"
#include "d_event.h"


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
void I_Endoom(void);

// Called by startup code
// to get the ammount of memory to malloc
// for the zone management.
void *I_ZoneBase (size_t *size);


// returns current time in nanoseconds.
dtime_t I_GetTime();

dtime_t I_ConvertTimeToMs(dtime_t value);
dtime_t I_ConvertTimeFromMs(dtime_t value);

// yields to the OS for the specified time (in nanoseconds)
void I_Sleep(dtime_t);
// yields to the OS for 1 millisecond
void I_Yield();

//
// Called by D_DoomLoop,
// called before processing each tic in a frame.
// Quick syncronous operations are performed here.
// Can call D_PostEvent.
void I_StartTic (void);

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

void I_Warning(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(void, I_Warning, fmt::CStringRef)
void I_Error(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(void, I_Error, fmt::CStringRef)
NORETURN void I_FatalError(fmt::CStringRef format, fmt::ArgList args);
FMT_VARIADIC(void, I_FatalError, fmt::CStringRef)

void addterm (void (STACK_ARGS *func)(void), const char *name);
#define atterm(t) addterm (t, #t)

// Repaint the pre-game console
void I_PaintConsole (void);

// Print a console string
void I_PrintStr (int x, const char *str, int count, BOOL scroll);

// Set the title string of the startup window
void I_SetTitleString (const char *title);

std::string I_ConsoleInput (void);

// Returns true if there will be no application window
bool I_IsHeadless();

// [RH] Returns millisecond-accurate time
dtime_t I_MSTime (void);

// [RH] Title string to display at bottom of console during startup
extern char DoomStartupTitle[256];

void I_FinishClockCalibration ();

std::string I_GetClipboardText();

/**
 * @brief Show an error message box.
 *
 * @param message Contents of the message box.
 */
void I_ErrorMessageBox(const char* message);

// Directory searching routines

typedef struct _finddata_t findstate_t;

long I_FindFirst (char *filespec, findstate_t *fileinfo);
int I_FindNext (long handle, findstate_t *fileinfo);
int I_FindClose (long handle);

#define I_FindName(a)	((a)->name)
#define I_FindAttr(a)	((a)->attrib)

#define FA_RDONLY	_A_RDONLY
#define FA_HIDDEN	_A_HIDDEN
#define FA_SYSTEM	_A_SYSTEM
#define FA_DIREC	_A_SUBDIR
#define FA_ARCH		_A_ARCH
