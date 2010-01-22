// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2009 by The Odamex Team.
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

#define CR_BRICK			0
#define CR_TAN				1
#define CR_GRAY				2
#define CR_GREY				2
#define CR_GREEN			3
#define CR_BROWN			4
#define CR_GOLD				5
#define CR_RED				6
#define CR_BLUE				7
#define CR_ORANGE			8
#define NUM_TEXT_COLORS		9

#define TEXTCOLOR_BRICK		"\x8aA"
#define TEXTCOLOR_TAN		"\x8aB"
#define TEXTCOLOR_GRAY		"\x8aC"
#define TEXTCOLOR_GREY		"\x8aC"
#define TEXTCOLOR_GREEN		"\x8aD"
#define TEXTCOLOR_BROWN		"\x8aE"
#define TEXTCOLOR_GOLD		"\x8aF"
#define TEXTCOLOR_RED		"\x8aG"
#define TEXTCOLOR_BLUE		"\x8aH"
#define TEXTCOLOR_ORANGE	"\x8aI"

#define TEXTCOLOR_NORMAL	"\x8a-"
#define TEXTCOLOR_BOLD		"\x8a+"

int V_StringWidth (const byte *str);
inline int V_StringWidth (const char *str) { return V_StringWidth ((const byte *)str); }

int V_LargeStringWidth (const byte *str);
inline int V_LargeStringWidth (const char *str) { return V_StringWidth ((const byte *)str); }

brokenlines_t *V_BreakLines (int maxwidth, const byte *str);
void V_FreeBrokenLines (brokenlines_t *lines);
inline brokenlines_t *V_BreakLines (int maxwidth, const char *str) { return V_BreakLines (maxwidth, (const byte *)str); }

void V_InitConChars (byte transcolor);

#endif //__V_TEXT_H__

