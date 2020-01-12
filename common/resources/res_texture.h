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

class ResourceLoader;
class ResourceManager;
class RawResourceAccessor;
class Texture;

enum TextureSearchOrdering {
	WALL,
	FLOOR,
	SPRITE,
	PATCH,
};


//
// Res_GetTextureResourceId
//
const ResourceId Res_GetTextureResourceId(const OString& name, TextureSearchOrdering ordering);


//
// Texture Animations
//
void Res_ReadAnimationDefinitions();
void Res_UpdateTextureAnimations();
const ResourceId Res_GetAnimatedTextureResourceId(const ResourceId res_id);


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(ResourceId res_id, int tag = PU_CACHE);
const Texture* Res_CacheTexture(const OString& lump_name, TextureSearchOrdering ordering, int tag = PU_CACHE);
const Texture* Res_CacheTexture(const OString& lump_name, const ResourcePath& directory, int tag = PU_CACHE);


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

	Texture()
	{
		init(0, 0);
	}

	void init(int width, int height);
	static uint32_t calculateSize(int width, int height);

	fixed_t getScaledHeight() const
	{
		return FixedMul(mHeight << FRACBITS, mScaleY);
	}
	
	fixed_t getScaledWidth() const
	{
		return FixedMul(mWidth << FRACBITS, mScaleX);
	}

	const palindex_t* getColumn(int col) const
	{	
		return mData + mHeight * col;
	}

	fixed_t				mScaleX;
	fixed_t				mScaleY;

	unsigned short		mWidth;
	unsigned short		mHeight;
	
	short				mOffsetX;
	short				mOffsetY;

	byte				mWidthBits;
	byte				mHeightBits;

	unsigned short		mWidthMask;
	unsigned short		mHeightMask;

	byte				mMaskColor;

	palindex_t*			mData;

private:
	static uint32_t calculateHeaderSize(int width, int height)
	{
		return sizeof(Texture);
	}

	static uint32_t calculateDataSize(int width, int height)
	{
		return sizeof(palindex_t) * width * height;
	}
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

	void addResourcesToManager(ResourceManager* manager);
	void addResourceToManagerByDir(ResourceManager* manager, const ResourcePath& dir);

	const ResourceIdList buildPNamesLookup(ResourceManager* manager, const OString& lump_name) const;
	void addCompositeTextureResources(ResourceManager* manager, const ResourceIdList& pnames_lookup, const OString& lump_name);
	CompositeTextureDefinition buildCompositeTextureDefinition(const uint8_t* data, const ResourceIdList& pnames_lookup) const;

	typedef OHashTable<ResourceId, ResourceLoader*> ResourceLoaderLookupTable;
	ResourceLoaderLookupTable		mResourceLoaderLookup;

	const ResourceLoader* getResourceLoader(const ResourceId res_id) const;

};



//
// AnimatedTextureManager
//
// Handles animation frames for animated flat and wall textures, translating
// between ResourceIds based on the current animation frame.
//
class AnimatedTextureManager
{
public:
	AnimatedTextureManager();
	
	~AnimatedTextureManager() { }

	void readAnimationDefinitions();

	void updateAnimatedTextures();

	const ResourceId getResourceId(const ResourceId res_id) const;

private:

	void loadAnimationsFromAnimatedLump();
	void loadAnimationsFromAnimDefLump();

	typedef OHashTable<ResourceId, ResourceId> ResourceIdMap;
	ResourceIdMap		mTextureTranslation;


	// animated textures
	struct anim_t
	{
		static const unsigned int MAX_ANIM_FRAMES = 32;
		ResourceId		basepic;
		short			numframes;
		byte			istexture;		// TODO: Remove this
		byte			uniqueframes;
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
};

#endif // __RES_TEXTURE_H__