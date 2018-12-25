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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __M_SWAP_H__
#define __M_SWAP_H__

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

unsigned short LESHORT(unsigned short x);
short LESHORT(short x);

unsigned int LELONG(unsigned int x);
int LELONG(int x);

unsigned long LELONG(unsigned long x);
long LELONG(long x);

unsigned short BESHORT(unsigned short x);
short BESHORT(short x);

unsigned int BELONG(unsigned int x);
int BELONG(int x);

unsigned long BELONG(unsigned long x);
long BELONG(long x);

#else

unsigned short LESHORT(unsigned short x);
short LESHORT(short x);

unsigned int LELONG(unsigned int x);
int LELONG(int x);

unsigned long LELONG(unsigned long x);
long LELONG(long x);

unsigned short BESHORT(unsigned short x);
short BESHORT(short x);

unsigned int BELONG(unsigned int x);
int BELONG(int x);

unsigned long BELONG(unsigned long x);
long BELONG(long x);

#endif // __BIG_ENDIAN__

#endif // __M_SWAP_H__


