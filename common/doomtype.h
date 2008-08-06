// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Simple basic typedefs, isolated here to make it easier
//	 separating modules.
//    
//-----------------------------------------------------------------------------


#ifndef __DOOMTYPE__
#define __DOOMTYPE__

#include "version.h"

#ifndef __BYTEBOOL__
#define __BYTEBOOL__
// [RH] Some windows includes already define this
#if !defined(_WINDEF_) && !defined(__wtypes_h__)
typedef int BOOL;
#endif
#ifndef __cplusplus
#define false (0)
#define true (1)
#endif
typedef unsigned char byte;
#endif

#if defined(_MSC_VER) || defined(__WATCOMC__)
#define STACK_ARGS __cdecl
#else
#define STACK_ARGS
#endif

// Predefined with some OS.
#ifndef UNIX
#ifndef _MSC_VER
#include <values.h>
#endif
#endif

#if defined(__GNUC__) && !defined(OSF1)
#define __int64 long long
#endif

#ifdef OSF1
#define __int64 long
#endif

#ifdef UNIX
#define stricmp strcasecmp
#define strnicmp strncasecmp
#endif

#ifndef MAXCHAR
#define MAXCHAR 		((char)0x7f)
#endif
#ifndef MAXSHORT
#define MAXSHORT		((short)0x7fff)
#endif

// Max pos 32-bit int.
#ifndef MAXINT
#define MAXINT			((int)0x7fffffff)
#endif
#ifndef MAXLONG
#ifndef ALPHA
#define MAXLONG 		((long)0x7fffffff)
#else
#define MAXLONG			((long)0x7fffffffffffffff)
#endif
#endif

#ifndef MINCHAR
#define MINCHAR 		((char)0x80)
#endif
#ifndef MINSHORT
#define MINSHORT		((short)0x8000)
#endif

// Max negative 32-bit integer.
#ifndef MININT
#define MININT			((int)0x80000000)
#endif
#ifndef MINLONG
#ifndef ALPHA
#define MINLONG 		((long)0x80000000)
#else
#define MINLONG			((long)0x8000000000000000)
#endif
#endif

typedef unsigned char		BYTE;
typedef signed char			SBYTE;

typedef unsigned short		WORD;
typedef signed short		SWORD;

// denis - todo - this 64 bit fix conflicts with windows' idea of long
#ifndef WIN32
typedef unsigned int		DWORD;
typedef signed int			SDWORD;
#else
typedef unsigned long		DWORD;
typedef signed long			SDWORD;
#endif

typedef unsigned __int64	QWORD;
typedef signed __int64		SQWORD;

typedef DWORD				BITFIELD;

#ifndef NOASM
#ifndef USEASM
#define USEASM 1
#endif
#else
#ifdef USEASM
#undef USEASM
#endif
#endif



// [RH] This gets used all over; define it here:
int STACK_ARGS Printf (int printlevel, const char *, ...);
// [RH] Same here:
int STACK_ARGS DPrintf (const char *, ...);

// Simple log file
#include <fstream>

extern std::ofstream LOG;
extern std::ifstream CON;

// game print flags
#define	PRINT_LOW			0		// pickup messages
#define	PRINT_MEDIUM		1		// death messages
#define	PRINT_HIGH			2		// critical messages
#define	PRINT_CHAT			3		// chat messages
#define PRINT_TEAMCHAT		4		// chat messages from a teammate

#endif


