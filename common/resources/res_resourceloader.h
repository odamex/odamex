// Loads a resource from a ResourceContainer, performing any necessary data
// conversion and caching the resulting instance in the Zone memory system.
//


#ifndef __RES_RESOURCELOADER_H__
#define __RES_RESOURCELOADER_H__

#include "doomtype.h"
#include "m_ostring.h"
#include "resources/res_resourceid.h"
#include "resources/res_texture.h"

class ResourcePath;
class RawResourceAccessor;

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
	ResourceLoader(const RawResourceAccessor* accessor) :
		mRawResourceAccessor(accessor)
	{}

	virtual ~ResourceLoader() {}

	virtual bool validate() const
	{
		return true;
	}

	virtual uint32_t size() const = 0;
	virtual void load(void* data) const = 0;

protected:
	const RawResourceAccessor*		mRawResourceAccessor;
};


// ---------------------------------------------------------------------------
// GenericResourceLoader class interface
//
// Generic resource loading functionality. Simply reads raw data and returns
// a pointer to the cached data.
// ---------------------------------------------------------------------------

class GenericResourceLoader : public ResourceLoader
{
public:
	GenericResourceLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		ResourceLoader(accessor),
		mResId(res_id)
	{}

	virtual ~GenericResourceLoader() {}

	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	const ResourceId			mResId;
};



class BaseTextureLoader : public ResourceLoader
{
public:
	BaseTextureLoader(const RawResourceAccessor* accessor) :
		ResourceLoader(accessor)
	{}

	virtual ~BaseTextureLoader() {}
	
protected:
	uint32_t calculateTextureSize(uint16_t width, uint16_t height) const;
	Texture* createTexture(void* data, uint16_t width, uint16_t height) const;
};


//
// RowMajorTextureLoader
//
// Abstract base class for loading an image from a headerless row-major
// image resource and converting it to a Texture.
//
class RowMajorTextureLoader : public BaseTextureLoader
{
public:
	RowMajorTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		BaseTextureLoader(accessor),
		mResId(res_id)
	{}

	virtual ~RowMajorTextureLoader() {}
	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	virtual uint16_t getWidth() const = 0;
	virtual uint16_t getHeight() const = 0;

	const ResourceId	mResId;
};


//
// FlatTextureLoader
//
// Loads a Flat image resource and converts it to a Texture.
// Flat image resources are expected to be square in dimensions and each
// dimension should be a power-of-two. Certain exceptions to this are
// also handled.
//
class FlatTextureLoader : public RowMajorTextureLoader
{
public:
	FlatTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		RowMajorTextureLoader(accessor, res_id)
	{}

	virtual ~FlatTextureLoader() {}

protected:
	virtual uint16_t getWidth() const;
	virtual uint16_t getHeight() const;
};


//
// RawTextureLoader
//
// Loads a raw full-screen (320x200) image resource and converts it to a Texture.
//
class RawTextureLoader : public RowMajorTextureLoader
{
public:
	RawTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		RowMajorTextureLoader(accessor, res_id)
	{}

	virtual ~RawTextureLoader() {}

protected:
	virtual uint16_t getWidth() const;
	virtual uint16_t getHeight() const;
};


//
// BasePatchTextureLoader
//
class BasePatchTextureLoader : public BaseTextureLoader
{
public:
	BasePatchTextureLoader(const RawResourceAccessor* accessor, const ResourceId res_id) :
		BaseTextureLoader(accessor),
		mResId(res_id)
	{}

	virtual ~BasePatchTextureLoader() {}
	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	const ResourceId	mResId;
};


//
// CompositeTextureLoader
//
//
class CompositeTextureLoader : public BaseTextureLoader
{
public:
	CompositeTextureLoader(const RawResourceAccessor* accessor, const CompositeTextureDefinition& texdef) :
		BaseTextureLoader(accessor),
		mTexDef(texdef)
	{}

	virtual ~CompositeTextureLoader() {}

	virtual uint32_t size() const;
	virtual void load(void* data) const;

protected:
	const CompositeTextureDefinition	mTexDef;
};



















#if 0



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
	virtual void load(const ResourceId res_id, void* data) const {}		// TODO: remove this
	virtual void load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const = 0;
	virtual Texture* initTexture(void* data, uint16_t width, uint16_t height, palindex_t maskcolor=0) const;
	uint32_t calculateTextureSize(uint16_t width, uint16_t height) const;
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
	FlatTextureLoader(const RawResourceAccessor* accessor);
	virtual ~FlatTextureLoader() {}

	virtual uint32_t size(const ResourceId res_id) const;
	virtual void load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const;

private:
	int16_t getWidth(uint32_t size) const;
	int16_t getHeight(uint32_t size) const;

	const RawResourceAccessor* mRawResourceAccessor;
	const palindex_t* mColorMap;
};


// ----------------------------------------------------------------------------
// PatchTextureLoader class interface
//
// Loads patch_t format graphic lumps.
// ----------------------------------------------------------------------------

class PatchTextureLoader : public TextureLoader
{
public:
	PatchTextureLoader(const RawResourceAccessor* accessor);
	virtual ~PatchTextureLoader() {}

	virtual bool validate(const ResourceId res_id) const;
	virtual uint32_t size(const ResourceId res_id) const;
	virtual void load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const;

private:
	bool validateHelper(const uint8_t* raw_data, uint32_t raw_size) const;

	const RawResourceAccessor* mRawResourceAccessor;
};


// ----------------------------------------------------------------------------
// SpriteTextureLoader class interface
//
// Loads patch_t format sprite graphic lumps.
// ----------------------------------------------------------------------------

class SpriteTextureLoader : public PatchTextureLoader
{
public:
	SpriteTextureLoader(const RawResourceAccessor* accessor);
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
	CompositeTextureLoader(
			const RawResourceAccessor* accessor,
			const CompositeTextureDefinition* texture_def);
	virtual ~CompositeTextureLoader() {}

	virtual bool validate(const ResourceId res_id) const;
	virtual uint32_t size(const ResourceId res_id) const;
	virtual void load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const;

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
	RawTextureLoader(const RawResourceAccessor* accessor);
	virtual ~RawTextureLoader() {}

	virtual bool validate(const ResourceId res_id) const;
	virtual uint32_t size(const ResourceId res_id) const;
	virtual void load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
};


// ----------------------------------------------------------------------------
// PngTextureLoader class interface
//
// Loads PNG format graphic lumps.
// ----------------------------------------------------------------------------

class PngTextureLoader : public TextureLoader
{
public:
	PngTextureLoader(const RawResourceAccessor* accessor, const OString& name);
	virtual ~PngTextureLoader() {}

	virtual bool validate(const ResourceId res_id) const;
	virtual uint32_t size(const ResourceId res_id) const;
	virtual void load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const;

private:
	const RawResourceAccessor* mRawResourceAccessor;
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

#endif // if 0

#endif	// __RES_RESOURCELOADER_H__
