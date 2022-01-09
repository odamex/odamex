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
//  A basic debugging logger.
//
//-----------------------------------------------------------------------------

#include "log.h"

#if defined(_MSC_VER)

static void PlatformDebugOut(const char* str)
{
	OutputDebugStringA(str);
}

#else

static void PlatformDebugOut(const char* str)
{
	fprintf(stderr, "%s", str);
}

#endif

void Log_Debug(const char* fmt, ...)
{
	va_list args;
	static char buffer[8192];

	va_start(args, fmt);
	vsnprintf(buffer, ARRAY_LENGTH(buffer), fmt, args);
	va_end(args);

	PlatformDebugOut(buffer);
}
