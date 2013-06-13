// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_draw.h 1837 2010-09-02 04:21:09Z spleen $
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_intrin.h"
#include "r_defs.h"

extern "C" byte**		ylookup;
extern "C" int*			columnofs;

extern "C" int			dc_pitch;		// [RH] Distance between rows

extern "C" shaderef_t	dc_colormap;
extern "C" int			dc_x;
extern "C" int			dc_yl;
extern "C" int			dc_yh;
extern "C" fixed_t		dc_iscale;
extern "C" fixed_t		dc_texturemid;
extern "C" fixed_t		dc_texturefrac;
extern "C" fixed_t		dc_textureheight;
extern "C" int			dc_color;		// [RH] For flat colors (no texturing)

// first pixel in a column
extern "C" byte*			dc_source;

extern "C" tallpost_t*		dc_topposts[MAXWIDTH];
extern "C" tallpost_t*		dc_midposts[MAXWIDTH];
extern "C" tallpost_t*		dc_bottomposts[MAXWIDTH];

// [RH] Temporary buffer for column drawing
extern "C" byte			dc_temp[MAXHEIGHT * 4];
extern "C" unsigned int	dc_tspans[4][256];
extern "C" unsigned int	*dc_ctspan[4];
extern "C" unsigned int	horizspans[4];

void R_RenderColumnRange(int start, int stop, bool columnmethod, void (*colblast)(), void (*hcolblast)(), bool calc_light);

// [RH] Pointers to the different column and span drawers...

// The span blitting interface.
// Hook in assembler or system specific BLT here.
extern void (*R_DrawColumn)(void);

// The Spectre/Invisibility effect.
extern void (*R_DrawFuzzColumn)(void);

// [RH] Draw translucent column;
extern void (*R_DrawTranslucentColumn)(void);

// Draw with color translation tables,
//	for player sprite rendering,
//	Green/Red/Blue/Indigo shirts.
extern void (*R_DrawTranslatedColumn)(void);

// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
extern void (*R_DrawSpan)(void);

extern void (*R_DrawSlopeSpan)(void);

extern void (*R_FillColumn)(void);
extern void (*R_FillSpan)(void);

// [RH] Span blit into an interleaved intermediate buffer
extern void (*R_DrawColumnHoriz)(void);
void R_DrawMaskedColumnHoriz(tallpost_t *post);

// [RH] Initialize the above function pointers
void R_InitColumnDrawers ();

void R_InitDrawers ();

void	R_DrawColumnHorizP (void);
void	R_DrawColumnP (void);
void	R_DrawFuzzColumnP (void);
void	R_DrawTranslucentColumnP (void);
void	R_DrawTranslatedColumnP (void);
void	R_DrawSpanP (void);
void	R_DrawSlopeSpanIdealP_C (void);

void	R_DrawColumnD (void);
void	R_DrawFuzzColumnD (void);
void	R_DrawTranslucentColumnD (void);
void	R_DrawTranslatedColumnD (void);

void	R_DrawTlatedLucentColumnP (void);
#define R_DrawTlatedLucentColumn R_DrawTlatedLucentColumnP
void	R_StretchColumnP (void);
#define R_StretchColumn R_StretchColumnP

void	R_BlankColumn (void);
void	R_FillColumnP (void);
void	R_FillColumnHorizP (void);
void	R_BlankSpan (void);
void	R_FillSpanP (void);
void	R_FillSpanD (void);

// [RH] Moves data from the temporary horizontal buffer to the screen.
void rt_copy1colP (int hx, int sx, int yl, int yh);
void rt_copy4colsP (int sx, int yl, int yh);
void rt_map1colP (int hx, int sx, int yl, int yh);
void rt_map4colsP (int sx, int yl, int yh);
void rt_lucent1colP (int hx, int sx, int yl, int yh);
void rt_lucent4colsP (int sx, int yl, int yh);
void rt_tlate1colP (int hx, int sx, int yl, int yh);
void rt_tlate4colsP (int sx, int yl, int yh);
void rt_tlatelucent1colP (int hx, int sx, int yl, int yh);
void rt_tlatelucent4colsP (int sx, int yl, int yh);

void rt_copy1colD (int hx, int sx, int yl, int yh);
void rt_copy4colsD (int sx, int yl, int yh);
void rt_map1colD (int hx, int sx, int yl, int yh);
void rt_map4colsD (int sx, int yl, int yh);
void rt_lucent1colD (int hx, int sx, int yl, int yh);
void rt_lucent4colsD (int sx, int yl, int yh);
void rt_tlate1colD (int hx, int sx, int yl, int yh);
void rt_tlate4colsD (int sx, int yl, int yh);
void rt_tlatelucent1colD (int hx, int sx, int yl, int yh);
void rt_tlatelucent4colsD (int sx, int yl, int yh);

void R_DrawSpanD_c(void);
void R_DrawSlopeSpanD_c(void);

#define SPANJUMP 16
#define INTERPSTEP (0.0625f)

void rt_draw1col (int hx, int sx);
void rt_draw4cols (int sx);

// [RH] Preps the temporary horizontal buffer.
void rt_initcols (void);

// Vectorizable functions:

template<typename pixel_t>
void rtv_lucent4cols_c(byte *source, pixel_t *dest, int bga, int fga)
{
	for (int i = 0; i < 4; ++i)
	{
		const pixel_t fg = rt_mapcolor<pixel_t>(dc_colormap, source[i]);
		const pixel_t bg = dest[i];

		dest[i] = rt_blend2<pixel_t>(bg, bga, fg, fga);
	}
}

void r_dimpatchD_c(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h);

#ifdef __SSE2__
template<typename pixel_t>
void rtv_lucent4cols_SSE2(byte *source, pixel_t *dest, int bga, int fga);
void R_DrawSpanD_SSE2(void);
void R_DrawSlopeSpanD_SSE2(void);
void r_dimpatchD_SSE2(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

#ifdef __MMX__
template<typename pixel_t>
void rtv_lucent4cols_MMX(byte *source, pixel_t *dest, int bga, int fga);
void R_DrawSpanD_MMX(void);
void R_DrawSlopeSpanD_MMX(void);
void r_dimpatchD_MMX(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

#ifdef __ALTIVEC__
template<typename pixel_t>
void rtv_lucent4cols_ALTIVEC(byte *source, pixel_t *dest, int bga, int fga);
void R_DrawSpanD_ALTIVEC(void);
void R_DrawSlopeSpanD_ALTIVEC(void);
void r_dimpatchD_ALTIVEC(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

// Palettized (8bpp) vs. Direct (32bpp) switchable function pointers:
extern void (*rt_copy1col) (int hx, int sx, int yl, int yh);
extern void (*rt_copy4cols) (int sx, int yl, int yh);
extern void (*rt_map1col) (int hx, int sx, int yl, int yh);
extern void (*rt_map4cols) (int sx, int yl, int yh);
extern void (*rt_lucent1col) (int hx, int sx, int yl, int yh);
extern void (*rt_lucent4cols) (int sx, int yl, int yh);
extern void (*rt_tlate1col) (int hx, int sx, int yl, int yh);
extern void (*rt_tlate4cols) (int sx, int yl, int yh);
extern void (*rt_tlatelucent1col) (int hx, int sx, int yl, int yh);
extern void (*rt_tlatelucent4cols) (int sx, int yl, int yh);

// Vectorizable function pointers:
extern void (*rtv_lucent4colsP)(byte *source, palindex_t *dest, int bga, int fga);
extern void (*rtv_lucent4colsD)(byte *source, argb_t *dest, int bga, int fga);
extern void (*R_DrawSpanD)(void);
extern void (*R_DrawSlopeSpanD)(void);
extern void (*r_dimpatchD)(const DCanvas *const cvs, argb_t color, int alpha, int x1, int y1, int w, int h);

extern "C" int				ds_colsize;		// [RH] Distance between columns

extern "C" int				ds_y;
extern "C" int				ds_x1;
extern "C" int				ds_x2;

extern "C" shaderef_t		ds_colormap;

extern "C" dsfixed_t		ds_xfrac;
extern "C" dsfixed_t		ds_yfrac;
extern "C" dsfixed_t		ds_xstep;
extern "C" dsfixed_t		ds_ystep;

// start of a 64*64 tile image
extern "C" byte*			ds_source;

extern "C" int				ds_color;		// [RH] For flat color (no texturing)

// [SL] 2012-03-19 - For sloped planes
extern "C" float			ds_iu;
extern "C" float			ds_iv;
extern "C" float			ds_iustep;
extern "C" float			ds_ivstep;
extern "C" float			ds_id;
extern "C" float			ds_idstep;
extern "C" shaderef_t		slopelighting[MAXWIDTH];

extern byte*			translationtables;
extern translationref_t dc_translation;
extern argb_t           translationRGB[MAXPLAYERS+1][16];

extern fixed_t dc_translevel;

enum
{
	TRANSLATION_Shaded,
	TRANSLATION_Players,
	TRANSLATION_PlayersExtra,
	TRANSLATION_Standard,
	TRANSLATION_LevelScripted,
	TRANSLATION_Decals,

	NUM_TRANSLATION_TABLES
};

#define TRANSLATION(a,b)	(((a)<<8)|(b))

const int MAX_ACS_TRANSLATIONS = 32;



// [RH] Double view pixels by detail mode
void R_DetailDouble (void);

void
R_InitBuffer
( int		width,
  int		height );


// Initialize color translation tables,
//	for player rendering etc.
void R_InitTranslationTables (void);
void R_FreeTranslationTables (void);

void R_CopyTranslationRGB (int fromplayer, int toplayer);

// [RH] Actually create a player's translation table.
void R_BuildPlayerTranslation (int player, int color);

// [Nes] Classic player translation table.
void R_BuildClassicPlayerTranslation (int player, int color);


// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder (void);
void R_DrawBorder (int x1, int y1, int x2, int y2);

// [RH] Added for muliresolution support
void R_InitFuzzTable (void);


#endif


