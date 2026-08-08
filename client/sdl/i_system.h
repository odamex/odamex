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
extern uint32_t LanguageIDs[4];
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
void I_Quit();

void I_BaseWarning(const std::string& errortext);
[[noreturn]] void I_BaseError(const std::string& errortext);
[[noreturn]] void I_BaseFatalError(const std::string& errortext);

template <typename... ARGS>
void I_Warning(fmt::format_string<ARGS...> format, ARGS&&... args)
{
	I_BaseWarning(fmt::format(format, std::forward<ARGS>(args)...));
}

template <typename... ARGS>
[[noreturn]] void I_Error(fmt::format_string<ARGS...> format, ARGS&&... args)
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

// Repaint the pre-game console
void I_PaintConsole (void);

// Returns true if there will be no application window
bool I_IsHeadless();

void I_FinishClockCalibration ();

std::string I_GetClipboardText(bool use_primary_selection = false);

/**
 * @brief Show an error message box.
 *
 * @param message Contents of the message box.
 */
void I_ErrorMessageBox(const char* message);
