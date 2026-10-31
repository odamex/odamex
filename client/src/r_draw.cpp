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

#include "r_context.h"

#include <assert.h>
#include <cmath>
#include <algorithm>

#include "i_sdl.h"
#include "r_intrin.h"
#include "r_drawt_sse2.h"
#include "r_drawt_altivec.h"
#include "r_drawt_neon.h"

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
void (*R_DrawColumn)(rendercontext_t&);
void (*R_DrawFuzzColumn)(rendercontext_t&);
void (*R_DrawTranslucentColumn)(rendercontext_t&);
void (*R_DrawTranslatedColumn)(rendercontext_t&);
void (*R_DrawTlatedLucentColumn)(rendercontext_t&);
void (*R_DrawSkyForegroundColumn)(rendercontext_t&);
void (*R_FillColumn)(rendercontext_t&);
void (*R_FillSpan)(rendercontext_t&);
void (*R_FillTranslucentSpan)(rendercontext_t&);

// Possibly vectorized functions:
void (*R_DrawLevelColumn)(rendercontext_t&);
void (*R_DrawTranslucentLevelColumn)(rendercontext_t&);
void (*R_DrawLevelGroup)(rendercontext_t&);

void (*R_DrawLevelGroupD)(rendercontext_t&);
void (*R_DrawSlopeColumn)(rendercontext_t&);
void (*R_DrawTranslucentSlopeColumn)(rendercontext_t&);
void (*r_dimpatchD)(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h);
void (*R_StreamStoredLines8)(argb_t*, const palindex_t*, int, int, int, int, const argb_t*) = nullptr;
void (*R_StreamStoredLines32)(argb_t*, const argb_t*, int, int, int, int) = nullptr;


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
void R_BlankColumn(rendercontext_t& ctx)
{
}

//
// R_BlankSpan
//
// [SL] - Does nothing (obviously). Used when a span drawing function
// pointer should not draw anything.
//
void R_BlankSpan(rendercontext_t& ctx)
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

	if (drawcolumn.masked and drawcolumn.source != nullptr)
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
	if (drawcolumn.x < 0 or drawcolumn.x >= viewwidth or drawcolumn.yl < 0 or drawcolumn.yh >= viewheight)
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
// Texture maps one screen column of a level plane. This is Doom's span mapping
// collected on distance instead of on x, so it needs no divide: the entire dependence on y is one
// yslope[] load.
//
// NOTE: ushift/vshift stay inside the loop deliberately. Folding them into
// ubase/eu would save two ALU ops per pixel but move the double-to-fixed
// truncation from 16.16 granularity to TEXEL granularity, where truncation
// toward zero is floor+1 -- a full one-texel shift for negative coordinates and
// a discontinuity where one crosses zero. Fold them only together with a
// per-plane positivity bias that is an exact multiple of the texture period.
//
// The texel plane a plane column samples: palette indices, or the native ARGB
// plane of a true-colour flat. The color functor takes whichever this is.
template<typename SOURCE_T>
static forceinline const SOURCE_T* R_PlaneColumnSource(const drawplanecol_t& d);

template<>
forceinline const palindex_t* R_PlaneColumnSource<palindex_t>(const drawplanecol_t& d)
{
	return d.source;
}

template<>
forceinline const argb_t* R_PlaneColumnSource<argb_t>(const drawplanecol_t& d)
{
	return d.argbsource;
}

template<bool CONSTLIGHT, typename PIXEL_T, typename COLORFUNC, typename SOURCE_T = palindex_t>
static forceinline void R_DrawLevelColumnGeneric(PIXEL_T* dest, const drawplanecol_t& d)
{
#ifdef RANGECHECK
	if (d.x < 0 or d.x >= viewwidth or d.yl < 0 or d.yh >= viewheight)
	{
		PrintFmt(PRINT_HIGH, "R_DrawLevelColumn: {} to {} at {}\n", d.yl, d.yh, d.x);
		return;
	}
#endif

	int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const int step = 1;		// the contiguous store this path exists for

	const SOURCE_T* const source = R_PlaneColumnSource<SOURCE_T>(d);

	// Both streamed linearly down the column, so the prefetcher has them.
	const fixed_t* ys = yslope.get() + d.yl;
	const uint16_t* off = CONSTLIGHT ? nullptr : d.lightoff + d.yl;	// lightoff is NULL for CONSTLIGHT

	const double ubase = d.ubase, vbase = d.vbase;
	const double eu = d.eu, ev = d.ev;
	const int umask = d.umask, vmask = d.vmask;
	const int ushift = d.ushift, vshift = d.vshift;

	COLORFUNC colorfunc(d);

	do {
		// int64_t is what keeps an out-of-range double from being undefined,
		// while keeping the low-32-bit wrap of the fixed-point pipeline.
		const double Y = static_cast<double>(*ys++);
		const dsfixed_t ufrac = static_cast<dsfixed_t>(static_cast<int64_t>(ubase + (eu * Y)));
		const dsfixed_t vfrac = static_cast<dsfixed_t>(static_cast<int64_t>(vbase + (ev * Y)));

		const unsigned int spot = ((vfrac >> vshift) & vmask) | ((ufrac >> ushift) & umask);

		colorfunc(source[spot], CONSTLIGHT ? 0u : *off, dest);
		if (not CONSTLIGHT)
			off++;
		dest += step;
	} while (--count);
}

//
// R_DrawSlopeColumnGeneric
//
// Texture maps one screen column of a sloped plane with an exact perspective
// divide at every pixel. iu/iv/id are affine in screen y as they are in x, so
// each pixel's are the column's top values plus a multiple of a per-row step.
//
// Set by R_InitVectorizedDrawers when r_optimize selects a vector stepper.
static bool have_vector_slopes = false;

#if defined(__SSE2__)
template<bool SLOPELIGHT> using SlopeColumnVector = SlopeColumnSSE2<SLOPELIGHT>;
#define R_HAVE_SLOPE_COLUMN_VECTOR
#elif defined(__ALTIVEC__)
template<bool SLOPELIGHT> using SlopeColumnVector = SlopeColumnAltivec<SLOPELIGHT>;
#define R_HAVE_SLOPE_COLUMN_VECTOR
#elif defined(R_HAVE_NEON_SLOPES)
template<bool SLOPELIGHT> using SlopeColumnVector = SlopeColumnNEON<SLOPELIGHT>;
#define R_HAVE_SLOPE_COLUMN_VECTOR
#endif

template<bool SLOPELIGHT, typename PIXEL_T, typename COLORFUNC, typename SOURCE_T = palindex_t>
static forceinline void R_DrawSlopeColumnGeneric(PIXEL_T* dest, const drawplanecol_t& d)
{
#ifdef RANGECHECK
	if (d.x < 0 or d.x >= viewwidth or d.yl < 0 or d.yh >= viewheight)
	{
		PrintFmt(PRINT_HIGH, "R_DrawSlopeColumn: {} to {} at {}\n", d.yl, d.yh, d.x);
		return;
	}
#endif

	const int count = d.yh - d.yl + 1;
	if (count <= 0)
		return;

	const SOURCE_T* const source = R_PlaneColumnSource<SOURCE_T>(d);

	COLORFUNC colorfunc(d);

#ifdef R_HAVE_SLOPE_COLUMN_VECTOR
	if (have_vector_slopes)
	{
		SlopeColumnVector<SLOPELIGHT> quad(d);
		alignas(16) uint32_t spots[4];
		alignas(16) uint32_t lights[4];

		for (int i = 0; i < count; i += 4)
		{
			quad.next(spots, lights);

			const int n = count - i < 4 ? count - i : 4;
			for (int k = 0; k < n; k++)
				colorfunc(source[spots[k]], SLOPELIGHT ? lights[k] : 0u, dest++);
		}

		return;
	}
#endif

	const float iu = d.iu, iv = d.iv, id = d.id;
	const float ius = d.iustep, ivs = d.ivstep, ids = d.idstep;

	const int umask = d.umask, vmask = d.vmask;
	const int ushift = d.ushift, vshift = d.vshift;

	// Wrapping, as the integer add down the column would.
	uint32_t lmap = static_cast<uint32_t>(d.lmap);
	const uint32_t lstep = static_cast<uint32_t>(d.lmapstep);

	static constexpr int RUN = 16;
	unsigned int spots[RUN];
	unsigned int lights[RUN];

	for (int i0 = 0; i0 < count; i0 += RUN)
	{
		const int n = count - i0 < RUN ? count - i0 : RUN;

		for (int k = 0; k < n; k++)
		{
			// From the column's top rather than accumulated, so a long column
			// cannot drift.
			const float fi = static_cast<float>(i0 + k);
			const float mul = 65536.0f / (id + (ids * fi));
			const fixed_t ufrac = static_cast<fixed_t>((iu + (ius * fi)) * mul);
			const fixed_t vfrac = static_cast<fixed_t>((iv + (ivs * fi)) * mul);
			spots[k] = ((ufrac >> ushift) & umask) | ((vfrac >> vshift) & vmask);
		}

		// The colormap selection, stepping down the column: +1 and extralight
		// are in lbias, and a negative index means the base colormap.
		if (SLOPELIGHT)
		{
			for (int k = 0; k < n; k++)
			{
				const int index = std::clamp((static_cast<fixed_t>(lmap) >> FRACBITS) + d.lbias, 0, NUMCOLORMAPS - 1);
				lights[k] = static_cast<unsigned int>(index) << 8;
				lmap += lstep;
			}
		}

		for (int k = 0; k < n; k++)
			colorfunc(source[spots[k]], SLOPELIGHT ? lights[k] : 0u, dest++);
	}
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
	if (drawspan.x2 < drawspan.x1 or drawspan.x1 < 0 or drawspan.x2 >= viewwidth or
		drawspan.y >= viewheight or drawspan.y < 0)
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
	PaletteFuzzyFunc(const drawcolumn_t& drawcolumn) :
			colormap(&V_GetDefaultPalette()->maps, 6),
			fuzz(drawcolumn.fuzz), pitch(drawcolumn.pitch_in_pixels) { }

	forceinline void operator()(byte c, palindex_t* dest) const
	{
		*dest = colormap.index(dest[fuzz->getValue(pitch)]);
		fuzz->incrementRow();
	}

private:
	shaderef_t  colormap;
	FuzzTable*  fuzz;
	int         pitch;
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

class PaletteSkyForegroundColormapFunc
{
public:
	PaletteSkyForegroundColormapFunc(const drawcolumn_t& drawcolumn) : colormap(drawcolumn.colormap) {}

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
static forceinline ptrdiff_t R_ColumnOffset(const drawcolumn_t& dcol)
{
	return (ptrdiff_t(dcol.x) * dcol.colstep) + dcol.yl;
}

static forceinline ptrdiff_t R_SpanOffset(const drawspan_t& dspan)
{
	return (ptrdiff_t(dspan.x1) * dspan.colstep) + dspan.y;
}

// ----------------------------------------------------------------------------
//
// 8bpp color column drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_P (static_cast<palindex_t*>(dcol.destination) + R_ColumnOffset(dcol))

//
// R_FillColumnP
//
// Fills a column in the 8bpp palettized screen buffer with a solid color,
// determined by dcol.color. Performs no shading.
//
void R_FillColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	R_FillColumnGeneric<palindex_t, PaletteFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawColumnP
//
// Renders a column to the 8bpp palettized screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
//
void R_DrawColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	R_DrawColumnGeneric<palindex_t, PaletteColormapFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawFuzzColumnP
//
// Alters a column in the 8bpp palettized screen buffer using Doom's partial
// invisibility effect, which shades the column and rearranges the ordering
// the pixels to create distortion. Shading is performed using colormap 6.
//
void R_DrawFuzzColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	// adjust the borders (prevent buffer over/under-reads)
	if (dcol.yl <= 0)
		dcol.yl = 1;
	if (dcol.yh >= viewheight - 1)
		dcol.yh = viewheight - 2;

	R_FillColumnGeneric<palindex_t, PaletteFuzzyFunc>(FB_COLDEST_P, dcol);
	ctx.draw.fuzztable.incrementColumn();
}

//
// R_DrawTranslucentColumnP
//
// Renders a translucent column to the 8bpp palettized screen buffer from the
// source buffer dcol.source and scaled by dcol.iscale. The amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTranslucentColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	R_DrawColumnGeneric<palindex_t, PaletteTranslucentColormapFunc>(FB_COLDEST_P, dcol);
}

//
// R_DrawTranslatedColumnP
//
// Renders a column to the 8bpp palettized screen buffer with color-remapping
// from the source buffer dcol.source and scaled by dcol.iscale. The translation
// table is supplied by dcol.translation. Shading is performed using dcol.colormap.
//
void R_DrawTranslatedColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

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
void R_DrawTlatedLucentColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

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
void R_DrawSkyForegroundColumnP(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	R_DrawColumnGeneric<palindex_t, PaletteSkyForegroundColormapFunc>(FB_COLDEST_P, dcol);
}

// ----------------------------------------------------------------------------
//
// 8bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_P (dspan.destination + R_SpanOffset(dspan))

//
// R_FillSpanP
//
// Fills a span in the 8bpp palettized screen buffer with a solid color,
// determined by dspan.color. Performs no shading.
//
void R_FillSpanP(rendercontext_t& ctx)
{
	drawspan_t& dspan = ctx.draw.dspan;

	R_FillSpanGeneric<palindex_t, PaletteFunc>(FB_SPANDEST_P, dspan);
}

//
// R_FillTranslucentSpanP
//
// Fills a span in the 8bpp palettized screen buffer with a solid color,
// determined by dspan.color using translucency. Shading is performed
// using dspan.colormap.
//
void R_FillTranslucentSpanP(rendercontext_t& ctx)
{
	drawspan_t& dspan = ctx.draw.dspan;

	R_FillSpanGeneric<palindex_t, PaletteTranslucentColormapFunc>(FB_SPANDEST_P, dspan);
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
	DirectFunc(const drawcolumn_t& drawcolumn) : basecolormap(drawcolumn.basecolormap) { }
	DirectFunc(const drawspan_t& drawspan) : basecolormap(drawspan.basecolormap) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		*dest = basecolormap->shade(c);
	}

private:
	const shaderef_t* basecolormap;
};

class DirectColormapFunc
{
public:
	DirectColormapFunc(const drawcolumn_t& drawcolumn) :
			colormap(drawcolumn.colormap) { }

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
	DirectFuzzyFunc(const drawcolumn_t& drawcolumn) :
			fuzz(drawcolumn.fuzz), pitch(drawcolumn.pitch_in_pixels) { }

	forceinline void operator()(byte c, argb_t* dest) const
	{
		const argb_t work = dest[fuzz->getValue(pitch)];
		*dest = work - ((work >> 2) & 0x3f3f3f);
		fuzz->incrementRow();
	}

private:
	FuzzTable*  fuzz;
	int         pitch;
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

class DirectSkyForegroundColormapFunc
{
public:
	DirectSkyForegroundColormapFunc(const drawcolumn_t& drawcolumn) : colormap(drawcolumn.colormap) {}

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
// For each light diminishing level, three 256-entry tables map a channel value
// to its diminished, faded and gamma-corrected result -- the shademap's math
// applied to a true color. Every dynamic colormap owns a set built from its
// light and fade color, so a texel costs three lookups whatever lights it.
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

void R_BuildARGBShadeLUT(argbshadelut_t* lut, const argb_t color, const argb_t fade)
{
	for (int m = 0; m <= NUMCOLORMAPS; m++)
	{
		// The expression shaderef_t::shadeargb evaluates per texel.
		const int scale_r = color.getr() * (NUMCOLORMAPS - m);
		const int scale_g = color.getg() * (NUMCOLORMAPS - m);
		const int scale_b = color.getb() * (NUMCOLORMAPS - m);

		const int add_r = (fade.getr() * m) + (NUMCOLORMAPS / 2);
		const int add_g = (fade.getg() * m) + (NUMCOLORMAPS / 2);
		const int add_b = (fade.getb() * m) + (NUMCOLORMAPS / 2);

		for (int c = 0; c < 256; c++)
		{
			lut->r[m][c] = gammatable[((c * scale_r / 255) + add_r) / NUMCOLORMAPS];
			lut->g[m][c] = gammatable[((c * scale_g / 255) + add_g) / NUMCOLORMAPS];
			lut->b[m][c] = gammatable[((c * scale_b / 255) + add_b) / NUMCOLORMAPS];
		}
	}
}

argbshadelut_t* R_NewARGBShadeLUT(const argb_t color, const argb_t fade)
{
	argbshadelut_t* lut = Z_Malloc<argbshadelut_t>(PU_LEVEL);
	R_BuildARGBShadeLUT(lut, color, fade);
	return lut;
}

// White light under the level fade, which shades the default palette's maps
// (no dynamic colormap) and NormalLight.
static argbshadelut_t argb_default_lut;

argbshadelut_t* R_DefaultARGBShadeLUT()
{
	return &argb_default_lut;
}

// Rebuild the default tables (new map or gamma bump).
void R_UpdateARGBShadeLUT()
{
	const argb_t fadecolor(level.fadeto_color[0], level.fadeto_color[1],
	                       level.fadeto_color[2], level.fadeto_color[3]);

	R_BuildARGBShadeLUT(&argb_default_lut, argb_t(255, 255, 255, 255), fadecolor);
}

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

		// Every dynamic colormap gets its tables where it is made (GetSpecialLights,
		// V_RefreshColormaps), so the default is only reached without one.
		const argbshadelut_t* lut = &argb_default_lut;
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

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = std::clamp(static_cast<int>((translevel & ~0x03FF) >> 8), 0, 255);
	}

	const ARGBShader shader;
	int fga;
};

//
// True color plane columns
//
// A plane column crosses light bands, so instead of one shader per span it
// carries a table of them, one per band, indexed by the same relative offset the
// palettized columns add to cbase.
//
// R_PrepareARGBPlaneShading fills it once per plane, and only for planes with an
// ARGB flat, so ordinary planes never pay the full cost.
//
static bool argb_plane_drawers = false;

static thread_local ARGBShader argb_plane_shaders[NUMCOLORMAPS];

bool R_PrepareARGBPlaneShading(rendercontext_t& ctx, const shaderef_t& light)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	// The span drawers' test: a special colormap (invulnerability) has no RGB
	// equivalent and stays palettized.
	if (not argb_plane_drawers or dpcol.argbsource == nullptr or light.mapnum() >= NUMCOLORMAPS)
	{
		dpcol.argbshade = nullptr;
		return false;
	}

	for (int rel = 0; rel < NUMCOLORMAPS; rel++)
		argb_plane_shaders[rel] = ARGBShader(light.m_dyncolormap, light.mapnum() + rel);

	dpcol.argbshade = argb_plane_shaders;
	return true;
}

//
// R_ShadeOpaqueARGB
//
// One texel of an opaque true-color surface: a plane has no backdrop, so a
// translucent texel composites onto black, as DirectARGBColormapFunc does for
// spans.
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
// DirectARGBLevelFunc
//
// DirectARGBColormapFunc for a plane column: the shader comes from the band
// table, and a plane has no backdrop, so translucent texels composite onto black.
//
class DirectARGBLevelFunc
{
public:
	DirectARGBLevelFunc(const drawplanecol_t& d) : shaders(d.argbshade) { }

	forceinline void operator()(argb_t c, unsigned int lightoff, argb_t* dest) const
	{
		*dest = R_ShadeOpaqueARGB(shaders[lightoff >> 8], c);
	}

private:
	const ARGBShader* shaders;
};


// ----------------------------------------------------------------------------
//
// 32bpp color drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_D (reinterpret_cast<argb_t*>(dcol.destination) + R_ColumnOffset(dcol))

//
// R_ColumnHasNativeARGB
//
// Returns true when the current column should be drawn from its
// texture's native ARGB plane: the plane must exist, and special colormaps
// (e.g. invulnerability) have no RGB equivalent so those columns stay on
// the palettized path.
//
static forceinline bool R_ColumnHasNativeARGB(const drawcolumn_t& dcol)
{
	return dcol.argbtexturedata != nullptr and dcol.source != nullptr and
	       dcol.colormap.mapnum() < NUMCOLORMAPS;
}

// The four-pixel column drawers r_optimize selected, or NULL where it has none.
// Set by R_InitVectorizedDrawers, which runs long before any column is drawn.
static void (*R_DrawColumnQuadD)(rendercontext_t&) = nullptr;
static void (*R_DrawTranslucentColumnQuadD)(rendercontext_t&) = nullptr;

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
static forceinline bool R_ColumnWantsQuad(void (*drawer)(rendercontext_t&),
                                          const drawcolumn_t& dcol)
{
	return drawer != nullptr and
			dcol.yh - dcol.yl + 1 >= R_QUAD_MIN_COLUMN and
			(dcol.masked or
				(dcol.textureheight & (dcol.textureheight - 1)) == 0);
}

//
// R_FillColumnD
//
// Fills a column in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dcol.color. Performs no shading.
//
void R_FillColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	R_FillColumnGeneric<argb_t, DirectFunc>(FB_COLDEST_D, dcol);
}

//
// R_DrawColumnD
//
// Renders a column to the 32bpp ARGB8888 screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Shading is performed using dcol.colormap.
//
void R_DrawColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	if (R_ColumnHasNativeARGB(dcol))
		R_DrawColumnGenericARGB<DirectARGBColormapFunc>(FB_COLDEST_D, dcol);
	// Worth having despite costing slightly MORE instructions than the scalar
	// drawer -- 12.25 per pixel against 12 for SSE2. An opaque column is one
	// gather and one store per pixel and SSE2 cannot vectorize a gather, so the
	// arithmetic says this should be a wash.
	//
	// It is not: the quad form issues ONE 16-byte store where the scalar issues four,
	// and this loop is limited by store-port throughput rather than by issue rate.
	else if (R_ColumnWantsQuad(R_DrawColumnQuadD, dcol))
		R_DrawColumnQuadD(ctx);
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
void R_DrawFuzzColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	// adjust the borders (prevent buffer over/under-reads)
	if (dcol.yl <= 0)
		dcol.yl = 1;
	if (dcol.yh >= viewheight - 1)
		dcol.yh = viewheight - 2;

	R_FillColumnGeneric<argb_t, DirectFuzzyFunc>(FB_COLDEST_D, dcol);
	ctx.draw.fuzztable.incrementColumn();
}

//
// R_DrawTranslucentColumnD
//
// Renders a translucent column to the 32bpp ARGB8888 screen buffer from the
// source buffer dcol.source and scaled by dcol.iscale. The amount of
// translucency is controlled by dcol.translevel. Shading is performed using
// dcol.colormap.
//
void R_DrawTranslucentColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	if (R_ColumnHasNativeARGB(dcol))
		R_DrawColumnGenericARGB<DirectARGBTranslucentColormapFunc>(FB_COLDEST_D, dcol);
	else if (R_ColumnWantsQuad(R_DrawTranslucentColumnQuadD, dcol))
		R_DrawTranslucentColumnQuadD(ctx);
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
void R_DrawTranslatedColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

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
void R_DrawTlatedLucentColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

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
void R_DrawSkyForegroundColumnD(rendercontext_t& ctx)
{
	drawcolumn_t& dcol = ctx.draw.dcol;

	R_DrawColumnGeneric<argb_t, DirectSkyForegroundColormapFunc>(FB_COLDEST_D, dcol);
}

// ----------------------------------------------------------------------------
//
// 32bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_D (reinterpret_cast<argb_t*>(dspan.destination) + R_SpanOffset(dspan))

//
// R_FillSpanD
//
// Fills a span in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dspan.color. Performs no shading.
//
void R_FillSpanD(rendercontext_t& ctx)
{
	drawspan_t& dspan = ctx.draw.dspan;

	R_FillSpanGeneric<argb_t, DirectFunc>(FB_SPANDEST_D, dspan);
}

//
// R_FillTranslucentSpanD
//
// Fills a span in the 32bpp ARGB8888 screen buffer with a solid color,
// determined by dspan.color using translucency. Shading is performed
// using dspan.colormap.
//
void R_FillTranslucentSpanD(rendercontext_t& ctx)
{
	drawspan_t& dspan = ctx.draw.dspan;

	R_FillSpanGeneric<argb_t, DirectTranslucentColormapFunc>(FB_SPANDEST_D, dspan);
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
	OPTIMIZE_ALTIVEC,
	OPTIMIZE_NEON
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
		case OPTIMIZE_NEON:    return "neon";
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
	#ifdef __ARM_NEON
	if (SDL_HasNEON())
		optimizations_available.push_back(OPTIMIZE_NEON);
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
	else if (stricmp(val, "neon") == 0 and R_IsOptimizationAvailable(OPTIMIZE_NEON))
		optimize_kind = OPTIMIZE_NEON;
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
	R_DrawLevelGroupD       = R_DrawLevelGroupD_c;
	R_DrawColumnQuadD       = nullptr;
	R_DrawTranslucentColumnQuadD = nullptr;
	have_vector_slopes      = false;
	r_dimpatchD             = r_dimpatchD_c;
	R_StreamStoredLines8    = nullptr;
	R_StreamStoredLines32   = nullptr;

	#ifdef __SSE2__
	if (optimize_kind == OPTIMIZE_SSE2)
	{
		R_DrawLevelGroupD       = R_DrawLevelGroupD_SSE2;
		R_DrawColumnQuadD       = R_DrawColumnD_SSE2;
		R_DrawTranslucentColumnQuadD = R_DrawTranslucentColumnD_SSE2;
		have_vector_slopes      = true;
		r_dimpatchD             = r_dimpatchD_SSE2;
		R_StreamStoredLines8    = R_StreamStoredLines8_SSE2;
		R_StreamStoredLines32   = R_StreamStoredLines32_SSE2;
	}
	#endif
	#ifdef __MMX__
	if (optimize_kind == OPTIMIZE_MMX)
	{
		R_DrawLevelGroupD       = R_DrawLevelGroupD_MMX;
		R_DrawColumnQuadD       = R_DrawColumnD_MMX;
		R_DrawTranslucentColumnQuadD = R_DrawTranslucentColumnD_MMX;
		r_dimpatchD             = r_dimpatchD_MMX;
	}
	#endif
	#ifdef __ALTIVEC__
	if (optimize_kind == OPTIMIZE_ALTIVEC)
	{
		R_DrawLevelGroupD       = R_DrawLevelGroupD_ALTIVEC;
		R_DrawTranslucentColumnQuadD = R_DrawTranslucentColumnD_ALTIVEC;
		have_vector_slopes      = true;
		r_dimpatchD             = r_dimpatchD_ALTIVEC;
		R_StreamStoredLines32   = R_StreamStoredLines32_ALTIVEC;
	}
	#endif
	#ifdef __ARM_NEON
	if (optimize_kind == OPTIMIZE_NEON)
	{
		R_DrawLevelGroupD       = R_DrawLevelGroupD_NEON;
		R_DrawColumnQuadD       = R_DrawColumnD_NEON;
		R_DrawTranslucentColumnQuadD = R_DrawTranslucentColumnD_NEON;
		#ifdef R_HAVE_NEON_SLOPES
		have_vector_slopes      = true;
		#endif
		r_dimpatchD             = r_dimpatchD_NEON;
		R_StreamStoredLines8    = R_StreamStoredLines8_NEON;
		R_StreamStoredLines32   = R_StreamStoredLines32_NEON;
	}
	#endif

	// Independent ifs, not an else-if chain: optimize_kind already makes them
	// exclusive, and chaining would leave a dangling else on any build without
	// __SSE2__

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
void R_BindViewBuffer(rendercontext_t& ctx, IWindowSurface* surface)
{
	drawcolumn_t&   dcol  = ctx.draw.dcol;
	drawspan_t&     dspan = ctx.draw.dspan;
	drawplanecol_t& dpcol = ctx.draw.dpcol;

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
#define FB_LEVELDEST_P (static_cast<palindex_t*>(dpcol.destination) + R_LevelColumnOffset(dpcol))
#define FB_LEVELDEST_D (reinterpret_cast<argb_t*>(dpcol.destination) + R_LevelColumnOffset(dpcol))

static forceinline ptrdiff_t R_LevelColumnOffset(const drawplanecol_t& dpcol)
{
	return (ptrdiff_t(dpcol.x) * dpcol.colstep) + dpcol.yl;
}

void R_DrawLevelColumnP(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.lightoff == nullptr)
		R_DrawLevelColumnGeneric<true, palindex_t, PaletteLevelFunc>(FB_LEVELDEST_P, dpcol);
	else
		R_DrawLevelColumnGeneric<false, palindex_t, PaletteLevelFunc>(FB_LEVELDEST_P, dpcol);
}

void R_DrawTranslucentLevelColumnP(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.lightoff == nullptr)
		R_DrawLevelColumnGeneric<true, palindex_t, PaletteTranslucentLevelFunc>(FB_LEVELDEST_P, dpcol);
	else
		R_DrawLevelColumnGeneric<false, palindex_t, PaletteTranslucentLevelFunc>(FB_LEVELDEST_P, dpcol);
}

void R_DrawLevelColumnD(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.argbshade != nullptr)
	{
		if (dpcol.lightoff == nullptr)
			R_DrawLevelColumnGeneric<true, argb_t, DirectARGBLevelFunc, argb_t>(FB_LEVELDEST_D, dpcol);
		else
			R_DrawLevelColumnGeneric<false, argb_t, DirectARGBLevelFunc, argb_t>(FB_LEVELDEST_D, dpcol);
	}
	else if (dpcol.lightoff == nullptr)
		R_DrawLevelColumnGeneric<true, argb_t, DirectLevelFunc>(FB_LEVELDEST_D, dpcol);
	else
		R_DrawLevelColumnGeneric<false, argb_t, DirectLevelFunc>(FB_LEVELDEST_D, dpcol);
}

void R_DrawTranslucentLevelColumnD(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.lightoff == nullptr)
		R_DrawLevelColumnGeneric<true, argb_t, DirectTranslucentLevelFunc>(FB_LEVELDEST_D, dpcol);
	else
		R_DrawLevelColumnGeneric<false, argb_t, DirectTranslucentLevelFunc>(FB_LEVELDEST_D, dpcol);
}

//
// Sloped plane column drawing wrappers
//
// slopelight is false when the plane resolves to one colormap -- fixedlightlev
// or an active fixedcolormap -- and cbase/sbase already point at it.
//
void R_DrawSlopeColumnP(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.slopelight)
		R_DrawSlopeColumnGeneric<true, palindex_t, PaletteLevelFunc>(FB_LEVELDEST_P, dpcol);
	else
		R_DrawSlopeColumnGeneric<false, palindex_t, PaletteLevelFunc>(FB_LEVELDEST_P, dpcol);
}

void R_DrawTranslucentSlopeColumnP(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.slopelight)
		R_DrawSlopeColumnGeneric<true, palindex_t, PaletteTranslucentLevelFunc>(FB_LEVELDEST_P, dpcol);
	else
		R_DrawSlopeColumnGeneric<false, palindex_t, PaletteTranslucentLevelFunc>(FB_LEVELDEST_P, dpcol);
}

void R_DrawSlopeColumnD(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.argbshade != nullptr)
	{
		if (dpcol.slopelight)
			R_DrawSlopeColumnGeneric<true, argb_t, DirectARGBLevelFunc, argb_t>(FB_LEVELDEST_D, dpcol);
		else
			R_DrawSlopeColumnGeneric<false, argb_t, DirectARGBLevelFunc, argb_t>(FB_LEVELDEST_D, dpcol);
	}
	else if (dpcol.slopelight)
		R_DrawSlopeColumnGeneric<true, argb_t, DirectLevelFunc>(FB_LEVELDEST_D, dpcol);
	else
		R_DrawSlopeColumnGeneric<false, argb_t, DirectLevelFunc>(FB_LEVELDEST_D, dpcol);
}

void R_DrawTranslucentSlopeColumnD(rendercontext_t& ctx)
{
	drawplanecol_t& dpcol = ctx.draw.dpcol;

	if (dpcol.slopelight)
		R_DrawSlopeColumnGeneric<true, argb_t, DirectTranslucentLevelFunc>(FB_LEVELDEST_D, dpcol);
	else
		R_DrawSlopeColumnGeneric<false, argb_t, DirectTranslucentLevelFunc>(FB_LEVELDEST_D, dpcol);
}

//
// R_DrawLevelGroupD_c
//
// Reference implementation of the four-row group kernel, and the fallback when
// no vectorized one is available. Worth having even without SSE2: it lifts the
// double multiplies and the per-y colormap load out of the inner loop, which is
// most of the scalar column drawer's cost.
//
void R_DrawLevelGroupD_c(rendercontext_t& ctx)
{
	drawplanegroup_t& dpgroup = ctx.draw.dpgroup;

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
			(ptrdiff_t(g.xa) * g.colstep) + g.y0;

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
// R_DrawLevelGroupARGBD_c
//
// R_DrawLevelGroupD_c for a true-color flat: the same four-row DDA, sampling the
// ARGB plane and shading each lane with its band's shader, which is as
// loop-invariant as shade[i] is there.
//
void R_DrawLevelGroupARGBD_c(rendercontext_t& ctx)
{
	const drawplanegroup_t& g = ctx.draw.dpgroup;

	const argb_t* const source = g.argbsource;

	// By value, like the span functor's shader: through a pointer, every pixel
	// would reload its tables from memory.
	const ARGBShader s0 = g.argbshade[g.argbband[0]];
	const ARGBShader s1 = g.argbshade[g.argbband[1]];
	const ARGBShader s2 = g.argbshade[g.argbband[2]];
	const ARGBShader s3 = g.argbshade[g.argbband[3]];

	const int umask = g.umask, vmask = g.vmask;
	const int ushift = g.ushift, vshift = g.vshift;

	dsfixed_t u0 = g.ufrac[0], u1 = g.ufrac[1], u2 = g.ufrac[2], u3 = g.ufrac[3];
	dsfixed_t v0 = g.vfrac[0], v1 = g.vfrac[1], v2 = g.vfrac[2], v3 = g.vfrac[3];

	argb_t* dest = reinterpret_cast<argb_t*>(g.destination) +
			(ptrdiff_t(g.xa) * g.colstep) + g.y0;

	for (int n = g.xb - g.xa + 1; n; --n)
	{
		dest[0] = R_ShadeOpaqueARGB(s0, source[((v0 >> vshift) & vmask) | ((u0 >> ushift) & umask)]);
		dest[1] = R_ShadeOpaqueARGB(s1, source[((v1 >> vshift) & vmask) | ((u1 >> ushift) & umask)]);
		dest[2] = R_ShadeOpaqueARGB(s2, source[((v2 >> vshift) & vmask) | ((u2 >> ushift) & umask)]);
		dest[3] = R_ShadeOpaqueARGB(s3, source[((v3 >> vshift) & vmask) | ((u3 >> ushift) & umask)]);

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
		R_FillColumn			= R_FillColumnP;
		R_FillSpan				= R_FillSpanP;
		R_FillTranslucentSpan	= R_FillTranslucentSpanP;
		R_DrawLevelColumn		= R_DrawLevelColumnP;
		R_DrawTranslucentLevelColumn = R_DrawTranslucentLevelColumnP;
		R_DrawSlopeColumn		= R_DrawSlopeColumnP;
		R_DrawTranslucentSlopeColumn = R_DrawTranslucentSlopeColumnP;
		argb_plane_drawers		= false;

		// No 8bpp group kernel: sixteen lanes would need sixteen colormap bases
		// and SSE2 has no pinsrb to assemble them. 8bpp level planes go through
		// R_DrawLevelPlaneColumns instead.
		R_DrawLevelGroup		= nullptr;
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
		R_FillColumn			= R_FillColumnD;
		R_FillSpan				= R_FillSpanD;
		R_FillTranslucentSpan	= R_FillTranslucentSpanD;
		R_DrawLevelColumn		= R_DrawLevelColumnD;
		R_DrawTranslucentLevelColumn = R_DrawTranslucentLevelColumnD;
		R_DrawSlopeColumn		= R_DrawSlopeColumnD;
		R_DrawTranslucentSlopeColumn = R_DrawTranslucentSlopeColumnD;
		argb_plane_drawers		= true;

		// The group kernel's premise: four rows of one column are 16 contiguous
		// bytes.
		R_DrawLevelGroup		= R_DrawLevelGroupD;
	}
}

// [RH] Initialize the column drawer pointers
void R_InitColumnDrawers ()
{
	if (not I_VideoInitialized())
		return;

	R_SelectDrawers();
}

VERSION_CONTROL (r_draw_cpp, "$Id$")
