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

#include "i_sdl.h"
#include "r_intrin.h"

#ifdef __SSE2__

#include <assert.h>
#include <algorithm>
#include <emmintrin.h>

#ifdef _MSC_VER
#define SSE2_ALIGNED(x) _CRT_ALIGN(16) x
#else
#define SSE2_ALIGNED(x) x __attribute__((aligned(16)))
#endif

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
	return (alignment - (reinterpret_cast<uintptr_t>(data) & mask)) & mask;
}

// ============================================================================
//
// Four-pixel column drawing
//
// Four vertically adjacent 32bpp pixels are 16 contiguous bytes, so one loadu and
// one storeu cover them.
//
// The scalar drawers these replace are already tight -- 8.6 instructions per pixel
// for an opaque wall, 12 for a sprite -- so the win is narrower than it was for
// flats. The translucent drawer has the room: 32 per pixel, four of them imuls
// this does four at a time.
//
// R_DrawColumnGeneric is deliberately NOT touched: it carries ten instantiations
// shared by every opaque drawer, and a template parameter there would move all of
// their codegen for the sake of two.
//
// ============================================================================

//
// QuadColormapFunc
//
// Opaque: shade four texels and store them. Nothing is read back from the
// framebuffer unless the column is masked, where the transparent lanes have to
// keep what was already there.
//
class QuadColormapFunc
{
public:
	// By value. shaderef_t::m_shademap is mutable, so reading it through the
	// reference costs a reload every pixel -- visible in the listing as a
	// `mov rax, QWORD PTR [rip+...]` inside the loop -- and would be a data race
	// the moment two workers share a dcol.
	QuadColormapFunc(const drawcolumn_t& d) : shademap(d.colormap.m_shademap) { }

	template<bool MASKED>
	forceinline void quad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3,
			argb_t* dest, const __m128i keep) const
	{
		const __m128i fg = _mm_setr_epi32(shademap[i0], shademap[i1],
										shademap[i2], shademap[i3]);

		if (!MASKED)
		{
			_mm_storeu_si128(reinterpret_cast<__m128i*>(dest), fg);
			return;
		}

		const __m128i bg = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dest));

		_mm_storeu_si128(reinterpret_cast<__m128i*>(dest),
				_mm_or_si128(_mm_and_si128(keep, bg), _mm_andnot_si128(keep, fg)));
	}

	forceinline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = shademap[c];
	}

private:
	const argb_t* shademap;
};


//
// QuadTranslucentFunc
//
// Blends four pixels against the framebuffer. This is the one that pays: the
// scalar form is four imuls, and here four lanes share them.
//
class QuadTranslucentFunc
{
public:
	QuadTranslucentFunc(const drawcolumn_t& d) : shademap(d.colormap.m_shademap)
	{
		fga = std::clamp(static_cast<int>((d.translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;

		mfga = _mm_set1_epi16(static_cast<short>(fga));
		mbga = _mm_set1_epi16(static_cast<short>(bga));
		mamask = _mm_set1_epi32(argb_t::alphaMask());
	}

	template<bool MASKED>
	forceinline void quad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3,
			argb_t* dest, const __m128i keep) const
	{
		const __m128i zero = _mm_setzero_si128();

		const __m128i fg = _mm_setr_epi32(shademap[i0], shademap[i1],
										shademap[i2], shademap[i3]);
		const __m128i bg = _mm_loadu_si128(reinterpret_cast<const __m128i*>(dest));

		// bga + fga == 255 and bytes are at most 255, so a 16-bit lane holds at
		// most 65025: mullo is exact and paddw cannot carry into its neighbour.
		// After the shift the maximum is 254, so packus never saturates -- load
		// bearing, because packus CLAMPS above 255 where argb_t's uint8_t
		// parameters TRUNCATE. The constructor's clamp is what keeps the two
		// paths agreeing.
		const __m128i lo = _mm_srli_epi16(_mm_add_epi16(
				_mm_mullo_epi16(_mm_unpacklo_epi8(bg, zero), mbga),
				_mm_mullo_epi16(_mm_unpacklo_epi8(fg, zero), mfga)), 8);
		const __m128i hi = _mm_srli_epi16(_mm_add_epi16(
				_mm_mullo_epi16(_mm_unpackhi_epi8(bg, zero), mbga),
				_mm_mullo_epi16(_mm_unpackhi_epi8(fg, zero), mfga)), 8);

		// Alpha is forced BEFORE the select, not after. The scalar drawer never
		// writes a transparent pixel at all, so those lanes must keep whatever
		// alpha the framebuffer held; only blended lanes get 255.
		const __m128i res = _mm_or_si128(_mm_packus_epi16(lo, hi), mamask);

		if (!MASKED)
		{
			_mm_storeu_si128(reinterpret_cast<__m128i*>(dest), res);
			return;
		}

		_mm_storeu_si128(reinterpret_cast<__m128i*>(dest),
				_mm_or_si128(_mm_and_si128(keep, bg), _mm_andnot_si128(keep, res)));
	}

	forceinline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = alphablend2a(*dest, bga, shademap[c], fga);
	}

private:
	const argb_t*	shademap;
	__m128i			mfga, mbga, mamask;
	int				fga, bga;
};


//
// R_DrawColumnQuadGeneric
//
// MASKED mirrors R_DrawColumnGeneric's masked arm: the texel index is used raw
// and a zero texel leaves the destination alone. Otherwise it mirrors the
// power-of-two arm, masking the index and writing every pixel.
//
// EARLYOUT skips a quad whose four texels are all transparent, which also skips
// the framebuffer read-modify-write behind it. The top and bottom of a masked
// column are transparent-dominated by construction, so this usually pays; it
// costs a few percent on a column with no holes at all.
//
template<typename QUADFUNC, bool MASKED, bool EARLYOUT>
static forceinline void R_DrawColumnQuadGeneric(argb_t* dest, const drawcolumn_t& d)
{
	int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const palindex_t* const source = d.source;
	const fixed_t fracstep = d.iscale;
	fixed_t frac = d.texturefrac;

	const int mask = (d.textureheight >> FRACBITS) - 1;

	const QUADFUNC f(d);

	// `count >= 4` is a CORRECTNESS bound, not a tuning threshold: it keeps all
	// sixteen bytes of the store inside [yl, yh] of this column. A column's pixels
	// are one contiguous run, so four pixels starting inside it cannot leave it.
	// Under a column-sliced renderer a store past yh is another worker's memory.
	while (count >= 4)
	{
		// The texel OFFSETS, then the palette indices they name. The drawers shade
		// and test the palette value, never the offset -- conflating the two
		// samples the wrong colour and tests the wrong thing against zero.
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
			f.template quad<false>(i0, i1, i2, i3, dest, _mm_setzero_si128());
		}
		else if (!EARLYOUT || (i0 | i1 | i2 | i3))
		{
			// All ones in the lanes the scalar drawer would not have written.
			const __m128i keep = _mm_cmpeq_epi32(
					_mm_setr_epi32(int(i0), int(i1), int(i2), int(i3)),
					_mm_setzero_si128());

			f.template quad<true>(i0, i1, i2, i3, dest, keep);
		}

		dest += 4;
		frac += fracstep * 4;
		count -= 4;
	}

	// The tail stays scalar. Positioning one final overlapping vector at end-4
	// is the usual trick and is illegal here: it would re-blend up to three
	// pixels, and a blend is not idempotent.
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


//
// The entry points. dest is computed as FB_COLDEST_D does in r_draw.cpp: the
// column step times x, plus the first row.
//
static forceinline argb_t* R_QuadColumnDest()
{
	return reinterpret_cast<argb_t*>(dcol.destination) +
			ptrdiff_t(dcol.x) * dcol.colstep + dcol.yl;
}

void R_DrawColumnD_SSE2()
{
	if (dcol.masked)
		R_DrawColumnQuadGeneric<QuadColormapFunc, true, true>(R_QuadColumnDest(), dcol);
	else
		R_DrawColumnQuadGeneric<QuadColormapFunc, false, false>(R_QuadColumnDest(), dcol);
}

void R_DrawTranslucentColumnD_SSE2()
{
	if (dcol.masked)
		R_DrawColumnQuadGeneric<QuadTranslucentFunc, true, true>(R_QuadColumnDest(), dcol);
	else
		R_DrawColumnQuadGeneric<QuadTranslucentFunc, false, false>(R_QuadColumnDest(), dcol);
}


//
// R_DrawLevelGroupD_SSE2
//
// Four screen rows in the four lanes, marching in x. See drawplanegroup_t for
// why this is the form that vectorizes: the per-lane yslope makes the mapping an
// integer DDA again (one paddd for u, one for v) and folds per-y lighting into
// four loop-invariant pointers.
//
// Storeu, not store: one row down is one pixel, so the base is only 4-byte
// aligned at 32bpp for an arbitrary y. The pitch is a multiple of 64, so the phase
// is constant down a run -- all-or-nothing, never fixable by a prologue.
//
void R_DrawLevelGroupD_SSE2()
{
	const drawplanegroup_t& g = dpgroup;

	const palindex_t* const source = g.source;
	const argb_t* const s0 = g.shade[0];
	const argb_t* const s1 = g.shade[1];
	const argb_t* const s2 = g.shade[2];
	const argb_t* const s3 = g.shade[3];

	__m128i mu = _mm_loadu_si128(reinterpret_cast<const __m128i*>(g.ufrac));
	__m128i mv = _mm_loadu_si128(reinterpret_cast<const __m128i*>(g.vfrac));
	const __m128i mus = _mm_loadu_si128(reinterpret_cast<const __m128i*>(g.ustep));
	const __m128i mvs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(g.vstep));

	const __m128i mumask = _mm_set1_epi32(g.umask);
	const __m128i mvmask = _mm_set1_epi32(g.vmask);

	// psrld takes its count in an xmm register, so both hoist out of the loop. The
	// scalar drawer pays two `mov rcx, <shift>` per pixel, since x86 variable
	// shifts route through CL.
	const __m128i cu = _mm_cvtsi32_si128(g.ushift);
	const __m128i cv = _mm_cvtsi32_si128(g.vshift);

	argb_t* dest = reinterpret_cast<argb_t*>(g.destination) +
			ptrdiff_t(g.xa) * g.colstep + g.y0;
	const int colstep = g.colstep;

	for (int n = g.xb - g.xa + 1; n; --n)
	{
		const __m128i mspots = _mm_or_si128(
				_mm_and_si128(_mm_srl_epi32(mu, cu), mumask),
				_mm_and_si128(_mm_srl_epi32(mv, cv), mvmask));

		// Same idiom as the span drawer above, which MSVC keeps in registers.
		const auto spots = std::bit_cast<std::array<uint32_t, 4>>(mspots);

		_mm_storeu_si128(reinterpret_cast<__m128i*>(dest), _mm_setr_epi32(
				s0[source[spots[0]]], s1[source[spots[1]]],
				s2[source[spots[2]]], s3[source[spots[3]]]));

		mu = _mm_add_epi32(mu, mus);
		mv = _mm_add_epi32(mv, mvs);

		dest += colstep;
	}
}


void r_dimpatchD_SSE2(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	// A screen column is the contiguous run, so the vectorized walk goes DOWN a
	// column and the outer loop steps across them.
	const int rowstep = surface->getRowStepInPixels();
	const int colstep = surface->getColStepInPixels();

	const int run = h;
	const int count = w;
	const int stride = colstep;

	const int line_inc = stride - run;

	// SSE2 temporaries:
	const __m128i vec_color			= _mm_unpacklo_epi8(_mm_set1_epi32(color), _mm_setzero_si128());
	const __m128i vec_alphacolor	= _mm_mullo_epi16(vec_color, _mm_set1_epi16(alpha));
	const __m128i vec_invalpha		= _mm_set1_epi16(256 - alpha);

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

		// SSE2 optimize the bulk in batches of 8 pixels:
		while (batches--)
		{
			// Load 4 pixels into input0 and 4 pixels into input1
			const __m128i vec_input0 = _mm_load_si128(reinterpret_cast<__m128i*>(dest + 0));
			const __m128i vec_input1 = _mm_load_si128(reinterpret_cast<__m128i*>(dest + 4));

			// Expand the width of each color channel from 8-bits to 16-bits
			// by splitting each input vector into two 128-bit variables, each
			// containing 2 ARGB values. 16-bit color channels are needed to
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

			// Compress the width of each color channel to 8-bits again and store in dest
			_mm_store_si128(reinterpret_cast<__m128i*>(dest + 0), _mm_packus_epi16(vec_lower0, vec_upper0));
			_mm_store_si128(reinterpret_cast<__m128i*>(dest + 4), _mm_packus_epi16(vec_lower1, vec_upper1));

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


VERSION_CONTROL (r_drawt_sse2_cpp, "$Id$")

#endif
