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
//  AutoMap module.
//
//-----------------------------------------------------------------------------


#ifndef __AMMAP_H__
#define __AMMAP_H__

// Forward reference
struct event_t;

// Used by ST StatusBar stuff.
#define AM_MSGHEADER (('a'<<24)+('m'<<16))
#define AM_MSGENTERED (AM_MSGHEADER | ('e'<<8))
#define AM_MSGEXITED (AM_MSGHEADER | ('x'<<8))

extern int am_cheating;

// Called by main loop.
BOOL AM_Responder(event_t* ev);

// Called by main loop.
void AM_Ticker();

// Called by main loop,
// called instead of view drawer if automap active.
void AM_Drawer();

// Called to force the automap to quit
// if the level is completed while it is up.
void AM_Stop();

bool AM_ClassicAutomapVisible();
bool AM_OverlayAutomapVisible();


#endif
