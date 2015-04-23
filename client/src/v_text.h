// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2015 by The Odamex Team.
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

struct brokenlines_s {
	int width;
	char *string;
};
typedef struct brokenlines_s brokenlines_t;

enum EColorRange
{
	CR_BRICK,
	CR_TAN,
	CR_GRAY,
	CR_GREY = CR_GRAY,
	CR_GREEN,
	CR_BROWN,
	CR_GOLD,
	CR_RED,
	CR_BLUE,
	CR_ORANGE,
	CR_WHITE,
	CR_YELLOW,

	CR_UNTRANSLATED,
	CR_BLACK,
	CR_LIGHTBLUE,
	CR_CREAM,
	CR_OLIVE,
	CR_DARKGREEN,
	CR_DARKRED,
	CR_DARKBROWN,
	CR_PURPLE,
	CR_DARKGRAY,
	CR_DARKGREY = CR_DARKGRAY,
	CR_CYAN,
	NUM_TEXT_COLORS
};

#define TEXTCOLOR_BRICK			"\\ca"
#define TEXTCOLOR_TAN			"\\cb"
#define TEXTCOLOR_GRAY			"\\cc"
#define TEXTCOLOR_GREY			"\\cc"
#define TEXTCOLOR_GREEN			"\\cd"
#define TEXTCOLOR_BROWN			"\\ce"
#define TEXTCOLOR_GOLD			"\\cf"
#define TEXTCOLOR_RED			"\\cg"
#define TEXTCOLOR_BLUE			"\\ch"
#define TEXTCOLOR_ORANGE		"\\ci"
#define TEXTCOLOR_WHITE			"\\cj"
#define TEXTCOLOR_YELLOW		"\\ck"

#define TEXTCOLOR_UNTRANSLATED	"\\cl"
#define TEXTCOLOR_BLACK			"\\cm"
#define TEXTCOLOR_LIGHTBLUE		"\\cn"
#define TEXTCOLOR_CREAM			"\\co"
#define TEXTCOLOR_OLIVE			"\\cp"
#define TEXTCOLOR_DARKGREEN		"\\cq"
#define TEXTCOLOR_DARKRED		"\\cr"
#define TEXTCOLOR_DARKBROWN		"\\cs"
#define TEXTCOLOR_PURPLE		"\\ct"
#define TEXTCOLOR_DARKGRAY		"\\cu"
#define TEXTCOLOR_DARKGREY		"\\cu"
#define TEXTCOLOR_CYAN			"\\cv"

#define TEXTCOLOR_NORMAL		"\\c-"
#define TEXTCOLOR_BOLD			"\\c+"

int V_StringWidth(const byte* str);
inline int V_StringWidth(const char* str) { return V_StringWidth((const byte*)str); }

brokenlines_t *V_BreakLines (int maxwidth, const byte *str);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t *V_BreakLines (int maxwidth, const char *str) { return V_BreakLines (maxwidth, (const byte *)str); }

int V_GetTextColor(const char* str);

#endif //__V_TEXT_H__

