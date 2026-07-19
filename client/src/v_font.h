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

#include <string>
#include <vector>

#include "i_system.h"
#include "v_video.h"
#include "resources/res_texture.h"
#include "z_zone.h"

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
	int getTextHeight(char c) const;
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
	Texture* createGlyph(int width, int height);

	// Caches a source graphic and records it so that the font releases it
	// again on unload. Returns NULL if the resource is missing or empty.
	const Texture* cacheSourceTexture(const char* name);

	const Texture*			mCharacters[256];

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

	TrueTypeFont(const char* lumpname, int size, unsigned int stylemask);
	TrueTypeFont(const char* lumpname, int size, unsigned int stylemask,
	             ScaleFunc scale_func);

	virtual int getHeight() const
	{	return mHeight;	}

	virtual int getAdvanceX(char c) const;
	virtual int getAdvanceY(char c) const;

protected:
	virtual void buildGlyphs();

private:
	std::string		mLumpName;
	int				mBaseSize;
	unsigned int	mStyleMask;

	int				mHeight;

	int				mAdvanceX[256];
	int				mAdvanceY[256];
};
