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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include "r_intrin.h"
#include "r_defs.h"

typedef struct
{
	const palindex_t*	source;

	// palletized texture data
	const palindex_t*	texturedata;

	// raw argb texture data (null if not argb)
	const argb_t*		argbtexturedata;

	uint8_t*			destination;

	// Steps, in pixels, to the neighbouring element one row down and one column
	// right. The view buffer is column-major, so this is (1, pitch).
	// pitch_in_pixels stays because some callers read it directly -- the fuzz
	// effect samples a screen row away, R_DrawPixel addresses with it.
	int					pitch_in_pixels;
	int					colstep;

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
	bool				masked;
} drawcolumn_t;

extern "C" drawcolumn_t dcol;

typedef struct
{
	const palindex_t*	source;

	// null if no argb data
	const argb_t*		argbsource;

	uint8_t*			destination;

	// See drawcolumn_t: row step then column step.
	int					pitch_in_pixels;
	int					colstep;

	shaderef_t			colormap;

	int					y;
	int					x1;
	int					x2;

	dsfixed_t			ufrac;
	dsfixed_t			vfrac;
	dsfixed_t			ustep;
	dsfixed_t			vstep;

	int					umask;
	int					vmask;
	int					ushift;
	int					vshift;

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


// ----------------------------------------------------------------------------
//
// Level plane column drawing
//
// Spans need no per-pixel divide, because a screen row of a level plane has
// constant depth -- but against a column-major framebuffer every pixel of a span
// is a separate cache line.
//
// Columns need no divide either. Collecting R_MapLevelPlane's arithmetic on
// distance rather than on x gives, for a fixed column,
//
//     u(y) = ubase + eu * yslope[y]
//     v(y) = vbase + ev * yslope[y]
//
// so the whole dependence on y is one yslope[] load.
//
// ----------------------------------------------------------------------------

typedef struct
{
	const palindex_t* source;
	const argb_t*     argbsource;  // null if the texture carries no ARGB plane

	uint8_t*    destination;

	// See drawcolumn_t: row step then column step.
	int         pitch_in_pixels;
	int         colstep;

	int         x;
	int         yl;
	int         yh;

	// Already scaled so that (ubase + eu * yslope[y]) is a 16.16 value: yslope
	// is 16.16, so FIXED2DOUBLE's 1/65536 and the conversion's 65536 cancel and
	// eu/ev multiply the raw table entry.
	double				ubase, vbase;
	double				eu, ev;

	int					umask, vmask;
	int					ushift, vshift;

	// Lighting. A level plane's colormap is a pure function of screen y -- x
	// appears nowhere in R_MapLevelPlane's selection -- so one table serves the
	// whole plane. Entries are the relative colormap index pre-multiplied by 256,
	// so cbase[lightoff[y] + c] is basecolormap.with(rel).index(c).
	const uint16_t*		lightoff;		// indexed by view-relative y, as yl/yh are;
									// NULL when the plane is one flat colormap
	const palindex_t*	cbase;			// basecolormap.m_colormap
	const argb_t*		sbase;			// basecolormap.m_shademap

	fixed_t				translevel;
} drawplanecol_t;

extern "C" drawplanecol_t dpcol;

// ----------------------------------------------------------------------------
//
// Level planes, four rows at a time
//
// Walking DOWN a column makes yslope[y] vary per pixel -- a reciprocal ramp, not
// linear in y -- so there is no DDA and every pixel pays two double multiplies
// and two cvttsd2si. That is why the scalar column drawer costs 28 instructions
// per pixel against the SIMD span drawer's 8.75.
//
// But a 16-byte store only needs its four pixels VERTICALLY ADJACENT; it does not
// need the loop to walk down a column. So put four consecutive screen rows in the
// four lanes and march in x instead:
//
//   * yslope[y] becomes a per-LANE CONSTANT, so the mapping is R_MapLevelPlane's
//     own 16.16 integer DDA again -- one paddd. Exactly: the 16.16 step is
//     eu_step * yslope[y], a per-row constant independent of x.
//
//   * lightoff is a pure function of y, and y is now the lane index, so per-y
//     lighting collapses into four loop-invariant colormap bases and costs
//     nothing. No CONSTLIGHT variant is needed -- constant light is just
//     shade[0] == shade[1] == shade[2] == shade[3].
//
// Four rows of one column are 16 contiguous bytes at 32bpp, which is what makes
// this worth doing.
//
// ----------------------------------------------------------------------------

typedef struct
{
	const palindex_t*	source;

	uint8_t*			destination;
	int					colstep;		// pixels from one column to the next

	int					xa, xb;			// inclusive column range to march
	int					y0;				// first of the four rows

	// Lane i carries screen row y0 + i. ufrac/vfrac are anchored at column xa
	// with the same expression the scalar column drawer uses, so the anchor is
	// bit-identical to it; the steps are R_MapLevelPlane's.
	dsfixed_t			ufrac[4], vfrac[4];
	dsfixed_t			ustep[4], vstep[4];

	int					umask, vmask;
	int					ushift, vshift;

	// Already offset by lightoff[y0 + i], so the drawer indexes them with the
	// raw texel. NULL lightoff folds to four copies of the same base.
	const argb_t*		shade[4];		// 32bpp
	const palindex_t*	cmap[4];		// 8bpp
} drawplanegroup_t;

extern "C" drawplanegroup_t dpgroup;



//
// R_PixelCeil
//
// ceil(num / den) in whole pixels, for raw fixed-point numbers where
// num >= 0 and den > 0.
// 
// For when you need to ceiling divide 16.16 floating point numbers and NOT
// discard remainders after a certain quotient.
//
// Used for calculating texturefrac post coordinates.
//
static inline int R_PixelCeil(fixed_t num, fixed_t den)
{
	return static_cast<int>((static_cast<int64_t>(num) + den - 1) / den);
}


// [RH] Temporary buffer for column drawing

void R_RenderColumnRange(int start, int stop, const int* top, const int* bottom,
		const palindex_t** posts, void (*colblast)(), bool calc_light, int columnmethod);

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

extern void (*R_DrawTlatedLucentColumn)(void);

// [EB] Draw sky foreground with palette 0 transparency
extern void (*R_DrawSkyForegroundColumn)(void);

// Span blitting for rows, floor/ceiling.
// No Sepctre effect needed.
extern void (*R_DrawSpan)(void);

// Textured spans blended over the framebuffer by dspan.translevel
// (stacked-sector portal boundary flats).
extern void (*R_DrawTranslucentSpan)(void);
extern void (*R_DrawTranslucentSlopeSpan)(void);

extern void (*R_DrawSlopeSpan)(void);

// Draws one column of a level plane. NULL whenever planes must go through the
// span path (r_drawflat and nodrawers)
extern void (*R_DrawLevelColumn)(void);

// Draws one four-row group of a level plane across [xa, xb].
extern void (*R_DrawLevelGroup)(void);
extern void (*R_DrawTranslucentLevelColumn)(void);

extern void (*R_FillColumn)(void);
extern void (*R_FillSpan)(void);
extern void (*R_FillTranslucentSpan)(void);

// [RH] Initialize the above function pointers
void R_InitColumnDrawers ();

void R_InitVectorizedDrawers();


void	R_BlankColumn (void);
void	R_BlankSpan (void);

void R_DrawLevelGroupD_c(void);

void R_UpdateARGBShadeLUT(void);

#define SPANJUMP 16
#define INTERPSTEP (0.0625f)

class IWindowSurface;

void r_dimpatchD_c(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h);

// ----------------------------------------------------------------------------
//
// Transposed view buffer
//
// EVERY surface is column-major -- a screen column is a contiguous run of pixels
// -- and the GPU turns the primary one the right way round at present time. This
// is not a setting: there is one drawer table and it addresses the view
// column-major.
//
// That includes every secondary surface: the scaled status bar, patch canvases,
// the wipe capture, the vid_320x200 / vid_640x400 emulated surface. IWindowSurface
// still exposes a row/column step pair, but only as the vocabulary for walking a
// surface -- the pair is now always (1, pitch), and nothing branches on it.
//
// ----------------------------------------------------------------------------

// Points dcol/dspan/dpcol at the view rect of the surface.
void R_BindViewBuffer(IWindowSurface* surface);

// Fills the view with a solid colour (r_flashhom).
void R_ClearViewBuffer(IWindowSurface* surface, argb_t color);

#ifdef __SSE2__
// Four vertically adjacent pixels per iteration -- see R_DrawColumnQuadGeneric.
// Both read dcol, and both require a 32bpp column-major destination.
void R_DrawColumnD_SSE2(void);
void R_DrawTranslucentColumnD_SSE2(void);
void R_DrawLevelGroupD_SSE2(void);
void r_dimpatchD_SSE2(IWindowSurface*, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

#ifdef __MMX__
void r_dimpatchD_MMX(IWindowSurface*, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

#ifdef __ALTIVEC__
void r_dimpatchD_ALTIVEC(IWindowSurface*, argb_t color, int alpha, int x1, int y1, int w, int h);
#endif

// Vectorizable function pointers:
extern void (*R_DrawLevelGroupD)(void);
extern void (*r_dimpatchD)(IWindowSurface* surface, argb_t color, int alpha, int x1, int y1, int w, int h);

inline byte bosstable[256];
inline byte friendtable[256];
inline byte greentable[MAXPLAYERS+1][256];
inline byte redtable[MAXPLAYERS + 1][256];
inline byte*			translationtables;
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

constexpr int MAX_ACS_TRANSLATIONS = 32;


// Initialize color translation tables,
//	for player rendering etc.
void R_InitTranslationTables (void);
void R_FreeTranslationTables (void);

void R_CopyTranslationRGB (int fromplayer, int toplayer);
void R_RebuildPlayerTintTables(int player);

// [RH] Actually create a player's translation table.
void R_BuildPlayerTranslation(int player, argb_t dest_color, int colorpreset);

// [Nes] Classic player translation table.
void R_BuildClassicPlayerTranslation(int player, int color);

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder (void);
void R_DrawBorder (int x1, int y1, int x2, int y2);
