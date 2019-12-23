#include <cmath>

#include "resources/res_resourceloader.h"
#include "resources/res_texture.h"
#include "i_system.h"
#include "m_memio.h"
#include "v_palette.h"

#include "cmdlib.h"

size_t R_CalculateNewPatchSize(patch_t *patch, size_t length);
void R_ConvertPatch(patch_t *rawpatch, patch_t *newpatch);

#ifdef USE_PNG
	#define PNG_SKIP_SETJMP_CHECK
	#include <setjmp.h>		// used for error handling by libpng

	#include <zlib.h>
	#include <png.h>

	#if (PNG_LIBPNG_VER < 10400)
		// [SL] add data types to support versions of libpng prior to 1.4.0

		/* png_alloc_size_t is guaranteed to be no smaller than png_size_t,
		 * and no smaller than png_uint_32.  Casts from png_size_t or png_uint_32
		 * to png_alloc_size_t are not necessary; in fact, it is recommended
		 * not to use them at all so that the compiler can complain when something
		 * turns out to be problematic.
		 * Casts in the other direction (from png_alloc_size_t to png_size_t or
		 * png_uint_32) should be explicitly applied; however, we do not expect
		 * to encounter practical situations that require such conversions.
		 */

		#if defined(__TURBOC__) && !defined(__FLAT__)
		   typedef unsigned long png_alloc_size_t;
		#else
		#  if defined(_MSC_VER) && defined(MAXSEG_64K)
			 typedef unsigned long    png_alloc_size_t;
		#  else
			 /* This is an attempt to detect an old Windows system where (int) is
			  * actually 16 bits, in that case png_malloc must have an argument with a
			  * bigger size to accomodate the requirements of the library.
			  */
		#    if (defined(_Windows) || defined(_WINDOWS) || defined(_WINDOWS_)) && \
				(!defined(INT_MAX) || INT_MAX <= 0x7ffffffeL)
			   typedef DWORD         png_alloc_size_t;
		#    else
			   typedef png_size_t    png_alloc_size_t;
		#    endif
		#  endif
		#endif
	#endif	// PNG_LIBPNG_VER < 10400
#endif	// USE_PNG


//
// Res_DrawPatchIntoTexture
//
// Draws a lump in patch_t format into a Texture at the given offset.
//
static void Res_DrawPatchIntoTexture(
		Texture* texture,
		const byte* lump_data, uint32_t lump_length,
		int xoffs, int yoffs)
{
	if (lump_length < 8)		// long enough for header data?
		return;

	int texwidth = texture->getWidth();
	int texheight = texture->getHeight();
	int16_t patchwidth = LESHORT(*(int16_t*)(lump_data + 0));
	int16_t patchheight = LESHORT(*(int16_t*)(lump_data + 2));

	const int32_t* colofs = (int32_t*)(lump_data + 8);

	if (patchwidth <= 0 || patchheight <= 0 ||
		lump_length < 8 + patchwidth * sizeof(*colofs))		// long enough for column offset table?
		return;

	int x1 = MAX(xoffs, 0);
	int x2 = MIN(xoffs + patchwidth - 1, texwidth - 1);

	for (int x = x1; x <= x2; x++)
	{
		int abstopdelta = 0;

		int32_t offset = LELONG(colofs[x - xoffs]);
		if (offset < 0 || lump_length < (uint32_t)offset + 1)		// long enough for this post's topdelta? 
			return;

		const byte* post = lump_data + offset; 
		while (*post != 0xFF)
		{
			if (lump_length < (uint32_t)(post - lump_data) + 2)		// long enough for this post's header?
				return;

			int posttopdelta = *(post + 0);
			int postlength = *(post + 1);

			if (lump_length < (uint32_t)(post - lump_data) + 4 + postlength)
				return;

			// [SL] Handle DeePsea tall patches: topdelta is treated as a relative
			// offset instead of an absolute offset.
			// See: http://doomwiki.org/wiki/Picture_format#Tall_patches
			if (posttopdelta <= abstopdelta)
				abstopdelta += posttopdelta;
			else
				abstopdelta = posttopdelta;

			int topoffset = yoffs + abstopdelta;
			int y1 = MAX(topoffset, 0);
			int y2 = MIN(topoffset + postlength - 1, texheight - 1);

			if (y1 <= y2)
			{
				byte* dest = texture->getData() + texheight * x + y1;
				const byte* source = post + 3;
				memcpy(dest, source, y2 - y1 + 1);
			}
			
			post += postlength + 4;
		}
	}

	/*
	for (int x = 0; x < texture->getWidth(); x++)
	{
		byte* destoffset = (byte*)texture->mCols + x * (texheight + 4);	
		*(unsigned short*)(destoffset + 0) = 0;
		*(unsigned short*)(destoffset + 2) = texheight;
		memcpy(destoffset + 4, texture->mData + x * texheight, texheight);
	}
	*/
}


// ============================================================================
//
// RawResourceAccessor class implementations
//
// ============================================================================

uint32_t RawResourceAccessor::getResourceSize(const ResourceId res_id) const
{
	const ResourceContainerId container_id = mResourceManager->getResourceContainerId(res_id);
	const ResourceContainer* container = mResourceManager->mContainers[container_id];
	return container->getResourceSize(res_id);
}

void RawResourceAccessor::loadResource(const ResourceId res_id, void* data, uint32_t size) const
{
	const ResourceContainerId container_id = mResourceManager->getResourceContainerId(res_id);
	const ResourceContainer* container = mResourceManager->mContainers[container_id];
	container->loadResource(data, res_id, size);
}


// ============================================================================
//
// ResourceLoader class implementations
//
// ============================================================================


// ----------------------------------------------------------------------------
// DefaultResourceLoader class implementation
// ----------------------------------------------------------------------------

DefaultResourceLoader::DefaultResourceLoader(const RawResourceAccessor* accessor) :
	mRawResourceAccessor(accessor)
{
	assert(mRawResourceAccessor != NULL);
}


//
// DefaultResourceLoader::size
//
// Returns the size of the raw resource lump
//
uint32_t DefaultResourceLoader::size(const ResourceId res_id) const
{
	assert(res_id != ResourceId::INVALID_ID);
	return mRawResourceAccessor->getResourceSize(res_id);
}


//
// DefaultResourceLoader::load
//
void DefaultResourceLoader::load(const ResourceId res_id, void* data) const
{
	assert(res_id != ResourceId::INVALID_ID);
	mRawResourceAccessor->loadResource(res_id, data, size(res_id));
}












// ============================================================================
//
// TextureLoader class implementations
//
// ============================================================================

//
// TextureLoader::calculateTextureSize
//
// Calculates how much memory needs to be allocated to store this texture.
//
uint32_t TextureLoader::calculateTextureSize(uint16_t width, uint16_t height) const
{
	uint32_t size = sizeof(Texture);
	#if CLIENT_APP
	size += sizeof(palindex_t) * width * height;
	#endif
	return size;
}


//
// TextureLoader::initTexture
//
Texture* TextureLoader::initTexture(void* data, uint16_t width, uint16_t height, palindex_t maskcolor) const
{
	Texture* texture = static_cast<Texture*>(data);

	texture->mWidth = std::min<int>(width, Texture::MAX_TEXTURE_WIDTH);
	texture->mHeight = std::min<int>(height, Texture::MAX_TEXTURE_HEIGHT);
	texture->mWidthBits = Log2(texture->mWidth);
	texture->mHeightBits = Log2(texture->mHeight);
	texture->mOffsetX = 0;
	texture->mOffsetY = 0;
	texture->mScaleX = FRACUNIT;
	texture->mScaleY = FRACUNIT;
	texture->mMasked = false;
	texture->mMaskColor = maskcolor;
	texture->mData = NULL;

	#if CLIENT_APP
	// mData follows the header in memory
	texture->mData = (palindex_t*)(data + sizeof(Texture));
	memset(texture->mData, texture->mMaskColor, sizeof(palindex_t) * width * height);
	#endif

	return texture;
}


// ----------------------------------------------------------------------------
// InvalidTextureLoader class implementation
//
// ----------------------------------------------------------------------------

InvalidTextureLoader::InvalidTextureLoader()
{ }


//
// InvalidTextureLoader::size
//
// Calculates the size of the resulting Texture instance for the
// generated texture.
//
uint32_t InvalidTextureLoader::size() const
{
	return calculateTextureSize(WIDTH, HEIGHT);
}


//
// InvalidTextureLoader::load
//
// Generate a checkerboard pattern Texture instance.
//
void InvalidTextureLoader::load(void* data) const
{
	Texture* texture = initTexture(data, WIDTH, HEIGHT);

	#if CLIENT_APP
	const argb_t color1(0, 0, 255);		// blue
	const argb_t color2(0, 255, 255);	// yellow
	const palindex_t color1_index = V_BestColor(V_GetDefaultPalette()->basecolors, color1);
	const palindex_t color2_index = V_BestColor(V_GetDefaultPalette()->basecolors, color2);

	for (int x = 0; x < WIDTH / 2; x++)
	{
		memset(texture->mData + x * HEIGHT + 0, color1_index, HEIGHT / 2);
		memset(texture->mData + x * HEIGHT + HEIGHT / 2, color2_index, HEIGHT / 2);
	}
	for (int x = WIDTH / 2; x < WIDTH; x++)
	{
		memset(texture->mData + x * HEIGHT + 0, color2_index, HEIGHT / 2);
		memset(texture->mData + x * HEIGHT + HEIGHT / 2, color1_index, HEIGHT / 2);
	}
	#endif
}


// ----------------------------------------------------------------------------
//
// FlatTextureLoader class implementation
//
// ----------------------------------------------------------------------------

FlatTextureLoader::FlatTextureLoader(const RawResourceAccessor* accessor) :
	mRawResourceAccessor(accessor)
{
	assert(mRawResourceAccessor != NULL);
}


//
// FlatTextureLoader::getWidth
//
// Returns the width of the FLAT texture. There is no header and the texture is
// assumed to be a square.
//
// From http://zdoom.org/wiki/Flat:
// Heretic features a few 64x65 flats, and Hexen a few 64x128 flats. Those
// were used to "cheat" with the implementation of scrolling effects. ZDoom
// does not need the same hacks to make flats scroll properly, and therefore
// ignores the excess pixels in these flats.
//
int16_t FlatTextureLoader::getWidth(uint32_t size) const
{
	if (size == 64 * 64)
		return 64;
	else if (size == 128 * 128)
		return 128;
	else if (size == 256 * 256)
		return 256;
	else if (size == 8 * 8)
		return 8;
	else if (size == 16 * 16)
		return 16;
	else if (size == 32 * 32)
		return 32;
	else if (size == 64 * 65)		// Hexen scrolling flat
		return 64;
	else if (size == 64 * 128)		// Hexen scrolling flat
		return 64;
	else if (size > 0)
		return (int16_t)sqrt(double(size));
	return 0;
}

//
// FlatTextureLoader::getHeight
//
// Flats are assumed to be square so the height will always match the width.
//
int16_t FlatTextureLoader::getHeight(uint32_t size) const
{
	return getWidth(size);
}


//
// FlatTextureLoader::size
//
// Calculates the size of the resulting Texture instance.
//
uint32_t FlatTextureLoader::size(const ResourceId res_id) const
{
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	const uint16_t width = getWidth(raw_size), height = getHeight(raw_size);
	return calculateTextureSize(width, height);
}


//
// FlatTextureLoader::load
//
// Converts the FLAT texture resource to a Texture instance.
//
void FlatTextureLoader::load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const
{
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	const uint16_t width = getWidth(raw_size), height = getHeight(raw_size);
	Texture* texture = initTexture(data, width, height, maskcolor);

	#if CLIENT_APP
	uint8_t* raw_data = new uint8_t[width * height];
	mRawResourceAccessor->loadResource(res_id, raw_data, width * height);

	// convert the row-major raw data to into column-major
	Res_TransposeImage(texture->getData(), raw_data, width, height);

	/*
	palindex_t* _data = texture->getData();
	for (int i = 0; i < width * height; i++)
		_data[i] = colormap[_data[i]];
	*/

	delete [] raw_data;
	#endif
}


// ----------------------------------------------------------------------------
// PatchTextureLoader class implementation
//
// ----------------------------------------------------------------------------

PatchTextureLoader::PatchTextureLoader(const RawResourceAccessor* accessor) :
	mRawResourceAccessor(accessor)
{
	assert(mRawResourceAccessor != NULL);
}


//
// PatchTextureLoader::validate
//
bool PatchTextureLoader::validate(const ResourceId res_id) const
{
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	uint8_t* raw_data = new uint8_t[raw_size];
	mRawResourceAccessor->loadResource(res_id, raw_data, raw_size);

	bool valid = validateHelper(raw_data, raw_size);

	delete [] raw_data;
	return valid;
}

//
// PatchTextureLoader::validateHelper
//
// Returns true if the raw patch_t data is valid.
//
bool PatchTextureLoader::validateHelper(const uint8_t* raw_data, uint32_t raw_size) const
{
	if (raw_size > 8)
	{
		const int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
		const int16_t height = LESHORT(*(int16_t*)(raw_data + 2));

		const uint32_t column_table_offset = 8;
		const uint32_t column_table_length = sizeof(int32_t) * width;

		if (width > 0 && height > 0 && raw_size >= column_table_offset + column_table_length)
		{
			const int32_t* column_offset = (const int32_t*)(raw_data + column_table_offset);
			const int32_t min_column_offset = column_table_offset + column_table_length;
			const int32_t max_column_offset = raw_size - 4;

			for (int i = 0; i < width; i++, column_offset++)
				if (*column_offset < min_column_offset || *column_offset > max_column_offset)
					return false;
			return true;
		}
	}
	return false;
}


//
// PatchTextureLoader::size
//
// Reads the patch_t header and calculates the size of the resulting
// Texture instance.
//
uint32_t PatchTextureLoader::size(const ResourceId res_id) const
{
	#ifdef USE_TEXTURES
	uint8_t raw_data[4];
	mRawResourceAccessor->loadResource(res_id, raw_data, 4);
	int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
	int16_t height = LESHORT(*(int16_t*)(raw_data + 2));
	return calculateTextureSize(width, height);

	#else
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	uint8_t* raw_data = new uint8_t[raw_size];
	mRawResourceAccessor->loadResource(res_id, raw_data, raw_size);
	size_t resource_size = R_CalculateNewPatchSize((patch_t*)raw_data, raw_size);
	delete [] raw_data;
	return resource_size;
	#endif
}


//
// PatchTextureLoader::load
//
// Converts the PATCH format graphic lump to a Texture instance.
//
void PatchTextureLoader::load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const
{
	#ifdef USE_TEXTURES
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	uint8_t* raw_data = new uint8_t[raw_size]; 
	mRawResourceAccessor->loadResource(res_id, raw_data, raw_size);

	int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
	int16_t height = LESHORT(*(int16_t*)(raw_data + 2));
	int16_t offsetx = LESHORT(*(int16_t*)(raw_data + 4));
	int16_t offsety = LESHORT(*(int16_t*)(raw_data + 6));

	Texture* texture = initTexture(data, width, height, maskcolor);
	texture->setOffsetX(offsetx);
	texture->setOffsetY(offsety);

	#if CLIENT_APP
	Res_DrawPatchIntoTexture(texture, raw_data, raw_size, 0, 0);

	// texture->mHasMask = (memchr(texture->mData, texture->mMaskColor, sizeof(palindex_t) * width * height) != NULL);
	#endif	// CLIENT_APP

	delete [] raw_data;
	
	
	#else
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	uint8_t* raw_data = new uint8_t[raw_size];
	mRawResourceAccessor->loadResource(res_id, raw_data, raw_size);

	bool valid = validateHelper(raw_data, raw_size);

	if (valid)
	{
		// valid patch
		R_ConvertPatch((patch_t*)data, (patch_t*)raw_data);
	}
	else
	{
		// invalid patch - just create a header with width = 0, height = 0
		memset(data, 0, sizeof(patch_t));
	}

	delete [] raw_data;
	#endif
} 



// ----------------------------------------------------------------------------
//
// SpriteTextureLoader class implementation
//
// ----------------------------------------------------------------------------

SpriteTextureLoader::SpriteTextureLoader(const RawResourceAccessor* accessor) :
	PatchTextureLoader(accessor)
{ }





// ----------------------------------------------------------------------------
// CompositeTextureLoader class implementation
//
// Generates composite textures given a wall texture definition.
// ----------------------------------------------------------------------------

CompositeTextureLoader::CompositeTextureLoader(
		const RawResourceAccessor* accessor,
		const CompositeTextureDefinition* texture_def) :
	mRawResourceAccessor(accessor), mTextureDef(texture_def)
{
	assert(mRawResourceAccessor != NULL);
}


//
// CompositeTextureLoader::validate
//
bool CompositeTextureLoader::validate(const ResourceId res_id) const
{
	return true;
}


//
// CompositeTextureLoader::size
//
// Calculates the size of the Texture instance resulting from the given
// CompositeTextureDefinition information.
//
uint32_t CompositeTextureLoader::size(const ResourceId res_id) const
{
	return Texture::calculateSize(mTextureDef->mWidth, mTextureDef->mHeight);
}


//
// CompositeTextureLoader::load
//
// Composes a Texture instance from a set of PATCH format graphic lumps and
// their accompanying positioning information.
//
void CompositeTextureLoader::load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const
{
	Texture* texture = initTexture(data, mTextureDef->mWidth, mTextureDef->mHeight);
	// texture->mOffsetX = offsetx;
	// texture->mOffsetY = offsety;
	// if (mTextureDef->mScaleX)
		// texture->mScaleX = mTextureDef->mScaleX << (FRACBITS - 3);
	// if (mTextureDef->mScaleY)
		// texture->mScaleY = mTextureDef->mScaleY << (FRACBITS - 3);

	#if CLIENT_APP
	// TODO: remove this once proper masking is in place
	memset(texture->mData, 0, mTextureDef->mWidth * mTextureDef->mHeight);

	// compose the texture out of a set of patches
	for (int i = 0; i < mTextureDef->mPatchCount; i++)
	{
		const ResourceId res_id = mTextureDef->mPatches[i].mResId;

		// TODO: validate the resource id when creating the composite texture definition
		// if (mResourceManager->validateResourceId(res_id))
		if (true)
		{
			// TODO: The patch data should probably be cached...
			uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
			uint8_t* raw_data = new uint8_t[raw_size];
			mRawResourceAccessor->loadResource(res_id, raw_data, raw_size);

			Res_DrawPatchIntoTexture(
					texture,
					raw_data,
					raw_size,
					mTextureDef->mPatches[i].mOriginX,
					mTextureDef->mPatches[i].mOriginY);
		}
	}
	#endif
}

// ----------------------------------------------------------------------------
// RawTextureLoader class implementation
//
// ----------------------------------------------------------------------------

RawTextureLoader::RawTextureLoader(const RawResourceAccessor* accessor) :
	mRawResourceAccessor(accessor)
{
	assert(mRawResourceAccessor != NULL);
}

//
// RawTextureLoader::validate
//
// Verifies that the lump has the appropriate size.
//
bool RawTextureLoader::validate(const ResourceId res_id) const
{
	return mRawResourceAccessor->getResourceSize(res_id) == 320 * 200 * sizeof(uint8_t);
}


//
// RawTextureLoader::size
//
// Calculates the size of the resulting Texture instance for a 320x200
// raw graphic lump.
//
uint32_t RawTextureLoader::size(const ResourceId res_id) const
{
	return calculateTextureSize(320, 200);
}


//
// RawTextureLoader::load
//
// Convert the 320x200 raw graphic lump to a Texture instance.
//
void RawTextureLoader::load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const
{
	const uint16_t width = 320, height = 200;
	Texture* texture = initTexture(data, width, height);

	#if CLIENT_APP
	uint8_t* raw_data = new uint8_t[width * height];
	mRawResourceAccessor->loadResource(res_id, raw_data, width * height);

	// convert the row-major raw data to into column-major
	Res_TransposeImage(texture->mData, raw_data, width, height);
	delete [] raw_data;
	#endif
}


// ----------------------------------------------------------------------------
// PngTextureLoader class implementation
//
// ----------------------------------------------------------------------------

//
// Res_ReadPNGCallback
//
// Callback function required for reading PNG format images stored in
// a memory buffer.
//
#ifdef CLIENT_APP
static void Res_ReadPNGCallback(png_struct* png_ptr, png_byte* dest, png_size_t length)
{
	MEMFILE* mfp = (MEMFILE*)png_get_io_ptr(png_ptr);
	mem_fread(dest, sizeof(byte), length, mfp);
}
#endif


//
// Res_PNGCleanup
//
// Helper function for TextureManager::cachePNGTexture which takes care of
// freeing the memory allocated for reading a PNG image using libpng. This
// can be called in the event of success or failure when reading the image.
//
#ifdef CLIENT_APP
static void Res_PNGCleanup(png_struct** png_ptr, png_info** info_ptr, byte** lump_data,
							png_byte** row_data, MEMFILE** mfp)
{
	png_destroy_read_struct(png_ptr, info_ptr, NULL);
	*png_ptr = NULL;
	*info_ptr = NULL;

	delete [] *lump_data;
	*lump_data = NULL;
	delete [] *row_data;
	*row_data = NULL;

	if (*mfp)
		mem_fclose(*mfp);
	*mfp = NULL;
}
#endif


PngTextureLoader::PngTextureLoader(
		const RawResourceAccessor* accessor,
		const OString& name) :
	mRawResourceAccessor(accessor), mResourceName(name)
{ }


//
// PngTextureLoader::validate
//
bool PngTextureLoader::validate(const ResourceId res_id) const
{
	// TODO: implement this correctly
	return true;
}


//
// PngTextureLoader::size
//
uint32_t PngTextureLoader::size(const ResourceId res_id) const
{
	// TODO: implement this correctly
	return sizeof(Texture);
}


//
// PngTextureLoader::load
//
// Convert the given graphic lump in PNG format to a Texture instance,
// converting from 32bpp to 8bpp using the default game palette.
//
void PngTextureLoader::load(const ResourceId res_id, void* data, palindex_t maskcolor, const palindex_t* colormap) const
{
#ifdef CLIENT_APP
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(res_id);
	uint8_t* raw_data = new uint8_t[raw_size];

	mRawResourceAccessor->loadResource(res_id, raw_data, raw_size);

	png_struct* png_ptr = NULL;
	png_info* info_ptr = NULL;
	png_byte* row_data = NULL;
	MEMFILE* mfp = NULL;

	if (!png_check_sig(raw_data, 8))
	{
		Printf(PRINT_HIGH, "Bad PNG header in %s.\n", mResourceName.c_str());
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		Printf(PRINT_HIGH, "PNG out of memory reading %s.\n", mResourceName.c_str());
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		Printf(PRINT_HIGH, "PNG out of memory reading %s.\n", mResourceName.c_str());
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	// tell libpng to retrieve image data from memory buffer instead of a disk file
	mfp = mem_fopen_read(raw_data, raw_size);
	png_set_read_fn(png_ptr, mfp, Res_ReadPNGCallback);

	png_read_info(png_ptr, info_ptr);

	// read the png header
	png_uint_32 width = 0, height = 0;
	int bitsperpixel = 0, colortype = -1;
	png_uint_32 ret = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitsperpixel, &colortype, NULL, NULL, NULL);

	if (ret != 1)
	{
		Printf(PRINT_HIGH, "Bad PNG header in %s.\n", mResourceName.c_str());
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	Texture* texture = initTexture(data, width, height);
	memset(texture->mData, 0, width * height);

	// convert the PNG image to a convenient format

	// convert transparency to full alpha
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	// convert grayscale, if needed.
	if (colortype == PNG_COLOR_TYPE_GRAY && bitsperpixel < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);

	// convert paletted images to RGB
	if (colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	// convert from RGB to ARGB
	if (colortype == PNG_COLOR_TYPE_PALETTE || colortype == PNG_COLOR_TYPE_RGB)
	   png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);

	// process the above transformations
	png_read_update_info(png_ptr, info_ptr);

	// Read the new color type after updates have been made.
	colortype = png_get_color_type(png_ptr, info_ptr);

	// read the image and store in temp_image
	const png_size_t row_size = png_get_rowbytes(png_ptr, info_ptr);

	row_data = new png_byte[row_size];
	for (unsigned int y = 0; y < height; y++)
	{
		png_read_row(png_ptr, row_data, NULL);
		byte* dest = texture->mData + y;
		//byte* mask = texture->mMask + y;

		for (unsigned int x = 0; x < width; x++)
		{
			argb_t color(row_data[(x << 2) + 3], row_data[(x << 2) + 0],
						row_data[(x << 2) + 1], row_data[(x << 2) + 2]);

			//*mask = color.geta() != 0;
			//if (*mask)
			//	*dest = V_BestColor(V_GetDefaultPalette()->basecolors, color);

			dest += height;
			//mask += height;
		}
	}

	Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
#endif	// CLIENT_APP
}




// ============================================================================
//
// CompositeTextureDefinitionParser
//
// ============================================================================

//
// CompositeTextureDefinitionParser::CompositeTextureDefinitionParser
//
CompositeTextureDefinitionParser::CompositeTextureDefinitionParser(
		const RawResourceAccessor* accessor,
		const ResourceNameTranslator* translator) :
	mRawResourceAccessor(accessor),
	mNameTranslator(translator)
{
	ResourceIdList pnames_lookup = buildPNamesLookup();

	if (mNameTranslator->translate(ResourcePath("GLOBAL/TEXTURE1")) == ResourceId::INVALID_ID)
		I_Error("Res_InitTextures: TEXTURE1 lump not found");

	// Read each of the TEXTURE definition lumps and create a new CompositeTextureDefinition for each definition.
	static const char* const texture_definition_path_names[] = { "GLOBAL/TEXTURE1", "GLOBAL/TEXTURE2", "GLOBAL/TEXTURES", "" };

	for (size_t i = 0; texture_definition_path_names[i][0] != '\0'; i++)
	{
		const ResourcePath path(texture_definition_path_names[i]);
		const ResourceId res_id = mNameTranslator->translate(path);
		if (res_id != ResourceId::INVALID_ID)
			addTexturesFromDefinitionLump(res_id, pnames_lookup);
	}
}


//
// CompositeTextureDefinitionParser::buildPNamesLookup
//
// Parses the PNAMES resource and creates a lookup table mapping entries in
// the PNAMES resource lump to ResourceIds.
//
ResourceIdList CompositeTextureDefinitionParser::buildPNamesLookup()
{
	// Read the PNAMES lump and store the ResourceId of each patch
	// listed in the lump in the pnames_lookup array.
	const ResourceId pnames_res_id = mNameTranslator->translate(ResourcePath("GLOBAL/PNAMES"));
	if (pnames_res_id == ResourceId::INVALID_ID)
		I_Error("Res_InitTextures: PNAMES lump not found");

	uint32_t pnames_size = mRawResourceAccessor->getResourceSize(pnames_res_id);
	if (pnames_size < 4)			// not long enough to store pnames_count
		I_Error("Res_InitTextures: invalid PNAMES lump");

	uint8_t* pnames_raw_data = new uint8_t[pnames_size];

	int32_t pnames_count = LELONG(*((int32_t*)(pnames_raw_data + 0)));
	if ((uint32_t)pnames_count * 8 + 4 != pnames_size)
		I_Error("Res_InitTextures: invalid PNAMES lump");

	ResourceIdList pnames_lookup;

	for (int32_t i = 0; i < pnames_count; i++)
	{
		const char* str = (const char*)(pnames_raw_data + 4 + 8 * i);
		const OString lump_name = OStringToUpper(str, 8);
		ResourcePath path(patches_directory_name + "/" + lump_name);
		ResourceId res_id = mNameTranslator->translate(path);

		// killough 4/17/98:
		// Some wads use sprites as wall patches, so repeat check and
		// look for sprites this time, but only if there were no wall
		// patches found. This is the same as allowing for both, except
		// that wall patches always win over sprites, even when they
		// appear first in a wad. This is a kludgy solution to the wad
		// lump namespace problem.

		if (res_id == ResourceId::INVALID_ID)
		{
			path = ResourcePath(sprites_directory_name + "/" + lump_name);
			res_id = mNameTranslator->translate(path);
		}

		pnames_lookup.push_back(res_id);
	}

	delete [] pnames_raw_data;

	return pnames_lookup;
}


//
// CompositeTextureDefinitionParser::addTexturesFromDefinitionLump
//
// Creates a CompositeTextureDefinition for each texture defined in the
// given TEXTURE lump.
//
void CompositeTextureDefinitionParser::addTexturesFromDefinitionLump(const ResourceId res_id, const ResourceIdList& pnames_lookup)
{
	uint32_t lump_size = mRawResourceAccessor->getResourceSize(res_id);
	if (lump_size < 4)		// not long enough to store definition_count
		return;

	uint8_t* lump_data = new uint8_t[lump_size];
	mRawResourceAccessor->loadResource(res_id, lump_data, lump_size);

	int32_t definition_count = LELONG(*((int32_t*)(lump_data + 0)));
	for (int32_t i = 0; i < definition_count; i++)
	{
		int32_t def_offset = 4 * i + 4;
		if (lump_size < (unsigned int)def_offset)
			break;

		// Read a texture definition, create a CompositeTextureDefinition,
		// and add a new CompositeTextureLoader to the list.
		int32_t tex_offset = LELONG(*((int32_t*)(lump_data + def_offset)));
		if (lump_size < (unsigned int)tex_offset + 22)
			break;

		const char* str = (const char*)(lump_data + tex_offset + 0);
		const OString name = OStringToUpper(str, 8);

		// From ChocolateDoom r_data.c:
		// Vanilla Doom does a linear search of the texures array
		// and stops at the first entry it finds.  If there are two
		// entries with the same name, the first one in the array
		// wins.
		if (getByName(name) == NULL)
		{
			CompositeTextureDefinition texture_def;
			texture_def.mScaleX = *(uint8_t*)(lump_data + tex_offset + 10);
			texture_def.mScaleY = *(uint8_t*)(lump_data + tex_offset + 11);
			texture_def.mWidth = LESHORT(*((int16_t*)(lump_data + tex_offset + 12)));
			texture_def.mHeight = LESHORT(*((int16_t*)(lump_data + tex_offset + 14)));

			texture_def.mPatchCount = LESHORT(*((int16_t*)(lump_data + tex_offset + 20)));
			texture_def.mPatches = new CompositeTextureDefinition::texdefpatch_t[texture_def.mPatchCount];
			for (int16_t j = 0; j < texture_def.mPatchCount; j++)
			{
				int32_t patch_offset = tex_offset + 22 + 10 * j;		// sizeof(mappatch_t) == 10
				if (lump_size < (unsigned int)patch_offset + 6)
					break;

				texture_def.mPatches[j].mOriginX = LESHORT(*((int16_t*)(lump_data + patch_offset + 0)));
				texture_def.mPatches[j].mOriginY = LESHORT(*((int16_t*)(lump_data + patch_offset + 2)));
				int16_t patch_num = LESHORT(*((int16_t*)(lump_data + patch_offset + 4)));
				if ((size_t)patch_num < pnames_lookup.size())
					texture_def.mPatches[j].mResId = pnames_lookup[patch_num];
				// TODO: handle invalid pnames indices
			}

			mTextureDefinitionLookup.insert(std::make_pair(name, texture_def));
		}
	}

	delete [] lump_data;
}


//
// CompositeTextureDefinitionParser::getByName
//
const CompositeTextureDefinition* CompositeTextureDefinitionParser::getByName(const OString& name) const
{
	TextureDefinitionTable::const_iterator it = mTextureDefinitionLookup.find(name);
	if (it != mTextureDefinitionLookup.end())
		return &(it->second);
	return NULL;
}



// ============================================================================
//
// TextureLoaderFactory
//
// ============================================================================

//
// TextureLoaderFactory::TextureLoaderFactory
//
TextureLoaderFactory::TextureLoaderFactory(
		const RawResourceAccessor* accessor,
		const ResourceNameTranslator* translator,
		const CompositeTextureDefinitionParser* texture_definitions) :
	mRawResourceAccessor(accessor),
	mNameTranslator(translator),
	mCompositeTextureDefinitions(texture_definitions)
{ }


TextureLoader* TextureLoaderFactory::createTextureLoader(
		const ResourcePath& res_path,
		const ResourceId res_id) const
{
	const OString& directory = res_path.first();

	// Handle omitted wall textures
	// const OString& name = res_path.last();
	// if (directory == textures_directory_name && name.size() > 0 && name.c_str()[0] == '-')
		// return new InvalidTextureLoader();

	if (directory == "FLATS")
	{
		return new FlatTextureLoader(mRawResourceAccessor);
	}
	else if (directory == "TEXTURES")
	{
		const CompositeTextureDefinition* tex_def = mCompositeTextureDefinitions->getByName(res_path[1]);
		// TODO: need to fix this
		//return new CompositeTextureLoader(mRawResourceAccessor);
		return NULL;
	}
	else if (directory == "PATCHES")
	{
		return new PatchTextureLoader(mRawResourceAccessor);
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