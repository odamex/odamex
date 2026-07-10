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


#include "odamex.h"

#include <stdlib.h>

#include "i_system.h"
#include "m_alloc.h"

void* M_Malloc(size_t size)
{
	// We don't want implementation-defined behaviour!
	if (!size)
		return nullptr;

	void* zone = malloc(size);

	if (!zone)
		I_FatalError("Could not malloc {} bytes", size);

	return zone;
}

void* M_Calloc (size_t num, size_t size)
{
	// We don't want implementation-defined behaviour!
	if (!num || !size)
		return nullptr;

	void *zone = calloc (num, size);

	if (!zone)
		I_FatalError("Could not calloc {} bytes", num * size);

	return zone;
}

void* Realloc(void* memblock, size_t size)
{
	// We don't want implementation-defined behaviour! Especially for this
	// as realloc() behaves like malloc() (which doesn't use our Malloc())
	if (!size && memblock == nullptr)
		return nullptr;

	void* zone = realloc (memblock, size);

	if (!zone)
		I_FatalError("Could not realloc {} bytes", size);

	return zone;
}

VERSION_CONTROL (m_alloc_cpp, "$Id$")
