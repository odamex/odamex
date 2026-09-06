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

#include <array>
#include <vector>

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
void	R_DrawTranslucentSpanP (void);
void	R_DrawTranslucentSpanD (void);
void	R_DrawTranslucentSlopeSpanP (void);
void	R_DrawTranslucentSlopeSpanD (void);
void	R_DrawSlopeSpanIdealP_C (void);

void	R_DrawColumnD (void);
void	R_DrawFuzzColumnD (void);
void	R_DrawTranslucentColumnD (void);
void	R_DrawTranslatedColumnD (void);

void	R_DrawTlatedLucentColumnP (void);
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

// The green ramp that player colors are built out of.
constexpr palindex_t PLAYER_COLOR_START = 0x70;
constexpr palindex_t PLAYER_COLOR_END = 0x7F;

//
// A self-contained color translation
//
// Unlike the global translationtables / translationRGB pair, which are fixed
// arrays indexed by player id and only ever recolor the player range, this owns
// both halves of a translation over any part of the palette and can be handed
// out by pointer.
//
// Corpses use these today, but can be expanded for:
// 
// 1. blood colors
// 2. recolored fonts
// 3. player translations
//
// An rgb entry with zero alpha means that index is not translated in 32bpp and
// falls back to the 8bpp remap, so a translation may cover as much or as
// little of the palette as it likes.
//
struct translationtable_t
{
	std::array<palindex_t, 256> remap; // 8bpp palette remap
	std::array<argb_t, 256>     rgb;   // 32bpp colors, alpha 0 where untranslated

	translationref_t ref() const { return translationref_t(remap.data(), rgb.data()); }
};

// Resets a translation to the identity, translating nothing.
void R_ClearTranslation(translationtable_t& tlate);

// Recolors from start to end into a ramp running towards dest_color, the way player
// colors are built.
void R_BuildTranslationRamp(translationtable_t& tlate, palindex_t start, palindex_t end,
                            argb_t dest_color);

// Recolors an ordered run of source indices into a linear gradient, the way font
// translations are built.
//
// The source is a list rather than a range because a font's colors are neither guaranteed
// to be contiguous nor to run dark to light in palette order.
void R_BuildTranslationGradient(translationtable_t& tlate, const palindex_t* src, size_t count,
                                argb_t start_color, argb_t end_color);

// Same, but for a source that happens to be the contiguous range from start to end.
void R_BuildTranslationGradient(translationtable_t& tlate, palindex_t start, palindex_t end,
                                argb_t start_color, argb_t end_color);

// Collects the palette indices a run of patches actually uses, ordered darkest
// to brightest.
//
// This is needed because gfx that do not sit on one of the palette's standard ramps -
// like a font with its own colors - has to be sampled before it can be recolored,
// since there is no other way to know which indices to translate.
void R_SampleLuminosity(const patch_t* const* patches, size_t count,
                        std::vector<palindex_t>& out);

//
// Shared translation lifetime
//
enum translationlife_t
{
	TRANSLIFE_MAP,    // until the map is unloaded - corpses and the like
	TRANSLIFE_WAD,    // until the resource set changes - dehacked and font recolors
	TRANSLIFE_STATIC, // for the run - fonts and other engine-defined colors
};

//
// Shared translations, built on demand and reused between everyone asking for
// the same one.
//
// If you request a translation with a color that exists in a shorter lifetime,
// that translation gets "promoted" rather than rebuilt.
//
translationref_t R_GetRampTranslation(translationlife_t life, palindex_t start, palindex_t end,
                                      argb_t color);
translationref_t R_GetGradientTranslation(translationlife_t life, palindex_t start,
                                          palindex_t end, argb_t start_color, argb_t end_color);

// Drops every shared translation that does not outlive the specified lifetime.
void R_ExpireTranslations(translationlife_t life);

// Rebuilds every shared translation against the current palette.
// The tables keep their addresses, so refs already handed out pick up the new colors.
void R_RebuildTranslations();

// The color a player is drawn in, once the game mode and the r_force*color
// cvars have had their say.
// isconsoleplayer is asked because playerids are recycled between the players
// who hold them (depending on connect/disconnect during a game).
argb_t R_GetPlayerDrawColor(argb_t user_color, team_t team, bool isconsoleplayer);

// A translation for one player identity.
translationref_t R_GetPlayerTranslation(translationlife_t life, argb_t user_color, team_t team,
                                        bool isconsoleplayer);

// The identity a corpse's owner died with.
// Corpses do not survive the map, so neither do their translations.
inline translationref_t R_GetCorpseTranslation(argb_t user_color, team_t team, bool isconsoleplayer)
{
	return R_GetPlayerTranslation(TRANSLIFE_MAP, user_color, team, isconsoleplayer);
}

// If the view size is not full screen, draws a border around it.
void R_DrawViewBorder (void);
void R_DrawBorder (int x1, int y1, int x2, int y2);
