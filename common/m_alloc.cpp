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
//	M_ALLOC
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "i_system.h"
#include "m_alloc.h"

void *Malloc (size_t size)
{
	void *zone = malloc (size);

	if (!zone)
		I_FatalError ("Could not malloc %ld bytes", size);

	return zone;
}

void *Calloc (size_t num, size_t size)
{
	void *zone = calloc (num, size);

	if (!zone)
		I_FatalError ("Could not calloc %ld bytes", num * size);

	return zone;
}

void *Realloc (void *memblock, size_t size)
{
	void *zone = realloc (memblock, size);

	if (!zone)
		I_FatalError ("Could not realloc %ld bytes", size);

	return zone;
}

VERSION_CONTROL (m_alloc_cpp, "$Id:$")

