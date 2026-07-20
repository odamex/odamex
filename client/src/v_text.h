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

#include "v_textcolors.h"	// Ch0wW : Colorized textcodes
#include "hu_stuff.h"
#include "r_defs.h"
#include "w_wad.h"
#include "resources/res_texture.h"

struct OGlobalFont
{
	const Texture* operator[](const size_t idx)
	{
		return m_fontData[idx];
	}
	const Texture* at(const size_t idx)
	{
		if (idx >= HU_FONTSIZE)
			throw std::out_of_range("Out-of-bounds font char");

		return m_fontData[idx];
	}
	void setFont(const Texture* font[HU_FONTSIZE], const int lineHeight)
	{
		for (int i = 0; i < HU_FONTSIZE; i++)
		{
			m_fontData[i] = font[i];
		}
		m_lineHeight = lineHeight;
	}
	int lineHeight() const
	{
		return m_lineHeight;
	}
	void clear()
	{
		for (size_t i = 0; i < HU_FONTSIZE; i++)
		{
			m_fontData[i] = NULL;
		}
	}
  private:
	const Texture* m_fontData[HU_FONTSIZE];
	int m_lineHeight;
};

void V_TextInit();
void V_TextShutdown();
void V_SetFont(const char* fontname);
int V_TextScaleXAmount();
int V_TextScaleYAmount();

struct brokenlines_t
{
	int width;
	char *string;
};

int V_StringWidth(const byte* str);
inline int V_StringWidth(const char* str) { return V_StringWidth(reinterpret_cast<const byte*>(str)); }
int V_StringHeight(const char* str);
int V_LineHeight();

brokenlines_t *V_BreakLines (int maxwidth, const byte *str);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t *V_BreakLines (int maxwidth, const char *str) { return V_BreakLines (maxwidth, reinterpret_cast<const byte*>(str)); }

int V_GetTextColor(std::string_view str);

extern OGlobalFont hu_font;

class OFont;

extern OFont* menu_font;
extern OFont* hud_font;

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
// Unlike V_GetHudFont this ignores whatever V_SetFont last selected, so use
// it for text that should always draw in a particular face.
//
OFont* V_GetFont(const char* lumpname, int pixel_size);

int V_FontStringWidthClean(const OFont* font, const char* str);
int V_FontLineHeightClean(const OFont* font);

brokenlines_t* V_BreakLinesFont(const OFont* font, int maxwidth, const byte* str);
inline brokenlines_t* V_BreakLinesFont(const OFont* font, int maxwidth, const char* str)
{
	return V_BreakLinesFont(font, maxwidth, reinterpret_cast<const byte*>(str));
}
