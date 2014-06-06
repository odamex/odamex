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

// Blend 4 colors against 1 color with alpha
#define blend4vs1_altivec(input,blendMult,blendInvAlpha,upper8mask) \
	((vu8)vec_packsu( \
		(vu16)vec_sr( \
			(vu16)vec_mladd( \
				(vu16)vec_and((vu8)vec_unpackh((vs8)input), (vu8)upper8mask), \
				(vu16)blendInvAlpha, \
				(vu16)blendMult \
			), \
			(vu16)vec_splat_u16(8) \
		), \
		(vu16)vec_sr( \
			(vu16)vec_mladd( \
				(vu16)vec_and((vu8)vec_unpackl((vs8)input), (vu8)upper8mask), \
				(vu16)blendInvAlpha, \
				(vu16)blendMult \
			), \
			(vu16)vec_splat_u16(8) \
		) \
	))

// Direct rendering (32-bit) functions for ALTIVEC optimization:



void r_dimpatchD_ALTIVEC(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int surface_width = surface->getWidth(), surface_height = surface->getHeight();
	int surface_pitch_pixels = surface->getPitchInPixels();

	argb_t* line = (argb_t*)surface->getBuffer() + y1 * surface_pitch_pixels;

	int batches = w / 4;
	int remainder = w & 3;

	// determine the layout of the color channels in memory
	const PixelFormat* format = surface->getPixelFormat();
	int apos = (24 - format->getAShift()) >> 3;
	int rpos = (24 - format->getRShift()) >> 3;
	int gpos = (24 - format->getGShift()) >> 3;
	int bpos = (24 - format->getBShift()) >> 3;

	uint16_t values[4];

	// AltiVec temporaries:
	const vu16 zero				= { 0, 0, 0, 0, 0, 0, 0, 0 };

	values[apos] = 0; values[rpos] = values[gpos] = values[bpos] = 0xFF;
	const vu16 upper8mask		= { values[0], values[1], values[2], values[3],
									values[0], values[1], values[2], values[3] };

	values[apos] = 0; values[rpos] = values[gpos] = values[bpos] = alpha;
	const vu16 blendAlpha		= { values[0], values[1], values[2], values[3],
									values[0], values[1], values[2], values[3] };

	values[apos] = 0; values[rpos] = values[gpos] = values[bpos] = 256 - alpha;
	const vu16 blendInvAlpha		= { values[0], values[1], values[2], values[3],
									values[0], values[1], values[2], values[3] };
	
	values[apos] = 0; values[rpos] = color.getr(), values[gpos] = color.getg(), values[bpos] = color.getb();
	const vu16 blendColor		= { values[0], values[1], values[2], values[3],
									values[0], values[1], values[2], values[3] };

	const vu16 blendMult		= vec_mladd(blendColor, blendAlpha, zero);

	for (int y = y1; y < y1 + h; y++)
	{
		int x = x1;

		// AltiVec optimize the bulk in batches of 4 colors:
		for (int i = 0; i < batches; ++i, x += 4)
		{
			const vu32 input = {line[x + 0], line[x + 1], line[x + 2], line[x + 3]};
			const vu32 output = (vu32)blend4vs1_altivec(input, blendMult, blendInvAlpha, upper8mask);
			vec_ste(output, 0, &line[x]);
			vec_ste(output, 4, &line[x]);
			vec_ste(output, 8, &line[x]);
			vec_ste(output, 12, &line[x]);
		}

		if (remainder)
		{
			// Pick up the remainder:
			for (; x < x1 + w; x++)
				line[x] = alphablend1a(line[x], color, alpha);
		}

		line += surface_pitch_pixels;
	}
}

VERSION_CONTROL (r_drawt_altivec_cpp, "$Id$")

#endif
