// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: v_palette.h 1788 2010-08-24 04:42:57Z russellrice $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	V_PALETTE
//
//-----------------------------------------------------------------------------

#ifndef __V_PALETTE_H__
#define __V_PALETTE_H__

#include "doomtype.h"
#include "r_defs.h"

struct palette_t
{
	argb_t			basecolors[256];		// non-gamma corrected colors
	argb_t			colors[256];			// gamma corrected colors

	shademap_t      maps;
	byte			*colormapsbase;
};

struct dyncolormap_s {
	shaderef_t		maps;
	argb_t			color;
	argb_t			fade;
	struct dyncolormap_s *next;
};
typedef struct dyncolormap_s dyncolormap_t;

extern byte newgamma[256];

// Alpha blend between two RGB colors with only dest alpha value
// 0 <=   toa <= 256
argb_t alphablend1a(const argb_t from, const argb_t to, const int toa);
// Alpha blend between two RGB colors with two alpha values
// 0 <= froma <= 256
// 0 <=   toa <= 256
argb_t alphablend2a(const argb_t from, const int froma, const argb_t to, const int toa);

// InitPalettes()
//	input: name:  the name of the default palette lump
//				  (normally GAMEPAL)
//
// Returns a pointer to the default palette.
palette_t* InitPalettes(const char *name);

// GetDefaultPalette()
//
//	Returns the palette created through InitPalettes()
palette_t* GetDefaultPalette();

//
// V_RestoreScreenPalette
//
// Restore original screen palette from current gamma level
void V_RestoreScreenPalette();

// RefreshPalette()
//	input: pal: the palette to refresh
//
// Generates all colormaps or shadings for the specified palette
// with the current blending levels.
void RefreshPalette(palette_t* pal);

// Sets up the default colormaps and shademaps based on the given palette:
void BuildDefaultColorAndShademap (palette_t *pal, shademap_t &maps);
// Sets up the default shademaps (no colormaps) based on the given palette:
void BuildDefaultShademap (palette_t *pal, shademap_t &maps);

// GammaAdjustPalette()
//
// Builds the colors table for the specified palette based
// on the current gamma correction setting. It will not rebuild
// the shading table if the palette has one.
void GammaAdjustPalette(palette_t* pal);

// V_SetBlend()
//	input: blendr: red component of blend
//		   blendg: green component of blend
//		   blendb: blue component of blend
//		   blenda: alpha component of blend
//
void V_SetBlend (int blendr, int blendg, int blendb, int blenda);

// V_ForceBlend()
//
// Normally, V_SetBlend() does nothing if the new blend is the
// same as the old. This function will performing the blending
// even if the blend hasn't changed.
void V_ForceBlend (int blendr, int blendg, int blendb, int blenda);

void V_DoPaletteEffects();

void V_ResetPalette();

// Colorspace conversion RGB <-> HSV
void RGBtoHSV (float r, float g, float b, float *h, float *s, float *v);
void HSVtoRGB (float *r, float *g, float *b, float h, float s, float v);

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb);

#endif //__V_PALETTE_H__


