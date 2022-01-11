// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//  Platform-specific functions.
//
//-----------------------------------------------------------------------------

#include "plat.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#else
#include <unistd.h>
#endif

void Plat_DebugOut(const char* str)
{
#ifdef _WIN32
	OutputDebugStringA(str);
#else
	fprintf(stderr, "%s", str);
#endif
}

uint32_t Plat_GetCoreCount()
{
#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
	// [AM] BSD uses a different mechanism.
	return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
#endif
}
