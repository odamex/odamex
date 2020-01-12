#include <cmath>

#include "resources/res_resourceloader.h"
#include "resources/res_texture.h"
#include "i_system.h"
#include "m_memio.h"
#include "v_palette.h"

#include "cmdlib.h"


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
// Res_ValidatePatchData
//
// Returns true if the raw patch_t data is valid.
//
bool Res_ValidatePatchData(const uint8_t* patch_data, uint32_t patch_size)
{
	if (patch_size > 8)
	{
		const int16_t width = LESHORT(*(int16_t*)(patch_data + 0));
		const int16_t height = LESHORT(*(int16_t*)(patch_data + 2));

		const uint32_t column_table_offset = 8;
		const uint32_t column_table_length = sizeof(int32_t) * width;

		if (width > 0 && height > 0 && patch_size >= column_table_offset + column_table_length)
		{
			const int32_t* column_offset = (const int32_t*)(patch_data + column_table_offset);
			const int32_t min_column_offset = column_table_offset + column_table_length;
			const int32_t max_column_offset = patch_size - 1;

			for (int i = 0; i < width; i++, column_offset++)
				if (*column_offset < min_column_offset || *column_offset > max_column_offset)
					return false;
			return true;
		}
	}
	return false;
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
