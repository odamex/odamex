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

#include "resources/res_texture.h"
#include "resources/res_main.h"
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

Texture::Texture()
{
	init(0, 0);
}


uint32_t Texture::calculateSize(int width, int height)
{
	uint32_t size = sizeof(Texture);
	#if CLIENT_APP
	size += width * height						// mData
		+ width * height						// mMask
		// TODO: DELETE THIS!
		+ width * height + width * 4;						// mCols
	#endif
	return size;
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
	mData = mMask = NULL;

	#if CLIENT_APP
	// mData follows the header in memory
	mData = (byte*)((byte*)this + sizeof(*this));
	// mMask follows mData
	mMask = (byte*)(mData) + sizeof(byte) * width * height;

	// TODO: DELETE THIS
	mCols = (byte*)(mMask) + sizeof(byte) * width * height + 4 * width;
	#endif
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
	const ResourceIdList res_ids = Res_GetAllResourceIds(ResourcePath("GLOBAL/ANIMDEFS"));
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
					//warp.warped_texture = initTexture(data, width, height);
					//warp.warped_texture = Texture::createTexture(width, height);

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

	const uint8_t* lumpdata = (uint8_t*)Res_LoadResource(res_id, PU_STATIC);

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

	Res_ReleaseResource(res_id);
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
}


//
// TextureManager::registerTextureResources
//
// Creates a TextureLoader instance for each texture resource in the provided
// list of ResourceIds.
//
void TextureManager::registerTextureResources(ResourceManager* manager)
{
	/*
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
			// mTextureLoaders.push_back(loader);
			// TODO: save this ResourceId somewhere
			const ResourceId res_id = manager->addResource(path, this); 
		}
	}
	*/
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
const Texture* TextureManager::getTexture(const ResourceId res_id)
{
	/*
	const Texture* texture = NULL;
	if (lump_id < mTextures.size())
	{
		texture = mTextures[lump_id];
		if (texture)
			mTextures[lump_id] = texture;

		// TODO: set mTextureIdMap[lump_id] to not found texture if null
	}
	return texture;
	*/
	return NULL;
}










//
// Res_GetTextureResourceId
//
// Returns the ResourceId for the texture that matches the supplied name.
//
// TODO: USE THIS LOGIC TO INSTANTIATE A ResourceLoader INSTANCE
//
const ResourceId Res_GetTextureResourceId(const ResourcePath& res_path)
{
	const OString& directory = res_path.first();
	const OString& name = res_path.last();
	if (directory == textures_directory_name && name.size() > 0 && name.c_str()[0] == '-')
		//return ResourceManager::NO_TEXTURE_RESOURCE_ID;
		return ResourceId::INVALID_ID;

	ResourceId res_id = ResourceId::INVALID_ID;
	//ResourceId res_id = ResourceManager::NOT_FOUND_TEXTURE_RESOURCE_ID;

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

	return res_id;
}




VERSION_CONTROL (res_texture_cpp, "$Id$")
