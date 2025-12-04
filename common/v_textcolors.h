// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//	V_TEXTCOLOR : Defines all colored codes that both 
//	the server and client will use to format their messages.
//	
//	... Obviously, you won't see any results on the server yet.
//
//-----------------------------------------------------------------------------


#pragma once

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

#define TEXTCOLOR_ESCAPE		'\034'

#define TEXTCOLOR_BRICK			"\034a"
#define TEXTCOLOR_TAN			"\034b"
#define TEXTCOLOR_GRAY			"\034c"
#define TEXTCOLOR_GREY			"\034c"
#define TEXTCOLOR_GREEN			"\034d"
#define TEXTCOLOR_BROWN			"\034e"
#define TEXTCOLOR_GOLD			"\034f"
#define TEXTCOLOR_RED			"\034g"
#define TEXTCOLOR_BLUE			"\034h"
#define TEXTCOLOR_ORANGE		"\034i"
#define TEXTCOLOR_WHITE			"\034j"
#define TEXTCOLOR_YELLOW		"\034k"

#define TEXTCOLOR_UNTRANSLATED	"\034l"
#define TEXTCOLOR_BLACK			"\034m"
#define TEXTCOLOR_LIGHTBLUE		"\034n"
#define TEXTCOLOR_CREAM			"\034o"
#define TEXTCOLOR_OLIVE			"\034p"
#define TEXTCOLOR_DARKGREEN		"\034q"
#define TEXTCOLOR_DARKRED		"\034r"
#define TEXTCOLOR_DARKBROWN		"\034s"
#define TEXTCOLOR_PURPLE		"\034t"
#define TEXTCOLOR_DARKGRAY		"\034u"
#define TEXTCOLOR_DARKGREY		"\034u"
#define TEXTCOLOR_CYAN			"\034v"

#define TEXTCOLOR_NORMAL		"\034-"
#define TEXTCOLOR_BOLD			"\034+"

inline std::string TextColorFromRange(EColorRange colorRange)
{
	switch (colorRange)
	{
	case CR_BRICK:			return TEXTCOLOR_BRICK;
	case CR_TAN:			return TEXTCOLOR_TAN;
	case CR_GRAY:			return TEXTCOLOR_GRAY;
	//case CR_GREY:			return TEXTCOLOR_GREY;
	case CR_GREEN:			return TEXTCOLOR_GREEN;
	case CR_BROWN:			return TEXTCOLOR_BROWN;
	case CR_GOLD:			return TEXTCOLOR_GOLD;
	case CR_RED:			return TEXTCOLOR_RED;
	case CR_BLUE:			return TEXTCOLOR_BLUE;
	case CR_ORANGE:			return TEXTCOLOR_ORANGE;
	case CR_WHITE:			return TEXTCOLOR_WHITE;
	case CR_YELLOW:			return TEXTCOLOR_YELLOW;
	case CR_UNTRANSLATED:	return TEXTCOLOR_UNTRANSLATED;
	case CR_BLACK:			return TEXTCOLOR_BLACK;
	case CR_LIGHTBLUE:		return TEXTCOLOR_LIGHTBLUE;
	case CR_CREAM:			return TEXTCOLOR_CREAM;
	case CR_OLIVE:			return TEXTCOLOR_OLIVE;
	case CR_DARKGREEN:		return TEXTCOLOR_DARKGREEN;
	case CR_DARKRED:		return TEXTCOLOR_DARKRED;
	case CR_DARKBROWN:		return TEXTCOLOR_DARKBROWN;
	case CR_PURPLE:			return TEXTCOLOR_PURPLE;
	case CR_DARKGRAY:		return TEXTCOLOR_DARKGRAY;
	//case CR_DARKGREY:		return TEXTCOLOR_DARKGREY;
	case CR_CYAN:			return TEXTCOLOR_CYAN;
	default:				return TEXTCOLOR_NORMAL;
	}
}
