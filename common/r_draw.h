// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: r_draw.h 1837 2010-09-02 04:21:09Z spleen $
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __R_DRAW__
#define __R_DRAW__

#include "r_intrin.h"
#include "r_defs.h"

typedef struct 
{
	byte*				source;
	byte*				destination;

	int					pitch_in_pixels;

	tallpost_t*			post;

	shaderef_t			colormap;

	int					x;
	int					yl;
	int					yh;
	
	fixed_t				iscale;
	fixed_t				texturemid;
	fixed_t				texturefrac;
	fixed_t				textureheight;

	fixed_t				translevel;

	translationref_t	translation;

	palindex_t			color;				// for r_drawflat
} drawcolumn_t;

extern "C" drawcolumn_t dcol;

typedef struct
{
	byte*				source;
	byte*				destination;

	int					pitch_in_pixels;

	shaderef_t			colormap;

	int					y;
	int					x1;
	int					x2;

	dsfixed_t			xfrac;
	dsfixed_t			yfrac;
	dsfixed_t			xstep;
	dsfixed_t			ystep;

	float				iu;
	float				iv;
	float				id;
	float				iustep;
	float				ivstep;
	float				idstep;

	fixed_t				translevel;

	shaderef_t			slopelighting[MAXWIDTH];

	palindex_t			color;
} drawspan_t;

extern "C" drawspan_t dspan;


// [RH] Temporary buffer for column drawing

void R_RenderColumnRange(int start, int stop, int* top, int* bottom,
		tallpost_t** posts, void (*colblast)(), bool calc_light, int columnmethod);

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
extern void (*R_FillTranslucentSpan)(void);

// [RH] Initialize the above function pointers
void R_InitColumnDrawers ();

void R_InitVectorizedDrawers();

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
void	R_BlankSpan (void);
void	R_FillSpanP (void);
void	R_FillSpanD (void);

void R_DrawSpanD_c(void);
void R_DrawSlopeSpanD_c(void);

#define SPANJUMP 16
#define INTERPSTEP (0.0625f)

class IWindowSurface;

void r_dimpatchD_c(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h);

#ifdef __SSE2__
void R_DrawSpanD_SSE2(void);
void R_DrawSlopeSpanD_SSE2(void);
void r_dimpatchD_SSE2(IWindowSurface*, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

#ifdef __MMX__
void R_DrawSpanD_MMX(void);
void R_DrawSlopeSpanD_MMX(void);
void r_dimpatchD_MMX(IWindowSurface*, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

#ifdef __ALTIVEC__
void R_DrawSpanD_ALTIVEC(void);
void R_DrawSlopeSpanD_ALTIVEC(void);
void r_dimpatchD_ALTIVEC(IWindowSurface*, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

// Vectorizable function pointers:
extern void (*R_DrawSpanD)(void);
extern void (*R_DrawSlopeSpanD)(void);
extern void (*r_dimpatchD)(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h);

extern byte*			translationtables;
extern argb_t           translationRGB[MAXPLAYERS+1][16];

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


// Initialize color translation tables,
//	for player rendering etc.
void R_InitTranslationTables (void);
void R_FreeTranslationTables (void);

void R_CopyTranslationRGB (int fromplayer, int toplayer);

// [RH] Actually create a player's translation table.
void R_BuildPlayerTranslation(int player, argb_t dest_color);

// [Nes] Classic player translation table.
void R_BuildClassicPlayerTranslation(int player, int color);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder (void);
void R_DrawBorder (int x1, int y1, int x2, int y2);


#endif


