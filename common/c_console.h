// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_CONSOLE
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdarg.h>

#include "d_event.h"
#include "cmdlib.h"
#include "d_player.h"

#define C_BLINKRATE			(TICRATE/2)
#define MAX_CHATSTR_LEN		128

// Console visibility / slide-animation state is owned internally by the console.
// Query these predicates instead of the former global ConsoleState / constate_e enum.
bool C_IsConsoleUp();             // fully hidden (c_up)
bool C_IsConsoleDown();           // fully open (c_down)
bool C_IsConsoleActivating();     // open or sliding into the half-screen (c_down || c_falling)
bool C_IsConsoleCapturingInput(); // occupies enough screen to capture input (c_down || c_falling || c_fallfull)

// Initialize the console
void C_InitConsole();
void STACK_ARGS C_ShutdownConsole();

void C_InitConsoleBackground();
void STACK_ARGS C_ShutdownConsoleBackground();

void C_InitConCharsFont();
void STACK_ARGS C_ShutdownConCharsFont();

void C_ClearCommand();

// Adjust the console for a new screen mode
void C_NewModeAdjust (void);

void C_Ticker (void);
void C_DisplayTicker(void);

void C_AddNotifyString (int printlevel, const char *s);
void C_DrawConsole (void);
void C_ToggleConsole (void);
void C_FullConsole (void);
void C_HideConsole (void);
void C_AdjustBottom (void);
void C_FlushDisplay (void);

void C_InitTicker (const char *label, unsigned int max);
void C_SetTicker (unsigned int at);

void C_MidPrint (const char *msg, player_t *p = NULL, int msgtime=0);
void C_DrawMid (void);
void C_GMidPrint(const char* msg, int color, int msgtime);
void C_DrawGMid (void);

bool C_Responder(const event_t* ev);

void C_AddTabCommand (const char *name);
void C_RemoveTabCommand (const char *name);

void C_RevealSecret ();
