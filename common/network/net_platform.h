// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//
// Network Subsystem
//
//-----------------------------------------------------------------------------

#ifndef __NET_PLATFORM_H__
#define __NET_PLATFORM_H__

#ifdef _WIN32
	#define WIN32_LEAN_AND_MEAN
    #include "win32inc.h"

    #ifdef _XBOX
	    #include "i_xbox.h"
		#include <xtl.h>
    #else
		#include <windows.h>
		#include <winsock2.h>
		#include <ws2tcpip.h>
    #endif // !_XBOX

    typedef int socklen_t;
    #define SETSOCKOPTCAST(x) ((const char *)(x))
#else
    #include <sys/types.h>
    #include <errno.h>
    #include <unistd.h>
    #include <sys/time.h>

    #ifdef GEKKO // Wii/GC
	    #include "i_wii.h"
        #include <network.h>
    #else
        #include <sys/socket.h>
        #include <netinet/in.h>
        #include <arpa/inet.h>
        #include <netdb.h>
        #include <sys/ioctl.h>
        #include <fcntl.h>

        #define SOCKET_ERROR -1
        #define INVALID_SOCKET -1
    #endif // GEKKO

    typedef int SOCKET;

    #define closesocket close
    #define ioctlsocket ioctl
    #define Sleep(x)	usleep (x * 1000)

    #define SETSOCKOPTCAST(x) ((const void *)(x))
#endif // WIN32


#ifdef ODA_HAVE_MINIUPNP
    #define MINIUPNP_STATICLIB
    #include "miniwget.h"
    #include "miniupnpc.h"
    #include "upnpcommands.h"
#endif  // ODA_HAVE_MINIUPNP

#endif  // __NET_PLATFORM_H__