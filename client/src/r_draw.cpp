// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		The actual span/column drawing functions.
//		Here find the main potential for optimization,
//		 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <assert.h>
#include <cmath>
#include <algorithm>

#include "i_sdl.h"
#include "r_intrin.h"

#include "c_dispatch.h"
#include "m_alloc.h"
#include "z_zone.h"
#include "r_local.h"
#include "r_plane.h"
#include "i_video.h"
#include "v_video.h"

#include "gi.h"
#include "v_text.h"
#include "resources/res_texture.h"
#include "st_stuff.h"
#include "g_gametype.h"

#undef RANGECHECK

EXTERN_CVAR(r_forceenemycolor)
EXTERN_CVAR(r_forceteamcolor)

// status bar height at bottom of screen
// [RH] status bar position at bottom of screen
extern	int		ST_Y;

extern IWindowSurface* screenblocks_surface;
extern IWindowSurface* scaled_screenblocks_surface;

//
// All drawing to the view buffer is accomplished in this file.
// The other refresh files only know about ccordinates,
//	not the architecture of the frame buffer.
// Conveniently, the frame buffer is a linear one,
//	and we need only the base address,
//	and the total size == width*height*depth/8.,
//

extern "C" {
drawcolumn_t dcol;
drawspan_t dspan;
drawplanecol_t dpcol;
drawplanegroup_t dpgroup;
}

byte*			viewimage;

extern "C" {
int 			viewwidth;
int 			viewheight;
}

int 			scaledviewwidth;
int 			viewwindowx;
int 			viewwindowy;

// [RH] Pointers to the different column drawers.
//		These get changed depending on the current
//		screen depth.
void (*R_DrawColumn)(void);
void (*R_DrawFuzzColumn)(void);
void (*R_DrawTranslucentColumn)(void);
void (*R_DrawTranslatedColumn)(void);
void (*R_DrawTlatedLucentColumn)(void);
void (*R_DrawSkyForegroundColumn)(void);
void (*R_DrawSpan)(void);
void (*R_DrawTranslucentSpan)(void);
void (*R_DrawSlopeSpan)(void);
void (*R_DrawTranslucentSlopeSpan)(void);
void (*R_FillColumn)(void);
void (*R_FillSpan)(void);
void (*R_FillTranslucentSpan)(void);

// Possibly vectorized functions:
void (*R_DrawLevelColumn)(void);
void (*R_DrawTranslucentLevelColumn)(void);
void (*R_DrawLevelGroup)(void);

void (*R_DrawLevelGroupD)(void);
void (*r_dimpatchD)(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h);

// ============================================================================
//
// Fuzz Table
//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels from adjacent ones to left and right.
// Used with an all black colormap, this could create the SHADOW effect,
// i.e. spectres and invisible players.
//
// ============================================================================

class FuzzTable
{
public:
	FuzzTable() : pos(0) { }

	forceinline void incrementRow()
	{
		pos = (pos + 1) % FuzzTable::size;
	}

	forceinline void incrementColumn()
	{
		pos = (pos + 3) % FuzzTable::size;
	}

	forceinline int getValue() const
	{
		// [SL] quickly convert the table value (-1 or 1) into (-pitch or pitch).
		// [AM] Replaced with a multiply that returns accurate results.  Hopefully
		//      we can find a way to improve upon an imul someday.
		const int pitch = dcol.pitch_in_pixels;
		const int value = table[pos];
		return pitch * value;
	}

private:
	static constexpr size_t size = 64;
	static const int table[FuzzTable::size];
	int pos;
};

constexpr int FuzzTable::table[FuzzTable::size] = {
		1,-1, 1,-1, 1, 1,-1, 1,
		1,-1, 1, 1, 1,-1, 1, 1,
		1,-1,-1,-1,-1, 1,-1,-1,
		1, 1, 1, 1,-1, 1,-1, 1,
		1,-1,-1, 1, 1,-1,-1,-1,
	   -1, 1, 1, 1, 1,-1, 1, 1,
	   -1, 1, 1, 1,-1, 1, 1, 1,
	   -1, 1, 1,-1, 1, 1,-1, 1 };


static FuzzTable fuzztable;

// ============================================================================
//
// Translucency Table
//
// ============================================================================

/*
[RH] This translucency algorithm is based on DOSDoom 0.65's, but uses
a 32k RGB table instead of an 8k one. At least on my machine, it's
slightly faster (probably because it uses only one shift instead of
two), and it looks considerably less green at the ends of the
translucency range. The extra size doesn't appear to be an issue.

The following note is from DOSDoom 0.65:

New translucency algorithm, by Erik Sandberg:

Basically, we compute the red, green and blue values for each pixel, and
then use a RGB table to check which one of the palette colours that best
represents those RGB values. The RGB table is 8k big, with 4 R-bits,
5 G-bits and 4 B-bits. A 4k table gives a bit too bad precision, and a 32k
table takes up more memory and results in more cache misses, so an 8k
table seemed to be quite ultimate.

The computation of the RGB for each pixel is accelerated by using two
1k tables for each translucency level.
The xth element of one of these tables contains the r, g and b values for
the colour x, weighted for the current translucency level (for example,
the weighted rgb values for background colour at 75% translucency are 1/4
of the original rgb values). The rgb values are stored as three
low-precision fixed point values, packed into one long per colour:
Bit 0-4:   Frac part of blue  (5 bits)
Bit 5-8:   Int  part of blue  (4 bits)
Bit 9-13:  Frac part of red   (5 bits)
Bit 14-17: Int  part of red   (4 bits)
Bit 18-22: Frac part of green (5 bits)
Bit 23-27: Int  part of green (5 bits)
Bit 28-31: All zeros          (4 bits)

The point of this format is that the two colours now can be added, and
then be converted to a RGB table index very easily: First, we just set
all the frac bits and the four upper zero bits to 1. It's now possible
to get the RGB table index by anding the current value >> 5 with the
current value >> 19. When asm-optimised, this should be the fastest
algorithm that uses RGB tables.
*/


// ============================================================================
//
// Indexed-color Translation Table
//
// Used to draw player sprites with the green colorramp mapped to others.
// Could be used with different translation tables, e.g. the lighter colored
// version of the BaronOfHell, the HellKnight, uses identical sprites, kinda
// brightened up.
//
// ============================================================================

argb_t translationRGB[MAXPLAYERS+1][16];
byte *Ranges;
struct alignas(256) TranslationTables {
	// 1 player setup menu + 255 players + 3 classic translations + 21 font translations
    byte data[256 * (1 + MAXPLAYERS + 3 + 21)];
};
static std::unique_ptr<TranslationTables> translationtablesmem = nullptr;

static void R_BuildFontTranslation(int color_num, argb_t start_color, argb_t end_color)
{
	static constexpr palindex_t chexstart_index = 0x70;
	static constexpr palindex_t chexend_index = 0x7F;
	static constexpr palindex_t hacxstart_index = 0xC3;
	static constexpr palindex_t hacxmid1_index = 0xCF;
	static constexpr palindex_t hacxmid2_index = 0xF0;
	static constexpr palindex_t hacxend_index = 0xF2;
	static constexpr palindex_t start_index = 0xB0;
	static constexpr palindex_t end_index = 0xBF;
	const int index_range = end_index - start_index + 1;

	palindex_t* dest = static_cast<palindex_t*>(Ranges) + color_num * 256;

	if (IsChexMission(gamemission))
	{
		for (int index = 0; index < chexstart_index; index++)
			dest[index] = index;
		for (int index = chexend_index + 1; index < 256; index++)
			dest[index] = index;
	}
	else if (gamemission == commercial_hacx)
	{
		for (int index = 0; index < hacxstart_index; index++)
			dest[index] = index;
		for (int index = hacxmid1_index + 1; index < hacxmid2_index; index++)
			dest[index] = index;
		for (int index = hacxend_index + 1; index < 256; index++)
			dest[index] = index;
	}
	else
	{
		for (int index = 0; index < start_index; index++)
			dest[index] = index;
		for (int index = end_index + 1; index < 256; index++)
			dest[index] = index;
	}

	int r_diff = end_color.getr() - start_color.getr();
	int g_diff = end_color.getg() - start_color.getg();
	int b_diff = end_color.getb() - start_color.getb();
	int hacxtrack;

	if (IsChexMission(gamemission))
	{
		for (palindex_t index = chexstart_index; index <= chexend_index; index++)
		{
			const int i = index - chexstart_index;

			const int r = start_color.getr() + i * r_diff / index_range;
			const int g = start_color.getg() + i * g_diff / index_range;
			const int b = start_color.getb() + i * b_diff / index_range;

			dest[index] = V_BestColor(V_GetDefaultPalette()->basecolors, r, g, b);
		}

		dest[0x2C] = dest[0x2D] = dest[0x2F] = dest[chexend_index];
	}
	else if (gamemission == commercial_hacx)
	{
		for (palindex_t index = hacxstart_index; index <= hacxend_index; index++)
		{
			if (index > hacxmid1_index && index < hacxmid2_index)
				index = hacxmid2_index;

			if (index <= hacxmid1_index)
				hacxtrack = hacxstart_index;
			else
				hacxtrack = hacxmid2_index - (hacxmid1_index - hacxstart_index + 1);
			int i = index - hacxtrack;

			int r = start_color.getr() + i * r_diff / index_range;
			int g = start_color.getg() + i * g_diff / index_range;
			int b = start_color.getb() + i * b_diff / index_range;

			dest[index] = V_BestColor(V_GetDefaultPalette()->basecolors, r, g, b);
		}

		dest[0x2C] = dest[0x2D] = dest[0x2F] = dest[hacxend_index];
	}
	else
	{
		for (palindex_t index = start_index; index <= end_index; index++)
		{
			int i = index - start_index;

			int r = start_color.getr() + i * r_diff / index_range;
			int g = start_color.getg() + i * g_diff / index_range;
			int b = start_color.getb() + i * b_diff / index_range;

			dest[index] = V_BestColor(V_GetDefaultPalette()->basecolors, r, g, b);
		}

		dest[0x2C] = dest[0x2D] = dest[0x2F] = dest[end_index];
	}
}

/**
 * @brief Apply a soft light filter using Pegtop's formula.
 *
 * @see https://en.wikipedia.org/wiki/Blend_modes#Soft_Light
 *
 * @param bot Bottom channel value.
 * @param top Top channel value.
 * @return Filtered value.
*/
static byte SoftLight(const byte bot, const byte top)
{
	const float a = bot / 255.f;
	const float b = top / 255.f;
	const float res = (1.f - 2.f * b) * pow(a, 2.f) + (2.f * b * a);
	return res * 255;
}

void R_RebuildPlayerGreenTintTables(int player)
{
	argb_t gtop(0x62, 0xff, 0x5c);
	for (size_t i = 0; i < ARRAY_LENGTH(::greentable[player]); i++)
	{
		argb_t bot = argb_t();
		if (i > 0x70 && i < 0x80)
		{
			bot = translationRGB[player][i - 0x70];
		}
		else
		{
			bot = V_GetDefaultPalette()->basecolors[i];
		}

		const argb_t mul(SoftLight(bot.getr(), gtop.getr()),
		                 SoftLight(bot.getg(), gtop.getg()),
		                 SoftLight(bot.getb(), gtop.getb()));

		::greentable[player][i] = V_BestColor(V_GetDefaultPalette()->basecolors, mul);
	}
}

void R_RebuildPlayerRedTintTables(int player)
{
	argb_t rtop(0xff, 0x28, 0x28);
	for (size_t i = 0; i < ARRAY_LENGTH(::redtable[player]); i++)
	{
		argb_t bot = argb_t();
		if (i > 0x70 && i < 0x80)
		{
			bot = translationRGB[player][i - 0x70];
		}
		else
		{
			bot = V_GetDefaultPalette()->basecolors[i];
		}

		const argb_t mul(SoftLight(bot.getr(), rtop.getr()),
		                 SoftLight(bot.getg(), rtop.getg()),
		                 SoftLight(bot.getb(), rtop.getb()));

		::redtable[player][i] = V_BestColor(V_GetDefaultPalette()->basecolors, mul);
	}
}

//
// R_InitTranslationTables
//
// Creates the translation tables to map
//	the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
void R_InitTranslationTables()
{
    R_FreeTranslationTables();

	// Boss translation is a yellow tint.
    const argb_t ytop(0xff, 0xff, 0x73);
	for (size_t i = 0; i < ARRAY_LENGTH(::bosstable); i++)
	{
		const argb_t bot = V_GetDefaultPalette()->basecolors[i];
		const argb_t mul(SoftLight(bot.getr(), ytop.getr()),
		                 SoftLight(bot.getg(), ytop.getg()),
		                 SoftLight(bot.getb(), ytop.getb()));

		::bosstable[i] = V_BestColor(V_GetDefaultPalette()->basecolors, mul);
	}

	// Friend translation is a pink tint.
	const argb_t ptop(0xff, 0x70, 0xB9);
	for (size_t i = 0; i < ARRAY_LENGTH(::friendtable); i++)
	{
		const argb_t bot = V_GetDefaultPalette()->basecolors[i];
		const argb_t mul(SoftLight(bot.getr(), ptop.getr()),
		                 SoftLight(bot.getg(), ptop.getg()),
		                 SoftLight(bot.getb(), ptop.getb()));

		::friendtable[i] = V_BestColor(V_GetDefaultPalette()->basecolors, mul);
	}

	translationtablesmem = std::make_unique<TranslationTables>();

	// [Toke - fix13]
	// denis - cleaned this up somewhat
	// [EB] alignment now ensured by alignas on the type
	translationtables = translationtablesmem->data;

	// [RH] Each player now gets their own translation table
	//		(soon to be palettes). These are set up during
	//		netgame arbitration and as-needed rather than
	//		in here. We do, however load some text translation
	//		tables from our PWAD (ala BOOM).

	for (int i = 0; i < 256; i++)
		translationtables[i] = i;

	// Set up default translationRGB tables:
	const palette_t* pal = V_GetDefaultPalette();
	for (int i = 0; i < MAXPLAYERS; ++i)
	{
		for (int j = 0x70; j < 0x80; ++j)
			translationRGB[i][j - 0x70] = pal->basecolors[j];
	}

	for (int i = 1; i < MAXPLAYERS+3; i++)
		memcpy (translationtables + i*256, translationtables, 256);

	// create translation tables for dehacked patches that expect them
	for (int i = 0x70; i < 0x80; i++) {
		// map green ramp to gray, brown, red
		translationtables[i+(MAXPLAYERS+0)*256] = 0x60 + (i&0xf);
		translationtables[i+(MAXPLAYERS+1)*256] = 0x40 + (i&0xf);
		translationtables[i+(MAXPLAYERS+2)*256] = 0x20 + (i&0xf);
	}

	// Create powerup tints by grabbing each players rgb translation table
	for (int i = 0; i < MAXPLAYERS + 1; i++)
	{
		R_RebuildPlayerGreenTintTables(i);
		R_RebuildPlayerRedTintTables(i);
	}

	Ranges = translationtables + (MAXPLAYERS+3)*256;

	R_BuildFontTranslation(CR_BRICK,	argb_t(0xFF, 0xB8, 0xB8), argb_t(0x47, 0x00, 0x00));
	R_BuildFontTranslation(CR_TAN,		argb_t(0xFF, 0xEB, 0xDF), argb_t(0x33, 0x2B, 0x13));
	R_BuildFontTranslation(CR_GRAY,		argb_t(0xEF, 0xEF, 0xEF), argb_t(0x27, 0x27, 0x27));
	R_BuildFontTranslation(CR_GREEN,	argb_t(0x77, 0xFF, 0x6F), argb_t(0x0B, 0x17, 0x07));
	R_BuildFontTranslation(CR_BROWN,	argb_t(0xBF, 0xA7, 0x8F), argb_t(0x53, 0x3F, 0x2F));
	R_BuildFontTranslation(CR_GOLD,		argb_t(0xFF, 0xFF, 0x73), argb_t(0x73, 0x2B, 0x00));
	R_BuildFontTranslation(CR_RED,		argb_t(0xFF, 0x00, 0x00), argb_t(0x3F, 0x00, 0x00));
	R_BuildFontTranslation(CR_BLUE,		argb_t(0x00, 0x00, 0xFF), argb_t(0x00, 0x00, 0x27));
	R_BuildFontTranslation(CR_ORANGE,	argb_t(0xFF, 0x80, 0x00), argb_t(0x20, 0x00, 0x00));
	R_BuildFontTranslation(CR_WHITE,	argb_t(0xFF, 0xFF, 0xFF), argb_t(0x24, 0x24, 0x24));
	R_BuildFontTranslation(CR_YELLOW,	argb_t(0xFC, 0xD0, 0x43), argb_t(0x27, 0x27, 0x27));
	R_BuildFontTranslation(CR_BLACK,	argb_t(0x50, 0x50, 0x50), argb_t(0x13, 0x13, 0x13));
	R_BuildFontTranslation(CR_LIGHTBLUE,argb_t(0xB4, 0xB4, 0xFF), argb_t(0x00, 0x00, 0x73));
	R_BuildFontTranslation(CR_CREAM,	argb_t(0xFF, 0xD7, 0xBB), argb_t(0xCF, 0x83, 0x53));
	R_BuildFontTranslation(CR_OLIVE,	argb_t(0x7B, 0x7F, 0x50), argb_t(0x2F, 0x37, 0x1F));
	R_BuildFontTranslation(CR_DARKGREEN,argb_t(0x43, 0x93, 0x37), argb_t(0x0B, 0x17, 0x07));
	R_BuildFontTranslation(CR_DARKRED,	argb_t(0xAF, 0x2B, 0x2B), argb_t(0x2B, 0x00, 0x00));
	R_BuildFontTranslation(CR_DARKBROWN,argb_t(0xA3, 0x6B, 0x3F), argb_t(0x1F, 0x17, 0x0B));
	R_BuildFontTranslation(CR_PURPLE,	argb_t(0xCF, 0x00, 0xCF), argb_t(0x23, 0x00, 0x23));
	R_BuildFontTranslation(CR_DARKGRAY,	argb_t(0x8B, 0x8B, 0x8B), argb_t(0x23, 0x23, 0x23));
	R_BuildFontTranslation(CR_CYAN,		argb_t(0x00, 0xF0, 0xF0), argb_t(0x00, 0x1F, 0x1F));
}

void R_FreeTranslationTables (void)
{
	translationtablesmem.reset();
}

// [Nes] Vanilla player translation table.
void R_BuildClassicPlayerTranslation (int player, int color)
{
	const palette_t* pal = V_GetDefaultPalette();

	const auto buildtranslation = [&](int base)
	{
		for (int i = 0x70; i < 0x80; i++)
		{
			translationtables[i + (player * 256)] = base + (i & 0xf);
			translationRGB[player][i - 0x70] = pal->basecolors[translationtables[i + (player * 256)]];
		}
	};

	switch (color)
	{
		case COLOR_GREEN:
			buildtranslation(0x70);
			break;
		case COLOR_INDIGO:
			buildtranslation(0x60);
			break;
		case COLOR_BROWN:
			buildtranslation(0x40);
			break;
		case COLOR_RED:
			buildtranslation(0x20);
			break;
		case COLOR_BLUE:
			buildtranslation(0xC0);
			break;
		case COLOR_ORANGE:
			buildtranslation(0xD0);
			break;
		default:
			break;
	}
}

void R_RebuildPlayerTintTables(int playerid)
{
	R_RebuildPlayerGreenTintTables(playerid);
	R_RebuildPlayerRedTintTables(playerid);
}

void R_CopyTranslationRGB (int fromplayer, int toplayer)
{
	for (int i = 0x70; i < 0x80; ++i)
	{
		translationRGB[toplayer][i - 0x70] = translationRGB[fromplayer][i - 0x70];
		translationtables[i+(toplayer * 256)] = translationtables[i+(fromplayer * 256)];
	}

	R_RebuildPlayerTintTables(toplayer);
}

/*
[Acts 19 quiz] Check if a specific color is being enforced on a sprite due to CVARs or gametype.
- G_IsTeamGame(): Team modes (CTF and Team DM/LMS) enforce blue, red, or green on all sprites, no exceptions.
- r_forceteamcolor 1 enforces a user-specified color on teammates in Coop/Horde, but not DM.
- r_forceenemycolor 1 enforces a user-specified color on rival players in DM, but not Coop/Horde.
- player != displayplayer_id: r_forceXXXXcolor is ignored on the display player.
- !consoleplayer().spectator: r_forceXXXXcolor is ignored on others from a spectating display player's POV.
- player != 0: r_forceXXXXcolor is ignored on the player preview in PLAYER SETUP.
*/
bool R_IsForcedColor(int player, bool forceteamcolor, bool forceenemycolor)
{
	return G_IsTeamGame() || (player != displayplayer_id && !consoleplayer().spectator &&
	       player != nullplayer_id && ((forceteamcolor && G_IsCoopGame()) || (forceenemycolor && G_IsFFAGame())));
}

CVAR_FUNC_IMPL(cl_customcolor)
{
	EXTERN_CVAR(cl_color)
	cl_color.ForceSet(var.cstring());
}

// [RH] Create a player's translation table based on
//		a given mid-range color.
void R_BuildPlayerTranslation(int player, argb_t dest_color, int colorpreset)
{
	if (!R_IsForcedColor(player, r_forceteamcolor, r_forceenemycolor) && colorpreset < NUMVANILLACOLOR)
	{
		return R_BuildClassicPlayerTranslation(player, colorpreset);
	}
	else
	{
		const palette_t* pal = V_GetDefaultPalette();
		byte* table = &translationtables[player * 256];

		const fahsv_t hsv_temp = V_RGBtoHSV(dest_color);
		const float h = hsv_temp.geth();
		float s = hsv_temp.gets(), v = hsv_temp.getv();

		s -= 0.23f;
		if (s < 0.0f)
			s = 0.0f;
		float sdelta = 0.014375f;

		v += 0.1f;
		if (v > 1.0f)
			v = 1.0f;
		float vdelta = -0.05882f;

		for (int i = 0x70; i < 0x80; i++)
		{
			const argb_t color(V_HSVtoRGB(fahsv_t(h, s, v)));

			// Set up RGB values for 32bpp translation:
			translationRGB[player][i - 0x70] = color;
			table[i] = V_BestColor(pal->basecolors, color);

			s += sdelta;
			if (s > 1.0f)
			{
				s = 1.0f;
				sdelta = 0.0f;
			}

			v += vdelta;
			if (v < 0.0f)
			{
				v = 0.0f;
				vdelta = 0.0f;
			}
		}
	}
}


// ============================================================================
//
// Spans
//
// With DOOM style restrictions on view orientation,
// the floors and ceilings consist of horizontal slices
// or spans with constant z depth.
// However, rotation around the world z axis is possible,
// thus this mapping, while simpler and faster than
// perspective correct texture mapping, has to traverse
// the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
// and the inner loop has to step in texture space u and v.
//
// ============================================================================


// ============================================================================
//
// Generic Drawers
//
// Templated versions of column and span drawing functions
//
// ============================================================================

//
// R_BlankColumn
//
// [SL] - Does nothing (obviously). Used when a column drawing function
// pointer should not draw anything.
//
void R_BlankColumn()
{
}

//
// R_BlankSpan
//
// [SL] - Does nothing (obviously). Used when a span drawing function
// pointer should not draw anything.
//
void R_BlankSpan()
{
}

//
// R_FillColumnGeneric
//
// Templated version of a function to fill a column with a solid color.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template<typename PIXEL_T, typename COLORFUNC>
static forceinline void R_FillColumnGeneric(PIXEL_T* dest, const drawcolumn_t& drawcolumn)
{
#ifdef RANGECHECK
	if (drawcolumn.x < 0 || drawcolumn.x >= viewwidth || drawcolumn.yl < 0 || drawcolumn.yh >= viewheight)
	{
		PrintFmt(PRINT_HIGH, "R_FillColumn: {} to {} at {}\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
		return;
	}
#endif

	int color = drawcolumn.color;
	const int pitch = 1;		// column-major: one row down is one pixel
	int count = drawcolumn.yh - drawcolumn.yl + 1;
	if (count <= 0)
		return;

	COLORFUNC colorfunc(drawcolumn);

	if (drawcolumn.masked && drawcolumn.source != NULL)
	{
		// preserve the silhouette of the masked texture
		const palindex_t* source = drawcolumn.source;
		const fixed_t fracstep = drawcolumn.iscale; 
		fixed_t frac = drawcolumn.texturefrac;
		do {
			palindex_t pixel = source[frac >> FRACBITS];
			if (pixel != 0)
				colorfunc(color, dest);
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}
	else
	{
		// non-masked so just fill it all in
		do {
			colorfunc(color, dest);
			dest += pitch;
		} while (--count);
	}
} 


//
// R_DrawColumnGeneric
//
// A column is a vertical slice/span from a wall texture that,
// given the DOOM style restrictions on the view orientation,
// will always have constant z depth.
// Thus a special case loop for very fast rendering can
// be used. It has also been used with Wolfenstein 3D.
//
// Templated version of a column mapping function.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template<typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawColumnGeneric(PIXEL_T* dest, const drawcolumn_t& drawcolumn)
{
#ifdef RANGECHECK
	if (drawcolumn.x < 0 || drawcolumn.x >= viewwidth || drawcolumn.yl < 0 || drawcolumn.yh >= viewheight)
	{
		PrintFmt(PRINT_HIGH, "R_DrawColumn: {} to {} at {}\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
		return;
	}
#endif

	const palindex_t* source = drawcolumn.source;
	const int pitch = 1;		// column-major: one row down is one pixel
	int count = drawcolumn.yh - drawcolumn.yl + 1;
	if (count <= 0)
		return;

	const fixed_t fracstep = drawcolumn.iscale;
	fixed_t frac = drawcolumn.texturefrac;

	const int texheight = drawcolumn.textureheight;
	const int mask = (texheight >> FRACBITS) - 1;

	COLORFUNC colorfunc(drawcolumn);

	if (drawcolumn.masked)
	{
		// handle masked (partially transparent) textures
		do
		{
			palindex_t pixel = source[frac >> FRACBITS];
			if (pixel != 0)
				colorfunc(pixel, dest);
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}
	else if (texheight & (texheight - 1))
	{
		// [SL] Properly tile textures whose heights are not a power-of-2,
		// avoiding a tutti-frutti effect.  From Eternity Engine.

		// texture height is NOT a power-of-2
		// just do a simple blit to the dest buffer (I'm lazy)
		if (frac < 0)
			while ((frac += texheight) < 0);
		else
			while (frac >= texheight)
				frac -= texheight;

		while (count--)
		{
			colorfunc(source[frac >> FRACBITS], dest);
			dest += pitch;
			if ((frac += fracstep) >= texheight)
				frac -= texheight;
		}
	}
	else
	{
		// texture height is a power-of-2
		// do some loop unrolling
		while (count >= 8)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			count -= 8;
		}

		if (count & 4)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
		}
		
		if (count & 2)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch; frac += fracstep;
		}

		if (count & 1)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
		}
	}
}


//
// R_DrawColumnGenericARGB
//
// Version of R_DrawColumnGeneric that samples the source texture's
// native ARGB plane instead of its palettized plane. The ARGB plane is
// parallel to the palettized plane, so the column's texels are found by
// rebasing dcol.source onto the ARGB plane. The color-remapping functor
// is responsible for transparency and alpha blending.
//
template<typename COLORFUNC>
static forceinline void R_DrawColumnGenericARGB(argb_t* dest, const drawcolumn_t& drawcolumn)
{
#ifdef RANGECHECK
	if (drawcolumn.x < 0 || drawcolumn.x >= viewwidth || drawcolumn.yl < 0 || drawcolumn.yh >= viewheight)
	{
		PrintFmt(PRINT_HIGH, "R_DrawColumn: {} to {} at {}\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
		return;
	}
#endif

	const argb_t* source = drawcolumn.argbtexturedata + (drawcolumn.source - drawcolumn.texturedata);
	const int pitch = 1;		// column-major: one row down is one pixel
	int count = drawcolumn.yh - drawcolumn.yl + 1;
	if (count <= 0)
		return;

	const fixed_t fracstep = drawcolumn.iscale;
	fixed_t frac = drawcolumn.texturefrac;

	const int texheight = drawcolumn.textureheight;
	const int mask = (texheight >> FRACBITS) - 1;

	COLORFUNC colorfunc(drawcolumn);

	if (drawcolumn.masked)
	{
		// handle masked (partially transparent) textures
		do
		{
			colorfunc(source[frac >> FRACBITS], dest);
			dest += pitch;
			frac += fracstep;
		} while (--count);
	}
	else if (texheight & (texheight - 1))
	{
		// texture height is NOT a power-of-2 (see R_DrawColumnGeneric)
		if (frac < 0)
			while ((frac += texheight) < 0);
		else
			while (frac >= texheight)
				frac -= texheight;

		while (count--)
		{
			colorfunc(source[frac >> FRACBITS], dest);
			dest += pitch;
			if ((frac += fracstep) >= texheight)
				frac -= texheight;
		}
	}
	else
	{
		// texture height is a power-of-2
		// do some loop unrolling
		while (count >= 8)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			count -= 8;
		}

		if (count & 4)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
		}

		if (count & 2)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
			frac += fracstep;
		}

		if (count & 1)
		{
			colorfunc(source[(frac >> FRACBITS) & mask], dest);
			dest += pitch;
		}
	}
}




// ----------------------------------------------------------------------------
//
// Level plane column colour functors
//
// Unlike the span functors these take a per-pixel colormap offset, because a
// column of a level plane crosses light bands. The offset comes from the
// plane's own per-y table and is already multiplied by 256, so adding it to the
// base colormap pointer is exactly what shaderef_t::with() would have computed.
//
// ----------------------------------------------------------------------------

class PaletteLevelFunc
{
public:
	PaletteLevelFunc(const drawplanecol_t& d) : cbase(d.cbase) { }

	forceinline void operator()(palindex_t c, unsigned int lightoff, palindex_t* dest) const
	{
		*dest = cbase[lightoff + c];
	}

private:
	const palindex_t* cbase;
};

class PaletteTranslucentLevelFunc
{
public:
	PaletteTranslucentLevelFunc(const drawplanecol_t& d) : cbase(d.cbase)
	{
		const fixed_t level = d.translevel & ~0x3ff;
		fg2rgb = Col2RGB8[level >> 10];
		bg2rgb = Col2RGB8[(FRACUNIT - level) >> 10];
	}

	forceinline void operator()(palindex_t c, unsigned int lightoff, palindex_t* dest) const
	{
		const unsigned int fg = fg2rgb[cbase[lightoff + c]];
		const unsigned int bg = bg2rgb[*dest];
		const unsigned int mix = (fg + bg) | 0x1f07c1f;
		*dest = RGB32k[0][0][mix & (mix >> 15)];
	}

private:
	const palindex_t*	cbase;
	const argb_t*		fg2rgb;
	const argb_t*		bg2rgb;
};

class DirectLevelFunc
{
public:
	DirectLevelFunc(const drawplanecol_t& d) : sbase(d.sbase) { }

	forceinline void operator()(palindex_t c, unsigned int lightoff, argb_t* dest) const
	{
		*dest = sbase[lightoff + c];
	}

private:
	const argb_t* sbase;
};

class DirectTranslucentLevelFunc
{
public:
	DirectTranslucentLevelFunc(const drawplanecol_t& d) : sbase(d.sbase)
	{
		calculate_alpha(d.translevel);
	}

	forceinline void operator()(palindex_t c, unsigned int lightoff, argb_t* dest) const
	{
		*dest = alphablend2a(*dest, bga, sbase[lightoff + c], fga);
	}

private:
	// Masked and clamped like its five siblings, which this one was not.
	// An unpaired stack boundary gives translevel == FRACUNIT, i.e. fga 256
	// and bga -1, and a weight out of range does not saturate one channel --
	// it carries into the next.
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;
	}

	const argb_t*	sbase;
	int				fga;
	int				bga;
};

//
// R_DrawLevelColumnGeneric
//
// Texture maps one screen column of a level plane. This is R_MapLevelPlane's
// own arithmetic collected on distance instead of on x, so it needs no divide:
// the entire dependence on y is one yslope[] load.
//
// NOTE: ushift/vshift stay inside the loop deliberately. Folding them into
// ubase/eu would save two ALU ops per pixel but move the double-to-fixed
// truncation from 16.16 granularity to TEXEL granularity, where truncation
// toward zero is floor+1 -- a full one-texel shift for negative coordinates and
// a discontinuity where one crosses zero. Fold them only together with a
// per-plane positivity bias that is an exact multiple of the texture period.
//
template<bool CONSTLIGHT, typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawLevelColumnGeneric(PIXEL_T* dest, const drawplanecol_t& d)
{
#ifdef RANGECHECK
	if (d.x < 0 || d.x >= viewwidth || d.yl < 0 || d.yh >= viewheight)
	{
		PrintFmt(PRINT_HIGH, "R_DrawLevelColumn: {} to {} at {}\n", d.yl, d.yh, d.x);
		return;
	}
#endif

	int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const int step = 1;		// the contiguous store this path exists for

	const palindex_t* const source = d.source;

	// Both streamed linearly down the column, so the prefetcher has them.
	const fixed_t* ys = yslope.get() + d.yl;
	const uint16_t* off = d.lightoff + d.yl;

	const double ubase = d.ubase, vbase = d.vbase;
	const double eu = d.eu, ev = d.ev;
	const int umask = d.umask, vmask = d.vmask;
	const int ushift = d.ushift, vshift = d.vshift;

	COLORFUNC colorfunc(d);

	do {
		// int64_t is what keeps an out-of-range double from being undefined,
		// the same reason R_DoubleToDsFixed casts through it.
		const double Y = static_cast<double>(*ys++);
		const dsfixed_t ufrac = static_cast<dsfixed_t>(static_cast<int64_t>(ubase + eu * Y));
		const dsfixed_t vfrac = static_cast<dsfixed_t>(static_cast<int64_t>(vbase + ev * Y));

		const unsigned int spot = ((vfrac >> vshift) & vmask) | ((ufrac >> ushift) & umask);

		colorfunc(source[spot], CONSTLIGHT ? 0u : *off, dest);
		if (!CONSTLIGHT)
			off++;
		dest += step;
	} while (--count);
}

//
// R_FillSpanGeneric
//
// Templated version of a function to fill a span with a solid color.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template<typename PIXEL_T, typename COLORFUNC>
static forceinline void R_FillSpanGeneric(PIXEL_T* dest, const drawspan_t& drawspan)
{
#ifdef RANGECHECK
	if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth ||
		drawspan.y >= viewheight || drawspan.y < 0)
	{
		PrintFmt(PRINT_HIGH, "R_FillSpan: {} to {} at {}", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	int color = drawspan.color;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;

	const int step = drawspan.colstep;		// one pixel right is one column

	COLORFUNC colorfunc(drawspan);

	do {
		colorfunc(color, dest);
		dest += step;
	} while (--count);
}


//
// R_DrawLevelSpanGeneric
//
// Templated version of a function to fill a horizontal span with a texture map.
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template<typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawLevelSpanGeneric(PIXEL_T* dest, const drawspan_t& drawspan)
{
#ifdef RANGECHECK
	if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth ||
		drawspan.y >= viewheight || drawspan.y < 0)
	{
		PrintFmt(PRINT_HIGH, "R_DrawLevelSpan: {} to {} at {}", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	const palindex_t* source = drawspan.source;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;

	// Consecutive x are a column apart, so at least one cache line each.
	// See openQuestions.
	const int step = drawspan.colstep;
	
	dsfixed_t ufrac = dspan.ufrac, vfrac = dspan.vfrac;
	dsfixed_t ustep = dspan.ustep, vstep = dspan.vstep;
	const int umask = dspan.umask, vmask = dspan.vmask;
	const int ushift = dspan.ushift, vshift = dspan.vshift;

	COLORFUNC colorfunc(drawspan);

	do {
		const unsigned int spot = ((vfrac >> vshift) & vmask) | ((ufrac >> ushift) & umask); 
		colorfunc(source[spot], dest);
		dest += step;
		ufrac += ustep;
		vfrac += vstep;
	} while (--count);
}


//
// R_DrawSlopedSpanGeneric
//
// Texture maps a sloped surface using affine texturemapping for each row of
// the span.  Not as pretty as a perfect texturemapping but should be much
// faster.
//
// Based on R_DrawSlope_8_64 from Eternity Engine, written by SoM/Quasar
//
// The data type of the destination pixels and a color-remapping functor
// are passed as template parameters.
//
template<typename PIXEL_T, typename COLORFUNC>
static forceinline void R_DrawSlopedSpanGeneric(PIXEL_T* dest, const drawspan_t& drawspan)
{
#ifdef RANGECHECK
	if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth ||
		drawspan.y >= viewheight || drawspan.y < 0)
	{
		PrintFmt(PRINT_HIGH, "R_DrawSlopedSpan: {} to {} at {}", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	const palindex_t* source = drawspan.source;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;

	// The renderer's worst store pattern: one pixel per cache line. Wants a
	// four-rows-in-four-lanes kernel like R_DrawLevelGroupD_SSE2's.
	// See openQuestions.
	const int step = drawspan.colstep;

	float iu = drawspan.iu, iv = drawspan.iv;
	const float ius = drawspan.iustep, ivs = drawspan.ivstep;
	float id = drawspan.id, ids = drawspan.idstep;

	int ltindex = 0;

	shaderef_t colormap;
	COLORFUNC colorfunc(drawspan);

	const int umask = dspan.umask, vmask = dspan.vmask;
	const int ushift = dspan.ushift, vshift = dspan.vshift;

	while (count >= SPANJUMP)
	{
		const float mulstart = 65536.0f / id;
		id += ids * SPANJUMP;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = static_cast<fixed_t>(ustart);
		fixed_t vfrac = static_cast<fixed_t>(vstart);

		iu += ius * SPANJUMP;
		iv += ivs * SPANJUMP;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		const fixed_t ustep = static_cast<fixed_t>((uend - ustart) * INTERPSTEP);
		const fixed_t vstep = static_cast<fixed_t>((vend - vstart) * INTERPSTEP);

		int incount = SPANJUMP;
		while (incount--)
		{
			colormap = drawspan.slopelighting[ltindex++];

			const unsigned int spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask); 
			colorfunc(source[spot], dest);
			dest += step;
			ufrac += ustep;
			vfrac += vstep;
		}

		count -= SPANJUMP;
	}

	if (count > 0)
	{
		const float mulstart = 65536.0f / id;
		id += ids * count;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = static_cast<fixed_t>(ustart);
		fixed_t vfrac = static_cast<fixed_t>(vstart);

		iu += ius * count;
		iv += ivs * count;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		const fixed_t ustep = static_cast<fixed_t>((uend - ustart) / count);
		const fixed_t vstep = static_cast<fixed_t>((vend - vstart) / count);

		int incount = count;
		while (incount--)
		{
			colormap = drawspan.slopelighting[ltindex++];

			const unsigned int spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);
			colorfunc(source[spot], dest);
			dest += step;
			ufrac += ustep;
			vfrac += vstep;
		}
	}
}


//
// R_DrawLevelSpanGenericARGB
//
// Version of R_DrawLevelSpanGeneric that samples the source texture's
// native ARGB plane (drawspan.argbsource), which is parallel to the
// palettized plane.
//
template<typename COLORFUNC>
static forceinline void R_DrawLevelSpanGenericARGB(argb_t* dest, const drawspan_t& drawspan)
{
#ifdef RANGECHECK
	if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth ||
		drawspan.y >= viewheight || drawspan.y < 0)
	{
		PrintFmt(PRINT_HIGH, "R_DrawLevelSpan: {} to {} at {}", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	const argb_t* source = drawspan.argbsource;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;

	// One pixel per cache line, and the path a level plane with a PNG flat takes
	// (argbsource fails R_DrawLevelPlane's column gate), so a full-screen ARGB
	// floor pays it. See openQuestions.
	const int step = drawspan.colstep;

	dsfixed_t ufrac = dspan.ufrac, vfrac = dspan.vfrac;
	dsfixed_t ustep = dspan.ustep, vstep = dspan.vstep;
	const int umask = dspan.umask, vmask = dspan.vmask;
	const int ushift = dspan.ushift, vshift = dspan.vshift;

	COLORFUNC colorfunc(drawspan);

	do {
		const unsigned int spot = ((vfrac >> vshift) & vmask) | ((ufrac >> ushift) & umask);
		colorfunc(source[spot], dest);
		dest += step;
		ufrac += ustep;
		vfrac += vstep;
	} while (--count);
}


//
// R_DrawSlopedSpanGenericARGB
//
// Version of R_DrawSlopedSpanGeneric that samples the source texture's
// native ARGB plane (drawspan.argbsource).
//
template<typename COLORFUNC>
static forceinline void R_DrawSlopedSpanGenericARGB(argb_t* dest, const drawspan_t& drawspan)
{
#ifdef RANGECHECK
	if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth ||
		drawspan.y >= viewheight || drawspan.y < 0)
	{
		PrintFmt(PRINT_HIGH, "R_DrawSlopedSpan: {} to {} at {}", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	const argb_t* source = drawspan.argbsource;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;

	const int step = drawspan.colstep;		// see R_DrawSlopedSpanGeneric

	float iu = drawspan.iu, iv = drawspan.iv;
	const float ius = drawspan.iustep, ivs = drawspan.ivstep;
	float id = drawspan.id, ids = drawspan.idstep;

	COLORFUNC colorfunc(drawspan);

	const int umask = dspan.umask, vmask = dspan.vmask;
	const int ushift = dspan.ushift, vshift = dspan.vshift;

	while (count >= SPANJUMP)
	{
		const float mulstart = 65536.0f / id;
		id += ids * SPANJUMP;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = static_cast<fixed_t>(ustart);
		fixed_t vfrac = static_cast<fixed_t>(vstart);

		iu += ius * SPANJUMP;
		iv += ivs * SPANJUMP;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		const fixed_t ustep = static_cast<fixed_t>((uend - ustart) * INTERPSTEP);
		const fixed_t vstep = static_cast<fixed_t>((vend - vstart) * INTERPSTEP);

		int incount = SPANJUMP;
		while (incount--)
		{
			const unsigned int spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);
			colorfunc(source[spot], dest);
			dest += step;
			ufrac += ustep;
			vfrac += vstep;
		}

		count -= SPANJUMP;
	}

	if (count > 0)
	{
		const float mulstart = 65536.0f / id;
		id += ids * count;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = static_cast<fixed_t>(ustart);
		fixed_t vfrac = static_cast<fixed_t>(vstart);

		iu += ius * count;
		iv += ivs * count;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		const fixed_t ustep = static_cast<fixed_t>((uend - ustart) / count);
		const fixed_t vstep = static_cast<fixed_t>((vend - vstart) / count);

		int incount = count;
		while (incount--)
		{
			const unsigned int spot = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);
			colorfunc(source[spot], dest);
			dest += step;
			ufrac += ustep;
			vfrac += vstep;
		}
	}
}


/************************************/
/*									*/
/* Palettized drawers (C versions)	*/
/*									*/
/************************************/

// ----------------------------------------------------------------------------
//
// 8bpp color remapping functors
//
// These functors provide a variety of ways to manipulate a source pixel
// color (given by 8bpp palette index) and write the result to the destination
// buffer.
//
// The functors are instantiated with a shaderef_t* parameter (typically
// dcol.colormap or dspan.colormap) that will be used to shade the pixel.
//
// ----------------------------------------------------------------------------

class PaletteFunc
{
public:
	PaletteFunc(const drawcolumn_t& drawcolumn) { }
	PaletteFunc(const drawspan_t& drawspan) { }

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		*dest = c;
	}
};

class PaletteColormapFunc
{
public:
	PaletteColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap) { }
	PaletteColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.colormap) { }

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		*dest = colormap.index(c);
	}

private:
	const shaderef_t& colormap;
};

class PaletteFuzzyFunc
{
public:
	PaletteFuzzyFunc(const drawcolumn_t& drawcolum) :
			colormap(&V_GetDefaultPalette()->maps, 6) { }

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		*dest = colormap.index(dest[fuzztable.getValue()]);
		fuzztable.incrementRow();
	}

private:
	shaderef_t colormap;
};

class PaletteTranslucentColormapFunc
{
public:
	PaletteTranslucentColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap)
	{
		calculate_alpha(drawcolumn.translevel);
	}

	PaletteTranslucentColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.colormap)
	{
		calculate_alpha(drawspan.translevel);
	}

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		const palindex_t fg = colormap.index(c);
		const palindex_t bg = *dest;

		*dest = rt_blend2<palindex_t>(bg, bga, fg, fga);
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;
	}

	const shaderef_t& colormap;
	int fga, bga;
};

class PaletteTranslatedColormapFunc
{
public:
	PaletteTranslatedColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap), translation(drawcolumn.translation) { }

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		*dest = colormap.index(translation.tlate(c));
	}

private:
	const shaderef_t& colormap;
	const translationref_t& translation;
};

class PaletteTranslatedTranslucentColormapFunc
{
public:
	PaletteTranslatedTranslucentColormapFunc(const drawcolumn_t& drawcolumn) :
			tlatefunc(drawcolumn), translation(drawcolumn.translation) { }

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		tlatefunc(translation.tlate(c), dest);
	}

private:
	PaletteTranslucentColormapFunc tlatefunc;
	const translationref_t& translation;
};

class PaletteSlopeColormapFunc
{
public:
	PaletteSlopeColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.slopelighting) { }

	forceinline void operator()(byte c, palindex_t* dest)
	{
		*dest = colormap->index(c);
		colormap++;
	}

private:
	const shaderef_t* colormap;
};

class PaletteSlopeTranslucentColormapFunc
{
public:
	PaletteSlopeTranslucentColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.slopelighting)
	{
		calculate_alpha(drawspan.translevel);
	}

	forceinline void operator()(byte c, palindex_t* dest)
	{
		const palindex_t fg = colormap->index(c);
		const palindex_t bg = *dest;
		*dest = rt_blend2<palindex_t>(bg, bga, fg, fga);
		colormap++;
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;
	}

	const shaderef_t* colormap;
	int fga, bga;
};

class PaletteSkyForegroundColormapFunc
{
public:
	PaletteSkyForegroundColormapFunc(const drawcolumn_t& drawcolumn) : colormap(drawcolumn.colormap) {}

	PaletteSkyForegroundColormapFunc(const drawspan_t& drawspan) : colormap(drawspan.colormap) {}

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		*dest = c == 0 ? *dest : colormap.index(c);
	}

private:
	const shaderef_t& colormap;
};

//
// R_ColumnOffset / R_SpanOffset
//
// Where in the view buffer a column or span starts, in pixels. The view buffer
// is column-major: x carries the column stride and y is the offset inside the
// column.
//
static forceinline ptrdiff_t R_ColumnOffset()
{
	return ptrdiff_t(dcol.x) * dcol.colstep + dcol.yl;
}

static forceinline ptrdiff_t R_SpanOffset()
{
	return ptrdiff_t(dspan.x1) * dspan.colstep + dspan.y;
}

// ----------------------------------------------------------------------------
//
// 8bpp color column drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_P (static_cast<palindex_t*>(dcol.destination) + R_ColumnOffset())

//
// R_FillColumnP
//
// Fills a column in the 8bpp palettized screen buffer with a solid color,
// determined by dcol.color. Performs no shading.
//
void R_FillColumnP()
{
	R_FillColumnGeneric<palindex_t, PaletteFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawColumnP
//
// Renders a column to the 8bpp palettized screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
//
void R_DrawColumnP()
{
	R_DrawColumnGeneric<palindex_t, PaletteColormapFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawFuzzColumnP
//
// Alters a column in the 8bpp palettized screen buffer using Doom's partial
// invisibility effect, which shades the column and rearranges the ordering
// the pixels to create distortion. Shading is performed using colormap 6.
//
void R_DrawFuzzColumnP()
{
	// adjust the borders (prevent buffer over/under-reads)
	if (dcol.yl <= 0)
		dcol.yl = 1;
	if (dcol.yh >= viewheight - 1)
		dcol.yh = viewheight - 2;

	R_FillColumnGeneric<palindex_t, PaletteFuzzyFunc>(FB_COLDEST_P, dcol);
	fuzztable.incrementColumn();
}

//
// R_DrawTranslucentColumnP
//
// Renders a translucent column to the 8bpp palettized screen buffer from the
// source buffer dcol.source and scaled by dcol.iscale. The amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTranslucentColumnP()
{
	R_DrawColumnGeneric<palindex_t, PaletteTranslucentColormapFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawTranslatedColumnP
//
// Renders a column to the 8bpp palettized screen buffer with color-remapping
// from the source buffer dcol.source and scaled by dcol.iscale. The translation
// table is supplied by dcol.translation. Shading is performed using dcol.colormap.
//
void R_DrawTranslatedColumnP()
{
	R_DrawColumnGeneric<palindex_t, PaletteTranslatedColormapFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawTlatedLucentColumnP
//
// Renders a translucent column to the 8bpp palettized screen buffer with
// color-remapping from the source buffer dcol.source and scaled by dcol.iscale.
// The translation table is supplied by dcol.translation and the amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTlatedLucentColumnP()
{
	R_DrawColumnGeneric<palindex_t, PaletteTranslatedTranslucentColormapFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawSkyForegroundColumnP
//
// Renders a column to the 8bpp palettized screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
// Palette index 0 is treated as transparent.
// This is because we can't use SKYTRAN.
//
void R_DrawSkyForegroundColumnP()
{
	R_DrawColumnGeneric<palindex_t, PaletteSkyForegroundColormapFunc>(FB_COLDEST_P, dcol);
}

// ----------------------------------------------------------------------------
//
// 8bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_P (dspan.destination + R_SpanOffset())

//
// R_FillSpanP
//
// Fills a span in the 8bpp palettized screen buffer with a solid color,
// determined by dspan.color. Performs no shading.
//
void R_FillSpanP()
{
	R_FillSpanGeneric<palindex_t, PaletteFunc>(FB_SPANDEST_P, dspan);
}

//
// R_FillTranslucentSpanP
//
// Fills a span in the 8bpp palettized screen buffer with a solid color,
// determined by dspan.color using translucency. Shading is performed
// using dspan.colormap.
//
void R_FillTranslucentSpanP()
{
	R_FillSpanGeneric<palindex_t, PaletteTranslucentColormapFunc>(FB_SPANDEST_P, dspan);
}

//
// R_DrawSpanP
//
// Renders a span for a level plane to the 8bpp palettized screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSpanP()
{
	R_DrawLevelSpanGeneric<palindex_t, PaletteColormapFunc>(FB_SPANDEST_P, dspan);
}

//
// R_DrawTranslucentSpanP
//
// Renders a span for a level plane to the 8bpp palettized screen buffer from
// the source buffer dspan.source, blended with the framebuffer by
// dspan.translevel. Shading is performed using dspan.colormap.
//
void R_DrawTranslucentSpanP()
{
	R_DrawLevelSpanGeneric<palindex_t, PaletteTranslucentColormapFunc>(FB_SPANDEST_P, dspan);
}

//
// R_DrawSlopeSpanP
//
// Renders a span for a sloped plane to the 8bpp palettized screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSlopeSpanP()
{
	R_DrawSlopedSpanGeneric<palindex_t, PaletteSlopeColormapFunc>(FB_SPANDEST_P, dspan);
}

//
// R_DrawTranslucentSlopeSpanP
//
// Renders a span for a sloped plane to the 8bpp palettized screen buffer from
// the source buffer dspan.source, blended with the framebuffer by
// dspan.translevel. Shading is performed using dspan.slopelighting.
//
void R_DrawTranslucentSlopeSpanP()
{
	R_DrawSlopedSpanGeneric<palindex_t, PaletteSlopeTranslucentColormapFunc>(FB_SPANDEST_P, dspan);
}


/****************************************/
/*										*/
/* [RH] ARGB8888 drawers (C versions)	*/
/*										*/
/****************************************/

// ----------------------------------------------------------------------------
//
// 32bpp color remapping functors
//
// These functors provide a variety of ways to manipulate a source pixel
// color (given by 8bpp palette index) and write the result to the destination
// buffer.
//
// The functors are instantiated with a shaderef_t* parameter (typically
// dcol.colormap or dspan.colormap) that will be used to shade the pixel.
//
// ----------------------------------------------------------------------------

class DirectFunc
{
public:
	DirectFunc(const drawcolumn_t& drawcolumn) { }
	DirectFunc(const drawspan_t& drawspan) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		*dest = basecolormap.shade(c);
	}
};

class DirectColormapFunc
{
public:
	DirectColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap) { }
	DirectColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.colormap) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		*dest = colormap.shade(c);
	}

private:
	const shaderef_t& colormap;
};

class DirectFuzzyFunc
{
public:
	DirectFuzzyFunc(const drawcolumn_t& drawcolumn) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		const argb_t work = dest[fuzztable.getValue()];
		*dest = work - ((work >> 2) & 0x3f3f3f);
		fuzztable.incrementRow();
	}
};

class DirectTranslucentColormapFunc
{
public:
	DirectTranslucentColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap)
	{
		calculate_alpha(drawcolumn.translevel);
	}

	DirectTranslucentColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.colormap)
	{
		calculate_alpha(drawspan.translevel);
	}

	forceinline void operator()(byte c, argb_t* dest) const
	{
		const argb_t fg = colormap.shade(c);
		const argb_t bg = *dest;
		*dest = alphablend2a(bg, bga, fg, fga);
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;
	}

	const shaderef_t& colormap;
	int fga, bga;
};

class DirectTranslatedColormapFunc
{
public:
	DirectTranslatedColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap), translation(drawcolumn.translation) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		*dest = colormap.tlate(translation, c);
	}

private:
	const shaderef_t& colormap;
	const translationref_t& translation;
};

class DirectTranslatedTranslucentColormapFunc
{
public:
	DirectTranslatedTranslucentColormapFunc(const drawcolumn_t& drawcolumn) :
			tlatefunc(drawcolumn), translation(drawcolumn.translation) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		tlatefunc(translation.tlate(c), dest);
	}

private:
	DirectTranslucentColormapFunc tlatefunc;
	const translationref_t& translation;
};

class DirectSlopeColormapFunc
{
public:
	DirectSlopeColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.slopelighting) { }

	forceinline void operator()(byte c, argb_t* dest)
	{
		*dest = colormap->shade(c);
		colormap++;
	}

private:
	const shaderef_t* colormap;
};

class DirectSlopeTranslucentColormapFunc
{
public:
	DirectSlopeTranslucentColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.slopelighting)
	{
		calculate_alpha(drawspan.translevel);
	}

	forceinline void operator()(byte c, argb_t* dest)
	{
		const argb_t fg = colormap->shade(c);
		const argb_t bg = *dest;
		*dest = alphablend2a(bg, bga, fg, fga);
		colormap++;
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
		bga = 255 - fga;
	}

	const shaderef_t* colormap;
	int fga, bga;
};

class DirectSkyForegroundColormapFunc
{
public:
	DirectSkyForegroundColormapFunc(const drawcolumn_t& drawcolumn) : colormap(drawcolumn.colormap) {}

	DirectSkyForegroundColormapFunc(const drawspan_t& drawspan) : colormap(drawspan.colormap) {}

	forceinline void operator()(byte c, argb_t* dest) const
	{
		*dest = c == 0 ? *dest : colormap.shade(c);
	}

private:
	const shaderef_t& colormap;
};


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
// Gives the native ARGB drawers a shademap-like lookup - for each light
// diminishing level, three 256-entry tables map a channel value to its
// diminished, faded and gamma-corrected result. Colormaps carrying a
// dynamic colormap (sector colormaps) are shaded separately.
//
static byte argb_shade_lut_r[NUMCOLORMAPS + 1][256];
static byte argb_shade_lut_g[NUMCOLORMAPS + 1][256];
static byte argb_shade_lut_b[NUMCOLORMAPS + 1][256];

// Rebuild shade LUT table (new map or gamma bump).
void R_UpdateARGBShadeLUT()
{
	const argb_t fadecolor(level.fadeto_color[0], level.fadeto_color[1],
	                       level.fadeto_color[2], level.fadeto_color[3]);

	for (int m = 0; m <= NUMCOLORMAPS; m++)
	{
		const int fade_r = fadecolor.getr() * m + NUMCOLORMAPS / 2;
		const int fade_g = fadecolor.getg() * m + NUMCOLORMAPS / 2;
		const int fade_b = fadecolor.getb() * m + NUMCOLORMAPS / 2;

		for (int c = 0; c < 256; c++)
		{
			argb_shade_lut_r[m][c] = gammatable[(c * (NUMCOLORMAPS - m) + fade_r) / NUMCOLORMAPS];
			argb_shade_lut_g[m][c] = gammatable[(c * (NUMCOLORMAPS - m) + fade_g) / NUMCOLORMAPS];
			argb_shade_lut_b[m][c] = gammatable[(c * (NUMCOLORMAPS - m) + fade_b) / NUMCOLORMAPS];
		}
	}
}

//
// ARGBShader
//
// Shades native ARGB texels for a fixed colormap. The common case
// (no dynamic colormap) is three lookups in the per-frame shade tables.
// Colormaps with a dynamic colormap precompute the light diminishing
// constants once and shade when needed.
//
class ARGBShader
{
public:
	ARGBShader(const shaderef_t& colormap)
	{
		const int mapnum =
		    colormap.mapnum() < NUMCOLORMAPS ? colormap.mapnum() : NUMCOLORMAPS;

		if (colormap.m_dyncolormap == NULL)
		{
			lut_r = argb_shade_lut_r[mapnum];
			lut_g = argb_shade_lut_g[mapnum];
			lut_b = argb_shade_lut_b[mapnum];
		}
		else
		{
			lut_r = lut_g = lut_b = NULL;

			const argb_t lightcolor = colormap.m_dyncolormap->color;
			const argb_t fadecolor = colormap.m_dyncolormap->fade;

			scale_r = lightcolor.getr() * (NUMCOLORMAPS - mapnum);
			scale_g = lightcolor.getg() * (NUMCOLORMAPS - mapnum);
			scale_b = lightcolor.getb() * (NUMCOLORMAPS - mapnum);

			add_r = fadecolor.getr() * mapnum + NUMCOLORMAPS / 2;
			add_g = fadecolor.getg() * mapnum + NUMCOLORMAPS / 2;
			add_b = fadecolor.getb() * mapnum + NUMCOLORMAPS / 2;
		}
	}

	forceinline argb_t shade(const argb_t c) const
	{
		if (lut_r != NULL)
			return argb_t(c.geta(), lut_r[c.getr()], lut_g[c.getg()], lut_b[c.getb()]);

		return argb_t(c.geta(),
		              gammatable[(c.getr() * scale_r / 255 + add_r) / NUMCOLORMAPS],
		              gammatable[(c.getg() * scale_g / 255 + add_g) / NUMCOLORMAPS],
		              gammatable[(c.getb() * scale_b / 255 + add_b) / NUMCOLORMAPS]);
	}

private:
	const byte* lut_r;
	const byte* lut_g;
	const byte* lut_b;
	int scale_r, scale_g, scale_b;
	int add_r, add_g, add_b;
};

class DirectARGBColormapFunc
{
public:
	DirectARGBColormapFunc(const drawcolumn_t& drawcolumn) :
			shader(drawcolumn.colormap), blend_to_dest(drawcolumn.masked) { }
	DirectARGBColormapFunc(const drawspan_t& drawspan) :
			shader(drawspan.colormap), blend_to_dest(false) { }

	forceinline void operator()(argb_t c, argb_t* dest) const
	{
		const int a = c.geta();
		if (a == 255)
		{
			*dest = shader.shade(c);
		}
		else if (a > 0)
		{
			const argb_t bg = blend_to_dest ? *dest : argb_t(255, 0, 0, 0);
			*dest = alphablend2a(bg, 255 - a, shader.shade(c), a);
		}
		else if (!blend_to_dest)
		{
			*dest = argb_t(255, 0, 0, 0);
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

	DirectARGBTranslucentColormapFunc(const drawspan_t& drawspan) :
			shader(drawspan.colormap)
	{
		calculate_alpha(drawspan.translevel);
	}

	forceinline void operator()(argb_t c, argb_t* dest) const
	{
		// combine the surface's translucency with the texel's alpha
		const int a = c.geta() * fga / 255;
		if (a == 0)
			return;

		*dest = alphablend2a(*dest, 255 - a, shader.shade(c), a);
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
	}

	const ARGBShader shader;
	int fga;
};

class DirectARGBSlopeColormapFunc
{
public:
	DirectARGBSlopeColormapFunc(const drawspan_t& drawspan) :
			colormap(drawspan.slopelighting) { }

	forceinline void operator()(argb_t c, argb_t* dest)
	{
		const ARGBShader shader(*colormap);
		colormap++;

		const int a = c.geta();
		if (a == 255)
			*dest = shader.shade(c);
		else if (a > 0)
			*dest = alphablend2a(argb_t(255, 0, 0, 0), 255 - a, shader.shade(c), a);
		else
			*dest = argb_t(255, 0, 0, 0);
	}

private:
	const shaderef_t* colormap;
};


// ----------------------------------------------------------------------------
//
// 32bpp color drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_D (reinterpret_cast<argb_t*>(dcol.destination) + R_ColumnOffset())

//
// R_ColumnHasNativeARGB
//
// Returns true when the current column should be drawn from its
// texture's native ARGB plane: the plane must exist, and special colormaps
// (e.g. invulnerability) have no RGB equivalent so those columns stay on
// the palettized path.
//
static forceinline bool R_ColumnHasNativeARGB()
{
	return dcol.argbtexturedata != NULL && dcol.source != NULL &&
	       dcol.colormap.mapnum() < NUMCOLORMAPS;
}

// Set by R_InitVectorizedDrawers, which runs long before any column is drawn.
static bool have_quad_columns = false;

// The shortest column worth handing to the four-pixel drawers.
//
// Not tuning: short columns measure SLOWER than scalar, 0.68x to 1.04x at twelve
// pixels, because building the vector constants per column -- plus MSVC spilling
// the callee-saved xmm6-xmm9 -- swamps two or three quads of work.
//
// 16 for margin. The curve is flat here: 16, 32 and 64 measured within 0.4%.
constexpr int R_QUAD_MIN_COLUMN = 16;

//
// R_ColumnWantsQuad
//
// A non-masked column tiles its texel index with a power-of-two mask, which is
// the right tiling only when the height really is a power of two; otherwise
// R_DrawColumnGeneric takes its own modulo arm and the quad drawer must not.
//
static forceinline bool R_ColumnWantsQuad()
{
	return have_quad_columns &&
			dcol.yh - dcol.yl + 1 >= R_QUAD_MIN_COLUMN &&
			(dcol.masked ||
				(dcol.textureheight & (dcol.textureheight - 1)) == 0);
}

//
// R_FillColumnD
//
// Fills a column in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dcol.color. Performs no shading.
//
void R_FillColumnD()
{
	R_FillColumnGeneric<argb_t, DirectFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawColumnD
//
// Renders a column to the 32bpp ARGB8888 screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
//
void R_DrawColumnD()
{
	if (R_ColumnHasNativeARGB())
		R_DrawColumnGenericARGB<DirectARGBColormapFunc>(FB_COLDEST_D, dcol);
#ifdef __SSE2__
	// Worth having despite costing slightly MORE instructions than the scalar
	// drawer -- 12.25 per pixel against 12. An opaque column is one gather and
	// one store per pixel and SSE2 cannot vectorize a gather, so the arithmetic
	// says this should be a wash. It is not: the quad form issues ONE 16-byte
	// store where the scalar issues four, and this loop is limited by store-port
	// throughput rather than by issue rate. Measured at 2% of the whole view
	// render in an ordinary scene, consistently across four runs.
	else if (R_ColumnWantsQuad())
		R_DrawColumnD_SSE2();
#endif
	else
		R_DrawColumnGeneric<argb_t, DirectColormapFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawFuzzColumnD
//
// Alters a column in the 32bpp ARGB8888 screen buffer using Doom's partial
// invisibility effect, which shades the column and rearranges the ordering
// the pixels to create distortion. Shading is performed using colormap 6.
//
void R_DrawFuzzColumnD()
{
	// adjust the borders (prevent buffer over/under-reads)
	if (dcol.yl <= 0)
		dcol.yl = 1;
	if (dcol.yh >= viewheight - 1)
		dcol.yh = viewheight - 2;

	R_FillColumnGeneric<argb_t, DirectFuzzyFunc>(FB_COLDEST_D, dcol);
	fuzztable.incrementColumn();
}

//
// R_DrawTranslucentColumnD
//
// Renders a translucent column to the 32bpp ARGB8888 screen buffer from the
// source buffer dcol.source and scaled by dcol.iscale. The amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTranslucentColumnD()
{
	if (R_ColumnHasNativeARGB())
		R_DrawColumnGenericARGB<DirectARGBTranslucentColormapFunc>(FB_COLDEST_D, dcol);
#ifdef __SSE2__
	else if (R_ColumnWantsQuad())
		R_DrawTranslucentColumnD_SSE2();
#endif
	else
		R_DrawColumnGeneric<argb_t, DirectTranslucentColormapFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawTranslatedColumnD
//
// Renders a column to the 32bpp ARGB8888 screen buffer with color-remapping
// from the source buffer dcol.source and scaled by dcol.iscale. The translation
// table is supplied by dcol.translation. Shading is performed using dcol.colormap.
//
void R_DrawTranslatedColumnD()
{
	R_DrawColumnGeneric<argb_t, DirectTranslatedColormapFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawTlatedLucentColumnD
//
// Renders a translucent column to the 32bpp ARGB8888 screen buffer with
// color-remapping from the source buffer dcol.source and scaled by dcol.iscale.
// The translation table is supplied by dcol.translation and the amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTlatedLucentColumnD()
{
	R_DrawColumnGeneric<argb_t, DirectTranslatedTranslucentColormapFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawSkyForegroundColumnD
//
// Renders a column to the 32bpp ARGB8888 screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
// Palette index 0 is treated as transparent.
// This is because we can't use SKYTRAN.
//
void R_DrawSkyForegroundColumnD()
{
	R_DrawColumnGeneric<argb_t, DirectSkyForegroundColormapFunc>(FB_COLDEST_D, dcol);
}

// ----------------------------------------------------------------------------
//
// 32bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_D (reinterpret_cast<argb_t*>(dspan.destination) + R_SpanOffset())

//
// R_FillSpanD
//
// Fills a span in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dspan.color. Performs no shading.
//
void R_FillSpanD()
{
	R_FillSpanGeneric<argb_t, DirectFunc>(FB_SPANDEST_D, dspan);
}

//
// R_FillTranslucentSpanD
//
// Fills a span in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dspan.color using translucency. Shading is performed
// using dspan.colormap.
//
void R_FillTranslucentSpanD()
{
	R_FillSpanGeneric<argb_t, DirectTranslucentColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawSpanD
//
// Renders a span for a level plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSpanD()
{
	if (dspan.argbsource != NULL && dspan.colormap.mapnum() < NUMCOLORMAPS)
		R_DrawLevelSpanGenericARGB<DirectARGBColormapFunc>(FB_SPANDEST_D, dspan);
	else
		R_DrawLevelSpanGeneric<argb_t, DirectColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawTranslucentSpanD
//
// Renders a span for a level plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source, blended with the framebuffer by
// dspan.translevel. Shading is performed using dspan.colormap.
//
void R_DrawTranslucentSpanD()
{
	R_DrawLevelSpanGeneric<argb_t, DirectTranslucentColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawTranslucentSlopeSpanD
//
// Renders a span for a sloped plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source, blended with the framebuffer by
// dspan.translevel. Shading is performed using dspan.slopelighting.
//
void R_DrawTranslucentSlopeSpanD()
{
	R_DrawSlopedSpanGeneric<argb_t, DirectSlopeTranslucentColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawSlopeSpanD
//
// Renders a span for a sloped plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSlopeSpanD()
{
	if (dspan.argbsource != NULL && dspan.slopelighting[0].mapnum() < NUMCOLORMAPS)
		R_DrawSlopedSpanGenericARGB<DirectARGBSlopeColormapFunc>(FB_SPANDEST_D, dspan);
	else
		R_DrawSlopedSpanGeneric<argb_t, DirectSlopeColormapFunc>(FB_SPANDEST_D, dspan);
}


/****************************************************/

void R_InitializeScreenblocksCanvas()
{
	IWindowSurface* primary_surface = R_GetRenderingSurface();
	int surface_width = primary_surface->getWidth(),
	    surface_height = primary_surface->getHeight();

	// Draw screenblocks to a 320x200 surface and scale it based on viewport height
	// If it doesn't reach the side edges of viewport or over, scale it via
	// top of surface and spill over the bottom and right
	int screenblockWidth = I_GetAspectCorrectWidth(surface_height, 200.0f, 320);
	int screenblockHeight = surface_height;

	if (screenblockWidth < surface_width)
	{
		float width_scale_ratio = static_cast<float>(surface_width) / static_cast<float>(screenblockWidth);
		screenblockWidth *= width_scale_ratio;
		screenblockHeight *= width_scale_ratio;
	}

	if (screenblocks_surface == NULL)
		screenblocks_surface = I_AllocateSurface(320, 200, 8);
	if (scaled_screenblocks_surface == NULL)
		scaled_screenblocks_surface = I_AllocateSurface(surface_width, surface_height, 8);

	screenblocks_surface->clear();
	scaled_screenblocks_surface->clear();

	screenblocks_surface->lock();
	scaled_screenblocks_surface->lock();

	scaled_screenblocks_surface->getDefaultCanvas();

	// background
	const Texture* border_texture = Res_CacheTexture(::gameinfo.borderFlat, FLOOR);
	if (border_texture)
	{
		// Support high resolution flats
		screenblocks_surface->getDefaultCanvas()->FlatFill(border_texture, 0, 0, 320, 200);
	}
	else
	{
		screenblocks_surface->getDefaultCanvas()->Clear(0, 0, 320, 200, argb_t(0, 0, 0));
	}

	// Blit the smaller screenblocks into the big one
	scaled_screenblocks_surface->blitcrop(screenblocks_surface, 0, 0, 320, 200,
	   0, 0, screenblockWidth, screenblockHeight);

	screenblocks_surface->unlock();
	scaled_screenblocks_surface->unlock();
}

void R_DrawBorder(int x1, int y1, int x2, int y2)
{
	IWindowSurface* primary_surface = R_GetRenderingSurface();

	if (!scaled_screenblocks_surface || !screenblocks_surface)
		R_InitializeScreenblocksCanvas();

	scaled_screenblocks_surface->lock();

	// Callers pass x2/y2 as the right and bottom EDGE, and blit() wants extents.
	// This used to pass x1 + x2, which blit() clamped to the surface edge -- right
	// by accident for the strips whose edge already was the surface edge, and
	// wrong for the bottom one, which then painted over the status bar.
	primary_surface->blit(scaled_screenblocks_surface, x1, y1,
	   x2 - x1, y2 - y1,
	   x1, y1, x2 - x1, y2 - y1);

	scaled_screenblocks_surface->unlock();
}


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void V_MarkRect (int x, int y, int width, int height);

void R_DrawViewBorder()
{
	if (!R_BorderVisible())
		return;

	IWindowSurface* surface = R_GetRenderingSurface();
	const DCanvas* canvas = surface->getDefaultCanvas();
	const int surface_width = surface->getWidth();
	const int surface_height = surface->getHeight();
	const int top = 0, bottom = ST_StatusBarY(surface_width, surface_height);
	const int left = 0, right = surface_width;

	// draw top border
	R_DrawBorder(left, top, right, viewwindowy);
	// draw bottom border
	R_DrawBorder(left, viewwindowy + viewheight, right, bottom);
	// draw left border
	R_DrawBorder(left, viewwindowy, viewwindowx, viewwindowy + viewheight);
	// draw right border
	R_DrawBorder(viewwindowx + viewwidth, viewwindowy, right, viewwindowy + viewheight);

	const gameborder_t& border = gameinfo.border;
	const int offset = border.offset;
	const int size = border.size;

	if (size == 0)
		return;

	const Texture* t_texture = Res_CacheTexture(border.t, PATCH);
	const Texture* b_texture = Res_CacheTexture(border.b, PATCH);
	const Texture* l_texture = Res_CacheTexture(border.l, PATCH);
	const Texture* r_texture = Res_CacheTexture(border.r, PATCH);
	const Texture* tl_texture = Res_CacheTexture(border.tl, PATCH);
	const Texture* tr_texture = Res_CacheTexture(border.tr, PATCH);
	const Texture* bl_texture = Res_CacheTexture(border.bl, PATCH);
	const Texture* br_texture = Res_CacheTexture(border.br, PATCH);

	// draw beveled edge for the viewing window's top and bottom edges
	for (int x = viewwindowx; x < viewwindowx + viewwidth; x += size)
	{
		canvas->DrawTexture(t_texture, x, viewwindowy - offset);
		canvas->DrawTexture(b_texture, x, viewwindowy + viewheight);
	}

	// draw beveled edge for the viewing window's left and right edges
	for (int y = viewwindowy; y < viewwindowy + viewheight; y += size)
	{
		canvas->DrawTexture(l_texture, viewwindowx - offset, y);
		canvas->DrawTexture(r_texture, viewwindowx + viewwidth, y);
	}

	// draw beveled edge for the viewing window's corners
	canvas->DrawTexture(tl_texture, viewwindowx - offset, viewwindowy - offset);
	canvas->DrawTexture(tr_texture, viewwindowx + viewwidth, viewwindowy - offset);
	canvas->DrawTexture(bl_texture, viewwindowx - offset, viewwindowy + viewheight);
	canvas->DrawTexture(br_texture, viewwindowx + viewwidth, viewwindowy + viewheight);

	V_MarkRect(left, top, right, bottom);
}


enum r_optimize_kind {
	OPTIMIZE_NONE,
	OPTIMIZE_SSE2,
	OPTIMIZE_MMX,
	OPTIMIZE_ALTIVEC
};

static r_optimize_kind optimize_kind = OPTIMIZE_NONE;
static std::vector<r_optimize_kind> optimizations_available;

static const char *get_optimization_name(r_optimize_kind kind)
{
	switch (kind)
	{
		case OPTIMIZE_SSE2:    return "sse2";
		case OPTIMIZE_MMX:     return "mmx";
		case OPTIMIZE_ALTIVEC: return "altivec";
		case OPTIMIZE_NONE:
		default:
			return "none";
	}
}

static std::string get_optimization_name_list(const bool includeNone)
{
	std::string str;
	std::vector<r_optimize_kind>::const_iterator it = optimizations_available.begin();
	if (!includeNone)
		++it;

	for (; it != optimizations_available.end(); ++it)
	{
		str.append(get_optimization_name(*it));
		if (it+1 != optimizations_available.end())
			str.append(", ");
	}
	return str;
}

static void print_optimizations()
{
	PrintFmt(PRINT_HIGH, "r_optimize detected \"{}\"\n", get_optimization_name_list(false));
}

static bool detect_optimizations()
{
	if (!optimizations_available.empty())
		return false;

	optimizations_available.clear();

	// Start with default non-optimized:
	optimizations_available.push_back(OPTIMIZE_NONE);

	// Detect CPU features in ascending order of preference:
	#ifdef __MMX__
	if (SDL_HasMMX())
		optimizations_available.push_back(OPTIMIZE_MMX);
	#endif
	#ifdef __SSE2__
	if (SDL_HasSSE2())
		optimizations_available.push_back(OPTIMIZE_SSE2);
	#endif
	#ifdef __ALTIVEC__
	if (SDL_HasAltiVec())
		optimizations_available.push_back(OPTIMIZE_ALTIVEC);
	#endif

	return true;
}

//
// R_IsOptimizationAvailable
//
// Returns true if Odamex was compiled with support for the optimization
// and the current CPU also supports it.
//
static bool R_IsOptimizationAvailable(r_optimize_kind kind)
{
	return std::find(optimizations_available.begin(), optimizations_available.end(), kind)
			!= optimizations_available.end();
}


CVAR_FUNC_IMPL(r_optimize)
{
	const char* val = var.cstring();

	// Only print the detected list the first time:
	if (detect_optimizations())
		print_optimizations();

	// Set the optimization based on availability:
	if (stricmp(val, "none") == 0)
		optimize_kind = OPTIMIZE_NONE;
	else if (stricmp(val, "sse2") == 0 && R_IsOptimizationAvailable(OPTIMIZE_SSE2))
		optimize_kind = OPTIMIZE_SSE2;
	else if (stricmp(val, "mmx") == 0 && R_IsOptimizationAvailable(OPTIMIZE_MMX))
		optimize_kind = OPTIMIZE_MMX;
	else if (stricmp(val, "altivec") == 0 && R_IsOptimizationAvailable(OPTIMIZE_ALTIVEC))
		optimize_kind = OPTIMIZE_ALTIVEC;
	else if (stricmp(val, "detect") == 0)
		// Default to the most preferred:
		optimize_kind = optimizations_available.back();
	else
	{
		PrintFmt(PRINT_HIGH, "Invalid value for r_optimize. Available options are \"{}, detect\"\n",
		         get_optimization_name_list(true));

		// Restore the original setting:
		var.Set(get_optimization_name(optimize_kind));
		return;
	}

	const char* optimize_name = get_optimization_name(optimize_kind);
	if (stricmp(val, optimize_name) != 0)
	{
		// update the cvar string
		// this will trigger the callback to run a second time
		PrintFmt(PRINT_HIGH, "r_optimize set to \"{}\" based on availability\n", optimize_name);
		var.Set(optimize_name);
	}
	else
	{
		// cvar string is current, now intialize the drawing function pointers
		R_InitVectorizedDrawers();
		R_InitColumnDrawers();
	}
}


//
// R_InitVectorizedDrawers
//
// Sets up the function pointers based on CPU optimization selected.
//
void R_InitVectorizedDrawers()
{
	// [SL] defaults are the non-vectorized drawers
	R_DrawLevelGroupD		= R_DrawLevelGroupD_c;
	r_dimpatchD             = r_dimpatchD_c;

	#ifdef __SSE2__
	if (optimize_kind == OPTIMIZE_SSE2)
	{
		R_DrawLevelGroupD		= R_DrawLevelGroupD_SSE2;
		r_dimpatchD             = r_dimpatchD_SSE2;
		have_quad_columns		= true;
	}
	#endif
	#ifdef __MMX__
	if (optimize_kind == OPTIMIZE_MMX)
	{
		r_dimpatchD             = r_dimpatchD_MMX;
	}
	#endif
	#ifdef __ALTIVEC__
	if (optimize_kind == OPTIMIZE_ALTIVEC)
	{
		r_dimpatchD             = r_dimpatchD_ALTIVEC;
	}
	#endif

	// Independent ifs, not an else-if chain: optimize_kind already makes them
	// exclusive, and chaining would leave a dangling else on any build without
	// __SSE2__ -- which is every Altivec build.
	//
	// Only SSE2 has a group kernel. MMX and Altivec supply the dim-patch drawer
	// alone and keep R_DrawLevelGroupD_c.

	// Check that all pointers are definitely assigned!
	assert(R_DrawLevelGroupD != NULL);
	assert(r_dimpatchD != NULL);
}

// ============================================================================
//
// Transposed view buffer
//
// Every surface is column-major -- a screen column is a contiguous run of pixels
// -- and the GPU turns the primary one the right way round at present time
// (i_video_sdl20.cpp).
//
// There is no untranspose pass; an earlier version folded a separate view buffer
// back on the CPU and cost 7.2ms a frame at 2560x1210, latency bound on gathers
// nothing can prefetch. Nor is the layout a setting: from identical viewpoints
// the column-major render is 4.123 -> 2.779 ms, 1.48x. One drawer table, and it
// addresses the view column-major.
//
// ============================================================================

//
// R_BindViewBuffer
//
// Points dcol/dspan/dpcol at the view rect of the surface. Called from
// R_InitViewWindow.
//
void R_BindViewBuffer(IWindowSurface* surface)
{
	dcol.destination = dspan.destination = dpcol.destination =
			surface->getBuffer(viewwindowx, viewwindowy);
	dcol.pitch_in_pixels = dspan.pitch_in_pixels = dpcol.pitch_in_pixels =
			surface->getRowStepInPixels();
	dcol.colstep = dspan.colstep = dpcol.colstep = surface->getColStepInPixels();

	R_InitColumnDrawers();
}

//
// R_ClearViewBuffer
//
// Fills the view with a solid colour (r_flashhom). The canvas follows the
// surface layout, so this needs no special case.
//
void R_ClearViewBuffer(IWindowSurface* surface, argb_t color)
{
	const int x1 = viewwindowx, y1 = viewwindowy;
	const int x2 = viewwindowx + viewwidth - 1, y2 = viewwindowy + viewheight - 1;

	surface->getDefaultCanvas()->Clear(x1, y1, x2, y2, color);
}

//
// Level plane column drawing wrappers
//
// CONSTLIGHT drops the per-y colormap table. R_DrawLevelPlane sets it when the
// plane resolves to one colormap for every row: fixedlightlev, an active
// fixedcolormap, or a plane shallow enough to land in one light band.
//
#define FB_LEVELDEST_P (static_cast<palindex_t*>(dpcol.destination) + R_LevelColumnOffset())
#define FB_LEVELDEST_D (reinterpret_cast<argb_t*>(dpcol.destination) + R_LevelColumnOffset())

static forceinline ptrdiff_t R_LevelColumnOffset()
{
	return ptrdiff_t(dpcol.x) * dpcol.colstep + dpcol.yl;
}

void R_DrawLevelColumnP()
{
	if (dpcol.lightoff == NULL)
		R_DrawLevelColumnGeneric<true, palindex_t, PaletteLevelFunc>(FB_LEVELDEST_P, dpcol);
	else
		R_DrawLevelColumnGeneric<false, palindex_t, PaletteLevelFunc>(FB_LEVELDEST_P, dpcol);
}

void R_DrawTranslucentLevelColumnP()
{
	if (dpcol.lightoff == NULL)
		R_DrawLevelColumnGeneric<true, palindex_t, PaletteTranslucentLevelFunc>(FB_LEVELDEST_P, dpcol);
	else
		R_DrawLevelColumnGeneric<false, palindex_t, PaletteTranslucentLevelFunc>(FB_LEVELDEST_P, dpcol);
}

void R_DrawLevelColumnD()
{
	if (dpcol.lightoff == NULL)
		R_DrawLevelColumnGeneric<true, argb_t, DirectLevelFunc>(FB_LEVELDEST_D, dpcol);
	else
		R_DrawLevelColumnGeneric<false, argb_t, DirectLevelFunc>(FB_LEVELDEST_D, dpcol);
}

void R_DrawTranslucentLevelColumnD()
{
	if (dpcol.lightoff == NULL)
		R_DrawLevelColumnGeneric<true, argb_t, DirectTranslucentLevelFunc>(FB_LEVELDEST_D, dpcol);
	else
		R_DrawLevelColumnGeneric<false, argb_t, DirectTranslucentLevelFunc>(FB_LEVELDEST_D, dpcol);
}

//
// R_DrawLevelGroupD_c
//
// Reference implementation of the four-row group kernel, and the fallback when
// no vectorized one is available. Worth having even without SSE2: it lifts the
// double multiplies and the per-y colormap load out of the inner loop, which is
// most of the scalar column drawer's cost.
//
void R_DrawLevelGroupD_c()
{
	const drawplanegroup_t& g = dpgroup;

	const palindex_t* const source = g.source;
	const argb_t* const s0 = g.shade[0];
	const argb_t* const s1 = g.shade[1];
	const argb_t* const s2 = g.shade[2];
	const argb_t* const s3 = g.shade[3];

	const int umask = g.umask, vmask = g.vmask;
	const int ushift = g.ushift, vshift = g.vshift;

	dsfixed_t u0 = g.ufrac[0], u1 = g.ufrac[1], u2 = g.ufrac[2], u3 = g.ufrac[3];
	dsfixed_t v0 = g.vfrac[0], v1 = g.vfrac[1], v2 = g.vfrac[2], v3 = g.vfrac[3];

	argb_t* dest = reinterpret_cast<argb_t*>(g.destination) +
			ptrdiff_t(g.xa) * g.colstep + g.y0;

	for (int n = g.xb - g.xa + 1; n; --n)
	{
		dest[0] = s0[source[((v0 >> vshift) & vmask) | ((u0 >> ushift) & umask)]];
		dest[1] = s1[source[((v1 >> vshift) & vmask) | ((u1 >> ushift) & umask)]];
		dest[2] = s2[source[((v2 >> vshift) & vmask) | ((u2 >> ushift) & umask)]];
		dest[3] = s3[source[((v3 >> vshift) & vmask) | ((u3 >> ushift) & umask)]];

		u0 += g.ustep[0]; u1 += g.ustep[1]; u2 += g.ustep[2]; u3 += g.ustep[3];
		v0 += g.vstep[0]; v1 += g.vstep[1]; v2 += g.vstep[2]; v3 += g.vstep[3];

		dest += g.colstep;
	}
}


//
// R_SelectDrawers
//
// Installs the drawer table.
//
static void R_SelectDrawers()
{
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		R_DrawColumn			= R_DrawColumnP;
		R_DrawFuzzColumn		= R_DrawFuzzColumnP;
		R_DrawTranslucentColumn	= R_DrawTranslucentColumnP;
		R_DrawTranslatedColumn	= R_DrawTranslatedColumnP;
		R_DrawTlatedLucentColumn = R_DrawTlatedLucentColumnP;
		R_DrawSkyForegroundColumn= R_DrawSkyForegroundColumnP;
		R_DrawSlopeSpan			= R_DrawSlopeSpanP;
		R_DrawTranslucentSlopeSpan = R_DrawTranslucentSlopeSpanP;
		R_DrawSpan				= R_DrawSpanP;
		R_DrawTranslucentSpan	= R_DrawTranslucentSpanP;
		R_FillColumn			= R_FillColumnP;
		R_FillSpan				= R_FillSpanP;
		R_FillTranslucentSpan	= R_FillTranslucentSpanP;
		R_DrawLevelColumn		= R_DrawLevelColumnP;
		R_DrawTranslucentLevelColumn = R_DrawTranslucentLevelColumnP;

		// No 8bpp group kernel: sixteen lanes would need sixteen colormap bases
		// and SSE2 has no pinsrb to assemble them. 8bpp level planes go through
		// R_DrawLevelPlaneColumns instead.
		R_DrawLevelGroup		= NULL;
	}
	else
	{
		// 32bpp rendering functions:
		R_DrawColumn			= R_DrawColumnD;
		R_DrawFuzzColumn		= R_DrawFuzzColumnD;
		R_DrawTranslucentColumn	= R_DrawTranslucentColumnD;
		R_DrawTranslatedColumn	= R_DrawTranslatedColumnD;
		R_DrawTlatedLucentColumn = R_DrawTlatedLucentColumnD;
		R_DrawSkyForegroundColumn= R_DrawSkyForegroundColumnD;
		R_DrawTranslucentSlopeSpan = R_DrawTranslucentSlopeSpanD;
		R_DrawTranslucentSpan	= R_DrawTranslucentSpanD;
		R_FillColumn			= R_FillColumnD;
		R_FillSpan				= R_FillSpanD;
		R_FillTranslucentSpan	= R_FillTranslucentSpanD;
		R_DrawLevelColumn		= R_DrawLevelColumnD;
		R_DrawTranslucentLevelColumn = R_DrawTranslucentLevelColumnD;

		// Scalar. The SSE2 span drawers wrote contiguous 4-pixel chunks, which is
		// no use against a column-major buffer, so sloped planes and ARGB-flat
		// level planes store one pixel per cache line. The fix is a column-major
		// sloped rasterizer on R_DrawLevelGroupD_SSE2's plan.
		R_DrawSlopeSpan			= R_DrawSlopeSpanD;
		R_DrawSpan				= R_DrawSpanD;

		// The group kernel's premise: four rows of one column are 16 contiguous
		// bytes.
		R_DrawLevelGroup		= R_DrawLevelGroupD;
	}
}

// [RH] Initialize the column drawer pointers
void R_InitColumnDrawers ()
{
	if (!I_VideoInitialized())
		return;

	R_SelectDrawers();
}

VERSION_CONTROL (r_draw_cpp, "$Id$")
