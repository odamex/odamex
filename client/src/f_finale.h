// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	F_FINALE
//    
//-----------------------------------------------------------------------------

#pragma once


#include "d_event.h"
//
// FINALE
//

// Called by main loop.
BOOL F_Responder (event_t* ev);

// Called by main loop.
void F_Ticker (void);

// Called by main loop.
void F_Drawer (void);

struct finale_options_t
{
	const char* music;
	const char* flat;
	const char* text;
	const char* pic;
};

void F_StartFinale(finale_options_t& options);
void STACK_ARGS F_ShutdownFinale();
