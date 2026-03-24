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

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include "v_textcolors.h"	// Ch0wW : Colorized textcodes
#include "hu_stuff.h"
#include "r_defs.h"
#include "w_wad.h"

struct OFont
{
	lumpHandle_t operator[](const size_t idx) const
	{
		return m_glyphs[idx];
	}
	lumpHandle_t at(const size_t idx) const
	{
		if (idx >= HU_FONTSIZE)
			throw std::out_of_range("Out-of-bounds font char");

		return m_glyphs[idx];
	}
	bool empty() const
	{
		return m_glyphs[0].empty();
	}
	void setGlyph(const size_t idx, const lumpHandle_t glyph)
	{
		m_glyphs[idx] = glyph;
	}
	void setLineHeight(const int lineHeight)
	{
		m_lineHeight = lineHeight;
	}
	int lineHeight() const
	{
		return m_lineHeight;
	}
  private:
	std::array<lumpHandle_t, HU_FONTSIZE> m_glyphs{};
	int m_lineHeight = 0;
};

class OFontRegistry
{
  public:
	void clear();
	const OFont* find(std::string_view name) const;
	OFont& create(const std::string& name);
	const OFont* big() const;
	const OFont* small() const;
	const OFont* digits() const;

 private:
	std::unordered_map<std::string, OFont> m_fonts;
};

extern OFontRegistry OFonts;

void V_TextInit();
void V_TextShutdown();
const OFont* V_GetFont(const char* fontname);
int V_GetFontLineHeight(const char* fontname);
int V_TextScaleXAmount();
int V_TextScaleYAmount();

struct brokenlines_t
{
	int width;
	char *string;
};

int V_StringWidth(const OFont* font, const byte* str);
int V_StringWidth(const char* fontname, const byte* str);
inline int V_StringWidth(const OFont* font, const char* str)
{
	return V_StringWidth(font, reinterpret_cast<const byte*>(str));
}
inline int V_StringWidth(const char* fontname, const char* str)
{
	return V_StringWidth(fontname, reinterpret_cast<const byte*>(str));
}
int V_StringHeight(const OFont* font, const char* str);
int V_StringHeight(const char* fontname, const char* str);
int V_LineHeight(const OFont* font);
int V_LineHeight(const char* fontname);

brokenlines_t *V_BreakLines (const OFont* font, int maxwidth, const byte *str);
brokenlines_t *V_BreakLines (const char* fontname, int maxwidth, const byte *str);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t* V_BreakLines(const OFont* font, int maxwidth, const char* str)
{
	return V_BreakLines(font, maxwidth, reinterpret_cast<const byte*>(str));
}
inline brokenlines_t* V_BreakLines(const char* fontname, int maxwidth, const char* str)
{
	return V_BreakLines(fontname, maxwidth, reinterpret_cast<const byte*>(str));
}

int V_GetTextColor(std::string_view str);
