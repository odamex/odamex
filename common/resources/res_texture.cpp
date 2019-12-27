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
#include "resources/res_resourceloader.h"

#include "v_video.h"
#include "v_palette.h"

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

	fixed_t xfrac = 0;
	for (int xcount = destwidth; xcount > 0; xcount--)
	{
		int source_offset = (sx1 + (xfrac >> FRACBITS)) * source_texture->getHeight() + sy1;
		const byte* source = source_texture->getData() + source_offset;

		fixed_t yfrac = 0;
		for (int ycount = destheight; ycount > 0; ycount--)
		{
			*dest++ = source[yfrac >> FRACBITS];	
			yfrac += ystep;
		}

		dest += dest_texture->getHeight() - destheight;

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
// TODO: Use cache-blocking to optimize
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
	uint32_t size = calculateHeaderSize(width, height);
	#if CLIENT_APP
	size += calculateDataSize(width, height);
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
	mMasked = false;
	mMaskColor = 0;
	mData = NULL;

	#if CLIENT_APP
	// mData follows the header in memory
	mData = (uint8_t*)((uint8_t*)this + calculateHeaderSize(width, height));
	memset(mData, mMaskColor, calculateDataSize(width, height));
	#endif
}


// ============================================================================
//
// TextureManager
//
// ============================================================================

TextureManager::TextureManager(const ResourceContainerId& container_id, ResourceManager* manager) :
	ResourceContainer(container_id, manager),
	mResourceManager(manager),
	mResourceLoaderLookup(1024),
	mMaskColor(0),
	mMaskColorMap(NULL)
{
	addCompositeTextureResources(manager);

	addResourcesToManager(manager);

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

	delete [] mMaskColorMap;
	mMaskColorMap = NULL;

	for (ResourceLoaderLookupTable::iterator it = mResourceLoaderLookup.begin(); it != mResourceLoaderLookup.end(); ++it)
		delete it->second;
	mResourceLoaderLookup.clear();
}


//
// TextureManager::addResourcesToManager
//
// Discovers all raw texture resources and notifies ResourceManager
// 
void TextureManager::addResourcesToManager(ResourceManager* manager)
{
	addResourceToManagerByDir(manager, flats_directory_name);
	addResourceToManagerByDir(manager, patches_directory_name);
	addResourceToManagerByDir(manager, sprites_directory_name);
}


void TextureManager::addResourceToManagerByDir(ResourceManager* manager, const ResourcePath& dir)
{
	const RawResourceAccessor* accessor = manager->getRawResourceAccessor();

	const ResourcePathList paths = manager->listResourceDirectory(dir);

	for (ResourcePathList::const_iterator it = paths.begin(); it != paths.end(); ++it)
	{
		const ResourcePath& path = *it;
		const ResourceId raw_res_id = manager->getResourceId(path);

		ResourceLoader* loader = NULL;
		if (dir == flats_directory_name)
			loader = new FlatTextureLoader(accessor, raw_res_id);
		else if (dir == patches_directory_name)
			loader = new PatchTextureLoader(accessor, raw_res_id);
		else if (dir == sprites_directory_name)
			loader = new PatchTextureLoader(accessor, raw_res_id);

		const ResourceId res_id = manager->addResource(path, this, loader);

		// save the ResourceLoader pointers so they can be freed later
		mResourceLoaderLookup.insert(std::make_pair(res_id, loader));
	}
}


//
// TextureManager::getResourceSize
//
// Returns the size of the specified Texture resource.
//
uint32_t TextureManager::getResourceSize(const ResourceId res_id) const
{
	const ResourceLoader* loader = getResourceLoader(res_id);
	assert(loader != NULL);
	return loader->size();
}


//
// TextureManager::getResourceSize
//
// Loads the specified Texture resource.
//
uint32_t TextureManager::loadResource(void* data, const ResourceId res_id, uint32_t size) const
{
	if (!mMaskColorMap)
	{
		mMaskColorMap = new palindex_t[256];
		analyzePalette(mMaskColorMap, &mMaskColor);
	}

	const ResourceLoader* loader = getResourceLoader(res_id);
	assert(loader != NULL);
	loader->load(data);
	return size;
}


//
// TextureManager::getResourceLoader
//
const ResourceLoader* TextureManager::getResourceLoader(const ResourceId res_id) const
{
	ResourceLoaderLookupTable::const_iterator it = mResourceLoaderLookup.find(res_id);
	if (it != mResourceLoaderLookup.end())
		return it->second;
	return NULL;
}



//
// TextureManager::addCompositeTextureResources
//
// Parses the TEXTURE1 & TEXTURE2 lumps and adds the composite textures
// to ResourceManager as stand-alone resources.
//
void TextureManager::addCompositeTextureResources(ResourceManager* manager)
{
	const RawResourceAccessor* accessor = manager->getRawResourceAccessor();

	// TODO: the lump name "PNAMES" can be modified with DeHacked
	const ResourceIdList pnames_lookup = buildPNamesLookup("PNAMES");

	ResourceId res_ids[2];
	res_ids[0] = Res_GetResourceId("TEXTURE1", global_directory_name);
	res_ids[1] = Res_GetResourceId("TEXTURE2", global_directory_name);
	if (!Res_CheckResource(res_ids[0]))
		I_Error("\"TEXTURE1\" resource not found!");

	for (int n = 0; n < sizeof(res_ids) / sizeof(*res_ids); n++)	
	{
		if (res_ids[n] == ResourceId::INVALID_ID)
			continue;

		uint32_t res_size = Res_GetResourceSize(res_ids[n]);
		if (res_size < 4)		// not long enough to store definition_count
			return;

		const uint8_t* raw_def_data = (uint8_t*)Res_LoadResource(res_ids[n], PU_STATIC);

		int32_t definition_count = LELONG(*((int32_t*)(raw_def_data + 0)));
		for (int32_t i = 0; i < definition_count; i++)
		{
			int32_t def_offset = 4 * i + 4;
			if (res_size < (unsigned int)def_offset)
				break;

			// Read a texture definition, create a CompositeTextureDefinition,
			// and add a new CompositeTextureLoader to the list.
			int32_t tex_offset = LELONG(*((int32_t*)(raw_def_data + def_offset)));
			if (res_size < (uint32_t)tex_offset + 22)
				break;

			const char* str = (const char*)(raw_def_data + tex_offset + 0);
			const OString lump_name = OStringToUpper(str, 8);

			// From ChocolateDoom r_data.c:
			// Vanilla Doom does a linear search of the texures array
			// and stops at the first entry it finds.  If there are two
			// entries with the same name, the first one in the array
			// wins.
			//if (getByName(name) != NULL)
			//	continue;

			CompositeTextureDefinition tex_def = buildCompositeTextureDefinition(raw_def_data + tex_offset, pnames_lookup);

			ResourcePath path = textures_directory_name + lump_name;
			ResourceLoader* loader = new CompositeTextureLoader(manager->getRawResourceAccessor(), tex_def);
			const ResourceId res_id = manager->addResource(path, this, loader);

			// save the ResourceLoader pointers so they can be freed later
			mResourceLoaderLookup.insert(std::make_pair(res_id, loader));
		}

		Res_ReleaseResource(res_ids[n]);
	}
}


CompositeTextureDefinition TextureManager::buildCompositeTextureDefinition(const uint8_t* data, const ResourceIdList& pnames_lookup) const
{
	CompositeTextureDefinition tex_def;

	tex_def.mScaleX = *(uint8_t*)(data + 10);
	tex_def.mScaleY = *(uint8_t*)(data + 11);
	tex_def.mWidth = LESHORT(*((int16_t*)(data + 12)));
	tex_def.mHeight = LESHORT(*((int16_t*)(data + 14)));

	int16_t patch_count = LESHORT(*((int16_t*)(data + 20)));
	for (int16_t j = 0; j < patch_count; j++)
	{
		const uint8_t* patch_data = data + 22 + 10 * j;

		tex_def.mPatchDefs.push_back(CompositeTextureDefinition::PatchDef());
		CompositeTextureDefinition::PatchDef& patch_def = tex_def.mPatchDefs.back();

		patch_def.mOriginX = LESHORT(*((int16_t*)(patch_data + 0)));
		patch_def.mOriginY = LESHORT(*((int16_t*)(patch_data + 2)));

		// TODO: handle invalid pnames indices
		int16_t patch_num = LESHORT(*((int16_t*)(patch_data + 4)));
		if ((size_t)patch_num < pnames_lookup.size())
			patch_def.mResId = pnames_lookup[patch_num];
	}

	return tex_def;
}


//
// TextureManager::buildPNamesLookup
//
// Parses the PNAMES resource and creates a lookup table mapping entries in
// the PNAMES resource lump to ResourceIds.
//
const ResourceIdList TextureManager::buildPNamesLookup(const OString& lump_name) const
{
	// Read the PNAMES lump and store the ResourceId of each patch
	// listed in the lump in the pnames_lookup array.
	const ResourceId pnames_res_id = Res_GetResourceId(lump_name, global_directory_name);
	if (pnames_res_id == ResourceId::INVALID_ID)
		I_Error("Res_InitTextures: PNAMES lump not found");

	uint32_t pnames_size = Res_GetResourceSize(pnames_res_id);
	if (pnames_size < 4)			// not long enough to store pnames_count
		I_Error("Res_InitTextures: invalid PNAMES lump");

	const uint8_t* pnames_raw_data = (uint8_t*)Res_LoadResource(pnames_res_id, PU_STATIC);

	int32_t pnames_count = LELONG(*((int32_t*)(pnames_raw_data + 0)));
	if ((uint32_t)pnames_count * 8 + 4 != pnames_size)
		I_Error("Res_InitTextures: invalid PNAMES lump");

	ResourceIdList pnames_lookup;
	pnames_lookup.reserve(pnames_count);

	for (int32_t i = 0; i < pnames_count; i++)
	{
		const char* str = (const char*)(pnames_raw_data + 4 + 8 * i);
		const OString lump_name = OStringToUpper(str, 8);

		ResourceId res_id = Res_GetResourceId(lump_name, patches_directory_name);

		// killough 4/17/98:
		// Some wads use sprites as wall patches, so repeat check and
		// look for sprites this time, but only if there were no wall
		// patches found. This is the same as allowing for both, except
		// that wall patches always win over sprites, even when they
		// appear first in a wad. This is a kludgy solution to the wad
		// lump namespace problem.

		if (res_id == ResourceId::INVALID_ID)
			res_id = Res_GetResourceId(lump_name, sprites_directory_name);

		pnames_lookup.push_back(res_id);
	}

	Res_ReleaseResource(pnames_res_id);

	return pnames_lookup;
}


//
// TextureManager::analyzePalette
//
// Examines all of the colors of the palette and determines the two closest colors.
// One of those two colors are selected to be used to represent transparency.
//
// A palette map is generated to remap the transparent color to the next closest color.
///
void TextureManager::analyzePalette(palindex_t* colormap, palindex_t* maskcolor) const
{
	const argb_t* palette_colors = V_GetGamePalette()->basecolors;
	palindex_t color1, color2;

	V_ClosestColors(palette_colors, color1, color2);
	*maskcolor = color1;

	for (int i = 0; i < 256; i++)
		colormap[i] = i;
	colormap[color1] = color2;
}


//
// TextureManager::readAnimDefLump
//
// [RH] This uses a Hexen ANIMDEFS lump to define the animation sequences.
//
void TextureManager::readAnimDefLump()
{
	/*
	const ResourceIdList res_ids = Res_GetAllResourceIds(ResourcePath("/GLOBAL/ANIMDEFS"));
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
	const ResourceId res_id = Res_GetResourceId("ANIMATED", global_directory_name);
	if (!Res_CheckResource(res_id))
		return;

	uint32_t lumplen = Res_GetResourceSize(res_id);
	if (lumplen == 0)
		return;

	const uint8_t* data = (uint8_t*)Res_LoadResource(res_id, PU_STATIC);

	for (const uint8_t* ptr = data; *ptr != 255; ptr += 23)
	{
		bool is_flat = (*(ptr + 0) == 0);
		const ResourcePath& path = is_flat ? flats_directory_name : textures_directory_name;
		const OString start_name((char*)(ptr + 10), 8);
		const OString end_name((char*)(ptr + 1), 8);

		const ResourceId start_res_id = Res_GetResourceId(start_name, path);
		const ResourceId end_res_id = Res_GetResourceId(end_name, path);

		if (!Res_CheckResource(start_res_id) || !Res_CheckResource(end_res_id))
			continue;

		anim_t anim;
		anim.basepic = start_res_id;
		anim.numframes = end_res_id - start_res_id + 1;

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
}


//
// TextureManager::updateAnimatedTextures
//
// TextureIds ticking the animated textures and cyles the Textures within an
// animation definition.
//
void TextureManager::updateAnimatedTextures()
{
	if (!clientside)
		return;

	/*
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
// Res_GetTextureResourceId
//
// Returns the ResourceId for the texture that matches the supplied name.
//
// NOTE: Vanilla Doom considered any sidedef whose texture starts with "-"
// (such as "-FLAT-") to imply no texture. ZDoom changes this behavior to 
// allow texture names that start with "-" if they contain more than 1 character.
//
const ResourceId Res_GetTextureResourceId(const OString& name, TextureSearchOrdering ordering)
{
	if (ordering == WALL && name.size() == 1 && name.c_str()[0] == '-')
		return ResourceId::INVALID_ID;

	ResourcePathList paths;
	if (ordering == WALL)
	{
		paths.push_back(textures_directory_name);
		paths.push_back(flats_directory_name);
		paths.push_back(patches_directory_name);
		paths.push_back(sprites_directory_name);
		paths.push_back(global_directory_name);
	}
	else if (ordering == FLOOR)
	{
		paths.push_back(flats_directory_name);
		paths.push_back(textures_directory_name);
		paths.push_back(patches_directory_name);
		paths.push_back(sprites_directory_name);
		paths.push_back(global_directory_name);
	}
	else if (ordering == PATCH)
	{
		paths.push_back(patches_directory_name);
		paths.push_back(global_directory_name);
		paths.push_back(textures_directory_name);
		paths.push_back(flats_directory_name);
		paths.push_back(sprites_directory_name);
	}
	else if (ordering == SPRITE)
	{
		paths.push_back(sprites_directory_name);
		paths.push_back(patches_directory_name);
		paths.push_back(textures_directory_name);
		paths.push_back(flats_directory_name);
		paths.push_back(global_directory_name);
	}

	for (ResourcePathList::const_iterator it = paths.begin(); it != paths.end(); ++it)
	{
		const ResourcePath& path = *it;
		const ResourceId res_id = Res_GetResourceId(name, path);
		if (res_id != ResourceId::INVALID_ID)
			return res_id;
	}

	return ResourceId::INVALID_ID;
}


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(ResourceId res_id, int tag)
{
	if (res_id != ResourceId::INVALID_ID)
	{
		return static_cast<Texture*>(Res_LoadResource(res_id, tag));
	}
	else
	{
		// TODO: return invalid texture
		return static_cast<Texture*>(NULL);
	}
}


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(const OString& lump_name, const ResourcePath& directory, int tag)
{
	if (directory == textures_directory_name)
		return Res_CacheTexture(Res_GetTextureResourceId(lump_name, WALL), tag);
	if (directory == flats_directory_name)
		return Res_CacheTexture(Res_GetTextureResourceId(lump_name, FLOOR), tag);
	if (directory == patches_directory_name)
		return Res_CacheTexture(Res_GetTextureResourceId(lump_name, PATCH), tag);
	if (directory == sprites_directory_name)
		return Res_CacheTexture(Res_GetTextureResourceId(lump_name, SPRITE), tag);
	Printf(PRINT_HIGH, "Res_CacheTexture of unknown type %s", directory.c_str());
	return NULL;
}


//
// Res_CachePatch
//
// Place-holder function for resolving a patch name to a resource ID and
// caching and returning that resource's data.
//
struct patch_s;
typedef patch_s patch_t;

const patch_t* Res_CachePatch(const OString& name, int tag)
{
	ResourceId res_id = Res_GetTextureResourceId(name, PATCH);
	return (patch_t*)Res_LoadResource(res_id, tag);
}


VERSION_CONTROL (res_texture_cpp, "$Id$")
