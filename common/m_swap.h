// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

#pragma once

#if TARGET_CPU_X86 || TARGET_CPU_X86_64
#ifdef __BIG_ENDIAN__
#undef __BIG_ENDIAN__
#endif
#ifndef __LITTLE_ENDIAN__
#define __LITTLE_ENDIAN__
#endif
#endif

#if TARGET_CPU_PPC
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#ifdef __LITTLE_ENDIAN__
#undef __LITTLE_ENDIAN__
#endif
#endif

// Endianess handling.
// WAD files are stored little endian.
#ifdef __BIG_ENDIAN__

// Swap 16bit, that is, MSB and LSB byte.
// No masking with 0xFF should be necessary.

inline static unsigned short LESHORT(unsigned short x)
{
	return (unsigned short)((x >> 8) | (x << 8));
}

inline static short LESHORT(short x)
{
	return (short)((((unsigned short)x) >> 8) | (((unsigned short)x) << 8));
}

// Swapping 32bit.
inline static unsigned int LELONG(unsigned int x)
{
	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) |
	                      (x << 24));
}

inline static int LELONG(int x)
{
	return (int)((((unsigned int)x) >> 24) | ((((unsigned int)x) >> 8) & 0xff00) |
	             ((((unsigned int)x) << 8) & 0xff0000) | (((unsigned int)x) << 24));
}

inline static unsigned long LELONG(unsigned long x)
{
	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) |
	                       (x << 24));
}

inline static long LELONG(long x)
{
	return (long)((((unsigned int)x) >> 24) | ((((unsigned int)x) >> 8) & 0xff00) |
	              ((((unsigned int)x) << 8) & 0xff0000) | (((unsigned int)x) << 24));
}

inline static unsigned short BESHORT(unsigned short x)
{
	return x;
}

inline static short BESHORT(short x)
{
	return x;
}

inline static unsigned int BELONG(unsigned int x)
{
	return x;
}

inline static int BELONG(int x)
{
	return x;
}

inline static unsigned long BELONG(unsigned long x)
{
	return x;
}

inline static long BELONG(long x)
{
	return x;
}

#else

inline static unsigned short LESHORT(unsigned short x)
{
	return x;
}

inline static short LESHORT(short x)
{
	return x;
}

inline static unsigned int LELONG(unsigned int x)
{
	return x;
}

inline static int LELONG(int x)
{
	return x;
}

inline static unsigned long LELONG(unsigned long x)
{
	return x;
}

inline static long LELONG(long x)
{
	return x;
}

inline static unsigned short BESHORT(unsigned short x)
{
	return (unsigned short)((x >> 8) | (x << 8));
}

inline static short BESHORT(short x)
{
	return (short)((((unsigned short)x) >> 8) | (((unsigned short)x) << 8));
}

inline static unsigned int BELONG(unsigned int x)
{
	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) |
	                      (x << 24));
}

inline static int BELONG(int x)
{
	return (int)((((unsigned int)x) >> 24) | ((((unsigned int)x) >> 8) & 0xff00) |
	             ((((unsigned int)x) << 8) & 0xff0000) | (((unsigned int)x) << 24));
}

inline static unsigned long BELONG(unsigned long x)
{
	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x << 8) & 0xff0000) |
	                       (x << 24));
}

inline static long BELONG(long x)
{
	return (long)((((unsigned int)x) >> 24) | ((((unsigned int)x) >> 8) & 0xff00) |
	              ((((unsigned int)x) << 8) & 0xff0000) | (((unsigned int)x) << 24));
}

#endif
