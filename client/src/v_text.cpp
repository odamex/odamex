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
#include <map>

#include "v_text.h"

#include "i_video.h"
#include "v_video.h"
#include "v_font.h"
#include "hu_stuff.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

#include "hashtable.h"
#include "cmdlib.h"

EXTERN_CVAR(msg0color)
EXTERN_CVAR(msg1color)
EXTERN_CVAR(msg2color)
EXTERN_CVAR(msg3color)
EXTERN_CVAR(msg4color)

EXTERN_CVAR(hud_scaletext)
EXTERN_CVAR(hud_transparency)


OGlobalFont hu_font;

const Texture* hu_bigfont[HU_FONTSIZE];
const Texture* hu_smallfont[HU_FONTSIZE];
const Texture* hu_digfont[HU_FONTSIZE];

static int hu_bigfont_height;
static int hu_smallfont_height;
static int hu_digfont_height;

static std::string hu_bigfont_name[HU_FONTSIZE];
static std::string hu_smallfont_name[HU_FONTSIZE];
static std::string hu_digfont_name[HU_FONTSIZE];

byte *ConChars;
extern byte *Ranges;

OFont* menu_font = NULL;
OFont* hud_font = NULL;

static const char* TTF_LUMP_NAME = "FONT_SM";

//
// V_CreateFont
//
// Builds a TrueType font, falling back to the equivalent bitmap font if the
// TrueType lump is missing or unusable -- text disappearing entirely is a far
// worse failure mode than text in the wrong typeface.
//
static OFont* V_CreateFont(unsigned int stylemask, OFont::ScaleFunc scale_func)
{
	OFont* font = new TrueTypeFont(TTF_LUMP_NAME, 8, stylemask, scale_func);
	if (font->isUsable())
		return font;

	PrintFmt(PRINT_HIGH, "TrueType font {} unusable, falling back to the bitmap font.\n",
	         TTF_LUMP_NAME);

	delete font;
	return new SmallDoomFont(scale_func);
}

//
// V_FontLumpExists
//
// Optional faces are looked up quietly -- TrueTypeFont complains when it
// cannot find its lump, and a face that is simply not shipped should not
// report that on every font rebuild.
//
static bool V_FontLumpExists(const char* lumpname)
{
	return Res_CheckResource(Res_GetResourceId(lumpname, global_directory_name));
}


//
// V_CreateFontAtSize
//
// As V_CreateFont, but pinned to an absolute pixel size and a named face
// rather than following a scale provider.
//
static OFont* V_CreateFontAtSize(unsigned int stylemask, int size, const char* lumpname)
{
	if (!V_FontLumpExists(lumpname))
		lumpname = TTF_LUMP_NAME;

	OFont* font = new TrueTypeFont(lumpname, size, stylemask);
	if (font->isUsable())
		return font;

	delete font;
	return new SmallDoomFont(MAX(1, size / 8) * FRACUNIT);
}


//
// V_GetFont
//
OFont* V_GetFont(const char* lumpname, int pixel_size)
{
	typedef std::map<std::string, OFont*> FontCache;
	static FontCache cache;

	pixel_size = clamp(pixel_size, 4, 128);

	const std::string key = fmt::format("{}:{}", lumpname, pixel_size);

	FontCache::iterator it = cache.find(key);
	if (it != cache.end())
		return it->second;

	OFont* font = V_CreateFontAtSize(TrueTypeFont::TTF_TEXTURE, pixel_size, lumpname);
	cache[key] = font;
	return font;
}


//
// V_GetHudFontSized
//
OFont* V_GetHudFontSized(int pixel_size)
{
	return V_GetFont(TTF_LUMP_NAME, pixel_size);
}


//
// What each named face is made of. Indexed by fontface_t.
//
struct hudfacedef_t
{
	const char*	lumpname;
	int			base_size;
};

static const hudfacedef_t hud_faces[] =
{
	{ "FONT_SM",  8 },		// FACE_SMALL
	{ "FONT_BIG", 12 },		// FACE_BIG
	{ "FONT_DIG", 8 },		// FACE_DIGITS
};


//
// V_GetFaceFont
//
OFont* V_GetFaceFont(fontface_t face, int pixel_scale)
{
	// The bitmap HUD font is 8 pixels tall, so a scale factor of N matches a
	// face built at N times its base size.
	pixel_scale = MAX(1, pixel_scale);

	const hudfacedef_t& def = hud_faces[face];
	return V_GetFont(def.lumpname, def.base_size * pixel_scale);
}


//
// V_GetHudFont
//
OFont* V_GetHudFont(int pixel_scale)
{
	return V_GetFaceFont(FACE_SMALL, pixel_scale);
}


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
		buffer = fmt::sprintf(bigfont, j++ - sub);

		// Some letters of this font are missing.
		int num = Res_GetTextureResourceId(buffer.c_str(), PATCH);
		if (Res_CheckResource(num))
		{
			::hu_bigfont[i] = Res_CacheTexture(num, PU_STATIC);
			::hu_bigfont_name[i] = buffer.c_str();
		}
		else
		{
			::hu_bigfont[i] = Res_CacheTexture("TNT1A0", PATCH, PU_STATIC);
			::hu_bigfont_name[i] = "";
		}
	}

	// Normal doom chat/message font, starts at index 33.
	j = HU_FONTSTART;
	sub = 0;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		buffer = fmt::sprintf(smallfont, j++ - sub);
		::hu_smallfont[i] = Res_CacheTexture(buffer.c_str(), PATCH, PU_STATIC);
		::hu_smallfont_name[i] = buffer.c_str();
	}

	const char* digfont = "DIG%02d";
	const char* digfont_literal = "DIG%c";

	// BOOM "Dig" font, way more complicated than it needed to be.  Letters
	// and numbers are themselves, other characters are their ASCII values.
	j = HU_FONTSTART;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		if ((j >= '0' && j <= '9') || (j >= 'A' && j <= 'Z'))
		{
			buffer = fmt::sprintf(digfont_literal, j++);
		}
		else
		{
			buffer = fmt::sprintf(digfont, j++);
		}

		// Some letters of this font might be missing.
		int num = Res_GetTextureResourceId(buffer.c_str(), GRAPHICS);
		if (Res_CheckResource(num))
		{
			::hu_digfont[i] = Res_CacheTexture(num, PU_STATIC);
			::hu_digfont_name[i] = buffer.c_str();
		}
		else
		{
			::hu_digfont[i] = Res_CacheTexture("TNT1A0", PATCH, PU_STATIC);
			::hu_digfont_name[i] = buffer.c_str();
		}
	}

	// Font heights.
	::hu_bigfont_height = ::hu_bigfont['M' - HU_FONTSTART]->mHeight;
	::hu_smallfont_height = ::hu_smallfont['M' - HU_FONTSTART]->mHeight;
	::hu_digfont_height = ::hu_digfont['M' - HU_FONTSTART]->mHeight;

	if (!::menu_font)
		::menu_font = V_CreateFont(TrueTypeFont::TTF_GRADIENT | TrueTypeFont::TTF_SHADOW,
		                           V_FontScaleClean);
	if (!::hud_font)
		::hud_font = V_CreateFont(TrueTypeFont::TTF_TEXTURE, V_FontScaleHudText);

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
		if (!::hu_bigfont_name[i].empty())
		{
			ResourceId res_id = Res_GetTextureResourceId(hu_bigfont_name[i], PATCH);
			Res_ReleaseResource(res_id);
			hu_bigfont_name[i] = "";
		}
		::hu_bigfont[i] = NULL;

		if (!::hu_smallfont_name[i].empty())
		{
			ResourceId res_id = Res_GetTextureResourceId(hu_smallfont_name[i], PATCH);
			Res_ReleaseResource(res_id);
			hu_smallfont_name[i] = "";
		}
		::hu_smallfont[i] = NULL;

		if (!::hu_digfont_name[i].empty())
		{
			ResourceId res_id = Res_GetTextureResourceId(hu_digfont_name[i], PATCH);
			Res_ReleaseResource(res_id);
			hu_digfont_name[i] = "";
		}
		::hu_digfont[i] = NULL;
	}

	::hu_font.clear();
}

/**
 * @brief Set the current font.
 *
 * @param fontname Font name, can be one of "BIGFONT" or "SMALLFONT".
 */
void V_SetFont(const char* fontname)
{
	if (!stricmp(fontname, "BIGFONT"))
	{
		::hu_font.setFont(::hu_bigfont, hu_bigfont_height);
	}
	else if (!stricmp(fontname, "SMALLFONT"))
	{
		::hu_font.setFont(::hu_smallfont, hu_smallfont_height);
	}
	else if (!stricmp(fontname, "DIGFONT"))
	{
		::hu_font.setFont(::hu_digfont, hu_digfont_height);
	}
}

int V_TextScaleXAmount()
{
	int ret = hud_scaletext.asInt();

	if (!ret)
		return CleanXfac;

	return ret;
}

int V_TextScaleYAmount()
{
	int ret = hud_scaletext.asInt();

	if (!ret)
		return CleanYfac;

	return ret;
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

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y, const byte *string) const
{
	TextSWrapper(drawer, normalcolor, x, y, string, CleanXfac, CleanYfac);
}

void DCanvas::TextSWrapper (EWrapperCode drawer, int normalcolor, int x, int y,
							const byte *string, int scalex, int scaley) const
{
	if (!::hu_font[0])
		return;

	if (normalcolor < 0 || normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;

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

		int w = hu_font[c]->mWidth * scalex;
		if (cx + w > I_GetSurfaceWidth())
			break;

        DrawSWrapper(drawer, hu_font[c], cx, cy, hu_font[c]->mWidth * scalex,
		             hu_font[c]->mHeight * scaley);

		cx += w;
	}
}

// ----------------------------------------------------------------------------
//
// OFont-backed text
//
// ----------------------------------------------------------------------------

//
// DCanvas::DrawFontTextRaw
//
// Shared core of the OFont drawing entry points. Coordinates are real screen
// pixels and glyphs are blitted at their built size -- the font already
// carries its scale, so scaling here would apply it twice.
//
void DCanvas::DrawFontTextRaw(const OFont* font, EWrapperCode drawer,
		int normalcolor, int x, int y, const char* string) const
{
	if (!font || !string)
		return;

	if (normalcolor < 0 || normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;

	V_ColorMap = translationref_t(Ranges + normalcolor * 256);

	const int startx = x;
	const int ascent = font->getAscent();

	const bool blend = (mSurface->getBitsPerPixel() == 32);
	int blend_level = 255;

	if (drawer == EWrapper_Lucent || drawer == EWrapper_TlatedLucent)
		blend_level = clamp(static_cast<int>(hud_transparency * 255), 0, 255);

	for (const char* str = string; str[0] != '\0'; )
	{
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			const int new_color = V_GetTextColor(str);
			V_ColorMap = translationref_t(Ranges + new_color * 256);
			str += 2;
			continue;
		}

		if (str[0] == '\n')
		{
			x = startx;
			y += font->getHeight();
			str++;
			continue;
		}

		const char c = str[0];
		str++;

		const Texture* glyph = font->getGlyph(c);
		if (glyph && glyph->mWidth > 0 && glyph->mHeight > 0)
		{
			if (x + glyph->mWidth > I_GetSurfaceWidth())
				break;

			// glyph bearings are relative to the baseline, which sits
			// getAscent() below the top of the line
			const int glyph_x = x - glyph->mOffsetX;
			const int glyph_y = y + ascent - glyph->mOffsetY;

			const byte* coverage = font->getGlyphCoverage(c);
			const palindex_t* fill = font->getGlyphFill(c);

			if (blend && coverage && fill)
			{
				DrawGlyphBlended(fill, coverage, glyph->mWidth, glyph->mHeight,
				                 glyph_x, glyph_y, blend_level);
			}
			else
			{
				DrawWrapper(drawer, glyph, x, y + ascent);
			}
		}

		x += font->getTextWidth(c);
	}
}


//
// DCanvas::DrawFontTextCleanMove
//
// Takes 320x200 virtual coordinates, transformed the same way
// DrawTextCleanMove does.
//
void DCanvas::DrawFontTextCleanMove(const OFont* font, int normalcolor, int x, int y,
		const char* string) const
{
	DrawFontTextRaw(font, EWrapper_Translated, normalcolor,
	                getCleanX(x), getCleanY(y), string);
}


//
// DCanvas::DrawFontText
//
// Takes real screen coordinates, for callers that have already done their own
// positioning. Translucent unless the caller forces otherwise, matching the
// HUD's existing text drawing.
//
void DCanvas::DrawFontText(const OFont* font, int normalcolor, int x, int y,
		const char* string, bool force_opaque) const
{
	DrawFontTextRaw(font, force_opaque ? EWrapper_Translated : EWrapper_TlatedLucent,
	                normalcolor, x, y, string);
}


//
// V_FontStringWidthClean
//
int V_FontStringWidthClean(const OFont* font, const char* str)
{
	if (!font || !str)
		return 0;

	return font->getTextWidth(str) / MAX(1, CleanXfac);
}

//
// V_FontLineHeightClean
//
int V_FontLineHeightClean(const OFont* font)
{
	if (!font)
		return 0;

	return font->getHeight() / MAX(1, CleanYfac);
}


//
// Find string width from hu_font chars
//
int V_StringWidth(const byte* str)
{
	// Default width without a font loaded is 8.
	if (!::hu_font[0])
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
		{
			width += 4;
		}
		else
		{
			width += hu_font[c]->mWidth;
		}
	}

	return width;
}

int V_StringHeight(const char* str)
{
	// Default width without a font loaded is 8.
	if (!::hu_font[0])
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

brokenlines_t* V_BreakLines(int maxwidth, const byte* str)
{
	if (!::hu_font[0])
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
		{
			nw = 4;
		}
		else
		{
			nw = hu_font[c - HU_FONTSTART]->mWidth;
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

//
// breakitFont
//
// The same as breakit, but measures with an OFont. Reported widths are in virtual
// units so that callers can keep centering in 320x200 space.
//
static void breakitFont(const OFont* font, brokenlines_t* line, const byte* start,
		const byte* string, const char* prefix = nullptr)
{
	// Leave out trailing white space
	while (string > start && isspace(*(string - 1)))
		string--;

	const size_t prefix_len = prefix ? strlen(prefix) : 0;

	line->string = new char[string - start + 1 + prefix_len];

	if (prefix_len)
		strncpy(line->string + 0, prefix, prefix_len);

	strncpy(line->string + prefix_len, reinterpret_cast<const char*>(start), string - start);
	line->string[string - start + prefix_len] = 0;
	line->width = V_FontStringWidthClean(font, line->string);
}


//
// V_BreakLinesFont
//
brokenlines_t* V_BreakLinesFont(const OFont* font, int maxwidth, const byte* str)
{
	return V_BreakLinesFontPixels(font, maxwidth * MAX(1, CleanXfac), str);
}


//
// V_BreakLinesFontPixels
//
// The same as V_BreakLinesFont, but the width is already in real pixels.
// Callers that lay out in their own coordinate system -- the HUD, at its
// own scale -- use this so the wrap width matches the font the text is
// actually drawn with.
//
brokenlines_t* V_BreakLinesFontPixels(const OFont* font, int maxwidth_px, const byte* str)
{
	if (!font || !str)
		return NULL;

	brokenlines_t lines[128];	// Support up to 128 lines (should be plenty)

	const byte* space = NULL;
	const byte* start = str;
	bool lastWasSpace = false;
	int i = 0, w = 0, nw = 0;

	char color_code_str[4] = { 0 };

	while (*str)
	{
		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			snprintf(color_code_str, 4, "\034%c", str[1]);
			str += 2;
			continue;
		}

		// no case folding here: unlike the Doom bitmap fonts, a TrueType
		// face has real lowercase glyphs
		const int c = *str++;

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

		nw = font->getTextWidth((char)c);

		if (w + nw > maxwidth_px || c == '\n')
		{
			// Time to break the line
			if (!space)
				space = str - 1;

			breakitFont(font, &lines[i], start, space, color_code_str);

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
			if (!isspace(*s++))
			{
				breakitFont(font, &lines[i++], start, str, color_code_str);
				break;
			}
		}
	}

	// Make a copy of the broken lines and return them
	brokenlines_t* broken = new brokenlines_t[i + 1];

	memcpy(broken, lines, sizeof(brokenlines_t) * i);
	broken[i].string = NULL;
	broken[i].width = -1;

	return broken;
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
