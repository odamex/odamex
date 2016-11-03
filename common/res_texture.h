// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
// Manager for texture resource loading and converting.
//
//-----------------------------------------------------------------------------

#ifndef __RES_TEXTURE_H__
#define __RES_TEXTURE_H__

#include "doomtype.h"
#include "m_fixed.h"

#include <vector>

#include "m_ostring.h"
#include "hashtable.h"

typedef unsigned int texhandle_t;

class Texture;
class TextureManager;

void Res_InitTextureManager();
void Res_ShutdownTextureManager();

const Texture* Res_LoadTexture(const char* name);

void Res_CopySubimage(Texture* dest_texture, const Texture* source_texture,
	int dx1, int dy1, int dx2, int dy2,
	int sx1, int sy1, int sx2, int sy2);

void Res_TransposeImage(byte* dest, const byte* source, int width, int height);

// ============================================================================
//
// Texture
//
// ============================================================================
//
// Texture is a straight-forward abstraction of Doom's various graphic formats.
// The image is stored in column-major format as a set of 8-bit palettized
// pixels.
//
// A Texture is allocated and initialized by TextureManager so that all memory
// used by a Texture can freed at the same time when the Zone heap needs to
// free memory.
//
class Texture
{
public:
	typedef enum
	{
		TEX_FLAT,
		TEX_PATCH,
		TEX_SPRITE,
		TEX_WALLTEXTURE,
		TEX_RAW,
		TEX_PNG,
	} TextureSourceType;

	static const unsigned int MAX_TEXTURE_WIDTH			= 2048;
	static const unsigned int MAX_TEXTURE_HEIGHT		= 2048;

	texhandle_t getHandle() const
	{	return mHandle;	}

	byte* getData() const
	{	return mData;	}

	byte* getMaskData() const
	{	return mMask;	}

	int getWidth() const
	{	return mWidth;	}

	int getHeight() const
	{	return mHeight;	}

	int getWidthBits() const
	{	return mWidthBits;	}

	int getHeightBits() const
	{	return mHeightBits;	}

	int getOffsetX() const
	{	return mOffsetX;	}

	int getOffsetY() const
	{	return mOffsetY;	}

	fixed_t getScaleX() const
	{	return mScaleX;	}

	fixed_t getScaleY() const
	{	return mScaleY;	}

	void setOffsetX(int value)
	{	mOffsetX = value;	}

	void setOffsetY(int value)
	{	mOffsetY = value;	}
	
private:
	friend class TextureManager;

	Texture();

	static size_t calculateSize(int width, int height);
	void init(int width, int height);

	texhandle_t			mHandle;

	fixed_t				mScaleX;
	fixed_t				mScaleY;

	unsigned short		mWidth;
	unsigned short		mHeight;
	
	short				mOffsetX;
	short				mOffsetY;

	byte				mWidthBits;
	byte				mHeightBits;

	bool				mHasMask;

	byte*				mMask;
	byte*				mData;

};


// ============================================================================
//
// TextureManager
//
// ============================================================================
//
// TextureManager provides a unified interface for loading and accessing the
// various types of graphic formats needed by Doom's renderer and interface.
// Its goal is to simplify loading graphic lumps and allow the different
// format graphic lumps to be used interchangeably, for example, allowing
// flats to be used as wall textures.
//
// A handle to a texture is retrieved by calling getHandle() with the name
// of the texture and a texture source type. The name can be either the name
// of a lump in the WAD (in the case of a flat, sprite, or patch) or it can
// be the name of an entry in one of the TEXTURE* lumps. The texture source
// type parameter is used to indicate where to initially search for the
// texture. For example, if the type is TEX_WALLTEXTURE, the TEXTURE* lumps
// will be searched for a matching name first, followed by flats, patches,
// and sprites.
//
// A pointer to a Texture object can be retrieved by passing a texture
// handle to getTexture(). The allocation and freeing of Texture objects is
// managed internally by TextureManager and as such, users should not store
// Texture pointers but instead store the texture handle and use getTexture
// when a Texture object is needed.
//
// TODO: properly load Heretic skies where the texture definition reports
// the sky texture is 128 pixels high but uses a 200 pixel high patch.
// The texture height should be adjusted to the height of the tallest
// patch in the texture.
//
class TextureManager
{
public:
	TextureManager();
	~TextureManager();

	void startup();
	void shutdown();

	void precache();

	void updateAnimatedTextures();

	texhandle_t getHandle(const char* name, Texture::TextureSourceType type);
	texhandle_t getHandle(const OString& name, Texture::TextureSourceType type);
	texhandle_t getHandle(unsigned int lumpnum, Texture::TextureSourceType type);
	const Texture* getTexture(texhandle_t handle);

	Texture* createTexture(texhandle_t handle, int width, int height);
	void freeTexture(texhandle_t handle);

	texhandle_t createCustomHandle();
	void freeCustomHandle(texhandle_t texhandle);

	static const texhandle_t NO_TEXTURE_HANDLE			= 0x0;
	static const texhandle_t NOT_FOUND_TEXTURE_HANDLE	= 0x1;
	static const texhandle_t GARBAGE_TEXTURE_HANDLE;

private:
	static const unsigned int FLAT_HANDLE_MASK			= 0x00010000ul;
	static const unsigned int PATCH_HANDLE_MASK			= 0x00020000ul;
	static const unsigned int SPRITE_HANDLE_MASK		= 0x00040000ul;
	static const unsigned int WALLTEXTURE_HANDLE_MASK	= 0x00080000ul;
	static const unsigned int RAW_HANDLE_MASK			= 0x000A0000ul;
	static const unsigned int PNG_HANDLE_MASK			= 0x00100000ul;
	static const unsigned int CUSTOM_HANDLE_MASK		= 0x01000000ul;

	// initialization routines
	void clear();
	void generateNotFoundTexture();
	void readPNamesDirectory();
	void addTextureDirectory(const char* lumpname); 
	void readAnimDefLump();
	void readAnimatedLump();

	// patches
	texhandle_t getPatchHandle(unsigned int lumpnum);
	texhandle_t getPatchHandle(const OString& name);
	void cachePatch(texhandle_t handle);

	// sprites
	texhandle_t getSpriteHandle(unsigned int lumpnum);
	texhandle_t getSpriteHandle(const OString& name);
	void cacheSprite(texhandle_t handle);

	// flats
	texhandle_t getFlatHandle(unsigned int lumpnum);
	texhandle_t getFlatHandle(const OString& name);
	void cacheFlat(texhandle_t handle);

	// wall textures
	texhandle_t getWallTextureHandle(unsigned int lumpnum);
	texhandle_t getWallTextureHandle(const OString& name);
	void cacheWallTexture(texhandle_t handle);

	// raw images
	texhandle_t getRawTextureHandle(unsigned int lumpnum);
	texhandle_t getRawTextureHandle(const OString& name);
	void cacheRawTexture(texhandle_t handle);

	// PNG images
	texhandle_t getPNGTextureHandle(unsigned int lumpnum);
	texhandle_t getPNGTextureHandle(const OString& name);
	void cachePNGTexture(texhandle_t handle);

	// maps texture handles to Texture*
	typedef OHashTable<texhandle_t, Texture*> HandleMap;
	typedef std::pair<texhandle_t, Texture*> HandleMapPair;
	HandleMap					mHandleMap;

	// lookup table to translate flatnum to mTextures index
	unsigned int				mFirstFlatLumpNum;
	unsigned int				mLastFlatLumpNum;

	// definitions for texture composition
	struct texdefpatch_t
	{
		int 		originx;
		int 		originy;
		int 		patch;
	};

	struct texdef_t
	{
		short			width;
		short			height;
		byte			scalex;
		byte			scaley;

		short			patchcount;
		texdefpatch_t	patches[1];
	};

	int*						mPNameLookup;
	std::vector<texdef_t*>		mTextureDefinitions;

	// lookup table to translate texdef_t name to indices in mTextureDefinitions
	typedef OHashTable<OString, unsigned int> TextureNameTranslationMap;
	TextureNameTranslationMap	mTextureNameTranslationMap;

	// handle management for the creation of new handles
	static const unsigned int MAX_CUSTOM_HANDLES = 1024;
	unsigned int				mFreeCustomHandlesHead;
	unsigned int				mFreeCustomHandlesTail;
	texhandle_t					mFreeCustomHandles[MAX_CUSTOM_HANDLES];

	// animated textures
	struct anim_t
	{
		static const unsigned int MAX_ANIM_FRAMES = 32;
		texhandle_t		basepic;
		short			numframes;
		byte			countdown;
		byte			curframe;
		byte 			speedmin[MAX_ANIM_FRAMES];
		byte			speedmax[MAX_ANIM_FRAMES];
		texhandle_t		framepic[MAX_ANIM_FRAMES];
	};

	std::vector<anim_t>			mAnimDefs;

	// warped textures
	struct warp_t
	{
		const Texture*	original_texture;
		Texture*		warped_texture;
	};

	std::vector<warp_t>			mWarpDefs;
};

extern TextureManager texturemanager;

#endif // __RES_TEXTURE_H__
