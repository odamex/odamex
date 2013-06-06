// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//		The actual span/column drawing functions.
//		Here find the main potential for optimization,
//		 e.g. inline assembly, different algorithms.
//
//-----------------------------------------------------------------------------

#include <stddef.h>
#include <assert.h>
#include <algorithm>

#include "SDL_cpuinfo.h"
#include "r_intrin.h"

#include "m_alloc.h"
#include "doomdef.h"
#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"
#include "r_local.h"
#include "v_video.h"
#include "doomstat.h"
#include "st_stuff.h"

#include "gi.h"

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


byte*			viewimage;
extern "C" {
int 			viewwidth;
int 			viewheight;
}
int 			scaledviewwidth;
int 			viewwindowx;
int 			viewwindowy;
byte**			ylookup;
int* 			columnofs;

extern "C" {
int				realviewwidth;		// [RH] Physical width of view window
int				realviewheight;		// [RH] Physical height of view window
int				detailxshift;		// [RH] X shift for horizontal detail level
int				detailyshift;		// [RH] Y shift for vertical detail level
}

// [RH] Pointers to the different column drawers.
//		These get changed depending on the current
//		screen depth.
void (*R_DrawColumnHoriz)(void);
void (*R_DrawColumn)(void);
void (*R_DrawFuzzColumn)(void);
void (*R_DrawTranslucentColumn)(void);
void (*R_DrawTranslatedColumn)(void);
void (*R_DrawSpan)(void);
void (*R_DrawSlopeSpan)(void);
void (*R_FillColumn)(void);
void (*R_FillSpan)(void);
void (*rt_copy1col) (int hx, int sx, int yl, int yh);
void (*rt_copy2cols) (int hx, int sx, int yl, int yh);
void (*rt_copy4cols) (int sx, int yl, int yh);
void (*rt_map1col) (int hx, int sx, int yl, int yh);
void (*rt_map2cols) (int hx, int sx, int yl, int yh);
void (*rt_map4cols) (int sx, int yl, int yh);
void (*rt_lucent1col) (int hx, int sx, int yl, int yh);
void (*rt_lucent2cols) (int hx, int sx, int yl, int yh);
void (*rt_lucent4cols) (int sx, int yl, int yh);
void (*rt_tlate1col) (int hx, int sx, int yl, int yh);
void (*rt_tlate2cols) (int hx, int sx, int yl, int yh);
void (*rt_tlate4cols) (int sx, int yl, int yh);
void (*rt_tlatelucent1col) (int hx, int sx, int yl, int yh);
void (*rt_tlatelucent2cols) (int hx, int sx, int yl, int yh);
void (*rt_tlatelucent4cols) (int sx, int yl, int yh);

// Possibly vectorized functions:
void (*R_DrawSpanD)(void);
void (*R_DrawSlopeSpanD)(void);
void (*r_dimpatchD)(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h);

//
// R_DrawColumn
// Source is the top of the column to scale.
//
extern "C" {
int				dc_pitch=0x12345678;	// [RH] Distance between rows

shaderef_t		dc_colormap;
int 			dc_x; 
int 			dc_yl; 
int 			dc_yh; 
fixed_t 		dc_iscale; 
fixed_t 		dc_texturemid;
fixed_t			dc_texturefrac;
int				dc_color;				// [RH] Color for column filler

// first pixel in a column (possibly virtual) 
byte*			dc_source;				

// just for profiling 
int 			dccount;
}

/************************************/
/*									*/
/* Palettized drawers (C versions)	*/
/*									*/
/************************************/

//
// A column is a vertical slice/span from a wall texture that,
//	given the DOOM style restrictions on the view orientation,
//	will always have constant z depth.
// Thus a special case loop for very fast rendering can
//	be used. It has also been used with Wolfenstein 3D.
// 
void R_DrawColumnP (void)
{
	int 				count;
	byte*				dest;
	fixed_t 			frac;
	fixed_t 			fracstep;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;

	count++;

#ifdef RANGECHECK 
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height) {
		Printf (PRINT_HIGH, "R_DrawColumnP: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
		return;
	}
#endif

	// Framebuffer destination address.
	// Use ylookup LUT to avoid multiply with ScreenWidth.
	// Use columnofs LUT for subwindows?
	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Determine scaling,
	//	which is the only mapping to be done.
	fracstep = dc_iscale; 
	frac = dc_texturefrac;

	{
		// [RH] Get local copies of these variables so that the compiler
		//		has a better chance of optimizing this well.
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;
		byte *source = dc_source;
		int pitch = dc_pitch;

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
				*dest = dc_colormap.index(source[frac>>FRACBITS]);
				dest += pitch;
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{
			// texture height is a power-of-2
			
			// Inner loop that does the actual texture mapping,
			//	e.g. a DDA-lile scaling.
			// This is as fast as it gets.
			do
			{
				// Re-map color indices from wall texture column
				//	using a lighting/special effects LUT.
				*dest = dc_colormap.index(source[(frac>>FRACBITS)&mask]);

				dest += pitch;
				frac += fracstep;
			} while (--count);
		}
	}
} 


// [RH] Same as R_DrawColumnP except that it doesn't do any colormapping.
//		Used by the sky drawer because the sky is always fullbright.
void R_StretchColumnP (void)
{
	int 				count;
	byte*				dest;
	fixed_t 			frac;
	fixed_t 			fracstep;

	count = dc_yh - dc_yl;

	if (count < 0)
		return;

	count++;

#ifdef RANGECHECK 
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height) {
		Printf (PRINT_HIGH, "R_StretchColumnP: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
		return;
	}
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x];
	fracstep = dc_iscale; 
	frac = dc_texturefrac;

	{
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;
		byte *source = dc_source;
		int pitch = dc_pitch;

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
				dest += pitch;
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				*dest = source[(frac>>FRACBITS)&mask];
				dest += pitch;
				frac += fracstep;
			} while (--count);
		}
	}
} 

//
// R_BlankColumn
//
// [SL] - Does nothing (obviously). Used when a column drawing function
// pointer should not draw anything.
//
void R_BlankColumn (void)
{
}

// [RH] Just fills a column with a color
void R_FillColumnP (void)
{
	int 				count;
	byte*				dest;

	count = dc_yh - dc_yl;

	if (count < 0)
		return;

	count++;

#ifdef RANGECHECK 
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height) {
		Printf (PRINT_HIGH, "R_StretchColumnP: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
		return;
	}
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x];

	{
		int pitch = dc_pitch;
		byte color = dc_color;

		do
		{
			*dest = color;
			dest += pitch;
		} while (--count);
	}
} 

//
// Spectre/Invisibility.
//
// [RH] FUZZTABLE changed from 50 to 64
#define FUZZTABLE	64
#define FUZZOFF		(screen->pitch)

extern "C"
{
int 	fuzzoffset[FUZZTABLE];
int 	fuzzpos = 0; 
}
/*
	FUZZOFF,-FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
	FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
	FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,
	FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,
	FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,
	FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,
	FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF,FUZZOFF,-FUZZOFF,FUZZOFF 
*/

static const signed char fuzzinit[FUZZTABLE] = {
	1,-1, 1,-1, 1, 1,-1, 1,
	1,-1, 1, 1, 1,-1, 1, 1,
	1,-1,-1,-1,-1, 1,-1,-1,
	1, 1, 1, 1,-1, 1,-1, 1,
	1,-1,-1, 1, 1,-1,-1,-1,
   -1, 1, 1, 1, 1,-1, 1, 1,
   -1, 1, 1, 1,-1, 1, 1, 1,
   -1, 1, 1,-1, 1, 1,-1, 1
};

void R_InitFuzzTable (void)
{
	int i;
	int fuzzoff;

	screen->Lock ();
	fuzzoff = FUZZOFF << detailyshift;
	screen->Unlock ();

	for (i = 0; i < FUZZTABLE; i++)
		fuzzoffset[i] = fuzzinit[i] * fuzzoff;
}


//
// Framebuffer postprocessing.
// Creates a fuzzy image by copying pixels
//	from adjacent ones to left and right.
// Used with an all black colormap, this
//	could create the SHADOW effect,
//	i.e. spectres and invisible players.
//
void R_DrawFuzzColumnP (void)
{
	int count;
	byte *dest;

	// Adjust borders. Low...
	if (!dc_yl)
		dc_yl = 1;

	// .. and high.
	if (dc_yh == realviewheight-1)
		dc_yh = realviewheight - 2;

	count = dc_yh - dc_yl;

	// Zero length.
	if (count < 0)
		return;

	count++;

#ifdef RANGECHECK
	if (dc_x >= screen->width
		|| dc_yl < 0 || dc_yh >= screen->height)
	{
		I_Error ("R_DrawFuzzColumnP: %i to %i at %i",
				 dc_yl, dc_yh, dc_x);
	}
#endif

	dest = ylookup[dc_yl] + columnofs[dc_x];

	// Looks like an attempt at dithering,
	//	using the colormap #6 (of 0-31, a bit
	//	brighter than average).
	{
		// [RH] Make local copies of global vars to try and improve
		//		the optimizations made by the compiler.
		int pitch = dc_pitch;
		int fuzz = fuzzpos;

		//byte *map = GetDefaultPalette()->maps.colormap + 6*256;
		shaderef_t  map(&GetDefaultPalette()->maps, 6);

		do 
		{
			// Lookup framebuffer, and retrieve
			//	a pixel that is either one column
			//	left or right of the current one.
			// Add index from colormap to index.
			*dest = map.index(dest[fuzzoffset[fuzz]]);

			// Clamp table lookup index.
			fuzz = (fuzz + 1) & (FUZZTABLE - 1);
			
			dest += pitch;
		} while (--count);

		fuzzpos = (fuzz + 3) & (FUZZTABLE - 1);
	}
} 

//
// R_DrawTranlucentColumn
//
fixed_t dc_translevel;

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

void R_DrawTranslucentColumnP (void)
{
	int count;
	byte *dest;
	fixed_t frac;
	fixed_t fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
	count++;

#ifdef RANGECHECK
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height)
	{
		I_Error ( "R_DrawTranslucentColumnP: %i to %i at %i",
				  dc_yl, dc_yh, dc_x);
	}
#endif 

	int bga, fga;
	{
		fixed_t fglevel, bglevel;

		fglevel = dc_translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
#if 1
		fga = fglevel >> 8;
		bga = bglevel >> 8;
#else
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
#endif
	}

	dest = ylookup[dc_yl] + columnofs[dc_x];

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	{
		byte *source = dc_source;
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;
		int pitch = dc_pitch;

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
				palindex_t fg = dc_colormap.index(source[(frac>>FRACBITS)]);
				palindex_t bg = *dest;
				
#if 1
				*dest = rt_blend2<palindex_t>(bg, bga, fg, fga);
#else
				fg = fg2rgb[fg];
				bg = bg2rgb[bg];
				fg = (fg+bg) | 0x1f07c1f;
				*dest = RGB32k[0][0][fg & (fg>>15)];
#endif

				dest += pitch;
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				palindex_t fg = dc_colormap.index(source[(frac>>FRACBITS)&mask]);
				palindex_t bg = *dest;

#if 1
				*dest = rt_blend2<palindex_t>(bg, bga, fg, fga);
#else
				fg = fg2rgb[fg];
				bg = bg2rgb[bg];
				fg = (fg+bg) | 0x1f07c1f;
				*dest = RGB32k[0][0][fg & (fg>>15)];
#endif

				dest += pitch;
				frac += fracstep;
			} while (--count);
		}
	}
}

//
// R_DrawTranslatedColumn
// Used to draw player sprites
//	with the green colorramp mapped to others.
// Could be used with different translation
//	tables, e.g. the lighter colored version
//	of the BaronOfHell, the HellKnight, uses
//	identical sprites, kinda brightened up.
//
translationref_t dc_translation;
byte*	translationtables;
argb_t           translationRGB[MAXPLAYERS+1][16];

void R_DrawTranslatedColumnP (void)
{ 
	int 				count;
	byte*				dest;
	fixed_t 			frac;
	fixed_t 			fracstep;

	count = dc_yh - dc_yl;
	if (count < 0) 
		return;
	count++;

#ifdef RANGECHECK 
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height)
	{
		I_Error ( "R_DrawTranslatedColumnP: %i to %i at %i",
				  dc_yl, dc_yh, dc_x);
	}
	
#endif 

	dest = ylookup[dc_yl] + columnofs[dc_x];

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	{
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;
		byte *source = dc_source;
		int pitch = dc_pitch;

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
				*dest = dc_colormap.index(dc_translation.tlate(source[(frac>>FRACBITS)]));
				dest += pitch;
				
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				*dest = dc_colormap.index(dc_translation.tlate(source[(frac>>FRACBITS) & mask]));
				dest += pitch;

				frac += fracstep;
			} while (--count);
		}
	}
}

// Draw a column that is both translated and translucent
void R_DrawTlatedLucentColumnP (void)
{
	int count;
	byte *dest;
	fixed_t frac;
	fixed_t fracstep;
	argb_t *fg2rgb, *bg2rgb;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
	count++;

#ifdef RANGECHECK
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height)
	{
		I_Error ( "R_DrawTlatedLucentColumnP: %i to %i at %i",
				  dc_yl, dc_yh, dc_x);
	}
	
#endif 

	{
		fixed_t fglevel, bglevel;

		fglevel = dc_translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;
		fg2rgb = Col2RGB8[fglevel>>10];
		bg2rgb = Col2RGB8[bglevel>>10];
	}

	dest = ylookup[dc_yl] + columnofs[dc_x];

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	{
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;
		byte *source = dc_source;
		int pitch = dc_pitch;

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
				unsigned int fg = dc_colormap.index(dc_translation.tlate(source[(frac>>FRACBITS)]));
				unsigned int bg = *dest;

				fg = fg2rgb[fg];
				bg = bg2rgb[bg];
				fg = (fg+bg) | 0x1f07c1f;
				*dest = RGB32k[0][0][fg & (fg>>15)];
				dest += pitch;
				
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				unsigned int fg = dc_colormap.index(dc_translation.tlate(source[(frac>>FRACBITS)&mask]));
				unsigned int bg = *dest;

				fg = fg2rgb[fg];
				bg = bg2rgb[bg];
				fg = (fg+bg) | 0x1f07c1f;
				*dest = RGB32k[0][0][fg & (fg>>15)];
				dest += pitch;
				frac += fracstep;
			} while (--count);
		}
	}
}


//
// R_DrawSpan 
// With DOOM style restrictions on view orientation,
//	the floors and ceilings consist of horizontal slices
//	or spans with constant z depth.
// However, rotation around the world z axis is possible,
//	thus this mapping, while simpler and faster than
//	perspective correct texture mapping, has to traverse
//	the texture at an angle in all but a few cases.
// In consequence, flats are not stored by column (like walls),
//	and the inner loop has to step in texture space u and v.
//
extern "C" {
int						ds_colsize=0xdeadbeef;	// [RH] Distance between columns
int						ds_color;				// [RH] color for non-textured spans

int 					ds_y; 
int 					ds_x1; 
int 					ds_x2;

shaderef_t				ds_colormap; 

dsfixed_t 				ds_xfrac; 
dsfixed_t 				ds_yfrac; 
dsfixed_t 				ds_xstep; 
dsfixed_t 				ds_ystep;

// start of a 64*64 tile image 
byte*					ds_source;		

// just for profiling
int 					dscount;

// [SL] 2012-03-19 - For sloped planes
float					ds_iu;
float					ds_iv;
float					ds_iustep;
float					ds_ivstep;
float					ds_id;
float					ds_idstep;
shaderef_t				slopelighting[MAXWIDTH];
}

//
// Draws the actual span.

void R_DrawSpanP (void)
{
	dsfixed_t			xfrac;
	dsfixed_t			yfrac;
	dsfixed_t			xstep;
	dsfixed_t			ystep;
	byte*				dest;
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

	dest = ylookup[ds_y] + columnofs[ds_x1];

	// We do not check for zero spans here?
	count = ds_x2 - ds_x1 + 1;

	xstep = ds_xstep;
	ystep = ds_ystep;

	do {
		// Current texture index in u,v.
		spot = ((yfrac>>(32-6-6))&(63*64)) + (xfrac>>(32-6));

		// Lookup pixel from flat texture tile,
		//  re-index using light/colormap.
		*dest = ds_colormap.index(ds_source[spot]);
		dest += ds_colsize;

		// Next step in u,v.
		xfrac += xstep;
		yfrac += ystep;
	} while (--count);
}

// [RH] Just fill a span with a color
void R_FillSpanP (void)
{
#ifdef RANGECHECK
	if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=screen->width
		|| ds_y>screen->height)
	{
		I_Error( "R_FillSpan: %i to %i at %i",
				 ds_x1,ds_x2,ds_y);
	}
//		dscount++;
#endif

	memset (ylookup[ds_y] + columnofs[ds_x1], ds_color, (ds_x2 - ds_x1 + 1) * ds_colsize);
}


//
// R_DrawSlopeSpan
//
// Texture maps a sloped surface using affine texturemapping for each row of
// the span.  Not as pretty as a perfect texturemapping but should be much
// faster.
//
// Based on R_DrawSlope_8_64 from Eternity Engine, written by SoM/Quasar
//

void R_DrawSlopeSpanP(void)
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

	float iu = ds_iu, iv = ds_iv;
	float ius = ds_iustep, ivs = ds_ivstep;
	float id = ds_id, ids = ds_idstep;
	
	// framebuffer	
	byte *dest = ylookup[ds_y] + columnofs[ds_x1];
	
	// texture data
	byte *src = (byte *)ds_source;

	const int colsize = ds_colsize;
	shaderef_t colormap;
	int ltindex = 0;		// index into the lighting table

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
			colormap = slopelighting[ltindex++];
			*dest = colormap.index(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest += colsize;
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
			colormap = slopelighting[ltindex++];
			*dest = colormap.index(src[((vfrac >> 10) & 0xFC0) | ((ufrac >> 16) & 63)]);
			dest += colsize;
			ufrac += ustep;
			vfrac += vstep;
		}
	}
}

//
// R_DrawSlopeSpanIdeal
//
// Texture maps a sloped surface using an ideal method of texturemapping.
// This is likely slower than desired and therefore useful mostly as
// a reference only.
//
// Based on R_DrawSlope_8_64 from Eternity Engine, written by SoM/Quasar
//
void R_DrawSlopeSpanIdealP_C(void)
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

	float iu = ds_iu, iv = ds_iv;
	float ius = ds_iustep, ivs = ds_ivstep;
	float id = ds_id, ids = ds_idstep;
	
	// framebuffer	
	byte *dest = ylookup[ds_y] + columnofs[ds_x1];
	
	// texture data
	byte *src = (byte *)ds_source;

	int colsize = ds_colsize;
	shaderef_t colormap;
	int ltindex = 0;		// index into the lighting table

	do
	{
		float mul = 1.0f / id;

		int u = (int)(iu * mul);
		int v = (int)(iv * mul);
		unsigned texl = (v & 63) * 64 + (u & 63);

		colormap = slopelighting[ltindex++];
		*dest = colormap.index(src[texl]);
		dest += colsize;

		iu += ius;
		iv += ivs;
		id += ids;	
	} while (--count);
}

/****************************************/
/*										*/
/* [RH] ARGB8888 drawers (C versions)	*/
/*										*/
/****************************************/

void R_DrawColumnD (void)
{ 
	int 				count;
	argb_t*		dest;
	fixed_t 			frac;
	fixed_t 			fracstep;

	count = dc_yh - dc_yl;

	// Zero length, column does not exceed a pixel.
	if (count < 0)
		return;
	count++;

#ifdef RANGECHECK 
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height) {
		Printf (PRINT_HIGH, "R_DrawColumnD: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
		return;
	}
#endif

	dest = (argb_t *)(ylookup[dc_yl] + columnofs[dc_x]);

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	{
		byte *source = dc_source;
		int pitch = dc_pitch / sizeof(DWORD);
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;

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
				*dest = dc_colormap.shade(source[(frac>>FRACBITS)]);

				dest += pitch;
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				*dest = dc_colormap.shade(source[(frac>>FRACBITS)&mask]);

				dest += pitch;
				frac += fracstep;
			} while (--count);
		}
	}
}

void R_DrawFuzzColumnD (void)
{
	int 				count;
	argb_t*		dest;

	// Adjust borders. Low...
	if (!dc_yl)
		dc_yl = 1;

	// .. and high.
	if (dc_yh == realviewheight-1)
		dc_yh = realviewheight - 2;

	count = dc_yh - dc_yl;

	// Zero length.
	if (count < 0)
		return;
	count++;

#ifdef RANGECHECK
	if (dc_x >= screen->width
		|| dc_yl < 0 || dc_yh >= screen->height)
	{
		I_Error ("R_DrawFuzzColumnD: %i to %i at %i",
				 dc_yl, dc_yh, dc_x);
	}
#endif

	dest = (argb_t *)(ylookup[dc_yl] + columnofs[dc_x]);

	// [RH] This is actually slightly brighter than
	//		the indexed version, but it's close enough.
	{
		int fuzz = fuzzpos;
		int pitch = dc_pitch / sizeof(argb_t);

		do
		{
			argb_t work = dest[fuzzoffset[fuzz]>>2];
			*dest = work - ((work >> 2) & 0x3f3f3f);

			// Clamp table lookup index.
			fuzz = (fuzz + 1) & (FUZZTABLE - 1);
			
			dest += pitch;
		} while (--count);

		fuzzpos = (fuzz + 3) & (FUZZTABLE - 1);
	}
}

void R_DrawTranslucentColumnD (void)
{
	int 				count;
	argb_t*		dest;
	fixed_t 			frac;
	fixed_t 			fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
	count++;

#ifdef RANGECHECK
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height)
	{
		I_Error ( "R_DrawTranslucentColumnD: %i to %i at %i",
				  dc_yl, dc_yh, dc_x);
	}
	
#endif 

	int fga, bga;
	{
		fixed_t fglevel, bglevel;

		fglevel = dc_translevel & ~0x3ff;
		bglevel = FRACUNIT-fglevel;

		fga = fglevel >> 8;
		bga = bglevel >> 8;
	}

	dest = (argb_t *)(ylookup[dc_yl] + columnofs[dc_x]);

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	{
		byte *source = dc_source;
		int pitch = dc_pitch / sizeof(argb_t);
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;

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
				argb_t fg = dc_colormap.shade(source[(frac>>FRACBITS)]);
				argb_t bg = *dest;
				*dest = alphablend2a(bg, bga, fg, fga);
				dest += pitch;
				
				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				argb_t fg = dc_colormap.shade(source[(frac>>FRACBITS)&mask]);
				argb_t bg = *dest;
				*dest = alphablend2a(bg, bga, fg, fga);
				dest += pitch;

				frac += fracstep;
			} while (--count);
		}
	}
}

void R_DrawTranslatedColumnD (void)
{
	int 				count;
	argb_t*		dest;
	fixed_t 			frac;
	fixed_t 			fracstep;

	count = dc_yh - dc_yl;
	if (count < 0)
		return;
	count++;

#ifdef RANGECHECK
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height)
	{
		I_Error ( "R_DrawTranslatedColumnD: %i to %i at %i",
				  dc_yl, dc_yh, dc_x);
	}
	
#endif


	dest = (argb_t *)(ylookup[dc_yl] + columnofs[dc_x]);

	fracstep = dc_iscale;
	frac = dc_texturefrac;

	// Here we do an additional index re-mapping.
	{
		byte *source = dc_source;
		int pitch = dc_pitch / sizeof(argb_t);
		int texheight = dc_textureheight;
		int mask = (texheight >> FRACBITS) - 1;

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
				*dest = dc_colormap.tlate(dc_translation, source[(frac>>FRACBITS)]);
				dest += pitch;

				if ((frac += fracstep) >= texheight)
					frac -= texheight;
			} while(--count);
		}
		else
		{		
			// texture height is a power-of-2
			do
			{
				*dest = dc_colormap.tlate(dc_translation, source[(frac>>FRACBITS) & mask]);
				dest += pitch;
			
				frac += fracstep;
			} while (--count);
		}
	}
}

// [RH] Just fills a column with a color
void R_FillColumnD (void)
{
	int 				count;
	argb_t*				dest;

	count = dc_yh - dc_yl + 1;

	if (count <= 0)
		return;

#ifdef RANGECHECK 
	if (dc_x >= screen->width
		|| dc_yl < 0
		|| dc_yh >= screen->height) {
		Printf (PRINT_HIGH, "R_FillColumnD: %i to %i at %i\n", dc_yl, dc_yh, dc_x);
		return;
	}
#endif

	dest = (argb_t *)(ylookup[dc_yl] + columnofs[dc_x]);

	{
		int pitch = dc_pitch / sizeof(argb_t);
		argb_t color = dc_colormap.shade(dc_color);

		do
		{
			*dest = color;
			dest += pitch;
		} while (--count);
	}
}

// [RH] Just fill a span with a color
void R_FillSpanD (void)
{
	int 				count;
	argb_t*				dest;
	
	count = ds_x2 - ds_x1 + 1;

	if (count <= 0)
		return;

#ifdef RANGECHECK
	if (ds_x2 < ds_x1
		|| ds_x1<0
		|| ds_x2>=screen->width
		|| ds_y>screen->height)
	{
		I_Error( "R_FillSpanD: %i to %i at %i",
				 ds_x1,ds_x2,ds_y);
	}
#endif

	dest = (argb_t *)(ylookup[ds_y] + columnofs[ds_x1]);
	
	{
		argb_t color = dc_colormap.shade(ds_color);
		int colsize = ds_colsize;

		do
		{
			*dest = color;
			dest += colsize;
		} while (--count);
	}
}

/****************************************************/
/****************************************************/

//
// R_InitTranslationTables
// Creates the translation tables to map
//	the green color ramp to gray, brown, red.
// Assumes a given structure of the PLAYPAL.
// Could be read from a lump instead.
//
byte *Ranges;

static byte *translationtablesmem = NULL;

void R_InitTranslationTables (void)
{
	static const char ranges[23][8] = {
		"CRBRICK",
		"CRTAN",
		"CRGRAY",
		"CRGREEN",
		"CRBROWN",
		"CRGOLD",
		"CRRED",
		"CRBLUE2",
		{ 'C','R','O','R','A','N','G','E' },
		"CRGRAY", // "White"
		{ 'C','R','Y','E','L','L','O','W' },
		"CRRED", // "Untranslated"
		"CRGRAY", // "Black"
		"CRBLUE",
		"CRTAN", // "Cream"
		"CRGREEN", // "Olive"
		"CRGREEN", // "Dark Green"
		"CRRED", // "Dark Red"
		"CRBROWN", // "Dark Brown"
		"CRRED", // "Purple"
		"CRGRAY", // "Dark Gray"
		"CRBLUE" // "Cyan"
	};
	int i;
	
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

	for (i = 0; i < 256; i++)
		translationtables[i] = i;

	// Set up default translationRGB tables:
	palette_t *pal = GetDefaultPalette();
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		for (int j = 0x70; j < 0x80; ++j)
			translationRGB[i][j - 0x70] = pal->basecolors[j];
	}

	for (i = 1; i < MAXPLAYERS+3; i++)
		memcpy (translationtables + i*256, translationtables, 256);

	// create translation tables for dehacked patches that expect them
	for (i = 0x70; i < 0x80; i++) {
		// map green ramp to gray, brown, red
		translationtables[i+(MAXPLAYERS+0)*256] = 0x60 + (i&0xf);
		translationtables[i+(MAXPLAYERS+1)*256] = 0x40 + (i&0xf);
		translationtables[i+(MAXPLAYERS+2)*256] = 0x20 + (i&0xf);
	}

	Ranges = translationtables + (MAXPLAYERS+3)*256;
	for (i = 0; i < 22; i++)
		W_ReadLump (W_GetNumForName (ranges[i]), Ranges + 256 * i);

}

void R_FreeTranslationTables (void)
{
    delete[] translationtablesmem;
    translationtablesmem = NULL;
}

// [Nes] Vanilla player translation table.
void R_BuildClassicPlayerTranslation (int player, int color)
{
	palette_t *pal = GetDefaultPalette();
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
void R_BuildPlayerTranslation (int player, int color)
{
	palette_t *pal = GetDefaultPalette();
	byte *table = &translationtables[player * 256];
	int i;
	float r = (float)RPART(color) / 255.0f;
	float g = (float)GPART(color) / 255.0f;
	float b = (float)BPART(color) / 255.0f;
	float h, s, v;
	float sdelta, vdelta;

	RGBtoHSV (r, g, b, &h, &s, &v);

	s -= 0.23f;
	if (s < 0.0f)
		s = 0.0f;
	sdelta = 0.014375f;

	v += 0.1f;
	if (v > 1.0f)
		v = 1.0f;
	vdelta = -0.05882f;

	for (i = 0x70; i < 0x80; i++) {
		HSVtoRGB (&r, &g, &b, h, s, v);

		// Set up RGB values for 32bpp translation:
		translationRGB[player][i - 0x70] = MAKERGB(
			(int)(r * 255.0f),
			(int)(g * 255.0f),
			(int)(b * 255.0f)
		);

		table[i] = BestColor (pal->basecolors,
							  (int)(r * 255.0f),
							  (int)(g * 255.0f),
							  (int)(b * 255.0f),
							  pal->numcolors);
		s += sdelta;
		if (s > 1.0f) {
			s = 1.0f;
			sdelta = 0.0f;
		}

		v += vdelta;
		if (v < 0.0f) {
			v = 0.0f;
			vdelta = 0.0f;
		}
	}
}


//
// R_InitBuffer 
// Creats lookup tables that avoid
//  multiplies and other hazzles
//  for getting the framebuffer address
//  of a pixel to draw.
//
void
R_InitBuffer
( int		width,
  int		height ) 
{ 
	int 		i;
	byte		*buffer;
	int			pitch;
	int			xshift;

	// Handle resize,
	//	e.g. smaller view windows
	//	with border and/or status bar.
	viewwindowx = (screen->width-(width<<detailxshift))>>1;

	// [RH] Adjust column offset according to bytes per pixel
	//		and detail mode
	xshift = (screen->is8bit()) ? 0 : 2;
	xshift += detailxshift;

	// Column offset. For windows
	for (i = 0; i < width; i++)
		columnofs[i] = (viewwindowx + i) << xshift;

	// Same with base row offset.
	if ((width<<detailxshift) == screen->width)
		viewwindowy = 0;
	else
		viewwindowy = (ST_Y-(height<<detailyshift)) >> 1;

	screen->Lock ();
	buffer = screen->buffer;
	pitch = screen->pitch;
	screen->Unlock ();

	// Precalculate all row offsets.
	for (i=0 ; i<height ; i++)
		ylookup[i] = buffer + ((i<<detailyshift)+viewwindowy)*pitch;
}


void R_DrawBorder (int x1, int y1, int x2, int y2)
{
	int lump;

	lump = W_CheckNumForName (gameinfo.borderFlat, ns_flats);
	if (lump >= 0)
	{
		screen->FlatFill (x1 & ~63, y1, x2, y2,
			(byte *)W_CacheLumpNum (lump, PU_CACHE));
	}
	else
	{
		screen->Clear (x1, y1, x2, y2, 0);
	}
}


//
// R_DrawViewBorder
// Draws the border around the view
//  for different size windows?
//
void V_MarkRect (int x, int y, int width, int height);

void R_DrawViewBorder (void)
{
	int x, y;
	int offset, size;
	gameborder_t *border;

	if (realviewwidth == screen->width) {
		return;
	}

	border = gameinfo.border;
	offset = border->offset;
	size = border->size;

	R_DrawBorder (0, 0, screen->width, viewwindowy);
	R_DrawBorder (0, viewwindowy, viewwindowx, realviewheight + viewwindowy);
	R_DrawBorder (viewwindowx + realviewwidth, viewwindowy, screen->width, realviewheight + viewwindowy);
	R_DrawBorder (0, viewwindowy + realviewheight, screen->width, ST_Y);

	for (x = viewwindowx; x < viewwindowx + realviewwidth; x += size)
	{
		screen->DrawPatch (W_CachePatch (border->t),
			x, viewwindowy - offset);
		screen->DrawPatch (W_CachePatch (border->b),
			x, viewwindowy + realviewheight);
	}
	for (y = viewwindowy; y < viewwindowy + realviewheight; y += size)
	{
		screen->DrawPatch (W_CachePatch (border->l),
			viewwindowx - offset, y);
		screen->DrawPatch (W_CachePatch (border->r),
			viewwindowx + realviewwidth, y);
	}
	// Draw beveled edge.
	screen->DrawPatch (W_CachePatch (border->tl),
		viewwindowx-offset, viewwindowy-offset);
	
	screen->DrawPatch (W_CachePatch (border->tr),
		viewwindowx+realviewwidth, viewwindowy-offset);
	
	screen->DrawPatch (W_CachePatch (border->bl),
		viewwindowx-offset, viewwindowy+realviewheight);
	
	screen->DrawPatch (W_CachePatch (border->br),
		viewwindowx+realviewwidth, viewwindowy+realviewheight);

	V_MarkRect (0, 0, screen->width, ST_Y);
}

// [RH] Double pixels in the view window horizontally
//		and/or vertically (or not at all).
void R_DetailDouble (void)
{
	switch ((detailxshift << 1) | detailyshift)
	{
		case 1:		// y-double
		{
			int rowsize = realviewwidth << ((screen->is8bit()) ? 0 : 2);
			int pitch = screen->pitch;
			int y;
			byte *line;

			line = screen->buffer + viewwindowy*pitch + viewwindowx;
			for (y = 0; y < viewheight; y++, line += pitch<<1)
			{
				memcpy (line+pitch, line, rowsize);
			}
		}
		break;

		case 2:		// x-double
		{
			int rowsize = realviewwidth >> 2;
			int pitch = screen->pitch >> (2-detailyshift);
			int y,x;
			unsigned *line,a,b;

			line = (unsigned *)(screen->buffer + viewwindowy*screen->pitch + viewwindowx);
			for (y = 0; y < viewheight; y++, line += pitch)
			{
				for (x = 0; x < rowsize; x += 2)
				{
					a = line[x+0];
					b = line[x+1];
					a &= 0x00ff00ff;
					b &= 0x00ff00ff;
					line[x+0] = a | (a << 8);
					line[x+1] = b | (b << 8);
				}
			}
		}
		break;

		case 3:		// x- and y-double
		{
			int rowsize = realviewwidth >> 2;
			int pitch = screen->pitch >> (2-detailyshift);
			int realpitch = screen->pitch >> 2;
			int y,x;
			unsigned *line,a,b;

			line = (unsigned *)(screen->buffer + viewwindowy*screen->pitch + viewwindowx);
			for (y = 0; y < viewheight; y++, line += pitch)
			{
				for (x = 0; x < rowsize; x += 2)
				{
					a = line[x+0];
					b = line[x+1];
					a &= 0x00ff00ff;
					b &= 0x00ff00ff;
					line[x+0] = a | (a << 8);
					line[x+0+realpitch] = a | (a << 8);
					line[x+1] = b | (b << 8);
					line[x+1+realpitch] = b | (b << 8);
				}
			}
		}
		break;
	}
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
	std::string list;
	std::vector<r_optimize_kind>::iterator it = optimizations_available.begin();
	if (!includeNone) ++it;
	for (; it != optimizations_available.end(); ++it)
	{
		list.append(get_optimization_name(*it));
		if (it+1 != optimizations_available.end())
			list.append(", ");
	}
	return list;
}

static void print_optimizations()
{
	Printf(PRINT_HIGH, "r_optimize detected \"%s\"\n", get_optimization_name_list(false).c_str());
}

static bool detect_optimizations()
{
	if (optimizations_available.size() != 0)
		return false;

	optimizations_available.clear();

	// Start with default non-optimized:
	optimizations_available.push_back(OPTIMIZE_NONE);

	// Detect CPU features in ascending order of preference:
#ifdef __MMX__
	if (SDL_HasMMX())
	{
		optimizations_available.push_back(OPTIMIZE_MMX);
	}
#endif

#ifdef __SSE2__
	if (SDL_HasSSE2())
	{
		optimizations_available.push_back(OPTIMIZE_SSE2);
	}
#endif

#ifdef __ALTIVEC__
	if (SDL_HasAltiVec())
	{
		optimizations_available.push_back(OPTIMIZE_ALTIVEC);
	}
#endif

	return true;
}

CVAR_FUNC_IMPL (r_optimize)
{
	// NOTE(jsd): Stupid hack to prevent stack overflow when trying to set the value from within this callback.
	static bool resetting = false;
	if (resetting)
	{
		resetting = false;
		return;
	}

	const char *val = var.cstring();
	//Printf(PRINT_HIGH, "r_optimize called with \"%s\"\n", val);

	// Only print the detected list the first time:
	if (detect_optimizations())
		print_optimizations();

	// Set the optimization based on availability:
	r_optimize_kind trykind = optimize_kind;
	if (stricmp(val, "none") == 0)
		trykind = OPTIMIZE_NONE;
	else if (stricmp(val, "sse2") == 0)
		trykind = OPTIMIZE_SSE2;
	else if (stricmp(val, "mmx") == 0)
		trykind = OPTIMIZE_MMX;
	else if (stricmp(val, "altivec") == 0)
		trykind = OPTIMIZE_ALTIVEC;
	else if (stricmp(val, "detect") == 0)
		// Default to the most preferred:
		trykind = optimizations_available.back();
	else
	{
		Printf(PRINT_HIGH, "Invalid value for r_optimize. Try one of \"%s, detect\"\n", get_optimization_name_list(true).c_str());

		// Restore the original setting:
		resetting = true;
		var.Set(get_optimization_name(optimize_kind));
		R_InitDrawers();
		R_InitColumnDrawers();
		return;
	}

	// If we found the CPU feature, use it:
	std::vector<r_optimize_kind>::iterator it = std::find(optimizations_available.begin(), optimizations_available.end(), trykind);
	if (it != optimizations_available.end())
	{
		optimize_kind = trykind;
		R_InitDrawers();
		R_InitColumnDrawers();
	}

	// Update the cvar string:
	const char *resetname = get_optimization_name(optimize_kind);
	Printf(PRINT_HIGH, "r_optimize set to \"%s\" based on availability\n", resetname);
	resetting = true;
	var.Set(resetname);
}

// Sets up the r_*D function pointers based on CPU optimization selected
void R_InitDrawers ()
{
	if (optimize_kind == OPTIMIZE_SSE2)
	{
#ifdef __SSE2__
		rtv_lucent4colsP        = rtv_lucent4cols_SSE2;
		rtv_lucent4colsD        = rtv_lucent4cols_SSE2;
		R_DrawSpanD				= R_DrawSpanD_SSE2;
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_SSE2;
		r_dimpatchD             = r_dimpatchD_SSE2;
#else
		// No SSE2 support compiled in.
		optimize_kind = OPTIMIZE_NONE;
		goto setNone;
#endif
	}
	else if (optimize_kind == OPTIMIZE_MMX)
	{
#ifdef __MMX__
		rtv_lucent4colsP        = rtv_lucent4cols_MMX;
		rtv_lucent4colsD        = rtv_lucent4cols_MMX;
		R_DrawSpanD				= R_DrawSpanD_c;		// TODO
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_c;	// TODO
		r_dimpatchD             = r_dimpatchD_MMX;
#else
		// No MMX support compiled in.
		optimize_kind = OPTIMIZE_NONE;
		goto setNone;
#endif
	}
	else if (optimize_kind == OPTIMIZE_ALTIVEC)
	{
#ifdef __ALTIVEC__
		rtv_lucent4colsP        = rtv_lucent4cols_ALTIVEC;
		rtv_lucent4colsD        = rtv_lucent4cols_ALTIVEC;
		R_DrawSpanD				= R_DrawSpanD_c;		// TODO
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_c;	// TODO
		r_dimpatchD             = r_dimpatchD_ALTIVEC;
#else
		// No ALTIVEC support compiled in.
		optimize_kind = OPTIMIZE_NONE;
		goto setNone;
#endif
	}
	else
	{
		// No CPU vectorization available.
setNone:
		rtv_lucent4colsP        = rtv_lucent4cols_c;
		rtv_lucent4colsD        = rtv_lucent4cols_c;
		R_DrawSpanD				= R_DrawSpanD_c;
		R_DrawSlopeSpanD		= R_DrawSlopeSpanD_c;
		r_dimpatchD             = r_dimpatchD_c;
	}

	// Check that all pointers are definitely assigned!
	assert(rtv_lucent4colsP != NULL);
	assert(rtv_lucent4colsD != NULL);
	assert(R_DrawSpanD != NULL);
	assert(R_DrawSlopeSpanD != NULL);
	assert(r_dimpatchD != NULL);
}

// [RH] Initialize the column drawer pointers
void R_InitColumnDrawers ()
{
	if (!screen)
		return;

	// NOTE(jsd): It's okay to use R_DrawColumnHorizP because it renders to a temp buffer first.
	R_DrawColumnHoriz		= R_DrawColumnHorizP;

	if (screen->is8bit())
	{
		R_DrawColumn			= R_DrawColumnP;
		R_DrawFuzzColumn		= R_DrawFuzzColumnP;
		R_DrawTranslucentColumn = R_DrawTranslucentColumnP;
		R_DrawTranslatedColumn	= R_DrawTranslatedColumnP;
		R_DrawSlopeSpan         = R_DrawSlopeSpanP;
		R_DrawSpan				= R_DrawSpanP;
		R_FillColumn			= R_FillColumnP;
		R_FillSpan				= R_FillSpanP;

		rt_copy1col             = rt_copy1colP;
		rt_copy2cols            = rt_copy2colsP;
		rt_copy4cols            = rt_copy4colsP;
		rt_map1col              = rt_map1colP;
		rt_map2cols             = rt_map2colsP;
		rt_map4cols             = rt_map4colsP;
		rt_lucent1col			= rt_lucent1colP;
		rt_lucent2cols          = rt_lucent2colsP;
		rt_lucent4cols          = rt_lucent4colsP;
		rt_tlate1col            = rt_tlate1colP;
		rt_tlate2cols           = rt_tlate2colsP;
		rt_tlate4cols           = rt_tlate4colsP;
		rt_tlatelucent1col      = rt_tlatelucent1colP;
		rt_tlatelucent2cols     = rt_tlatelucent2colsP;
		rt_tlatelucent4cols     = rt_tlatelucent4colsP;
	}
	else
	{
		// 32bpp rendering functions:
		R_DrawColumn			= R_DrawColumnD;
		R_DrawFuzzColumn		= R_DrawFuzzColumnD;
		R_DrawTranslucentColumn = R_DrawTranslucentColumnD;
		R_DrawTranslatedColumn	= R_DrawTranslatedColumnD;
		R_DrawSlopeSpan			= R_DrawSlopeSpanD;
		R_DrawSpan				= R_DrawSpanD;
		R_FillColumn			= R_FillColumnD;
		R_FillSpan				= R_FillSpanD;
		
		rt_copy1col             = rt_copy1colD;
		rt_copy2cols            = rt_copy2colsD;
		rt_copy4cols            = rt_copy4colsD;
		rt_map1col              = rt_map1colD;
		rt_map2cols             = rt_map2colsD;
		rt_map4cols             = rt_map4colsD;
		rt_lucent1col			= rt_lucent1colD;
		rt_lucent2cols          = rt_lucent2colsD;
		rt_lucent4cols          = rt_lucent4colsD;
		rt_tlate1col            = rt_tlate1colD;
		rt_tlate2cols           = rt_tlate2colsD;
		rt_tlate4cols           = rt_tlate4colsD;
		rt_tlatelucent1col      = rt_tlatelucent1colD;
		rt_tlatelucent2cols     = rt_tlatelucent2colsD;
		rt_tlatelucent4cols     = rt_tlatelucent4colsD;
	}
}

VERSION_CONTROL (r_draw_cpp, "$Id$")

