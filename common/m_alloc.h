// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Wrappers around the standard memory allocation routines.
//
//-----------------------------------------------------------------------------


#pragma once

void* M_Malloc (size_t size);
void* M_Calloc (size_t num, size_t size);

// don't use this, use the macro below instead!
void* Realloc (void* memblock, size_t size);
#define M_Realloc(p,s) Realloc(static_cast<void *>(p), s)

//
// M_Free
//
// Wraps around the standard free() memory function. This variation is slightly
// safer, as it nulls the pointer on exit.
template <typename T>
inline void M_Free(T*& p) {
    free(static_cast<void*>(p));
    p = nullptr;
}
