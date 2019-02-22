// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
#include <cassert>

#include "doomstat.h"
#include "i_system.h"
#include "v_video.h"
#include "m_alloc.h"
#include "r_main.h"		// For lighting constants
#include "w_wad.h"
#include "z_zone.h"
#include "i_video.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "g_level.h"
#include "st_stuff.h"

#include "v_palette.h"


static palette_t default_palette;
static palette_t game_palette;

//
// V_GetDefaultPalette
//
// Returns a pointer to the default palette for the video subsystem. The
// palette returned should be the default palette defined in the PLAYPAL lump with the
// user's gamma correction setting applied.
//
const palette_t* V_GetDefaultPalette()
{
	return &default_palette;
}


//
// V_GetGamePalette
//
// Returns a pointer to the game palette that is used in 8bpp video modes. The
// palette returned is chosen from the palettes in the PLAYPAL lump based on
// the displayplayer's current game status (eg, recently was damaged, wearing
// radiation suite, etc.).
//
const palette_t* V_GetGamePalette()
{
	return &game_palette;
}


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

dyncolormap_t NormalLight;

static char palette_lumpname[9];

static int current_palette_num;

translationref_t::translationref_t() :
	m_table(NULL), m_player_id(-1)
{
}

translationref_t::translationref_t(const translationref_t &other) :
	m_table(other.m_table), m_player_id(other.m_player_id)
{
}

translationref_t::translationref_t(const byte *table) :
	m_table(table), m_player_id(-1)
{
}

translationref_t::translationref_t(const byte *table, const int player_id) :
	m_table(table), m_player_id(player_id)
{
}

shaderef_t::shaderef_t() :
	m_colors(NULL), m_mapnum(-1), m_colormap(NULL), m_shademap(NULL)
{
}

shaderef_t::shaderef_t(const shaderef_t &other) :
	m_colors(other.m_colors), m_mapnum(other.m_mapnum),
	m_colormap(other.m_colormap), m_shademap(other.m_shademap), m_dyncolormap(other.m_dyncolormap)
{
}

shaderef_t::shaderef_t(const shademap_t* const colors, const int mapnum) :
	m_colors(colors), m_mapnum(mapnum)
{
	#if ODAMEX_DEBUG
	// NOTE(jsd): Arbitrary value picked here because we don't record the max number of colormaps for dynamic ones... or do we?
	if (m_mapnum >= 8192)
	{
		char tmp[100];
		sprintf(tmp, "32bpp: shaderef_t::shaderef_t() called with mapnum = %d, which looks too large", m_mapnum);
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

		if (m_colors != &(V_GetDefaultPalette()->maps))
		{
			// Find the dynamic colormap by the `m_colors` pointer:
			dyncolormap_t* colormap = &NormalLight;

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


// ----------------------------------------------------------------------------
//
// Gamma Correction
//
// ----------------------------------------------------------------------------

byte gammatable[256];

enum {
	GAMMA_DOOM = 0,
	GAMMA_ZDOOM = 1
};


EXTERN_CVAR(vid_gammatype)
EXTERN_CVAR(gammalevel)


//
// GammaStrategy
//
// Encapsulate the differences of the Doom and ZDoom gamma types with
// a strategy pattern. Provides a common interface for generation of gamma
// tables.
//
class GammaStrategy
{
public:
	virtual float min() const = 0;
	virtual float max() const = 0;
	virtual float increment(float level) const = 0;
	virtual void generateGammaTable(byte* table, float level) const = 0;
};

class DoomGammaStrategy : public GammaStrategy
{
public:
	float min() const
	{
		return 0.0f;
	}

	float max() const
	{
		return 7.0f;
	}

	float increment(float level) const
	{
		level += 1.0f;
		if (level > max())
			level = min();
		return level;
	}

	void generateGammaTable(byte* table, float level) const
	{
		// [SL] Use vanilla Doom's gamma table
		//
		// This was derived from the original Doom gammatable after some
		// trial and error and several beers.  The +0.5 is used to round
		// while the 255/256 is to scale to ensure 255 isn't exceeded.
		// This generates a 1:1 match with the original gammatable but also
		// allows for intermediate values.

		const double basefac = pow(2.0, (double)level) * (255.0/256.0);
		const double exp = 1.0 - 0.125 * level;

		for (int i = 0; i < 256; i++)
			table[i] = (byte)(0.5 + basefac * pow(double(i) + 1.0, exp));
	}
};

class ZDoomGammaStrategy : public GammaStrategy
{
public:
	float min() const
	{
		return 0.5f;
	}

	float max() const
	{
		return 3.0f;
	}

	float increment(float level) const
	{
		level += 0.1f;
		if (level > max())
			level = min();
		return level;
	}

	void generateGammaTable(byte* table, float level) const
	{
		// [SL] Use ZDoom 1.22 gamma correction

		// [RH] I found this formula on the web at
		// http://panda.mostang.com/sane/sane-gamma.html

		double invgamma = 1.0 / level;

		for (int i = 0; i < 256; i++)
			table[i] = (byte)(255.0 * pow(double(i) / 255.0, invgamma));
	}
};

static DoomGammaStrategy doomgammastrat;
static ZDoomGammaStrategy zdoomgammastrat;
GammaStrategy* gammastrat = &doomgammastrat;

float V_GetMinimumGammaLevel()
{
	return gammastrat->min();
}

float V_GetMaximumGammaLevel()
{
	return gammastrat->max();
}

void V_IncrementGammaLevel()
{
	float level(gammalevel.value());
	gammalevel.Set(gammastrat->increment(level));
}


//
// V_GammaAdjustPalette
//
static void V_GammaAdjustPalette(palette_t* palette)
{
	const argb_t* from = palette->basecolors;
	argb_t* to = palette->colors;

	for (int i = 0; i < 256; i++)
		*to++ = V_GammaCorrect(*from++);
}


//
// V_UpdateGammaLevel
//
// Calls the concrete GammaStrategy generateGammaTable function to populate
// the gammatable array. The palette is also gamma-corrected.
//
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

		gammastrat->generateGammaTable(gammatable, level);
		V_GammaAdjustPalette(&default_palette);

		V_RestoreScreenPalette();

		if (I_GetPrimarySurface()->getBitsPerPixel() == 32)
			V_RefreshColormaps();
	}
}


//
// vid_gammatype
//
// Changes gammastrat to a new concrete GammaStrategy and forces the palette
// to be gamma-corrected.
//
CVAR_FUNC_IMPL(vid_gammatype)
{
	if (vid_gammatype == GAMMA_ZDOOM)
		gammastrat = &zdoomgammastrat;
	else
		gammastrat = &doomgammastrat;

	gammalevel.Set(gammalevel);
}


//
// gammalevel
//
// Specifies the gamma correction level. The level is clamped to the concrete
// GammaStrategy's minimum and maximum values prior to updating gammatable by
// calling V_UpdateGammaLevel.
//
CVAR_FUNC_IMPL(gammalevel)
{
	float sanitized_var = clamp(var.value(), gammastrat->min(), gammastrat->max());
	if (var == sanitized_var)
		V_UpdateGammaLevel(var);
	else
		var.Set(sanitized_var);
}


//
// bumpgamma
//
// Increments gammalevel by a value controlled by the concrete GammaStrategy.
//
BEGIN_COMMAND(bumpgamma)
{
	V_IncrementGammaLevel();

	if (gammalevel.value() == 0.0f)
	    Printf (PRINT_HIGH, "Gamma correction off\n");
	else
	    Printf (PRINT_HIGH, "Gamma correction level %g\n", gammalevel.value());
}
END_COMMAND(bumpgamma)


// [Russell] - Restore original screen palette from current gamma level
void V_RestoreScreenPalette()
{
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
		V_ForceBlend(blend_color);
}


//
// V_BestColor
//
// (borrowed from Quake2 source: utils3/qdata/images.c)
// [SL] Also nearly identical to BestColor in dcolors.c in Doom utilites
//
palindex_t V_BestColor(const argb_t* palette_colors, int r, int g, int b)
{
	int bestdistortion = MAXINT;
	int bestcolor = 0;		/// let any color go to 0 as a last resort

	for (int i = 0; i < 256; i++)
	{
		argb_t color(palette_colors[i]);

		int dr = r - color.getr();
		int dg = g - color.getg();
		int db = b - color.getb();
		int distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (distortion == 0)
				return i;		// perfect match

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return bestcolor;
}

palindex_t V_BestColor(const argb_t* palette_colors, argb_t color)
{
	return V_BestColor(palette_colors, color.getr(), color.getg(), color.getb());
}


//
// V_ClosestColors
//
// Sets color1 and color2 to the palette indicies of the pair of colors that
// are the closest amongst the colors of the given palette. This is an N^2
// algorithm so use sparingly.
//
void V_ClosestColors(const argb_t* palette_colors, palindex_t& color1, palindex_t& color2)
{
	int bestdistortion = MAXINT;

	color1 = color2 = 0;	// go to color 0 as a last resort

	for (int x = 0; x < 256; x++)
	{
		for (int y = 0; y < 256 - x; y++)
		{
			// don't compare a color with itself
			if (x == y)
				continue;

			int dr = (int)palette_colors[y].getr() - (int)palette_colors[x].getr();
			int dg = (int)palette_colors[y].getg() - (int)palette_colors[x].getg();
			int db = (int)palette_colors[y].getb() - (int)palette_colors[x].getb();
			int distortion = dr*dr + dg*dg + db*db;
			if (distortion < bestdistortion)
			{
				color1 = x, color2 = y;
				bestdistortion = distortion;
				if (bestdistortion == 0)
					return;				// perfect match
			}
		}
	}
}


//
// V_GetColorStringByName
//
// Returns a string with 6 hexadecimal digits suitable for use with
// V_GetColorFromString. A given colorname is looked up in the X11R6RGB lump
// and its value is returned.
//
static std::string V_GetColorStringByName(const std::string& name)
{
	/* Note: The X11R6RGB lump used by this function *MUST* end
	 * with a NULL byte. This is so that COM_Parse is able to
	 * detect the end of the lump.
	 */
	char *rgbNames, *data, descr[5*3];
	int c[3], step;

	if (!(rgbNames = (char*)wads.CacheLumpName("X11R6RGB", PU_CACHE)))
	{
		Printf(PRINT_HIGH, "X11R6RGB lump not found\n");
		return "";
	}

	// skip past the header line
	data = strchr(rgbNames, '\n');
	step = 0;

	while ( (data = COM_Parse (data)) )
	{
		if (step < 3)
		{
			c[step++] = atoi (com_token);
		}
		else
		{
			step = 0;
			if (*data >= ' ')		// In case this name contains a space...
			{
				char *newchar = com_token + strlen(com_token);

				while (*data >= ' ')
					*newchar++ = *data++;
				*newchar = 0;
			}

			if (!stricmp(com_token, name.c_str()))
			{
				sprintf(descr, "%04x %04x %04x",
						 (c[0] << 8) | c[0],
						 (c[1] << 8) | c[1],
						 (c[2] << 8) | c[2]);
				return descr;
			}
		}
	}
	return "";
}


//
// V_GetColorFromString
//
// Parses a string of 6 hexadecimal digits representing an RGB triplet
// and converts it into an argb_t value. It will also accept the name of a
// color, as defined in the X11R6RGB lump, using V_GetColorStringByName
// to look up the RGB triplet value.
//
argb_t V_GetColorFromString(const std::string& input_string)
{
	// first check if input_string is the name of a color
	const std::string color_name_string = V_GetColorStringByName(input_string);

	// if not a valid color name, try to parse the color channel values
	const char* str = color_name_string.empty() == false ?
					color_name_string.c_str() :
					input_string.c_str();

	int c[3], i, p;
	char val[5];
	const char *s, *g;

	val[4] = 0;
	for (s = str, i = 0; i < 3; i++)
	{
		c[i] = 0;

		while ((*s <= ' ') && (*s != 0))
			s++;

		if (*s)
		{
			p = 0;

			while (*s > ' ')
			{
				if (p < 4)
					val[p++] = *s;

				s++;
			}

			g = val;

			while (p < 4)
				val[p++] = *g++;

			c[i] = ParseHex(val);
		}
	}

	return argb_t(c[0] >> 8, c[1] >> 8, c[2] >> 8);
}


/****************************/
/* Palette management stuff */
/****************************/

//
// V_InitPalette
//
// Initializes the default palette, loading the raw palette lump resource.
//
void V_InitPalette(const char* lumpname)
{
	strncpy(palette_lumpname, lumpname, 8);
	palette_lumpname[8] = '\0';

	int lumpnum = wads.GetNumForName(palette_lumpname);
	if (lumpnum < 0)
		I_FatalError("Could not initialize %s palette", palette_lumpname);

	current_palette_num = -1;

	if (default_palette.maps.colormap)
		delete [] default_palette.maps.colormap;
	if (default_palette.maps.shademap)
		delete [] default_palette.maps.shademap;

	default_palette.maps.colormap = new palindex_t[(NUMCOLORMAPS + 1) * 256];
	default_palette.maps.shademap = new argb_t[(NUMCOLORMAPS + 1) * 256];

	const byte* data = (byte*)wads.CacheLumpNum(lumpnum, PU_CACHE);

	for (int i = 0; i < 256; i++, data += 3)
		default_palette.basecolors[i] = argb_t(255, data[0], data[1], data[2]);

	V_GammaAdjustPalette(&default_palette);

	V_ForceBlend(argb_t(0, 255, 255, 255));

	V_RefreshColormaps();

	V_ResetPalette();

	assert(default_palette.maps.colormap != NULL);
	assert(default_palette.maps.shademap != NULL);
	V_Palette = shaderef_t(&default_palette.maps, 0);

	game_palette = default_palette;
}


// This is based (loosely) on the ColorShiftPalette()
// function from the dcolors.c file in the Doom utilities.
static void V_DoBlending(argb_t* dest, const argb_t* source, argb_t color)
{
	if (color.geta() == 0)
	{
		if (source != dest)
			memcpy(dest, source, 256 * sizeof(argb_t));
	}
	else
	{
		for (int i = 0; i < 256; i++, source++, dest++)
		{
			int fromr = source->getr();
			int fromg = source->getg();
			int fromb = source->getb();

			int toa = color.geta();
			int tor = color.getr();
			int tog = color.getg();
			int tob = color.getb();

			int dr = tor - fromr;
			int dg = tog - fromg;
			int db = tob - fromb;

			argb_t newcolor(
					source->geta(),
					fromr + ((dr * toa) >> 8),
					fromg + ((dg * toa) >> 8),
					fromb + ((db * toa) >> 8));

			*dest = newcolor;
		}
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

void BuildDefaultColorAndShademap(const palette_t* pal, shademap_t& maps)
{
	BuildLightRamp(maps);

	// [SL] Modified algorithm from RF_BuildLights in dcolors.c
	// from Doom Utilities. Now accomodates fading to non-black colors.

	const argb_t* palette = pal->basecolors;
	argb_t fadecolor(level.fadeto_color[0], level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);
	
	palindex_t* colormap = maps.colormap;
	argb_t* shademap = maps.shademap;

	for (int i = 0; i < NUMCOLORMAPS; i++, colormap += 256, shademap += 256)
	{
		for (int c = 0; c < 256; c++)
		{
			unsigned int r = (palette[c].getr() * (NUMCOLORMAPS - i) + fadecolor.getr() * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int g = (palette[c].getg() * (NUMCOLORMAPS - i) + fadecolor.getg() * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int b = (palette[c].getb() * (NUMCOLORMAPS - i) + fadecolor.getb() * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;

			argb_t color(255, r, g, b);
			colormap[c] = V_BestColor(palette, color);
			shademap[c] = V_GammaCorrect(color);
		}
	}

	// build special maps (e.g. invulnerability)
	for (int c = 0; c < 256; c++)
	{
		int grayint = (int)(255.0f * clamp(1.0f -
						(palette[c].getr() * 0.00116796875f +
						 palette[c].getg() * 0.00229296875f +
			 			 palette[c].getb() * 0.0005625f), 0.0f, 1.0f));

		argb_t color(255, grayint, grayint, grayint); 
		colormap[c] = V_BestColor(palette, color);
		shademap[c] = V_GammaCorrect(color); 
	}
}

void BuildDefaultShademap(const palette_t* pal, shademap_t& maps)
{
	BuildLightRamp(maps);

	// [SL] Modified algorithm from RF_BuildLights in dcolors.c
	// from Doom Utilities. Now accomodates fading to non-black colors.

	const argb_t* palette = pal->basecolors;
	argb_t fadecolor(level.fadeto_color[0], level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);
	
	argb_t* shademap = maps.shademap;

	for (int i = 0; i < NUMCOLORMAPS; i++, shademap += 256)
	{
		for (int c = 0; c < 256; c++)
		{
			unsigned int r = (palette[c].getr() * (NUMCOLORMAPS - i) + fadecolor.getr() * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int g = (palette[c].getg() * (NUMCOLORMAPS - i) + fadecolor.getg() * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;
			unsigned int b = (palette[c].getb() * (NUMCOLORMAPS - i) + fadecolor.getb() * i
					+ NUMCOLORMAPS/2) / NUMCOLORMAPS;

			argb_t color(255, r, g, b);
			shademap[c] = V_GammaCorrect(color);
		}
	}

	// build special maps (e.g. invulnerability)
	for (int c = 0; c < 256; c++)
	{
		int grayint = (int)(255.0f * clamp(1.0f -
						(palette[c].getr() * 0.00116796875f +
						 palette[c].getg() * 0.00229296875f +
			 			 palette[c].getb() * 0.0005625f), 0.0f, 1.0f));

		argb_t color(255, grayint, grayint, grayint); 
		shademap[c] = V_GammaCorrect(color); 
	}
}


//
// V_RefreshColormaps
//
void V_RefreshColormaps()
{
	BuildDefaultColorAndShademap(&default_palette, default_palette.maps);

	NormalLight.maps = shaderef_t(&default_palette.maps, 0);
	NormalLight.color = argb_t(255, 255, 255, 255);
	NormalLight.fade = argb_t(level.fadeto_color[0], level.fadeto_color[1],
							level.fadeto_color[2], level.fadeto_color[3]);

	R_ReinitColormap();
}


//
// V_AddBlend
//
// Blends an ARGB color with an existing ARGB color blend.
//
// [RH] This is from Q2.
// [SL] Modified slightly to use fargb_t types.
//
static void V_AddBlend(fargb_t& blend, const fargb_t& newcolor)
{
	if (newcolor.geta() <= 0.0f)
		return;

	float a = blend.geta() + newcolor.geta() * (1.0f - blend.geta());
	float old_amount = blend.geta() / a;

	blend.seta(a);
	blend.setr(blend.getr() * old_amount + newcolor.getr() * (1.0f - old_amount));
	blend.setg(blend.getg() * old_amount + newcolor.getg() * (1.0f - old_amount));
	blend.setb(blend.getb() * old_amount + newcolor.getb() * (1.0f - old_amount));
}


//
// V_ForceBlend
//
// Normally, V_SetBlend does nothing if the new blend is the
// same as the old. This function will perform the blending
// even if the blend hasn't changed.
//
void V_ForceBlend(const argb_t color)
{
	blend_color = color;

	// blend the palette for 8-bit mode
	// shademap_t::shade takes care of blending
	// [SL] actually, an alpha overlay is drawn on top of the rendered screen
	// in R_RenderPlayerView
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		argb_t palette_colors[256];
		V_DoBlending(palette_colors, game_palette.basecolors, blend_color);

		for (int i = 0; i < 256; i++)
		{
			game_palette.basecolors[i] = palette_colors[i];
			game_palette.colors[i] = V_GammaCorrect(palette_colors[i]);
		}

		I_SetPalette(game_palette.colors);
	}
}


//
// V_SetBlend
//
// Sets the global blending color and blends the color with the default
// palette and passes the palette to the video hardware (in 8bpp mode).
//
//
void V_SetBlend(const argb_t color)
{
	// Don't do anything if the new blend is the same as the old
	if (blend_color.geta() == 0 && color.geta() == 0)
		return;

	if (blend_color.geta() == color.geta() &&
		blend_color.getr() == color.getr() &&
		blend_color.getg() == color.getg() &&
		blend_color.getb() == color.getb())
		return;

	V_ForceBlend(color);
}


BEGIN_COMMAND (testblend)
{
	if (argc < 3)
	{
		Printf (PRINT_HIGH, "testblend <color> <amount>\n");
	}
	else
	{
		argb_t color(V_GetColorFromString(argv[1]));

		int alpha = 255.0 * clamp((float)atof(argv[2]), 0.0f, 1.0f);
		R_SetSectorBlend(argb_t(alpha, color.getr(), color.getg(), color.getb()));
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
		argb_t color(V_GetColorFromString(argv[1]));

		level.fadeto_color[0] = color.geta();
		level.fadeto_color[1] = color.getr();
		level.fadeto_color[2] = color.getg();
		level.fadeto_color[3] = color.getb();

		V_RefreshColormaps();
		NormalLight.maps = shaderef_t(&V_GetDefaultPalette()->maps, 0);
	}
}
END_COMMAND (testfade)

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
fahsv_t V_RGBtoHSV(const fargb_t &color)
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
fargb_t V_HSVtoRGB(const fahsv_t &color)
{
	float a = color.geta(), h = color.geth(), s = color.gets(), v = color.getv();

	if (s == 0.0f)						// achromatic (grey)
		return fargb_t(a, v, v, v);

	float f = (h / 60.0f) - floor(h / 60.0f);
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


/****** Colored Lighting Stuffs (Sorry, 8-bit only) ******/

// Builds NUMCOLORMAPS colormaps lit with the specified color
void BuildColoredLights(shademap_t* maps, int lr, int lg, int lb, int r, int g, int b)
{
	// The default palette is assumed to contain the maps for white light.
	if (!maps)
		return;

	BuildLightRamp(*maps);

	const argb_t* palette_colors = V_GetDefaultPalette()->basecolors;

	// build normal (but colored) light mappings
	for (unsigned int l = 0; l < NUMCOLORMAPS; l++)
	{
		byte a = maps->ramp[l * 255 / NUMCOLORMAPS];

		// Write directly to the shademap for blending:
		argb_t* colors = maps->shademap + 256 * l;
		V_DoBlending(colors, palette_colors, argb_t(a, r, g, b));

		// Build the colormap and shademap:
		palindex_t* colormap = maps->colormap + 256*l;
		argb_t* shademap = maps->shademap + 256*l;
		for (unsigned int c = 0; c < 256; c++)
		{
			argb_t color(255, colors[c].getr() * lr / 255,
						colors[c].getg() * lg / 255, colors[c].getb() * lb / 255);
			shademap[c] = V_GammaCorrect(color);
			colormap[c] = V_BestColor(palette_colors, color);
		}
	}
}

dyncolormap_t* GetSpecialLights(int lr, int lg, int lb, int fr, int fg, int fb)
{
	argb_t color(255, lr, lg, lb);
	argb_t fade(255, fr, fg, fb);
	dyncolormap_t* colormap = &NormalLight;

	// Bah! Simple linear search because I want to get this done.
	while (colormap)
	{
		if (color.getr() == colormap->color.getr() &&
			color.getg() == colormap->color.getg() &&
			color.getb() == colormap->color.getb() &&
			fade.getr() == colormap->fade.getr() &&
			fade.getg() == colormap->fade.getg() &&
			fade.getb() == colormap->fade.getb())
			return colormap;
		else
			colormap = colormap->next;
	}

	// Not found. Create it.
	colormap = (dyncolormap_t*)Z_Malloc(sizeof(*colormap), PU_LEVEL, 0);

	shademap_t* maps = new shademap_t();
	maps->colormap = (palindex_t*)Z_Malloc(NUMCOLORMAPS * 256 * sizeof(palindex_t), PU_LEVEL, 0);
	maps->shademap = (argb_t*)Z_Malloc(NUMCOLORMAPS * 256 * sizeof(argb_t), PU_LEVEL, 0);

	colormap->maps = shaderef_t(maps, 0);
	colormap->color = color;
	colormap->fade = fade;
	colormap->next = NormalLight.next;
	NormalLight.next = colormap;

	BuildColoredLights(maps, lr, lg, lb, fr, fg, fb);

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
		argb_t color(V_GetColorFromString(argv[1]));

		BuildColoredLights((shademap_t*)NormalLight.maps.map(),
				color.getr(), color.getg(), color.getb(),
				level.fadeto_color[1], level.fadeto_color[2], level.fadeto_color[3]);
	}
}
END_COMMAND (testcolor)

//
// V_DoPaletteEffects
//
// Handles changing the palette or the blend_color global based on damage
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
			// [SL] Load palette_num from disk and setup game_palette
			current_palette_num = palette_num;
			const byte* data = (byte*)wads.CacheLumpName(palette_lumpname, PU_CACHE) + palette_num * 768;

			for (int i = 0; i < 256; i++, data += 3)
			{
				game_palette.basecolors[i] = argb_t(255, data[0], data[1], data[2]);
				game_palette.colors[i] = V_GammaCorrect(game_palette.basecolors[i]);
			}

			// Sets the video adapter's palette to the given 768 byte palette lump.
			I_SetPalette(game_palette.colors);
		}
	}
	else
	{
		fargb_t blend(0.0f, 0.0f, 0.0f, 0.0f);

		V_AddBlend(blend, R_GetSectorBlend());
		V_AddBlend(blend, plyr->blend_color);

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
				V_AddBlend(blend, fargb_t(alpha, red, green, blue));
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
				V_AddBlend(blend, fargb_t(alpha, red, green, blue));
			}
		}

		// green tint for radiation suit
		if (plyr->powers[pw_ironfeet] > 4*32 || plyr->powers[pw_ironfeet] & 8)
		{
			static const float alpha = 1.0f / 8.0f;
			static const float red = 0.0f;
			static const float green = 255.0f / 255.0f;
			static const float blue = 0.0f;
			V_AddBlend(blend, fargb_t(alpha, red, green, blue));
		}

		V_SetBlend(blend);
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
		game_palette = default_palette;
		I_SetPalette(game_palette.colors);
		fargb_t blend(0.0f, 0.0f, 0.0f, 0.0f);
		V_SetBlend(blend);
	}
}



VERSION_CONTROL (v_palette_cpp, "$Id$")

