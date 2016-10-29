// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//
//-----------------------------------------------------------------------------

#include "i_sdl.h"
#include "r_intrin.h"

#ifdef __MMX__

// NOTE(jsd): Do not consider MMX deprecated so lightly. The XBOX and other older systems still make use of it.

#include <stdio.h>
#include <stdlib.h>
#include <mmintrin.h>

#ifdef _MSC_VER
#define MMX_ALIGNED(x) _CRT_ALIGN(8) x
#else
#define MMX_ALIGNED(x) x __attribute__((aligned(8)))
#endif

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "i_video.h"


// Direct rendering (32-bit) functions for MMX optimization:

//
// R_GetBytesUntilAligned
//
static inline uintptr_t R_GetBytesUntilAligned(void* data, uintptr_t alignment)
{
	uintptr_t mask = alignment - 1;
	return (alignment - ((uintptr_t)data & mask)) & mask;
}


void r_dimpatchD_MMX(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int surface_pitch_pixels = surface->getPitchInPixels();
	int line_inc = surface_pitch_pixels - w;

	argb_t* dest = (argb_t*)surface->getBuffer() + y1 * surface_pitch_pixels + x1;

	// MMX temporaries:
	const __m64 vec_color		= _mm_unpacklo_pi8(_mm_set1_pi32(color), _mm_setzero_si64());
	const __m64 vec_alphacolor	= _mm_mullo_pi16(vec_color, _mm_set1_pi16(alpha));
	const __m64 vec_invalpha	= _mm_set1_pi16(256 - alpha);

	for (int rowcount = h; rowcount > 0; --rowcount)
	{
		// [SL] Calculate how many pixels of each row need to be drawn before dest is
		// aligned to a 64-bit boundary.
		int align = R_GetBytesUntilAligned(dest, 64/8) / sizeof(argb_t);
		if (align > w)
			align = w;

		const int batch_size = 4;
		int batches = (w - align) / batch_size;
		int remainder = (w - align) & (batch_size - 1);

		// align the destination buffer to 64-bit boundary
		while (align--)
		{
			*dest = alphablend1a(*dest, color, alpha);
			dest++;
		}

		// MMX optimize the bulk in batches of 4 pixels:
		while (batches--)
		{
			// Load 2 pixels into input0 and 2 pixels into input1
			const __m64 vec_input0 = *((__m64*)(dest + 0));
			const __m64 vec_input1 = *((__m64*)(dest + 2));

			// Expand the width of each color channel from 8-bits to 16-bits
			// by splitting each input vector into two 64-bit variables, each
			// containing 1 ARGB value. 16-bit color channels are needed to
			// accomodate multiplication.
			__m64 vec_lower0 = _mm_unpacklo_pi8(vec_input0, _mm_setzero_si64());
			__m64 vec_upper0 = _mm_unpackhi_pi8(vec_input0, _mm_setzero_si64());
			__m64 vec_lower1 = _mm_unpacklo_pi8(vec_input1, _mm_setzero_si64());
			__m64 vec_upper1 = _mm_unpackhi_pi8(vec_input1, _mm_setzero_si64());

			// ((input * invAlpha) + (color * Alpha)) >> 8
			vec_lower0 = _mm_srli_pi16(_mm_add_pi16(_mm_mullo_pi16(vec_lower0, vec_invalpha), vec_alphacolor), 8); 
			vec_upper0 = _mm_srli_pi16(_mm_add_pi16(_mm_mullo_pi16(vec_upper0, vec_invalpha), vec_alphacolor), 8); 
			vec_lower1 = _mm_srli_pi16(_mm_add_pi16(_mm_mullo_pi16(vec_lower1, vec_invalpha), vec_alphacolor), 8); 
			vec_upper1 = _mm_srli_pi16(_mm_add_pi16(_mm_mullo_pi16(vec_upper1, vec_invalpha), vec_alphacolor), 8); 

			// Compress the width of each color channel to 8-bits again and store in dest
			*((__m64*)(dest + 0)) = _mm_packs_pu16(vec_lower0, vec_upper0);
			*((__m64*)(dest + 2)) = _mm_packs_pu16(vec_lower1, vec_upper1);

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

	// Required to reset FP:
	_mm_empty();
}

VERSION_CONTROL (r_drawt_mmx_cpp, "$Id$")

#endif
