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

#include <algorithm>
#include <array>
#include <map>

#include "v_text.h"

#include "i_video.h"
#include "v_video.h"
#include "hu_stuff.h"
#include "w_wad.h"

#include "hashtable.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "r_draw.h"
#include "v_palette.h"

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

namespace
{
OFontColors hu_bigfont_colors;
OFontColors hu_smallfont_colors;
OFontColors hu_digfont_colors;
} // namespace

std::vector<byte> ConChars;
OFontColors ConCharsColors;

void OFontColors::reset(size_t glyphs)
{
	m_glyph_source.assign(glyphs, -1);
	m_sources.clear();
	m_sets.clear();
}

void OFontColors::addSource(const std::string& name, const std::vector<size_t>& glyphs,
                            const colorcounts_t& counts)
{
	colorset_t& colors = m_sets.emplace_back();
	for (int color = 0; color < NUM_TEXT_COLORS; color++)
	{
		R_ClearTranslation(colors[color]);

		argb_t bright;
		argb_t dark;
		if (R_GetFontColorRange(color, bright, dark))
			R_BuildTranslationLuminance(colors[color], counts, bright, dark);
	}

	const int source = static_cast<int>(m_sources.size());
	m_sources.push_back(
	    {.name = name,
	     .glyphs = glyphs.size(),
	     .colors = static_cast<size_t>(
	         std::ranges::count_if(counts, [](uint32_t n) { return n != 0; }))});

	for (const size_t glyph : glyphs)
	{
		if (glyph < m_glyph_source.size())
			m_glyph_source[glyph] = source;
	}
}

translationref_t OFontColors::get(size_t glyph, int color) const
{
	if (color < 0 or color >= NUM_TEXT_COLORS)
		color = CR_UNTRANSLATED;

	const int source = glyph < m_glyph_source.size() ? m_glyph_source[glyph] : -1;

	// a stand-in for a glyph no resource supplied has no art to color
	if (source == -1)
	{
		static const translationtable_t untranslated = [] {
			translationtable_t tlate;
			R_ClearTranslation(tlate);
			return tlate;
		}();
		return untranslated.ref();
	}

	return m_sets[source][color].ref();
}

//
// V_BuildFontColors
//
// Groups a font's glyphs by the resource file each came from, since a PWAD
// often replaces only some of them, and works out how each group takes color.
//
void V_BuildFontColors(OFontColors& colors, const std::array<int, HU_FONTSIZE>& lumps)
{
	colors.reset(HU_FONTSIZE);

	std::map<int, std::vector<size_t>> by_file;
	for (size_t i = 0; i < lumps.size(); i++)
	{
		if (lumps[i] != -1)
			by_file[::lumpinfo[lumps[i]].file].push_back(i);
	}

	for (const auto& [file, glyphs] : by_file)
	{
		colorcounts_t counts{};
		for (const size_t glyph : glyphs)
			R_CountPatchColors(W_CachePatch(lumps[glyph]), counts);

		colors.addSource(std::string(W_LumpFileName(lumps[glyphs.front()])), glyphs, counts);
	}
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

	// The lump each glyph came from, -1 where a stand-in fills a gap.
	std::array<int, HU_FONTSIZE> bigfont_lumps{};
	std::array<int, HU_FONTSIZE> smallfont_lumps{};
	std::array<int, HU_FONTSIZE> digfont_lumps{};

	// Level name font, used between levels, starts at index 1.
	j = 1;
	sub = 0;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		buffer = fmt::sprintf(bigfont, j++ - sub);

		// Some letters of this font are missing.
		int num = W_CheckNumForName(buffer.c_str());
		bigfont_lumps[i] = num;
		if (num != -1)
			::hu_bigfont[i] = W_CachePatchHandle(buffer.c_str(), PU_STATIC);
		else
			::hu_bigfont[i] = W_CachePatchHandle("TNT1A0", PU_STATIC, ns_sprites);
	}

	// Normal doom chat/message font, starts at index 33.
	j = HU_FONTSTART;
	sub = 0;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		buffer = fmt::sprintf(smallfont, j++ - sub);
		::hu_smallfont[i] = W_CachePatchHandle(buffer.c_str(), PU_STATIC);
		smallfont_lumps[i] = W_CheckNumForName(buffer.c_str());
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
		int num = W_CheckNumForName(buffer.c_str());
		digfont_lumps[i] = num;
		if (num != -1)
		{
			::hu_digfont[i] = W_CachePatchHandle(buffer.c_str(), PU_STATIC);
		}
		else
		{
			::hu_digfont[i] = W_CachePatchHandle("TNT1A0", PU_STATIC, ns_sprites);
		}
	}

	// Font heights.
	::hu_bigfont_height =
	    W_ResolvePatchHandle(::hu_bigfont['M' - HU_FONTSTART])->height();
	::hu_smallfont_height =
	    W_ResolvePatchHandle(::hu_smallfont['M' - HU_FONTSTART])->height();
	::hu_digfont_height =
	    W_ResolvePatchHandle(::hu_digfont['M' - HU_FONTSTART])->height();

	V_BuildFontColors(::hu_bigfont_colors, bigfont_lumps);
	V_BuildFontColors(::hu_smallfont_colors, smallfont_lumps);
	V_BuildFontColors(::hu_digfont_colors, digfont_lumps);

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
		::hu_font.setFont(::hu_bigfont, ::hu_bigfont_height, &::hu_bigfont_colors);
	else if (!stricmp(fontname, "SMALLFONT"))
		::hu_font.setFont(::hu_smallfont, ::hu_smallfont_height, &::hu_smallfont_colors);
	else if (!stricmp(fontname, "DIGFONT"))
		::hu_font.setFont(::hu_digfont, ::hu_digfont_height, &::hu_digfont_colors);
}

const OFontColors* V_FontColors(const char* fontname)
{
	if (not stricmp(fontname, "BIGFONT"))
		return &::hu_bigfont_colors;
	if (not stricmp(fontname, "SMALLFONT"))
		return &::hu_smallfont_colors;
	if (not stricmp(fontname, "DIGFONT"))
		return &::hu_digfont_colors;
	if (not stricmp(fontname, "CONCHARS"))
		return &::ConCharsColors;

	return nullptr;
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
        set('G', 'g', CR_RED);
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
	if (::ConChars.empty())
		return;

	const int char_size = 8 * scale;

	if (default_color < 0)
		default_color = CR_GRAY;

	int color = default_color;

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
			color = V_GetTextColor(str);
			if (color == -1)
				color = default_color;

			str += 2;
			continue;
		}

		int c = static_cast<byte>(*str);

		// each glyph can come from a different sheet, with colors of its own
		const translationref_t trans = ::ConCharsColors.get(c, color);

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
	if (::hu_font[0].empty())
		return;

	if (normalcolor < 0 || normalcolor > NUM_TEXT_COLORS)
		normalcolor = CR_RED;

	int color = normalcolor;

	int cx = x;
	int cy = y;

	const char*	str = reinterpret_cast<const char*>(string);

	while (1)
	{
		if (str[0] == '\0')
			break;

		if (str[0] == TEXTCOLOR_ESCAPE && str[1] != '\0')
		{
			color = V_GetTextColor(str);
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

		// each glyph can come from a different file, with colors of its own
		V_ColorMap = ::hu_font.colors().get(c, color);

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


//
// Look up one of the fonts by name, or nullptr if there is no such font.
//
namespace
{
const lumpHandle_t* V_FontByName(const char* fontname)
{
	if (!stricmp(fontname, "BIGFONT"))
		return ::hu_bigfont;
	if (!stricmp(fontname, "SMALLFONT"))
		return ::hu_smallfont;
	if (!stricmp(fontname, "DIGFONT"))
		return ::hu_digfont;

	return nullptr;
}
} // namespace

//
// Temporary cvar dumpfontcolors
//
// Reports which palette indices a font's glyphs actually uses.
//
// Recoloring a font means translating those indices, which are
// placed between the dark and bright ends of each text color by
// their luminance, as UZDoom does.
//
// Each source the font was pieced together from is listed with the number of
// colors its art uses, which its text color translations are built from.
//
BEGIN_COMMAND(dumpfontcolors)
{
	const lumpHandle_t* font = ::hu_font.data();
	const OFontColors* colors = font != nullptr ? &::hu_font.colors() : nullptr;
	const char* fontname = "current";

	if (argc > 1)
	{
		font = V_FontByName(argv[1]);
		colors = V_FontColors(argv[1]);
		fontname = argv[1];

		if (colors == nullptr)
		{
			PrintFmt(PRINT_HIGH, "dumpfontcolors [BIGFONT|SMALLFONT|DIGFONT|CONCHARS]\n");
			return;
		}
	}

	if (colors != nullptr)
	{
		for (const OFontColors::source_t& source : colors->sources())
		{
			PrintFmt(PRINT_HIGH, "{} font from {}: {} glyphs in {} colors\n", fontname,
			         source.name, source.glyphs, source.colors);
		}
	}

	// the console font is not made of patches, so there is nothing more to sample
	if (font == nullptr)
	{
		if (colors == nullptr)
			PrintFmt(PRINT_HIGH, "No font loaded.\n");
		return;
	}

	std::vector<const patch_t*> glyphs;
	for (int i = 0; i < HU_FONTSIZE; i++)
	{
		if (!font[i].empty())
			glyphs.push_back(W_ResolvePatchHandle(font[i]));
	}

	std::vector<palindex_t> indices;
	R_SampleLuminosity(glyphs.data(), glyphs.size(), indices);

	if (indices.empty())
	{
		PrintFmt(PRINT_HIGH, "{} font: {} glyphs, no colors sampled.\n", fontname,
		         glyphs.size());
		return;
	}

	PrintFmt(PRINT_HIGH, "{} font: {} glyphs, {} colors.\n", fontname, glyphs.size(),
	         indices.size());

	// Ascending, collapsed into runs, to line up against the hardcoded ramps.
	std::vector<palindex_t> ascending = indices;
	std::ranges::sort(ascending);

	std::string ranges;
	for (size_t i = 0; i < ascending.size();)
	{
		size_t run = i;
		while (run + 1 < ascending.size() && ascending[run + 1] == ascending[run] + 1)
			run++;

		if (!ranges.empty())
			ranges += ", ";

		if (run == i)
			ranges += fmt::format("{:02x}", ascending[i]);
		else
			ranges += fmt::format("{:02x}-{:02x}", ascending[i], ascending[run]);

		i = run + 1;
	}
	PrintFmt(PRINT_HIGH, "  indices: {}\n", ranges);

	// The order a translation would actually walk them in.
	const palette_t* pal = V_GetDefaultPalette();
	std::string ordered;
	for (const palindex_t index : indices)
	{
		if (!ordered.empty())
			ordered += " ";

		ordered += fmt::format("{:02x}", index);
	}
	PrintFmt(PRINT_HIGH, "  darkest first: {}\n", ordered);
	PrintFmt(PRINT_HIGH, "  luminance {:.3f} to {:.3f}\n",
	         V_Luminance(pal->basecolors[indices.front()]),
	         V_Luminance(pal->basecolors[indices.back()]));
}
END_COMMAND(dumpfontcolors)

VERSION_CONTROL (v_text_cpp, "$Id$")
