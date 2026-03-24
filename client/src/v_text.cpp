// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#include "odamex.h"

#include <ctype.h>

#include "v_text.h"

#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "gi.h"
#include "w_wad.h"

#include "hashtable.h"
#include "cmdlib.h"

EXTERN_CVAR(msg0color)
EXTERN_CVAR(msg1color)
EXTERN_CVAR(msg2color)
EXTERN_CVAR(msg3color)
EXTERN_CVAR(msg4color)

EXTERN_CVAR(hud_scaletext)

OGlobalFont hu_font;

static lumpHandle_t hu_bigfont[HU_FONTSIZE];
static lumpHandle_t hu_smallfont[HU_FONTSIZE];
static lumpHandle_t hu_digfont[HU_FONTSIZE];

static int hu_bigfont_height;
static int hu_smallfont_height;
static int hu_digfont_height;

byte *ConChars;
extern byte *Ranges;

namespace
{
const fontdef_t& V_GetResolvedFontDef(const std::string& name)
{
	const std::string lookup = StdStringToUpper(name);
	const auto it = fontdefs.find(lookup);
	if (it == fontdefs.end())
		I_Error("Unknown fontdef '{}'", name);

	return it->second;
}

void V_LoadHudFont(lumpHandle_t* font, const fontdef_t& def)
{
	if (def.pattern.empty())
		I_Error("Fontdef is missing a pattern");

	int lump = def.lumpStart;
	int j = HU_FONTSTART;
	std::string buffer;

	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		if (!def.pattern_literal.empty())
		{
			if ((j >= '0' && j <= '9') || (j >= 'A' && j <= 'Z'))
			{
				buffer = fmt::sprintf(def.pattern_literal.c_str(), j++);
			}
			else
			{
				buffer = fmt::sprintf(def.pattern.c_str(), j++);
			}
		}
		else
		{
			buffer = fmt::sprintf(def.pattern.c_str(), lump++);
		}

		int num = W_CheckNumForName(buffer.c_str());
		if (num != -1)
			font[i] = W_CachePatchHandle(buffer.c_str(), PU_STATIC);
		else
			font[i] = W_CachePatchHandle("TNT1A0", PU_STATIC, ns_sprites);
	}
}
}

/**
 * @brief Initialize fonts.
 */
void V_TextInit()
{
	std::string buffer;
	const fontdef_t& bigfont = V_GetResolvedFontDef(gameinfo.bigFont);
	const fontdef_t& smallfont = V_GetResolvedFontDef(gameinfo.smallFont);
	const fontdef_t& digfont = V_GetResolvedFontDef(gameinfo.digFont);


	// Level name font. Indexing/pattern are gameinfo-driven.
	V_LoadHudFont(::hu_bigfont, bigfont);

	// Chat/message small font.
	V_LoadHudFont(::hu_smallfont, smallfont);

	// Status bar digits.
	V_LoadHudFont(::hu_digfont, digfont);

	// Font heights.
	::hu_bigfont_height = bigfont.lineHeight > 0
	    ? bigfont.lineHeight
	    : W_ResolvePatchHandle(::hu_bigfont['M' - HU_FONTSTART])->height();
	::hu_smallfont_height = smallfont.lineHeight > 0
	    ? smallfont.lineHeight
	    : W_ResolvePatchHandle(::hu_smallfont['M' - HU_FONTSTART])->height();
	::hu_digfont_height = digfont.lineHeight > 0
	    ? digfont.lineHeight
	    : W_ResolvePatchHandle(::hu_digfont['M' - HU_FONTSTART])->height();

	// Default font is SMALLFONT.
	V_SetFont("SMALLFONT");
}

/**
 * @brief Shut down and free fonts.
 */
void V_TextShutdown()
{
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		::hu_bigfont[i].clear();
		::hu_smallfont[i].clear();
		::hu_digfont[i].clear();
	}
}

/**
 * @brief Set the current font.
 *
 * @param fontname Font name, can be one of "BIGFONT" or "SMALLFONT".
 */
void V_SetFont(const char* fontname)
{
	if (!stricmp(fontname, "BIGFONT"))
		::hu_font.setFont(::hu_bigfont, ::hu_bigfont_height);
	else if (!stricmp(fontname, "SMALLFONT"))
		::hu_font.setFont(::hu_smallfont, ::hu_smallfont_height);
	else if (!stricmp(fontname, "DIGFONT"))
		::hu_font.setFont(::hu_digfont, ::hu_digfont_height);
}

int V_GetFontLineHeight(const char* fontname)
{
	if (!stricmp(fontname, "BIGFONT"))
		return ::hu_bigfont_height > 0 ? ::hu_bigfont_height : 16;
	if (!stricmp(fontname, "SMALLFONT"))
		return ::hu_smallfont_height > 0 ? ::hu_smallfont_height : 8;
	if (!stricmp(fontname, "DIGFONT"))
		return ::hu_digfont_height > 0 ? ::hu_digfont_height : 8;

	return 8;
}

int V_TextScaleXAmount()
{
	return hud_scaletext.asInt();
}

int V_TextScaleYAmount()
{
	return hud_scaletext.asInt();
}


//
// V_GetTextColor
//
// Decodes a \c escape sequence and returns the index of the appropriate
// color translation to use. This assumes that str is at least three characters
// in length.
//
int V_GetTextColor(std::string_view str)
{
	static constexpr std::array<int, 128> table = []{
        std::array<int, 128> t{};
        t.fill(-1);

        auto set = [&](char ch, char ch2, int color){
            t[static_cast<unsigned char>(ch)] = color;
            t[static_cast<unsigned char>(ch2)] = color;
        };

        set('A', 'a', CR_BRICK);
        set('B', 'b', CR_TAN);
        set('C', 'c', CR_GRAY);
        set('D', 'd', CR_GREEN);
        set('E', 'e', CR_BROWN);
        set('F', 'f', CR_GOLD);
        set('G', 'e', CR_RED);
        set('H', 'h', CR_BLUE);
        set('I', 'i', CR_ORANGE);
        set('J', 'j', CR_WHITE);
        set('K', 'k', CR_YELLOW);
        set('M', 'm', CR_BLACK);
        set('N', 'n', CR_LIGHTBLUE);
        set('O', 'o', CR_CREAM);
        set('P', 'p', CR_OLIVE);
        set('Q', 'q', CR_DARKGREEN);
        set('R', 'r', CR_DARKRED);
        set('S', 's', CR_DARKBROWN);
        set('T', 't', CR_PURPLE);
        set('U', 'u', CR_DARKGRAY);
        set('V', 'v', CR_CYAN);

        return t;
    }();

	if (str[0] == TEXTCOLOR_ESCAPE && str[1] < 128)
	{
		switch (int c = str[1])
		{
			case '-':
				return msg2color; // use print color
			case '+':
				return CR_GREEN;  // use print bold color
			case '*':
				return msg3color; // use chat color
			case '!':
				return msg4color; // use team chat color
			default:
				return table[c];
		}
	}
	return -1;
}

//
// V_PrintStr
// Print a line of text using the console font
//
void DCanvas::PrintStr(int x, int y, const char* str, int default_color, bool use_color_codes, int scale) const
{
	// Don't try and print a string without conchars loaded.
	if (::ConChars == NULL)
		return;

	const int char_size = 8 * scale;

	if (default_color < 0)
		default_color = CR_GRAY;

	translationref_t trans = translationref_t(Ranges + default_color * 256);

	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getPitch();

	if (y > (surface_height - char_size) || y < 0)
		return;

	if (x < 0)
	{
		int skip = -(x - (char_size - 1)) / char_size;
		x += skip * char_size;
		if (static_cast<int>(strlen(str)) <= skip)
			return;

		str += skip;
	}

	x = x / char_size * char_size;
	byte* destline = mSurface->getBuffer() + y * mSurface->getPitch();

	while (*str && x <= (surface_width - char_size))
	{
	    // john - tab 4 spaces
	    if (*str == '\t')
	    {
	        str++;
	        x += char_size * 4;
	        continue;
	    }

		// [SL] parse color escape codes (\cX)
		if (use_color_codes && str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			int new_color = V_GetTextColor(str);
			if (new_color == -1)
				new_color = default_color;

			trans = translationref_t(Ranges + new_color * 256);

			str += 2;
			continue;
		}

		int c = static_cast<byte>(*str);

		if (mSurface->getBitsPerPixel() == 8)
		{
			const byte* source = &ConChars[c * 128];
			palindex_t* dest = static_cast<palindex_t*>(destline) + x;
			for (int z = 0; z < 8; z++)
			{
				// repeat each scanline based on scale
				for (int sy = 0; sy < scale; ++sy)
				{
					for (int a = 0; a < 8; a++)
					{
						const palindex_t mask = source[a+8];
						palindex_t color = trans.tlate(source[a]);

						// repeat each pixel based on scale
						for (int sx = 0; sx < scale; ++sx)
							dest[a*scale + sx] = (dest[a*scale + sx] & mask) ^ color;
					}
					dest += surface_pitch;
				}
				source += 16;
			}
		}
		else
		{
			byte* source = &ConChars[c * 128];
			argb_t* dest = reinterpret_cast<argb_t*>(destline) + x;
			for (int z = 0; z < 8; z++)
			{
				// repeat each scanline based on scale
				for (int sy = 0; sy < scale; ++sy)
				{
					for (int a = 0; a < 8; a++)
					{
						const argb_t mask = (source[a+8] << 24) | (source[a+8] << 16)
											| (source[a+8] << 8) | source[a+8];

						argb_t color = V_Palette.shade(trans.tlate(source[a])) & ~mask;
						// repeat each pixel based on scale
						for (int sx = 0; sx < scale; ++sx)
							dest[a*scale + sx] = (dest[a*scale + sx] & mask) ^ color;
					}
					dest += surface_pitch >> 2;
				}
				source += 16;
			}
		}

		str++;
		x += char_size;
	}
}

//
// V_DrawText
//
// Write a string using the hu_font
//

void DCanvas::TextWrapper(EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper(drawer, normalcolor, x, y, string, 1, 1);
}

void DCanvas::DrawText(const char* fontname, int normalcolor, int x, int y, const byte* string) const
{
	V_FontScope scope(fontname);
	DrawText(normalcolor, x, y, string);
}

void DCanvas::DrawTextLuc(const char* fontname, int normalcolor, int x, int y, const byte* string) const
{
	V_FontScope scope(fontname);
	DrawTextLuc(normalcolor, x, y, string);
}

void DCanvas::DrawTextClean(const char* fontname, int normalcolor, int x, int y, const byte* string) const
{
	V_FontScope scope(fontname);
	DrawTextClean(normalcolor, x, y, string);
}

void DCanvas::DrawTextCleanLuc(const char* fontname, int normalcolor, int x, int y, const byte* string) const
{
	V_FontScope scope(fontname);
	DrawTextCleanLuc(normalcolor, x, y, string);
}

void DCanvas::DrawTextCleanMove(const char* fontname, int normalcolor, int x, int y,
                                const byte* string) const
{
	V_FontScope scope(fontname);
	DrawTextCleanMove(normalcolor, x, y, string);
}

void DCanvas::DrawTextStretched(const char* fontname, int normalcolor, int x, int y,
                                const byte* string, int scalex, int scaley) const
{
	V_FontScope scope(fontname);
	DrawTextStretched(normalcolor, x, y, string, scalex, scaley);
}

void DCanvas::DrawTextStretchedLuc(const char* fontname, int normalcolor, int x, int y,
                                   const byte* string, int scalex, int scaley) const
{
	V_FontScope scope(fontname);
	DrawTextStretchedLuc(normalcolor, x, y, string, scalex, scaley);
}

void DCanvas::DrawText(const char* fontname, int normalcolor, int x, int y, const char* string) const
{
	DrawText(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string));
}

void DCanvas::DrawTextLuc(const char* fontname, int normalcolor, int x, int y, const char* string) const
{
	DrawTextLuc(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string));
}

void DCanvas::DrawTextClean(const char* fontname, int normalcolor, int x, int y, const char* string) const
{
	DrawTextClean(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string));
}

void DCanvas::DrawTextCleanLuc(const char* fontname, int normalcolor, int x, int y,
                               const char* string) const
{
	DrawTextCleanLuc(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string));
}

void DCanvas::DrawTextCleanMove(const char* fontname, int normalcolor, int x, int y,
                                const char* string) const
{
	DrawTextCleanMove(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string));
}

void DCanvas::DrawTextStretched(const char* fontname, int normalcolor, int x, int y,
                                const char* string, int scalex, int scaley) const
{
	DrawTextStretched(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string), scalex,
	                  scaley);
}

void DCanvas::DrawTextStretchedLuc(const char* fontname, int normalcolor, int x, int y,
                                   const char* string, int scalex, int scaley) const
{
	DrawTextStretchedLuc(fontname, normalcolor, x, y, reinterpret_cast<const byte*>(string),
	                     scalex, scaley);
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper(drawer, normalcolor, x, y, string, CleanXfac, CleanYfac);
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y,
							const byte *string, int scalex, int scaley) const
{
	if (::hu_font[0].empty())
		return;

	if (normalcolor < 0 || normalcolor >= NUM_TEXT_COLORS)
		normalcolor = CR_RED;

	if (!Ranges)
		return;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	int cx = x;
	int cy = y;

	const char*	str = reinterpret_cast<const char*>(string);

	while (1)
	{
		if (str[0] == '\0')
			break;

		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			int new_color = V_GetTextColor(str);
			if (new_color >= 0 && new_color < NUM_TEXT_COLORS)
				V_ColorMap = translationref_t(Ranges + new_color * 256);
			str += 2;
			continue;
		}

		if (str[0] == '\n')
		{
			cx = x;
			cy += V_LineHeight() * scalex;
			str++;
			continue;
		}

		int c = toupper(str[0]) - HU_FONTSTART;
		str++;

		if (c < 0 || c >= HU_FONTSIZE)
		{
			cx += 4 * scaley;
			continue;
		}

		patch_t* ch = W_ResolvePatchHandle(hu_font[c]);

		int w = ch->width() * scalex;
		if (cx + w > I_GetSurfaceWidth())
			break;

        DrawSWrapper(drawer, ch, cx, cy, ch->width() * scalex, ch->height() * scaley);

		cx += w;
	}
}

//
// Find string width from hu_font chars
//
int V_StringWidth(const byte* str)
{
	// Default width without a font loaded is 8.
	if (::hu_font[0].empty())
		return 8;

	int width = 0;

	while (*str)
	{
		// skip over color markup escape codes
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		int c = toupper((*str++) & 0x7f) - HU_FONTSTART;
		if (c < 0 || c >= HU_FONTSIZE)
			width += 4;
		else
			width += W_ResolvePatchHandle(hu_font[c])->width();
	}

	return width;
}

int V_StringWidth(const char* fontname, const byte* str)
{
	V_FontScope scope(fontname);
	return V_StringWidth(str);
}

int V_StringHeight(const char* str)
{
	// Default width without a font loaded is 8.
	if (::hu_font[0].empty())
		return 8;

	int lineheight = V_LineHeight();
	int height = lineheight;

	while (str[0] != '\0')
	{
		// skip over color markup escape codes
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		if (str[0] == '\n')
			height += lineheight;

		str += 1;
	}

	return height;
}

int V_StringHeight(const char* fontname, const char* str)
{
	V_FontScope scope(fontname);
	return V_StringHeight(str);
}

//
// Break long lines of text into multiple lines no longer than maxwidth pixels
//
static void breakit(brokenlines_t* line, const byte* start, const byte* string, const char* prefix = NULL)
{
	// Leave out trailing white space
	while (string > start && isspace(*(string - 1)))
		string--;

	size_t prefix_len = prefix ? strlen(prefix) : 0;

	line->string = new char[string - start + 1 + prefix_len];

	if (prefix_len)
		strncpy(line->string + 0, prefix, prefix_len);

	strncpy(line->string + prefix_len, reinterpret_cast<const char*>(start), string - start);
	line->string[string - start + prefix_len] = 0;
	line->width = V_StringWidth(line->string);
}

int V_LineHeight()
{
	return ::hu_font.lineHeight();
}

int V_LineHeight(const char* fontname)
{
	V_FontScope scope(fontname);
	return V_LineHeight();
}

brokenlines_t* V_BreakLines(int maxwidth, const byte* str)
{
	if (::hu_font[0].empty())
		return NULL;

	brokenlines_t lines[128];	// Support up to 128 lines (should be plenty)

	const byte* space = NULL;
	const byte* start = str;
	int i, w, nw;
	bool lastWasSpace = false;

	i = w = 0;

	char color_code_str[4] = { 0 };

	while (*str)
	{
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			snprintf(color_code_str, 4, "\034%c", str[1]);
			str += 2;
			continue;
		}

		int c = toupper(*str++);

		if (isspace(c))
		{
			if (!lastWasSpace)
			{
				space = str - 1;
				lastWasSpace = true;
			}
		}
		else
		{
			lastWasSpace = false;
		}

		if (c < HU_FONTSTART || c >= HU_FONTSTART + HU_FONTSIZE)
			nw = 4;
		else
			nw = W_ResolvePatchHandle(hu_font[c - HU_FONTSTART])->width();

		if (w + nw > maxwidth || c == '\n')
		{
			// Time to break the line
			if (!space)
				space = str - 1;

			breakit(&lines[i], start, space, color_code_str);

			i++;
			w = 0;
			lastWasSpace = false;
			start = space;
			space = NULL;

			while (*start && isspace(*start) && *start != '\n')
				start++;

			if (*start == '\n')
				start++;
			else
				while (*start && isspace(*start))
					start++;

			str = start;
		}
		else
		{
			w += nw;
		}
	}

	if (str - start > 1)
	{
		const byte* s = start;

		while (s < str)
		{
			if (!isspace (*s++))
			{
				breakit(&lines[i++], start, str, color_code_str);
				break;
			}
		}
	}

	{
		// Make a copy of the broken lines and return them
		brokenlines_t* broken = new brokenlines_t[i + 1];

		memcpy(broken, lines, sizeof(brokenlines_t) * i);
		broken[i].string = NULL;
		broken[i].width = -1;

		return broken;
	}
}

brokenlines_t* V_BreakLines(const char* fontname, int maxwidth, const byte* str)
{
	V_FontScope scope(fontname);
	return V_BreakLines(maxwidth, str);
}

void V_FreeBrokenLines(brokenlines_t* lines)
{
	if (lines)
	{
		for (int i = 0; lines[i].width != -1; i++)
		{
			delete [] lines[i].string;
			lines[i].string = NULL;
		}

		delete [] lines;
	}
}


VERSION_CONTROL (v_text_cpp, "$Id$")
