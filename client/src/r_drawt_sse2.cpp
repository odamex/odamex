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
#include "i_video.h"

// Direct rendering (32-bit) functions for SSE2 optimization:

//
// R_GetBytesUntilAligned
//
static inline uintptr_t R_GetBytesUntilAligned(void* data, uintptr_t alignment)
{
	uintptr_t mask = alignment - 1;
	return (alignment - ((uintptr_t)data & mask)) & mask;
}


void R_DrawSpanD_SSE2 (void)
{
#ifdef RANGECHECK
	if (dspan.x2 < dspan.x1 || dspan.x1 < 0 || dspan.x2 >= viewwidth ||
		dspan.y >= viewheight || dspan.y < 0)
	{
		Printf(PRINT_HIGH, "R_DrawLevelSpan: %i to %i at %i", dspan.x1, dspan.x2, dspan.y);
		return;
	}
#endif

	const int width = dspan.x2 - dspan.x1 + 1;

	// TODO: store flats in column-major format and swap u and v
	dsfixed_t ufrac = dspan.yfrac;
	dsfixed_t vfrac = dspan.xfrac;
	dsfixed_t ustep = dspan.ystep;
	dsfixed_t vstep = dspan.xstep;

	const byte* source = dspan.source;
	argb_t* dest = (argb_t*)dspan.destination + dspan.y * dspan.pitch_in_pixels + dspan.x1;

	shaderef_t colormap = dspan.colormap;
	
	const int texture_width_bits = 6, texture_height_bits = 6;

	const unsigned int umask = ((1 << texture_width_bits) - 1) << texture_height_bits;
	const unsigned int vmask = (1 << texture_height_bits) - 1;
	// TODO: don't shift the values of ufrac and vfrac by 10 in R_MapLevelPlane
	const int ushift = FRACBITS - texture_height_bits + 10;
	const int vshift = FRACBITS + 10;

	int align = R_GetBytesUntilAligned(dest, 16) / sizeof(argb_t);
	if (align > width)
		align = width;

	int batches = (width - align) / 4;
	int remainder = (width - align) & 3;

	// Blit until we align ourselves with a 16-byte offset for SSE2:
	while (align--)
	{
		// Current texture index in u,v.
		const unsigned int spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask); 

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest = colormap.shade(source[spot]);
		dest++;

		// Next step in u,v.
		ufrac += ustep;
		vfrac += vstep;
	}

	// SSE2 optimized and aligned stores for quicker memory writes:
	const __m128i mumask = _mm_set1_epi32(umask);
	const __m128i mvmask = _mm_set1_epi32(vmask);

	__m128i mufrac = _mm_setr_epi32(ufrac+ustep*0, ufrac+ustep*1, ufrac+ustep*2, ufrac+ustep*3);
	const __m128i mufracinc = _mm_set1_epi32(ustep*4);
	__m128i mvfrac = _mm_setr_epi32(vfrac+vstep*0, vfrac+vstep*1, vfrac+vstep*2, vfrac+vstep*3);
	const __m128i mvfracinc = _mm_set1_epi32(vstep*4);

	while (batches--)
	{
//		[SL] The below SSE2 intrinsics are equivalent to the following block:
//		const int spot0 = (((ufrac + ustep*0) >> ushift) & umask) | (((vfrac + vstep*0) >> vshift) & vmask); 
//		const int spot1 = (((ufrac + ustep*1) >> ushift) & umask) | (((vfrac + vstep*1) >> vshift) & vmask); 
//		const int spot2 = (((ufrac + ustep*2) >> ushift) & umask) | (((vfrac + vstep*2) >> vshift) & vmask); 
//		const int spot3 = (((ufrac + ustep*3) >> ushift) & umask) | (((vfrac + vstep*3) >> vshift) & vmask); 

		__m128i u = _mm_and_si128(_mm_srli_epi32(mufrac, ushift), mumask);
		__m128i v = _mm_and_si128(_mm_srli_epi32(mvfrac, vshift), mvmask);
		__m128i mspots = _mm_or_si128(u, v);
		unsigned int* spots = (unsigned int*)&mspots;

		// get the color of the pixels at each of the spots
		byte pixel0 = source[spots[0]];
		byte pixel1 = source[spots[1]];
		byte pixel2 = source[spots[2]];
		byte pixel3 = source[spots[3]];

		const __m128i finalColors = _mm_setr_epi32(
			colormap.shade(pixel0),
			colormap.shade(pixel1),
			colormap.shade(pixel2),
			colormap.shade(pixel3)
		);

		_mm_store_si128((__m128i*)dest, finalColors);

		dest += 4;

		mufrac = _mm_add_epi32(mufrac, mufracinc);
		mvfrac = _mm_add_epi32(mvfrac, mvfracinc);
	}

	ufrac = (dsfixed_t)((dsfixed_t*)&mufrac)[0];
	vfrac = (dsfixed_t)((dsfixed_t*)&mvfrac)[0];

	// blit the remaining 0 - 3 pixels
	while (remainder--)
	{
		// Current texture index in u,v.
		const int spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask); 

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest = colormap.shade(source[spot]);
		dest++;

		// Next step in u,v.
		ufrac += ustep;
		vfrac += vstep;
	}
}

void R_DrawSlopeSpanD_SSE2 (void)
{
	int count = dspan.x2 - dspan.x1 + 1;
	if (count <= 0)
		return;

#ifdef RANGECHECK 
	if (dspan.x2 < dspan.x1
		|| dspan.x1 < 0
		|| dspan.x2 >= I_GetSurfaceWidth()
		|| dspan.y >= I_GetSurfaceHeight())
	{
		I_Error ("R_DrawSlopeSpan: %i to %i at %i",
				 dspan.x1, dspan.x2, dspan.y);
	}
#endif

	float iu = dspan.iu, iv = dspan.iv;
	float ius = dspan.iustep, ivs = dspan.ivstep;
	float id = dspan.id, ids = dspan.idstep;
	
	// framebuffer	
	argb_t* dest = (argb_t*)dspan.destination + dspan.y * dspan.pitch_in_pixels + dspan.x1;
	
	// texture data
	byte *src = (byte *)dspan.source;

	int ltindex = 0;		// index into the lighting table

	// Blit the bulk in batches of SPANJUMP columns:
	while (count >= SPANJUMP)
	{
		const float mulstart = 65536.0f / id;
		id += ids * SPANJUMP;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = (fixed_t)ustart;
		fixed_t vfrac = (fixed_t)vstart;

		iu += ius * SPANJUMP;
		iv += ivs * SPANJUMP;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		fixed_t ustep = (fixed_t)((uend - ustart) * INTERPSTEP);
		fixed_t vstep = (fixed_t)((vend - vstart) * INTERPSTEP);

		int incount = SPANJUMP;

		// Blit up to the first 16-byte aligned position:
		while ((((size_t)dest) & 15) && (incount > 0))
		{
			const shaderef_t &colormap = dspan.slopelighting[ltindex++];
			*dest = colormap.shade(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest++;
			ufrac += ustep;
			vfrac += vstep;
			incount--;
		}

		if (incount > 0)
		{
			const int rounds = incount >> 2;
			if (rounds > 0)
			{
				for (int i = 0; i < rounds; ++i, incount -= 4)
				{
					const int spot0 = (((vfrac+vstep*0) >> 10) & 0xFC0) | (((ufrac+ustep*0) >> 16) & 63);
					const int spot1 = (((vfrac+vstep*1) >> 10) & 0xFC0) | (((ufrac+ustep*1) >> 16) & 63);
					const int spot2 = (((vfrac+vstep*2) >> 10) & 0xFC0) | (((ufrac+ustep*2) >> 16) & 63);
					const int spot3 = (((vfrac+vstep*3) >> 10) & 0xFC0) | (((ufrac+ustep*3) >> 16) & 63);

					const __m128i finalColors = _mm_setr_epi32(
						dspan.slopelighting[ltindex+0].shade(src[spot0]),
						dspan.slopelighting[ltindex+1].shade(src[spot1]),
						dspan.slopelighting[ltindex+2].shade(src[spot2]),
						dspan.slopelighting[ltindex+3].shade(src[spot3])
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
				const shaderef_t &colormap = dspan.slopelighting[ltindex++];
				const int spot = ((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63);
				*dest = colormap.shade(src[spot]);
				dest++;

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
		const float mulstart = 65536.0f / id;
		id += ids * count;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = (fixed_t)ustart;
		fixed_t vfrac = (fixed_t)vstart;

		iu += ius * count;
		iv += ivs * count;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		fixed_t ustep = (fixed_t)((uend - ustart) / count);
		fixed_t vstep = (fixed_t)((vend - vstart) / count);

		int incount = count;
		while (incount--)
		{
			const shaderef_t &colormap = dspan.slopelighting[ltindex++];
			*dest = colormap.shade(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest++;
			ufrac += ustep;
			vfrac += vstep;
		}
	}
}


void r_dimpatchD_SSE2(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int surface_pitch_pixels = surface->getPitchInPixels();
	int line_inc = surface_pitch_pixels - w;

	// SSE2 temporaries:
	const __m128i vec_color			= _mm_unpacklo_epi8(_mm_set1_epi32(color), _mm_setzero_si128());
	const __m128i vec_alphacolor	= _mm_mullo_epi16(vec_color, _mm_set1_epi16(alpha));
	const __m128i vec_invalpha		= _mm_set1_epi16(256 - alpha);

	argb_t* dest = (argb_t*)surface->getBuffer() + y1 * surface_pitch_pixels + x1;

	for (int rowcount = h; rowcount > 0; --rowcount)
	{
		// [SL] Calculate how many pixels of each row need to be drawn before dest is
		// aligned to a 16-byte boundary.
		int align = R_GetBytesUntilAligned(dest, 16) / sizeof(argb_t);
		if (align > w)
			align = w;

		int batches = (w - align) / 8;
		int remainder = (w - align) & (8 - 1);

		// align the destination buffer to 16-byte boundary
		while (align--)
		{
			*dest = alphablend1a(*dest, color, alpha);
			dest++;
		}

		// SSE2 optimize the bulk in batches of 8 pixels:
		while (batches--)
		{
			// Load 4 pixels into input0 and 4 pixels into input1
			const __m128i vec_input0 = _mm_load_si128((__m128i*)(dest + 0));
			const __m128i vec_input1 = _mm_load_si128((__m128i*)(dest + 4));

			// Expand the width of each color channel from 8bits to 16 bits
			// by splitting input into two 128bit variables, each
			// containing 2 ARGB values. 16bit color channels are needed to
			// accomodate multiplication.
			__m128i vec_lower0 = _mm_unpacklo_epi8(vec_input0, _mm_setzero_si128());
			__m128i vec_upper0 = _mm_unpackhi_epi8(vec_input0, _mm_setzero_si128());
			__m128i vec_lower1 = _mm_unpacklo_epi8(vec_input1, _mm_setzero_si128());
			__m128i vec_upper1 = _mm_unpackhi_epi8(vec_input1, _mm_setzero_si128());

			// ((input * invAlpha) + (color * Alpha)) >> 8
			vec_lower0 = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(vec_lower0, vec_invalpha), vec_alphacolor), 8); 
			vec_upper0 = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(vec_upper0, vec_invalpha), vec_alphacolor), 8); 
			vec_lower1 = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(vec_lower1, vec_invalpha), vec_alphacolor), 8); 
			vec_upper1 = _mm_srli_epi16(_mm_add_epi16(_mm_mullo_epi16(vec_upper1, vec_invalpha), vec_alphacolor), 8); 

			// Compress the width of each color channel to 8bits again and store in dest
			_mm_store_si128((__m128i*)(dest + 0), _mm_packus_epi16(vec_lower0, vec_upper0));
			_mm_store_si128((__m128i*)(dest + 4), _mm_packus_epi16(vec_lower1, vec_upper1));

			dest += 8;
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


VERSION_CONTROL (r_drawt_sse2_cpp, "$Id$")

#endif
