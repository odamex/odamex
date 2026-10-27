// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	NEON drawers for ARM: the four-pixel columns, the level-plane row groups
//	and the dim patch of r_drawt_sse2.cpp, for AArch64 and for 32-bit ARM
//	built with NEON.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "r_context.h"

#include "i_sdl.h"
#include "r_intrin.h"

#ifdef __ARM_NEON

#include <algorithm>
#include <arm_neon.h>

#include "r_draw.h"
#include "r_main.h"
#include "i_video.h"

// ============================================================================
//
// Four-pixel column drawing
//
// The SSE2 quads of r_drawt_sse2.cpp: the same four pixels per iteration, the
// same masking and the same blend, so every comment there about why holds here.
// NEON loads and stores unaligned, so the quads start wherever the column does.
//
// ============================================================================

//
// R_NeonGather
//
// Four shaded texels in one vector. NEON has no gather, so each texel loads
// straight into its lane.
//
forceinline uint32x4_t R_NeonGather(const argb_t* shademap,
    uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
{
	const uint32_t* const s = reinterpret_cast<const uint32_t*>(shademap);

	uint32x4_t v = vld1q_dup_u32(s + i0);
	v = vld1q_lane_u32(s + i1, v, 1);
	v = vld1q_lane_u32(s + i2, v, 2);
	return vld1q_lane_u32(s + i3, v, 3);
}

//
// R_NeonLanes
//
// Four values already in registers as one vector, for the same reason.
//
forceinline uint32x4_t R_NeonLanes(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3)
{
	uint32x4_t v = vdupq_n_u32(i0);
	v = vsetq_lane_u32(i1, v, 1);
	v = vsetq_lane_u32(i2, v, 2);
	return vsetq_lane_u32(i3, v, 3);
}

//
// QuadColormapFuncNEON
//
// Opaque: shade four texels and store them. A masked column reads the
// framebuffer back so its transparent lanes keep what was there.
//
class QuadColormapFuncNEON
{
public:
	// By value, as QuadColormapFunc takes it.
	QuadColormapFuncNEON(const drawcolumn_t& d) : shademap(d.colormap.m_shademap) { }

	template<bool MASKED>
	forceinline void quad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3,
			argb_t* dest, const uint32x4_t keep) const
	{
		uint32_t* const d = reinterpret_cast<uint32_t*>(dest);
		const uint32x4_t fg = R_NeonGather(shademap, i0, i1, i2, i3);

		if (!MASKED)
		{
			vst1q_u32(d, fg);
			return;
		}

		vst1q_u32(d, vbslq_u32(keep, vld1q_u32(d), fg));
	}

	forceinline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = shademap[c];
	}

private:
	const argb_t* shademap;
};


//
// QuadTranslucentFuncNEON
//
// Blends four pixels against the framebuffer.
//
class QuadTranslucentFuncNEON
{
public:
	QuadTranslucentFuncNEON(const drawcolumn_t& d) : shademap(d.colormap.m_shademap)
	{
		fga = std::clamp(static_cast<int>((d.translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;

		mfga = vdup_n_u8(static_cast<uint8_t>(fga));
		mbga = vdup_n_u8(static_cast<uint8_t>(bga));
		mamask = vdupq_n_u32(argb_t::alphaMask());
	}

	template<bool MASKED>
	forceinline void quad(uint32_t i0, uint32_t i1, uint32_t i2, uint32_t i3,
			argb_t* dest, const uint32x4_t keep) const
	{
		uint32_t* const d = reinterpret_cast<uint32_t*>(dest);
		const uint32x4_t fg = R_NeonGather(shademap, i0, i1, i2, i3);
		const uint32x4_t bg = vld1q_u32(d);

		const uint8x16_t fg8 = vreinterpretq_u8_u32(fg);
		const uint8x16_t bg8 = vreinterpretq_u8_u32(bg);

		const uint16x8_t lo = vmlal_u8(vmull_u8(vget_low_u8(bg8), mbga), vget_low_u8(fg8), mfga);
		const uint16x8_t hi = vmlal_u8(vmull_u8(vget_high_u8(bg8), mbga), vget_high_u8(fg8), mfga);

		// Alpha before the select, as in QuadTranslucentFunc.
		const uint32x4_t res = vorrq_u32(
				vreinterpretq_u32_u8(vcombine_u8(vshrn_n_u16(lo, 8), vshrn_n_u16(hi, 8))), mamask);

		if (!MASKED)
		{
			vst1q_u32(d, res);
			return;
		}

		vst1q_u32(d, vbslq_u32(keep, bg, res));
	}

	forceinline void operator()(palindex_t c, argb_t* dest) const
	{
		*dest = alphablend2a(*dest, bga, shademap[c], fga);
	}

private:
	const argb_t* shademap;
	uint8x8_t     mfga;
	uint8x8_t     mbga;
	uint32x4_t    mamask;
	int           fga;
	int           bga;
};


//
// R_DrawColumnQuadGenericNEON
//
// R_DrawColumnQuadGeneric for NEON.
//
template<typename QUADFUNC, bool MASKED, bool EARLYOUT>
forceinline void R_DrawColumnQuadGenericNEON(argb_t* dest, const drawcolumn_t& d)
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
		          : int((frac                    >> FRACBITS) & mask);
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
			f.template quad<false>(i0, i1, i2, i3, dest, vdupq_n_u32(0));
		}
		else if (!EARLYOUT || (i0 | i1 | i2 | i3))
		{
			// All ones in the lanes the scalar drawer would not have written.
			f.template quad<true>(i0, i1, i2, i3, dest, vceqq_u32(R_NeonLanes(i0, i1, i2, i3), vdupq_n_u32(0)));
		}

		dest += 4;
		frac += fracstep * 4;
		count -= 4;
	}

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
// The entry points, addressed as FB_COLDEST_D.
//
forceinline argb_t* R_QuadColumnDestNEON(const drawcolumn_t& dcol)
{
	return reinterpret_cast<argb_t*>(dcol.destination) +
			ptrdiff_t(dcol.x) * dcol.colstep + dcol.yl;
}

void R_DrawColumnD_NEON(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	if (dcol.masked)
		R_DrawColumnQuadGenericNEON<QuadColormapFuncNEON, true, true>(R_QuadColumnDestNEON(dcol), dcol);
	else
		R_DrawColumnQuadGenericNEON<QuadColormapFuncNEON, false, false>(R_QuadColumnDestNEON(dcol), dcol);
}

void R_DrawTranslucentColumnD_NEON(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	if (dcol.masked)
		R_DrawColumnQuadGenericNEON<QuadTranslucentFuncNEON, true, true>(R_QuadColumnDestNEON(dcol), dcol);
	else
		R_DrawColumnQuadGenericNEON<QuadTranslucentFuncNEON, false, false>(R_QuadColumnDestNEON(dcol), dcol);
}


//
// R_DrawLevelGroupD_NEON
//
// R_DrawLevelGroupD_SSE2 ported to NEON
//
void R_DrawLevelGroupD_NEON(rendercontext_t& ctx)
{
	const drawplanegroup_t& g = ctx.draw.dpgroup;

	const palindex_t* const source = g.source;
	const uint32_t* const s0 = reinterpret_cast<const uint32_t*>(g.shade[0]);
	const uint32_t* const s1 = reinterpret_cast<const uint32_t*>(g.shade[1]);
	const uint32_t* const s2 = reinterpret_cast<const uint32_t*>(g.shade[2]);
	const uint32_t* const s3 = reinterpret_cast<const uint32_t*>(g.shade[3]);

	uint32x4_t mu = vld1q_u32(g.ufrac);
	uint32x4_t mv = vld1q_u32(g.vfrac);
	const uint32x4_t mus = vld1q_u32(g.ustep);
	const uint32x4_t mvs = vld1q_u32(g.vstep);

	const uint32x4_t mumask = vdupq_n_u32(static_cast<uint32_t>(g.umask));
	const uint32x4_t mvmask = vdupq_n_u32(static_cast<uint32_t>(g.vmask));

	// A negative count shifts right; logical, as the lanes are unsigned.
	const int32x4_t cu = vdupq_n_s32(-g.ushift);
	const int32x4_t cv = vdupq_n_s32(-g.vshift);

	argb_t* dest = reinterpret_cast<argb_t*>(g.destination) +
			ptrdiff_t(g.xa) * g.colstep + g.y0;
	const int colstep = g.colstep;

	for (int n = g.xb - g.xa + 1; n; --n)
	{
		const uint32x4_t spots = vorrq_u32(
		    vandq_u32(vshlq_u32(mu, cu), mumask),
		    vandq_u32(vshlq_u32(mv, cv), mvmask));

		uint32x4_t texels = vld1q_dup_u32(s0 + source[vgetq_lane_u32(spots, 0)]);
		texels = vld1q_lane_u32(s1 + source[vgetq_lane_u32(spots, 1)], texels, 1);
		texels = vld1q_lane_u32(s2 + source[vgetq_lane_u32(spots, 2)], texels, 2);
		texels = vld1q_lane_u32(s3 + source[vgetq_lane_u32(spots, 3)], texels, 3);
		vst1q_u32(reinterpret_cast<uint32_t*>(dest), texels);

		mu = vaddq_u32(mu, mus);
		mv = vaddq_u32(mv, mvs);

		dest += colstep;
	}
}


//
// r_dimpatchD_NEON
//
// NEON port of r_dimpatchD_SSE2's blend
//
void r_dimpatchD_NEON(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	const int rowstep = surface->getRowStepInPixels();
	const int colstep = surface->getColStepInPixels();

	const uint16x8_t vec_color = vmovl_u8(vreinterpret_u8_u32(vdup_n_u32(color)));
	const uint16x8_t vec_alphacolor = vmulq_n_u16(vec_color, static_cast<uint16_t>(alpha));
	const uint16_t invalpha = static_cast<uint16_t>(256 - alpha);

	argb_t* column = reinterpret_cast<argb_t*>(surface->getBuffer()) + y1 * rowstep + x1 * colstep;

	for (int x = 0; x < w; x++, column += colstep)
	{
		argb_t* dest = column;
		int run = h;

		for (; run >= 4; run -= 4, dest += 4)
		{
			uint32_t* const d = reinterpret_cast<uint32_t*>(dest);
			const uint8x16_t in = vreinterpretq_u8_u32(vld1q_u32(d));

			const uint16x8_t lo = vmlaq_n_u16(vec_alphacolor, vmovl_u8(vget_low_u8(in)), invalpha);
			const uint16x8_t hi = vmlaq_n_u16(vec_alphacolor, vmovl_u8(vget_high_u8(in)), invalpha);

			vst1q_u32(d, vreinterpretq_u32_u8(vcombine_u8(vshrn_n_u16(lo, 8), vshrn_n_u16(hi, 8))));
		}

		for (; run > 0; run--, dest++)
			*dest = alphablend1a(*dest, color, alpha);
	}
}

VERSION_CONTROL (r_drawt_neon_cpp, "$Id$")

#endif
