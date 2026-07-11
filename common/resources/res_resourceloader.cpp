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
// Loads a resource from a ResourceContainer, performing any necessary data
// conversion and caching the resulting instance in the Zone memory system.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <cmath>

#include "resources/res_main.h"
#include "resources/res_texture.h"

#include "resources/res_resourceloader.h"
#include "resources/res_identifier.h"

#include "v_palette.h"

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

			if (postlength < 0 || lump_length < (uint32_t)(post - lump_data) + 4 + postlength)
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
	uint8_t raw_data[4];
	if (mRawResourceAccessor->getResourceSize(mResId) < sizeof(raw_data))
		return calculateTextureSize(0, 0);

	// read the patch_t header to extract width & height
	mRawResourceAccessor->loadResource(mResId, raw_data, 4);
	int16_t width = LESHORT(*(int16_t*)(raw_data + 0));
	int16_t height = LESHORT(*(int16_t*)(raw_data + 2));

	// nonsense dimensions mean load() will reject the patch data and create
	// an empty texture, so don't let them inflate the allocation either
	if (width < 0 || height < 0)
		return calculateTextureSize(0, 0);

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
	if (mTexDef.mScaleX > 0)
		texture->mScaleX = mTexDef.mScaleX << (FRACBITS - 3);
	if (mTexDef.mScaleY > 0)
		texture->mScaleY = mTexDef.mScaleY << (FRACBITS - 3);

	#if CLIENT_APP
	for (CompositeTextureDefinition::PatchDefList::const_iterator it = mTexDef.mPatchDefs.begin(); it != mTexDef.mPatchDefs.end(); ++it)
	{
		const CompositeTextureDefinition::PatchDef& patch_def = *it;

		if (Res_CheckResource(patch_def.mResId))
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


//
// InMemoryTextureLoader::size
//
uint32_t InMemoryTextureLoader::size() const
{
	return calculateTextureSize(mWidth, mHeight);
}


//
// InMemoryTextureLoader::load
//
void InMemoryTextureLoader::load(void* data) const
{
	Texture* texture = createTexture(data, mWidth, mHeight);
	#if CLIENT_APP
	memcpy(texture->mData, mSourceData.get(), mWidth * mHeight * sizeof(palindex_t));
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
static void Res_PNGCleanup(png_struct** png_ptr, png_info** info_ptr, byte** lump_data, MEMFILE** mfp)
{
	png_destroy_read_struct(png_ptr, info_ptr, NULL);
	*png_ptr = NULL;
	*info_ptr = NULL;

	delete [] *lump_data;
	*lump_data = NULL;

	if (*mfp)
		mem_fclose(*mfp);
	*mfp = NULL;
}
#endif


//
// Res_ReadPNGGrabChunk
//
// Scans the raw PNG data for a "grAb" chunk (the ZDoom extension storing a
// graphic's patch offsets) and writes the offsets when found. The chunk is
// required to appear before the image data.
//
#if defined(USE_PNG) && defined(CLIENT_APP)
static bool Res_ReadPNGGrabChunk(const uint8_t* data, uint32_t size, int16_t& xoffs, int16_t& yoffs)
{
	// walk the chunk list: 8-byte signature, then chunks laid out as
	// [4-byte length][4-byte type][data][4-byte CRC]
	uint32_t pos = 8;
	while (pos + 8 <= size)
	{
		const uint32_t length = BELONG(*(const uint32_t*)(data + pos));
		const uint8_t* chunk_type = data + pos + 4;

		if (memcmp(chunk_type, "IDAT", 4) == 0 || memcmp(chunk_type, "IEND", 4) == 0)
			break;

		if (memcmp(chunk_type, "grAb", 4) == 0 && length == 8 && pos + 16 <= size)
		{
			xoffs = (int16_t)(int32_t)BELONG(*(const uint32_t*)(data + pos + 8));
			yoffs = (int16_t)(int32_t)BELONG(*(const uint32_t*)(data + pos + 12));
			return true;
		}

		pos += 12 + length;
	}
	return false;
}
#endif	// USE_PNG && CLIENT_APP


//
// PngTextureLoader::readHeader
//
// Loads the PNG header to read the width, height, and color type.
//
void PngTextureLoader::readHeader()
{
	#ifdef USE_PNG
	// 8-byte signature + IHDR chunk header (8) + width (4) + height (4)
	// + bit depth (1) + color type (1)
	const uint32_t data_size = 26;
	if (mRawResourceAccessor->getResourceSize(mResId) >= data_size)
	{
		uint8_t data[data_size];
		mRawResourceAccessor->loadResource(mResId, data, data_size);
		mWidth = BELONG(*(uint32_t*)(data + 16));
		mHeight = BELONG(*(uint32_t*)(data + 20));
		mBitDepth = data[24];
		mColorType = data[25];
	}
	#endif	// USE_PNG
}


//
// PngTextureLoader::size
//
uint32_t PngTextureLoader::size() const
{
	uint32_t total = calculateTextureSize(mWidth, mHeight);

	#if defined(USE_PNG) && defined(CLIENT_APP)
	// Every PNG carries its image natively in an additional ARGB plane
	// (aligned to the pixel size) for the 32bpp renderer, the palettized
	// plane holds the 8bpp renderer's down-converted approximation.
	total += (sizeof(argb_t) - 1) + sizeof(argb_t) * mWidth * mHeight;
	#endif

	return total;
}


//
// Res_DitherAlphaThreshold
//
// The 8bpp renderer cannot blend, so a texel's partial alpha is
// knocked down to either opaque ortransparent with no in between.
// A flat threshold makes subtle translucency (alpha below the cutoff)
// vanish entirely, so instead the cutoff is a 4x4 Bayer dithering
// matrix over the texel position -- partial alpha get a screen door look,
// with the fraction of opaque texels tracking the alpha value.
//
static forceinline int Res_DitherAlphaThreshold(int x, int y)
{
	static const uint8_t bayer4x4[4][4] = {
		{  0,  8,  2, 10 },
		{ 12,  4, 14,  6 },
		{  3, 11,  1,  9 },
		{ 15,  7, 13,  5 }
	};

	// spread the 16 dither levels across 8..248 so alpha 0 is always
	// transparent and alpha 255 is always opaque
	return bayer4x4[y & 3][x & 3] * 16 + 8;
}


//
// PngTextureLoader::load
//
// Converts the given graphic lump in PNG format to a Texture instance.
// The image is decoded natively into the texture's ARGB plane for the
// 32bpp renderer, alongside an 8bpp approximation (converted using the
// default game palette) for the 8bpp renderer.
//
void PngTextureLoader::load(void* data) const
{
#if defined(USE_PNG) && defined(CLIENT_APP)
	Texture* texture = createTexture(data, mWidth, mHeight);
	const palette_t* palette = V_GetDefaultPalette();

	const char* resource_name = mRawResourceAccessor->getResourcePath(mResId).c_str();
	uint32_t raw_size = mRawResourceAccessor->getResourceSize(mResId);
	uint8_t* raw_data = new uint8_t[raw_size];

	mRawResourceAccessor->loadResource(mResId, raw_data, raw_size);

	png_struct* png_ptr = NULL;
	png_info* info_ptr = NULL;
	MEMFILE* mfp = NULL;

	if (png_sig_cmp(raw_data, 0, 8) != 0)
	{
		PrintFmt(PRINT_HIGH, "Bad PNG header in {}.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &mfp);
		return;
	}

	// Doom engines store patch offsets for PNG graphics in a grAb chunk.
	int16_t grab_xoffs, grab_yoffs;
	if (Res_ReadPNGGrabChunk(raw_data, raw_size, grab_xoffs, grab_yoffs))
	{
		texture->mOffsetX = grab_xoffs;
		texture->mOffsetY = grab_yoffs;
	}

	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		PrintFmt(PRINT_HIGH, "PNG out of memory reading {}.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &mfp);
		return;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		PrintFmt(PRINT_HIGH, "PNG out of memory reading {}.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &mfp);
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
		PrintFmt(PRINT_HIGH, "Bad PNG header in {}.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &mfp);
		return;
	}

	const bool palettized = (colortype == PNG_COLOR_TYPE_PALETTE);

	// handle 16 bpp images
	if (bitsperpixel == 16)
		png_set_strip_16(png_ptr);

	if (palettized)
	{
		// unpack 1/2/4-bit palette indices to one index per byte,
		// but keep the image palettized.
		if (bitsperpixel < 8)
			png_set_packing(png_ptr);
	}
	else
	{
		// convert transparency to full alpha
		if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
			png_set_tRNS_to_alpha(png_ptr);

		// convert grayscale, if needed.
		if (colortype == PNG_COLOR_TYPE_GRAY && bitsperpixel < 8)
			png_set_expand_gray_1_2_4_to_8(png_ptr);
		if (colortype == PNG_COLOR_TYPE_GRAY || colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
			png_set_gray_to_rgb(png_ptr);

		// convert from RGB to ARGB
		if (colortype == PNG_COLOR_TYPE_RGB || colortype == PNG_COLOR_TYPE_GRAY)
			png_set_add_alpha(png_ptr, 0xFF, PNG_FILLER_AFTER);
	}

	// process the above transformations
	png_read_update_info(png_ptr, info_ptr);

	const png_byte final_colortype = png_get_color_type(png_ptr, info_ptr);
	if (final_colortype != PNG_COLOR_TYPE_RGBA && final_colortype != PNG_COLOR_TYPE_PALETTE)
	{
		PrintFmt(PRINT_HIGH, "Invalid PNG color type in {}.\n", resource_name);
		Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &mfp);
		return;
	}

	// create row pointers for reading the image
	png_bytep* row_pointers = new png_bytep[mHeight];
	for (uint16_t y = 0; y < mHeight; y++)
		row_pointers[y] = new png_byte[png_get_rowbytes(png_ptr, info_ptr)];

	png_read_image(png_ptr, row_pointers);

	// The native ARGB plane follows the palettized image plane
	// inside this texture's allocation, aligned to the pixel size.
	uintptr_t plane_addr = reinterpret_cast<uintptr_t>(
	    texture->mData + sizeof(palindex_t) * mWidth * mHeight);
	plane_addr = (plane_addr + sizeof(argb_t) - 1) & ~uintptr_t(sizeof(argb_t) - 1);
	argb_t* argb_plane = reinterpret_cast<argb_t*>(plane_addr);
	texture->mARGBData = argb_plane;

	if (palettized)
	{
		// Build lookup tables translating the PNG's palette entries to
		// their native color and to game palette indices. Entries with
		// partial tRNS alpha are dithered between their matched color and
		// the mask color in the 8bpp plane, while the ARGB plane keeps their
		// true alpha.
		png_color* png_palette = NULL;
		int num_palette = 0;
		png_get_PLTE(png_ptr, info_ptr, &png_palette, &num_palette);

		png_byte* trans_alpha = NULL;
		int num_trans = 0;
		png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, NULL);

		palindex_t lut[256];
		argb_t argb_lut[256];
		for (int i = 0; i < 256; i++)
		{
			const int alpha = (i < num_trans && trans_alpha) ? trans_alpha[i] : 255;

			if (i < num_palette)
			{
				argb_lut[i] = argb_t(alpha, png_palette[i].red, png_palette[i].green,
				                     png_palette[i].blue);
				lut[i] = V_BestColor(palette->basecolors, argb_lut[i]);
			}
			else
			{
				argb_lut[i] = argb_t(0, 0, 0, 0);
				lut[i] = palette->mask_color;
			}
		}

		// write the image into the Texture in column-major form
		for (uint16_t y = 0; y < mHeight; y++)
		{
			const png_byte* row = row_pointers[y];
			uint8_t* dest = texture->mData + y;

			for (int x = 0; x < mWidth; x++)
			{
				const argb_t color = argb_lut[row[x]];
				if (color.geta() < Res_DitherAlphaThreshold(x, y))
					*dest = palette->mask_color;
				else
					*dest = lut[row[x]];
				dest += mHeight;
				argb_plane[mHeight * x + y] = color;
			}
		}
	}
	else
	{
		// write the image in the Texture, converting to column-major form
		// and converting pixels to 8bpp palettized using nearest-color matching.
		for (uint16_t y = 0; y < mHeight; y++)
		{
			const png_byte* row = row_pointers[y];
			uint8_t* dest = texture->mData + y;

			for (int x = 0; x < mWidth; x++)
			{
				const png_byte* ptr = &(row[x << 2]);
				argb_t color(ptr[3], ptr[0], ptr[1], ptr[2]);
				if (color.geta() < Res_DitherAlphaThreshold(x, y))
					*dest = palette->mask_color;
				else
					*dest = V_BestColor(palette->basecolors, color);
				dest += mHeight;

				argb_plane[mHeight * x + y] = color;
			}
		}
	}

	for (int y = 0; y < mHeight; y++)
		delete [] row_pointers[y];
	delete [] row_pointers;

	Res_PNGCleanup(&png_ptr, &info_ptr, &raw_data, &mfp);
#endif	// USE_PNG && CLIENT_APP
}
