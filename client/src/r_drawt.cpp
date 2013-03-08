// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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

// Stretches a column into a temporary buffer which is later
// drawn to the screen along with up to three other columns.
void R_DrawColumnHorizP (void)
{
	byte *dest;
	fixed_t fracstep;
	fixed_t frac;

	int count = dc_yh - dc_yl + 1;
	if (count <= 0)
		return;

	int x = dc_x & 3;
	unsigned int **span = &dc_ctspan[x];

	(*span)[0] = dc_yl;
	(*span)[1] = dc_yh;
	*span += 2;
	dest = &dc_temp[x + 4*dc_yl];

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	int texheight = dc_textureheight;
	int mask = (texheight >> FRACBITS) - 1;
	
	byte *source = dc_source;

	// [SL] Properly tile textures whose heights are not a power-of-2,
	// avoiding a tutti-frutti effect.  From Eternity Engine.
	if (texheight & (texheight - 1))
	{
		// texture height is not a power-of-2
		if (frac < 0)
			while((frac += texheight) < 0);
		else
			while(frac >= texheight)
				frac -= texheight;

		do
		{
			*dest = source[frac>>FRACBITS];
			dest += 4;
			if ((frac += fracstep) >= texheight)
				frac -= texheight;
		} while (--count);	
	}
	else
	{
		// texture height is a power-of-2
		if (count & 1) {
			*dest = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest += 4;				
		}
		if (count & 2) {
			dest[0] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[4] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest += 8;
		}
		if (count & 4) {
			dest[0] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[4] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[8] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[12] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest += 16;
		}
		count >>= 3;
		if (!count)
			return;

		do
		{
			dest[0] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[4] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[8] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[12] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[16] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[20] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[24] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest[28] = source[(frac>>FRACBITS) & mask];	frac += fracstep;
			dest += 32;
		} while (--count);
	}
}

// [RH] Just fills a column with a given color
void R_FillColumnHorizP (void)
{
	int count = dc_yh - dc_yl;
	byte color = dc_color;
	byte *dest;

	if (count++ < 0)
		return;

	count++;
	{
		int x = dc_x & 3;
		unsigned int **span = &dc_ctspan[x];

		(*span)[0] = dc_yl;
		(*span)[1] = dc_yh;
		*span += 2;
		dest = &dc_temp[x + 4*dc_yl];
	}

	if (count & 1) {
		*dest = color;
		dest += 4;
	}
	if (!(count >>= 1))
		return;
	do {
		dest[0] = color;
		dest[4] = color;
		dest += 8;
	} while (--count);
}



	
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

	int fga, bga;
	{
		fixed_t fglevel, bglevel;

		fglevel = dc_translevel & ~0x3ff;
		bglevel = FRACUNIT - fglevel;

		// alphas should be in [0 .. 256]
		fga = (fglevel >> 8) * 256 / 255;
		bga = 256 - fga;
}

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

	int fga, bga;
	{
		fixed_t fglevel, bglevel;

		fglevel = dc_translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;

		// alphas should be in [0 .. 256]
		fga = (fglevel >> 8) * 256 / 255;
		bga = 256 - fga;
	}

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

// Copies two spans at hx and hx+1 to the screen at sx and sx+1.
void rt_copy2colsP (int hx, int sx, int yl, int yh)
{
	rt_copycols<byte, 2>(hx, sx, yl, yh);
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

// Maps two spans at hx and hx+1 to the screen at sx and sx+1.
void rt_map2colsP (int hx, int sx, int yl, int yh)
{
	rt_mapcols<byte, 2>(hx, sx, yl, yh);
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

// Translates two spans at hx and hx+1 to the screen at sx and sx+1.
void rt_tlate2colsP (int hx, int sx, int yl, int yh)
{
	rt_tlatecols<byte, 2>(hx, sx, yl, yh);
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

// Mixes two spans at hx and hx+1 to the screen at sx and sx+1.
void rt_lucent2colsP (int hx, int sx, int yl, int yh)
{
	rt_lucentcols<byte, 2>(hx, sx, yl, yh);
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

// Translates and mixes two spans at hx and hx+1 to the screen at sx and sx+1.
void rt_tlatelucent2colsP (int hx, int sx, int yl, int yh)
{
	rt_tlatelucentcols<byte, 2>(hx, sx, yl, yh);
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

void rt_copy2colsD (int hx, int sx, int yl, int yh)
{
	rt_copycols<argb_t, 2>(hx, sx, yl, yh);
}

void rt_copy4colsD (int sx, int yl, int yh)
	{
	rt_copycols<argb_t, 4>(0, sx, yl, yh);
}

void rt_map1colD (int hx, int sx, int yl, int yh)
{
	rt_mapcols<argb_t, 1>(hx, sx, yl, yh);
	}

void rt_map2colsD (int hx, int sx, int yl, int yh)
{
	rt_mapcols<argb_t, 2>(hx, sx, yl, yh);
}

void rt_map4colsD (int sx, int yl, int yh)
{
	rt_mapcols<argb_t, 4>(0, sx, yl, yh);
}

void rt_tlate1colD (int hx, int sx, int yl, int yh)
{
	rt_tlatecols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_tlate2colsD (int hx, int sx, int yl, int yh)
{
	rt_tlatecols<argb_t, 2>(hx, sx, yl, yh);
}

void rt_tlate4colsD (int sx, int yl, int yh)
{
	rt_tlatecols<argb_t, 4>(0, sx, yl, yh);
}

void rt_lucent1colD (int hx, int sx, int yl, int yh)
	{
	rt_lucentcols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_lucent2colsD (int hx, int sx, int yl, int yh)
{
	rt_lucentcols<argb_t, 2>(hx, sx, yl, yh);
	}

void rt_lucent4colsD (int sx, int yl, int yh)
{
	rt_lucentcols<argb_t, 4>(0, sx, yl, yh);
}

void rt_tlatelucent1colD (int hx, int sx, int yl, int yh)
{
	rt_tlatelucentcols<argb_t, 1>(hx, sx, yl, yh);
}

void rt_tlatelucent2colsD (int hx, int sx, int yl, int yh)
{
	rt_tlatelucentcols<argb_t, 2>(hx, sx, yl, yh);
}

void rt_tlatelucent4colsD (int sx, int yl, int yh)
{
	rt_tlatelucentcols<argb_t, 4>(0, sx, yl, yh);
}


void R_DrawSpanD_c (void)
	{
	dsfixed_t			xfrac;
	dsfixed_t			yfrac;
	dsfixed_t			xstep;
	dsfixed_t			ystep;
	argb_t*             dest;
	int 				count;
	int 				spot;

#ifdef RANGECHECK
	if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=screen->width
		|| ds_y>screen->height)
	{
		I_Error ("R_DrawSpan: %i to %i at %i",
				 ds_x1,ds_x2,ds_y);
	}
//		dscount++;
#endif

	xfrac = ds_xfrac;
	yfrac = ds_yfrac;

	dest = (argb_t *)(ylookup[ds_y] + columnofs[ds_x1]);

	// We do not check for zero spans here?
	count = ds_x2 - ds_x1 + 1;

	xstep = ds_xstep;
	ystep = ds_ystep;

	do {
		// Current texture index in u,v.
		spot = ((yfrac>>(32-6-6))&(63*64)) + (xfrac>>(32-6));

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest = ds_colormap.shade(ds_source[spot]);
		dest += ds_colsize;

		// Next step in u,v.
		xfrac += xstep;
		yfrac += ystep;
	} while (--count);
}


void R_DrawSlopeSpanD_c (void)
{
	int count = ds_x2 - ds_x1 + 1;
	if (count <= 0)
		return;

#ifdef RANGECHECK 
	if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=screen->width
		|| ds_y>screen->height)
	{
		I_Error ("R_DrawSlopeSpan: %i to %i at %i",
					ds_x1,ds_x2,ds_y);
	}
#endif

	double iu = ds_iu, iv = ds_iv;
	double ius = ds_iustep, ivs = ds_ivstep;
	double id = ds_id, ids = ds_idstep;

	// framebuffer	
	argb_t *dest = (argb_t *)( ylookup[ds_y] + columnofs[ds_x1] );

	// texture data
	byte *src = (byte *)ds_source;

	int colsize = ds_colsize;
	shaderef_t colormap;
	int ltindex = 0;		// index into the lighting table

	while(count >= SPANJUMP)
	{
		double ustart, uend;
		double vstart, vend;
		double mulstart, mulend;
		unsigned int ustep, vstep, ufrac, vfrac;
		int incount;

		mulstart = 65536.0f / id;
		id += ids * SPANJUMP;
		mulend = 65536.0f / id;

		ufrac = (int)(ustart = iu * mulstart);
		vfrac = (int)(vstart = iv * mulstart);
		iu += ius * SPANJUMP;
		iv += ivs * SPANJUMP;
		uend = iu * mulend;
		vend = iv * mulend;

		ustep = (int)((uend - ustart) * INTERPSTEP);
		vstep = (int)((vend - vstart) * INTERPSTEP);

		incount = SPANJUMP;
		while(incount--)
		{
			colormap = slopelighting[ltindex++];
			*dest = colormap.shade(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest += colsize;
			ufrac += ustep;
			vfrac += vstep;
}

		count -= SPANJUMP;
	}
	if (count > 0)
{
		double ustart, uend;
		double vstart, vend;
		double mulstart, mulend;
		unsigned int ustep, vstep, ufrac, vfrac;
		int incount;

		mulstart = 65536.0f / id;
		id += ids * count;
		mulend = 65536.0f / id;

		ufrac = (int)(ustart = iu * mulstart);
		vfrac = (int)(vstart = iv * mulstart);
		iu += ius * count;
		iv += ivs * count;
		uend = iu * mulend;
		vend = iv * mulend;

		ustep = (int)((uend - ustart) / count);
		vstep = (int)((vend - vstart) / count);

		incount = count;
		while(incount--)
		{
			colormap = slopelighting[ltindex++];
			*dest = colormap.shade(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest += colsize;
			ufrac += ustep;
			vfrac += vstep;
		}
	}
}

// Functions for v_video.cpp support

void r_dimpatchD_c(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h)
	{
	int x, y;
	argb_t *line;
	int invAlpha = 256 - alpha;

	int dpitch = cvs->pitch / sizeof(argb_t);
	line = (argb_t *)cvs->buffer + y1 * dpitch;

	for (y = y1; y < y1 + h; y++)
	{
		for (x = x1; x < x1 + w; x++)
		{
			line[x] = alphablend1a(line[x], color, alpha);
		}
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

// Adjusts two columns so that they both start on the same row.
// Returns false if it succeeded, true if a column ran out.
static BOOL rt_nudgecols (int hx, int sx)
{
	if (horizspan[hx][0] < horizspan[hx+1][0]) {
spaghetti1:
		// first column starts before the second; it might also end before it
		if (horizspan[hx][1] < horizspan[hx+1][0]){
			while (horizspan[hx] < dc_ctspan[hx] && horizspan[hx][1] < horizspan[hx+1][0]) {
				hcolfunc_post1 (hx, sx, horizspan[hx][0], horizspan[hx][1]);
				horizspan[hx] += 2;
			}
			if (horizspan[hx] >= dc_ctspan[hx]) {
				// the first column ran out of spans
				rt_draw1col (hx+1, sx+1);
				return true;
			}
			if (horizspan[hx][0] > horizspan[hx+1][0])
				goto spaghetti2;	// second starts before first now
			else if (horizspan[hx][0] == horizspan[hx+1][0])
				return false;
		}
		hcolfunc_post1 (hx, sx, horizspan[hx][0], horizspan[hx+1][0] - 1);
		horizspan[hx][0] = horizspan[hx+1][0];
	}
	if (horizspan[hx][0] > horizspan[hx+1][0]) {
spaghetti2:
		// second column starts before the first; it might also end before it
		if (horizspan[hx+1][1] < horizspan[hx][0]) {
			while (horizspan[hx+1] < dc_ctspan[hx+1] && horizspan[hx+1][1] < horizspan[hx][0]) {
				hcolfunc_post1 (hx+1, sx+1, horizspan[hx+1][0], horizspan[hx+1][1]);
				horizspan[hx+1] += 2;
			}
			if (horizspan[hx+1] >= dc_ctspan[hx+1]) {
				// the second column ran out of spans
				rt_draw1col (hx, sx);
				return true;
			}
			if (horizspan[hx][0] < horizspan[hx+1][0])
				goto spaghetti1;	// first starts before second now
			else if (horizspan[hx][0] == horizspan[hx+1][0])
				return false;
		}
		hcolfunc_post1 (hx+1, sx+1, horizspan[hx+1][0], horizspan[hx][0] - 1);
		horizspan[hx+1][0] = horizspan[hx][0];
	}
	return false;
}

// Copies all spans at hx and hx+1 to the screen at sx and sx+1.
// hx and sx should be word-aligned.
void rt_draw2cols (int hx, int sx)
{
    while(1) // keep going until all columns have no more spans
    {
        if (horizspan[hx] >= dc_ctspan[hx]) {
            // no first column, do the second (if any)
            rt_draw1col (hx+1, sx+1);
            break;
        }
        if (horizspan[hx+1] >= dc_ctspan[hx+1]) {
            // no second column, do the first
            rt_draw1col (hx, sx);
            break;
        }

        // both columns have spans, align their tops
        if (rt_nudgecols (hx, sx))
            break;

        // now draw as much as possible as a series of words
        if (horizspan[hx][1] < horizspan[hx+1][1]) {
            // first column ends first, so draw down to its bottom
            hcolfunc_post2 (hx, sx, horizspan[hx][0], horizspan[hx][1]);
            horizspan[hx+1][0] = horizspan[hx][1] + 1;
            horizspan[hx] += 2;
        } else {
            // second column ends first, or they end at the same spot
            hcolfunc_post2 (hx, sx, horizspan[hx+1][0], horizspan[hx+1][1]);
            if (horizspan[hx][1] == horizspan[hx+1][1]) {
                horizspan[hx] += 2;
                horizspan[hx+1] += 2;
            } else {
                horizspan[hx][0] = horizspan[hx+1][1] + 1;
                horizspan[hx+1] += 2;
            }
        }
    }
}

// Copies all spans in all four columns to the screen starting at sx.
// sx should be longword-aligned.
void rt_draw4cols (int sx)
{
loop:
	if (horizspan[0] >= dc_ctspan[0]) {
		// no first column, do the second (if any)
		rt_draw1col (1, sx+1);
		rt_draw2cols (2, sx+2);
		return;
	}
	if (horizspan[1] >= dc_ctspan[1]) {
		// no second column, we already know there is a first one
		rt_draw1col (0, sx);
		rt_draw2cols (2, sx+2);
		return;
	}
	if (horizspan[2] >= dc_ctspan[2]) {
		// no third column, do the fourth (if any)
		rt_draw2cols (0, sx);
		rt_draw1col (3, sx+3);
		return;
	}
	if (horizspan[3] >= dc_ctspan[3]) {
		// no fourth column, but there is a third
		rt_draw2cols (0, sx);
		rt_draw1col (2, sx+2);
		return;
	}

	// if we get here, then we know all four columns have something,
	// make sure they all align at the top
	if (rt_nudgecols (0, sx)) {
		rt_draw2cols (2, sx+2);
		return;
	}
	if (rt_nudgecols (2, sx+2)) {
		rt_draw2cols (0, sx);
		return;
	}

	// first column is now aligned with second at top, and third is aligned
	// with fourth at top. now make sure both halves align at the top and
	// also have some shared space to their bottoms.
	{
		unsigned int bot1 = horizspan[0][1] < horizspan[1][1] ? horizspan[0][1] : horizspan[1][1];
		unsigned int bot2 = horizspan[2][1] < horizspan[3][1] ? horizspan[2][1] : horizspan[3][1];

		if (horizspan[0][0] < horizspan[2][0]) {
			// first half starts before second half
			if (bot1 >= horizspan[2][0]) {
				// first half ends after second begins
				hcolfunc_post2 (0, sx, horizspan[0][0], horizspan[2][0] - 1);
				horizspan[0][0] = horizspan[1][0] = horizspan[2][0];
			} else {
				// first half ends before second begins
				hcolfunc_post2 (0, sx, horizspan[0][0], bot1);
				if (horizspan[0][1] == bot1)
					horizspan[0] += 2;
				else
					horizspan[0][0] = bot1 + 1;
				if (horizspan[1][1] == bot1)
					horizspan[1] += 2;
				else
					horizspan[1][0] = bot1 + 1;
				goto loop;	// start over
			}
		} else if (horizspan[0][0] > horizspan[2][0]) {
			// second half starts before the first
			if (bot2 >= horizspan[0][0]) {
				// second half ends after first begins
				hcolfunc_post2 (2, sx+2, horizspan[2][0], horizspan[0][0] - 1);
				horizspan[2][0] = horizspan[3][0] = horizspan[0][0];
			} else {
				// second half ends before first begins
				hcolfunc_post2 (2, sx+2, horizspan[2][0], bot2);
				if (horizspan[2][1] == bot2)
					horizspan[2] += 2;
				else
					horizspan[2][0] = bot2 + 1;
				if (horizspan[3][1] == bot2)
					horizspan[3] += 2;
				else
					horizspan[3][0] = bot2 + 1;
				goto loop;	// start over
			}
		}

		// all four columns are now aligned at the top; draw all of them
		// until one ends.
		bot1 = bot1 < bot2 ? bot1 : bot2;

		hcolfunc_post4 (sx, horizspan[0][0], bot1);

		{
			int x;

			for (x = 3; x >= 0; x--)
				if (horizspan[x][1] == bot1)
					horizspan[x] += 2;
				else
					horizspan[x][0] = bot1 + 1;
		}
	}

	goto loop;	// keep going until all columns have no more spans
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

