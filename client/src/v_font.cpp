// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// Bitmapped font routines
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <algorithm>
#include <vector>

#include "i_system.h"
#include "c_cvars.h"
#include "doomfunc.h"
#include "v_video.h"
#include "v_palette.h"
#include "v_text.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"
#include "z_zone.h"
#include "m_random.h"

#include "v_font.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_IMAGE_H

extern byte* Ranges;

// The palette's transparency slot. Column drawers skip pixels holding it.
static const palindex_t TRANSPARENT_INDEX		= 0;

// Doom's font glyphs are recolored into the translation range so that
// V_ColorMap can tint them to any text color at draw time.
static const palindex_t TRANSLATION_RANGE_START	= 0xB0;
static const palindex_t TRANSLATION_RANGE_END	= 0xBF;

// the palette range the stock Doom font graphics actually use
static const palindex_t DOOM_FONT_RANGE_START	= 0x50;
static const palindex_t DOOM_FONT_RANGE_END		= 0x5F;


//
// V_TranslateFontChar
//
// Shifts a glyph's pixels from the palette range the stock Doom font
// graphics uses into the translation range, so that the glyph can be
// recolored when it is drawn.
//
static void V_TranslateFontChar(Texture* texture)
{
	palindex_t* data = texture->mData;
	const int count = texture->mWidth * texture->mHeight;

	for (int i = 0; i < count; i++)
	{
		if (data[i] >= DOOM_FONT_RANGE_START && data[i] <= DOOM_FONT_RANGE_END)
			data[i] += (TRANSLATION_RANGE_START - DOOM_FONT_RANGE_START);
	}
}


//
// V_FillTexture
//
// Fills dest_texture with source_texture, starting from a random x and y
// offset and tiling.
//
static void V_FillTexture(Texture* dest_texture, const Texture* source_texture)
{
	if (!source_texture || source_texture->mWidth == 0 || source_texture->mHeight == 0)
		return;

	int bgx = M_Random() % source_texture->mWidth;
	int bgy = M_Random() % source_texture->mHeight;

	for (int ty = 0; ty < dest_texture->mHeight; )
	{
		int block_height = std::min(dest_texture->mHeight - ty, source_texture->mHeight - bgy);

		for (int tx = 0; tx < dest_texture->mWidth; )
		{
			int block_width = std::min(dest_texture->mWidth - tx, source_texture->mWidth - bgx);

			Res_CopySubimage(dest_texture, source_texture,
				tx, ty, tx + block_width - 1, ty + block_height - 1,
				bgx, bgy, bgx + block_width - 1, bgy + block_height - 1);

			tx += block_width;
			bgx = (bgx + block_width) % source_texture->mWidth;
		}

		ty += block_height;
		bgy = (bgy + block_height) % source_texture->mHeight;
	}
}


//
// V_FillGradient
//
// Fills a texture's color plane with a vertical gradient running from
// start_color at the top to end_color at the bottom over dist rows.
//
static void V_FillGradient(Texture* dest_texture, palindex_t start_color, palindex_t end_color, int dist)
{
	palindex_t* dest = dest_texture->mData;
	memset(dest, end_color, dest_texture->mWidth * dest_texture->mHeight * sizeof(palindex_t));

	if (dist > 0)
	{
		const int color_count = end_color - start_color + 1;
		const fixed_t frac = FixedDiv(color_count * FRACUNIT, dist * FRACUNIT);

		// the color plane is column-major, so the inner loop walks a column
		for (int x = 0; x < dest_texture->mWidth; x++)
		{
			for (int y = 0; y < dest_texture->mHeight; y++)
			{
				int color = start_color + ((y * frac) >> FRACBITS);
				color = MAX(color, (int)start_color);
				color = MIN(color, (int)end_color);
				*dest++ = static_cast<palindex_t>(color);
			}
		}
	}
}


// ----------------------------------------------------------------------------
//
// OFont base class implementation
//
// ----------------------------------------------------------------------------

//
// The set of live fonts. Kept in a function-local static so that a font
// constructed during static initialization still finds a valid list.
//
static std::vector<OFont*>& V_FontRegistry()
{
	static std::vector<OFont*> registry;
	return registry;
}

//
// Stock scale providers
//
fixed_t V_FontScaleHudText()
{
	return V_TextScaleXAmount() * FRACUNIT;
}

fixed_t V_FontScaleClean()
{
	return MIN(CleanXfac, CleanYfac) * FRACUNIT;
}


OFont::OFont() :
	mMissingGlyphWidth(0), mLoaded(false), mScale(FRACUNIT), mScaleFunc(nullptr)
{
	for (int i = 0; i < 256; i++)
		mCharacters[i] = nullptr;

	V_FontRegistry().push_back(this);
}

OFont::~OFont()
{
	unload();

	std::vector<OFont*>& registry = V_FontRegistry();
	registry.erase(std::remove(registry.begin(), registry.end(), this), registry.end());
}

void OFont::setScale(fixed_t scale)
{
	mScale = scale;
	mScaleFunc = nullptr;
}

void OFont::setScale(ScaleFunc scale_func)
{
	mScaleFunc = scale_func;
}

fixed_t OFont::getScale() const
{
	// resolved on every load so that a rebuild picks up the current value
	return mScaleFunc ? mScaleFunc() : mScale;
}

bool OFont::isUsable() const
{
	// a font with no letterforms is one whose source resource was missing
	const Texture* texture = mCharacters[(byte)'T'];
	return texture != nullptr && texture->mWidth > 0 && texture->mHeight > 0;
}

void OFont::load()
{
	unload();
	buildGlyphs();
	mLoaded = true;
}

void OFont::unload()
{
	for (size_t i = 0; i < mOwnedTextures.size(); i++)
		Z_Free(mOwnedTextures[i]);

	mOwnedTextures.clear();

	// hand the source graphics back so that a resource reload can free them
	for (size_t i = 0; i < mCachedResources.size(); i++)
		Res_ReleaseResource(mCachedResources[i]);

	mCachedResources.clear();

	for (int i = 0; i < 256; i++)
		mCharacters[i] = nullptr;

	mMissingGlyphWidth = 0;
	mLoaded = false;
}


//
// V_FontInit
//
void V_FontInit()
{
	std::vector<OFont*>& registry = V_FontRegistry();
	for (size_t i = 0; i < registry.size(); i++)
		registry[i]->load();
}


//
// V_FontShutdown
//
void V_FontShutdown()
{
	std::vector<OFont*>& registry = V_FontRegistry();
	for (size_t i = 0; i < registry.size(); i++)
		registry[i]->unload();
}


//
// V_FontRebuild
//
void V_FontRebuild()
{
	std::vector<OFont*>& registry = V_FontRegistry();
	for (size_t i = 0; i < registry.size(); i++)
	{
		if (registry[i]->isLoaded())
			registry[i]->load();
	}
}


//
// Rebuild scaled fonts when the text scale cvar changes.
//
CVAR_FUNC_IMPL(hud_scaletext)
{
	V_FontRebuild();
}

const Texture* OFont::cacheSourceTexture(const char* name)
{
	const ResourceId res_id = Res_GetTextureResourceId(name, GRAPHICS);
	if (!Res_CheckResource(res_id))
		return nullptr;

	const Texture* texture = Res_CacheTexture(res_id, PU_STATIC);
	if (!texture || texture->mWidth == 0 || texture->mHeight == 0)
	{
		Res_ReleaseResource(res_id);
		return nullptr;
	}

	mCachedResources.push_back(res_id);
	return texture;
}

Texture* OFont::createGlyph(int width, int height)
{
	const uint32_t size = Texture::calculateSize(width, height);
	Texture* texture = static_cast<Texture*>(Z_Malloc(size, PU_STATIC, nullptr));
	texture->init(width, height);

	mOwnedTextures.push_back(texture);
	return texture;
}

int OFont::getAdvanceX(char c) const
{
	return 0;
}

int OFont::getAdvanceY(char c) const
{
	return 0;
}

void OFont::printCharacter(const DCanvas* canvas, int& x, int& y, char c) const
{
	if (c == ' ' || c == '\t')
	{
		x += getTextWidth(c);
	}
	else if (c == '\r' || c == '\n')
	{
		return;
	}
	else
	{
		const Texture* texture = mCharacters[static_cast<byte>(c)];
		if (texture)
		{
			// glyph bearings are relative to the baseline, which sits
			// getAscent() below the top of the line
			canvas->DrawTranslatedTexture(texture, x, y + getAscent());
		}
		x += getTextWidth(c);
	}
}

void OFont::printText(const DCanvas* canvas, int x, int y, int color, const char* str) const
{
	V_ColorMap = translationref_t(Ranges + color * 256);

	while (*str)
	{
		printCharacter(canvas, x, y, *str);
		str++;
	}
}

int OFont::getGlyphAdvance(char c) const
{
	// A bitmap font's glyph is its own metric: step by the drawn width.
	const Texture* texture = mCharacters[static_cast<byte>(c)];
	return texture ? texture->mWidth - texture->mOffsetX : 0;
}

int OFont::getTextWidth(char c) const
{
	if (c == '\t')
		return 4 * getTextWidth(' ');

	const Texture* texture = mCharacters[static_cast<byte>(c)];
	if (!texture)
	{
		// a character the font has no glyph for: draw nothing, but leave a
		// gap so words do not run together
		return mMissingGlyphWidth;
	}

	const int advance = getGlyphAdvance(c);
	if (advance > 0)
		return advance;

	// no advance and nothing drawn -- a bitmap font's blank space glyph
	if (c == ' ')
		return getTextWidth('T') / 2;

	return 0;
}

int OFont::getTextWidth(const char* str) const
{
	int width = 0;

	while (*str)
	{
		width += getTextWidth(*str);
		str++;
	}

	return width;
}

int OFont::getTextHeight(char c) const
{
	const Texture* texture = mCharacters[static_cast<byte>(c)];

	if (c == '\t')
		return 4 * getTextHeight(' ');

	if (c == ' ' && (!texture || texture->mHeight == 0))
		return getTextHeight('T');

	if (texture)
		return texture->mHeight - texture->mOffsetY + getAdvanceY(c);
	else
		return 0;
}

int OFont::getTextHeight(const char* str) const
{
	int height = 0;

	while (*str)
	{
		height = MAX(height, getTextHeight(*str));
		str++;
	}

	return height;
}


// ----------------------------------------------------------------------------
//
// ConCharsFont implementation
//
// ----------------------------------------------------------------------------

ConCharsFont::ConCharsFont(fixed_t scale) :
	mHeight(0)
{
	setScale(scale);
	load();
}

ConCharsFont::ConCharsFont(ScaleFunc scale_func) :
	mHeight(0)
{
	setScale(scale_func);
	load();
}

void ConCharsFont::buildGlyphs()
{
	const fixed_t scale = getScale();
	static const char lumpname[] = "CONCHARS";
	const Texture* conchars_texture = cacheSourceTexture(lumpname);
	if (!conchars_texture)
		I_Error("Unable to load {} lump.", lumpname);

	const int numcolumns = conchars_texture->mWidth / charwidth;
	const int numrows = conchars_texture->mHeight / charheight;

	// scaled size of characters
	const int dest_charwidth = (charwidth * scale) >> FRACBITS;
	const int dest_charheight = (charheight * scale) >> FRACBITS;

	for (int row = 0; row < numrows; row++)
	{
		for (int column = 0; column < numcolumns; column++)
		{
			const int charnum = row * numcolumns + column;
			if (charnum >= 256)
				continue;

			Texture* texture = createGlyph(dest_charwidth, dest_charheight);
			mCharacters[charnum] = texture;

			const int x1 = column * charwidth;
			const int x2 = x1 + charwidth - 1;
			const int y1 = row * charheight;
			const int y2 = y1 + charheight - 1;

			Res_CopySubimage(texture, conchars_texture,
					0, 0, dest_charwidth - 1, dest_charheight - 1,
					x1, y1, x2, y2);

			V_TranslateFontChar(texture);
		}
	}

	// base font height on the letter T
	mHeight = mCharacters['T'] ? mCharacters['T']->mHeight : dest_charheight;
}


// ----------------------------------------------------------------------------
//
// SmallDoomFont implementation
//
// ----------------------------------------------------------------------------

SmallDoomFont::SmallDoomFont(fixed_t scale) :
	mHeight(0)
{
	setScale(scale);
	load();
}

SmallDoomFont::SmallDoomFont(ScaleFunc scale_func) :
	mHeight(0)
{
	setScale(scale_func);
	load();
}

void SmallDoomFont::buildGlyphs()
{
	const fixed_t scale = getScale();
	char name[12];

	// load the glyphs
	for (int charnum = '!'; charnum <= '_'; charnum++)
	{
		snprintf(name, sizeof(name), "STCFN%.3d", charnum);

		const Texture* source_texture = cacheSourceTexture(name);
		if (!source_texture)
			continue;

		const int dest_charwidth = (source_texture->mWidth * scale) >> FRACBITS;
		const int dest_charheight = (source_texture->mHeight * scale) >> FRACBITS;

		Texture* texture = createGlyph(dest_charwidth, dest_charheight);
		Res_CopySubimage(texture, source_texture,
				0, 0, dest_charwidth - 1, dest_charheight - 1,
				0, 0, source_texture->mWidth - 1, source_texture->mHeight - 1);

		V_TranslateFontChar(texture);
		mCharacters[charnum] = texture;
	}

	// lowercase glyphs are the same as uppercase
	for (int charnum = 'a'; charnum <= 'z'; charnum++)
		mCharacters[charnum] = mCharacters[charnum - 32];

	// add blank glyphs for any not present in the font
	const Texture* blank_texture = createGlyph((4 * scale) >> FRACBITS, (7 * scale) >> FRACBITS);

	for (int charnum = 0; charnum < 256; charnum++)
	{
		if (mCharacters[charnum] == nullptr)
			mCharacters[charnum] = blank_texture;
	}

	mHeight = mCharacters['T']->mHeight + (scale >> FRACBITS);
}


// ----------------------------------------------------------------------------
//
// LargeDoomFont implementation
//
// ----------------------------------------------------------------------------

LargeDoomFont::LargeDoomFont(fixed_t scale) :
	mHeight(0)
{
	setScale(scale);
	load();
}

LargeDoomFont::LargeDoomFont(ScaleFunc scale_func) :
	mHeight(0)
{
	setScale(scale_func);
	load();
}

void LargeDoomFont::buildGlyphs()
{
	const fixed_t scale = getScale();
	char name[12];

	// load the glyphs
	for (int charnum = '!'; charnum <= 'Z'; charnum++)
	{
		snprintf(name, sizeof(name), "FONTB%02u", charnum - 32);

		const Texture* source_texture = cacheSourceTexture(name);
		if (!source_texture)
			continue;

		const int dest_charwidth = (source_texture->mWidth * scale) >> FRACBITS;
		const int dest_charheight = (source_texture->mHeight * scale) >> FRACBITS;

		Texture* texture = createGlyph(dest_charwidth, dest_charheight);
		Res_CopySubimage(texture, source_texture,
				0, 0, dest_charwidth - 1, dest_charheight - 1,
				0, 0, source_texture->mWidth - 1, source_texture->mHeight - 1);

		V_TranslateFontChar(texture);
		mCharacters[charnum] = texture;
	}

	// lowercase glyphs are the same as uppercase
	for (int charnum = 'a'; charnum <= 'z'; charnum++)
		mCharacters[charnum] = mCharacters[charnum - 32];

	// add blank glyphs for any not present in the font
	const Texture* blank_texture = createGlyph((12 * scale) >> FRACBITS, (12 * scale) >> FRACBITS);

	for (int charnum = 0; charnum < 256; charnum++)
	{
		if (mCharacters[charnum] == nullptr)
			mCharacters[charnum] = blank_texture;
	}

	mHeight = mCharacters['T']->mHeight + (scale >> FRACBITS);
}


// ----------------------------------------------------------------------------
//
// TrueTypeFont implementation
//
// ----------------------------------------------------------------------------

TrueTypeFont::TrueTypeFont(const char* lumpname, int size, unsigned int stylemask) :
	mLumpName(lumpname), mBaseSize(size), mStyleMask(stylemask), mHeight(0), mAscent(0)
{
	load();
}

TrueTypeFont::TrueTypeFont(const char* lumpname, int size, unsigned int stylemask,
		ScaleFunc scale_func) :
	mLumpName(lumpname), mBaseSize(size), mStyleMask(stylemask), mHeight(0), mAscent(0)
{
	setScale(scale_func);
	load();
}

void TrueTypeFont::buildGlyphs()
{
	const char* lumpname = mLumpName.c_str();
	const unsigned int stylemask = mStyleMask;

	// the requested pixel size is the base size taken at the current scale
	const int size = MAX(1, (mBaseSize * getScale()) >> FRACBITS);

	mHeight = 0;
	mAscent = 0;
	memset(mAdvanceX, 0, sizeof(mAdvanceX));
	memset(mAdvanceY, 0, sizeof(mAdvanceY));

	// read the TTF out of the engine resource file
	const ResourceId res_id = Res_GetResourceId(lumpname, global_directory_name);
	if (!Res_CheckResource(res_id))
	{
		PrintFmt(PRINT_HIGH, "Unable to locate TrueType font {}!\n", lumpname);
		return;
	}

	// FreeType reads from this buffer for the lifetime of the face, so keep
	// our own copy rather than relying on the resource cache holding it.
	const uint32_t lumplen = Res_GetResourceSize(res_id);
	const void* lumpdata = Res_LoadResource(res_id, PU_STATIC);
	if (lumplen == 0 || lumpdata == nullptr)
	{
		PrintFmt(PRINT_HIGH, "Unable to read TrueType font {}!\n", lumpname);
		return;
	}

	std::vector<byte> rawlumpdata(lumplen);
	memcpy(&rawlumpdata[0], lumpdata, lumplen);
	Res_ReleaseResource(res_id);

	// Initialize FreeType 2
	FT_Library ftlibrary;
	int error = FT_Init_FreeType(&ftlibrary);
	if (error)
	{
		PrintFmt(PRINT_HIGH, "Error initializing FreeType 2 library: {}\n", error);
		return;
	}

	// open the TTF for usage
	FT_Face face;
	error = FT_New_Memory_Face(ftlibrary, &rawlumpdata[0], lumplen, 0, &face);
	if (error)
	{
		PrintFmt(PRINT_HIGH, "Error loading TrueType font {}: {}\n", lumpname, error);
		FT_Done_FreeType(ftlibrary);
		return;
	}

	// set the size of the font
	error = FT_Set_Pixel_Sizes(face, 0, size);
	if (error)
	{
		PrintFmt(PRINT_HIGH, "Error resizing TrueType font {}: {}\n", lumpname, error);
		FT_Done_Face(face);
		FT_Done_FreeType(ftlibrary);
		return;
	}

	const Texture* background_texture = nullptr;
	if (stylemask & TTF_TEXTURE)
		background_texture = cacheSourceTexture("FONTBACK");

	// stand-in for a fill pixel that landed on the transparency slot
	const palindex_t opaque_fill_fallback =
			V_BestOpaqueColor(V_GetDefaultPalette()->basecolors, argb_t(0, 0, 0));

	for (int charnum = ' '; charnum < '~'; charnum++)
	{
		if (FT_Get_Char_Index(face, charnum) == 0)
			continue;

		error = FT_Load_Char(face, charnum, FT_LOAD_RENDER);
		if (error)
		{
			PrintFmt(PRINT_HIGH, "Error loading TrueType font {} glyph {}: {}\n", lumpname, charnum, error);
			continue;
		}

		const int width = face->glyph->bitmap.width;
		const int height = face->glyph->bitmap.rows;
		const int pitch = face->glyph->bitmap.pitch;

		mAdvanceX[charnum] = face->glyph->advance.x >> 6;
		mAdvanceY[charnum] = face->glyph->advance.y >> 6;

		Texture* texture = createGlyph(width, height);
		mCharacters[charnum] = texture;

		if (width == 0 || height == 0)
			continue;

		if (background_texture)
		{
			// tile a texture behind the glyph
			V_FillTexture(texture, background_texture);
		}
		else if (stylemask & TTF_GRADIENT)
		{
			// gradient from light (top) to dark (bottom)
			V_FillGradient(texture, TRANSLATION_RANGE_START, TRANSLATION_RANGE_END, size);
		}
		else
		{
			// set the color plane to a solid
			memset(texture->mData, TRANSLATION_RANGE_START, width * height * sizeof(palindex_t));
		}

		// Set the glyph's bearing only after the fill: filling from a
		// background graphic goes through Res_CopySubimage, which copies the
		// source's offsets onto the destination and would otherwise wipe
		// these out.
		// DrawWrapper places a texture at (x - mOffsetX, y - mOffsetY), and
		// we hand it the pen position on the baseline. FreeType's bearings
		// put the glyph at (pen + bitmap_left, baseline - bitmap_top), so
		// the offsets are the negation of one and the plain value of the
		// other. Getting mOffsetY's sign wrong makes every glyph deflect
		// the wrong way from the baseline.
		texture->mOffsetX = -face->glyph->bitmap_left;
		texture->mOffsetY = face->glyph->bitmap_top;

		// Cut the glyph's shape out of the fill laid down above. Coverage is
		// thresholded to a hard edge, and everything outside it becomes the
		// transparent palette index.
		//
		// A fill is only allowed to contain the transparent index where the
		// glyph isn't, so any opaque pixel that landed on it is nudged to the
		// nearest opaque color instead -- otherwise a background graphic with
		// transparency of its own would punch holes through the glyph.
		const byte* source = reinterpret_cast<const byte*>(face->glyph->bitmap.buffer);
		palindex_t* dest = texture->mData;

		for (int x = 0; x < width; x++)
		{
			for (int y = 0; y < height; y++)
			{
				const byte pixel = *(source + pitch * y + x);
				if (pixel > 127)
				{
					if (*dest == TRANSPARENT_INDEX)
						*dest = opaque_fill_fallback;
				}
				else
				{
					*dest = TRANSPARENT_INDEX;
				}
				dest++;
			}
		}
	}

	mAscent = face->size->metrics.ascender >> 6;
	mHeight = face->size->metrics.height >> 6;

	if (mHeight <= 0)
		mHeight = mAscent - (face->size->metrics.descender >> 6);

	FT_Done_Face(face);
	FT_Done_FreeType(ftlibrary);

	mMissingGlyphWidth = MAX(1, size / 2);
}

int TrueTypeFont::getGlyphAdvance(char c) const
{
	return mAdvanceX[static_cast<byte>(c)];
}

int TrueTypeFont::getAdvanceX(char c) const
{
	return mAdvanceX[static_cast<byte>(c)];
}

int TrueTypeFont::getAdvanceY(char c) const
{
	return mAdvanceY[static_cast<byte>(c)];
}
