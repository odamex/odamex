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
#include "z_zone.h"

#include <vector>

#include "m_ostring.h"
#include "hashtable.h"

#include "r_defs.h"
#include "resources/res_main.h"

class Texture;
class TextureManager;
class ResourceLoader;
class ResourceManager;
class RawResourceAccessor;

void Res_CopySubimage(Texture* dest_texture, const Texture* source_texture,
	int dx1, int dy1, int dx2, int dy2,
	int sx1, int sy1, int sx2, int sy2);

void Res_TransposeImage(byte* dest, const byte* source, int width, int height);


enum TextureSearchOrdering {
	WALL,
	FLOOR,
	SPRITE,
	PATCH,
};

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
	static const unsigned int MAX_TEXTURE_WIDTH		= 2048;
	static const unsigned int MAX_TEXTURE_HEIGHT	= 2048;

	void init(int width, int height);

	byte* getData() const
	{	return mData;	}

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

	fixed_t getScaledHeight() const
	{	return FixedMul(mHeight << FRACBITS, mScaleY);	}
	
	fixed_t getScaledWidth() const
	{	return FixedMul(mWidth << FRACBITS, mScaleX);	}

	static uint32_t calculateSize(int width, int height);

	const palindex_t* getColumn(int col) const
	{	
		return mData + mHeight * col;
	}

public:
	Texture();

	static uint32_t calculateHeaderSize(int width, int height)
	{	return sizeof(Texture);	}

	static uint32_t calculateDataSize(int width, int height)
	{	return sizeof(uint8_t) * width * height;	}

	fixed_t				mScaleX;
	fixed_t				mScaleY;

	unsigned short		mWidth;
	unsigned short		mHeight;
	
	short				mOffsetX;
	short				mOffsetY;

	byte				mWidthBits;
	byte				mHeightBits;

	byte				mMasked;
	byte				mMaskColor;


	palindex_t*			mData;
};



// ============================================================================
//
// AnimatedTexture
//
// ============================================================================
//
class AnimatedTexture : public Texture
{
public:
	byte* getData() const
	{	return mFrameData[mCurrentFrame];	}

	byte* getMaskData() const
	{	return mFrameMask[mCurrentFrame];	}

private:
	uint16_t			mCurrentFrame;
	byte**				mFrameData;
	byte**				mFrameMask;
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
	int16_t			mWidth;
	int16_t			mHeight;
	uint8_t			mScaleX;
	uint8_t			mScaleY;

	struct PatchDef
	{
		int 		mOriginX;
		int 		mOriginY;
		ResourceId	mResId;
	};

	typedef std::vector<PatchDef> PatchDefList;
	PatchDefList mPatchDefs;

	CompositeTextureDefinition() :
		mWidth(0), mHeight(0), mScaleX(0), mScaleY(0)
	{ }

	~CompositeTextureDefinition()
	{ }

	CompositeTextureDefinition(const CompositeTextureDefinition& other) 
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

			mPatchDefs = other.mPatchDefs;
		}
		return *this;
	}
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

	virtual uint32_t getResourceCount() const
	{
		return mResourceLoaderLookup.size();
	}

	virtual uint32_t getResourceSize(const ResourceId res_id) const;
	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

	void updateAnimatedTextures();


private:
	// initialization routines
	void clear();
	void readAnimDefLump();
	void readAnimatedLump();

	void analyzePalette(palindex_t* colormap, palindex_t* maskcolor) const;


	void addCompositeTextureResources(ResourceManager* manager);

	void addResourcesToManager(ResourceManager* manager);
	void addResourceToManagerByDir(ResourceManager* manager, const ResourcePath& dir);
	const ResourceId getRawResourceId(const ResourceId res_id) const;

	const ResourceIdList buildPNamesLookup(const OString& lump_name) const;
	CompositeTextureDefinition buildCompositeTextureDefinition(const uint8_t* data, const ResourceIdList& pnames_lookup) const;



	typedef OHashTable<ResourceId, ResourceLoader*> ResourceLoaderLookupTable;
	ResourceLoaderLookupTable		mResourceLoaderLookup;

	const ResourceLoader* getResourceLoader(const ResourceId res_id) const;


	typedef std::vector<const Texture*> TextureList;
	TextureList				mTextures;

	ResourceManager*				mResourceManager;

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
		ResourceId		framepic[MAX_ANIM_FRAMES];
	};

	std::vector<anim_t>			mAnimDefs;

	// warped textures
	struct warp_t
	{
		const Texture*	original_texture;
		Texture*		warped_texture;
	};

	std::vector<warp_t>			mWarpDefs;

	mutable palindex_t			mMaskColor;
	mutable palindex_t*			mMaskColorMap;
};




const ResourceId Res_GetTextureResourceId(const OString& name, TextureSearchOrdering ordering);

//
// Res_CachePatch
//
// Place-holder function for resolving a patch name to a resource ID and
// caching and returning that resource's data.
//
const patch_t* Res_CachePatch(const OString& name, int tag = PU_CACHE);


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(ResourceId res_id, int tag = PU_CACHE);
const Texture* Res_CacheTexture(const OString& lump_name, const ResourcePath& directory, int tag = PU_CACHE);


#endif // __RES_TEXTURE_H__
