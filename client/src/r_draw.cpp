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
//		The actual span/column drawing functions.
//		Here find the main potential for optimization,
//		 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

#include <stddef.h>
#include <assert.h>
#include <algorithm>

#include "i_sdl.h"
#include "r_intrin.h"

#include "m_alloc.h"
#include "doomdef.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "r_local.h"
#include "i_video.h"
#include "v_video.h"
#include "doomstat.h"
#include "st_stuff.h"

#include "gi.h"
#include "v_text.h"

#undef RANGECHECK

// status bar height at bottom of screen
// [RH] status bar position at bottom of screen
extern	int		ST_Y;

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
void (*R_DrawSpan)(void);
void (*R_DrawSlopeSpan)(void);
void (*R_FillColumn)(void);
void (*R_FillSpan)(void);
void (*R_FillTranslucentSpan)(void);

// Possibly vectorized functions:
void (*R_DrawSpanD)(void);
void (*R_DrawSlopeSpanD)(void);
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
		int pitch = R_GetRenderingSurface()->getPitchInPixels();
		int value = table[pos];
		return (pitch ^ value) + value;
	}

private:
	static const size_t size = 64;
	static const int table[FuzzTable::size];
	int pos;
};

const int FuzzTable::table[FuzzTable::size] = {
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

byte* translationtables;
argb_t translationRGB[MAXPLAYERS+1][16];
byte *Ranges;
static byte *translationtablesmem = NULL;


static void R_BuildFontTranslation(int color_num, argb_t start_color, argb_t end_color)
{
	const palindex_t start_index = 0xB0;
	const palindex_t end_index = 0xBF;
	const int index_range = end_index - start_index + 1;

	palindex_t* dest = (palindex_t*)Ranges + color_num * 256;

	for (int index = 0; index < start_index; index++)
		dest[index] = index;
	for (int index = end_index + 1; index < 256; index++)
		dest[index] = index;	

	int r_diff = end_color.getr() - start_color.getr();
	int g_diff = end_color.getg() - start_color.getg();
	int b_diff = end_color.getb() - start_color.getb();

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
	
	translationtablesmem = new byte[256*(MAXPLAYERS+3+22)+255]; // denis - fixme - magic numbers?

	// [Toke - fix13]
	// denis - cleaned this up somewhat
	translationtables = (byte *)(((ptrdiff_t)translationtablesmem + 255) & ~255);
	
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
    delete[] translationtablesmem;
    translationtablesmem = NULL;
}

// [Nes] Vanilla player translation table.
void R_BuildClassicPlayerTranslation (int player, int color)
{
	const palette_t* pal = V_GetDefaultPalette();
	int i;
	
	if (color == 1) // Indigo
		for (i = 0x70; i < 0x80; i++)
		{
			translationtables[i+(player * 256)] = 0x60 + (i&0xf);
			translationRGB[player][i - 0x70] = pal->basecolors[translationtables[i+(player * 256)]];
		}
	else if (color == 2) // Brown
		for (i = 0x70; i < 0x80; i++)
		{
			translationtables[i+(player * 256)] = 0x40 + (i&0xf);	
			translationRGB[player][i - 0x70] = pal->basecolors[translationtables[i+(player * 256)]];
		}
	else if (color == 3) // Red
		for (i = 0x70; i < 0x80; i++)
		{
			translationtables[i+(player * 256)] = 0x20 + (i&0xf);	
			translationRGB[player][i - 0x70] = pal->basecolors[translationtables[i+(player * 256)]];
		}
}

void R_CopyTranslationRGB (int fromplayer, int toplayer)
{
	for (int i = 0x70; i < 0x80; ++i)
	{
		translationRGB[toplayer][i - 0x70] = translationRGB[fromplayer][i - 0x70];
		translationtables[i+(toplayer * 256)] = translationtables[i+(fromplayer * 256)];
	}
}

// [RH] Create a player's translation table based on
//		a given mid-range color.
void R_BuildPlayerTranslation(int player, argb_t dest_color)
{
	const palette_t* pal = V_GetDefaultPalette();
	byte* table = &translationtables[player * 256];

	fahsv_t hsv_temp = V_RGBtoHSV(dest_color);
	float h = hsv_temp.geth(), s = hsv_temp.gets(), v = hsv_temp.getv();

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
		argb_t color(V_HSVtoRGB(fahsv_t(h, s, v)));

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
		Printf (PRINT_HIGH, "R_FillColumn: %i to %i at %i\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
		return;
	}
#endif

	int color = drawcolumn.color;
	int pitch = drawcolumn.pitch_in_pixels;
	int count = drawcolumn.yh - drawcolumn.yl + 1;
	if (count <= 0)
		return;

	COLORFUNC colorfunc(drawcolumn);

	do {
		colorfunc(color, dest);
		dest += pitch;
	} while (--count);
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
		Printf (PRINT_HIGH, "R_DrawColumn: %i to %i at %i\n", drawcolumn.yl, drawcolumn.yh, drawcolumn.x);
		return;
	}
#endif

	palindex_t* source = drawcolumn.source;
	int pitch = drawcolumn.pitch_in_pixels;
	int count = drawcolumn.yh - drawcolumn.yl + 1;
	if (count <= 0)
		return;

	const fixed_t fracstep = drawcolumn.iscale; 
	fixed_t frac = drawcolumn.texturefrac;

	const int texheight = drawcolumn.textureheight;
	const int mask = (texheight >> FRACBITS) - 1;

	COLORFUNC colorfunc(drawcolumn);

	// [SL] Properly tile textures whose heights are not a power-of-2,
	// avoiding a tutti-frutti effect.  From Eternity Engine.
	if (texheight & (texheight - 1))
	{
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

		if (count & 1)
		{
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
	if (drawspan.x2 < drawspan.x1 || drawspan.x1 < 0 || drawspan.x2 >= viewwidth ||
		drawspan.y >= viewheight || drawspan.y < 0)
	{
		Printf(PRINT_HIGH, "R_FillSpan: %i to %i at %i", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	int color = drawspan.color;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;

	COLORFUNC colorfunc(drawspan);

	do {
		colorfunc(color, dest);
		dest++;
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
		Printf(PRINT_HIGH, "R_DrawLevelSpan: %i to %i at %i", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	palindex_t* source = drawspan.source;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;
	
	dsfixed_t xfrac = drawspan.xfrac;
	dsfixed_t yfrac = drawspan.yfrac;
	const dsfixed_t xstep = drawspan.xstep;
	const dsfixed_t ystep = drawspan.ystep;

	COLORFUNC colorfunc(drawspan);

	do {
		// Current texture index in u,v.
		const int spot = ((yfrac >> (32-6-6)) & (63*64)) + (xfrac >> (32-6));

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.

		colorfunc(source[spot], dest);
		dest++;

		// Next step in u,v.
		xfrac += xstep;
		yfrac += ystep;
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
		Printf(PRINT_HIGH, "R_DrawSlopedSpan: %i to %i at %i", drawspan.x1, drawspan.x2, drawspan.y);
		return;
	}
#endif

	palindex_t* source = drawspan.source;
	int count = drawspan.x2 - drawspan.x1 + 1;
	if (count <= 0)
		return;
	
	float iu = drawspan.iu, iv = drawspan.iv;
	const float ius = drawspan.iustep, ivs = drawspan.ivstep;
	float id = drawspan.id, ids = drawspan.idstep;
	
	int ltindex = 0;

	shaderef_t colormap;
	COLORFUNC colorfunc(drawspan);

	while (count >= SPANJUMP)
	{
		const float mulstart = 65536.0f / id;
		id += ids * SPANJUMP;
		const float mulend = 65536.0f / id;

		const float ustart = iu * mulstart;
		const float vstart = iv * mulstart;

		fixed_t ufrac = (fixed_t)ustart;
		fixed_t vfrac = (fixed_t)vstart;

		iu += ius * SPANJUMP;
		iv += ivs * SPANJUMP;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		fixed_t ustep = (fixed_t)((uend - ustart) * INTERPSTEP);
		fixed_t vstep = (fixed_t)((vend - vstart) * INTERPSTEP);

		int incount = SPANJUMP;
		while (incount--)
		{
			colormap = drawspan.slopelighting[ltindex++];

			const int spot = ((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63);
			colorfunc(source[spot], dest);
			dest++;
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

		fixed_t ufrac = (fixed_t)ustart;
		fixed_t vfrac = (fixed_t)vstart;

		iu += ius * count;
		iv += ivs * count;

		const float uend = iu * mulend;
		const float vend = iv * mulend;

		fixed_t ustep = (fixed_t)((uend - ustart) / count);
		fixed_t vstep = (fixed_t)((vend - vstart) / count);

		int incount = count;
		while (incount--)
		{
			colormap = drawspan.slopelighting[ltindex++];

			const int spot = ((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63);
			colorfunc(source[spot], dest);
			dest++;
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
		fga = (translevel & ~0x03FF) >> 8;
		fga = fga > 255 ? 255 : fga;
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


// ----------------------------------------------------------------------------
//
// 8bpp color column drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_P ((palindex_t*)dcol.destination + dcol.yl * dcol.pitch_in_pixels + dcol.x)

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
// R_StretchColumnP
//
// Renders a column to the 8bpp palettized screen buffer from the source buffer
// dcol.source and scaled by dcol.iscale. Performs no shading.
//
void R_StretchColumnP()
{
	R_DrawColumnGeneric<palindex_t, PaletteFunc>(FB_COLDEST_P, dcol);
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


// ----------------------------------------------------------------------------
//
// 8bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_P ((palindex_t*)dspan.destination + dspan.y * dspan.pitch_in_pixels + dspan.x1)

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
// R_DrawSlopeSpanP
//
// Renders a span for a sloped plane to the 8bpp palettized screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSlopeSpanP()
{
	R_DrawSlopedSpanGeneric<palindex_t, PaletteSlopeColormapFunc>(FB_SPANDEST_P, dspan);
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
		argb_t work = dest[fuzztable.getValue()];
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
		argb_t fg = colormap.shade(c);
		argb_t bg = *dest;
		*dest = alphablend2a(bg, bga, fg, fga);	
	}

private:
	void calculate_alpha(fixed_t translevel)
	{
		fga = (translevel & ~0x03FF) >> 8;
		fga = fga > 255 ? 255 : fga;
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


// ----------------------------------------------------------------------------
//
// 32bpp color drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_COLDEST_D ((argb_t*)dcol.destination + dcol.yl * dcol.pitch_in_pixels + dcol.x)

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


// ----------------------------------------------------------------------------
//
// 32bpp color span drawing wrappers
//
// ----------------------------------------------------------------------------

#define FB_SPANDEST_D ((argb_t*)dspan.destination + dspan.y * dspan.pitch_in_pixels + dspan.x1)

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
void R_DrawSpanD_c()
{
	R_DrawLevelSpanGeneric<argb_t, DirectColormapFunc>(FB_SPANDEST_D, dspan);
}

//
// R_DrawSlopeSpanD
//
// Renders a span for a sloped plane to the 32bpp ARGB8888 screen buffer from
// the source buffer dspan.source. Shading is performed using dspan.colormap.
//
void R_DrawSlopeSpanD_c()
{
	R_DrawSlopedSpanGeneric<argb_t, DirectSlopeColormapFunc>(FB_SPANDEST_D, dspan);
}


/****************************************************/


void R_DrawBorder(int x1, int y1, int x2, int y2)
{
	IWindowSurface* surface = R_GetRenderingSurface();
	DCanvas* canvas = surface->getDefaultCanvas();

	int lumpnum = wads.CheckNumForName(gameinfo.borderFlat, ns_flats);
	if (lumpnum >= 0)
	{
		const byte* patch_data = (byte*)wads.CacheLumpNum(lumpnum, PU_CACHE);
		canvas->FlatFill(x1, y1, x2, y2, patch_data);
	}
	else
	{
		canvas->Clear(x1, y1, x2, y2, argb_t(0, 0, 0));
	}
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
	DCanvas* canvas = surface->getDefaultCanvas();
	int surface_width = surface->getWidth();

	const gameborder_t* border = gameinfo.border;
	const int offset = border->offset;
	const int size = border->size;

	// draw top border
	R_DrawBorder(0, 0, surface_width, viewwindowy);
	// draw bottom border
	R_DrawBorder(0, viewwindowy + viewheight, surface_width, ST_Y);
	// draw left border
	R_DrawBorder(0, viewwindowy, viewwindowx, viewwindowy + viewheight);
	// draw right border
	R_DrawBorder(viewwindowx + viewwidth, viewwindowy, surface_width, viewwindowy + viewheight);

	// draw beveled edge for the viewing window's top and bottom edges
	for (int x = viewwindowx; x < viewwindowx + viewwidth; x += size)
	{
		canvas->DrawPatch(wads.CachePatch(border->t), x, viewwindowy - offset);
		canvas->DrawPatch(wads.CachePatch(border->b), x, viewwindowy + viewheight);
	}

	// draw beveled edge for the viewing window's left and right edges
	for (int y = viewwindowy; y < viewwindowy + viewheight; y += size)
	{
		canvas->DrawPatch(wads.CachePatch(border->l), viewwindowx - offset, y);
		canvas->DrawPatch(wads.CachePatch(border->r), viewwindowx + viewwidth, y);
	}

	// draw beveled edge for the viewing window's corners
	canvas->DrawPatch(wads.CachePatch(border->tl), viewwindowx - offset, viewwindowy - offset);
	canvas->DrawPatch(wads.CachePatch(border->tr), viewwindowx + viewwidth, viewwindowy - offset);
	canvas->DrawPatch(wads.CachePatch(border->bl), viewwindowx - offset, viewwindowy + viewheight);
	canvas->DrawPatch(wads.CachePatch(border->br), viewwindowx + viewwidth, viewwindowy + viewheight);

	V_MarkRect(0, 0, surface_width, ST_Y);
}



enum r_optimize_kind {
	OPTIMIZE_NONE,
	OPTIMIZE_SSE2,
	OPTIMIZE_MMX,
	OPTIMIZE_ALTIVEC,
	OPTIMIZE_NEON,
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
	Printf(PRINT_HIGH, "r_optimize detected \"%s\"\n", get_optimization_name_list(false).c_str());
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
	#ifdef __ARM_NEON__
	# ifndef __SWITCH__ // we know for sure that the Switch has NEON
	if (SDL_HasNEON())
	# endif
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
	else if (stricmp(val, "neon") == 0 && R_IsOptimizationAvailable(OPTIMIZE_NEON))
		optimize_kind = OPTIMIZE_NEON;
	else if (stricmp(val, "detect") == 0)
		// Default to the most preferred:
		optimize_kind = optimizations_available.back();
	else
	{
		Printf(PRINT_HIGH, "Invalid value for r_optimize. Availible options are \"%s, detect\"\n",
				get_optimization_name_list(true).c_str());

		// Restore the original setting:
		var.Set(get_optimization_name(optimize_kind));
		return;
	}

	const char* optimize_name = get_optimization_name(optimize_kind);
	if (stricmp(val, optimize_name) != 0)
	{
		// update the cvar string
		// this will trigger the callback to run a second time
		Printf(PRINT_HIGH, "r_optimize set to \"%s\" based on availability\n", optimize_name);
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
	if (optimize_kind == OPTIMIZE_NONE)
	{
		// [SL] set defaults to non-vectorized drawers
		R_DrawSpanD				= R_DrawSpanD_c;
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_c;
		r_dimpatchD             = r_dimpatchD_c;
	}
	#ifdef __SSE2__
	if (optimize_kind == OPTIMIZE_SSE2)
	{
		R_DrawSpanD				= R_DrawSpanD_SSE2;
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_SSE2;
		r_dimpatchD             = r_dimpatchD_SSE2;
	}
	#endif
	#ifdef __MMX__
	else if (optimize_kind == OPTIMIZE_MMX)
	{
		R_DrawSpanD				= R_DrawSpanD_c;		// TODO
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_c;	// TODO
		r_dimpatchD             = r_dimpatchD_MMX;
	}
	#endif
	#ifdef __ALTIVEC__
	else if (optimize_kind == OPTIMIZE_ALTIVEC)
	{
		R_DrawSpanD				= R_DrawSpanD_c;		// TODO
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_c;	// TODO
		r_dimpatchD             = r_dimpatchD_ALTIVEC;
	}
	#endif
	#ifdef __ARM_NEON__
	else if (optimize_kind == OPTIMIZE_NEON)
	{
		R_DrawSpanD				= R_DrawSpanD_NEON;
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_NEON;
		r_dimpatchD             = r_dimpatchD_NEON;
	}
	#endif

	// Check that all pointers are definitely assigned!
	assert(R_DrawSpanD != NULL);
	assert(R_DrawSlopeSpanD != NULL);
	assert(r_dimpatchD != NULL);
}

// [RH] Initialize the column drawer pointers
void R_InitColumnDrawers ()
{
	if (!I_VideoInitialized())
		return;

	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		R_DrawColumn			= R_DrawColumnP;
		R_DrawFuzzColumn		= R_DrawFuzzColumnP;
		R_DrawTranslucentColumn	= R_DrawTranslucentColumnP;
		R_DrawTranslatedColumn	= R_DrawTranslatedColumnP;
		R_DrawSlopeSpan			= R_DrawSlopeSpanP;
		R_DrawSpan				= R_DrawSpanP;
		R_FillColumn			= R_FillColumnP;
		R_FillSpan				= R_FillSpanP;
		R_FillTranslucentSpan	= R_FillTranslucentSpanP;
	}
	else
	{
		// 32bpp rendering functions:
		R_DrawColumn			= R_DrawColumnD;
		R_DrawFuzzColumn		= R_DrawFuzzColumnD;
		R_DrawTranslucentColumn	= R_DrawTranslucentColumnD;
		R_DrawTranslatedColumn	= R_DrawTranslatedColumnD;
		R_DrawSlopeSpan			= R_DrawSlopeSpanD;
		R_DrawSpan				= R_DrawSpanD;
		R_FillColumn			= R_FillColumnD;
		R_FillSpan				= R_FillSpanD;
		R_FillTranslucentSpan	= R_FillTranslucentSpanD;
	}
}

VERSION_CONTROL (r_draw_cpp, "$Id$")

