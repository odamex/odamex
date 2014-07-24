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

#if !defined(__APPLE_ALTIVEC__)
#include <altivec.h>
#endif

#define ALTIVEC_ALIGNED(x) x __attribute__((aligned(16)))

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "i_video.h"

// Useful vector shorthand typedefs:
typedef vector signed char vs8;
typedef vector unsigned char vu8;
typedef vector unsigned short vu16;
typedef vector unsigned int vu32;

// Direct rendering (32-bit) functions for ALTIVEC optimization:

void r_dimpatchD_ALTIVEC(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int surface_pitch_pixels = surface->getPitchInPixels();
	int line_inc = surface_pitch_pixels - w;

	// ALTIVEC temporaries:
	const vu8 vec_zero				= (vu8)vec_splat_u8(0);
	const vu16 vec_color			= (vu16)vec_unpackl((vu8)vec_splat_u32(color), vec_zero);
	const vu16 vec_alphacolor		= (vu16)vec_mladd((vu16)vec_color, (vu16)vec_splat_u16(alpha), (vu16)vec_zero);
	const vu16 vec_invalpha			= (vu16)vec_splat_u16(256 - alpha);

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

		// SSE2 optimize the bulk in batches of 8 pixels:
		while (batches--)
		{
			// Load 4 pixels into input0 and 4 pixels into input1
			const vu32 vec_input0 = { dest[0], dest[1], dest[2], dest[3] };
			const vu32 vec_input1 = { dest[4], dest[5], dest[6], dest[7] };

			// Expand the width of each color channel from 8-bits to 16-bits
			// by splitting each input vector into two 128-bit variables, each
			// containing 2 ARGB values. 16-bit color channels are needed to
			// accomodate multiplication.
			vu16 vec_lower0 = (vu16)vec_unpackl((vu8)vec_input0, (vu8)vec_zero);
			vu16 vec_upper0 = (vu16)vec_unpackh((vu8)vec_input0, (vu8)vec_zero);
			vu16 vec_lower1 = (vu16)vec_unpackl((vu8)vec_input1, (vu8)vec_zero);
			vu16 vec_upper1 = (vu16)vec_unpackh((vu8)vec_input1, (vu8)vec_zero);

			// ((input * invAlpha) + (color * Alpha)) >> 8
			vec_lower0 = (vu16)vec_sr(vec_mladd(vec_lower0, vec_invalpha, vec_alphacolor), vec_splat_u16(8));
			vec_upper0 = (vu16)vec_sr(vec_mladd(vec_upper0, vec_invalpha, vec_alphacolor), vec_splat_u16(8));
			vec_lower1 = (vu16)vec_sr(vec_mladd(vec_lower1, vec_invalpha, vec_alphacolor), vec_splat_u16(8));
			vec_upper1 = (vu16)vec_sr(vec_mladd(vec_upper1, vec_invalpha, vec_alphacolor), vec_splat_u16(8));

			// Compress the width of each color channel to 8-bits again
			vu8 vec_output0 = (vu8)vec_packsu(vec_lower0, vec_upper0);
			vu8 vec_output1 = (vu8)vec_packsu(vec_lower1, vec_upper1);

			// Store in dest
			vec_ste(output0, 0, dest + 0);
			vec_ste(output0, 4, dest + 0);
			vec_ste(output0, 8, dest + 0);
			vec_ste(output0, 12, dest + 0);
			vec_ste(output1, 0, dest + 4);
			vec_ste(output1, 4, dest + 4);
			vec_ste(output1, 8, dest + 4);
			vec_ste(output1, 12, dest + 4);

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
