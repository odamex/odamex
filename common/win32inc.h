// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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

#ifndef __WIN32INC_H__
#define __WIN32INC_H__

#ifdef _WIN32

    // DrawText macros in winuser.h interfere with v_video.h
    #ifndef NODRAWTEXT
        #define NODRAWTEXT
    #endif  // NODRAWTEXT

    #ifndef NOMINMAX
        #define NOMINMAX
    #endif  // NOMINMAX;

    #define WIN32_LEAN_AND_MEAN
    #ifndef _XBOX
        // need to make winxp compat for raw mouse input
        #if (_WIN32_WINNT < 0x0501)
            #undef _WIN32_WINNT
            #define _WIN32_WINNT 0x0501
        #endif

        #include <windows.h>
    #else
        #define _WIN32_WINNT 0x0400 // win2000 compat
        #include <xtl.h>
    #endif // !_XBOX

	// avoid a conflict with the winuser.h macro DrawText
	#ifdef DrawText
        #undef DrawText
    #endif

    // LoadMenu macros in winuser.h interfere with m_menu.cpp
    #ifdef LoadMenu
        #undef LoadMenu
    #endif  // LoadMenu

    // POSIX functions
	#include <ctime>
    char * strptime(const char *buf, const char *fmt, struct tm *timeptr);
    time_t timegm(struct tm *tm);

    #if (defined _MSC_VER)
        #define strncasecmp _strnicmp
    #endif

    // C99 functions
    //
    // Missing from MSVC++ older than 2015, implementation in
    // common/sprintf.cpp.
    //
    // We must use this implementation because _snprintf and
    // _vsnprintf do not have the same behavior as their C99
    // counterparts, and are thus unsafe to substitute.
    #if defined(_MSC_VER) && _MSC_VER < 1900
        int snprintf(char *s, size_t n, const char *fmt, ...);
        int vsnprintf(char *s, size_t n, const char *fmt, va_list ap);
    #endif
#endif // WIN32

#endif  // __WIN32INC_H__
