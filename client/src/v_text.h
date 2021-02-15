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


#ifndef __V_TEXT_H__
#define __V_TEXT_H__

#include "doomtype.h"
#include "v_textcolors.h"	// Ch0wW : Colorized textcodes
#include "hu_stuff.h"
#include "r_defs.h"

enum font_e
{
	FONT_SMALLFONT,
	FONT_BIGFONT,
	FONT_DIGFONT,
};

class OFont
{
  protected:
	patch_t* m_font[HU_FONTSIZE];

  public:
	OFont();
	virtual ~OFont();

	/**
	 * @brief Override this method to load the font.
	 */
	virtual void load() = 0;

	/**
	 * @brief Get the correct patch for a given unicode codepoint.
	 *
	 * @param cp Codepoint to look up.
	 * @return Patch that should be rendered.
	 */
	patch_t* at(int cp)
	{
		cp -= HU_FONTSTART;
		clamp(cp, 0, HU_FONTSIZE - 1);
		return m_font[cp];
	}

	/**
	 * @brief Return a reasonable line height for the font.
	 * 
	 * @detail It is a reasonable expectation that an M is in the font and
	 *         that it extends from the cap height to the baseline.
	 */
	int lineHeight()
	{
		return at('M')->height();
	}

	/**
	 * @brief Return a reasonable width of a space.
	 */
	int spaceWidth()
	{
		return 4;
	}
};

void V_TextInit();
void V_TextShutdown();
void V_SetFont(font_e font);
OFont* V_GetFont();
OFont* V_GetFont(font_e font);
int V_TextScaleXAmount();
int V_TextScaleYAmount();

struct brokenlines_s {
	int width;
	char *string;
};
typedef struct brokenlines_s brokenlines_t;

int V_StringWidth(const byte* str);
inline int V_StringWidth(const char* str) { return V_StringWidth((const byte*)str); }

brokenlines_t *V_BreakLines (int maxwidth, const byte *str);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t *V_BreakLines (int maxwidth, const char *str) { return V_BreakLines (maxwidth, (const byte *)str); }

int V_GetTextColor(const char* str);

#endif //__V_TEXT_H__
