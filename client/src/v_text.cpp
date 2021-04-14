// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


#include <stdio.h>
#include <ctype.h>

#include "v_text.h"

#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "w_wad.h"

#include "hashtable.h"
#include "doomstat.h"
#include "cmdlib.h"

EXTERN_CVAR(msg0color)
EXTERN_CVAR(msg1color)
EXTERN_CVAR(msg2color)
EXTERN_CVAR(msg3color)
EXTERN_CVAR(msg4color)

EXTERN_CVAR(hud_scaletext)

patch_t* hu_font[HU_FONTSIZE];

static patch_t* hu_bigfont[HU_FONTSIZE];
static patch_t* hu_smallfont[HU_FONTSIZE];

byte *ConChars;
extern byte *Ranges;

/**
 * @brief Initialize fonts.
 */
void V_TextInit()
{
	int j, sub;
	std::string buffer;

	const char *bigfont = "FONTB%02d";
	const char *smallfont = "STCFN%.3d";

	// Level name font, used between levels, starts at index 1.
	j = 1;
	sub = 0;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		StrFormat(buffer, bigfont, j++ - sub);

		// Some letters of this font are missing.
		int num = W_CheckNumForName(buffer.c_str());
		if (num != -1)
			::hu_bigfont[i] = W_CachePatch(buffer.c_str(), PU_STATIC);
		else
			::hu_bigfont[i] = (patch_t*)W_CacheLumpNum(W_GetNumForName("TNT1A0", ns_sprites), PU_STATIC);
	}

	// Normal doom chat/message font, starts at index 33.
	j = HU_FONTSTART;
	sub = 0;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		StrFormat(buffer, smallfont, j++ - sub);
		::hu_smallfont[i] = W_CachePatch(buffer.c_str(), PU_STATIC);
	}

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
		::hu_font[i] = NULL;

		Z_Discard(&::hu_bigfont[i]);
		Z_Discard(&::hu_smallfont[i]);
	}
}

/**
 * @brief Set the current font.
 * 
 * @param fontname Font name, can be one of "BIGFONT" or "SMALLFONT".
 */
void V_SetFont(const char* fontname)
{
	if (stricmp(fontname, "BIGFONT") == 0)
		memcpy(::hu_font, ::hu_bigfont, sizeof(::hu_bigfont));
	else if (stricmp(fontname, "SMALLFONT") == 0)
		memcpy(::hu_font, ::hu_smallfont, sizeof(::hu_smallfont));
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
int V_GetTextColor(const char* str)
{
	static int table[128];
	static bool initialized = false;

	if (!initialized)
	{
		for (int i = 0; i < 128; i++)
			table[i] = -1;

		table['A'] = table['a'] = CR_BRICK;
		table['B'] = table['b'] = CR_TAN;
		table['C'] = table['c'] = CR_GRAY;
		table['D'] = table['d'] = CR_GREEN;
		table['E'] = table['e'] = CR_BROWN;
		table['F'] = table['f'] = CR_GOLD;
		table['G'] = table['g'] = CR_RED;
		table['H'] = table['h'] = CR_BLUE;
		table['I'] = table['i'] = CR_ORANGE;
		table['J'] = table['j'] = CR_WHITE;
		table['K'] = table['k'] = CR_YELLOW;
		table['M'] = table['m'] = CR_BLACK;
		table['N'] = table['n'] = CR_LIGHTBLUE;
		table['O'] = table['o'] = CR_CREAM;
		table['P'] = table['p'] = CR_OLIVE;
		table['Q'] = table['q'] = CR_DARKGREEN;
		table['R'] = table['r'] = CR_DARKRED;
		table['S'] = table['s'] = CR_DARKBROWN;
		table['T'] = table['t'] = CR_PURPLE;
		table['U'] = table['u'] = CR_DARKGRAY;
		table['V'] = table['v'] = CR_CYAN;

		initialized = true;
	}

	if (str[0] == TEXTCOLOR_ESCAPE && str[1] < 128)
	{
		int c = str[1];
		if (c == '-')
			return msg2color;		// use print color
		if (c == '+')
			return CR_GREEN;		// use print bold color
		if (c == '*')
			return msg3color;		// use chat color
		if (c == '!')
			return msg4color;		// use team chat color

		return table[c];
	}
	return -1;
}

//
// V_PrintStr
// Print a line of text using the console font
//
void DCanvas::PrintStr(int x, int y, const char* str, int default_color, bool use_color_codes) const
{
	// Don't try and print a string without conchars loaded.
	if (::ConChars == NULL)
		return;

	if (default_color < 0)
		default_color = CR_GRAY;

	translationref_t trans = translationref_t(Ranges + default_color * 256);

	int surface_width = mSurface->getWidth(), surface_height = mSurface->getHeight();
	int surface_pitch = mSurface->getPitch();

	if (y > (surface_height - 8) || y < 0)
		return;

	if (x < 0)
	{
		int skip = -(x - 7) / 8;
		x += skip * 8;
		if ((int)strlen(str) <= skip)
			return;

		str += skip;
	}

	x &= ~3;
	byte* destline = mSurface->getBuffer() + y * mSurface->getPitch();

	while (*str && x <= (surface_width - 8))
	{
	    // john - tab 4 spaces
	    if (*str == '\t')
	    {
	        str++;
	        x += 8 * 4;
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

		int c = *(byte*)str;

		if (mSurface->getBitsPerPixel() == 8)
		{
			const byte* source = (byte*)&ConChars[c * 128];
			palindex_t* dest = (palindex_t*)destline + x;
			for (int z = 0; z < 8; z++)
			{
				for (int a = 0; a < 8; a++)
				{
					const palindex_t mask = source[a+8];
					palindex_t color = trans.tlate(source[a]);
					dest[a] = (dest[a] & mask) ^ color;
				}

				dest += surface_pitch; 
				source += 16;
			}
		}
		else
		{
			byte* source = (byte*)&ConChars[c * 128];
			argb_t* dest = (argb_t*)destline + x;
			for (int z = 0; z < 8; z++)
			{
				for (int a = 0; a < 8; a++)
				{
					const argb_t mask = (source[a+8] << 24) | (source[a+8] << 16)
										| (source[a+8] << 8) | source[a+8];

					argb_t color = V_Palette.shade(trans.tlate(source[a])) & ~mask;
					dest[a] = (dest[a] & mask) ^ color; 
				}

				dest += surface_pitch >> 2; 
				source += 16;
			}
		}

		str++;
		x += 8;
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

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper(drawer, normalcolor, x, y, string, CleanXfac, CleanYfac);
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, 
							const byte *string, int scalex, int scaley) const
{
	if (::hu_font[0] == NULL)
		return;

	if (normalcolor < 0 || normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	int cx = x;
	int cy = y;

	const char*	str = (const char*)string;

	while (1)
	{
		if (str[0] == '\0')
			break;

		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			int new_color = V_GetTextColor(str);
			V_ColorMap = translationref_t(Ranges + new_color * 256);
			str += 2;	
			continue;
		}

		if (str[0] == '\n')
		{
			cx = x;
			cy += 9 * scalex;
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

		int w = hu_font[c]->width() * scalex;
		if (cx + w > I_GetSurfaceWidth())
			break;

        DrawSWrapper(drawer, hu_font[c], cx, cy,
                        hu_font[c]->width() * scalex,
                        hu_font[c]->height() * scaley);

		cx += w;
	}
}

//
// Find string width from hu_font chars
//
int V_StringWidth(const byte* str)
{
	// Default width without a font loaded is 8.
	if (::hu_font[0] == NULL)
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
			width += hu_font[c]->width();
	}

	return width;
}

//
// Break long lines of text into multiple lines no longer than maxwidth pixels
//
static void breakit(brokenlines_t* line, const byte* start, const byte* string, const char* prefix = NULL)
{
	// Leave out trailing white space
	while (string > start && isspace(*(string - 1)))
		string--;

	int prefix_len = prefix ? strlen(prefix) : 0;

	line->string = new char[string - start + 1 + prefix_len];

	if (prefix_len)
		strncpy(line->string + 0, prefix, prefix_len);

	strncpy(line->string + prefix_len, (char*)start, string - start);
	line->string[string - start + prefix_len] = 0;
	line->width = V_StringWidth(line->string);
}

brokenlines_t* V_BreakLines(int maxwidth, const byte* str)
{
	if (::hu_font[0] == NULL)
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
			sprintf(color_code_str, "\034%c", str[1]);
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
			nw = hu_font[c - HU_FONTSTART]->width();

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
