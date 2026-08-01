// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
extern uint32_t LanguageIDs[4];
extern void SetLanguageIDs ();

void I_BeginRead (void);
void I_EndRead (void);

// Called by DoomMain.
void I_Init (void);

// Called by startup code
// to get the ammount of memory to malloc
// for the zone management.
void *I_ZoneBase (size_t *size);

// Asynchronous interrupt functions should maintain private queues
// that are read by the synchronous functions
// to be converted into events.

// Either returns a null ticcmd,
// or calls a loadable driver to build it.
// This ticcmd will then be modified by the gameloop
// for normal input.
ticcmd_t *I_BaseTiccmd();


// Called by M_Responder when quit is selected.
// Clean exit, displays sell blurb.
void I_Quit();

[[noreturn]] void I_BaseError(const std::string& errortext);
[[noreturn]] void I_BaseFatalError(const std::string& errortext);

template <typename... ARGS>
[[noreturn]] void I_Error(fmt::format_string<ARGS...>format, ARGS&&... args)
{
	I_BaseError(fmt::format(format, std::forward<ARGS>(args)...));
}

template <typename... ARGS>
[[noreturn]] void I_FatalError(fmt::format_string<ARGS...> format, ARGS&&... args)
{
	I_BaseFatalError(fmt::format(format, std::forward<ARGS>(args)...));
}

void addterm (void (*func)(), const char *name);
#define atterm(t) addterm (t, #t)

bool I_ConsoleUseColor();
std::string I_ConsoleInput();

void I_FinishClockCalibration ();
