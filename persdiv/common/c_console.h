// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: c_console.h 1847 2010-09-04 05:16:08Z ladna $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2012 by The Odamex Team.
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


#ifndef __C_CONSOLE__
#define __C_CONSOLE__

#include <stdio.h>
#include <stdarg.h>

#include "doomtype.h"
#include "doomdef.h"
#include "d_event.h"
#include "cmdlib.h"
#include "d_player.h"

#define C_BLINKRATE			(TICRATE/2)
#define MAX_CHATSTR_LEN		128

typedef enum cstate_t {
	c_up=0, c_down, c_falling, c_rising, c_fallfull, c_risefull
} constate_e;

extern constate_e	ConsoleState;

// Initialize the console
void C_InitConsole (int width, int height, BOOL ingame);

// SoM
void C_ServerDisconnectEffect(void);

// Adjust the console for a new screen mode
void C_NewModeAdjust (void);

void C_Ticker (void);

int PrintString (int printlevel, const char *string);
int VPrintf (int printlevel, const char *format, va_list parms);
int STACK_ARGS Printf_Bold (const char *format, ...);

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

BOOL C_Responder (event_t *ev);

void C_AddTabCommand (const char *name);
void C_RemoveTabCommand (const char *name);

void C_RevealSecret ();

#endif


