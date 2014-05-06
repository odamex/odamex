// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#include <cstring>
#include <math.h>
#include <cstddef>

#include "doomstat.h"
#include "v_video.h"
#include "v_gamma.h"
#include "m_alloc.h"
#include "r_main.h"		// For lighting constants
#include "w_wad.h"
#include "z_zone.h"
#include "i_video.h"
#include "c_dispatch.h"
#include "g_level.h"
#include "st_stuff.h"

// Palette indices.
// For damage/bonus red-/gold-shifts
#define STARTREDPALS		1
#define STARTBONUSPALS		9
#define NUMREDPALS			8
#define NUMBONUSPALS		4
// Radiation suit, green shift.
#define RADIATIONPAL		13

EXTERN_CVAR(gammalevel)
EXTERN_CVAR(vid_gammatype)
EXTERN_CVAR(r_painintensity)
EXTERN_CVAR(sv_allowredscreen)

void BuildColoredLights (byte *maps, int lr, int lg, int lb, int fr, int fg, int fb);
static void DoBlending(argb_t *from, argb_t *to, int tor, int tog, int tob, int toa);
void V_ForceBlend (int blendr, int blendg, int blendb, int blenda);

dyncolormap_t NormalLight;

static int lu_palette;
static int current_palette_num;
static float current_blend[4];

static palette_t DefPal;

/* Current color blending values */
int		BlendR, BlendG, BlendB, BlendA;

translationref_t::translationref_t() : m_table(NULL), m_player_id(-1)
{
}

translationref_t::translationref_t(const translationref_t &other) : m_table(other.m_table), m_player_id(other.m_player_id)
{
}

translationref_t::translationref_t(const byte *table) : m_table(table), m_player_id(-1)
{
}

translationref_t::translationref_t(const byte *table, const int player_id) : m_table(table), m_player_id(player_id)
{
}

shaderef_t::shaderef_t() : m_colors(NULL), m_mapnum(-1), m_colormap(NULL), m_shademap(NULL)
{
}

shaderef_t::shaderef_t(const shaderef_t &other)
	: m_colors(other.m_colors), m_mapnum(other.m_mapnum),
	  m_colormap(other.m_colormap), m_shademap(other.m_shademap), m_dyncolormap(other.m_dyncolormap)
{
}

shaderef_t::shaderef_t(const shademap_t * const colors, const int mapnum) : m_colors(colors), m_mapnum(mapnum)
{
#if DEBUG
	// NOTE(jsd): Arbitrary value picked here because we don't record the max number of colormaps for dynamic ones... or do we?
	if (m_mapnum >= 8192)
	{
		char tmp[100];
		sprintf_s(tmp, "32bpp: shaderef_t::shaderef_t() called with mapnum = %d, which looks too large", m_mapnum);
		throw CFatalError(tmp);
	}
#endif

	if (m_colors != NULL)
	{
		if (m_colors->colormap != NULL)
			m_colormap = m_colors->colormap + (256 * m_mapnum);
		else
			m_colormap = NULL;

		if (m_colors->shademap != NULL)
			m_shademap = m_colors->shademap + (256 * m_mapnum);
		else
			m_shademap = NULL;

		// Detect if the colormap is dynamic:
		m_dyncolormap = NULL;

		extern palette_t DefPal;
		if (m_colors != &(DefPal.maps))
		{
			// Find the dynamic colormap by the `m_colors` pointer:
			extern dyncolormap_t NormalLight;
			dyncolormap_t *colormap = &NormalLight;

			do
			{
				if (m_colors == colormap->maps.m_colors)
				{
					m_dyncolormap = colormap;
					break;
				}
				colormap = colormap->next;
			} while (colormap);
		}
	}
	else
	{
		m_colormap = NULL;
		m_shademap = NULL;
		m_dyncolormap = NULL;
	}
}

/**************************/
/* Gamma correction stuff */
/**************************/

static DoomGammaStrategy doomgammastrat;
static ZDoomGammaStrategy zdoomgammastrat;
GammaStrategy* gammastrat = &doomgammastrat;

CVAR_FUNC_IMPL(vid_gammatype)
{
	if (vid_gammatype == GAMMA_ZDOOM)
		gammastrat = &zdoomgammastrat;
	else
		gammastrat = &doomgammastrat;

	gammalevel.Set(gammalevel);
}

byte newgamma[256];
static bool gamma_initialized = false;

static void V_UpdateGammaLevel(float level)
{
	static float lastgammalevel = 0.0f;
	static int lasttype = -1;			// ensure this gets set up the first time
	int type = vid_gammatype;

	if (lastgammalevel != level || lasttype != type)
	{
		// Only recalculate the gamma table if the new gamma
		// value is different from the old one.

		lastgammalevel = level;
		lasttype = type;

		gammastrat->generateGammaTable(newgamma, level);
		GammaAdjustPalette(GetDefaultPalette());

		if (!screen)
			return;
		if (I_GetVideoBitDepth() == 8)
			V_ForceBlend(BlendR, BlendG, BlendB, BlendA);
		else
			RefreshPalette(GetDefaultPalette());
	}
}

CVAR_FUNC_IMPL(gammalevel)
{
	float sanitized_var = clamp(var.value(), gammastrat->min(), gammastrat->max());
	if (var == sanitized_var)
		V_UpdateGammaLevel(var);
	else
		var.Set(sanitized_var);
}

BEGIN_COMMAND(bumpgamma)
{
	gammalevel.Set(gammastrat->increment(gammalevel));

	if (gammalevel.value() == gammastrat->min())
	    Printf (PRINT_HIGH, "Gamma correction off\n");
	else
	    Printf (PRINT_HIGH, "Gamma correction level %g\n", gammalevel.value());
}
END_COMMAND(bumpgamma)


// [Russell] - Restore original screen palette from current gamma level
void V_RestoreScreenPalette(void)
{
    if (screen && I_GetVideoBitDepth() == 8)
		V_ForceBlend(BlendR, BlendG, BlendB, BlendA);
}

/****************************/
/* Palette management stuff */
/****************************/

//
// V_SetPalette
//
// Sets the video adapter's palette to the given 768 byte palette lump.
//
static void V_SetPalette(const byte* data)
{
	const int alpha = 255;
	argb_t palette_colors[256];

	for (int i = 0; i < 256; i++, data += 3)
	{
		palette_colors[i].a = alpha;
		palette_colors[i].r = newgamma[data[0]];
		palette_colors[i].g = newgamma[data[1]];
		palette_colors[i].b = newgamma[data[2]];
	}

	I_SetPalette(palette_colors);
}


static void InternalCreatePalette(palette_t* palette, const byte* data)
{
	palette->maps.colormap = NULL;
	palette->maps.shademap = NULL;

	const int alpha = 255;

	for (int i = 0; i < 256; i++, data += 3)
	{
		argb_t color(alpha, data[0], data[1], data[2]);
		palette->basecolors[i] = color; 
		palette->colors[i].a = alpha;
		palette->colors[i].r = newgamma[color.r];
		palette->colors[i].g = newgamma[color.g];
		palette->colors[i].b = newgamma[color.b];
	}
}


palette_t* InitPalettes(const char* lumpname)
{
	current_palette_num = -1;
	current_blend[0] = current_blend[1] = current_blend[2] = current_blend[3] = 255.0f;

    lu_palette = W_GetNumForName(lumpname);
	if (lu_palette != -1)
	{
		const byte* data = (byte*)W_CacheLumpName(lumpname, PU_CACHE);
		InternalCreatePalette(&DefPal, data);
		return &DefPal;
	}

	return NULL;
}


palette_t* GetDefaultPalette()
{
	return &DefPal;
}


// This is based (loosely) on the ColorShiftPalette()
// function from the dcolors.c file in the Doom utilities.
static void DoBlending(argb_t *from, argb_t *to, int tor, int tog, int tob, int toa)
{
	if (toa == 0)
	{
		if (from != to)
			memcpy(to, from, 256 * sizeof(argb_t));
	}
	else
	{
		for (int i = 0; i < 256; i++, from++, to++)
		{
			int r = from->r;
			int g = from->g;
			int b = from->b;

			int dr = tor - r;
			int dg = tog - g;
			int db = tob - b;

			to->r = r + ((dr * toa) >> 8);
			to->g = g + ((dg * toa) >> 8);
			to->b = b + ((db * toa) >> 8);
		}
	}
}

static void DoBlendingWithGamma(argb_t* from, argb_t* to, int tor, int tog, int tob, int toa)
{
	for (int i = 0; i < 256; i++, from++, to++)
	{
		int r = from->r;
		int g = from->g;
		int b = from->b;

		int dr = tor - r;
		int dg = tog - g;
		int db = tob - b;

		to->r = newgamma[r + ((dr * toa) >> 8)];
		to->g = newgamma[g + ((dg * toa) >> 8)];
		to->b = newgamma[b + ((db * toa) >> 8)];
	}
}

static const float lightScale(float a)
{
	// NOTE(jsd): Revised inverse logarithmic scale; near-perfect match to COLORMAP lump's scale
	// 1 - ((Exp[1] - Exp[a*2 - 1]) / (Exp[1] - Exp[-1]))
	static float e1 = exp(1.0f);
	static float e1sube0 = e1 - exp(-1.0f);

	float newa = clamp(1.0f - (e1 - (float)exp(a * 2.0f - 1.0f)) / e1sube0, 0.0f, 1.0f);
	return newa;
}

void BuildLightRamp (shademap_t &maps)
{
	int l;
	// Build light ramp:
	for (l = 0; l < 256; ++l)
	{
		int a = (int)(255 * lightScale(l / 255.0f));
		maps.ramp[l] = a;
	}
}

void BuildDefaultColorAndShademap(palette_t *pal, shademap_t &maps)
{
	BuildLightRamp(maps);

	// [SL] Modified algorithm from RF_BuildLights in dcolors.c
	// from Doom Utilities. Now accomodates fading to non-black colors.

	const argb_t* palette = pal->basecolors;
	argb_t fadecolor = level.fadeto;
	
	palindex_t* colormap = maps.colormap;
	argb_t* shademap = maps.shademap;

	for (int i = 0; i < NUMCOLORMAPS; i++, colormap += 256, shademap += 256)
	{
		for (int c = 0; c < 256; c++)
		{
			unsigned int r = (palette[c].r * (NUMCOLORMAPS - i) + fadecolor.r * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int g = (palette[c].g * (NUMCOLORMAPS - i) + fadecolor.g * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int b = (palette[c].b * (NUMCOLORMAPS - i) + fadecolor.b * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;

			colormap[c] = BestColor(palette, r, g, b, 256);
			shademap[c] = argb_t(newgamma[r], newgamma[g], newgamma[b]);
		}
	}

	// build special maps (e.g. invulnerability)
	for (int c = 0; c < 256; c++)
	{
		int grayint = (int)(255.0f * clamp(1.0f -
						(palette[c].r * 0.00116796875f +
						 palette[c].g * 0.00229296875f +
			 			 palette[c].b * 0.0005625f), 0.0f, 1.0f));

		colormap[c] = BestColor(palette, grayint, grayint, grayint, 256);
		shademap[c] = argb_t(newgamma[grayint], newgamma[grayint], newgamma[grayint]);
	}
}

void BuildDefaultShademap(palette_t *pal, shademap_t &maps)
{
	BuildLightRamp(maps);

	// [SL] Modified algorithm from RF_BuildLights in dcolors.c
	// from Doom Utilities. Now accomodates fading to non-black colors.

	const argb_t* palette = pal->basecolors;
	argb_t fadecolor = level.fadeto;
	
	argb_t* shademap = maps.shademap;

	for (int i = 0; i < NUMCOLORMAPS; i++, shademap += 256)
	{
		for (int c = 0; c < 256; c++)
		{
			unsigned int r = (palette[c].r * (NUMCOLORMAPS - i) + fadecolor.r * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int g = (palette[c].g * (NUMCOLORMAPS - i) + fadecolor.g * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int b = (palette[c].b * (NUMCOLORMAPS - i) + fadecolor.b * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;

			shademap[c] = argb_t(newgamma[r], newgamma[g], newgamma[b]);
		}
	}

	// build special maps (e.g. invulnerability)
	for (int c = 0; c < 256; c++)
	{
		int grayint = (int)(255.0f * clamp(1.0f -
						(palette[c].r * 0.00116796875f +
						 palette[c].g * 0.00229296875f +
			 			 palette[c].b * 0.0005625f), 0.0f, 1.0f));

		shademap[c] = argb_t(newgamma[grayint], newgamma[grayint], newgamma[grayint]);
	}
}

void RefreshPalette(palette_t* pal)
{
	if (pal->maps.colormap && pal->maps.colormap - pal->colormapsbase >= 256)
		M_Free(pal->maps.colormap);

	pal->colormapsbase = (byte*)Realloc(pal->colormapsbase, (NUMCOLORMAPS + 1) * 256 + 255);
	pal->maps.colormap = (byte*)(((ptrdiff_t)(pal->colormapsbase) + 255) & ~0xff);
	pal->maps.shademap = (argb_t*)Realloc(pal->maps.shademap, (NUMCOLORMAPS + 1)*256*sizeof(argb_t) + 255);

	BuildDefaultColorAndShademap(pal, pal->maps);

	if (pal == &DefPal)
	{
		NormalLight.maps = shaderef_t(&DefPal.maps, 0);
		NormalLight.color = argb_t(255, 255, 255);
		NormalLight.fade = level.fadeto;
	}
}

void GammaAdjustPalette(palette_t* pal)
{
	if (!gamma_initialized)
		V_UpdateGammaLevel(gammalevel);

	for (int i = 0; i < 256; i++)
	{
		pal->colors[i].r = newgamma[pal->basecolors[i].r];
		pal->colors[i].g = newgamma[pal->basecolors[i].g];
		pal->colors[i].b = newgamma[pal->basecolors[i].b];
	}
}


//
// V_AddBlend
//
// [RH] This is from Q2.
//
void V_AddBlend(float r, float g, float b, float a, float* v_blend)
{
	float a2, a3;

	if (a <= 0.0f)
		return;
	a2 = v_blend[3] + (1.0f - v_blend[3]) * a;	// new total alpha
	a3 = v_blend[3] / a2;		// fraction of color from old

	v_blend[0] = v_blend[0] * a3 + r*(1.0f - a3);
	v_blend[1] = v_blend[1] * a3 + g*(1.0f - a3);
	v_blend[2] = v_blend[2] * a3 + b*(1.0f - a3);
	v_blend[3] = a2;
}

void V_SetBlend(int blendr, int blendg, int blendb, int blenda)
{
	// Don't do anything if the new blend is the same as the old
	if ((blenda == 0 && BlendA == 0) ||
		(blendr == BlendR &&
		 blendg == BlendG &&
		 blendb == BlendB &&
		 blenda == BlendA))
		return;

	V_ForceBlend(blendr, blendg, blendb, blenda);
}

void V_ForceBlend(int blendr, int blendg, int blendb, int blenda)
{
	BlendR = blendr;
	BlendG = blendg;
	BlendB = blendb;
	BlendA = blenda;

	// blend the palette for 8-bit mode
	// shademap_t::shade takes care of blending
	// [SL] actually, an alpha overlay is drawn on top of the rendered screen
	// in R_RenderPlayerView
	if (I_GetVideoBitDepth() == 8)
	{
		argb_t palette_colors[256];
		DoBlending(DefPal.colors, palette_colors, newgamma[BlendR], newgamma[BlendG], newgamma[BlendB], BlendA);
		I_SetPalette(palette_colors);
	}
}

BEGIN_COMMAND (testblend)
{
	if (argc < 3)
	{
		Printf (PRINT_HIGH, "testblend <color> <amount>\n");
	}
	else
	{
		std::string colorstring = V_GetColorStringByName(argv[1]);
		argb_t color;

		if (!colorstring.empty())
			color = (argb_t)V_GetColorFromString(NULL, colorstring.c_str());
		else
			color = (argb_t)V_GetColorFromString(NULL, argv[1]);

		float amt = clamp((float)atof(argv[2]), 0.0f, 1.0f);

		BaseBlendR = color.r;
		BaseBlendG = color.g;
		BaseBlendB = color.b;
		BaseBlendA = amt;
	}
}
END_COMMAND (testblend)

BEGIN_COMMAND (testfade)
{
	if (argc < 2)
	{
		Printf (PRINT_HIGH, "testfade <color>\n");
	}
	else
	{
		std::string colorstring = V_GetColorStringByName(argv[1]);
		argb_t color;

		if (!colorstring.empty())
			color = (argb_t)V_GetColorFromString(NULL, colorstring.c_str());
		else
			color = (argb_t)V_GetColorFromString(NULL, argv[1]);

		level.fadeto = color;
		RefreshPalette(GetDefaultPalette());
		NormalLight.maps = shaderef_t(&DefPal.maps, 0);
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
void BuildColoredLights (shademap_t *maps, int lr, int lg, int lb, int r, int g, int b)
{
	byte	*color;
	argb_t  *shade;

	// The default palette is assumed to contain the maps for white light.
	if (!maps)
		return;

	BuildLightRamp(*maps);

	// build normal (but colored) light mappings
	for (unsigned int l = 0; l < NUMCOLORMAPS; l++)
	{
		byte a = maps->ramp[l * 255 / NUMCOLORMAPS];

		// Write directly to the shademap for blending:
		argb_t* colors = maps->shademap + 256 * l;
		DoBlending(DefPal.basecolors, colors, r, g, b, a);

		// Build the colormap and shademap:
		color = maps->colormap + 256*l;
		shade = maps->shademap + 256*l;
		for (unsigned int c = 0; c < 256; c++)
		{
			shade[c].r = newgamma[colors[c].r * lr / 255];
			shade[c].g = newgamma[colors[c].g * lg / 255];
			shade[c].b = newgamma[colors[c].b * lb / 255];
			color[c] = BestColor(DefPal.basecolors, shade[c].r, shade[c].g, shade[c].b, 256);
		}
	}
}

dyncolormap_t *GetSpecialLights (int lr, int lg, int lb, int fr, int fg, int fb)
{
	argb_t color(lr, lg, lb);
	argb_t fade(fr, fg, fb);
	dyncolormap_t *colormap = &NormalLight;

	// Bah! Simple linear search because I want to get this done.
	while (colormap)
	{
		if (color == colormap->color && fade == colormap->fade)
			return colormap;
		else
			colormap = colormap->next;
	}

	// Not found. Create it.
	colormap = (dyncolormap_t *)Z_Malloc (sizeof(*colormap), PU_LEVEL, 0);
	shademap_t *maps = new shademap_t();
	maps->colormap = (byte *)Z_Malloc (NUMCOLORMAPS*256*sizeof(byte)+3+255, PU_LEVEL, 0);
	maps->colormap = (byte *)(((ptrdiff_t)maps->colormap + 255) & ~0xff);
	maps->shademap = (argb_t *)Z_Malloc (NUMCOLORMAPS*256*sizeof(argb_t)+3+255, PU_LEVEL, 0);
	maps->shademap = (argb_t *)(((ptrdiff_t)maps->shademap + 255) & ~0xff);

	colormap->maps = shaderef_t(maps, 0);
	colormap->color = color;
	colormap->fade = fade;
	colormap->next = NormalLight.next;
	NormalLight.next = colormap;

	BuildColoredLights (maps, lr, lg, lb, fr, fg, fb);

	return colormap;
}

BEGIN_COMMAND (testcolor)
{
	if (argc < 2)
	{
		Printf (PRINT_HIGH, "testcolor <color>\n");
	}
	else
	{
		std::string colorstring = V_GetColorStringByName(argv[1]);
		argb_t color;

		if (!colorstring.empty())
			color = (argb_t)V_GetColorFromString(NULL, colorstring.c_str());
		else
			color = (argb_t)V_GetColorFromString(NULL, argv[1]);

		BuildColoredLights((shademap_t*)NormalLight.maps.map(),
				color.r, color.g, color.b,
				level.fadeto.r, level.fadeto.g, level.fadeto.b);
	}
}
END_COMMAND (testcolor)

//
// V_DoPaletteEffects
//
// Handles changing the palette or the BlendR/G/B/A globals based on damage
// the player has taken, any power-ups, or environment such as deep water.
//
void V_DoPaletteEffects()
{
	IWindowSurface* primary_surface = I_GetPrimarySurface();

	player_t* plyr = &displayplayer();

	if (primary_surface->getBitsPerPixel() == 8)
	{
		int palette_num = 0;

		float red_count = (float)plyr->damagecount;
		if (!multiplayer || sv_allowredscreen)
			red_count *= r_painintensity;

		// slowly fade the berzerk out
		if (plyr->powers[pw_strength])
			red_count = MAX(red_count, 12.0f - float(plyr->powers[pw_strength] >> 6));

		if (red_count > 0.0f)
		{
			palette_num = ((int)red_count + 7) >> 3;

			if (gamemode == retail_chex)
				palette_num = RADIATIONPAL;
			else
			{
				if (palette_num >= NUMREDPALS)
					palette_num = NUMREDPALS - 1;

				palette_num += STARTREDPALS;

				if (palette_num < 0)
					palette_num = 0;
			}
		}
		else if (plyr->bonuscount)
		{
			palette_num = (plyr->bonuscount+7)>>3;

			if (palette_num >= NUMBONUSPALS)
				palette_num = NUMBONUSPALS - 1;

			palette_num += STARTBONUSPALS;
		}
		else if (plyr->powers[pw_ironfeet] > 4*32 || plyr->powers[pw_ironfeet] & 8)
			palette_num = RADIATIONPAL;

		if (palette_num != current_palette_num)
		{
			current_palette_num = palette_num;
			const byte* data = (byte*)W_CacheLumpNum(lu_palette, PU_CACHE) + palette_num * 768;
			V_SetPalette(data);
		}
	}
	else
	{
		float blend[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

		V_AddBlend(BaseBlendR / 255.0f, BaseBlendG / 255.0f, BaseBlendB / 255.0f, BaseBlendA, blend);
		V_AddBlend(plyr->BlendR, plyr->BlendG, plyr->BlendB, plyr->BlendA, blend);

		// red tint for pain / berzerk power
		if (plyr->damagecount || plyr->powers[pw_strength])
		{
			float red_amount = (float)plyr->damagecount;
			if (!multiplayer || sv_allowredscreen)
				red_amount *= r_painintensity;

			// slowly fade the berzerk out
			if (plyr->powers[pw_strength])
				red_amount = MAX(red_amount, 12.0f - float(plyr->powers[pw_strength]) / 64.0f);

			if (red_amount > 0.0f)
			{
				red_amount = MIN(red_amount, 56.0f);
				float alpha = (red_amount + 8.0f) / 72.0f;

				static const float red = 255.0f / 255.0f;
				static const float green = 0.0f;
				static const float blue = 0.0f;
				V_AddBlend(red, green, blue, alpha, blend);
			}
		}

		// yellow tint for item pickup
		if (plyr->bonuscount)
		{
			float bonus_amount = (float)plyr->bonuscount;
			if (bonus_amount > 0.0f)
			{
				bonus_amount = MIN(bonus_amount, 24.0f);
				float alpha = (bonus_amount + 8.0f) / 64.0f;				

				static const float red = 215.0f / 255.0f;
				static const float green = 186.0f / 255.0f;
				static const float blue = 69.0f / 255.0f;
				V_AddBlend(red, green, blue, alpha, blend);
			}
		}

		// green tint for radiation suit
		if (plyr->powers[pw_ironfeet] > 4*32 || plyr->powers[pw_ironfeet] & 8)
		{
			static const float alpha = 1.0f / 8.0f;
			static const float red = 0.0f;
			static const float green = 255.0f / 255.0f;
			static const float blue = 0.0f;
			V_AddBlend(red, green, blue, alpha, blend);
		}

		memcpy(current_blend, blend, sizeof(blend));

		V_SetBlend ((int)(blend[0] * 255.0f), (int)(blend[1] * 255.0f),
					(int)(blend[2] * 255.0f), (int)(blend[3] * 256.0f));
	}
}


//
// V_ResetPalette
//
// Resets the palette back to the default palette.
//
void V_ResetPalette()
{
	if (I_VideoInitialized())
	{
		const palette_t* palette = GetDefaultPalette();
		I_SetPalette(palette->colors);
	}
}



VERSION_CONTROL (v_palette_cpp, "$Id$")

