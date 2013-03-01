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
#include "r_things.h"
#include "v_video.h"

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

template<>
void rtv_lucent4cols_MMX(byte *source, argb_t *dest, int bga, int fga)
{
	// SSE2 temporaries:
	const __m64 upper8mask = _mm_set_pi16(0, 0xff, 0xff, 0xff);
	const __m64 fgAlpha = _mm_set_pi16(0, fga, fga, fga);
	const __m64 bgAlpha = _mm_set_pi16(0, bga, bga, bga);

#if 1
	const __m64 bgColors01 = _mm_setr_pi32(dest[0], dest[1]);
#else
	const __m64 bgColors01 = *((__m64 *)&dest[0]);
#endif
	const __m64 fgColors01 = _mm_setr_pi32(
		rt_mapcolor<argb_t>(dc_colormap, source[0]),
		rt_mapcolor<argb_t>(dc_colormap, source[1])
	);

	const __m64 finalColors01 = _mm_packs_pu16(
		_mm_srli_pi16(
			_mm_adds_pi16(
				_mm_mullo_pi16(_mm_and_si64(_mm_unpacklo_pi8(bgColors01, bgColors01), upper8mask), bgAlpha),
				_mm_mullo_pi16(_mm_and_si64(_mm_unpacklo_pi8(fgColors01, fgColors01), upper8mask), fgAlpha)
			),
			8
		),
		_mm_srli_pi16(
			_mm_adds_pi16(
				_mm_mullo_pi16(_mm_and_si64(_mm_unpackhi_pi8(bgColors01, bgColors01), upper8mask), bgAlpha),
				_mm_mullo_pi16(_mm_and_si64(_mm_unpackhi_pi8(fgColors01, fgColors01), upper8mask), fgAlpha)
			),
			8
		)
	);

#if 1
	const __m64 bgColors23 = _mm_setr_pi32(dest[2], dest[3]);
#else
	// NOTE(jsd): No guarantee of 64-bit alignment; cannot use.
	const __m64 bgColors23 = *((__m64 *)&dest[2]);
#endif
	const __m64 fgColors23 = _mm_setr_pi32(
		rt_mapcolor<argb_t>(dc_colormap, source[2]),
		rt_mapcolor<argb_t>(dc_colormap, source[3])
	);

	const __m64 finalColors23 = _mm_packs_pu16(
		_mm_srli_pi16(
			_mm_adds_pi16(
				_mm_mullo_pi16(_mm_and_si64(_mm_unpacklo_pi8(bgColors23, bgColors23), upper8mask), bgAlpha),
				_mm_mullo_pi16(_mm_and_si64(_mm_unpacklo_pi8(fgColors23, fgColors23), upper8mask), fgAlpha)
			),
			8
		),
		_mm_srli_pi16(
			_mm_adds_pi16(
				_mm_mullo_pi16(_mm_and_si64(_mm_unpackhi_pi8(bgColors23, bgColors23), upper8mask), bgAlpha),
				_mm_mullo_pi16(_mm_and_si64(_mm_unpackhi_pi8(fgColors23, fgColors23), upper8mask), fgAlpha)
			),
			8
		)
	);
	
#if 1
	dest[0] = _mm_cvtsi64_si32(_mm_srli_si64(finalColors01, 32*0));
	dest[1] = _mm_cvtsi64_si32(_mm_srli_si64(finalColors01, 32*1));
	dest[2] = _mm_cvtsi64_si32(_mm_srli_si64(finalColors23, 32*0));
	dest[3] = _mm_cvtsi64_si32(_mm_srli_si64(finalColors23, 32*1));
#else
	// NOTE(jsd): No guarantee of 64-bit alignment; cannot use.
	*((__m64 *)&dest[0]) = finalColors01;
	*((__m64 *)&dest[2]) = finalColors23;
#endif

	// Required to reset FP:
	_mm_empty();
}

template<>
void rtv_lucent4cols_MMX(byte *source, palindex_t *dest, int bga, int fga)
{
	for (int i = 0; i < 4; ++i)
	{
		const palindex_t fg = rt_mapcolor<palindex_t>(dc_colormap, source[i]);
		const palindex_t bg = dest[i];

		dest[i] = rt_blend2<palindex_t>(bg, bga, fg, fga);
	}
}

void r_dimpatchD_MMX(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int x, y, i;
	argb_t *line;
	int invAlpha = 256 - alpha;

	int dpitch = cvs->pitch / sizeof(DWORD);
	line = (argb_t *)cvs->buffer + y1 * dpitch;

	int batches = w / 2;
	int remainder = w & 1;

	// MMX temporaries:
	const __m64 upper8mask = _mm_set_pi16(0, 0xff, 0xff, 0xff);
	const __m64 blendAlpha = _mm_set_pi16(0, alpha, alpha, alpha);
	const __m64 blendInvAlpha = _mm_set_pi16(0, invAlpha, invAlpha, invAlpha);
	const __m64 blendColor = _mm_set_pi16(0, RPART(color), GPART(color), BPART(color));
	const __m64 blendMult = _mm_mullo_pi16(blendColor, blendAlpha);

	for (y = y1; y < y1 + h; y++)
	{
		// MMX optimize the bulk in batches of 2 colors:
		for (i = 0, x = x1; i < batches; ++i, x += 2)
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
			{
				line[x] = alphablend1a(line[x], color, alpha);
			}
		}

		line += dpitch;
	}

	// Required to reset FP:
	_mm_empty();
}

VERSION_CONTROL (r_drawt_mmx_cpp, "$Id$")

#endif
