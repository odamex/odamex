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


typedef OHashTable<ResourceId, ResourceId> ResourceIdMap;
static ResourceIdMap animated_texture_map;



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

	const palindex_t* source_buffer = source_texture->mData;

	int widthbits = source_texture->mWidthBits;
	int width = (1 << widthbits);
	int widthmask = width - 1; 
	int heightbits = source_texture->getHeightBits();
	int height = (1 << heightbits);
	int heightmask = height - 1;

	const int time_offset = level.time * 50;

	for (int x = 0; x < width; x++)
	{
		palindex_t* dest = dest_texture->mData + (x << heightbits);

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

	const palindex_t* source_buffer = source_texture->mData;
	palindex_t* warped_buffer = dest_texture->mData;
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
	mMaskColor = 0;
	mData = NULL;

	#if CLIENT_APP
	// mData follows the header in memory
	mData = (uint8_t*)((uint8_t*)this + calculateHeaderSize(width, height));
	memset(mData, mMaskColor, calculateDataSize(width, height));
	#endif
}


//
// Texture::calculateSize
//
uint32_t Texture::calculateSize(int width, int height)
{
	uint32_t size = calculateHeaderSize(width, height);
	#if CLIENT_APP
	size += calculateDataSize(width, height);
	size += 16;
	#endif
	return size;
}



// ============================================================================
//
// TextureManager
//
// ============================================================================

TextureManager::TextureManager(const ResourceContainerId& container_id, ResourceManager* manager) :
	ResourceContainer(container_id, manager),
	mResourceLoaderLookup(1024)
{
	const ResourceIdList pnames_lookup = buildPNamesLookup(manager, "PNAMES");
	addCompositeTextureResources(manager, pnames_lookup, "TEXTURE1");
	addCompositeTextureResources(manager, pnames_lookup, "TEXTURE2");

	addResourcesToManager(manager);
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


//
// TextureManager::addResourceToManagerByDir
//
// Iterates over a list of resource names in a given resource directory
// and creates a TextureLoader instance to handle loading the resource.
//
void TextureManager::addResourceToManagerByDir(ResourceManager* manager, const ResourcePath& dir)
{
	const RawResourceAccessor* accessor = manager->getRawResourceAccessor();
	const ResourcePathList paths = manager->listResourceDirectory(dir);

	for (ResourcePathList::const_iterator it = paths.begin(); it != paths.end(); ++it)
	{
		const ResourcePath& path = *it;
		const ResourceId raw_res_id = manager->getResourceId(path);

		// skip if we're adding a duplicate
		if (mResourceLoaderLookup.find(raw_res_id) != mResourceLoaderLookup.end())
			continue;

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
void TextureManager::addCompositeTextureResources(ResourceManager* manager, const ResourceIdList& pnames_lookup, const OString& lump_name)
{
	const ResourceId res_id = Res_GetResourceId(lump_name, global_directory_name);
	if (res_id == ResourceId::INVALID_ID)
	{
		if (lump_name == "TEXTURE1")
			I_Error("Res_InitTextures: TEXTURE1 lump not found");
		return;
	}

	const RawResourceAccessor* accessor = manager->getRawResourceAccessor();
	uint32_t res_size = accessor->getResourceSize(res_id);
	uint8_t* raw_def_data = new uint8_t[res_size];
	accessor->loadResource(res_id, raw_def_data, res_size);

	if (res_size < 4)		// not long enough to store definition_count
		return;

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
		const OString texture_lump_name = OStringToUpper(str, 8);
		ResourcePath path = textures_directory_name + texture_lump_name;

		// From ChocolateDoom r_data.c:
		// Vanilla Doom does a linear search of the texures array
		// and stops at the first entry it finds.  If there are two
		// entries with the same name, the first one in the array
		// wins.
		// [SL] Only add the first instance of a texture name
		if (manager->getResourceId(path) != ResourceId::INVALID_ID)
			continue;

		CompositeTextureDefinition tex_def = buildCompositeTextureDefinition(raw_def_data + tex_offset, pnames_lookup);
		ResourceLoader* loader = new CompositeTextureLoader(manager->getRawResourceAccessor(), tex_def);
		const ResourceId res_id = manager->addResource(path, this, loader);

		// save the ResourceLoader pointers so they can be freed later
		mResourceLoaderLookup.insert(std::make_pair(res_id, loader));
	}

	delete [] raw_def_data;
}


//
// TextureManager::buildCompositeTextureDefinition
//
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
const ResourceIdList TextureManager::buildPNamesLookup(ResourceManager* manager, const OString& lump_name) const
{
	// Read the PNAMES lump and store the ResourceId of each patch
	// listed in the lump in the pnames_lookup array.
	const ResourceId pnames_res_id = Res_GetResourceId(lump_name, global_directory_name);
	if (pnames_res_id == ResourceId::INVALID_ID)
		I_Error("Res_InitTextures: PNAMES lump not found");

	uint32_t pnames_size = Res_GetResourceSize(pnames_res_id);
	if (pnames_size < 4)			// not long enough to store pnames_count
		I_Error("Res_InitTextures: invalid PNAMES lump");

	const RawResourceAccessor* accessor = manager->getRawResourceAccessor();
	uint8_t* pnames_raw_data = new uint8_t[pnames_size];
	accessor->loadResource(pnames_res_id, pnames_raw_data, pnames_size);

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

	delete [] pnames_raw_data;

	return pnames_lookup;
}




// ============================================================================
//
// AnimatedTextureManager
//
// ============================================================================

AnimatedTextureManager::AnimatedTextureManager()
{
}


//
// AnimatedTextureManager::readAnimatedDefinitions
//
void AnimatedTextureManager::readAnimationDefinitions()
{
	#if CLIENT_APP
	mTextureTranslation.clear();
	loadAnimationsFromAnimatedLump();
	#endif
}


//
// AnimatedTextureManager::getResourceId
//
// Returns a resource ID for the current animation frame
//
const ResourceId AnimatedTextureManager::getResourceId(const ResourceId res_id) const
{
	ResourceIdMap::const_iterator it = mTextureTranslation.find(res_id);
	if (it != mTextureTranslation.end())
		return it->second;
	return res_id;
}


//
// AnimatedTextureManager::loadAnimationsFromAnimatedLump
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
void AnimatedTextureManager::loadAnimationsFromAnimatedLump()
{
	const ResourceId res_id = Res_GetResourceId("/GLOBAL/ANIMATED");
	if (!Res_CheckResource(res_id))
		return;

	uint32_t lumplen = Res_GetResourceSize(res_id);
	if (lumplen == 0)
		return;

	const uint8_t* data = (uint8_t*)Res_LoadResource(res_id, PU_STATIC);

	for (const uint8_t* ptr = data; *ptr != 255; ptr += 23)
	{
		bool is_flat = (*(ptr + 0) == 0);
		const OString start_name((char*)(ptr + 10), 8);
		const OString end_name((char*)(ptr + 1), 8);
		TextureSearchOrdering search_ordering = is_flat ? FLOOR : WALL;

		const ResourceId start_res_id = Res_GetTextureResourceId(start_name, search_ordering);
		const ResourceId end_res_id = Res_GetTextureResourceId(end_name, search_ordering);

		if (!Res_CheckResource(start_res_id) || !Res_CheckResource(end_res_id))
			continue;

		AnimatedTextureManager::anim_t anim;
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
// AnimatedTextureManager::loadAnimationsFromAnimDefLump
//
// [RH] This uses a Hexen ANIMDEFS lump to define the animation sequences.
//
void AnimatedTextureManager::loadAnimationsFromAnimDefLump()
{
	#if 0
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

					int width = 1 << warp.original_texture->mWidthBits;
					int height = 1 << warp.original_texture->mHeightBits;

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
	#endif	// if 0
}



//
// AnimatedTextureManager::updateAnimatedTextures
//
// TextureIds ticking the animated textures and cyles the Textures within an
// animation definition.
//
void AnimatedTextureManager::updateAnimatedTextures()
{
	if (!clientside)
		return;

	// cycle the animdef textures
	for (size_t i = 0; i < mAnimDefs.size(); i++)
	{
		AnimatedTextureManager::anim_t* anim = &mAnimDefs[i];
		if (--anim->countdown == 0)
		{
			anim->curframe = (anim->numframes) ? (anim->curframe + 1) % anim->numframes : 0;
			int speedframe = (anim->uniqueframes) ? anim->curframe : 0;

			if (anim->speedmin[speedframe] == anim->speedmax[speedframe])
				anim->countdown = anim->speedmin[speedframe];
			else
				anim->countdown = M_Random() % (anim->speedmax[speedframe] - anim->speedmin[speedframe]) + anim->speedmin[speedframe];
		}

		if (anim->uniqueframes)
		{
			const ResourceId res_id = anim->framepic[anim->curframe];
			for (int j = 0; j < anim->numframes; j++)
				mTextureTranslation[anim->framepic[j]] = res_id;
		}
		else
		{
			for (uint32_t j = (uint32_t)anim->basepic; j < (uint32_t)anim->basepic + anim->numframes; j++)
			{
				mTextureTranslation[j] = (ResourceId)(anim->basepic + (anim->curframe + j) % anim->numframes);
			}
		}
	}

	/*
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
	if (ordering == WALL && name.c_str()[0] == '-' && name.size() == 1)
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


static AnimatedTextureManager animated_texture_manager;

//
// Res_ReadAnimationDefinition
//
void Res_ReadAnimationDefinitions()
{
	animated_texture_manager.readAnimationDefinitions();
}


//
// Res_UpdateTextureAnimations
//
void Res_UpdateTextureAnimations()
{
	animated_texture_manager.updateAnimatedTextures();
}


//
// Res_GetAnimatedTextureResourceId
//
const ResourceId Res_GetAnimatedTextureResourceId(const ResourceId res_id)
{
	return animated_texture_manager.getResourceId(res_id);
}


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(ResourceId res_id, int tag)
{
	if (res_id == ResourceId::INVALID_ID)
		return static_cast<const Texture*>(NULL);
	return static_cast<const Texture*>(Res_LoadResource(res_id, tag));
}


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(const OString& lump_name, TextureSearchOrdering ordering, int tag)
{
	return Res_CacheTexture(Res_GetTextureResourceId(lump_name, ordering), tag);
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
	return NULL;
}


VERSION_CONTROL (res_texture_cpp, "$Id$")
