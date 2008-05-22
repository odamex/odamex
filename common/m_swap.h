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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __M_SWAP_H__
#define __M_SWAP_H__

#ifdef TARGET_CPU_X86
#ifdef __BIG_ENDIAN__
#undef __BIG_ENDIAN__
#endif
#endif

#ifdef TARGET_CPU_PPC
#ifndef __BIG_ENDIAN__
#define __BIG_ENDIAN__
#endif
#endif

// Endianess handling.
// WAD files are stored little endian.
#ifdef __BIG_ENDIAN__

// Swap 16bit, that is, MSB and LSB byte.
// No masking with 0xFF should be necessary. 
short SHORT (short x);
unsigned short SHORT (unsigned short x);

// Swapping 32bit.
unsigned int LONG (unsigned int x);
int LONG (int x);

#define BESHORT(x)		(x)
#define BELONG(x)		(x)

#else

#define SHORT(x)		(x)
#define LONG(x) 		(x)

short BESHORT (short x);
unsigned short BESHORT (unsigned short x);

unsigned int BELONG (unsigned int x);
int BELONG (int x);

#endif // __BIG_ENDIAN__

#endif // __M_SWAP_H__


