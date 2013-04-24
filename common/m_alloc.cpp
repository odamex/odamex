// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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


#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "m_alloc.h"

void *Malloc (size_t size)
{
	// We don't want implementation-defined behaviour!
	if (!size)
        return NULL;
	
	void *zone = malloc (size);

	if (!zone)
		I_FatalError ("Could not malloc %lu bytes", size);

	return zone;
}

void *Calloc (size_t num, size_t size)
{
	// We don't want implementation-defined behaviour!
	if (!num || !size)
        return NULL;
	
	void *zone = calloc (num, size);

	if (!zone)
		I_FatalError ("Could not calloc %lu bytes", num * size);

	return zone;
}

void *Realloc (void *memblock, size_t size)
{
	// We don't want implementation-defined behaviour! Especially for this
	// as realloc() behaves like malloc() (which doesn't use our Malloc())
	if (!size && memblock == NULL)
        return NULL;

	void *zone = realloc (memblock, size);

	if (!zone)
		I_FatalError ("Could not realloc %lu bytes", size);

	return zone;
}

//
// M_Free
//
// Wraps around the standard free() memory function. This variation is slightly 
// more safer, as it only frees a block if its not NULL and will NULL it on
// exiting.
void M_Free2 (void **memblock)
{
    if (*memblock != NULL)
    {               
        free(*memblock);
        *memblock = NULL;
    }
}

VERSION_CONTROL (m_alloc_cpp, "$Id$")

