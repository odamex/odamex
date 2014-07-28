// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2014 by The Odamex Team.
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
//
//-----------------------------------------------------------------------------

#include <SDL.h>
#if (SDL_VERSION > SDL_VERSIONNUM(1, 2, 7))
#include "SDL_cpuinfo.h"
#endif
#include "r_intrin.h"

#ifdef __ALTIVEC__

#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "i_video.h"

#if !defined(__APPLE_ALTIVEC__)
#include <altivec.h>
#endif

#define ALTIVEC_ALIGNED(x) x __attribute__((aligned(16)))

// Useful vector shorthand typedefs:
typedef vector signed char vs8;
typedef vector unsigned char vu8;
typedef vector unsigned short vu16;
typedef vector unsigned int vu32;

//
// R_GetBytesUntilAligned
//
static inline uintptr_t R_GetBytesUntilAligned(void* data, uintptr_t alignment)
{
	uintptr_t mask = alignment - 1;
	return (alignment - ((uintptr_t)data & mask)) & mask;
}


// Direct rendering (32-bit) functions for ALTIVEC optimization:

void r_dimpatchD_ALTIVEC(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int surface_pitch_pixels = surface->getPitchInPixels();
	int line_inc = surface_pitch_pixels - w;

	// ALTIVEC temporaries:
	const vu8 vec_mask = { 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF };
	const vu16 vec_eight = vec_splat_u16(8);

	const vu32 vec_color_temp = { color, color, color, color };
	const vu16 vec_color = (vu16)vec_and((vu8)vec_unpackl((vs8)vec_color_temp), vec_mask);
	const vu16 vec_alpha = { alpha, alpha, alpha, alpha, alpha, alpha, alpha, alpha };
	const vu16 vec_alphacolor = (vu16)vec_mladd(vec_alpha, vec_color, vec_splat_u16(0));

	const uint16_t invalpha = 256 - alpha;
	const vu16 vec_invalpha = { invalpha, invalpha, invalpha, invalpha, invalpha, invalpha, invalpha, invalpha };

	argb_t* dest = (argb_t*)surface->getBuffer() + y1 * surface_pitch_pixels + x1;

	for (int rowcount = h; rowcount > 0; --rowcount)
	{
		// [SL] Calculate how many pixels of each row need to be drawn before dest is
		// aligned to a 128-bit boundary.
		int align = R_GetBytesUntilAligned(dest, 128/8) / sizeof(argb_t);
		if (align > w)
			align = w;

		const int batch_size = 8;
		int batches = (w - align) / batch_size;
		int remainder = (w - align) & (batch_size - 1);

		// align the destination buffer to 128-bit boundary
		while (align--)
		{
			*dest = alphablend1a(*dest, color, alpha);
			dest++;
		}

		// ALTIVEC optimize the bulk in batches of 8 pixels:
		while (batches--)
		{
			// Load 4 pixels into input0 and 4 pixels into input1
			const vu32 vec_input0 = vec_ld(0, (uint32_t*)dest);
			const vu32 vec_input1 = vec_ld(16, (uint32_t*)dest);

			// Expand the width of each color channel from 8-bits to 16-bits
			// by splitting each input vector into two 128-bit variables, each
			// containing 2 ARGB values. 16-bit color channels are needed to
			// accomodate multiplication.
			vu16 vec_upper0 = (vu16)vec_and((vu8)vec_unpackh((vs8)vec_input0), vec_mask);
			vu16 vec_lower0 = (vu16)vec_and((vu8)vec_unpackl((vs8)vec_input0), vec_mask);
			vu16 vec_upper1 = (vu16)vec_and((vu8)vec_unpackh((vs8)vec_input1), vec_mask);
			vu16 vec_lower1 = (vu16)vec_and((vu8)vec_unpackl((vs8)vec_input1), vec_mask);

			// ((input * invAlpha) + (color * Alpha)) >> 8
			vec_upper0 = (vu16)vec_sr(vec_mladd(vec_upper0, vec_invalpha, vec_alphacolor), vec_eight);
			vec_lower0 = (vu16)vec_sr(vec_mladd(vec_lower0, vec_invalpha, vec_alphacolor), vec_eight);
			vec_upper1 = (vu16)vec_sr(vec_mladd(vec_upper1, vec_invalpha, vec_alphacolor), vec_eight);
			vec_lower1 = (vu16)vec_sr(vec_mladd(vec_lower1, vec_invalpha, vec_alphacolor), vec_eight);

			// Compress the width of each color channel to 8-bits again
			vu32 vec_output0 = (vu32)vec_packsu(vec_upper0, vec_lower0);
			vu32 vec_output1 = (vu32)vec_packsu(vec_upper1, vec_lower1);

			// Store in dest
			vec_st(vec_output0, 0, (uint32_t*)dest);
			vec_st(vec_output1, 16, (uint32_t*)dest);

			dest += batch_size;
		}

		// Pick up the remainder:
		while (remainder--)
		{
			*dest = alphablend1a(*dest, color, alpha);
			dest++;
		}

		dest += line_inc;
	}
}

VERSION_CONTROL (r_drawt_altivec_cpp, "$Id$")

#endif
