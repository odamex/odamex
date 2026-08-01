// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	V_TEXT
//
//-----------------------------------------------------------------------------


#pragma once

#include <stdexcept>
#include <vector>

#include "v_textcolors.h"	// Ch0wW : Colorized textcodes
#include "hu_stuff.h"
#include "r_defs.h"
#include "w_wad.h"
#include "resources/res_texture.h"

void V_TextInit();
void V_TextShutdown();
int V_TextScaleXAmount();
int V_TextScaleYAmount();

struct brokenlines_t
{
	int width;
	char *string;
};

void V_FreeBrokenLines (brokenlines_t *lines);

int V_GetTextColor(std::string_view str);

class OFont;
struct OFontVariation;

extern OFont* menu_font;
extern OFont* hud_font;

enum fonttype_t
{
	FONT_BITMAP,	// classic bitmap font
	FONT_TTF,		// TrueType font
};

//
// V_GetHudFont
//
// Returns a HUD font rasterized for the given integer pixel scale, building
// and caching one on first use.
//
// The HUD draws text at several different scales (hud_scale and
// hud_scalescoreboard), and an OFont bakes its scale into its glyphs, so one
// font cannot serve them all. Rasterizing a font per scale keeps every size
// crisp instead of stretching one size to fit.
//
enum fontface_t
{
	FACE_SMALL,
	FACE_BIG,
	FACE_DIGITS,
};

//
// V_GetFaceFont
//
// Returns one of the named faces built for the given scale factor. Each face
// has its own base size, so FACE_BIG comes out larger than FACE_SMALL at the
// same scale.
//
OFont* V_GetFaceFont(fontface_t face, int pixel_scale);

OFont* V_GetHudFont(int pixel_scale);

//
// V_GetHudFontSized
//
// The same as V_GetHudFont, but takes an absolute pixel size. Use this for
// text whose size is not a multiple of the 8-pixel bitmap font.
//
OFont* V_GetHudFontSized(int pixel_size);

//
// V_GetFont
//
// Returns a font built from a specific face at a specific pixel size,
// caching one per face-and-size combination. A face whose lump is not
// present quietly falls back to the standard one.
//
OFont* V_GetFont(const char* lumpname, int pixel_size);

//
// V_GetStyledFont
//
// The same as V_GetFont, but with an explicit TrueTypeFont style mask
// (TTF_TEXTURE / TTF_GRADIENT / TTF_OUTLINE / TTF_SHADOW).
// A TTF_GRADIENT font built this way uses the translation ramp and
// takes the color it is drawn in.
//
OFont* V_GetStyledFont(const char* lumpname, int pixel_size, unsigned int stylemask);

//
// V_GetGradientFont
//
// A TTF_GRADIENT font filled with a fixed top-to-bottom color gradient. The
// colors are baked into the glyphs as literal palette indices outside the
// 0xB0-0xBF translation range, so any normal text color range shows them as is.
//
OFont* V_GetGradientFont(const char* lumpname, int pixel_size, argb_t top, argb_t bottom);

//
// V_GetVariableFont
//
// Like V_GetStyledFont, but applies a set of OpenType variation-axis settings
// (weight, width, optical size, slant, or any custom axis the face exposes) to
// a variable font. Each distinct set of axis values is cached as its own font.
// A face with no matching axes renders at its defaults.
//
OFont* V_GetVariableFont(const char* lumpname, int pixel_size, unsigned int stylemask,
                         const std::vector<OFontVariation>& variations);

//
// V_GetBitmapFont
//
// The classic bitmap equivalent of a named face.
// Used by the per-subsystem font selection to force the bitmap
// typeface even when a TrueType lump is present.
//
OFont* V_GetBitmapFont(const char* lumpname, int pixel_size);

//
// V_FontApplyPreferences
//
// Rebuilds the global menu_font and hud_font from the ui_font_* cvars. Called
// once fonts are ready and again whenever one of those cvars changes.
//
void V_FontApplyPreferences();

//
// V_FontsReady
//
// False until V_TextInit has run. Callers that can be reached during
// early startup -- console printing, notably -- must check this before
// asking for a font.
//
bool V_FontsReady();

int V_FontStringWidthClean(const OFont* font, const char* str);
int V_FontLineHeightClean(const OFont* font);

brokenlines_t* V_BreakLinesFont(const OFont* font, int maxwidth, const byte* str);
brokenlines_t* V_BreakLinesFontPixels(const OFont* font, int maxwidth_px, const byte* str);
inline brokenlines_t* V_BreakLinesFont(const OFont* font, int maxwidth, const char* str)
{
	return V_BreakLinesFont(font, maxwidth, reinterpret_cast<const byte*>(str));
}
