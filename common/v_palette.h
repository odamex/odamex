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
inline fahsv_t V_RGBtoHSV(const fargb_t &color)
{
	float a = color.geta(), r = color.getr(), g = color.getg(), b = color.getb();

	float smallest = std::min(std::min(r, g), b);
	float largest = std::max(std::max(r, g), b);
	float delta = largest - smallest;

	if (delta == 0.0f)
		return fahsv_t(a, 0, 0, largest);

	float hue;

	if (largest == r)
		hue = (g - b) / delta;					// between yellow & magenta
	else if (largest == g)
		hue = 2.0f + (b - r) / delta;				// between cyan & yellow
	else
		hue = 4.0f + (r - g) / delta;				// between magenta & cyan

	hue *= 60.f;
	if (hue < 0.0f)
		hue += 360.0f;

	return fahsv_t(a, hue, delta / largest, largest);
}

//
// V_HSVtoRGB
//
// Converts from the HSV color space to the RGB color space.
//
inline fargb_t V_HSVtoRGB(const fahsv_t &color)
{
	float a = color.geta(), h = color.geth(), s = color.gets(), v = color.getv();

	if (s == 0.0f)						// achromatic (grey)
		return fargb_t(a, v, v, v);

	float f = (h / 60.0f) - std::floor(h / 60.0f);
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	int sector = int(h / 60.0f);
	switch (sector)
	{
		case 0:
			return fargb_t(a, v, t, p);
		case 1:
			return fargb_t(a, q, v, p);
		case 2:
			return fargb_t(a, p, v, t);
		case 3:
			return fargb_t(a, p, q, v);
		case 4:
			return fargb_t(a, t, p, v);
		case 5:
			return fargb_t(a, v, p, q);
	}

	return fargb_t(a, v, v, v);
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
	return (0.299f * color.getr() + 0.587f * color.getg() + 0.114f * color.getb()) / 255.0f;
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

	fahsv_t color = V_RGBtoHSV(base_color);
	color.setv(0.7f * color.getv() + 0.3f * V_RGBtoHSV(shade_color).getv());
	return V_HSVtoRGB(color);
}

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb);
