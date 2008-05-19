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
#define SWAP_WORD(x)   ((x>>8) | (x<<8))
#define SWAP_DWORD(x)  ((x>>24) | ((x>>8) & 0xff00) | ((x<<8) & 0xff0000) | (x<<24))

#if 0
// TODO: Why is this if 0'd ?
#define SWAP_QWORD(x)   do \
                        { \
                            x = (((x)>>56) | (((x)>>40)&(0xff<<8)) | \
                            (((x)>>24)&(0xff<<16)) | (((x)>>8)&(0xff<<24)) | \
                            (((x)<<8)&(QWORD)0xff00000000) | \
                            (((x)<<24)&(QWORD)0xff0000000000) | \
                            (((x)<<40)&(QWORD)0xff000000000000) | ((x)<<56))); \
                        } while(0)
#else
#define SWAP_QWORD(x)   do \
                        { \
                            DWORD *y = (DWORD *)&x; \
                            DWORD t=y[0]; \
                            y[0]=y[1]; \
                            y[1]=t; \
                            y = SWAP_DWORD(y[0]); \
                            y = SWAP_DWORD(y[1]); \
                        } while(0)
#endif
#define SWAP_FLOAT(x)   do \
                        { \
                            DWORD dw = *(DWORD *)&x; \
                            dw = SWAP_DWORD(dw); \
                            x = *(float *)&dw; \
                        } while(0)
#define SWAP_DOUBLE(x)  do \
                        { \
                            QWORD qw = *(QWORD *)&x; \
                            SWAP_QWORD(qw); \
                            x = *(double *)&qw; \
                        } while(0)
#else  // __LITTLE_ENDIAN__
#define SWAP_WORD(x) (x)
#define SWAP_DWORD(x) (x)
#define SWAP_QWORD(x) (x)
#define SWAP_FLOAT(x) (x)
#define SWAP_DOUBLE(x) (x)

#endif // __BIG_ENDIAN__

#endif // __M_SWAP_H__


