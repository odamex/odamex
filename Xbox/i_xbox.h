// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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

struct hostent
{
	char  *h_name;       /* canonical name of host */
	char **h_aliases;    /* alias list */
	int    h_addrtype;   /* host address type */
	int    h_length;     /* length of address */
	char **h_addr_list;  /* list of addresses */
#define h_addr h_addr_list
};

#undef getenv
char *getenv(const char *);
#undef putenv
int putenv(const char *);
#undef getcwd
char *getcwd(char *buf, size_t size);

struct hostent *gethostbyname(const char *name);
int gethostname(char *name, int namelen);

// Set the x, y screen starting position in relation to 0,0
int xbox_SetScreenPosition(float x, float y);
// Stretch the image to some percentage of its full resolution - 1.0 = 100%
int xbox_SetScreenStretch(float xs, float ys);

#endif // _XBOX

#endif // _I_XBOX_H