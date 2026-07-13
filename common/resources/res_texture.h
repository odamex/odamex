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

#pragma once

#include <vector>

#include "doomtype.h"
#include "m_fixed.h"
#include "z_zone.h"

#include "oscanner.h"
#include "olumpname.h"

#include "resources/res_container.h"

class ResourceLoader;
class ResourceManager;
class RawResourceAccessor;
class Texture;

enum TextureSearchOrdering {
	ANY,
	WALL,
	FLOOR,
	SPRITE,
	PATCH,
	GRAPHICS,
};


//
// Res_GetTextureResourceId
//
const ResourceId Res_GetTextureResourceId(const OString& name, TextureSearchOrdering ordering, bool use_placeholder = true);

inline const ResourceId Res_GetTextureResourceId(const char* name, TextureSearchOrdering ordering, bool use_placeholder = true)
{
	return Res_GetTextureResourceId(OStringToUpper(name), ordering, use_placeholder);
}

inline const ResourceId Res_GetTextureResourceId(const OLumpName& name, TextureSearchOrdering ordering, bool use_placeholder = true)
{
	return Res_GetTextureResourceId(OStringToUpper(name.c_str()), ordering, use_placeholder);
}


//
// Texture Animations
//
void Res_ReadAnimationDefinitions();
void Res_UpdateTextureAnimations();
const ResourceId Res_GetAnimatedTextureResourceId(const ResourceId res_id);


//
// Res_IsTextureResource
//
// Returns true if the ResourceId refers to a resource registered by the
// TextureManager and can therefore be safely cached as a Texture.
//
bool Res_IsTextureResource(const ResourceId res_id);


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(ResourceId res_id, zoneTag_e tag = PU_CACHE);
const Texture* Res_CacheTexture(const OString& lump_name, TextureSearchOrdering ordering, zoneTag_e tag = PU_CACHE);

inline const Texture* Res_CacheTexture(const char* lump_name, TextureSearchOrdering ordering, zoneTag_e tag = PU_CACHE)
{
	return Res_CacheTexture(OString(lump_name), ordering, tag);
}

inline const Texture* Res_CacheTexture(const OLumpName& lump_name, TextureSearchOrdering ordering, zoneTag_e tag = PU_CACHE)
{
	return Res_CacheTexture(OString(lump_name.c_str()), ordering, tag);
}


// ============================================================================
//
// Transitional patch-handle API
//
// ============================================================================
//
// Maps the old lump/patch caching interface onto the resource manager so
// UI code written against W_CachePatchHandle/W_ResolvePatchHandle works
// with ResourceIds and cached Textures. A ResourceId remains valid across
// resource file reloads the same way the generation-tagged lump handles did:
// resolving simply produces the current texture (or the not-found
// placeholder).
//
using lumpHandle_t = ResourceId;

inline lumpHandle_t W_CachePatchHandle(const char* name, zoneTag_e tag = PU_CACHE,
                                       TextureSearchOrdering ordering = PATCH)
{
	return Res_GetTextureResourceId(OStringToUpper(name), ordering);
}

inline lumpHandle_t W_CachePatchHandle(const OString& name, zoneTag_e tag = PU_CACHE,
                                       TextureSearchOrdering ordering = PATCH)
{
	return W_CachePatchHandle(name.c_str(), tag, ordering);
}

inline lumpHandle_t W_CachePatchHandle(const OLumpName& name, zoneTag_e tag = PU_CACHE,
                                       TextureSearchOrdering ordering = PATCH)
{
	return W_CachePatchHandle(name.c_str(), tag, ordering);
}

inline const Texture* W_ResolvePatchHandle(const lumpHandle_t handle)
{
	return Res_CacheTexture(handle);
}

// Looks like you already resolved the texture.
inline const Texture* W_ResolvePatchHandle(const Texture* texture)
{
	return texture;
}

inline const Texture* W_CachePatch(const char* name, zoneTag_e tag = PU_CACHE)
{
	return Res_CacheTexture(OStringToUpper(name), PATCH, tag);
}

inline const Texture* W_CachePatch(const OString& name, zoneTag_e tag = PU_CACHE)
{
	return W_CachePatch(name.c_str(), tag);
}

inline const Texture* W_CachePatch(const OLumpName& name, zoneTag_e tag = PU_CACHE)
{
	return W_CachePatch(name.c_str(), tag);
}

inline const Texture* W_CachePatch(const lumpHandle_t handle, zoneTag_e tag = PU_CACHE)
{
	return Res_CacheTexture(handle, tag);
}


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
	static constexpr unsigned int MAX_TEXTURE_WIDTH		= 2048;
	static constexpr unsigned int MAX_TEXTURE_HEIGHT	= 2048;

	Texture()
	{
		init(0, 0);
	}

	void init(int width, int height);
	static uint32_t calculateSize(int width, int height);

	[[nodiscard]] fixed_t getScaledHeight() const
	{
		return FixedMul(mHeight << FRACBITS, mScaleY);
	}

	[[nodiscard]] fixed_t getScaledWidth() const
	{
		return FixedMul(mWidth << FRACBITS, mScaleX);
	}

	// Accessors mirroring the old patch_t interface, so code written
	// against patches can work with cached textures unmodified.
	[[nodiscard]] short width() const { return mWidth; }
	[[nodiscard]] short height() const { return mHeight; }
	[[nodiscard]] short leftoffset() const { return mOffsetX; }
	[[nodiscard]] short topoffset() const { return mOffsetY; }

	[[nodiscard]] const palindex_t* getColumn(int col) const
	{
		return mData + mHeight * col;
	}

	// Wraps a column index into the texture's width. mWidthMask only tiles
	// correctly for power-of-two widths; non-power-of-two textures need a
	// true modulo, matching ZDoom.
	[[nodiscard]] int wrapColumn(int col) const
	{
		if (mWidthMask + 1 == mWidth)
			return col & mWidthMask;
		col %= mWidth;
		return col < 0 ? col + mWidth : col;
	}

	// True color ARGB color plane.
	[[nodiscard]] const argb_t* getARGBColumn(int col) const
	{
		return mARGBData ? mARGBData + mHeight * col : nullptr;
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
	argb_t*				mARGBData;

private:
	static constexpr uint32_t calculateHeaderSize(int width, int height)
	{
		return sizeof(Texture);
	}

	static constexpr uint32_t calculateDataSize(int width, int height)
	{
		return sizeof(palindex_t) * width * height;
	}
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
	int16_t			mWidth = 0;
	int16_t			mHeight = 0;
	uint8_t			mScaleX = 0;
	uint8_t			mScaleY = 0;

	struct PatchDef
	{
		int 		mOriginX = 0;
		int 		mOriginY = 0;
		ResourceId	mResId;
	};

	using PatchDefList = std::vector<PatchDef>;
	PatchDefList mPatchDefs;
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
	TextureManager(ResourceManager* manager);

	~TextureManager();

	virtual uint32_t getResourceCount() const
	{
		return mResourceLoaderLookup.size();
	}

	void addResources(ResourceManager* manager);

	virtual uint32_t getResourceSize(const ResourceId res_id) const;
	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

	// Returns true if the given ResourceId is one of the texture resources
	// registered by this TextureManager (as opposed to a raw file/lump
	// resource that no TextureLoader claimed).
	bool ownsResource(const ResourceId res_id) const
	{
		return mResourceLoaderLookup.find(res_id) != mResourceLoaderLookup.end();
	}

	void updateAnimatedTextures();


private:
	// initialization routines
	void clear();

	const ResourceLoader* getResourceLoader(const ResourceId res_id) const;

	void addResourceToManagerByDir(ResourceManager* manager, const ResourcePath& dir);

	const ResourceIdList buildPNamesLookup(ResourceManager* manager, const OString& lump_name) const;
	void addCompositeTextureResources(ResourceManager* manager, const ResourceIdList& pnames_lookup, const OString& lump_name);
	CompositeTextureDefinition buildCompositeTextureDefinition(const uint8_t* data, const ResourceIdList& pnames_lookup) const;
	ResourceId addMissingTexturePlaceholder(ResourceManager* manager);

	using ResourceLoaderLookupTable = OHashTable<ResourceId, ResourceLoader*>;
	ResourceLoaderLookupTable		mResourceLoaderLookup;
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
	AnimatedTextureManager() = default;

	~AnimatedTextureManager()
	{
		clear();
	}

	AnimatedTextureManager(const AnimatedTextureManager&) = delete;
	AnimatedTextureManager& operator=(const AnimatedTextureManager&) = delete;

	void readAnimationDefinitions();

	void updateAnimatedTextures();

	const ResourceId getResourceId(const ResourceId res_id) const;

private:
	void clear();
	void loadAnimationsFromAnimatedLump();
	void loadAnimationsFromAnimDefLump();

	void addWarpedTexture(const ResourceId res_id);
	void copyTexture(Texture* destination_texture, const Texture* source_texture) const;
	void parseAnim(OScanner& os, TextureSearchOrdering search_ordering);

	using ResourceIdMap = OHashTable<ResourceId, ResourceId>;
	ResourceIdMap		mTextureTranslation;

	// animated textures
	struct anim_t
	{
		static constexpr unsigned int MAX_ANIM_FRAMES = 32;
		ResourceId basepic;
		short numframes;
		bool uniqueframes;
		byte countdown;
		byte curframe;
		byte speedmin[MAX_ANIM_FRAMES];
		byte speedmax[MAX_ANIM_FRAMES];
		ResourceId framepic[MAX_ANIM_FRAMES];
	};

	std::vector<AnimatedTextureManager::anim_t> mAnimDefs;

	struct warp_t
	{
		Texture* original_texture;
		Texture* working_texture;
	};

	std::vector<AnimatedTextureManager::warp_t> mWarpedTextures;
};
