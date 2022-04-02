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
#include "resources/res_texture.h"

struct OGlobalFont
{
	const Texture* operator[](const size_t idx)
	{
		return m_fontData[idx];
	}
	const Texture* at(const size_t idx)
	{
		if (idx < 0 || idx >= HU_FONTSIZE)
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
		m_fontLoaded = true;
	}
	int lineHeight() const
	{
		return m_lineHeight;
	}
	bool isFontLoaded() const
	{
		return m_fontLoaded;
	}
  private:
	const Texture* m_fontData[HU_FONTSIZE];
	int m_lineHeight;
	bool m_fontLoaded = false;
};

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

extern OGlobalFont hu_font;
