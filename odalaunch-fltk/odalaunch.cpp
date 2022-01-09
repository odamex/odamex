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
//  Global defines.
//
//-----------------------------------------------------------------------------

#include "odalaunch.h"

#if defined(USE_MS_SNPRINTF)

int __cdecl ms_snprintf(char* buffer, size_t n, const char* format, ...)
{
	int retval;
	va_list argptr;

	va_start(argptr, format);
	retval = _vsnprintf(buffer, n, format, argptr);
	va_end(argptr);
	return retval;
}

int __cdecl ms_vsnprintf(char* s, size_t n, const char* format, va_list arg)
{
	return _vsnprintf(s, n, format, arg);
}

#endif
