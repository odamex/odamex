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
// DESCRIPTION:
//	Xbox Support
//
// AUTHORS:
//	 Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------

#ifndef _XBOX_MAIN_H
#define _XBOX_MAIN_H

#ifdef _XBOX

// hostent
struct hostent
{
	char  *h_name;       /* canonical name of host */
	char **h_aliases;    /* alias list */
	int    h_addrtype;   /* host address type */
	int    h_length;     /* length of address */
	char **h_addr_list;  /* list of addresses */
#define h_addr h_addr_list[0]
};

char *inet_ntoa(struct in_addr in);
struct hostent *gethostbyname(const char *name);
void xbox_OutputDebugString(const char *str);
int xbox_InitializeJoystick(void);
void xbox_EnableJoystickUpdates(bool);


#endif // _XBOX

#endif // _XBOX_MAIN_H