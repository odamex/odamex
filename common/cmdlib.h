// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Command library (?)
//
//-----------------------------------------------------------------------------


#ifndef __CMDLIB__
#define __CMDLIB__

#include <string>
#include <algorithm>

#ifdef _MSC_VER
#pragma warning(disable : 4244)     // MIPS
#pragma warning(disable : 4136)     // X86
#pragma warning(disable : 4051)     // ALPHA

#pragma warning(disable : 4018)     // signed/unsigned mismatch
#pragma warning(disable : 4305)     // truncate from double to float
#endif

#include "doomtype.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>

// the dec offsetof macro doesnt work very well...
#define myoffsetof(type,identifier) ((size_t)&((type *)0)->identifier)

extern std::string progdir, startdir;

void	FixPathSeparator (std::string &path);

int		ParseHex (char *str);
int 	ParseNum (char *str);
BOOL	IsNum (char *str);		// [RH] added
int		StdStringCompare(std::string string1, std::string string2, bool CaseInsensitive);
char	*COM_Parse (char *data);

extern	char	com_token[8192];
extern	BOOL	com_eof;

char	*copystring(const char *s);

void	CRC_Init(unsigned short *crcvalue);
void	CRC_ProcessByte(unsigned short *crcvalue, byte data);
unsigned short CRC_Value(unsigned short crcvalue);


#endif



