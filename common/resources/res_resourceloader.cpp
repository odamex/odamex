#include <cmath>
#include "cmdlib.h"
#include "i_system.h"
#include "v_palette.h"

#include "resources/res_resourceloader.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"
#include "resources/res_identifier.h"

#ifdef USE_PNG
	#define PNG_SKIP_SETJMP_CHECK
	#include <setjmp.h>		// used for error handling by libpng

	#include <zlib.h>
	#include <png.h>
	#include "m_memio.h"		// used for MEMFILE
#endif	// USE_PNG


//
// Res_TransposeImage
//
// Converts an image buffer from row-major format to column-major format.
// TODO: Use cache-blocking to optimize
//
static void Res_TransposeImage(palindex_t* dest, const palindex_t* source, int width, int height)
{
	const palindex_t* translation = V_GetDefaultPalette()->mask_translation;

	for (int x = 0; x < width; x++)
	{
		const palindex_t* source_column = source + x;
		
		for (int y = 0; y < height; y++)
		{
			*dest = translation[*source_column];
			source_column += width;
			dest++;
		}
	}
}


//
// Res_DrawPatchIntoTexture
//
// Draws a lump in patch_t format into a Texture at the given offset.
//
static void Res_DrawPatchIntoTexture(
		Texture* texture,
		const uint8_t* lump_data, uint32_t lump_length,
		int xoffs, int yoffs)
{
	if (lump_length < 8)		// long enough for header data?
		return;

	const palindex_t* translation = V_GetDefaultPalette()->mask_translation;

	int texwidth = texture->mWidth;
	int texheight = texture->mHeight;
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

		const uint8_t* post = lump_data + offset; 
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
				palindex_t* dest = texture->mData + texheight * x + y1;
				const palindex_t* source = post + 3;
				if (translation)
				{
					for (int i = 0; i < y2 - y1 + 1; i++)
						dest[i]	= translation[source[i]];
				}
				else
				{
					memcpy(dest, source, y2 - y1 + 1);
				}
			}
			
			post += postlength + 4;
		}
	}
}


//
// BaseTextureLoader::calculateTextureSize
//
uint32_t BaseTextureLoader::calculateTextureSize(uint16_t width, uint16_t height) const
{
	return Texture::calculateSize(width, height);
}


Texture* BaseTextureLoader::createTexture(void* data, uint16_t width, uint16_t height) const
{
	Texture* texture = static_cast<Texture*>(data);
	texture->init(width, height);
	return texture;
}


//
// RowMajorTextureLoader::size
//
uint32_t RowMajorTextureLoader::size() const
{
	return calculateTextureSize(getWidth(), getHeight());
}


//
// RowMajorTextureLoader::load
//
// Loads a raw image resource in row-major format and transposes it to
// column-major format.
//
void RowMajorTextureLoader::load(void* data) const
{
	uint16_t width = getWidth();
	uint16_t height = getHeight();
	Texture* texture = createTexture(data, width, height);

	#if CLIENT_APP
	if (width > 0 && height > 0)
	{
		uint32_t raw_size = mRawResourceAccessor->getResourceSize(mResId);
		uint8_t* raw_data = new uint8_t[raw_size];
		mRawResourceAccessor->loadResource(mResId, raw_data, raw_size);

		// convert the row-major raw data to into column-major
		Res_TransposeImage(texture->mData, raw_data, width, height);

		delete [] raw_data;
	}
	#endif
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
uint16_t FlatTextureLoader::getWidth() const
{
	uint32_t size = mRawResourceAccessor->getResourceSize(mResId);

	if (size == sizeof(palindex_t) * 64 * 64)
		return 64;
	else if (size == sizeof(palindex_t) * 128 * 128)
		return 128;
	else if (size == sizeof(palindex_t) * 256 * 256)
		return 256;
	else if (size == sizeof(palindex_t) * 8 * 8)
		return 8;
	else if (size == sizeof(palindex_t) * 16 * 16)
		return 16;
	else if (size == sizeof(palindex_t) * 32 * 32)
		return 32;
	else if (size == sizeof(palindex_t) * 64 * 65)		// Hexen scrolling flat
		return 64;
	else if (size == sizeof(palindex_t) * 64 * 128)		// Hexen scrolling flat
		return 64;
	else if (size > 0)
		return (uint16_t)(sqrt(double(size)) / sizeof(palindex_t));
	return 0;
}


//
// FlatTextureLoader::getHeight
//
// See the comment for FlatTextureLoader::getWidth
//
uint16_t FlatTextureLoader::getHeight() const
{
	return getWidth();
}


//
// RawTextureLoader::getWidth
//
// All raw image resources should be 320x200. If the resource appears
// to be non-conformant, return 0.
//
uint16_t RawTextureLoader::getWidth() const
{
	uint32_t size = mRawResourceAccessor->getResourceSize(mResId);
	if (size == sizeof(palindex_t) * 320 * 200)
		return 320;
	return 0;
}


//
// RawTextureLoader::getHeight
//
// All raw image resources should be 320x200. If the resource appears
// to be non-conformant, return 0.
//
uint16_t RawTextureLoader::getHeight() const
{
	uint32_t size = mRawResourceAccessor->getResourceSize(mResId);
	if (size == sizeof(palindex_t) * 320 * 200)
		return 200;
	return 0;
}


//
// PatchTextureLoader::size
//
uint32_t PatchTextureLoader::size() const
{
	// read the patch_t header to extract width & height
	uint8_t raw_data[4];
	mRawResourceAccessor->loadResource(mResId, raw_data, 4);
	int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
	int16_t height = LESHORT(*(int16_t*)(raw_data + 2));
	return calculateTextureSize(width, height);
}


//
// PatchTextureLoader::load
//
void PatchTextureLoader::load(void* data) const
{
	uint32_t patch_size = mRawResourceAccessor->getResourceSize(mResId);
	uint8_t* patch_data = new uint8_t[patch_size];
	mRawResourceAccessor->loadResource(mResId, patch_data, patch_size);

	int16_t width = 0, height = 0, offsetx = 0, offsety = 0;
	if (Res_ValidatePatchData(patch_data, patch_size))
	{
		width = LESHORT(*(int16_t*)(patch_data + 0));
		height = LESHORT(*(int16_t*)(patch_data + 2));
		offsetx = LESHORT(*(int16_t*)(patch_data + 4));
		offsety = LESHORT(*(int16_t*)(patch_data + 6));
	}

	Texture* texture = createTexture(data, width, height);
	texture->mOffsetX = offsetx;
	texture->mOffsetY = offsety;

	#if CLIENT_APP
	if (width > 0 && height > 0)
	{
		Res_DrawPatchIntoTexture(texture, patch_data, patch_size, 0, 0);
	}
	#endif

	delete [] patch_data;
}


//
// CompositeTextureLoader::size
//
uint32_t CompositeTextureLoader::size() const
{
	return calculateTextureSize(mTexDef.mWidth, mTexDef.mHeight);
}


//
// CompositeTextureLoader::load
//
// Composes a texture from one or more patch image resources.
//
void CompositeTextureLoader::load(void* data) const
{
	Texture* texture = createTexture(data, mTexDef.mWidth, mTexDef.mHeight);

	// Handle ZDoom scaling extensions
	// [RH] Special for beta 29: Values of 0 will use the tx/ty cvars
	// to determine scaling instead of defaulting to 8. I will likely
	// remove this once I finish the betas, because by then, users
	// should be able to actually create scaled textures.
	if (mTexDef.mScaleX > 0)
		texture->mScaleX = mTexDef.mScaleX << (FRACBITS - 3);
	if (mTexDef.mScaleY > 0)
		texture->mScaleY = mTexDef.mScaleY << (FRACBITS - 3);

	#if CLIENT_APP
	for (CompositeTextureDefinition::PatchDefList::const_iterator it = mTexDef.mPatchDefs.begin(); it != mTexDef.mPatchDefs.end(); ++it)
	{
		const CompositeTextureDefinition::PatchDef& patch_def = *it;

		if (patch_def.mResId != ResourceId::INVALID_ID)
		{

			uint32_t patch_size = mRawResourceAccessor->getResourceSize(patch_def.mResId);
			uint8_t* patch_data = new uint8_t[patch_size];
			mRawResourceAccessor->loadResource(patch_def.mResId, patch_data, patch_size);

			if (Res_ValidatePatchData(patch_data, patch_size))
			{
				Res_DrawPatchIntoTexture(texture, patch_data, patch_size, patch_def.mOriginX, patch_def.mOriginY);
			}

			delete [] patch_data;
		}
	}
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
#ifdef USE_PNG
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
#ifdef USE_PNG
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


//
// PngTextureLoader::readHeader
//
// Loads the PNG header to read the width, height, and color type.
//
void PngTextureLoader::readHeader()
{
	#ifdef USE_PNG
	const char* resource_name = mRawResourceAccessor->getResourcePath(mResId).c_str();
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(mResId);
	uint8_t* raw_data = new uint8_t[raw_size];

	mRawResourceAccessor->loadResource(mResId, raw_data, raw_size);

	png_struct* png_ptr = NULL;
	png_info* info_ptr = NULL;
	png_byte* row_data = NULL;
	MEMFILE* mfp = NULL;

	if (png_sig_cmp(raw_data, 0, 8) != 0)
	{
		Printf(PRINT_HIGH, "Bad PNG header in %s.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		Printf(PRINT_HIGH, "PNG out of memory reading %s.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		Printf(PRINT_HIGH, "PNG out of memory reading %s.\n", resource_name);
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
		Printf(PRINT_HIGH, "Bad PNG header in %s.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
		return;
	}

	mWidth = width;
	mHeight = height;
	mBitsPerPixel = bitsperpixel;
	mColorType = colortype;

	Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
	#endif	// USE_PNG
}


//
// PngTextureLoader::size
//
uint32_t PngTextureLoader::size() const
{
	return calculateTextureSize(mWidth, mHeight);
}


//
// PngTextureLoader::load
//
// Convert the given graphic lump in PNG format to a Texture instance,
// converting from 32bpp to 8bpp using the default game palette.
//
void PngTextureLoader::load(void* data) const
{
#if defined(USE_PNG) && defined(CLIENT_APP)
	Texture* texture = createTexture(data, mWidth, mHeight);
	const palette_t* palette = V_GetDefaultPalette();

	uint32_t raw_size = mRawResourceAccessor->getResourceSize(mResId);
	uint8_t* raw_data = new uint8_t[raw_size];

	mRawResourceAccessor->loadResource(mResId, raw_data, raw_size);

	// convert the PNG image to a convenient format
	png_struct* png_ptr = NULL;
	png_info* info_ptr = NULL;
	png_byte* row_data = NULL;
	MEMFILE* mfp = NULL;
	uint8_t colortype = mColorType;

	// convert transparency to full alpha
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);

	// convert grayscale, if needed.
	if (colortype == PNG_COLOR_TYPE_GRAY && mBitsPerPixel < 8)
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
	for (unsigned int y = 0; y < mHeight; y++)
	{
		png_read_row(png_ptr, row_data, NULL);
		byte* dest = texture->mData + y;

		for (unsigned int x = 0; x < mWidth; x++)
		{
			argb_t color(row_data[(x << 2) + 3], row_data[(x << 2) + 0],
						row_data[(x << 2) + 1], row_data[(x << 2) + 2]);

			if (color.geta() < 255)
				*dest = palette->mask_color;
			else
				*dest = V_BestColor(palette->basecolors, color);

			dest += mHeight;
		}
	}

	Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &row_data, &mfp);
#endif	// USE_PNG && CLIENT_APP
}