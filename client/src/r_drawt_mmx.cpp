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

// With MMX we can process 4 16-bit words at a time.

// Blend 2 colors against 1 color using MMX:
#define blend2vs1_mmx(input, blendMult, blendInvAlpha, upper8mask) \
	(_mm_packs_pu16( \
		_mm_srli_pi16( \
			_mm_add_pi16( \
				_mm_mullo_pi16( \
					_mm_and_si64(_mm_unpacklo_pi8(input, input), upper8mask), \
					blendInvAlpha \
				), \
				blendMult \
			), \
			8 \
		), \
		_mm_srli_pi16( \
			_mm_add_pi16( \
				_mm_mullo_pi16( \
					_mm_and_si64(_mm_unpackhi_pi8(input, input), upper8mask), \
					blendInvAlpha \
				), \
				blendMult \
			), \
			8 \
		) \
	))

// Direct rendering (32-bit) functions for MMX optimization:

//
// R_SetM64
//
// Sets an __m64 MMX register with the given color channel values.
// The surface is queried for the pixel format and the color channel values
// are set in the appropriate order for the pixel format.
//
static inline __m64 R_SetM64(const IWindowSurface* surface, int a, int r, int g, int b)
{
	// determine the layout of the color channels in memory
	const PixelFormat* format = surface->getPixelFormat();
	int apos = (24 - format->getAShift()) >> 3;
	int rpos = (24 - format->getRShift()) >> 3;
	int gpos = (24 - format->getGShift()) >> 3;
	int bpos = (24 - format->getBShift()) >> 3;

	uint16_t values[4];
	values[apos] = a; values[rpos] = r; values[gpos] = g; values[bpos] = b;

	return _mm_set_pi16(values[0], values[1], values[2], values[3]);
}


void r_dimpatchD_MMX(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int surface_pitch_pixels = surface->getPitchInPixels();

	argb_t* line = (argb_t*)surface->getBuffer() + y1 * surface_pitch_pixels;

	int batches = w / 2;
	int remainder = w & 1;

	// MMX temporaries:
	const __m64 upper8mask		= R_SetM64(surface, 0, 0xFF, 0xFF, 0xFF);
	const __m64 blendAlpha		= R_SetM64(surface, 0, alpha, alpha, alpha);
	const __m64 blendInvAlpha	= R_SetM64(surface, 0, 256 - alpha, 256 - alpha, 256 - alpha);
	const __m64 blendColor		= R_SetM64(surface, 0, color.getr(), color.getg(), color.getb()); 
	const __m64 blendMult		= _mm_mullo_pi16(blendColor, blendAlpha);

	for (int y = y1; y < y1 + h; y++)
	{
		int x = x1;

		// MMX optimize the bulk in batches of 2 colors:
		for (int i = 0; i < batches; ++i, x += 2)
		{
			#if 1
			const __m64 input = _mm_setr_pi32(line[x + 0], line[x + 1]);
			#else
			// NOTE(jsd): No guarantee of 64-bit alignment; cannot use.
			const __m64 input = *((__m64 *)line[x]);
			#endif

			const __m64 output = blend2vs1_mmx(input, blendMult, blendInvAlpha, upper8mask);
			#if 1
			line[x+0] = _mm_cvtsi64_si32(_mm_srli_si64(output, 32*0));
			line[x+1] = _mm_cvtsi64_si32(_mm_srli_si64(output, 32*1));
			#else
			// NOTE(jsd): No guarantee of 64-bit alignment; cannot use.
			*((__m64 *)line[x]) = output;
			#endif
		}

		if (remainder)
		{
			// Pick up the remainder:
			for (; x < x1 + w; x++)
				line[x] = alphablend1a(line[x], color, alpha);
		}

		line += surface_pitch_pixels;
	}

	// Required to reset FP:
	_mm_empty();
}

VERSION_CONTROL (r_drawt_mmx_cpp, "$Id$")

#endif
