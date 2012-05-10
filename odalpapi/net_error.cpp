// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Error handling
//
// AUTHORS:
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#include "net_error.h"

#include <cstdio>
#include <cstdarg>

#ifdef _WIN32
#include <windows.h>
#else
#include <string.h>
#include <errno.h>
#endif

// todo: detect agar
#if 0
#include <agar/core.h>
#endif

#ifdef _XBOX
#include "xbox_main.h"
#endif

namespace odalpapi {

#ifdef _WIN32
// Russell - bits from msdn:
static LPSTR _GetFormattedMessage(DWORD errnum)
{
    LPSTR pBuffer = NULL;

    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_ALLOCATE_BUFFER,
                  NULL,
                  errnum,
                  0,
                  (LPSTR)&pBuffer,
                  0,
                  NULL);

    return pBuffer;
}
#endif

static char *_GetStrError(int errnum)
{
    #ifdef _WIN32
    return _GetFormattedMessage(errnum);
    #else
    return strerror(errnum);
    #endif
}

// Get error code
static int _GetErrno()
{
    #ifdef _WIN32
    return GetLastError();
    #else
    return errno;
    #endif
}

// Formatted debug output
void _ReportError(const char *file, int line, const char *func,
    const char *fmt, ...)
{
	va_list ap;

	if(!func || !fmt)
		return;

    char *syserrmsg = _GetStrError(_GetErrno());

	va_start(ap, fmt);

#ifdef _XBOX
	char errorstr[1024];

	Xbox::OutputDebugString("[%s:%d] BufferedSocket::%s(): ", file, line, func);
	vsprintf(errorstr, fmt, ap);
	Xbox::OutputDebugString("%s\n syserrmsg - %s\n", errorstr, syserrmsg);
#else
	fprintf(stderr, "[%s:%d] BufferedSocket::%s(): ", file, line, func);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n syserrmsg - %s\n", syserrmsg);
	fputs("\n", stderr);
#endif // _XBOX

	va_end(ap);

    #ifdef _WIN32
    LocalFree(syserrmsg);
    #endif
}

} // namespace
