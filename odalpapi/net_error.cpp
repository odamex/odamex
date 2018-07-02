// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//  Error handling
//
// AUTHORS:
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#include "net_error.h"

#include <cstdio>
#include <cstdarg>

#if defined(_XBOX)
#include "xbox_main.h"
#elif defined (_WIN32)
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
#endif

namespace odalpapi
{

#if defined(_WIN32) && !defined(_XBOX)
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

static char* _GetStrError(int errnum)
{
#if defined(_WIN32) && !defined(_XBOX)
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

// We must lock the stderr stream for multiple thread access
#ifdef _WIN32
CRITICAL_SECTION STDERR_CRITICAL_SECTION;
static bool Init = false;

void InitLockStderr()
{
	if(!Init)
		InitializeCriticalSection(&STDERR_CRITICAL_SECTION);

	Init = true;
}

void LockStderr()
{
	EnterCriticalSection(&STDERR_CRITICAL_SECTION);
}

void UnlockStderr()
{
	LeaveCriticalSection(&STDERR_CRITICAL_SECTION);
}
#endif // _WIN32

// Formatted debug output
void _ReportError(const char* file, int line, const char* func,
                  const char* fmt, ...)
{
	va_list ap;

	if(!func || !fmt)
		return;

#if _WIN32 || _XBOX
	InitLockStderr();

	LockStderr();
#else
	flockfile(stderr);
#endif

	char* syserrmsg = NULL;
	int syserr = _GetErrno();

	if(syserr)
	{
		syserrmsg = _GetStrError(syserr);
	}

	va_start(ap, fmt);

#ifdef _XBOX
	char errorstr[1024];

	agOdalaunch::Xbox::OutputDebugString("[%s:%d] BufferedSocket::%s(): ", file, line, func);
	vsprintf(errorstr, fmt, ap);
	agOdalaunch::Xbox::OutputDebugString("%s\n", errorstr);
	if(NULL != syserrmsg)
	{
		agOdalaunch::Xbox::OutputDebugString("syserrmsg - %s\n", errorstr, syserrmsg);
	}
#else
	fprintf(stderr, "[%s:%d] BufferedSocket::%s(): ", file, line, func);
	vfprintf(stderr, fmt, ap);
	if(NULL != syserrmsg)
	{
		fprintf(stderr, "\n syserrmsg - %s\n", syserrmsg);
	}
	fputs("\n", stderr);
#endif // _XBOX

	va_end(ap);

#if _WIN32 || _XBOX
	if(NULL != syserrmsg)
	{
		LocalFree(syserrmsg);
	}
	UnlockStderr();
#else
	funlockfile(stderr);
#endif
}

#if _MSC_VER < 1400
void NET_ReportError(const char* fmt, ...)
{
	va_list ap;
	char errorstr[1024];

	va_start(ap, fmt);
	vsprintf(errorstr, fmt, ap);
	va_end(ap);

	_ReportError("", 0, "", errorstr);
}
#endif

} // namespace
