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
#include <string>
#include <vector>

#include "v_textcolors.h"	// Ch0wW : Colorized textcodes
#include "hu_stuff.h"
#include "r_defs.h"
#include "r_draw.h"
#include "w_wad.h"

//
// OFontColors
//
// How a font's glyphs take the text colors. Each resource that supplied
// glyphs gets translations built from the colors its own art uses, so a font
// recolors whatever palette range it was drawn in - the stock fonts of every
// IWAD as much as a PWAD's.
//
// Glyphs are grouped by the resource that supplied them, because a font can
// be pieced together from several, each in colors of its own.
//
class OFontColors
{
  public:
	struct source_t
	{
		std::string name;
		size_t      glyphs = 0;
		size_t      colors = 0;
	};

	// Forgets every source. Until a source claims them, glyphs are untranslated.
	void reset(size_t glyphs);

	// Hands glyphs to the source named name, whose art uses counts. Built
	// translations only cover the colors in counts; any other color in those
	// glyphs is drawn as it is.
	void addSource(const std::string& name, const std::vector<size_t>& glyphs,
	               const colorcounts_t& counts);

	// The translation to draw one glyph in color with.
	[[nodiscard]] translationref_t get(size_t glyph, int color) const;

	[[nodiscard]] const std::vector<source_t>& sources() const { return m_sources; }

  private:
	using colorset_t = std::array<translationtable_t, NUM_TEXT_COLORS>;

	std::vector<int>        m_glyph_source; // per glyph: index into m_sources, or -1
	std::vector<source_t>   m_sources;
	std::vector<colorset_t> m_sets;         // per source
};

// The console font: 256 glyphs of 8 rows, each row 8 color bytes followed by 8
// transparency mask bytes. Built from the CONCHARS lumps by C_InitConCharsFont.
extern std::vector<byte> ConChars;
extern OFontColors ConCharsColors;

struct OGlobalFont
{
	lumpHandle_t operator[](const size_t idx) const
	{
		return m_fontData[idx];
	}
	lumpHandle_t at(const size_t idx) const
	{
		if (idx >= HU_FONTSIZE)
			throw std::out_of_range("Out-of-bounds font char");

		return m_fontData[idx];
	}
	void setFont(const lumpHandle_t* font, const int lineHeight, const OFontColors* colors)
	{
		m_fontData = font;
		m_lineHeight = lineHeight;
		m_colors = colors;
	}
	int lineHeight() const
	{
		return m_lineHeight;
	}
	[[nodiscard]] const lumpHandle_t* data() const
	{
		return m_fontData;
	}
	[[nodiscard]] const OFontColors& colors() const
	{
		return *m_colors;
	}
  private:
	const lumpHandle_t* m_fontData;
	int m_lineHeight;
	const OFontColors* m_colors;
};

extern OGlobalFont hu_font;

void V_TextInit();
void V_TextShutdown();
void V_SetFont(const char* fontname);

// The colors of one of the fonts by name, or nullptr if there is no such font.
const OFontColors* V_FontColors(const char* fontname);
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
