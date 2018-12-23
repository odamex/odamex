// Loads a resource from a ResourceContainer, performing any necessary data
// conversion and caching the resulting instance in the Zone memory system.
//


#ifndef __RES_RESOURCELOADER_H__
#define __RES_RESOURCELOADER_H__

#include "doomtype.h"
#include "m_ostring.h"
#include "resources/res_resourceid.h"

class ResourcePath;
class Texture;
class CompositeTextureDefinition;
class CompositeTextureDefinitionParser;
class ResourceManager;
class ResourceNameTranslator;

//
// RawResourceAccessor class
//
// Provides access to the resources without post-processing. Instances of
// this class are typically used by the ResourceLoader hierarchy to read the
// raw resource data and then perform their own post-processign.
//
class RawResourceAccessor
{
public:
	RawResourceAccessor(const ResourceManager* manager) :
		mResourceManager(manager)
	{ }

	uint32_t getResourceSize(const ResourceId res_id) const;

	void loadResource(const ResourceId res_id, void* data, uint32_t size) const;

private:
	const ResourceManager*		mResourceManager;
};


// ============================================================================
//
// ResourceLoader class interface
//
// ============================================================================
//

// ----------------------------------------------------------------------------
// ResourceLoader abstract base class interface
// ----------------------------------------------------------------------------

class ResourceLoader
{
public:
	virtual ~ResourceLoader() {}

	virtual bool validate() const
	{	return true;	}

	virtual uint32_t size() const = 0;
	virtual void load(void* data) const = 0;
};


// ---------------------------------------------------------------------------
// DefaultResourceLoader class interface
//
// Generic resource loading functionality. Simply reads raw data and returns
// a pointer to the cached data.
// ---------------------------------------------------------------------------

class DefaultResourceLoader : public ResourceLoader
{
public:
	DefaultResourceLoader(const RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~DefaultResourceLoader() { }

	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const RawResourceAccessor*	mRawResourceAccessor;
	const ResourceId			mResourceId;
};


// ----------------------------------------------------------------------------
// PatchResourceLoader class interface
//
// Loads patch_t format graphic lumps.
// ----------------------------------------------------------------------------

class PatchResourceLoader : public ResourceLoader
{
public:
	PatchResourceLoader(const RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~PatchResourceLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
	const ResourceId	mResId;

	bool validateHelper(const uint8_t* raw_data, uint32_t raw_size) const;
};


// ----------------------------------------------------------------------------
// SpriteResourceLoader class interface
//
// Loads patch_t format sprite graphic lumps.
// ----------------------------------------------------------------------------

class SpriteResourceLoader : public PatchResourceLoader
{
public:
	SpriteResourceLoader(const RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~SpriteResourceLoader() {}
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
	virtual uint32_t getTextureSize(uint16_t width, uint16_t height) const;
	virtual Texture* initTexture(void* data, uint16_t width, uint16_t height) const;
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
	FlatTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~FlatTextureLoader() {}

	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	int16_t getWidth() const;
	int16_t getHeight() const;

	const RawResourceAccessor* mRawResourceAccessor;
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
	PatchTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~PatchTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
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
	CompositeTextureLoader(
			const RawResourceAccessor* accessor,
			const CompositeTextureDefinition* texture_def);
	virtual ~CompositeTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
	const CompositeTextureDefinition* mTextureDef;
};


// ----------------------------------------------------------------------------
// RawTextureLoader class interface
//
// Loads raw 320x200 graphic lumps.
// ----------------------------------------------------------------------------

class RawTextureLoader : public TextureLoader
{
public:
	RawTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~RawTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
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
	PngTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id, const OString& name);
	virtual ~PngTextureLoader() {}

	virtual bool validate() const;
	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
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
			const RawResourceAccessor* accessor,
			const ResourceNameTranslator* translator,
			const CompositeTextureDefinitionParser* composite_texture_defintions);

	TextureLoader* createTextureLoader(
			const ResourcePath& res_path,
			const ResourceId res_id) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
	const ResourceNameTranslator* mNameTranslator;
	const CompositeTextureDefinitionParser* mCompositeTextureDefinitions;
};


#endif	// __RES_RESOURCELOADER_H__
