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

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "i_system.h"
#include "v_video.h"
#include "resources/res_texture.h"
#include "z_zone.h"

static const char RULE_CHAR_FIRST	= 0x1D;
static const char RULE_CHAR_LAST	= 0x1F;

//
// OFontVariation
//
// A single OpenType variation-axis setting for a variable font.
// tag is a packed four-character axis tag
// Used to set things like font weight, optical size, etc.
//
struct OFontVariation
{
	uint32_t	tag;
	float		value;
};

//
// V_FontAxisTag
//
// Packs a four-character axis tag ("wght", "wdth", "opsz", "slnt", "ital", or
// any custom tag) into the integer form used by OFontVariation::tag.
//
uint32_t V_FontAxisTag(const char* tag);

//
// OFont
//
class OFont
{
public:
	typedef fixed_t (*ScaleFunc)();

	virtual ~OFont();

	virtual int getHeight() const = 0;
	virtual int getAdvanceX(char c) const;
	virtual int getAdvanceY(char c) const;
	int getTextWidth(char c) const;
	int getTextWidth(const char* str) const;
	virtual int getTextHeight(char c) const;
	int getTextHeight(const char* str) const;
	void printText(const DCanvas* canvas, int x, int y, int color, const char* str) const;

	// Builds the font's glyphs from the current resource set. Safe to call
	// repeatedly; any previously built glyphs are discarded first.
	void load();

	// Frees the font's glyphs and hands back its source graphics. The font
	// stays registered and can be loaded again later.
	void unload();

	bool isLoaded() const
	{	return mLoaded;	}

	// The glyph for a character, or NULL if the font has none.
	const Texture* getGlyph(char c) const
	{	return mCharacters[static_cast<byte>(c)];	}

	const byte* getGlyphCoverage(char c) const
	{	return mCharacterCoverage[static_cast<byte>(c)];	}

	const palindex_t* getGlyphFill(char c) const
	{	return mCharacterFill[static_cast<byte>(c)];	}

	// Distance from the top of a line of text down to the baseline that
	// glyphs sit on.
	virtual int getAscent() const
	{	return 0;	}

	// True if the font actually built usable glyphs.
	bool isUsable() const;

protected:
	OFont();

	// Builds the glyph lookup. Implementations may assume unload() has just
	// run, so every entry starts NULL.
	virtual void buildGlyphs() = 0;

	void setScale(fixed_t scale);
	void setScale(ScaleFunc scale_func);

	// The scale to build glyphs at, resolved fresh on each call.
	fixed_t getScale() const;

	// Allocates a glyph texture owned by this font, cleared to the
	// transparent palette index.
	Texture* createGlyph(int width, int height,
		byte** coverage_plane = nullptr, palindex_t** fill_plane = nullptr);

	// Caches a source graphic and records it so that the font releases it
	// again on unload. Returns NULL if the resource is missing or empty.
	const Texture* cacheSourceTexture(const char* name);

	// How far the pen moves after drawing a character.
	virtual int getGlyphAdvance(char c) const;

	const Texture*			mCharacters[256];
	const byte*				mCharacterCoverage[256];
	const palindex_t*		mCharacterFill[256];

	// Advance used for a character the font has no glyph for.
	int						mMissingGlyphWidth;

private:
	void printCharacter(const DCanvas* canvas, int& x, int& y, char c) const;

	bool						mLoaded;
	fixed_t						mScale;
	ScaleFunc					mScaleFunc;
	std::vector<Texture*>		mOwnedTextures;
	std::vector<ResourceId>		mCachedResources;
};


//
// Stock scale providers.
//
// V_FontScaleHudText follows the hud_scaletext cvar, falling back to the
// video mode's clean scaling factor when it is set to auto.
// V_FontScaleClean follows the clean scaling factor directly.
//
fixed_t V_FontScaleHudText();
fixed_t V_FontScaleClean();


//
// V_FontInit
//
// (Re)builds every registered font against the current resource set. Called
// from D_Init, so it runs again after each WAD change.
//
void V_FontInit();

//
// V_FontShutdown
//
// Releases every registered font's glyphs and source graphics, leaving the
// font objects themselves intact. Called from D_Shutdown.
//
void V_FontShutdown();

//
// V_FontRebuild
//
// Reloads only the fonts that are currently loaded, so that they pick up a
// new scale. Called when the video mode or a scale cvar changes; unlike
// V_FontInit it will not try to load fonts while the resource set is down.
//
void V_FontRebuild();


class ConCharsFont : public OFont
{
public:
	ConCharsFont(fixed_t scale);
	ConCharsFont(ScaleFunc scale_func);

	virtual int getHeight() const
	{	return mHeight;	}

protected:
	virtual void buildGlyphs();

private:
	static const int charwidth = 8;
	static const int charheight = 8;

	int				mHeight;
};


class SmallDoomFont : public OFont
{
public:
	SmallDoomFont(fixed_t scale);
	SmallDoomFont(ScaleFunc scale_func);

	virtual int getHeight() const
	{	return mHeight;	}

protected:
	virtual void buildGlyphs();

private:
	int				mHeight;
};


class LargeDoomFont : public OFont
{
public:
	LargeDoomFont(fixed_t scale);
	LargeDoomFont(ScaleFunc scale_func);

	virtual int getHeight() const
	{	return mHeight;	}

protected:
	virtual void buildGlyphs();

private:
	int				mHeight;
};


class TrueTypeFont : public OFont
{
public:
	enum
	{
		TTF_GRADIENT		= 0x01,
		TTF_TEXTURE			= 0x02,
		TTF_OUTLINE			= 0x04,
		TTF_SHADOW			= 0x08
	};

	TrueTypeFont(const char* lumpname, int size, unsigned int stylemask,
	             const std::vector<OFontVariation>& variations = std::vector<OFontVariation>());
	TrueTypeFont(const char* lumpname, int size, unsigned int stylemask,
	             ScaleFunc scale_func,
	             const std::vector<OFontVariation>& variations = std::vector<OFontVariation>());
	TrueTypeFont(const char* lumpname, int size, unsigned int stylemask,
	             argb_t grad_top, argb_t grad_bottom,
	             const std::vector<OFontVariation>& variations = std::vector<OFontVariation>());

	virtual int getHeight() const
	{	return mHeight;	}

	virtual int getAdvanceX(char c) const;
	virtual int getAdvanceY(char c) const;

	virtual int getAscent() const
	{	return mAscent;	}

	// Every line of an outline font occupies the face's line height,
	// regardless of which characters are on it.
	virtual int getTextHeight(char c) const
	{	return mHeight;	}

	// Sets the variable-font axis coordinates (weight, width, optical size,
	// slant, or any custom axis the face exposes) and rebuilds the glyphs if
	// the font is already loaded.
	void setVariations(const std::vector<OFontVariation>& variations);

protected:
	virtual void buildGlyphs();
	virtual int getGlyphAdvance(char c) const;

private:
	std::string		mLumpName;
	int				mBaseSize;
	unsigned int	mStyleMask;

	int				mHeight;
	int				mAscent;

	int				mAdvanceX[256];
	int				mAdvanceY[256];

	// Custom gradient endpoints (used only when mHasGradientColors).
	bool			mHasGradientColors;
	argb_t			mGradTop;
	argb_t			mGradBottom;

	// Requested variable-font axis coordinates
	std::vector<OFontVariation>	mVariations;
};
