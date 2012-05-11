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
//	Xbox Support
//
//-----------------------------------------------------------------------------

#ifndef _I_XBOX_H
#define _I_XBOX_H

#ifdef _XBOX

#include <SDL.h>

struct hostent
{
	char  *h_name;       /* canonical name of host */
	char **h_aliases;    /* alias list */
	int    h_addrtype;   /* host address type */
	int    h_length;     /* length of address */
	char **h_addr_list;  /* list of addresses */
#define h_addr h_addr_list[0]
};

// Xbox function overrides
#undef getenv
#define getenv xbox_Getenv
#undef putenv
#define putenv xbox_Putenv
#undef getcwd
#define getcwd xbox_GetCWD
#undef exit
#define exit xbox_Exit
#undef atexit
#define atexit xbox_AtExit
#define inet_ntoa xbox_InetNtoa
#define gethostbyname xbox_GetHostByName
#define gethostname xbox_GetHostname

// Xbox function override declarations
char *xbox_Getenv(const char *);
int xbox_Putenv(const char *);
char *xbox_GetCWD(char *buf, size_t size);
char *xbox_InetNtoa(struct in_addr in);
struct hostent *xbox_GetHostByName(const char *name);
int xbox_GetHostname(char *name, int namelen);
void xbox_Exit(int status);
void xbox_AtExit(void (*function)(void));

// Print useful memory information for debugging
void xbox_PrintMemoryDebug();

// Set the x, y screen starting position in relation to 0,0
int xbox_SetScreenPosition(float x, float y);
// Stretch the image to some percentage of its full resolution - 1.0 = 100%
int xbox_SetScreenStretch(float xs, float ys);

// Write a SaveMeta.xbx file
void xbox_WriteSaveMeta(std::string path, std::string text);

// Get a unique save path for a save slot
std::string xbox_GetSavePath(std::string file, int slot);

#endif // _XBOX

#endif // _I_XBOX_H
