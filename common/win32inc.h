// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: win32inc.h 3798 2013-04-24 03:09:33Z dr_sean $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

#ifdef WIN32

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

#endif // WIN32

#endif  // __WIN32INC_H__
