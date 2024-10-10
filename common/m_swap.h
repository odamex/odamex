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

#include <nonstd/bit.hpp>

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

inline static unsigned short LESHORT(const unsigned short x)
{
	return nonstd::bit::as_little_endian(x);
}

inline static short LESHORT(const short x)
{
	return nonstd::bit::as_little_endian(x);
}

inline static unsigned int LELONG(const unsigned int x)
{
	return nonstd::bit::as_little_endian(x);
}

inline static int LELONG(const int x)
{
	return nonstd::bit::as_little_endian(x);
}

inline static unsigned long LELONG(const unsigned long x)
{
	return nonstd::bit::as_little_endian(x);
}

inline static long LELONG(const long x)
{
	return nonstd::bit::as_little_endian(x);
}

inline static unsigned short BESHORT(const unsigned short x)
{
	return nonstd::bit::as_big_endian(x);
}

inline static short BESHORT(const short x)
{
	return nonstd::bit::as_big_endian(x);
}

inline static unsigned int BELONG(const unsigned int x)
{
	return nonstd::bit::as_big_endian(x);
}

inline static int BELONG(const int x)
{
	return nonstd::bit::as_big_endian(x);
}

inline static unsigned long BELONG(const unsigned long x)
{
	return nonstd::bit::as_big_endian(x);
}

inline static long BELONG(const long x)
{
	return nonstd::bit::as_big_endian(x);
}
