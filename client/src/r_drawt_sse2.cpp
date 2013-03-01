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

#ifdef __SSE2__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <emmintrin.h>

#ifdef _MSC_VER
#define SSE2_ALIGNED(x) _CRT_ALIGN(16) x
#else
#define SSE2_ALIGNED(x) x __attribute__((aligned(16)))
#endif

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"

// SSE2 alpha-blending functions.
// NOTE(jsd): We can only blend two colors per 128-bit register because we need 16-bit resolution for multiplication.

// Blend 4 colors against 1 color using SSE2:
#define blend4vs1_sse2(input,blendMult,blendInvAlpha,upper8mask) \
	(_mm_packus_epi16( \
		_mm_srli_epi16( \
			_mm_adds_epu16( \
				_mm_mullo_epi16( \
					_mm_and_si128(_mm_unpacklo_epi8(input, input), upper8mask), \
					blendInvAlpha \
				), \
				blendMult \
			), \
			8 \
		), \
		_mm_srli_epi16( \
			_mm_adds_epu16( \
				_mm_mullo_epi16( \
					_mm_and_si128(_mm_unpackhi_epi8(input, input), upper8mask), \
					blendInvAlpha \
				), \
				blendMult \
			), \
			8 \
		) \
	))

// Direct rendering (32-bit) functions for SSE2 optimization:

template<>
void rtv_lucent4cols_SSE2(byte *source, argb_t *dest, int bga, int fga)
{
	// SSE2 temporaries:
	const __m128i upper8mask = _mm_set_epi16(0, 0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff);
	const __m128i fgAlpha = _mm_set_epi16(0, fga, fga, fga, 0, fga, fga, fga);
	const __m128i bgAlpha = _mm_set_epi16(0, bga, bga, bga, 0, bga, bga, bga);

	const __m128i bgColors = _mm_loadu_si128((__m128i *)dest);
	const __m128i fgColors = _mm_setr_epi32(
		rt_mapcolor<argb_t>(dc_colormap, source[0]),
		rt_mapcolor<argb_t>(dc_colormap, source[1]),
		rt_mapcolor<argb_t>(dc_colormap, source[2]),
		rt_mapcolor<argb_t>(dc_colormap, source[3])
	);

	const __m128i finalColors = _mm_packus_epi16(
		_mm_srli_epi16(
			_mm_adds_epu16(
				_mm_mullo_epi16(_mm_and_si128(_mm_unpacklo_epi8(bgColors, bgColors), upper8mask), bgAlpha),
				_mm_mullo_epi16(_mm_and_si128(_mm_unpacklo_epi8(fgColors, fgColors), upper8mask), fgAlpha)
			),
			8
		),
		_mm_srli_epi16(
			_mm_adds_epu16(
				_mm_mullo_epi16(_mm_and_si128(_mm_unpackhi_epi8(bgColors, bgColors), upper8mask), bgAlpha),
				_mm_mullo_epi16(_mm_and_si128(_mm_unpackhi_epi8(fgColors, fgColors), upper8mask), fgAlpha)
			),
			8
		)
	);

	_mm_storeu_si128((__m128i *)dest, finalColors);
}

template<>
void rtv_lucent4cols_SSE2(byte *source, palindex_t *dest, int bga, int fga)
{
	for (int i = 0; i < 4; ++i)
	{
		const palindex_t fg = rt_mapcolor<palindex_t>(dc_colormap, source[i]);
		const palindex_t bg = dest[i];

		dest[i] = rt_blend2<palindex_t>(bg, bga, fg, fga);
	}
}

void R_DrawSpanD_SSE2 (void)
{
	dsfixed_t			xfrac;
	dsfixed_t			yfrac;
	dsfixed_t			xstep;
	dsfixed_t			ystep;
	argb_t*             dest;
	int 				count;
	int 				spot;

#ifdef RANGECHECK
	if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=screen->width
		|| ds_y>screen->height)
	{
		I_Error ("R_DrawSpan: %i to %i at %i",
				 ds_x1,ds_x2,ds_y);
	}
//		dscount++;
#endif

	xfrac = ds_xfrac;
	yfrac = ds_yfrac;

	dest = (argb_t *)(ylookup[ds_y] + columnofs[ds_x1]);

	// We do not check for zero spans here?
	count = ds_x2 - ds_x1 + 1;

	xstep = ds_xstep;
	ystep = ds_ystep;

	// NOTE(jsd): Can this just die already?
	assert(ds_colsize == 1);

	// Blit until we align ourselves with a 16-byte offset for SSE2:
	while (((size_t)dest) & 15)
	{
		// Current texture index in u,v.
		spot = ((yfrac>>(32-6-6))&(63*64)) + (xfrac>>(32-6));

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest = ds_colormap.shade(ds_source[spot]);
		dest += ds_colsize;

		// Next step in u,v.
		xfrac += xstep;
		yfrac += ystep;

		--count;
	}

	const int rounds = count / 4;
	if (rounds > 0)
	{
		// SSE2 optimized and aligned stores for quicker memory writes:
		for (int i = 0; i < rounds; ++i, count -= 4)
		{
			// TODO(jsd): Consider SSE2 bit twiddling here!
			const int spot0 = (((yfrac+ystep*0)>>(32-6-6))&(63*64)) + ((xfrac+xstep*0)>>(32-6));
			const int spot1 = (((yfrac+ystep*1)>>(32-6-6))&(63*64)) + ((xfrac+xstep*1)>>(32-6));
			const int spot2 = (((yfrac+ystep*2)>>(32-6-6))&(63*64)) + ((xfrac+xstep*2)>>(32-6));
			const int spot3 = (((yfrac+ystep*3)>>(32-6-6))&(63*64)) + ((xfrac+xstep*3)>>(32-6));

			const __m128i finalColors = _mm_setr_epi32(
				ds_colormap.shade(ds_source[spot0]),
				ds_colormap.shade(ds_source[spot1]),
				ds_colormap.shade(ds_source[spot2]),
				ds_colormap.shade(ds_source[spot3])
			);
			_mm_store_si128((__m128i *)dest, finalColors);
			dest += 4;

			// Next step in u,v.
			xfrac += xstep*4;
			yfrac += ystep*4;
		}
	}

	if (count > 0)
	{
		// Blit the last remainder:
		while (count--)
		{
			// Current texture index in u,v.
			spot = ((yfrac>>(32-6-6))&(63*64)) + (xfrac>>(32-6));

			// Lookup pixel from flat texture tile,
			//  re-index using light/colormap.
			*dest = ds_colormap.shade(ds_source[spot]);
			dest += ds_colsize;

			// Next step in u,v.
			xfrac += xstep;
			yfrac += ystep;
		}
	}
}

void R_DrawSlopeSpanD_SSE2 (void)
{
	int count = ds_x2 - ds_x1 + 1;
	if (count <= 0)
		return;

#ifdef RANGECHECK 
	if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=screen->width
		|| ds_y>screen->height)
	{
		I_Error ("R_DrawSlopeSpan: %i to %i at %i",
				 ds_x1,ds_x2,ds_y);
	}
#endif

	double iu = ds_iu, iv = ds_iv;
	double ius = ds_iustep, ivs = ds_ivstep;
	double id = ds_id, ids = ds_idstep;
	
	// framebuffer	
	argb_t *dest = (argb_t *)( ylookup[ds_y] + columnofs[ds_x1] );
	
	// texture data
	byte *src = (byte *)ds_source;

	assert (ds_colsize == 1);
	int colsize = ds_colsize;
	int ltindex = 0;		// index into the lighting table

	// Blit the bulk in batches of SPANJUMP columns:
	while (count >= SPANJUMP)
	{
		double ustart, uend;
		double vstart, vend;
		double mulstart, mulend;
		unsigned int ustep, vstep, ufrac, vfrac;
		int incount;

		mulstart = 65536.0f / id;
		id += ids * SPANJUMP;
		mulend = 65536.0f / id;

		ufrac = (int)(ustart = iu * mulstart);
		vfrac = (int)(vstart = iv * mulstart);
		iu += ius * SPANJUMP;
		iv += ivs * SPANJUMP;
		uend = iu * mulend;
		vend = iv * mulend;

		ustep = (int)((uend - ustart) * INTERPSTEP);
		vstep = (int)((vend - vstart) * INTERPSTEP);

		incount = SPANJUMP;

		// Blit up to the first 16-byte aligned position:
		while ((((size_t)dest) & 15) && (incount > 0))
		{
			const shaderef_t &colormap = slopelighting[ltindex++];
			*dest = colormap.shade(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest += colsize;
			ufrac += ustep;
			vfrac += vstep;
			incount--;
		}

		if (incount > 0)
		{
			const int rounds = incount / 4;
			if (rounds > 0)
			{
				for (int i = 0; i < rounds; ++i, incount -= 4)
				{
					const int spot0 = (((vfrac+vstep*0) >> 10) & 0xFC0) | (((ufrac+ustep*0) >> 16) & 63);
					const int spot1 = (((vfrac+vstep*1) >> 10) & 0xFC0) | (((ufrac+ustep*1) >> 16) & 63);
					const int spot2 = (((vfrac+vstep*2) >> 10) & 0xFC0) | (((ufrac+ustep*2) >> 16) & 63);
					const int spot3 = (((vfrac+vstep*3) >> 10) & 0xFC0) | (((ufrac+ustep*3) >> 16) & 63);

					const __m128i finalColors = _mm_setr_epi32(
						slopelighting[ltindex+0].shade(src[spot0]),
						slopelighting[ltindex+1].shade(src[spot1]),
						slopelighting[ltindex+2].shade(src[spot2]),
						slopelighting[ltindex+3].shade(src[spot3])
					);
					_mm_store_si128((__m128i *)dest, finalColors);

					dest += 4;
					ltindex += 4;

					ufrac += ustep * 4;
					vfrac += vstep * 4;
				}
			}
		}

		if (incount > 0)
		{
			while(incount--)
			{
				const shaderef_t &colormap = slopelighting[ltindex++];
				const int spot = ((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63);
				*dest = colormap.shade(src[spot]);
				dest += colsize;

				ufrac += ustep;
				vfrac += vstep;
			}
		}

		count -= SPANJUMP;
	}

	// Remainder:
	assert(count < SPANJUMP);
	if (count > 0)
	{
		double ustart, uend;
		double vstart, vend;
		double mulstart, mulend;
		unsigned int ustep, vstep, ufrac, vfrac;
		int incount;

		mulstart = 65536.0f / id;
		id += ids * count;
		mulend = 65536.0f / id;

		ufrac = (int)(ustart = iu * mulstart);
		vfrac = (int)(vstart = iv * mulstart);
		iu += ius * count;
		iv += ivs * count;
		uend = iu * mulend;
		vend = iv * mulend;

		ustep = (int)((uend - ustart) / count);
		vstep = (int)((vend - vstart) / count);

		incount = count;
		while (incount--)
		{
			const shaderef_t &colormap = slopelighting[ltindex++];
			*dest = colormap.shade(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest += colsize;
			ufrac += ustep;
			vfrac += vstep;
		}
	}
}

void r_dimpatchD_SSE2(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int x, y, i;
	argb_t *line;
	int invAlpha = 256 - alpha;

	int dpitch = cvs->pitch / sizeof(argb_t);
	line = (argb_t *)cvs->buffer + y1 * dpitch;

	int batches = w / 4;
	int remainder = w & 3;

	// SSE2 temporaries:
	const __m128i upper8mask = _mm_set_epi16(0, 0xff, 0xff, 0xff, 0, 0xff, 0xff, 0xff);
	const __m128i blendAlpha = _mm_set_epi16(0, alpha, alpha, alpha, 0, alpha, alpha, alpha);
	const __m128i blendInvAlpha = _mm_set_epi16(0, invAlpha, invAlpha, invAlpha, 0, invAlpha, invAlpha, invAlpha);
	const __m128i blendColor = _mm_set_epi16(0, RPART(color), GPART(color), BPART(color), 0, RPART(color), GPART(color), BPART(color));
	const __m128i blendMult = _mm_mullo_epi16(blendColor, blendAlpha);

	for (y = y1; y < y1 + h; y++)
	{
		// SSE2 optimize the bulk in batches of 4 colors:
		for (i = 0, x = x1; i < batches; ++i, x += 4)
		{
			const __m128i input = _mm_setr_epi32(line[x + 0], line[x + 1], line[x + 2], line[x + 3]);
			_mm_storeu_si128((__m128i *)&line[x], blend4vs1_sse2(input, blendMult, blendInvAlpha, upper8mask));
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

VERSION_CONTROL (r_drawt_sse2_cpp, "$Id$")

#endif
