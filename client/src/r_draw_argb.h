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
//	Shading for textures with a native ARGB plane, shared by the scalar
//	drawers and the vectorized ones that must agree with them bit for bit.
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_intrin.h"
#include "r_draw.h"
#include "r_main.h"
#include "v_palette.h"
#include "v_video.h"

#include <algorithm>

// ----------------------------------------------------------------------------
//
// 32bpp native ARGB color remapping functors
//
// These functors take a texel from a texture's native ARGB plane
// instead of a palette index. Light diminishing follows the same math as
// shaderef_t::shadeargb and the texel's alpha channel drives transparency:
// masked surfaces blend translucent texels with the scene already drawn
// behind them, while solid surfaces have no valid backdrop in the
// framebuffer and composite onto black instead.
//
// ----------------------------------------------------------------------------

//
// ARGB shade lookup tables
//
// For each light diminishing level, three 256-entry tables map a channel value
// to its diminished, faded and gamma-corrected result -- the shademap's math
// applied to a true color.
//
// Every dynamic colormap owns a set built from its light and fade color, so
// a texel costs three lookups whatever lights it.
// 
// Before, only white light under the level fade had tables, and a sector
// colormap -- NormalLight included, which lights every uncolored sector --
// paid two multiplies and two divisions per channel per pixel.
//
// Row NUMCOLORMAPS is full fade, where a special colormap's index is clamped.
//
struct argbshadelut_t
{
	byte r[NUMCOLORMAPS + 1][256];
	byte g[NUMCOLORMAPS + 1][256];
	byte b[NUMCOLORMAPS + 1][256];
};

//
// ARGBShader
//
// Shades native ARGB texels for one colormap: its dynamic colormap's tables at
// its light level.
//
class ARGBShader
{
public:
	ARGBShader() = default;

	ARGBShader(const shaderef_t& colormap) :
			ARGBShader(colormap.m_dyncolormap, colormap.mapnum()) { }

	// colormap.with(n) changes only the light level, so the plane drawers build a
	// band's shader from the base colormap's dynamic colormap and n directly.
	ARGBShader(const dyncolormap_t* dyncolormap, int mapnum)
	{
		mapnum = mapnum < NUMCOLORMAPS ? mapnum : NUMCOLORMAPS;

		// Every dynamic colormap gets its tables where it is made, so the default
		// is only reached without one.
		const argbshadelut_t* lut = R_DefaultARGBShadeLUT();
		if (dyncolormap != nullptr and dyncolormap->argblut != nullptr)
			lut = dyncolormap->argblut;

		lut_r = lut->r[mapnum];
		lut_g = lut->g[mapnum];
		lut_b = lut->b[mapnum];

		amask = argb_t::alphaMask();
		ashift = argb_t::alphaShift();
		rshift = argb_t::redShift();
		gshift = argb_t::greenShift();
		bshift = argb_t::blueShift();

		standard = ashift == 24 and rshift == 16 and gshift == 8 and bshift == 0;
	}

	// All this just to make the rgb shader LUT be one lookup per channel.
	forceinline argb_t shade(const argb_t c) const
	{
		const uint32_t v = c;
		if (standard)
			return argb_t((v & 0xFF000000u) |
			              (uint32_t(lut_r[(v >> 16) & 0xFF]) << 16) |
			              (uint32_t(lut_g[(v >> 8) & 0xFF]) << 8) |
			              uint32_t(lut_b[v & 0xFF]));

		return argb_t((v & amask) |
		              (uint32_t(lut_r[(v >> rshift) & 0xFF]) << rshift) |
		              (uint32_t(lut_g[(v >> gshift) & 0xFF]) << gshift) |
		              (uint32_t(lut_b[(v >> bshift) & 0xFF]) << bshift));
	}

	forceinline int alpha(const argb_t c) const
	{
		if (standard)
			return uint32_t(c) >> 24;
		return (uint32_t(c) >> ashift) & 0xFF;
	}

	// Opaque black, what a translucent pixel composites onto without a backdrop.
	forceinline argb_t black() const
	{
		return argb_t(amask);
	}

private:
	const byte* lut_r;
	const byte* lut_g;
	const byte* lut_b;

	uint32_t amask;
	int ashift, rshift, gshift, bshift;
	bool standard;		// alpha, red, green, blue from the top byte down
};

class DirectARGBColormapFunc
{
public:
	DirectARGBColormapFunc(const drawcolumn_t& drawcolumn) :
			shader(drawcolumn.colormap), blend_to_dest(drawcolumn.masked) { }

	forceinline void operator()(argb_t c, argb_t* dest) const
	{
		const int a = shader.alpha(c);
		if (a == 255)
		{
			*dest = shader.shade(c);
		}
		else if (a > 0)
		{
			const argb_t bg = blend_to_dest ? *dest : shader.black();
			*dest = alphablend2a(bg, 255 - a, shader.shade(c), a);
		}
		else if (not blend_to_dest)
		{
			*dest = shader.black();
		}
	}

private:
	const ARGBShader shader;
	const bool blend_to_dest;
};

class DirectARGBTranslucentColormapFunc
{
public:
	DirectARGBTranslucentColormapFunc(const drawcolumn_t& drawcolumn) :
			shader(drawcolumn.colormap)
	{
		calculate_alpha(drawcolumn.translevel);
	}

	forceinline void operator()(argb_t c, argb_t* dest) const
	{
		// combine the surface's translucency with the texel's alpha
		const int a = shader.alpha(c) * fga / 255;
		if (a == 0)
			return;

		*dest = alphablend2a(*dest, 255 - a, shader.shade(c), a);
	}

	// The surface's translucency, 0 to 255.
	int surfaceAlpha() const
	{
		return fga;
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
	}

	const ARGBShader shader;
	int fga;
};

//
// R_ShadeOpaqueARGB
//
// One texel of an opaque true-color surface: a plane has no backdrop, so a
// translucent texel composites onto black.
//
static forceinline argb_t R_ShadeOpaqueARGB(const ARGBShader& shader, const argb_t c)
{
	const int a = shader.alpha(c);
	if (a == 255)
		return shader.shade(c);
	if (a > 0)
		return alphablend2a(shader.black(), 255 - a, shader.shade(c), a);
	return shader.black();
}

//
// R_ARGBColumnSource
//
// The column's texels in its texture's native ARGB plane, which parallels the
// palettized plane dcol.source points into.
//
static forceinline const argb_t* R_ARGBColumnSource(const drawcolumn_t& dcol)
{
	return dcol.argbtexturedata + (dcol.source - dcol.texturedata);
}

// ----------------------------------------------------------------------------
//
// Shared by the vectorized drawers
//
// ----------------------------------------------------------------------------

static forceinline uint32_t R_ARGBBits(const argb_t c)
{
	return static_cast<uint32_t>(c);
}

// Whether any of four texels has alpha, and whether all four are opaque.
static forceinline bool R_ARGBAnyAlpha(const argb_t t0, const argb_t t1, const argb_t t2,
		const argb_t t3, const uint32_t amask)
{
	return ((R_ARGBBits(t0) | R_ARGBBits(t1) | R_ARGBBits(t2) | R_ARGBBits(t3)) & amask) != 0;
}

static forceinline bool R_ARGBAllOpaque(const argb_t t0, const argb_t t1, const argb_t t2,
		const argb_t t3, const uint32_t amask)
{
	return (R_ARGBBits(t0) & R_ARGBBits(t1) & R_ARGBBits(t2) & R_ARGBBits(t3) & amask) == amask;
}

//
// R_DrawARGBColumnQuadGeneric
//
// R_DrawColumnQuadGeneric over the native ARGB plane, for every instruction set:
// QUADFUNC::quad(t0, t1, t2, t3, dest) draws four pixels, and its operator()
// draws one exactly as the scalar functor does.
//
// MASKED picks the raw texel index of the masked arm over the power-of-two mask,
// transparency is the functor's business, by alpha.
//
template<typename QUADFUNC, bool MASKED>
static forceinline void R_DrawARGBColumnQuadGeneric(argb_t* dest, const drawcolumn_t& d)
forceinline void R_DrawARGBColumnQuadGeneric(argb_t* dest, const drawcolumn_t& d)
{
	int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const argb_t* const source = R_ARGBColumnSource(d);
	const fixed_t fracstep = d.iscale;
	fixed_t frac = d.texturefrac;

	const int mask = (d.textureheight >> FRACBITS) - 1;

	const QUADFUNC f(d);

	const auto texel = [source, mask](const fixed_t at) {
		return MASKED ? source[at >> FRACBITS] : source[(at >> FRACBITS) & mask];
	};

	// One pixel through the scalar functor itself.
	const auto single = [&]() {
		f(texel(frac), dest);
		dest++;
		frac += fracstep;
	};

	// `count >= 4` is the same correctness bound as in R_DrawColumnQuadGeneric.
	while (count >= 4)
	{
		f.quad(texel(frac), texel(frac + fracstep),
		       texel(frac + (fracstep * 2)), texel(frac + (fracstep * 3)), dest);

		dest += 4;
		frac += fracstep * 4;
		count -= 4;
	}

	for (; count > 0; count--)
		single();
}
