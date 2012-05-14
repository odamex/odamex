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
//
// DESCRIPTION:
//	Wii Remote Debugging Support
//
//-----------------------------------------------------------------------------
#ifndef	__net_print_h__
#define	__net_print_h__

int net_print_init(const char *rhost, unsigned short port);

int net_print_string( const char* file, int line, const char* format, ...);

int net_print_binary( int format, const void* binary, int len);

#define DEFAULT_NET_PRINT_PORT	5194 
#define	DEFAULT_NET_PRINT_HOST  "192.168.0.71" // Address of debug server

#endif
