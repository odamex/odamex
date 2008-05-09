// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2007 by The Odamex Team.
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


#ifndef __M_ALLOC_H__
#define __M_ALLOC_H__

#include <stdlib.h>
#include <stdint.h>

// These are the same as the same stdlib functions,
// except they bomb out with an error requester
// when they can't get the memory.

void *Malloc (size_t size);
void *Calloc (size_t num, size_t size);
void *Realloc (void *memblock, size_t size);

// don't use these, use the macros instead!
void M_Free2 (uintptr_t &memblock);

#define M_Free(p) do { M_Free2((uintptr_t &)p); } while(0)

#endif //__M_ALLOC_H__


