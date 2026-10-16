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

#ifdef __MMX__

// NOTE(jsd): Do not consider MMX deprecated so lightly. The XBOX and other older systems still make use of it.

#include <mmintrin.h>

#ifdef _MSC_VER
#define MMX_ALIGNED(x) _CRT_ALIGN(8) x
#else
#define MMX_ALIGNED(x) x __attribute__((aligned(8)))
#endif

#include <algorithm>

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
	return (alignment - (reinterpret_cast<uintptr_t>(data) & mask)) & mask;
}


void r_dimpatchD_MMX(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
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

	// MMX temporaries:
	const __m64 vec_color		= _mm_unpacklo_pi8(_mm_set1_pi32(color), _mm_setzero_si64());
	const __m64 vec_alphacolor	= _mm_mullo_pi16(vec_color, _mm_set1_pi16(alpha));
	const __m64 vec_invalpha	= _mm_set1_pi16(256 - alpha);

	argb_t* dest = reinterpret_cast<argb_t*>(surface->getBuffer()) + y1 * rowstep + x1 * colstep;

	for (int rowcount = count; rowcount > 0; --rowcount)
	{
		// [SL] Calculate how many pixels of each run need to be drawn before dest is
		// aligned to a 64-bit boundary.
		int align = R_GetBytesUntilAligned(dest, 64/8) / sizeof(argb_t);
		if (align > run)
			align = run;

		const int batch_size = 4;
		int batches = (run - align) / batch_size;
		int remainder = (run - align) & (batch_size - 1);

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
			const __m64 vec_input0 = *(reinterpret_cast<__m64*>(dest + 0));
			const __m64 vec_input1 = *(reinterpret_cast<__m64*>(dest + 2));

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
			*(reinterpret_cast<__m64*>(dest + 0)) = _mm_packs_pu16(vec_lower0, vec_upper0);
			*(reinterpret_cast<__m64*>(dest + 2)) = _mm_packs_pu16(vec_lower1, vec_upper1);

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

// ============================================================================
//
// Four-pixel column drawing
//
// ============================================================================

// The helpers leave the MMX state set for their entry point to clear, which MSVC
// warns about in each one (C4799) even where it inlines them.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4799)
#endif

//
// QuadColormapFuncMMX
//
// Opaque: shade two texels and store them. A masked column reads the
// framebuffer back so its transparent lanes keep what was there.
//
class QuadColormapFuncMMX
{
public:
	// By value, as QuadColormapFunc takes it.
	QuadColormapFuncMMX(const drawcolumn_t& d) : shademap(d.colormap.m_shademap) { }

	template<bool MASKED>
	forceinline void half(uint32_t i0, uint32_t i1, argb_t* dest, const __m64 keep) const
	{
		__m64* const d = reinterpret_cast<__m64*>(dest);
		const __m64 fg = _mm_setr_pi32(shademap[i0], shademap[i1]);

		if (!MASKED)
		{
			*d = fg;
			return;
		}

		*d = _mm_or_si64(_mm_and_si64(keep, *d), _mm_andnot_si64(keep, fg));
	}

	forceinline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = shademap[c];
	}

private:
	const argb_t* shademap;
};


//
// QuadTranslucentFuncMMX
//
// Blends two pixels against the framebuffer, a channel per 16-bit lane. The
// bounds argued in QuadTranslucentFunc hold lane for lane: no product or sum
// overflows a lane, and packuswb never saturates.
//
class QuadTranslucentFuncMMX
{
public:
	QuadTranslucentFuncMMX(const drawcolumn_t& d) : shademap(d.colormap.m_shademap)
	{
		fga = std::clamp(static_cast<int>((d.translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;

		mfga = _mm_set1_pi16(static_cast<short>(fga));
		mbga = _mm_set1_pi16(static_cast<short>(bga));
		mamask = _mm_set1_pi32(static_cast<int>(argb_t::alphaMask()));
	}

	template<bool MASKED>
	forceinline void half(uint32_t i0, uint32_t i1, argb_t* dest, const __m64 keep) const
	{
		const __m64 zero = _mm_setzero_si64();

		__m64* const d = reinterpret_cast<__m64*>(dest);
		const __m64 fg = _mm_setr_pi32(shademap[i0], shademap[i1]);
		const __m64 bg = *d;

		const __m64 lo = _mm_srli_pi16(_mm_add_pi16(
				_mm_mullo_pi16(_mm_unpacklo_pi8(bg, zero), mbga),
				_mm_mullo_pi16(_mm_unpacklo_pi8(fg, zero), mfga)), 8);
		const __m64 hi = _mm_srli_pi16(_mm_add_pi16(
				_mm_mullo_pi16(_mm_unpackhi_pi8(bg, zero), mbga),
				_mm_mullo_pi16(_mm_unpackhi_pi8(fg, zero), mfga)), 8);

		// Alpha before the select, as in QuadTranslucentFunc.
		const __m64 res = _mm_or_si64(_mm_packs_pu16(lo, hi), mamask);

		if (!MASKED)
		{
			*d = res;
			return;
		}

		*d = _mm_or_si64(_mm_and_si64(keep, bg), _mm_andnot_si64(keep, res));
	}

	forceinline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = alphablend2a(*dest, bga, shademap[c], fga);
	}

private:
	const argb_t*	shademap;
	__m64			mfga, mbga, mamask;
	int				fga, bga;
};


//
// R_DrawColumnQuadGenericMMX
//
// R_DrawColumnQuadGeneric with each quad as two halves. `count >= 4` is the same
// correctness bound, keeping every store inside [yl, yh].
//
template<typename QUADFUNC, bool MASKED, bool EARLYOUT>
static forceinline void R_DrawColumnQuadGenericMMX(argb_t* dest, const drawcolumn_t& d)
{
	int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const palindex_t* const source = d.source;
	const fixed_t fracstep = d.iscale;
	fixed_t frac = d.texturefrac;

	const int mask = (d.textureheight >> FRACBITS) - 1;

	const QUADFUNC f(d);

	while (count >= 4)
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

		if (!MASKED)
		{
			f.template half<false>(i0, i1, dest, _mm_setzero_si64());
			f.template half<false>(i2, i3, dest + 2, _mm_setzero_si64());
		}
		else if (!EARLYOUT || (i0 | i1 | i2 | i3))
		{
			// All ones in the lanes the scalar drawer would not have written.
			const __m64 zero = _mm_setzero_si64();
			f.template half<true>(i0, i1, dest,
					_mm_cmpeq_pi32(_mm_setr_pi32(int(i0), int(i1)), zero));
			f.template half<true>(i2, i3, dest + 2,
					_mm_cmpeq_pi32(_mm_setr_pi32(int(i2), int(i3)), zero));
		}

		dest += 4;
		frac += fracstep * 4;
		count -= 4;
	}

	// Scalar tail, for the reason R_DrawColumnQuadGeneric gives.
	while (count--)
	{
		const palindex_t pixel = MASKED ? source[frac >> FRACBITS]
									: source[(frac >> FRACBITS) & mask];
		if (!MASKED || pixel != 0)
			f(pixel, dest);
		dest++;
		frac += fracstep;
	}
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif

//
// The entry points, addressed as FB_COLDEST_D does in r_draw.cpp.
//
static forceinline argb_t* R_QuadColumnDestMMX(const drawcolumn_t& dcol)
{
	return reinterpret_cast<argb_t*>(dcol.destination) +
			ptrdiff_t(dcol.x) * dcol.colstep + dcol.yl;
}

void R_DrawColumnD_MMX(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	if (dcol.masked)
		R_DrawColumnQuadGenericMMX<QuadColormapFuncMMX, true, true>(R_QuadColumnDestMMX(dcol), dcol);
	else
		R_DrawColumnQuadGenericMMX<QuadColormapFuncMMX, false, false>(R_QuadColumnDestMMX(dcol), dcol);

	_mm_empty();
}

void R_DrawTranslucentColumnD_MMX(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	if (dcol.masked)
		R_DrawColumnQuadGenericMMX<QuadTranslucentFuncMMX, true, true>(R_QuadColumnDestMMX(dcol), dcol);
	else
		R_DrawColumnQuadGenericMMX<QuadTranslucentFuncMMX, false, false>(R_QuadColumnDestMMX(dcol), dcol);

	_mm_empty();
}


//
// R_DrawLevelGroupD_MMX
//
// R_DrawLevelGroupD_SSE2 with the four lanes in two registers: rows y0, y0 + 1
// in one and y0 + 2, y0 + 3 in the other. The DDA, the shifts and the masks are
// vector work; the four lookups are not, as in every form of this drawer.
//
void R_DrawLevelGroupD_MMX(rendercontext_t& ctx)
{
	const drawplanegroup_t& g = ctx.draw.dpgroup;

	const palindex_t* const source = g.source;
	const argb_t* const s0 = g.shade[0];
	const argb_t* const s1 = g.shade[1];
	const argb_t* const s2 = g.shade[2];
	const argb_t* const s3 = g.shade[3];

	__m64 mu01 = _mm_setr_pi32(int(g.ufrac[0]), int(g.ufrac[1]));
	__m64 mu23 = _mm_setr_pi32(int(g.ufrac[2]), int(g.ufrac[3]));
	__m64 mv01 = _mm_setr_pi32(int(g.vfrac[0]), int(g.vfrac[1]));
	__m64 mv23 = _mm_setr_pi32(int(g.vfrac[2]), int(g.vfrac[3]));
	const __m64 mus01 = _mm_setr_pi32(int(g.ustep[0]), int(g.ustep[1]));
	const __m64 mus23 = _mm_setr_pi32(int(g.ustep[2]), int(g.ustep[3]));
	const __m64 mvs01 = _mm_setr_pi32(int(g.vstep[0]), int(g.vstep[1]));
	const __m64 mvs23 = _mm_setr_pi32(int(g.vstep[2]), int(g.vstep[3]));

	const __m64 mumask = _mm_set1_pi32(g.umask);
	const __m64 mvmask = _mm_set1_pi32(g.vmask);

	// psrld takes its count in a register, so both hoist out of the loop.
	const __m64 cu = _mm_cvtsi32_si64(g.ushift);
	const __m64 cv = _mm_cvtsi32_si64(g.vshift);

	argb_t* dest = reinterpret_cast<argb_t*>(g.destination) +
			ptrdiff_t(g.xa) * g.colstep + g.y0;
	const int colstep = g.colstep;

	for (int n = g.xb - g.xa + 1; n; --n)
	{
		const __m64 spots01 = _mm_or_si64(
				_mm_and_si64(_mm_srl_pi32(mu01, cu), mumask),
				_mm_and_si64(_mm_srl_pi32(mv01, cv), mvmask));
		const __m64 spots23 = _mm_or_si64(
				_mm_and_si64(_mm_srl_pi32(mu23, cu), mumask),
				_mm_and_si64(_mm_srl_pi32(mv23, cv), mvmask));

		const uint32_t spot0 = uint32_t(_mm_cvtsi64_si32(spots01));
		const uint32_t spot1 = uint32_t(_mm_cvtsi64_si32(_mm_srli_si64(spots01, 32)));
		const uint32_t spot2 = uint32_t(_mm_cvtsi64_si32(spots23));
		const uint32_t spot3 = uint32_t(_mm_cvtsi64_si32(_mm_srli_si64(spots23, 32)));

		__m64* const d = reinterpret_cast<__m64*>(dest);
		d[0] = _mm_setr_pi32(s0[source[spot0]], s1[source[spot1]]);
		d[1] = _mm_setr_pi32(s2[source[spot2]], s3[source[spot3]]);

		mu01 = _mm_add_pi32(mu01, mus01);
		mu23 = _mm_add_pi32(mu23, mus23);
		mv01 = _mm_add_pi32(mv01, mvs01);
		mv23 = _mm_add_pi32(mv23, mvs23);

		dest += colstep;
	}

	_mm_empty();
}


VERSION_CONTROL (r_drawt_mmx_cpp, "$Id$")

#endif
