// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2026 by The Odamex Team.
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


#include "odamex.h"

#include "r_context.h"

#include "i_sdl.h"
#include "r_intrin.h"

#ifdef __ALTIVEC__

#include <stdlib.h>
#include <string.h>
#include <algorithm>

#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "i_video.h"

#include <altivec.h>

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
	// A screen column is the contiguous run, so the vectorized walk goes DOWN a
	// column and the outer loop steps across them. The vector body below is
	// unchanged: it only needs the run to be contiguous, not to be a screen row.
	//
	// A column starts at x * pitch + y1, and the pitch is a multiple of a cache
	// line, so the alignment phase is a function of y1 alone -- the same for every
	// column. The prologue below therefore does equal work per run rather than
	// thrashing between aligned and unaligned columns.
	const int rowstep = surface->getRowStepInPixels();
	const int colstep = surface->getColStepInPixels();

	const int run = h;
	const int count = w;
	const int stride = colstep;

	const int line_inc = stride - run;

	// ALTIVEC temporaries:
	const vu8 vec_mask = { 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF, 0, 0xFF };
	const vu16 vec_eight = vec_splat_u16(8);

	const vu32 vec_color_temp = { color, color, color, color };
	const vu16 vec_color = (vu16)vec_and((vu8)vec_unpackl((vs8)vec_color_temp), vec_mask);
	const vu16 vec_alpha = { alpha, alpha, alpha, alpha, alpha, alpha, alpha, alpha };
	const vu16 vec_alphacolor = (vu16)vec_mladd(vec_alpha, vec_color, vec_splat_u16(0));

	const uint16_t invalpha = 256 - alpha;
	const vu16 vec_invalpha = { invalpha, invalpha, invalpha, invalpha, invalpha, invalpha, invalpha, invalpha };

	argb_t* dest = reinterpret_cast<argb_t*>(surface->getBuffer()) + y1 * rowstep + x1 * colstep;

	for (int rowcount = count; rowcount > 0; --rowcount)
	{
		// [SL] Calculate how many pixels of each run need to be drawn before dest is
		// aligned to a 128-bit boundary.
		int align = R_GetBytesUntilAligned(dest, 128/8) / sizeof(argb_t);
		if (align > run)
			align = run;

		const int batch_size = 8;
		int batches = (run - align) / batch_size;
		int remainder = (run - align) & (batch_size - 1);

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

// ============================================================================
//
// Four-pixel column drawing
//
// ============================================================================

static inline vu8 R_SplatU8(uint8_t x)
{
	return (vu8){ x, x, x, x, x, x, x, x, x, x, x, x, x, x, x, x };
}

static inline vu16 R_SplatU16(uint16_t x)
{
	return (vu16){ x, x, x, x, x, x, x, x };
}

static inline vu32 R_SplatU32(uint32_t x)
{
	return (vu32){ x, x, x, x };
}

//
// QuadTranslucentFuncAltivec
//
// The blend of QuadTranslucentFunc: every byte becomes (bg * bga + fg * fga)
// >> 8, the same value its 16-bit lanes compute, and alpha is forced to 255
// before the select.
//
class QuadTranslucentFuncAltivec
{
public:
	QuadTranslucentFuncAltivec(const drawcolumn_t& d) : shademap(d.colormap.m_shademap)
	{
		fga = std::clamp(static_cast<int>((d.translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;

		mfga = R_SplatU8(static_cast<uint8_t>(fga));
		mbga = R_SplatU8(static_cast<uint8_t>(bga));
		mamask = R_SplatU32(argb_t::alphaMask());
	}

	// dest must be 16-byte aligned.
	template<bool MASKED>
	inline void quad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3, argb_t* dest) const
	{
		ALTIVEC_ALIGNED(uint32_t fgs[4]) = { shademap[i0], shademap[i1], shademap[i2], shademap[i3] };

		uint32_t* const d = reinterpret_cast<uint32_t*>(dest);
		const vu32 fg = vec_ld(0, fgs);
		const vu32 bg = vec_ld(0, d);

		// Products of bytes and weights at most 255, summing to at most 65025:
		// no lane overflows, and after the shift every result fits its byte.
		const vu16 even = vec_add(vec_mule((vu8)bg, mbga), vec_mule((vu8)fg, mfga));
		const vu16 odd = vec_add(vec_mulo((vu8)bg, mbga), vec_mulo((vu8)fg, mfga));

		const vu16 packed = vec_or(vec_and(even, R_SplatU16(0xFF00)), vec_sr(odd, vec_splat_u16(8)));
		const vu32 res = vec_or((vu32)packed, mamask);

		if (not MASKED)
		{
			vec_st(res, 0, d);
			return;
		}

		// The lanes the scalar drawer would not have written keep the background.
		const vu32 indices = { i0, i1, i2, i3 };
		vec_st(vec_sel(res, bg, vec_cmpeq(indices, vec_splat_u32(0))), 0, d);
	}

	inline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = alphablend2a(*dest, bga, shademap[c], fga);
	}

private:
	const argb_t*	shademap;
	vu8				mfga, mbga;
	vu32			mamask;
	int				fga, bga;
};


//
// R_DrawColumnQuadGenericAltivec
//
template<typename QUADFUNC, bool MASKED, bool EARLYOUT>
static inline void R_DrawColumnQuadGenericAltivec(argb_t* dest, const drawcolumn_t& d)
{
	int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const palindex_t* const source = d.source;
	const fixed_t fracstep = d.iscale;
	fixed_t frac = d.texturefrac;

	const int mask = (d.textureheight >> FRACBITS) - 1;

	const QUADFUNC f(d);

	int align = static_cast<int>(R_GetBytesUntilAligned(dest, 16) / sizeof(argb_t));
	if (align > count)
		align = count;
	count -= align;

	int tail = count & 3;
	count -= tail;

	// Scalar pixels before the first quad and after the last.
	const auto single = [&]() {
		const palindex_t pixel = MASKED ? source[frac >> FRACBITS]
									: source[(frac >> FRACBITS) & mask];
		if (not MASKED or pixel != 0)
			f(pixel, dest);
		dest++;
		frac += fracstep;
	};

	while (align--)
		single();

	for (; count; count -= 4)
	{
		// Texel offsets, then the palette indices they name.
		const int o0 = MASKED ? int( frac                   >> FRACBITS)
							: int((frac                   >> FRACBITS) & mask);
		const int o1 = MASKED ? int((frac + fracstep)       >> FRACBITS)
							: int(((frac + fracstep)       >> FRACBITS) & mask);
		const int o2 = MASKED ? int((frac + fracstep * 2)   >> FRACBITS)
							: int(((frac + fracstep * 2)   >> FRACBITS) & mask);
		const int o3 = MASKED ? int((frac + fracstep * 3)   >> FRACBITS)
							: int(((frac + fracstep * 3)   >> FRACBITS) & mask);

		const uint32_t i0 = source[o0];
		const uint32_t i1 = source[o1];
		const uint32_t i2 = source[o2];
		const uint32_t i3 = source[o3];

		if (not MASKED)
			f.template quad<false>(i0, i1, i2, i3, dest);
		else if (not EARLYOUT or (i0 | i1 | i2 | i3))
			f.template quad<true>(i0, i1, i2, i3, dest);

		dest += 4;
		frac += fracstep * 4;
	}

	while (tail--)
		single();
}


//
// R_DrawTranslucentColumnD_ALTIVEC
//
void R_DrawTranslucentColumnD_ALTIVEC(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	argb_t* const dest = reinterpret_cast<argb_t*>(dcol.destination) +
			ptrdiff_t(dcol.x) * dcol.colstep + dcol.yl;

	if (dcol.masked)
		R_DrawColumnQuadGenericAltivec<QuadTranslucentFuncAltivec, true, true>(dest, dcol);
	else
		R_DrawColumnQuadGenericAltivec<QuadTranslucentFuncAltivec, false, false>(dest, dcol);
}


//
// R_DrawLevelGroupD_ALTIVEC
//
void R_DrawLevelGroupD_ALTIVEC(rendercontext_t& ctx)
{
	const drawplanegroup_t& g = ctx.draw.dpgroup;

	const palindex_t* const source = g.source;
	const argb_t* const s0 = g.shade[0];
	const argb_t* const s1 = g.shade[1];
	const argb_t* const s2 = g.shade[2];
	const argb_t* const s3 = g.shade[3];

	// The struct's arrays carry no alignment, so through an aligned copy.
	ALTIVEC_ALIGNED(uint32_t lanes[4][4]);
	memcpy(lanes[0], g.ufrac, sizeof(lanes[0]));
	memcpy(lanes[1], g.vfrac, sizeof(lanes[1]));
	memcpy(lanes[2], g.ustep, sizeof(lanes[2]));
	memcpy(lanes[3], g.vstep, sizeof(lanes[3]));

	vu32 mu = vec_ld(0, lanes[0]);
	vu32 mv = vec_ld(0, lanes[1]);
	const vu32 mus = vec_ld(0, lanes[2]);
	const vu32 mvs = vec_ld(0, lanes[3]);

	const vu32 mumask = R_SplatU32(static_cast<uint32_t>(g.umask));
	const vu32 mvmask = R_SplatU32(static_cast<uint32_t>(g.vmask));
	const vu32 cu = R_SplatU32(static_cast<uint32_t>(g.ushift));
	const vu32 cv = R_SplatU32(static_cast<uint32_t>(g.vshift));

	ALTIVEC_ALIGNED(uint32_t spots[4]);

	argb_t* dest = reinterpret_cast<argb_t*>(g.destination) +
			ptrdiff_t(g.xa) * g.colstep + g.y0;
	const int colstep = g.colstep;

	for (int n = g.xb - g.xa + 1; n; --n)
	{
		vec_st(vec_or(vec_and(vec_sr(mu, cu), mumask), vec_and(vec_sr(mv, cv), mvmask)), 0, spots);

		dest[0] = s0[source[spots[0]]];
		dest[1] = s1[source[spots[1]]];
		dest[2] = s2[source[spots[2]]];
		dest[3] = s3[source[spots[3]]];

		mu = vec_add(mu, mus);
		mv = vec_add(mv, mvs);

		dest += colstep;
	}
}

// ============================================================================
//
// The texture upload
//
// ============================================================================

//
// R_StreamStoredLines32_ALTIVEC
//
// Copies 32bpp stored lines into a 16-byte aligned destination, four pixels at
// a time. stvxl is AltiVec's closest thing to a streaming store: it marks the
// line least recently used, so it is the first to be evicted.
//
// There is no 8bpp version. AltiVec can't move a general register into a lane,
// so four palette lookups would have to go through memory as words and come
// back as a vector, defeating the purpose as it creates a stall.
//
void R_StreamStoredLines32_ALTIVEC(argb_t* dest, const argb_t* source, int destpitchpixels,
                                   int srcpitchpixels, int line_length, int line_count)
{
	for (int line = 0; line < line_count; line++)
	{
		const unsigned int* const in = reinterpret_cast<const unsigned int*>(source);
		const vu8 merge = vec_lvsl(0, in);
		int i = 0;

		for (; i + 4 <= line_length; i += 4)
		{
			const vu32 v = vec_perm(vec_ld(0, in + i), vec_ld(15, in + i), merge);
			vec_stl(v, 0, reinterpret_cast<unsigned int*>(dest + i));
		}

		for (; i < line_length; i++)
			dest[i] = source[i];

		dest += destpitchpixels;
		source += srcpitchpixels;
	}
}


VERSION_CONTROL (r_drawt_altivec_cpp, "$Id$")

#endif
