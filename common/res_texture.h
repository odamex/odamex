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
	static const unsigned int MAX_TEXTURE_WIDTH			= 2048;
	static const unsigned int MAX_TEXTURE_HEIGHT		= 2048;

	void init(int width, int height);

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
	
	static uint32_t calculateSize(int width, int height);

private:
	friend class TextureManager;

	Texture();

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
	int16_t			mWidth;
	int16_t			mHeight;
	uint8_t			mScaleX;
	uint8_t			mScaleY;

	struct texdefpatch_t
	{
		int 		mOriginX;
		int 		mOriginY;
		ResourceId	mResId;
	};

	int16_t			mPatchCount;
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
// CompositeTextureDefinitionParser
//
// ============================================================================

class CompositeTextureDefinitionParser
{
public:
	CompositeTextureDefinitionParser(
			const ResourceManager::RawResourceAccessor* accessor,
			const ResourceNameTranslator* translator);

	const CompositeTextureDefinition* getByName(const OString& name) const;

private:
	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
	const ResourceNameTranslator* mNameTranslator;

	typedef OHashTable<OString, CompositeTextureDefinition> TextureDefinitionTable;
	TextureDefinitionTable mTextureDefinitionLookup;

	ResourceIdList buildPNamesLookup();
	void addTexturesFromDefinitionLump(const ResourceId res_id, const ResourceIdList& pnames_lookup);

public:
	typedef TextureDefinitionTable::iterator iterator;
	typedef TextureDefinitionTable::const_iterator const_iterator;
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

class TextureLoader : public ResourceLoader
{
public:
	virtual uint32_t getTextureSize(uint16_t width, uint16_t height) const
	{
		#if CLIENT_APP
		return sizeof(Texture)						// header
			+ sizeof(uint8_t) * width * height		// mData
			+ sizeof(uint8_t) * width * height;		// mMask
		#else
		return sizeof(Texture);
		#endif
	}

	virtual Texture* initTexture(void* data, uint16_t width, uint16_t height) const
	{
		width = std::min<int>(width, Texture::MAX_TEXTURE_WIDTH);
		height = std::min<int>(height, Texture::MAX_TEXTURE_HEIGHT);

		Texture* texture = static_cast<Texture*>(data);
		texture->init(width, height);
		return texture;
	}
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

	virtual uint32_t size() const;
	virtual void load(void* data) const;

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
	FlatTextureLoader(const ResourceManager::RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~FlatTextureLoader() {}

	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	int16_t getWidth() const;
	int16_t getHeight() const;

	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
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
	PatchTextureLoader(const ResourceManager::RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~PatchTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
	const ResourceId	mResId;

	bool validateHelper(const uint8_t* raw_data, uint32_t raw_size) const;
};


// ----------------------------------------------------------------------------
// SpriteTextureLoader class interface
//
// Loads patch_t format sprite graphic lumps. 
// ----------------------------------------------------------------------------

class SpriteTextureLoader : public PatchTextureLoader
{
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
	CompositeTextureLoader(const ResourceManager::RawResourceAccessor* accessor, const CompositeTextureDefinition& texture_def);
	virtual ~CompositeTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
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
	RawTextureLoader(const ResourceManager::RawResourceAccessor* accessor, const ResourceId res_id); 
	virtual ~RawTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
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
	PngTextureLoader(const ResourceManager::RawResourceAccessor* accessor, const ResourceId res_id, const OString& name); 
	virtual ~PngTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
	const ResourceId	mResId;
	const OString&		mResourceName;
};




// ============================================================================
//
// TextureLoaderFactory
//
// Instantiates a TextureLoader instance given a ResourceId and ResourcePath
//
// ============================================================================

class TextureLoaderFactory
{
public:
	TextureLoaderFactory(
			const ResourceManager::RawResourceAccessor* accessor,
			const ResourceNameTranslator* translator,
			const CompositeTextureDefinitionParser* composite_texture_defintions);

	TextureLoader* createTextureLoader(const ResourcePath& res_path, const ResourceId res_id) const
	{
		const OString& directory = res_path.first();

		// Handle omitted wall textures
		// const OString& name = res_path.last();
		// if (directory == textures_directory_name && name.size() > 0 && name.c_str()[0] == '-')
			// return new InvalidTextureLoader();

		if (directory == flats_directory_name)
		{
			return new FlatTextureLoader(mRawResourceAccessor, res_id);
		}
		else if (directory == textures_directory_name)
		{
			return new CompositeTextureLoader(mRawResourceAccessor, CompositeTextureDefinition());
		}
		else if (directory == patches_directory_name)
		{
			return new PatchTextureLoader(mRawResourceAccessor, res_id);
		}

		/*
		// check for the texture in the default location specified by type
		if (type == Texture::TEX_FLAT)
			tex_id = getFlatTextureId(uname);
		else if (type == Texture::TEX_WALLTEXTURE)
			tex_id = getWallTextureTextureId(uname);
		else if (type == Texture::TEX_PATCH)
			tex_id = getPatchTextureId(uname);
		else if (type == Texture::TEX_SPRITE)
			tex_id = getSpriteTextureId(uname);
		else if (type == Texture::TEX_RAW)
			tex_id = getRawTextureTextureId(uname);
		else if (type == Texture::TEX_PNG)
			tex_id = getPNGTextureTextureId(uname);

		// not found? check elsewhere
		if (tex_id == NOT_FOUND_TEXTURE_ID && type != Texture::TEX_FLAT)
			tex_id = getFlatTextureId(uname);
		if (tex_id == NOT_FOUND_TEXTURE_ID && type != Texture::TEX_WALLTEXTURE)
			tex_id = getWallTextureTextureId(uname);
		*/

		return NULL;
	}

private:
	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
	const ResourceNameTranslator* mNameTranslator;
	const CompositeTextureDefinitionParser* mCompositeTextureDefinitions;
};







class CompositeTextureContainer : public ResourceContainer
{
public:
	virtual ~CompositeTextureContainer() {}

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual uint32_t getResourceCount() const
	{
		return mTextureDefinitionLookup.size();
	}

	virtual uint32_t getResourceSize(const ResourceId res_id) const
	{
		return sizeof(CompositeTextureDefinition);
	}

	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const
	{
		const CompositeTextureDefinition* texture_def = getByResourceId(res_id);
		if (texture_def)
		{
			size = std::min<uint32_t>(size, sizeof(*texture_def));
			memcpy(data, (void*)texture_def, size);
			return size;
		}
		return 0;
	}

private:
	ResourceContainerId			mResourceContainerId;

	const ResourceManager::RawResourceAccessor* mRawResourceAccessor;
	const ResourceNameTranslator* mNameTranslator;

	typedef OHashTable<OString, CompositeTextureDefinition> TextureDefinitionTable;
	TextureDefinitionTable mTextureDefinitionLookup;

	typedef OHashTable<ResourceId, OString> TextureNameTable;
	TextureNameTable mTextureNameLookup;

	ResourceIdList buildPNamesLookup();
	void addTexturesFromDefinitionLump(const ResourceId res_id, const ResourceIdList& pnames_lookup);

	const CompositeTextureDefinition* getByName(const OString& name) const
	{
		TextureDefinitionTable::const_iterator it = mTextureDefinitionLookup.find(name);
		if (it != mTextureDefinitionLookup.end())
			return &(it->second);
		return NULL;
	}

	const CompositeTextureDefinition* getByResourceId(const ResourceId res_id) const
	{
		TextureNameTable::const_iterator it = mTextureNameLookup.find(res_id);
		if (it != mTextureNameLookup.end())
			return getByName(it->second);
		return NULL;
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

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual uint32_t getResourceCount() const
	{
		return 0; 
	}

	virtual uint32_t getResourceSize(const ResourceId res_id) const
	{
		return 0;
	}

	virtual uint32_t loadResource(const ResourceId res_id, void* data, uint32_t length) const
	{
		return 0;
	}

	void updateAnimatedTextures();

	const Texture* getTexture(const ResourceId res_id);

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
