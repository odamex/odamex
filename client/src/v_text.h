// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2020 by The Odamex Team.
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
	void setFont(const lumpHandle_t* font, const int lineHeight)
	{
		m_fontData = font;
		m_lineHeight = lineHeight;
	}
	int lineHeight() const
	{
		return m_lineHeight;
	}
  private:
	const lumpHandle_t* m_fontData;
	int m_lineHeight;
};

extern OGlobalFont hu_font;

void V_TextInit();
void V_TextShutdown();
void V_SetFont(const char* fontname);
int V_TextScaleXAmount();
int V_TextScaleYAmount();

struct brokenlines_s {
	int width;
	char *string;
};
typedef struct brokenlines_s brokenlines_t;

int V_StringWidth(const byte* str);
inline int V_StringWidth(const char* str) { return V_StringWidth((const byte*)str); }
int V_StringHeight(const char* str);
int V_LineHeight();

brokenlines_t *V_BreakLines (int maxwidth, const byte *str);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t *V_BreakLines (int maxwidth, const char *str) { return V_BreakLines (maxwidth, (const byte *)str); }

int V_GetTextColor(const char* str);
