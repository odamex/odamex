// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: res_texture.h 1856 2010-09-05 03:14:13Z ladna $
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

#include "res_main.h"

typedef unsigned int TextureId;

class Texture;
class TextureManager;
class ResourceManager;

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

	TextureId getTextureId() const
	{	return mTextureId;	}

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
	

	// 
	// Texture::createTexture
	//
	// Factory for creating Texture instances. Allocates memory for a new
	// Texture, initializes it and then returns a pointer to it.
	// 
	static Texture* createTexture(int width, int height) 
	{ 
		width = std::min<int>(width, Texture::MAX_TEXTURE_WIDTH); 
		height = std::min<int>(height, Texture::MAX_TEXTURE_HEIGHT); 
		 
		// server shouldn't allocate memory for texture data, only the header    
		uint32_t texture_size = clientside ? 
				Texture::calculateSize(width, height) : sizeof(Texture); 
							      
		Texture* texture = (Texture*)Z_Malloc(texture_size, PU_STATIC, NULL); 
		texture->init(width, height); 
														 
		return texture; 
	}

	static uint32_t calculateSize(int width, int height);


private:
	friend class TextureManager;

	Texture();

	void init(int width, int height);

	TextureId			mTextureId;

	fixed_t				mScaleX;
	fixed_t				mScaleY;

	unsigned short		mWidth;
	unsigned short		mHeight;
	
	short				mOffsetX;
	short				mOffsetY;

	byte				mWidthBits;
	byte				mHeightBits;

	bool				mHasMask;

// TODO: make these private
public:
	byte*				mMask;
	byte*				mData;
};


// ============================================================================
//
// CompositeTextureDefinition
//
// ============================================================================
//
// A structure to contain the data for a composite texture definition, which
// are defined in the TEXTURE1 and TEXTURE2 lumps.
//

struct CompositeTextureDefinition
{
	short			mWidth;
	short			mHeight;
	byte			mScaleX;
	byte			mScaleY;

	struct texdefpatch_t
	{
		int 		mOriginX;
		int 		mOriginY;
		ResourceId	mResId;
	};

	short			mPatchCount;
	texdefpatch_t*	mPatches;

	CompositeTextureDefinition() :
		mWidth(0), mHeight(0), mScaleX(0), mScaleY(0),
		mPatchCount(0), mPatches(NULL)
	{ }

	~CompositeTextureDefinition()
	{
		delete [] mPatches;
	}

	CompositeTextureDefinition(const CompositeTextureDefinition& other) :
		mPatches(NULL)
	{
		operator=(other);
	}

	CompositeTextureDefinition& operator=(const CompositeTextureDefinition& other)
	{
		if (this != &other)
		{
			mWidth = other.mWidth;
			mHeight = other.mHeight;
			mScaleX = other.mScaleX;
			mScaleY = other.mScaleY;

			delete [] mPatches;
			mPatchCount = other.mPatchCount;
			mPatches = new texdefpatch_t[mPatchCount];
			for (int i = 0; i < mPatchCount; i++)
			{
				mPatches[i].mOriginX = other.mPatches[i].mOriginX;
				mPatches[i].mOriginY = other.mPatches[i].mOriginY;
				mPatches[i].mResId = other.mPatches[i].mResId;
			}
		}
		return *this;
	}

};


// ============================================================================
//
// TextureLoader
//
// ============================================================================
//
// The TextureLoader classes each load a type of texture resource from the
// resource files.
//

// ----------------------------------------------------------------------------
// TextureLoader abstract base class interface
// ----------------------------------------------------------------------------

class TextureLoader
{
public:
	virtual ~TextureLoader() {}

	virtual bool validate() const = 0;
	virtual uint32_t size() const = 0;
	virtual const Texture* load() const = 0;
};


// ---------------------------------------------------------------------------
// InvalidTextureLoader class interface
//
// Creates a Texture instance that can be used when a resource is unavailible
// or is invalid
// ---------------------------------------------------------------------------

class InvalidTextureLoader : public TextureLoader
{
public:
	InvalidTextureLoader();
	virtual ~InvalidTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual const Texture* load() const;

private:
	static const int16_t WIDTH = 64;
	static const int16_t HEIGHT = 64;
};


// ----------------------------------------------------------------------------
// FlatTextureLoader class interface
//
// Loads 64x64, 128x128 or 256x256 raw graphic lumps, converting them from
// row-major to column-major format.
// ----------------------------------------------------------------------------

class FlatTextureLoader : public TextureLoader
{
public:
	FlatTextureLoader(ResourceManager* manager, const ResourceId res_id);
	virtual ~FlatTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual const Texture* load() const;

private:
	int16_t getWidth() const;
	int16_t getHeight() const;

	ResourceManager*	mResourceManager;
	const ResourceId	mResId;
};


// ----------------------------------------------------------------------------
// PatchTextureLoader class interface
//
// Loads patch_t format graphic lumps. 
// ----------------------------------------------------------------------------

class PatchTextureLoader : public TextureLoader
{
public:
	PatchTextureLoader(ResourceManager* manager, const ResourceId res_id);
	virtual ~PatchTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual const Texture* load() const;

private:
	ResourceManager*	mResourceManager;
	const ResourceId	mResId;
};


// ----------------------------------------------------------------------------
// SpriteTextureLoader class interface
//
// Loads patch_t format sprite graphic lumps. 
// ----------------------------------------------------------------------------

class SpriteTextureLoader : public PatchTextureLoader
{
public:
	SpriteTextureLoader(ResourceManager* manager, const ResourceId res_id) :
		PatchTextureLoader(manager, res_id)
	{}

	virtual ~SpriteTextureLoader() {}
};


// ----------------------------------------------------------------------------
// CompositeTextureLoader class interface
//
// Generates composite textures given a CompositeTextureDefinition from the
// TEXTURE1 or TEXTURE2 lumps. The texture is composed from one or more
// patch textures.
// ----------------------------------------------------------------------------

class CompositeTextureLoader : public TextureLoader
{
public:
	CompositeTextureLoader(ResourceManager* manager, const CompositeTextureDefinition& texture_def);
	virtual ~CompositeTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual const Texture* load() const;

private:
	ResourceManager*					mResourceManager;
	const CompositeTextureDefinition	mTextureDef;
};


// ----------------------------------------------------------------------------
// RawTextureLoader class interface
//
// Loads raw 320x200 graphic lumps. 
// ----------------------------------------------------------------------------

class RawTextureLoader : public TextureLoader
{
public:
	RawTextureLoader(ResourceManager* manager, const ResourceId res_id); 
	virtual ~RawTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual const Texture* load() const;

private:
	ResourceManager*	mResourceManager;
	const ResourceId	mResId;
};


// ----------------------------------------------------------------------------
// PngTextureLoader class interface
//
// Loads PNG format graphic lumps. 
// ----------------------------------------------------------------------------

class PngTextureLoader : public TextureLoader
{
public:
	PngTextureLoader(ResourceManager* manager, const ResourceId res_id); 
	virtual ~PngTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual const Texture* load() const;

private:
	ResourceManager*	mResourceManager;
	const ResourceId	mResId;
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
// The TextureManager will create a TextureLoader instance for each graphic
// lump in the loaded resource files as well as any composite graphics defined
// in TEXTURE1 and TEXTURE2. The mLumpId field of a ResourceId identifies the
// index into the mTextures and mTextureLoaders arrays for the cached texture
// and its TextureLoader instance.
//
//
// TODO: properly load Heretic skies where the texture definition reports
// the sky texture is 128 pixels high but uses a 200 pixel high patch.
// The texture height should be adjusted to the height of the tallest
// patch in the texture.
//
class TextureManager : public ResourceContainer
{
public:
	TextureManager(const ResourceContainerId& container_id, ResourceManager* manager);
	~TextureManager();

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual uint32_t getResourceCount() const
	{
		return mTextureLoaders.size();
	}

	virtual uint32_t getResourceSize(const LumpId lump_id) const
	{
		return 0;
	}

	virtual uint32_t loadResource(const LumpId lump_id, void* data, uint32_t length) const
	{
		return 0;
	}

	void updateAnimatedTextures();

	TextureId getTextureId(const char* name, Texture::TextureSourceType type);
	TextureId getTextureId(const OString& name, Texture::TextureSourceType type);
	const Texture* getTexture(const ResourceId res_id);

	Texture* createTexture(const TextureId tex_id, int width, int height);
	void freeTexture(const LumpId lump_id);

	TextureId createCustomTextureId();
	void freeCustomTextureId(const TextureId tex_id);

	static const TextureId NO_TEXTURE_ID			= 0x0;
	static const TextureId NOT_FOUND_TEXTURE_ID		= 0x1;

private:
	static const unsigned int FLAT_TEXTURE_ID_MASK		= 0x00010000ul;
	static const unsigned int PATCH_TEXTURE_ID_MASK		= 0x00020000ul;
	static const unsigned int SPRITE_TEXTURE_ID_MASK	= 0x00040000ul;
	static const unsigned int WALLTEXTURE_ID_MASK		= 0x00080000ul;
	static const unsigned int RAW_TEXTURE_ID_MASK		= 0x000A0000ul;
	static const unsigned int PNG_TEXTURE_ID_MASK		= 0x00100000ul;
	static const unsigned int CUSTOM_TEXTURE_ID_MASK	= 0x01000000ul;

	// initialization routines
	void clear();
	void addTextureDirectories(ResourceManager* manager); 
	void readAnimDefLump();
	void readAnimatedLump();

	void registerTextureResources(ResourceManager* manager);

	const ResourceContainerId		mResourceContainerId;

	typedef std::vector<TextureLoader*> TextureLoaderList;
	TextureLoaderList		mTextureLoaders;

	typedef std::vector<const Texture*> TextureList;
	TextureList				mTextures;

	// management for the creation of new TextureIds
	static const unsigned int MAX_CUSTOM_TEXTURE_IDS = 1024;
	unsigned int				mFreeCustomTextureIdsHead;
	unsigned int				mFreeCustomTextureIdsTail;
	TextureId					mFreeCustomTextureIds[MAX_CUSTOM_TEXTURE_IDS];

	// animated textures
	struct anim_t
	{
		static const unsigned int MAX_ANIM_FRAMES = 32;
		ResourceId		basepic;
		short			numframes;
		byte			countdown;
		byte			curframe;
		byte 			speedmin[MAX_ANIM_FRAMES];
		byte			speedmax[MAX_ANIM_FRAMES];
		TextureId		framepic[MAX_ANIM_FRAMES];
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

#endif // __RES_TEXTURE_H__
