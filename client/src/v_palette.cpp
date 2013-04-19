// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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


#include <string.h>
#include <math.h>
#include <cstddef>

#include "v_video.h"
#include "m_alloc.h"
#include "r_main.h"		// For lighting constants
#include "w_wad.h"
#include "z_zone.h"
#include "i_video.h"
#include "c_dispatch.h"
#include "g_level.h"
#include "st_stuff.h"

EXTERN_CVAR(vid_gammatype)

void BuildColoredLights (byte *maps, int lr, int lg, int lb, int fr, int fg, int fb);
static void DoBlending (DWORD *from, DWORD *to, unsigned count, int tor, int tog, int tob, int toa);

dyncolormap_t NormalLight;

palette_t DefPal;
palette_t *FirstPal;

DWORD IndexedPalette[256];


/* Current color blending values */
int		BlendR, BlendG, BlendB, BlendA;


/**************************/
/* Gamma correction stuff */
/**************************/

enum {
	GAMMA_DOOM = 0,
	GAMMA_ZDOOM = 1
};

byte newgamma[256];
static void V_UpdateGammaLevel(float var)
{
	static float lastgamma = 0;
	static int lasttype = -1;			// ensure this gets set up the first time
	int type = vid_gammatype;

	if (lastgamma != var || lasttype != type)
	{
		// Only recalculate the gamma table if the new gamma
		// value is different from the old one.

		lastgamma = var;
		lasttype = type;

		if (vid_gammatype == GAMMA_ZDOOM)
		{
			// [SL] Use ZDoom 1.22 gamma correction

			// [RH] I found this formula on the web at
			// http://panda.mostang.com/sane/sane-gamma.html
			double invgamma = 1.0 / var;

			for (int i = 0; i < 256; i++)
				newgamma[i] = (BYTE)(255.0 * pow (i / 255.0, invgamma));
		}
		else
		{	
			// [SL] Use vanilla Doom's gamma table
			//
			// This was derived from the original Doom gammatable after some
			// trial and error and several beers.  The +0.5 is used to round
			// while the 255/256 is to scale to ensure 255 isn't exceeded.
			// This generates a 1:1 match with the original gammatable but also
			// allows for intermediate values.

			const double basefac = pow(2.0, var - 1.0) * (255.0/256.0);
			const double exp = 1.0 - 0.125 * (var - 1.0);

			for (int i = 0; i < 256; i++)
				newgamma[i] = (byte)(0.5 + basefac * pow(static_cast<double>(i + 1), exp));
		}

		GammaAdjustPalettes ();
		if (screen && screen->is8bit())
		{
			DoBlending (DefPal.colors, IndexedPalette, DefPal.numcolors,
						newgamma[BlendR], newgamma[BlendG], newgamma[BlendB], BlendA);
			I_SetPalette (IndexedPalette);
		}
	}
}


CVAR_FUNC_IMPL (gammalevel)
{
	if (var < 1.0f)
	{
		var.Set(1.0f);
		return;
	}
	else if (var > 8)
	{
		var.Set(8.0f);
		return;
	}

	V_UpdateGammaLevel(var);
}

CVAR_FUNC_IMPL(vid_gammatype)
{
	if (var < GAMMA_DOOM)
	{
		var.Set(GAMMA_DOOM);
		return;
	}
	else if (var > GAMMA_ZDOOM)
	{
		var.Set(GAMMA_ZDOOM);
		return;
	}

	V_UpdateGammaLevel(gammalevel);
}

// [Russell] - Restore original screen palette from current gamma level
void V_RestoreScreenPalette(void)
{
    if (screen && screen->is8bit())
    {
        DoBlending (DefPal.colors, IndexedPalette, DefPal.numcolors,
                    newgamma[BlendR], newgamma[BlendG], newgamma[BlendB], BlendA);

        I_SetPalette (IndexedPalette);
    }
}

/****************************/
/* Palette management stuff */
/****************************/

bool InternalCreatePalette (palette_t *palette, const char *name, byte *colors,
							unsigned numcolors, unsigned flags)
{
	unsigned i;

	if (numcolors > 256)
		numcolors = 256;
	else if (numcolors == 0)
		return false;

	strncpy (palette->name.name, name, 8);
	palette->flags = flags;
	palette->usecount = 1;
	palette->maps.colormaps = NULL;

	M_Free(palette->basecolors);

	palette->basecolors = (DWORD *)Malloc (numcolors * 2 * sizeof(DWORD));
	palette->colors = palette->basecolors + numcolors;
	palette->numcolors = numcolors;

	if (numcolors == 1)
		palette->shadeshift = 0;
	else if (numcolors <= 2)
		palette->shadeshift = 1;
	else if (numcolors <= 4)
		palette->shadeshift = 2;
	else if (numcolors <= 8)
		palette->shadeshift = 3;
	else if (numcolors <= 16)
		palette->shadeshift = 4;
	else if (numcolors <= 32)
		palette->shadeshift = 5;
	else if (numcolors <= 64)
		palette->shadeshift = 6;
	else if (numcolors <= 128)
		palette->shadeshift = 7;
	else
		palette->shadeshift = 8;

	for (i = 0; i < numcolors; i++, colors += 3)
		palette->basecolors[i] = MAKERGB(colors[0],colors[1],colors[2]);

	GammaAdjustPalette (palette);

	return true;
}

palette_t *InitPalettes (const char *name)
{
	byte *colors;

	//if (DefPal.usecount)
	//	return &DefPal;

	if ( (colors = (byte *)W_CacheLumpName (name, PU_CACHE)) )
		if (InternalCreatePalette (&DefPal, name, colors, 256,
									PALETTEF_SHADE|PALETTEF_BLEND|PALETTEF_DEFAULT)) {
			return &DefPal;
		}
	return NULL;
}

palette_t *GetDefaultPalette (void)
{
	return &DefPal;
}

// MakePalette()
//	input: colors: ptr to 256 3-byte RGB values
//		   flags:  the flags for the new palette
//
palette_t *MakePalette (byte *colors, char *name, unsigned flags)
{
	palette_t *pal;

	pal = (palette_t *)Malloc (sizeof (palette_t));

	if (InternalCreatePalette (pal, name, colors, 256, flags)) {
		pal->next = FirstPal;
		pal->prev = NULL;
		FirstPal = pal;

		return pal;
	} else {
		M_Free(pal);
		return NULL;
	}
}

// LoadPalette()
//	input: name:  the name of the palette lump
//		   flags: the flags for the palette
//
//	This function will try and find an already loaded
//	palette and return that if possible.
palette_t *LoadPalette (char *name, unsigned flags)
{
	palette_t *pal;

	if (!(pal = FindPalette (name, flags))) {
		// Palette doesn't already exist. Create a new one.
		byte *colors = (byte *)W_CacheLumpName (name, PU_CACHE);

		pal = MakePalette (colors, name, flags);
	} else {
		pal->usecount++;
	}
	return pal;
}

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
void FreePalette (palette_t *palette)
{
	if (!(--palette->usecount)) {
		if (!(palette->flags & PALETTEF_DEFAULT)) {
			if (!palette->prev)
				FirstPal = palette->next;
			else
				palette->prev->next = palette->next;

			M_Free(palette->basecolors);

			M_Free(palette->colormapsbase);

			M_Free(palette);
		}
	}
}


palette_t *FindPalette (char *name, unsigned flags)
{
	palette_t *pal = FirstPal;
	union {
		char	s[9];
		int		x[2];
	} name8;

	int			v1;
	int			v2;

	// make the name into two integers for easy compares
	strncpy (name8.s,name,8);

	v1 = name8.x[0];
	v2 = name8.x[1];

	while (pal) {
		if (pal->name.nameint[0] == v1 && pal->name.nameint[1] == v2) {
			if ((flags == (unsigned)~0) || (flags == pal->flags))
				return pal;
		}
		pal = pal->next;
	}
	return NULL;
}


// This is based (loosely) on the ColorShiftPalette()
// function from the dcolors.c file in the Doom utilities.
static void DoBlending (DWORD *from, DWORD *to, unsigned count, int tor, int tog, int tob, int toa)
{
	unsigned i;

	if (toa == 0) {
		if (from != to)
			memcpy (to, from, count * sizeof(unsigned));
	} else {
		int dr,dg,db,r,g,b;

		for (i = 0; i < count; i++) {
			r = RPART(*from);
			g = GPART(*from);
			b = BPART(*from);
			from++;
			dr = tor - r;
			dg = tog - g;
			db = tob - b;
			*to++ = MAKERGB (r + ((dr*toa)>>8),
							 g + ((dg*toa)>>8),
							 b + ((db*toa)>>8));
		}
	}

}


void RefreshPalette (palette_t *pal)
{
	DWORD l,c,r,g,b;
	DWORD colors[256];

	if (screen->is8bit()) {
		if (pal->flags & PALETTEF_SHADE) {
			byte *shade;

			r = RPART (level.fadeto);
			g = GPART (level.fadeto);
			b = BPART (level.fadeto);
			if (pal->maps.colormaps && pal->maps.colormaps - pal->colormapsbase >= 256) {
				M_Free(pal->maps.colormaps);
			}
			pal->colormapsbase = (byte *)Realloc (pal->colormapsbase, (NUMCOLORMAPS + 1) * 256 + 255);
			pal->maps.colormaps = (byte *)(((ptrdiff_t)(pal->colormapsbase) + 255) & ~0xff);

			// build normal light mappings
			for (l = 0; l < NUMCOLORMAPS; l++) {
				DoBlending (pal->basecolors, colors, pal->numcolors, r, g, b, l * (256 / NUMCOLORMAPS));

				shade = pal->maps.colormaps + (l << pal->shadeshift);
				for (c = 0; c < pal->numcolors; c++) {
					*shade++ = BestColor (pal->basecolors,
										  RPART(colors[c]),
										  GPART(colors[c]),
										  BPART(colors[c]),
										  pal->numcolors);
				}
			}

			// build special maps (e.g. invulnerability)
			shade = pal->maps.colormaps + (NUMCOLORMAPS << pal->shadeshift);
			{
				int grayint;

				for (c = 0; c < pal->numcolors; c++) {
					grayint = (int)(255.0f * (1.0f -
						(RPART(pal->basecolors[c]) * 0.00116796875f +
						 GPART(pal->basecolors[c]) * 0.00229296875f +
						 BPART(pal->basecolors[c]) * 0.0005625)));
					*shade++ = BestColor (pal->basecolors, grayint, grayint, grayint, pal->numcolors);
				}
			}
		}
	} else {
		if (pal->flags & PALETTEF_SHADE) {
			r = newgamma[RPART (level.fadeto)];
			g = newgamma[GPART (level.fadeto)];
			b = newgamma[BPART (level.fadeto)];

            M_Free(pal->colormapsbase);

			pal->maps.shades = (DWORD *)Realloc (pal->colormapsbase, (NUMCOLORMAPS + 1)*256*sizeof(DWORD) + 255);

			// build normal light mappings
			for (l = 0; l < NUMCOLORMAPS; l++) {
				DoBlending (pal->colors,
							pal->maps.shades + (l << pal->shadeshift),
							pal->numcolors,
							r, g, b,
							l * (256 / NUMCOLORMAPS));
			}

			// build special maps (e.g. invulnerability)
			{
				DWORD *shade = pal->maps.shades + (NUMCOLORMAPS << pal->shadeshift);
				int grayint;

				for (c = 0; c < pal->numcolors; c++) {
					grayint = (int)(255.0f * (1.0f -
						(RPART(pal->colors[c]) * 0.00116796875f +
						 GPART(pal->colors[c]) * 0.00229296875f +
						 BPART(pal->colors[c]) * 0.0005625)));
					*shade++ = MAKERGB (grayint, grayint, grayint);
				}
			}
		}
	}

	if (pal == &DefPal) {
		NormalLight.maps = DefPal.maps.colormaps;
		NormalLight.color = MAKERGB(255,255,255);
		NormalLight.fade = level.fadeto;
	}
}

void RefreshPalettes (void)
{
	palette_t *pal = FirstPal;

	RefreshPalette (&DefPal);
	while (pal) {
		RefreshPalette (pal);
		pal = pal->next;
	}
}


void GammaAdjustPalette (palette_t *pal)
{
	unsigned i, color;

	if (pal->colors && pal->basecolors) {
		for (i = 0; i < pal->numcolors; i++) {
			color = pal->basecolors[i];
			pal->colors[i] = MAKERGB (
				newgamma[RPART(color)],
				newgamma[GPART(color)],
				newgamma[BPART(color)]
			);
		}
	}
}

void GammaAdjustPalettes (void)
{
	palette_t *pal = FirstPal;

	GammaAdjustPalette (&DefPal);
	while (pal) {
		GammaAdjustPalette (pal);
		pal = pal->next;
	}
}

void V_SetBlend (int blendr, int blendg, int blendb, int blenda)
{
	// Don't do anything if the new blend is the same as the old
	if ((blenda == 0 && BlendA == 0) ||
		(blendr == BlendR &&
		 blendg == BlendG &&
		 blendb == BlendB &&
		 blenda == BlendA))
		return;

	V_ForceBlend (blendr, blendg, blendb, blenda);
}

void V_ForceBlend (int blendr, int blendg, int blendb, int blenda)
{
	BlendR = blendr;
	BlendG = blendg;
	BlendB = blendb;
	BlendA = blenda;

	if (screen->is8bit()) {
		DoBlending (DefPal.colors, IndexedPalette, DefPal.numcolors,
					newgamma[BlendR], newgamma[BlendG], newgamma[BlendB], BlendA);
		I_SetPalette (IndexedPalette);
	} else {
		RefreshPalettes();
	}
}

BEGIN_COMMAND (testblend)
{
	int color;
	float amt;

	if (argc < 3)
	{
		Printf (PRINT_HIGH, "testblend <color> <amount>\n");
	}
	else
	{
		std::string colorstring = V_GetColorStringByName (argv[1]);

		if (colorstring.length())
			color = V_GetColorFromString (NULL, colorstring.c_str());
		else
			color = V_GetColorFromString (NULL, argv[1]);

		amt = (float)atof (argv[2]);
		if (amt > 1.0f)
			amt = 1.0f;
		else if (amt < 0.0f)
			amt = 0.0f;
		//V_SetBlend (RPART(color), GPART(color), BPART(color), (int)(amt * 256.0f));
		BaseBlendR = RPART(color);
		BaseBlendG = GPART(color);
		BaseBlendB = BPART(color);
		BaseBlendA = amt;
	}
}
END_COMMAND (testblend)

BEGIN_COMMAND (testfade)
{

	int color;

	if (argc < 2)
	{
		Printf (PRINT_HIGH, "testfade <color>\n");
	}
	else
	{
		std::string colorstring = V_GetColorStringByName (argv[1]);
		if (colorstring.length())
			color = V_GetColorFromString (NULL, colorstring.c_str());
		else
			color = V_GetColorFromString (NULL, argv[1]);

		level.fadeto = color;
		RefreshPalettes();
		NormalLight.maps = DefPal.maps.colormaps;
	}
}
END_COMMAND (testfade)

/****** Colorspace Conversion Functions ******/

// Code from http://www.cs.rit.edu/~yxv4997/t_convert.html

// r,g,b values are from 0 to 1
// h = [0,360], s = [0,1], v = [0,1]
//				if s == 0, then h = -1 (undefined)

// Green Doom guy colors:
// RGB - 0: {    .46  1 .429 } 7: {    .254 .571 .206 } 15: {    .0317 .0794 .0159 }
// HSV - 0: { 116.743 .571 1 } 7: { 112.110 .639 .571 } 15: { 105.071  .800 .0794 }
void RGBtoHSV (float r, float g, float b, float *h, float *s, float *v)
{
	float min, max, delta, foo;

	if (r == g && g == b) {
		*h = 0;
		*s = 0;
		*v = r;
		return;
	}

	foo = r < g ? r : g;
	min = (foo < b) ? foo : b;
	foo = r > g ? r : g;
	max = (foo > b) ? foo : b;

	*v = max;									// v

	delta = max - min;

	*s = delta / max;							// s

	if (r == max)
		*h = (g - b) / delta;					// between yellow & magenta
	else if (g == max)
		*h = 2 + (b - r) / delta;				// between cyan & yellow
	else
		*h = 4 + (r - g) / delta;				// between magenta & cyan

	*h *= 60;									// degrees
	if (*h < 0)
		*h += 360;
}

void HSVtoRGB (float *r, float *g, float *b, float h, float s, float v)
{
	int i;
	float f, p, q, t;

	if (s == 0) {
		// achromatic (grey)
		*r = *g = *b = v;
		return;
	}

	h /= 60;									// sector 0 to 5
	i = (int)floor (h);
	f = h - i;									// factorial part of h
	p = v * (1 - s);
	q = v * (1 - s * f);
	t = v * (1 - s * (1 - f));

	switch (i) {
		case 0:		*r = v; *g = t; *b = p; break;
		case 1:		*r = q; *g = v; *b = p; break;
		case 2:		*r = p; *g = v; *b = t; break;
		case 3:		*r = p; *g = q; *b = v; break;
		case 4:		*r = t; *g = p; *b = v; break;
		default:	*r = v; *g = p; *b = q; break;
	}
}

/****** Colored Lighting Stuffs (Sorry, 8-bit only) ******/

// Builds NUMCOLORMAPS colormaps lit with the specified color
void BuildColoredLights (byte *maps, int lr, int lg, int lb, int r, int g, int b)
{
	unsigned int l,c;
	DWORD colors[256];
	byte *shade;

	// The default palette is assumed to contain the maps for white light.
	if (!screen->is8bit() || !maps)
		return;

	// build normal (but colored) light mappings
	for (l = 0; l < NUMCOLORMAPS; l++) {
		DoBlending (DefPal.basecolors, colors, DefPal.numcolors, r, g, b, l * (256 / NUMCOLORMAPS));

		shade = maps + 256*l;
		for (c = 0; c < 256; c++) {
			*shade++ = BestColor (DefPal.basecolors,
								  (RPART(colors[c])*lr)/255,
								  (GPART(colors[c])*lg)/255,
								  (BPART(colors[c])*lb)/255,
								  256);
		}
	}
}

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb)
{
	unsigned int color = MAKERGB (lr, lg, lb);
	unsigned int fade = MAKERGB (fr, fg, fb);
	dyncolormap_t *colormap = &NormalLight;

	// Bah! Simple linear search because I want to get this done.
	while (colormap) {
		if (color == colormap->color && fade == colormap->fade)
			return colormap;
		else
			colormap = colormap->next;
	}

	// Not found. Create it.
	colormap = (dyncolormap_t *)Z_Malloc (sizeof(*colormap), PU_LEVEL, 0);
	colormap->maps = (byte *)Z_Malloc (NUMCOLORMAPS*256+3+255, PU_LEVEL, 0);
	colormap->maps = (byte *)(((ptrdiff_t)colormap->maps + 255) & ~0xff);
	colormap->color = color;
	colormap->fade = fade;
	colormap->next = NormalLight.next;
	NormalLight.next = colormap;

	BuildColoredLights (colormap->maps, lr, lg, lb, fr, fg, fb);

	return colormap;
}

BEGIN_COMMAND (testcolor)
{
	int color;

	if (argc < 2)
	{
		Printf (PRINT_HIGH, "testcolor <color>\n");
	}
	else
	{
		std::string colorstring = V_GetColorStringByName (argv[1]);

		if (colorstring.length())
			color = V_GetColorFromString (NULL, colorstring.c_str());
		else
			color = V_GetColorFromString (NULL, argv[1]);

		BuildColoredLights (NormalLight.maps, RPART(color), GPART(color), BPART(color),
			RPART(level.fadeto), GPART(level.fadeto), BPART(level.fadeto));
	}
}
END_COMMAND (testcolor)

VERSION_CONTROL (v_palette_cpp, "$Id$")

