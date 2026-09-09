// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	V_PALETTE
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_defs.h"

#include <algorithm>
#include <cmath>

struct palette_t
{
	argb_t			basecolors[256];		// non-gamma corrected colors
	argb_t			colors[256];			// gamma corrected colors

	shademap_t      maps;

	const palette_t& operator=(const palette_t& other)
	{
		for (size_t i = 0; i < 256; i++)
		{
			colors[i] = other.colors[i];
			basecolors[i] = other.basecolors[i];
		}
		maps = other.maps;
		return *this;
	}
};

struct dyncolormap_t {
	shaderef_t		maps;
	argb_t			color;
	argb_t			fade;
	dyncolormap_t *next;
};

extern fargb_t baseblend;

extern byte gammatable[256];
float V_GetMinimumGammaLevel();
float V_GetMaximumGammaLevel();
void V_IncrementGammaLevel();

static inline argb_t V_GammaCorrect(const argb_t value)
{
	extern byte gammatable[256];
	return argb_t(value.geta(), gammatable[value.getr()], gammatable[value.getg()], gammatable[value.getb()]);
}


palindex_t V_BestColor(const argb_t* palette_colors, int r, int g, int b);
palindex_t V_BestColor(const argb_t *palette_colors, argb_t color);

// Alpha blend between two RGB colors with only dest alpha value
// 0 <=   toa <= 256
argb_t alphablend1a(const argb_t from, const argb_t to, const int toa);
// Alpha blend between two RGB colors with two alpha values
// 0 <= froma <= 256
// 0 <=   toa <= 256
argb_t alphablend2a(const argb_t from, const int froma, const argb_t to, const int toa);

void V_InitPalette(const char* lumpname);


const palette_t* V_GetDefaultPalette();
const palette_t* V_GetGamePalette();

//
// V_RestoreScreenPalette
//
// Restore original screen palette from current gamma level
void V_RestoreScreenPalette();

// V_RefreshColormaps()
//
// Generates all colormaps or shadings for the default palette
// with the current blending levels.
void V_RefreshColormaps();

// Sets up the default colormaps and shademaps based on the given palette:
void BuildDefaultColorAndShademap(const palette_t* pal, shademap_t& maps);
// Sets up the default shademaps (no colormaps) based on the given palette:
void BuildDefaultShademap(const palette_t* pal, shademap_t& maps);

// V_SetBlend()
//	input: blendr: red component of blend
//		   blendg: green component of blend
//		   blendb: blue component of blend
//		   blenda: alpha component of blend
//
void V_SetBlend(const argb_t color);

// V_ForceBlend()
//
// Normally, V_SetBlend() does nothing if the new blend is the
// same as the old. This function will performing the blending
// even if the blend hasn't changed.
void V_ForceBlend(const argb_t color);

void V_DoPaletteEffects();

void V_ResetPalette();

/****** Colorspace Conversion Functions ******/

//
// V_RGBtoHSV
//
// Converts from the RGB color space to the HSV color space.
// Code from http://www.cs.rit.edu/~yxv4997/t_convert.html
//
// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
// if s == 0, then h = -1 (undefined)
// RGB - 0: {    .46  1 .429 } 7: {    .254 .571 .206 } 15: {    .0317 .0794 .0159 }
// HSV - 0: { 116.743 .571 1 } 7: { 112.110 .639 .571 } 15: { 105.071  .800 .0794 }
//
constexpr float hue_sector_degrees = 60.0f;
constexpr float hue_circle_degrees = 360.0f;

constexpr float hue_green_sector = 2.0f;
constexpr float hue_blue_sector = 4.0f;

inline fahsv_t V_RGBtoHSV(const fargb_t &color)
{
	const float a = color.geta();
	const float r = color.getr();
	const float g = color.getg();
	const float b = color.getb();

	const float smallest = std::min({r, g, b});
	const float largest = std::max({r, g, b});
	const float delta = largest - smallest;

	if (delta == 0.0f)
		return {a, 0, 0, largest};

	float hue;

	if (largest == r)
		hue = (g - b) / delta;                      // between yellow & magenta
	else if (largest == g)
		hue = hue_green_sector + ((b - r) / delta); // between cyan & yellow
	else
		hue = hue_blue_sector + ((r - g) / delta);  // between magenta & cyan

	hue *= hue_sector_degrees;
	if (hue < 0.0f)
		hue += hue_circle_degrees;

	return {a, hue, delta / largest, largest};
}

//
// V_HSVtoRGB
//
// Converts from the HSV color space to the RGB color space.
//
inline fargb_t V_HSVtoRGB(const fahsv_t &color)
{
	const float a = color.geta();
	const float h = color.geth();
	const float s = color.gets();
	const float v = color.getv();

	if (s == 0.0f)						// achromatic (grey)
		return {a, v, v, v};

	const float f = (h / hue_sector_degrees) - std::floor(h / hue_sector_degrees);
	const float p = v * (1.0f - s);
	const float q = v * (1.0f - (s * f));
	const float t = v * (1.0f - (s * (1.0f - f)));

	// The case labels are sector indices, not magic values.
	// NOLINTBEGIN(readability-magic-numbers)
	const int sector = static_cast<int>(h / hue_sector_degrees);
	switch (sector)
	{
		case 0:
			return {a, v, t, p};
		case 1:
			return {a, q, v, p};
		case 2:
			return {a, p, v, t};
		case 3:
			return {a, p, q, v};
		case 4:
			return {a, t, p, v};
		case 5:
			return {a, v, p, q};
		default:
			break;
	}
	// NOLINTEND(readability-magic-numbers)

	return {a, v, v, v};
}

//
// V_Luminance
//
// Perceived brightness of a color, 0 to 1.
// Palette index order is not brightness order, so anything that needs colors
// sorted dark to light has to go through this rather than comparing indices.
//
inline float V_Luminance(argb_t color)
{
	// Rec. 601 luma weights.
	constexpr float red_weight = 0.299f;
	constexpr float green_weight = 0.587f;
	constexpr float blue_weight = 0.114f;

	return ((red_weight * static_cast<float>(color.getr())) +
	        (green_weight * static_cast<float>(color.getg())) +
	        (blue_weight * static_cast<float>(color.getb()))) /
	       255.0f;
}

//
// V_ShadePlayerColor
//
// Shades base_color darker using the intensity of shade_color.
//
inline argb_t V_ShadePlayerColor(argb_t base_color, argb_t shade_color)
{
	if (base_color == shade_color)
		return base_color;

	// How much of each color's brightness survives the blend.
	constexpr float base_weight = 0.7f;
	constexpr float shade_weight = 0.3f;

	fahsv_t color = V_RGBtoHSV(base_color);
	color.setv((base_weight * color.getv()) +
	           (shade_weight * V_RGBtoHSV(shade_color).getv()));
	return V_HSVtoRGB(color);
}

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb);
