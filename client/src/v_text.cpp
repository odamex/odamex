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

static OFont* hu_font;

byte* ConChars;
extern byte* Ranges;

OFont::OFont()
{
	for (size_t i = 0; i < HU_FONTSIZE; i++)
	{
		m_font[i] = NULL;
	}
}

OFont::~OFont()
{
	for (size_t i = 0; i < HU_FONTSIZE; i++)
	{
		if (m_font[i] == NULL)
		{
			continue;
		}
		Z_ChangeTag(m_font[i], PU_CACHE);
		m_font[i] = NULL;
	}
}

class OSmallFont : public OFont
{
  public:
	void load()
	{
		std::string buffer;

		// Normal doom chat/message font, starts at index 33.
		const char* smallfont = "STCFN%.3d";

		int j = HU_FONTSTART;
		for (int i = 0; i < HU_FONTSIZE; i++)
		{
			StrFormat(buffer, smallfont, j++);

			// Some letters of this font might be missing.
			int num = W_CheckNumForName(buffer.c_str());
			if (num != -1)
			{
				m_font[i] = W_CachePatch(buffer.c_str(), PU_STATIC);
			}
			else
			{
				m_font[i] = (patch_t*)W_CacheLumpNum(
				    W_GetNumForName("TNT1A0", ns_sprites), PU_STATIC);
			}
		}
	}
};

class OBigFont : public OFont
{
  public:
	void load()
	{
		std::string buffer;

		// Level name font, used between levels, starts at index 1.
		const char* bigfont = "FONTB%02d";

		int j = 1;
		for (int i = 0; i < HU_FONTSIZE; i++)
		{
			StrFormat(buffer, bigfont, j++);

			// Some letters of this font are missing.
			int num = W_CheckNumForName(buffer.c_str());
			if (num != -1)
			{
				m_font[i] = W_CachePatch(buffer.c_str(), PU_STATIC);
			}
			else
			{
				m_font[i] = (patch_t*)W_CacheLumpNum(
				    W_GetNumForName("TNT1A0", ns_sprites), PU_STATIC);
			}
		}
	}
};

class ODigFont : public OFont
{
  public:
	void load()
	{
		std::string buffer;

		const char* digfont = "DIG%02d";
		const char* digfont_literal = "DIG%c";

		// BOOM "Dig" font, way more complicated than it needed to be.  Letters
		// and numbers are themselves, other characters are their ASCII values.
		int j = HU_FONTSTART;
		for (int i = 0; i < HU_FONTSIZE; i++)
		{
			if ((j >= '0' && j <= '9') || (j >= 'A' && j <= 'Z'))
			{
				StrFormat(buffer, digfont_literal, j++);
			}
			else
			{
				StrFormat(buffer, digfont, j++);
			}

			// Some letters of this font might be missing.
			int num = W_CheckNumForName(buffer.c_str());
			if (num != -1)
			{
				m_font[i] = W_CachePatch(buffer.c_str(), PU_STATIC);
			}
			else
			{
				m_font[i] = (patch_t*)W_CacheLumpNum(
				    W_GetNumForName("TNT1A0", ns_sprites), PU_STATIC);
			}
		}
	}
};

static OSmallFont* hu_smallfont;
static OBigFont* hu_bigfont;
static ODigFont* hu_digfont;

/**
 * @brief Initialize fonts.
 */
void V_TextInit()
{
	if (hu_smallfont == NULL)
	{
		::hu_smallfont = new OSmallFont();
		::hu_smallfont->load();
	}
	if (hu_bigfont == NULL)
	{
		::hu_bigfont = new OBigFont();
		::hu_bigfont->load();
	}
	if (hu_digfont == NULL)
	{
		::hu_digfont = new ODigFont();
		::hu_digfont->load();
	}

	// Default font is FONT_SMALLFONT.
	V_SetFont(FONT_SMALLFONT);
}

/**
 * @brief Shut down and free fonts.
 */
void V_TextShutdown()
{
	if (hu_smallfont != NULL)
	{
		delete ::hu_smallfont;
		::hu_smallfont = NULL;
	}
	if (hu_bigfont != NULL)
	{
		delete ::hu_bigfont;
		::hu_bigfont = NULL;
	}
	if (hu_digfont != NULL)
	{
		delete ::hu_digfont;
		::hu_digfont = NULL;
	}
}

/**
 * @brief Set the current font.
 */
void V_SetFont(font_e font)
{
	switch (font)
	{
	case FONT_SMALLFONT:
		::hu_font = ::hu_smallfont;
		break;
	case FONT_BIGFONT:
		::hu_font = ::hu_bigfont;
		break;
	case FONT_DIGFONT:
		::hu_font = ::hu_digfont;
		break;
	}
}

/**
 * @brief Get the current global font.
 */
OFont* V_GetFont()
{
	return ::hu_font;
}

/**
 * @brief Get a font by name.
 */
OFont* V_GetFont(font_e font)
{
	switch (font)
	{
	case FONT_SMALLFONT:
		return ::hu_smallfont;
	case FONT_BIGFONT:
		return ::hu_bigfont;
	case FONT_DIGFONT:
		return ::hu_bigfont;
	}
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

/**
 * @brief Draw a line of text using a composed set of parameters.
 * 
 * @detail This function gives you much more flexability over the scale of
 *         the font, but in return it will ignore newlines, since reflowing
 *         text is the job of the caller.
 * 
 *         If you update this method, please update V_TextSize to match.
 * 
 * @param string Null-terminated string to draw.
 * @param pos Upper left position of the string to draw.
 * @param params Parameters to use for drawing.
 */
void DCanvas::DrawText(const char* string, Vec2<int> pos, const FontParams& params) const
{
	const OFont& font = params.getFont();
	int lineHeight = params.getLineHeight();
	EColorRange color = params.getColor();
	float opacity = params.getOpacity();
	Vec2<int> targetRes = params.getTargetRes();

	// Scale starts at 1.
	Vec2<float> scale(1.0f, 1.0f);

	// Tune line height into a scale and scale both dirs by it.
	if (lineHeight == 0)
	{
		lineHeight = font.lineHeight();
	}
	M_MulVec2f(scale, scale,
	           Vec2<float>(static_cast<float>(lineHeight) / font.lineHeight(),
	                       static_cast<float>(lineHeight) / font.lineHeight()));

	// Turn our target resolution into a scale and scale both dirs by it.
	if (targetRes.x == 0)
	{
		targetRes.x = I_GetSurfaceWidth();
	}
	else
	{
		targetRes.x = clamp(targetRes.x, 320, I_GetSurfaceWidth());
	}

	if (targetRes.y == 0)
	{
		targetRes.y = I_GetSurfaceHeight();
	}
	else
	{
		targetRes.y = clamp(targetRes.y, 200, I_GetSurfaceHeight());
	}
	M_MulVec2f(scale, scale,
	           Vec2<float>(static_cast<float>(targetRes.x) / I_GetSurfaceWidth(),
	                       static_cast<float>(targetRes.y) / I_GetSurfaceHeight()));

	::V_ColorMap = translationref_t(::Ranges + color * 256);

	Vec2<int> current = pos;

	const char* str = string;
	for (;;)
	{
		// End of the string.
		if (str[0] == '\0')
			break;

		// Parse a translation.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			int new_color = V_GetTextColor(str);
			::V_ColorMap = translationref_t(::Ranges + new_color * 256);
			str += 2;
			continue;
		}

		char ch = toupper(str[0]);
		str++;

		// Detect if we need to skip past a space.  Unlike the old drawer,
		// we do not handle newlines automatically.
		if (ch < HU_FONTSTART || ch > HU_FONTEND)
		{
			current.x += font.spaceWidth() * scale.y;
			continue;
		}

		// Width and height of the final patch.
		int width = font.at(ch)->width() * scale.x;
		int height = font.at(ch)->height() * scale.y;

		// Extent of the screen.
		Vec2<int> screentl(0, 0);
		Vec2<int> screenbr(I_GetSurfaceWidth(), I_GetSurfaceHeight());

		// Check if the extent of the patch is in-bounds.
		bool inside =
		    M_PointInRect(Vec2<int>(current.x, current.y), screentl, screenbr) &&
		    M_PointInRect(Vec2<int>(current.x + width, current.y + height), screentl,
		                  screenbr);

		if (inside)
		{
			DrawSWrapper(DCanvas::EWrapper_Translated, ::hu_font->at(ch), current.x,
			             current.y, width, height);
			current.x += width;
		}
	}
}

/**
 * @brief Measure how large the text would be given the passed parameters.
 * 
 * @detail The origin is assumed to be at (0, 0) and does not take into account
 *         any sort of truncation due to running off the end of the screen.
 * 
 * @param string Null-terminated string to draw.
 * @param params Parameters to use for drawing.
 */
Vec2<int> V_TextExtent(const char* string, const FontParams& params)
{
	const OFont& font = params.getFont();
	int lineHeight = params.getLineHeight();
	EColorRange color = params.getColor();
	float opacity = params.getOpacity();
	Vec2<int> targetRes = params.getTargetRes();

	// Scale starts at 1.
	Vec2<float> scale(1.0f, 1.0f);

	// Tune line height into a scale and scale both dirs by it.
	if (lineHeight == 0)
	{
		lineHeight = font.lineHeight();
	}
	M_MulVec2f(scale, scale,
	           Vec2<float>(static_cast<float>(lineHeight) / font.lineHeight(),
	                       static_cast<float>(lineHeight) / font.lineHeight()));

	// Turn our target resolution into a scale and scale both dirs by it.
	if (targetRes.x == 0)
	{
		targetRes.x = I_GetSurfaceWidth();
	}
	else
	{
		targetRes.x = clamp(targetRes.x, 320, I_GetSurfaceWidth());
	}

	if (targetRes.y == 0)
	{
		targetRes.y = I_GetSurfaceHeight();
	}
	else
	{
		targetRes.y = clamp(targetRes.y, 200, I_GetSurfaceHeight());
	}
	M_MulVec2f(scale, scale,
	           Vec2<float>(static_cast<float>(targetRes.x) / I_GetSurfaceWidth(),
	                       static_cast<float>(targetRes.y) / I_GetSurfaceHeight()));

	Vec2<int> extent(0, 0);

	const char* str = string;
	for (;;)
	{
		// End of the string.
		if (str[0] == '\0')
			break;

		// Parse a translation.
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			str += 2;
			continue;
		}

		char ch = toupper(str[0]);
		str++;

		// Detect if we need to skip past a space.  Unlike the old drawer,
		// we do not handle newlines automatically.
		if (ch < HU_FONTSTART || ch > HU_FONTEND)
		{
			extent.x += font.spaceWidth() * scale.y;
			continue;
		}

		// Width and height of the final patch.
		int width = font.at(ch)->width() * scale.x;
		int height = font.at(ch)->height() * scale.y;

		extent.x += width;
		extent.y = MAX(extent.y, height);
	}

	return extent;
}

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
	if (::hu_font == NULL)
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

		int c = toupper(str[0]);
		str++;

		if (c < HU_FONTSTART || c > HU_FONTEND)
		{
			cx += ::hu_font->spaceWidth() * scaley;
			continue;
		}

		int w = ::hu_font->at(c)->width() * scalex;
		if (cx + w > I_GetSurfaceWidth())
			break;

		DrawSWrapper(drawer, ::hu_font->at(c), cx, cy, ::hu_font->at(c)->width() * scalex,
		             ::hu_font->at(c)->height() * scaley);

		cx += w;
	}
}

//
// Find string width from hu_font chars
//
int V_StringWidth(const byte* str)
{
	// Default width without a font loaded is 8.
	if (::hu_font == NULL)
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

		int c = toupper((*str++) & 0x7f);
		if (c < HU_FONTSTART || c > HU_FONTEND)
		{
			width += ::hu_font->spaceWidth();
		}
		else
		{
			width += ::hu_font->at(c)->width();
		}
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
	OFont* font = V_GetFont();

	if (font == NULL)
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

		if (c < HU_FONTSTART || c > HU_FONTEND)
		{
			nw = font->spaceWidth();
		}
		else
		{
			nw = font->at(c)->width();
		}

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
