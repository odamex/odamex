// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Functions for drawing columns into a temporary buffer and then
//	copying them to the screen. On machines with a decent cache, this
//	is faster than drawing them directly to the screen. Will I be able
//	to even understand any of this if I come back to it later? Let's
//	hope so. :-)
//
//-----------------------------------------------------------------------------

#include "SDL_cpuinfo.h"
#include "r_intrin.h"

#ifdef __ALTIVEC__

#include <stdio.h>
#include <stdlib.h>

// Compile on g++-4.0.1 with -faltivec, not -maltivec
#include <altivec.h>

#define ALTIVEC_ALIGNED(x) x __attribute__((aligned(16)))

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"

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

void r_dimpatchD_ALTIVEC(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int x, y, i;
	argb_t *line;
	int invAlpha = 256 - alpha;

	int dpitch = cvs->pitch / sizeof(argb_t);
	line = (argb_t *)cvs->buffer + y1 * dpitch;

	int batches = w / 4;
	int remainder = w & 3;

	// AltiVec temporaries:
	const vu16 zero = {0, 0, 0, 0, 0, 0, 0, 0};
	const vu16 upper8mask = {0, 0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff};
	const vu16 blendAlpha = {0, alpha, alpha, alpha, 0, alpha, alpha, alpha};
	const vu16 blendInvAlpha = {0, invAlpha, invAlpha, invAlpha, 0, invAlpha, invAlpha, invAlpha};
	const vu16 blendColor = {0, RPART(color), GPART(color), BPART(color), 0, RPART(color), GPART(color), BPART(color)};
	const vu16 blendMult = vec_mladd(blendColor, blendAlpha, zero);

	for (y = y1; y < y1 + h; y++)
	{
		// AltiVec optimize the bulk in batches of 4 colors:
		for (i = 0, x = x1; i < batches; ++i, x += 4)
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
			{
				line[x] = alphablend1a(line[x], color, alpha);
			}
		}

		line += dpitch;
	}
}

VERSION_CONTROL (r_drawt_altivec_cpp, "$Id$")

#endif
