// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Functions for drawing columns into a temporary buffer and then
//	copying them to the screen. On machines with a decent cache, this
//	is faster than drawing them directly to the screen. Will I be able
//	to even understand any of this if I come back to it later? Let's
//	hope so. :-)
//
//-----------------------------------------------------------------------------


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomdef.h"
#include "i_system.h"
#include "r_defs.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_things.h"
#include "v_video.h"

void (*rtv_lucent4colsP)(byte *source, palindex_t *dest, int bga, int fga) = NULL;
void (*rtv_lucent4colsD)(byte *source, argb_t *dest, int bga, int fga) = NULL;


// Palettized functions:

byte dc_temp[MAXHEIGHT * 4]; // denis - todo - security, overflow
unsigned int dc_tspans[4][256];
unsigned int *dc_ctspan[4];
unsigned int *horizspan[4];

template<typename pixel_t, int columns>
static forceinline void rt_copycols(int hx, int sx, int yl, int yh);

template<typename pixel_t, int columns>
static forceinline void rt_mapcols(int hx, int sx, int yl, int yh);

template<typename pixel_t, int columns>
static forceinline void rt_tlatecols(int hx, int sx, int yl, int yh);

template<typename pixel_t, int columns>
static forceinline void rt_lucentcols(int hx, int sx, int yl, int yh);

template<typename pixel_t, int columns>
static forceinline void rt_tlatelucentcols(int hx, int sx, int yl, int yh);


template<typename pixel_t, int columns>
static forceinline void rt_copycols(int hx, int sx, int yl, int yh)
{
	byte *source;
	pixel_t *dest;
	int count;
	int pitch;

	count = yh-yl;
	if (count < 0)
		return;
	count++;

	shaderef_t pal = shaderef_t(&realcolormaps, 0);
	dest = (pixel_t *)(ylookup[yl] + columnofs[sx]);
	source = &dc_temp[yl*4 + hx];
	pitch = dc_pitch / sizeof(pixel_t);

	if (count & 1)
	{
		for (int i = 0; i < columns; ++i)
			dest[pitch*0+i] = rt_rawcolor<pixel_t>(pal, source[0+i]);
		source += 4;
		dest += pitch;
	}
	if (!(count >>= 1))
		return;

	do
	{
		for (int i = 0; i < columns; ++i)
			dest[pitch*0+i] = rt_rawcolor<pixel_t>(pal, source[0+i]);
		for (int i = 0; i < columns; ++i)
			dest[pitch*1+i] = rt_rawcolor<pixel_t>(pal, source[4+i]);
		source += 8;
		dest += pitch*2;
	} while (--count);
}

template<typename pixel_t, int columns>
static forceinline void rt_mapcols(int hx, int sx, int yl, int yh)
{
	byte *source;
	pixel_t *dest;
	int count;
	int pitch;

	count = yh-yl;
	if (count < 0)
		return;
	count++;

	dest = (pixel_t *)(ylookup[yl] + columnofs[sx]);
	source = &dc_temp[yl*4 + hx];
	pitch = dc_pitch / sizeof(pixel_t);

	if (count & 1)
	{
		for (int i = 0; i < columns; ++i)
			dest[pitch*0+i] = rt_mapcolor<pixel_t>(dc_colormap, source[0+i]);
		source += 4;
		dest += pitch;
	}
	if (!(count >>= 1))
		return;

	do
	{
		for (int i = 0; i < columns; ++i)
			dest[pitch*0+i] = rt_mapcolor<pixel_t>(dc_colormap, source[0+i]);
		for (int i = 0; i < columns; ++i)
			dest[pitch*1+i] = rt_mapcolor<pixel_t>(dc_colormap, source[4+i]);
		source += 8;
		dest += pitch*2;
	} while (--count);
}

template<typename pixel_t, int columns>
static forceinline void rt_tlatecols(int hx, int sx, int yl, int yh)
{
	byte *source;
	pixel_t *dest;
	int count;
	int pitch;

	count = yh-yl;
	if (count < 0)
		return;
	count++;

	dest = (pixel_t *)( ylookup[yl] + columnofs[sx] );
	source = &dc_temp[yl*4 + hx];
	pitch = dc_pitch / sizeof(pixel_t);
	
	do
	{
		for (int i = 0; i < columns; ++i)
			dest[i] = rt_tlatecolor<pixel_t>(dc_colormap, dc_translation, source[i]);
		source += 4;
		dest += pitch;
	} while (--count);
}



template<typename pixel_t>
static forceinline void rtv_lucent4cols(byte *source, pixel_t *dest, int bga, int fga);

template<>
forceinline void rtv_lucent4cols(byte *source, palindex_t *dest, int bga, int fga)
{
	rtv_lucent4colsP(source, dest, bga, fga);
}

template<>
forceinline void rtv_lucent4cols(byte *source, argb_t *dest, int bga, int fga)
{
	rtv_lucent4colsD(source, dest, bga, fga);
}


template<typename pixel_t, int columns>
static forceinline void rt_lucentcols(int hx, int sx, int yl, int yh)
{
	byte *source;
	pixel_t *dest;
	int count;
	int pitch;

	count = yh-yl;
	if (count < 0)
		return;
	count++;

	int fga = (dc_translevel & ~0x03FF) >> 8;
	int bga = 255 - fga;

	dest = (pixel_t *)( ylookup[yl] + columnofs[sx] );
	source = &dc_temp[yl*4 + hx];
	pitch = dc_pitch / sizeof(pixel_t);

	do
	{
		if (columns == 4)
		{
			rtv_lucent4cols<pixel_t>(source, dest, bga, fga);
		}
		else
		{
			for (int i = 0; i < columns; ++i)
			{
				const pixel_t fg = rt_mapcolor<pixel_t>(dc_colormap, source[i]);
				const pixel_t bg = dest[i];

				dest[i] = rt_blend2<pixel_t>(bg, bga, fg, fga);
			}
		}
	
		source += 4;
		dest += pitch;
	} while (--count);
}

template<typename pixel_t, int columns>
static forceinline void rt_tlatelucentcols(int hx, int sx, int yl, int yh)
{
	byte *source;
	pixel_t *dest;
	int count;
	int pitch;

	count = yh-yl;
	if (count < 0)
		return;
	count++;

	int fga = (dc_translevel & ~0x03FF) >> 8;
	int bga = 255 - fga;

	dest = (pixel_t *)( ylookup[yl] + columnofs[sx] );
	source = &dc_temp[yl*4 + hx];
	pitch = dc_pitch / sizeof(pixel_t);

	do
	{
		for (int i = 0; i < columns; ++i)
		{
			const pixel_t fg = rt_tlatecolor<pixel_t>(dc_colormap, dc_translation, source[i]);
			const pixel_t bg = dest[i];

			dest[i] = rt_blend2<pixel_t>(bg, bga, fg, fga);
		}

		source += 4;
		dest += pitch;
	} while (--count);
}


// Copies one span at hx to the screen at sx.
void rt_copy1colP (int hx, int sx, int yl, int yh)
{
	rt_copycols<byte, 1>(hx, sx, yl, yh);
}

// Copies all four spans to the screen starting at sx.
void rt_copy4colsP (int sx, int yl, int yh)
{
	rt_copycols<byte, 4>(0, sx, yl, yh);
}

// Maps one span at hx to the screen at sx.
void rt_map1colP (int hx, int sx, int yl, int yh)
{
	rt_mapcols<byte, 1>(hx, sx, yl, yh);
}

// Maps all four spans to the screen starting at sx.
void rt_map4colsP (int sx, int yl, int yh)
{
	rt_mapcols<byte, 4>(0, sx, yl, yh);
}

// Translates one span at hx to the screen at sx.
void rt_tlate1colP (int hx, int sx, int yl, int yh)
{
	rt_tlatecols<byte, 1>(hx, sx, yl, yh);
}

// Translates all four spans to the screen starting at sx.
void rt_tlate4colsP (int sx, int yl, int yh)
{
	rt_tlatecols<byte, 4>(0, sx, yl, yh);
}

// Mixes one span at hx to the screen at sx.
void rt_lucent1colP (int hx, int sx, int yl, int yh)
{
	rt_lucentcols<byte, 1>(hx, sx, yl, yh);
}

// Mixes all four spans to the screen starting at sx.
void rt_lucent4colsP (int sx, int yl, int yh)
{
	rt_lucentcols<byte, 4>(0, sx, yl, yh);
}

// Translates and mixes one span at hx to the screen at sx.
void rt_tlatelucent1colP (int hx, int sx, int yl, int yh)
{
	rt_tlatelucentcols<byte, 1>(hx, sx, yl, yh);
}

// Translates and mixes all four spans to the screen starting at sx.
void rt_tlatelucent4colsP (int sx, int yl, int yh)
{
	rt_tlatelucentcols<byte, 4>(0, sx, yl, yh);
}


// Direct rendering (32-bit) functions:


void rt_copy1colD (int hx, int sx, int yl, int yh)
{
	rt_copycols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_copy4colsD (int sx, int yl, int yh)
{
	rt_copycols<argb_t, 4>(0, sx, yl, yh);
}

void rt_map1colD (int hx, int sx, int yl, int yh)
{
	rt_mapcols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_map4colsD (int sx, int yl, int yh)
{
	rt_mapcols<argb_t, 4>(0, sx, yl, yh);
}

void rt_tlate1colD (int hx, int sx, int yl, int yh)
{
	rt_tlatecols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_tlate4colsD (int sx, int yl, int yh)
{
	rt_tlatecols<argb_t, 4>(0, sx, yl, yh);
}

void rt_lucent1colD (int hx, int sx, int yl, int yh)
{
	rt_lucentcols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_lucent4colsD (int sx, int yl, int yh)
{
	rt_lucentcols<argb_t, 4>(0, sx, yl, yh);
}

void rt_tlatelucent1colD (int hx, int sx, int yl, int yh)
{
	rt_tlatelucentcols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_tlatelucent4colsD (int sx, int yl, int yh)
{
	rt_tlatelucentcols<argb_t, 4>(0, sx, yl, yh);
}

// Functions for v_video.cpp support

void r_dimpatchD_c(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h)
{
	int dpitch = cvs->pitch / sizeof(argb_t);
	argb_t* line = (argb_t *)cvs->buffer + y1 * dpitch;

	for (int y = y1; y < y1 + h; y++)
	{
		for (int x = x1; x < x1 + w; x++)
			line[x] = alphablend1a(line[x], color, alpha);

		line += dpitch;
	}
}

	
// Generic drawing functions which call either D(irect) or P(alettized) functions above:


// Draws all spans at hx to the screen at sx.
void rt_draw1col (int hx, int sx)
{
	while (horizspan[hx] < dc_ctspan[hx]) {
		hcolfunc_post1 (hx, sx, horizspan[hx][0], horizspan[hx][1]);
		horizspan[hx] += 2;
	}
}

// Copies all spans in all four columns to the screen starting at sx.
// sx should be dword-aligned
void rt_draw4cols(int sx)
{
	int x, bad;
	unsigned int maxtop, minbot, minnexttop;

	// Place a dummy "span" in each column. These don't get
	// drawn. They're just here to avoid special cases in the
	// max/min calculations below.
	for (x = 0; x < 4; ++x)
	{
		dc_ctspan[x][0] = viewheight + 1;
		dc_ctspan[x][1] = viewheight;
	}

	for (;;)
	{
		// If a column is out of spans, mark it as such
		bad = 0;
		minnexttop = 0xffffffff;

		for (x = 0; x < 4; ++x)
		{
			if (horizspan[x] >= dc_ctspan[x])
				bad |= 1 << x;
			else if ((horizspan[x]+2)[0] < minnexttop)
				minnexttop = (horizspan[x]+2)[0];
		}
		// Once all columns are out of spans, we're done
		if (bad == 15)
			return;

		// Find the largest shared area for the spans in each column
		maxtop = MAX (MAX (horizspan[0][0], horizspan[1][0]),
					  MAX (horizspan[2][0], horizspan[3][0]));
		minbot = MIN (MIN (horizspan[0][1], horizspan[1][1]),
					  MIN (horizspan[2][1], horizspan[3][1]));

		// If there is no shared area with these spans, draw each span
		// individually and advance to the next spans until we reach a shared area.
		// However, only draw spans down to the highest span in the next set of
		// spans. If we allow the entire height of a span to be drawn, it could
		// prevent any more shared areas from being drawn in these four columns.
		//
		// Example: Suppose we have the following arrangement:
		//			A CD
		//			A CD
		//			 B D
		//			 B D
		//			aB D
		//			aBcD
		//			aBcD
		//			aBc
		//
		// If we draw the entire height of the spans, we end up drawing this first:
		//			A CD
		//			A CD
		//			 B D
		//			 B D
		//			 B D
		//			 B D
		//			 B D
		//			 B D
		//			 B
		//
		// This leaves only the "a" and "c" columns to be drawn, and they are not
		// part of a shared area, but if we can include B and D with them, we can
		// get a shared area. So we cut off everything in the first set just
		// above the "a" column and end up drawing this first:
		//			A CD
		//			A CD
		//			 B D
		//			 B D
		//
		// Then the next time through, we have the following arrangement with an
		// easily shared area to draw:
		//			aB D
		//			aBcD
		//			aBcD
		//			aBc
		if (bad != 0 || maxtop > minbot)
		{
			for (x = 0; x < 4; ++x)
			{
				if (!(bad & 1))
				{
					if (horizspan[x][1] < minnexttop)
					{
						hcolfunc_post1(x, sx + x, horizspan[x][0], horizspan[x][1]);
						horizspan[x] += 2;
					}
					else if (minnexttop > horizspan[x][0])
					{
						hcolfunc_post1(x, sx + x, horizspan[x][0], minnexttop - 1);
						horizspan[x][0] = minnexttop;
					}
				}
				bad >>= 1;
			}
			continue;
		}

		// Draw any span fragments above the shared area.
		for (x = 0; x < 4; ++x)
		{
			if (maxtop > horizspan[x][0])
				hcolfunc_post1(x, sx + x, horizspan[x][0], maxtop - 1);
		}

		// Draw the shared area.
		hcolfunc_post4(sx, maxtop, minbot);

		// For each column, if part of the span is past the shared area,
		// set its top to just below the shared area. Otherwise, advance
		// to the next span in that column.
		for (x = 0; x < 4; ++x)
		{
			if (minbot < horizspan[x][1])
				horizspan[x][0] = minbot + 1;
			else
				horizspan[x] += 2;
		}
	}
}

// Before each pass through a rendering loop that uses these routines,
// call this function to set up the span pointers.
void rt_initcols (void)
{
	int y;

	for (y = 3; y >= 0; y--)
		horizspan[y] = dc_ctspan[y] = &dc_tspans[y][0];
}

// Same as R_DrawMaskedColumn() except that it always uses
// R_DrawColumnHoriz().
void R_DrawMaskedColumnHoriz (tallpost_t *post)
{
	while (!post->end())
	{
		if (post->length == 0)
		{
			post = post->next();
			continue;
		}

		// calculate unclipped screen coordinates for post
		int topscreen = sprtopscreen + spryscale * post->topdelta + 1;

		dc_yl = (topscreen + FRACUNIT) >> FRACBITS;
		dc_yh = (topscreen + spryscale * post->length) >> FRACBITS;

		if (dc_yh >= mfloorclip[dc_x])
			dc_yh = mfloorclip[dc_x] - 1;
		if (dc_yl <= mceilingclip[dc_x])
			dc_yl = mceilingclip[dc_x] + 1;

		dc_texturefrac = dc_texturemid - (post->topdelta << FRACBITS)
			+ (dc_yl*dc_iscale) - FixedMul(centeryfrac-FRACUNIT, dc_iscale);

		if (dc_texturefrac < 0)
		{
			int cnt = (FixedDiv(-dc_texturefrac, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
			dc_yl += cnt;
			dc_texturefrac += cnt * dc_iscale;
		}

		const fixed_t endfrac = dc_texturefrac + (dc_yh-dc_yl)*dc_iscale;
		const fixed_t maxfrac = post->length << FRACBITS;
		
		if (endfrac >= maxfrac)
		{
			int cnt = (FixedDiv(endfrac - maxfrac - 1, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
			dc_yh -= cnt;
		}

		dc_source = post->data();

		if (dc_yl >= 0 && dc_yh < viewheight && dc_yl <= dc_yh)
			hcolfunc_pre();
	
		post = post->next();
	}
}

VERSION_CONTROL (r_drawt_cpp, "$Id$")

