// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: v_palette.h 1788 2010-08-24 04:42:57Z russellrice $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2013 by The Odamex Team.
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

struct palette_s {
	struct palette_s *next, *prev;

	shademap_t      maps;
	byte			*colormapsbase;
	union {
		char		name[8];
		int			nameint[2];
	} name;
	argb_t			*colors;		// gamma corrected colors
	argb_t			*basecolors;	// non-gamma corrected colors
	unsigned		numcolors;
	unsigned		flags;
	unsigned		shadeshift;
	int				usecount;
};
typedef struct palette_s palette_t;

// Generate shading ramps for lighting
#define PALETTEB_SHADE		(0)
#define PALETTEF_SHADE		(1<<PALETTEB_SHADE)

// Apply blend color specified in V_SetBlend()
#define PALETTEB_BLEND		(1)
#define PALETTEF_BLEND		(1<<PALETTEB_SHADE)

// Default palette when none is specified (Do not set directly!)
#define PALETTEB_DEFAULT	(30)
#define PALETTEF_DEFAULT	(1<<PALETTEB_DEFAULT)



// Type values for LoadAttachedPalette():
#define LAP_PALETTE			(~0)	// Just pass thru to LoadPalette()
#define LAP_PATCH			(0)
#define LAP_SPRITE			(1)
#define LAP_FLAT			(2)
#define LAP_TEXTURE			(3)


struct dyncolormap_s {
	shaderef_t   maps;
	unsigned int color;
	unsigned int fade;
	struct dyncolormap_s *next;
};
typedef struct dyncolormap_s dyncolormap_t;


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
palette_t *InitPalettes (const char *name);

// GetDefaultPalette()
//
//	Returns the palette created through InitPalettes()
palette_t *GetDefaultPalette (void);

//
// V_RestoreScreenPalette
//
// Restore original screen palette from current gamma level
void V_RestoreScreenPalette(void);

// MakePalette()
//	input: colors: ptr to 256 3-byte RGB values
//		   name:   the palette's name (not checked for duplicates)
//		   flags:  the flags for the new palette
//
palette_t *MakePalette (byte *colors, char *name, unsigned flags);

// LoadPalette()
//	input: name:  the name of the palette lump
//		   flags: the flags for the palette
//
//	This function will try and find an already loaded
//	palette and return that if possible.
palette_t *LoadPalette (char *name, unsigned flags);

// LoadAttachedPalette()
//	input: name:  the name of a graphic whose palette should be loaded
//		   type:  the type of graphic whose palette is being requested
//		   flags: the flags for the palette
//
//	This function looks through the PALETTES lump for a palette
//	associated with the given graphic and returns that if possible.
palette_t *LoadAttachedPalette (char *name, int type, unsigned flags);

// FreePalette()
//	input: palette: the palette to free
//
//	This function decrements the palette's usecount and frees it
//	when it hits zero.
void FreePalette (palette_t *palette);

// FindPalette()
//	input: name:  the name of the palette
//		   flags: the flags to match on (~0 if it doesn't matter)
//
palette_t *FindPalette (char *name, unsigned flags);

// RefreshPalette()
//	input: pal: the palette to refresh
//
// Generates all colormaps or shadings for the specified palette
// with the current blending levels.
void RefreshPalette (palette_t *pal);

// Sets up the default colormaps and shademaps based on the given palette:
void BuildDefaultColorAndShademap (palette_t *pal, shademap_t &maps);
// Sets up the default shademaps (no colormaps) based on the given palette:
void BuildDefaultShademap (palette_t *pal, shademap_t &maps);

// RefreshPalettes()
//
// Calls RefreshPalette() for all palettes.
void RefreshPalettes (void);

// GammaAdjustPalette()
//
// Builds the colors table for the specified palette based
// on the current gamma correction setting. It will not rebuild
// the shading table if the palette has one.
void GammaAdjustPalette (palette_t *pal);

// GammaAdjustPalettes()
//
// Calls GammaAdjustPalette() for all palettes.
void GammaAdjustPalettes (void);

// V_SetBlend()
//	input: blendr: red component of blend
//		   blendg: green component of blend
//		   blendb: blue component of blend
//		   blenda: alpha component of blend
//
// Applies the blend to all palettes with PALETTEF_BLEND flag
void V_SetBlend (int blendr, int blendg, int blendb, int blenda);

// V_ForceBlend()
//
// Normally, V_SetBlend() does nothing if the new blend is the
// same as the old. This function will performing the blending
// even if the blend hasn't changed.
void V_ForceBlend (int blendr, int blendg, int blendb, int blenda);

void V_DoPaletteEffects();

// Colorspace conversion RGB <-> HSV
void RGBtoHSV (float r, float g, float b, float *h, float *s, float *v);
void HSVtoRGB (float *r, float *g, float *b, float h, float s, float v);

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb);

#endif //__V_PALETTE_H__


