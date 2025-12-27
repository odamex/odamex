// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// DESCRIPTION:
//	Common Windows includes and defines
//
//-----------------------------------------------------------------------------

#pragma once

#ifdef _WIN32

    // DrawText macros in winuser.h interfere with v_video.h
    #ifndef NODRAWTEXT
        #define NODRAWTEXT
    #endif  // NODRAWTEXT

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif  // NOMINMAX;

    #define WIN32_LEAN_AND_MEAN
    // need to make winxp compat for raw mouse input
    // need at least vista for SHGetKnownFolderPath
    #if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
        #undef _WIN32_WINNT
        #define _WIN32_WINNT _WIN32_WINNT_VISTA
    #endif

    #include <windows.h>

	// avoid a conflict with the winuser.h macro DrawText
	#ifdef DrawText
        #undef DrawText
    #endif

    // Same with PlaySound
    #ifdef PlaySound
        #undef PlaySound
    #endif

    // POSIX functions
	#include <ctime>
    char * strptime(const char *buf, const char *fmt, struct tm *timeptr);
    time_t timegm(struct tm *tm);

    #if (defined _MSC_VER)
        #define strncasecmp _strnicmp
    #endif
#endif // WIN32
