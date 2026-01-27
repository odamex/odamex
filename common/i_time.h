// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//      This module contains the basic time functions.
//
//-----------------------------------------------------------------------------

#pragma once

#include "doomtype.h"

// Returns current time in nanoseconds.
dtime_t I_GetTime();

// [RH] Returns current time in milliseconds.
dtime_t I_MSTime (void);

dtime_t I_ConvertTimeToMs(dtime_t value);
dtime_t I_ConvertTimeFromMs(dtime_t value);

// Yields to the OS for the specified time (in nanoseconds).
void I_Sleep(dtime_t);
// Yields to the OS for 1 millisecond.
void I_Yield();

// I_WaitVBL is never used to actually synchronize to the
// vertical blank. Instead, it's used for delay purposes.
// Sleeps for `count` * 70Hz intervals.
void I_WaitVBL(int count);
