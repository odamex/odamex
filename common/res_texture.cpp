// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: res_texture.cpp 3945 2013-07-03 14:32:48Z dr_sean $
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

#include "i_system.h"
#include "tables.h"
#include "r_defs.h"
#include "r_state.h"
#include "g_level.h"
#include "m_random.h"
#include "sc_man.h"
#include "m_memio.h"
#include "cmdlib.h"

#include <cstring>
#include <algorithm>
#include <math.h>

#include "res_texture.h"
#include "res_main.h"
#include "v_video.h"

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
static void Res_DrawPatchIntoTexture(Texture* texture, const byte* lump_data, uint32_t lump_length, int xoffs, int yoffs)
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

				// set up the mask
				byte* mask = texture->getMaskData() + texheight * x + y1;
				memset(mask, 1, y2 - y1 + 1);	
			}
			
			post += postlength + 4;
		}
	}
}


//
// Res_CopySubimage
//
// Copies a portion of source_texture and draws it into dest_texture.
// The source subimage is scaled to fit the dimensions of the destination
// texture.
//
// Note: no clipping is performed so it is possible to read past the
// end of the source texture.
//
void Res_CopySubimage(Texture* dest_texture, const Texture* source_texture,
	int dx1, int dy1, int dx2, int dy2,
	int sx1, int sy1, int sx2, int sy2)
{
	const int destwidth = dx2 - dx1 + 1; 
	const int destheight = dy2 - dy1 + 1;

	const int sourcewidth = sx2 - sx1 + 1;
	const int sourceheight = sy2 - sy1 + 1;

	const fixed_t xstep = FixedDiv(sourcewidth << FRACBITS, destwidth << FRACBITS) + 1;
	const fixed_t ystep = FixedDiv(sourceheight << FRACBITS, destheight << FRACBITS) + 1;

	int dest_offset = dx1 * dest_texture->getHeight() + dy1;
	byte* dest = dest_texture->getData() + dest_offset; 
	byte* dest_mask = dest_texture->getMaskData() + dest_offset; 

	fixed_t xfrac = 0;
	for (int xcount = destwidth; xcount > 0; xcount--)
	{
		int source_offset = (sx1 + (xfrac >> FRACBITS)) * source_texture->getHeight() + sy1;
		const byte* source = source_texture->getData() + source_offset;
		const byte* source_mask = source_texture->getMaskData() + source_offset;

		fixed_t yfrac = 0;
		for (int ycount = destheight; ycount > 0; ycount--)
		{
			*dest++ = source[yfrac >> FRACBITS];	
			*dest_mask++ = source_mask[yfrac >> FRACBITS];	
			yfrac += ystep;
		}

		dest += dest_texture->getHeight() - destheight;
		dest_mask += dest_texture->getHeight() - destheight; 

		xfrac += xstep;
	}

	// copy the source texture's offset info
	int xoffs = FixedDiv(source_texture->getOffsetX() << FRACBITS, xstep) >> FRACBITS;
	int yoffs = FixedDiv(source_texture->getOffsetY() << FRACBITS, ystep) >> FRACBITS;
	dest_texture->setOffsetX(xoffs);
	dest_texture->setOffsetY(yoffs);
}


//
// Res_TransposeImage
//
// Converts an image buffer from row-major format to column-major format.
//
void Res_TransposeImage(byte* dest, const byte* source, int width, int height)
{
	for (int x = 0; x < width; x++)
	{
		const byte* source_column = source + x;
		
		for (int y = 0; y < height; y++)
		{
			*dest = *source_column;
			source_column += width;
			dest++;
		}
	}
}


//
// Res_WarpTexture
//
// Alters the image in source_texture with a warping effect and saves the
// result in dest_texture.
//
static void Res_WarpTexture(Texture* dest_texture, const Texture* source_texture)
{
#if 0
	// [SL] Odamex experimental warping

	const palindex_t* source_buffer = source_texture->getData();

	int widthbits = source_texture->getWidthBits();
	int width = (1 << widthbits);
	int widthmask = width - 1; 
	int heightbits = source_texture->getHeightBits();
	int height = (1 << heightbits);
	int heightmask = height - 1;

	const int time_offset = level.time * 50;

	for (int x = 0; x < width; x++)
	{
		palindex_t* dest = dest_texture->getData() + (x << heightbits);

		for (int y = 0; y < height; y++)
		{
			// calculate angle such that one sinusoidal period is 64 pixels.
			// add an offset based on the current time to create movement.
			int angle_index = ((x << 7) + time_offset) & FINEMASK;

			// row and column offsets have the effect of stretching or
			// compressing the image according to a sine wave
			int row_offset = (finesine[angle_index] << 2) >> FRACBITS;
			int col_offset = (finesine[angle_index] << 1) >> FRACBITS;

			int xindex = (((x + row_offset) & widthmask) << heightbits) + y;
			int yindex = (x << heightbits) + ((y + col_offset) & heightmask);

			*dest++ = rt_blend2(source_buffer[xindex], 128, source_buffer[yindex], 127);
		}
	}
#endif 

#if 0
	// [SL] ZDoom 1.22 warping

	const palindex_t* source_buffer = source_texture->getData();
	palindex_t* warped_buffer = dest_texture->getData();
	palindex_t temp_buffer[Texture::MAX_TEXTURE_HEIGHT];

	int step = level.time * 32;
	for (int y = height - 1; y >= 0; y--)
	{
		const byte* source = source_buffer + y;
		byte* dest = warped_buffer + y;

		int xf = (finesine[(step + y * 128) & FINEMASK] >> 13) & widthmask;
		for (int x = 0; x < width; x++)
		{
			*dest = source[xf << heightbits];
			dest += height;
			xf = (xf + 1) & widthmask;
		}
	}

	step = level.time * 23;
	for (int x = width - 1; x >= 0; x--)
	{
		const byte *source = warped_buffer + (x << heightbits);
		byte *dest = temp_buffer;

		int yf = (finesine[(step + 128 * (x + 17)) & FINEMASK] >> 13) & heightmask;
		for (int y = 0; y < height; y++)
		{
			*dest++ = source[yf];
			yf = (yf + 1) & heightmask;
		}
		memcpy(warped_buffer + (x << heightbits), temp_buffer, height);
	}
#endif
}

// ============================================================================
//
// Texture
//
// ============================================================================

Texture::Texture() :
	mTextureId(TextureManager::NO_TEXTURE_ID)
{
	init(0, 0);
}


uint32_t Texture::calculateSize(int width, int height)
{
	#if CLIENT_APP
	return sizeof(Texture)						// header
		+ width * height						// mData
		+ width * height;						// mMask
	#else
	return sizeof(Texture);
	#endif
}


//
// Texture::init
//
// Sets up the member variable of Texture based on a supplied width and height.
//
void Texture::init(int width, int height)
{
	mWidth = width;
	mHeight = height;
	mWidthBits = Log2(width);
	mHeightBits = Log2(height);
	mOffsetX = 0;
	mOffsetY = 0;
	mScaleX = FRACUNIT;
	mScaleY = FRACUNIT;
	mHasMask = false;

	if (clientside)
	{
		// mData follows the header in memory
		mData = (byte*)((byte*)this + sizeof(*this));
		// mMask follows mData
		mMask = (byte*)(mData) + sizeof(byte) * width * height;
	}
	else
	{
		mData = NULL;
		mMask = NULL;
	}
}


// ============================================================================
//
// TextureLoader class implementations
//
// ============================================================================


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
	return getTextureSize(WIDTH, HEIGHT);
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
// FlatTextureLoader class implementation
//
// ----------------------------------------------------------------------------

FlatTextureLoader::FlatTextureLoader(ResourceManager* manager, const ResourceId res_id) :
	mResourceManager(manager), mResId(res_id)
{ }


//
// FlatTextureLoader::getWidth
//
// Returns the width of the FLAT texture. There is no header and the texture is
// assumed to be a square.
//
// TODO: Handle Heretic and Hexen's oddly formatted flats
// From http://zdoom.org/wiki/Flat:
// Heretic features a few 64x65 flats, and Hexen a few 64x128 flats. Those
// were used to "cheat" with the implementation of scrolling effects. ZDoom
// does not need the same hacks to make flats scroll properly, and therefore
// ignores the excess pixels in these flats.
//
int16_t FlatTextureLoader::getWidth() const
{
	uint32_t raw_size = mResourceManager->getResourceSize(mResId);
	if (raw_size > 0)
	{
		if (raw_size == 64 * 64)
			return 64;
		else if (raw_size == 8 * 8)
			return 8;
		else if (raw_size == 16 * 16)
			return 16;
		else if (raw_size == 32 * 32)
			return 32;
		else if (raw_size == 128 * 128)
			return 128;
		else if (raw_size == 256 * 256)
			return 256;
		else
			return (int16_t)sqrt(double(raw_size));		// probably not pretty... 
	}
	return 0;
}

int16_t FlatTextureLoader::getHeight() const
{
	return getWidth();
}


//
// FlatTextureLoader::size
//
// Reads the patch_t header and calculates the size of the resulting
// Texture instance.
//
uint32_t FlatTextureLoader::size() const
{
	const int16_t width = getWidth();
	const int16_t height = getHeight();
	return Texture::calculateSize(width, height);
}


//
// FlatTextureLoader::load
//
// Converts the FLAT texture resource to a Texture instance.
//
void FlatTextureLoader::load(void* data) const
{
	const uint16_t width = getWidth(), height = getHeight();
	Texture* texture = initTexture(data, width, height);

	#if CLIENT_APP
	uint8_t* raw_data = new uint8_t[width * height]; 
	mResourceManager->loadRawResource(mResId, raw_data, width * height);

	// convert the row-major raw data to into column-major
	Res_TransposeImage(texture->mData, raw_data, width, height);
	delete [] raw_data;
	#endif
}


// ----------------------------------------------------------------------------
// PatchTextureLoader class implementation
//
// ----------------------------------------------------------------------------

PatchTextureLoader::PatchTextureLoader(ResourceManager* manager, const ResourceId res_id) :
	mResourceManager(manager), mResId(res_id)
{ }


//
// PatchTextureLoader::validate
//
bool PatchTextureLoader::validate() const
{
	uint32_t raw_size = mResourceManager->getResourceSize(mResId);
	uint8_t* raw_data = new uint8_t[raw_size];
	mResourceManager->loadRawResource(mResId, raw_data, raw_size);

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
		const uint32_t column_table_length = raw_sizeof(int32_t) * width;

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
uint32_t PatchTextureLoader::size() const
{
	uint8_t raw_data[4];
	mResourceManager->loadRawResource(mResId, raw_data, 4);
	int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
	int16_t height = LESHORT(*(int16_t*)(raw_data + 2));

	return getTextureSize(width, height);
}


//
// PatchTextureLoader::load
//
// Converts the PATCH format graphic lump to a Texture instance.
//
void PatchTextureLoader::load(void* data) const
{
	uint32_t raw_size = mResourceManager->getResourceSize(mResId);
	uint8_t* raw_data = new uint8_t[raw_size]; 
	mResourceManager->loadRawResource(mResId, raw_data, raw_size);

	int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
	int16_t height = LESHORT(*(int16_t*)(raw_data + 2));
	//int16_t offsetx = LESHORT(*(int16_t*)(raw_data + 4));
	//int16_t offsety = LESHORT(*(int16_t*)(raw_data + 6));

	Texture* texture = initTexture(data, width, height);

	#if CLIENT_APP
	// TODO: remove this once proper masking is in place
	memset(texture->mData, 0, width * height);

	// initialize the mask to entirely transparent 
	memset(texture->mMask, 0, width * height);

	Res_DrawPatchIntoTexture(texture, raw_data, raw_size, 0, 0);
	// texture->mHasMask = (memchr(texture->mMask, 0, width * height) != NULL);
	#endif	// CLIENT_APP

	delete [] raw_data;
}


// ----------------------------------------------------------------------------
// CompositeTextureLoader class implementation
//
// Generates composite textures given a wall texture definition.
// ----------------------------------------------------------------------------

CompositeTextureLoader::CompositeTextureLoader(ResourceManager* manager, const CompositeTextureDefinition& texture_def) :
	mResourceManager(manager), mTextureDef(texture_def)
{ }


//
// CompositeTextureLoader::validate
//
bool CompositeTextureLoader::validate() const
{
	return true;
}


//
// CompositeTextureLoader::size
//
// Calculates the size of the Texture instance resulting from the given
// CompositeTextureDefinition information.
//
uint32_t CompositeTextureLoader::size() const
{
	return Texture::calculateSize(mTextureDef.mWidth, mTextureDef.mHeight);
}


//
// CompositeTextureLoader::load
//
// Composes a Texture instance from a set of PATCH format graphic lumps and
// their accompanying positioning information.
//
void CompositeTextureLoader::load(void* data) const
{
	Texture* texture = initTexture(data, mTextureDef.mWidth, mTextureDef.mHeight);
	// texture->mOffsetX = offsetx;
	// texture->mOffsetY = offsety;
	// if (mTextureDef.mScaleX)
		// texture->mScaleX = mTextureDef.mScaleX << (FRACBITS - 3);
	// if (mTextureDef.mScaleY)
		// texture->mScaleY = mTextureDef.mScaleY << (FRACBITS - 3);

	#if CLIENT_APP
	// TODO: remove this once proper masking is in place
	memset(texture->mData, 0, mTextureDef.mWidth * mTextureDef.mHeight);

	// initialize the mask to entirely transparent 
	memset(texture->mMask, 0, mTextureDef.mWidth * mTextureDef.mHeight);

	// compose the texture out of a set of patches
	for (int i = 0; i < mTextureDef.mPatchCount; i++)
	{
		const ResourceId res_id = mTextureDef.mPatches[i].mResId;
		if (mResourceManager->validateResourceId(res_id))
		{
			// TODO: The patch data should probably be cached...
			uint32_t raw_size = mResourceManager->getResourceSize(res_id);
			uint8_t* raw_data = new uint8_t[raw_size]; 
			mResourceManager->loadRawResource(res_id, raw_data, raw_size);

			Res_DrawPatchIntoTexture(
					texture,
					raw_data,
					raw_size,
					mTextureDef.mPatches[i].mOriginX,
					mTextureDef.mPatches[i].mOriginY);
		}
	}

	// texture->mHasMask = (memchr(texture->mMask, 0, mTextureDef.mWidth * mTextureDef.mHeight) != NULL);
	#endif
}

// ----------------------------------------------------------------------------
// RawTextureLoader class implementation
//
// ----------------------------------------------------------------------------

RawTextureLoader::RawTextureLoader(ResourceManager* manager, const ResourceId res_id) :
	mResourceManager(manager), mResId(res_id)
{ }


//
// RawTextureLoader::validate
//
// Verifies that the lump has the appropriate size.
//
bool RawTextureLoader::validate() const
{
	return mResourceManager->getResourceSize(mResId) == 320 * 200 * sizeof(uint8_t);
}


//
// RawTextureLoader::size
//
// Calculates the size of the resulting Texture instance for a 320x200
// raw graphic lump.
//
uint32_t RawTextureLoader::size() const
{
	return getTextureSize(320, 200);
}


//
// RawTextureLoader::load
//
// Convert the 320x200 raw graphic lump to a Texture instance.
//
void RawTextureLoader::load(void* data) const
{
	const uint16_t width = 320, height = 200;
	Texture* texture = initTexture(data, width, height);

	#if CLIENT_APP
	uint8_t* raw_data = new uint8_t[width * height]; 
	mResourceManager->loadRawResource(mResId, raw_data, width * height);

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


PngTextureLoader::PngTextureLoader(ResourceManager* manager, const ResourceId res_id) :
	mResourceManager(manager), mResId(res_id)
{ }


//
// PngTextureLoader::validate
//
bool PngTextureLoader::validate() const
{
	// TODO: implement this correctly
	return true;
}


//
// PngTextureLoader::size
//
uint32_t PngTextureLoader::size() const
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
void PngTextureLoader::load(void* data) const
{
#ifdef CLIENT_APP
	if (mResourceManager->validateResourceId(mResId))
	{
		const char* lump_name = OString(mResourceManager->getResourcePath(mResId)).c_str();
		uint32_t lump_length = mResourceManager->getResourceSize(mResId);
		byte* lump_data = new byte[lump_length];
		mResourceManager->loadRawResource(mResId, lump_data);

		png_struct* png_ptr = NULL;
		png_info* info_ptr = NULL;
		png_byte* row_data = NULL;
		MEMFILE* mfp = NULL;
		
		if (!png_check_sig(lump_data, 8))
		{
			Printf(PRINT_HIGH, "Bad PNG header in %s.\n", lump_name);
			Res_PNGCleanup(&png_ptr, &info_ptr, &lump_data, &row_data, &mfp);
			return;
		}

		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
		{
			Printf(PRINT_HIGH, "PNG out of memory reading %s.\n", lump_name);
			Res_PNGCleanup(&png_ptr, &info_ptr, &lump_data, &row_data, &mfp);
			return;	
		}
	  
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr)
		{
			Printf(PRINT_HIGH, "PNG out of memory reading %s.\n", lump_name);
			Res_PNGCleanup(&png_ptr, &info_ptr, &lump_data, &row_data, &mfp);
			return;
		}

		// tell libpng to retrieve image data from memory buffer instead of a disk file
		mfp = mem_fopen_read(lump_data, lump_length);
		png_set_read_fn(png_ptr, mfp, Res_ReadPNGCallback);

		png_read_info(png_ptr, info_ptr);

		// read the png header
		png_uint_32 width = 0, height = 0;
		int bitsperpixel = 0, colortype = -1;
		png_uint_32 ret = png_get_IHDR(png_ptr, info_ptr, &width, &height, &bitsperpixel, &colortype, NULL, NULL, NULL);

		if (ret != 1)
		{
			Printf(PRINT_HIGH, "Bad PNG header in %s.\n", lump_name);
			Res_PNGCleanup(&png_ptr, &info_ptr, &lump_data, &row_data, &mfp);
			return;
		}

		Texture* texture = Texture::createTexture(width, height);
		memset(texture->mData, 0, width * height);
		memset(texture->mMask, 0, width * height);

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
			byte* mask = texture->mMask + y;
			
			for (unsigned int x = 0; x < width; x++)
			{
				argb_t color(row_data[(x << 2) + 3], row_data[(x << 2) + 0],
							row_data[(x << 2) + 1], row_data[(x << 2) + 2]);

				*mask = color.geta() != 0;
				if (*mask)
					*dest = V_BestColor(V_GetDefaultPalette()->basecolors, color);

				dest += height;
				mask += height;
			}
		}

//		texture->mHasMask = (memchr(texture->mMask, 0, width * height) != NULL);

		Res_PNGCleanup(&png_ptr, &info_ptr, &lump_data, &row_data, &mfp);

		data = static_cast<void*>(texture);
	}
#endif	// CLIENT_APP
	data = NULL;
}



// ============================================================================
//
// TextureManager
//
// ============================================================================

TextureManager::TextureManager(const ResourceContainerId& container_id, ResourceManager* manager) :
	mResourceContainerId(container_id),
	mFreeCustomTextureIdsHead(0),
	mFreeCustomTextureIdsTail(TextureManager::MAX_CUSTOM_TEXTURE_IDS)
{
	registerTextureResources(manager);
	// initialize the TEXTURE1 & TEXTURE2 data
	addTextureDirectories(manager);

	if (clientside)
	{	
		readAnimDefLump();
		readAnimatedLump();
	}
}


TextureManager::~TextureManager()
{
	clear();
}


//
// TextureManager::clear
//
// Frees all memory used by TextureManager, freeing all Texture objects
// and the supporting lookup structures.
//
void TextureManager::clear()
{
	// free all custom texture tex_ids
	mFreeCustomTextureIdsHead = 0;
	mFreeCustomTextureIdsTail = TextureManager::MAX_CUSTOM_TEXTURE_IDS - 1;
	for (unsigned int i = mFreeCustomTextureIdsHead; i <= mFreeCustomTextureIdsTail; i++)
		mFreeCustomTextureIds[i] = CUSTOM_TEXTURE_ID_MASK | i;


	// Free all of the TextureLoader instances
	for (TextureLoaderList::iterator it = mTextureLoaders.begin(); it != mTextureLoaders.end(); ++it)
		delete *it;
	mTextureLoaders.clear();	

	// Free all of the Texture instances
	for (TextureList::iterator it = mTextures.begin(); it != mTextures.end(); ++it)
		if (*it)
			Z_Free((void*)*it);
	mTextures.clear();


	mAnimDefs.clear();

	// free warping original texture (not stored in mTextureIdMap)
//	for (size_t i = 0; i < mWarpDefs.size(); i++)
//		if (mWarpDefs[i].original_texture)
//			Z_Free((void*)mWarpDefs[i].original_texture);

	mWarpDefs.clear();
}


//
// TextureManager::readAnimDefLump
//
// [RH] This uses a Hexen ANIMDEFS lump to define the animation sequences.
//
void TextureManager::readAnimDefLump()
{
	/*
	const ResourceIdList res_ids = Res_GetAllResourceIds("ANIMDEFS");
	for (size_t i = 0; i < res_ids.size(); i++)
	{
		SC_OpenResourceLump(res_ids[i]);

		while (SC_GetString())
		{
			ResourcePath path;
			if (SC_Compare("flat"))
				path = flats_directory_name;
			else if (SC_Compare("texture"))
				path = textures_directory_name;

			if (!path.empty())
			{
				SC_MustGetString();
				const OString lump_name(sc_String);

				anim_t anim;
				anim.basepic = Res_GetResourceId(lump_name, path);

				anim.curframe = 0;
				anim.numframes = 0;
				memset(anim.speedmin, 1, anim_t::MAX_ANIM_FRAMES * sizeof(*anim.speedmin));
				memset(anim.speedmax, 1, anim_t::MAX_ANIM_FRAMES * sizeof(*anim.speedmax));

				while (SC_GetString())
				{
					if (!SC_Compare("pic"))
					{
						SC_UnGet();
						break;
					}

					if ((unsigned)anim.numframes == anim_t::MAX_ANIM_FRAMES)
						SC_ScriptError ("Animation has too many frames");

					byte min = 1, max = 1;
					
					SC_MustGetNumber();
					int frame = sc_Number;
					SC_MustGetString();
					if (SC_Compare("tics"))
					{
						SC_MustGetNumber();
						sc_Number = clamp(sc_Number, 0, 255);
						min = max = sc_Number;
					}
					else if (SC_Compare("rand"))
					{
						SC_MustGetNumber();
						min = MAX(sc_Number, 0);
						SC_MustGetNumber();
						max = MIN(sc_Number, 255);
						if (min > max)
							min = max = 1;
					}
					else
					{
						SC_ScriptError ("Must specify a duration for animation frame");
					}

					anim.speedmin[anim.numframes] = min;
					anim.speedmax[anim.numframes] = max;
					anim.framepic[anim.numframes] = frame + anim.basepic - 1;
					anim.numframes++;
				}

				anim.countdown = anim.speedmin[0];

				if (anim.basepic != TextureManager::NOT_FOUND_TEXTURE_ID &&
					anim.basepic != TextureManager::NO_TEXTURE_ID)
					mAnimDefs.push_back(anim);
			}
			else if (SC_Compare ("switch"))   // Don't support switchdef yet...
			{
				//P_ProcessSwitchDef ();
//				SC_ScriptError("switchdef not supported.");
			}
			else if (SC_Compare("warp"))
			{
				SC_MustGetString();

				ResourcePath path;
				if (SC_Compare("flat"))
					path = flats_directory_name;
				else if (SC_Compare("texture"))
					path = textures_directory_name;

				if (!path.empty())
				{
					SC_MustGetString();
					const OString lump_name(sc_String);

					const ResourceId res_id = Res_GetResourceId(lump_name, path);
					if (!res_id.valid())
						continue;

					warp_t warp;

					// backup the original texture
					warp.original_texture = getTexture(tex_id);

					int width = 1 << warp.original_texture->getWidthBits();
					int height = 1 << warp.original_texture->getHeightBits();

					// create a new texture of the same size for the warped image
					warp.warped_texture = Texture::createTexture(width, height);

					mWarpDefs.push_back(warp);
				}
				else
				{
					SC_ScriptError(NULL, NULL);
				}
			}
		}
		SC_Close ();
	}
	*/
}


//
// TextureManager::readAnimatedLump
//
// Reads animation definitions from the ANIMATED lump.
//
// Load the table of animation definitions, checking for existence of
// the start and end of each frame. If the start doesn't exist the sequence
// is skipped, if the last doesn't exist, BOOM exits.
//
// Wall/Flat animation sequences, defined by name of first and last frame,
// The full animation sequence is given using all lumps between the start
// and end entry, in the order found in the WAD file.
//
// This routine modified to read its data from a predefined lump or
// PWAD lump called ANIMATED rather than a static table in this module to
// allow wad designers to insert or modify animation sequences.
//
// Lump format is an array of byte packed animdef_t structures, terminated
// by a structure with istexture == -1. The lump can be generated from a
// text source file using SWANTBLS.EXE, distributed with the BOOM utils.
// The standard list of switches and animations is contained in the example
// source text file DEFSWANI.DAT also in the BOOM util distribution.
//
// [RH] Rewritten to support BOOM ANIMATED lump but also make absolutely
// no assumptions about how the compiler packs the animdefs array.
//
void TextureManager::readAnimatedLump()
{
	/*
	const ResourceId res_id = Res_GetResourceId("ANIMATED");
	if (!Res_CheckResource(res_id))
		return;

	uint32_t lumplen = Res_GetResourceSize(res_id);
	if (lumplen == 0)
		return;

	byte* lumpdata = new byte[lumplen];
	Res_LoadResource(res_id, lumpdata);

	for (byte* ptr = lumpdata; *ptr != 255; ptr += 23)
	{
		anim_t anim;

		const ResourcePath& path = *(ptr + 0) == 1 ?
				textures_directory_name : flats_directory_name;

		const char* startname = (const char*)(ptr + 10);
		const char* endname = (const char*)(ptr + 1);

		TextureId start_tex_id =
				texturemanager.getTextureId(startname, texture_type);
		TextureId end_tex_id =
				texturemanager.getTextureId(endname, texture_type);

		if (start_tex_id == TextureManager::NOT_FOUND_TEXTURE_ID ||
			start_tex_id == TextureManager::NO_TEXTURE_ID ||
			end_tex_id == TextureManager::NOT_FOUND_TEXTURE_ID ||
			end_tex_id == TextureManager::NO_TEXTURE_ID)
			continue;

		anim.basepic = start_tex_id;
		anim.numframes = end_tex_id - start_tex_id + 1;

		if (anim.numframes <= 0)
			continue;
		anim.curframe = 0;
			
		int speed = LELONG(*(int*)(ptr + 19));
		anim.countdown = speed - 1;

		for (int i = 0; i < anim.numframes; i++)
		{
			anim.framepic[i] = anim.basepic + i;
			anim.speedmin[i] = anim.speedmax[i] = speed;
		}

		mAnimDefs.push_back(anim);
	}

	delete [] lumpdata;
	*/
}


//
// TextureManager::updateAnimatedTextures
//
// TextureIds ticking the animated textures and cyles the Textures within an
// animation definition.
//
void TextureManager::updateAnimatedTextures()
{
	/*
	if (!clientside)
		return;

	// cycle the animdef textures
	for (size_t i = 0; i < mAnimDefs.size(); i++)
	{
		anim_t* anim = &mAnimDefs[i];
		if (--anim->countdown == 0)
		{
			anim->curframe = (anim->curframe + 1) % anim->numframes;

			if (anim->speedmin[anim->curframe] == anim->speedmax[anim->curframe])
				anim->countdown = anim->speedmin[anim->curframe];
			else
				anim->countdown = M_Random() %
					(anim->speedmax[anim->curframe] - anim->speedmin[anim->curframe]) +
					anim->speedmin[anim->curframe];

			// cycle the Textures
			getTexture(anim->framepic[0]);	// ensure Texture is still cached
			Texture* first_texture = mTextureIdMap[anim->framepic[0]];

			for (int frame1 = 0; frame1 < anim->numframes - 1; frame1++)
			{
				int frame2 = (frame1 + 1) % anim->numframes;
				getTexture(anim->framepic[frame2]);	// ensure Texture is still cached
				mTextureIdMap[anim->framepic[frame1]] = mTextureIdMap[anim->framepic[frame2]]; 
			}
			
			mTextureIdMap[anim->framepic[anim->numframes - 1]] = first_texture;
		}
	}

	// warp textures
	for (size_t i = 0; i < mWarpDefs.size(); i++)
	{
		const Texture* original_texture = mWarpDefs[i].original_texture;
		Texture* warped_texture = mWarpDefs[i].warped_texture;
		Res_WarpTexture(warped_texture, original_texture);
	}
	*/
}


//
// TextureManager::addTextureDirectory
//
// Reads the PNAMES, TEXTURE1 and TEXTURE2 lumps and adds a
// CompositeTextureLoader instance to the TextureLoader list to handle
// composing the texture for each definition in the TEXTURE1 and TEXTURE2
// lumps.
//
void TextureManager::addTextureDirectories(ResourceManager* manager)
{
	// Read the PNAMES lump and store the ResourceId of each patch
	// listed in the lump in the pnames_lookup array.
	const ResourceId pnames_res_id = Res_GetResourceId("PNAMES");
	if (!Res_CheckResource(pnames_res_id))
		I_Error("Res_InitTextures: PNAMES lump not found");

	uint32_t pnames_lump_length = Res_GetResourceSize(pnames_res_id);
	if (pnames_lump_length < 4)			// not long enough to store pnames_count
		I_Error("Res_InitTextures: invalid PNAMES lump");

	byte* pnames_lump_data = new byte[pnames_lump_length];
	Res_LoadResource(pnames_res_id, pnames_lump_data);

	int32_t pnames_count = LELONG(*((int32_t*)(pnames_lump_data + 0)));
	if ((uint32_t)pnames_count * 8 + 4 != pnames_lump_length)
		I_Error("Res_InitTextures: invalid PNAMES lump");
	ResourceId* pnames_lookup = new ResourceId[pnames_count];

	for (int32_t i = 0; i < pnames_count; i++)
	{
		const char* str = (const char*)(pnames_lump_data + 4 + 8 * i);
		const OString lump_name = OStringToUpper(str, 8);
		pnames_lookup[i] = Res_GetResourceId(lump_name, patches_directory_name);

		// killough 4/17/98:
		// Some wads use sprites as wall patches, so repeat check and
		// look for sprites this time, but only if there were no wall
		// patches found. This is the same as allowing for both, except
		// that wall patches always win over sprites, even when they
		// appear first in a wad. This is a kludgy solution to the wad
		// lump namespace problem.

		if (!Res_CheckResource(pnames_lookup[i]))
			pnames_lookup[i] = Res_GetResourceId(lump_name, sprites_directory_name);
	}

	delete [] pnames_lump_data;


	// Read each of the TEXTURE definition lumps and create a new
	// CompositeTextureDefinition for each definition.
	//
	static const char* const texture_definition_lump_names[] = { "TEXTURE1", "TEXTURE2", "TEXTURES", "" };

	for (uint32_t i = 0; texture_definition_lump_names[i][0] != '\0'; i++)
	{
		const ResourceId res_id = Res_GetResourceId(texture_definition_lump_names[i]);
		if (!Res_CheckResource(res_id))
		{
			if (i == 0)
				I_Error("Res_InitTextures: TEXTURE1 lump not found");
			else
				continue;		// skip this lump and go onto the next
		}

		uint32_t lump_length = Res_GetResourceSize(res_id);
		if (lump_length < 4)		// not long enough to store definition_count
			continue;

		byte* lump_data = new byte[lump_length];
		Res_LoadResource(res_id, lump_data);

		int32_t definition_count = LELONG(*((int32_t*)(lump_data + 0)));
		for (int32_t i = 0; i < definition_count; i++)
		{
			int32_t def_offset = 4 + 4 * i;
			if (lump_length < (unsigned int)def_offset)
				break;

			// Read a texture definition, create a CompositeTextureDefinition,
			// and add a new CompositeTextureLoader to the list.
			int32_t tex_offset = LELONG(*((int32_t*)(lump_data + def_offset)));
			if (lump_length < (unsigned int)tex_offset + 22)
				break;

			const char* str = (const char*)(lump_data + tex_offset + 0);
			const OString name = OStringToUpper(str, 8);
			// TODO: check if there is already an entry matching name and if so,
			// skip adding this texture.
			//
			// From ChocolateDoom r_data.c:
			// Vanilla Doom does a linear search of the texures array
			// and stops at the first entry it finds.  If there are two
			// entries with the same name, the first one in the array
			// wins. The new entry must therefore be added at the end
			// of the hash chain, so that earlier entries win.

		
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
				if (lump_length < (unsigned int)patch_offset + 6)
					break;

				texture_def.mPatches[j].mOriginX = LESHORT(*((int16_t*)(lump_data + patch_offset + 0))); 
				texture_def.mPatches[j].mOriginY = LESHORT(*((int16_t*)(lump_data + patch_offset + 2))); 
				int16_t patch_num = LESHORT(*((int16_t*)(lump_data + patch_offset + 4)));
				if (patch_num < pnames_count)
					texture_def.mPatches[j].mResId = pnames_lookup[patch_num];
				// TODO: handle invalid pnames indices
			}

			mTextures.push_back(NULL);

			// Create a new TextureLoader and add it to the list
			ResourceLoader* loader = new CompositeTextureLoader(manager, texture_def);
			mTextureLoaders.push_back(loader);
			const ResourcePath path(textures_directory_name + name);

			// TODO: save this ResourceId somewhere
			const ResourceId res_id = manager->addResource(path, this); 
		}

		delete [] lump_data;
	}

	delete [] pnames_lookup;
}


//
// TextureManager::registerTextureResources
//
// Creates a TextureLoader instance for each texture resource in the provided
// list of ResourceIds.
//
void TextureManager::registerTextureResources(ResourceManager* manager)
{
	std::vector<ResourceId> res_id_list = manager->getAllResourceIds();

	for (std::vector<ResourceId>::const_iterator it = res_id_list.begin(); it != res_id_list.end(); ++it)
	{
		const ResourceId res_id = *it;
		assert(Res_CheckResource(res_id));

		const ResourcePath& path = Res_GetResourcePath(res_id);
		const ResourcePath& directory(path.first());
		ResourceLoader* loader = NULL;

		if (directory == flats_directory_name)
			loader = new FlatTextureLoader(manager, res_id);
		else if (directory == patches_directory_name)
			loader = new PatchTextureLoader(manager, res_id);

		if (loader)
		{
			mTextures.push_back(NULL);
			mTextureLoaders.push_back(loader);
			// TODO: save this ResourceId somewhere
			const ResourceId res_id = manager->addResource(path, this); 
		}
	}
}


//
// TextureManager::createCustomTextureId
//
// Generates a valid tex_id that can be used by the engine to denote certain
// properties for a wall or ceiling or floor. For instance, a special use
// tex_id can be used to denote that a ceiling should be rendered with SKY2.
//
TextureId TextureManager::createCustomTextureId()
{
	if (mFreeCustomTextureIdsTail <= mFreeCustomTextureIdsHead)
		return TextureManager::NOT_FOUND_TEXTURE_ID;
	return mFreeCustomTextureIds[mFreeCustomTextureIdsHead++ % MAX_CUSTOM_TEXTURE_IDS];
}


//
// TextureManager::freeCustomTextureId
//
void TextureManager::freeCustomTextureId(const TextureId tex_id)
{
	mFreeCustomTextureIds[mFreeCustomTextureIdsTail++ % MAX_CUSTOM_TEXTURE_IDS] = tex_id;
}


//
// TextureManager::createTexture
//
// Allocates memory for a new texture and returns a pointer to it. The texture
// is inserted into mTextureIdsMap for future retrieval.
//
Texture* TextureManager::createTexture(const TextureId tex_id, int width, int height)
{
	width = std::min<int>(width, Texture::MAX_TEXTURE_WIDTH);
	height = std::min<int>(height, Texture::MAX_TEXTURE_HEIGHT);

	// server shouldn't allocate memory for texture data, only the header	
	uint32_t texture_size = clientside ?
			Texture::calculateSize(width, height) : sizeof(Texture);
	
	Texture* texture = (Texture*)Z_Malloc(texture_size, PU_STATIC, NULL);
	texture->init(width, height);

	texture->mTextureId = tex_id;

	mTextures.push_back(texture);
//	mTextureIdMap.insert(TextureIdMapPair(tex_id, texture));

	return texture;
}


//
// TextureManager::freeTexture
//
// Frees the memory used by the specified texture and removes it
// from mTextureIdsMap.
//
void TextureManager::freeTexture(const LumpId lump_id)
{
	if (mTextures[lump_id])
		Z_Free((void*)mTextures[lump_id]);
	mTextures[lump_id] = NULL;
}



//
// TextureManager::getTextureId
//
// Returns the tex_id for the texture that matches the supplied name.
//
TextureId TextureManager::getTextureId(const OString& name, Texture::TextureSourceType type)
{
	OString uname(StdStringToUpper(name));

	// sidedefs with the '-' texture indicate there should be no texture used
	if (uname[0] == '-' && type == Texture::TEX_WALLTEXTURE)
		return NO_TEXTURE_ID;

	TextureId tex_id = NOT_FOUND_TEXTURE_ID;

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

	return tex_id;
}


//
// TextureManager::getTextureId
//
// Returns the tex_id for the texture that matches the supplied name.
// [SL] This version will accept WAD lump names that are not properly
// zero terminated (max 8 characters).
//
TextureId TextureManager::getTextureId(const char* name, Texture::TextureSourceType type)
{
	OString uname(StdStringToUpper(name, 8));
	return getTextureId(uname, type);
}


//
// TextureManager::getTexture
//
// Returns the Texture for the appropriate tex_id. If the Texture is not
// currently cached, it will be loaded from the disk and cached.
//
/*
const Texture* TextureManager::getTexture(const TextureId tex_id) 
{
	Texture* texture = mTextureIdMap[tex_id];
	if (!texture)
	{
		if (tex_id & FLAT_TEXTURE_ID_MASK)
			cacheFlat(tex_id);
		else if (tex_id & WALLTEXTURE_ID_MASK)
			cacheWallTexture(tex_id);
		else if (tex_id & PATCH_TEXTURE_ID_MASK)
			cachePatch(tex_id);
		else if (tex_id & SPRITE_TEXTURE_ID_MASK)
			cacheSprite(tex_id);
		else if (tex_id & RAW_TEXTURE_ID_MASK)
			cacheRawTexture(tex_id);
		else if (tex_id & PNG_TEXTURE_ID_MASK)
			cachePNGTexture(tex_id);

		texture = mTextureIdMap[tex_id];
	}

	return texture;
}
*/


//
// TextureManager::getTexture
//
// Returns the Texture for the supplied ResourceId. If the Texture is not
// currently cached, it will be loaded from the disk and cached.
//
const Texture* TextureManager::getTexture(const LumpId lump_id)
{
	const Texture* texture = NULL;
	if (lump_id < mTextures.size())
	{
		texture = mTextures[lump_id];
		if (lump_id < mTextureLoaders.size())
			mTextureLoaders[lump_id]->load((void*)texture);
		if (texture)
			mTextures[lump_id] = texture;

		// TODO: set mTextureIdMap[lump_id] to not found texture if null
	}
	return texture;
}


VERSION_CONTROL (res_texture_cpp, "$Id: res_texture.cpp 3945 2013-07-03 14:32:48Z dr_sean $")
