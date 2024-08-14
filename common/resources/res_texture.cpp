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

#include "odamex.h"
#include "m_fixed.h"

#include "tables.h"
#include "g_level.h"
#include "m_random.h"
#include "m_ostring.h"
#include "cmdlib.h"

#include "resources/res_texture.h"
#include "resources/res_main.h"
#include "resources/res_resourceloader.h"
#include "resources/res_identifier.h"

#include "i_system.h"


//
// Res_WarpTexture
//
// Alters the image in source_texture with a warping effect and saves the
// result in dest_texture.
//
static void Res_WarpTexture(Texture* dest_texture, const Texture* source_texture)
{
	int width = source_texture->mWidth, height = source_texture->mHeight;
	int height_bits = source_texture->mHeightBits;
	int width_mask = source_texture->mWidthMask, height_mask = source_texture->mHeightMask;

	// [SL] ZDoom 1.22 warping
	palindex_t temp_buffer[Texture::MAX_TEXTURE_HEIGHT];
	palindex_t* warped = dest_texture->mData;

	int timebase = level.time * 23;
	for (int x = width - 1; x >= 0; x--)
	{
		int yf = (finesine[(timebase + ((x + 17) << 7)) & FINEMASK] >> 13) & height_mask;
		const palindex_t* source = source_texture->mData + x;
		palindex_t* dest = warped + x;
		for (int yt = height; yt; yt--)
		{
			*dest = *(source + (yf << height_bits));
			dest += width;
			yf = (yf + 1) & height_mask;
		}
	}
	timebase = level.time * 32;
	for (int y = height - 1; y >= 0; y--)
	{
		int xf = (finesine[(timebase + (y << 7)) & FINEMASK] >> 13) & width_mask;
		const palindex_t* source = warped + (y << height_bits);
		palindex_t* dest = temp_buffer;
		for (int xt = width; xt; xt--)
		{
			*dest++ = *(source + xf);
			xf = (xf + 1) & width_mask;
		}
		memcpy(warped + (y << height_bits), temp_buffer, width);
	}
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
	mWidthMask = (1 << mWidthBits) - 1;
	mHeightMask = (1 << mHeightBits) - 1;
	mOffsetX = 0;
	mOffsetY = 0;
	mScaleX = FRACUNIT;
	mScaleY = FRACUNIT;
	mMaskColor = 0;
	mData = NULL;

	#if CLIENT_APP
	// mData follows the header in memory
	mData = (uint8_t*)this + calculateHeaderSize(width, height);
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
	#endif
	return size;
}


// ============================================================================
//
// TextureManager
//
// ============================================================================

TextureManager::TextureManager(ResourceManager* manager) :
	mResourceLoaderLookup(1024)
{
	const ResourceIdList pnames_lookup = buildPNamesLookup(manager, "PNAMES");
	addCompositeTextureResources(manager, pnames_lookup, "TEXTURE1");
	addCompositeTextureResources(manager, pnames_lookup, "TEXTURE2");
}


//
// TextureManager::~TextureManager
//
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
// TextureManager::addResources
//
// Discovers all raw texture resources and notifies ResourceManager
// 
void TextureManager::addResources(ResourceManager* manager)
{
	addMissingTexturePlaceholder(manager);
	addResourceToManagerByDir(manager, flats_directory_name);
	addResourceToManagerByDir(manager, patches_directory_name);
	addResourceToManagerByDir(manager, sprites_directory_name);
	addResourceToManagerByDir(manager, textures_directory_name);
	addResourceToManagerByDir(manager, graphics_directory_name);
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

		const uint32_t header_data_size = 32;
		uint8_t header_data[header_data_size];
		if (accessor->getResourceSize(raw_res_id) >= header_data_size)
		{
			accessor->loadResource(raw_res_id, header_data, header_data_size);
			if (Res_ValidatePngData(header_data, header_data_size))
				loader = new PngTextureLoader(accessor, raw_res_id);
			else if (dir == flats_directory_name)
				loader = new FlatTextureLoader(accessor, raw_res_id);
			else if (dir == patches_directory_name)
				loader = new PatchTextureLoader(accessor, raw_res_id);
			else if (dir == sprites_directory_name)
				loader = new PatchTextureLoader(accessor, raw_res_id);
			else if (dir == graphics_directory_name)
				loader = new PatchTextureLoader(accessor, raw_res_id);
		}

		if (loader != NULL)
		{
			const ResourceId res_id = manager->addResource(path, this, loader);

			// save the ResourceLoader pointers so they can be freed later
			mResourceLoaderLookup.insert(std::make_pair(res_id, loader));
		}
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
	const ResourceId res_id = Res_GetResourceId(lump_name, NS_GLOBAL);
	if (!Res_CheckResource(res_id))
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

		// From ChocolateDoom r_data.c:
		// Vanilla Doom does a linear search of the texures array
		// and stops at the first entry it finds.  If there are two
		// entries with the same name, the first one in the array
		// wins.
		// [SL] Only add the first instance of a texture name
		const ResourceId existing_res_id = manager->getResourceId(texture_lump_name, NS_NEWTEXTURES, true);
		if (Res_CheckResource(existing_res_id))
			continue;

		CompositeTextureDefinition tex_def = buildCompositeTextureDefinition(raw_def_data + tex_offset, pnames_lookup);
		ResourceLoader* loader = new CompositeTextureLoader(manager->getRawResourceAccessor(), tex_def);
		ResourcePath path = textures_directory_name + texture_lump_name;
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
	const ResourceId pnames_res_id = Res_GetResourceId(lump_name, NS_GLOBAL);
	if (!Res_CheckResource(pnames_res_id))
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

		ResourceId res_id = Res_GetResourceId(lump_name, NS_PATCHES);

		// killough 4/17/98:
		// Some wads use sprites as wall patches, so repeat check and
		// look for sprites this time, but only if there were no wall
		// patches found. This is the same as allowing for both, except
		// that wall patches always win over sprites, even when they
		// appear first in a wad. This is a kludgy solution to the wad
		// lump namespace problem.

		if (!Res_CheckResource(res_id))
			res_id = Res_GetResourceId(lump_name, NS_SPRITES);

		pnames_lookup.push_back(res_id);
	}

	delete [] pnames_raw_data;

	return pnames_lookup;
}


//
// TextureManager::addMissingTexturePlaceholder
//
// Creates a dummy texture that can be will be used when an
// expected texture is missing.
//
ResourceId TextureManager::addMissingTexturePlaceholder(ResourceManager* manager)
{
	const uint16_t width = 64;
	const uint16_t height = 64;
	const palindex_t color1 = 200;
	const palindex_t color2 = 231;

	palindex_t data[width * height];
	palindex_t* ptr = data;
	const uintptr_t checker_width = width / 2;
	
	for (int y = 0; y < height / 2; y++)
	{
		memset(ptr, color1, checker_width);
		ptr += checker_width;
		memset(ptr, color2, checker_width);
		ptr += checker_width;
	}
	for (int y = height / 2; y < height; y++)
	{
		memset(ptr, color2, checker_width);
		ptr += checker_width;
		memset(ptr, color1, checker_width);
		ptr += checker_width;
	}

	ResourceLoader* loader = new InMemoryTextureLoader(data, width, height);
	ResourcePath path = textures_directory_name + "-MISSING-";
	const ResourceId res_id = manager->addResource(path, this, loader);

	mResourceLoaderLookup.insert(std::make_pair(res_id, loader));

	return res_id;
}

// ============================================================================
//
// AnimatedTextureManager
//
// ============================================================================

void AnimatedTextureManager::clear()
{
	#if CLIENT_APP
	mTextureTranslation.clear();
	for (size_t i = 0; i < mWarpedTextures.size(); i++)
		delete [] (uint8_t*)mWarpedTextures[i].original_texture;
	mWarpedTextures.clear();
	#endif
}


//
// AnimatedTextureManager::copyTexture
//
void AnimatedTextureManager::copyTexture(Texture* destination_texture, const Texture* source_texture) const
{
	uint16_t width = source_texture->mWidth, height = source_texture->mHeight;
	destination_texture->init(width, height);
	memcpy(destination_texture->mData, source_texture->mData, sizeof(palindex_t) * width * height);
}


//
// AnimatedTextureManager::readAnimatedDefinitions
//
void AnimatedTextureManager::readAnimationDefinitions()
{
	#if CLIENT_APP
	clear();
	loadAnimationsFromAnimatedLump();		// Boom ANIMATED lump
	loadAnimationsFromAnimDefLump();		// Hexen/ZDoom ANIMDEFS lump
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
	const ResourceId res_id = Res_GetResourceId("ANIMATED", NS_GLOBAL);
	if (!Res_CheckResource(res_id))
		return;

	uint32_t lumplen = Res_GetResourceSize(res_id);
	if (lumplen == 0)
		return;

	const uint8_t* data = (uint8_t*)Res_LoadResource(res_id, PU_STATIC);

	for (const uint8_t* ptr = data; *ptr != 255; ptr += 23)
	{
		bool is_wall = *ptr & 1;
		const OString start_name = OStringToUpper((char*)(ptr + 10), 8);
		const OString end_name = OStringToUpper((char*)(ptr + 1), 8);
		TextureSearchOrdering search_ordering = is_wall ? WALL : FLOOR;

		const ResourceId start_res_id = Res_GetTextureResourceId(start_name, search_ordering, false);
		const ResourceId end_res_id = Res_GetTextureResourceId(end_name, search_ordering, false);

		uint32_t numframes = end_res_id - start_res_id + 1;
		if (!Res_CheckResource(start_res_id) || !Res_CheckResource(end_res_id))
		{
			DPrintf("Missing texture for cycle from %s to %s in ANIMATED lump\n", start_name.c_str(), end_name.c_str());
		} 
		else if (numframes < 2 || numframes > AnimatedTextureManager::anim_t::MAX_ANIM_FRAMES)
		{
			Printf(PRINT_HIGH, "Bad cycle from %s to %s in ANIMATED lump\n", start_name.c_str(), end_name.c_str());
		}
		else
		{
			anim_t anim;
			anim.basepic = start_res_id;
			anim.numframes = numframes; 
			anim.uniqueframes = false;
			anim.curframe = 0;
			int speed = LELONG(*(int*)(ptr + 19));
			anim.speedmin[0] = anim.speedmax[0] = speed;
			anim.countdown = speed - 1;

			mAnimDefs.push_back(anim);
		}
	}

	Res_ReleaseResource(res_id);
}



//
// AnimatedTextureManager::parseAnim
//
//
void AnimatedTextureManager::parseAnim(OScanner& os, TextureSearchOrdering search_ordering)
{
	bool missing_texture_warning = true;

	os.mustScan();
	if (os.compareTokenNoCase("optional"))
	{
		missing_texture_warning = false;
		os.mustScan();
	}
	
	const OString texture_name = OStringToUpper(os.getToken());
	ResourceId base_res_id = Res_GetTextureResourceId(texture_name, search_ordering, false);

	AnimatedTextureManager::anim_t sink_anim;
	AnimatedTextureManager::anim_t* anim = NULL;

	if (base_res_id == ResourceId::INVALID_ID)
	{
		// The base resource isn't present.
		// Still parse the definition but ignore it since we can't use it 
		anim = &sink_anim;
		if (missing_texture_warning)
			Printf(PRINT_HIGH, "Missing texture %s required by ANIMDEFS definition\n", texture_name.c_str());
	}
	else
	{
		for (size_t i = 0; i < mAnimDefs.size(); i++)
		{
			if (mAnimDefs[i].basepic == base_res_id)
				anim = &mAnimDefs[i];
		}
		if (!anim)
		{
			mAnimDefs.push_back(AnimatedTextureManager::anim_t());
			anim = &mAnimDefs.back();
		}
	}

	anim->basepic = base_res_id;
	anim->curframe = 0;
	anim->numframes = 0;
	anim->uniqueframes = true;
	memset(anim->speedmin, 1,
	       AnimatedTextureManager::anim_t::MAX_ANIM_FRAMES * sizeof(*anim->speedmin));
	memset(anim->speedmax, 1,
	       AnimatedTextureManager::anim_t::MAX_ANIM_FRAMES * sizeof(*anim->speedmax));

	while (os.scan())
	{
		if (!os.compareTokenNoCase("pic"))
		{
			os.unScan();
			break;
		}

		if (anim->numframes == AnimatedTextureManager::anim_t::MAX_ANIM_FRAMES)
		{
			os.error("Animation has too many frames");
		}

		byte duration_min = 1;
		byte duration_max = 1;

		os.mustScanInt();
		const int res_id_offset = os.getTokenInt() - 1;
		os.mustScan();
		if (os.compareTokenNoCase("tics"))
		{
			os.mustScanInt();
			duration_min = duration_max = clamp(os.getTokenInt(), 0, 255);
		}
		else if (os.compareTokenNoCase("rand"))
		{
			os.mustScanInt();
			duration_min = std::max(os.getTokenInt(), 0);
			os.mustScanInt();
			duration_max = std::min(os.getTokenInt(), 255);
		}
		else
		{
			os.error("Must specify a duration for animation frame");
		}

		anim->speedmin[anim->numframes] = duration_min;
		anim->speedmax[anim->numframes] = duration_max;
		anim->framepic[anim->numframes] = anim->basepic + res_id_offset;
		anim->numframes++;
	}

	if (anim->numframes < 2)
	{
		os.error("Animation needs at least 2 frames");
	}

	anim->countdown = anim->speedmin[0];
}


//
// AnimatedTextureManager::loadAnimationsFromAnimDefLump
//
// [RH] This uses a Hexen ANIMDEFS lump to define the animation sequences.
//
void AnimatedTextureManager::loadAnimationsFromAnimDefLump()
{
	try
	{
		const ResourceIdList res_ids = Res_GetAllResourceIds(ResourcePath("/GLOBAL/ANIMDEFS"));

		for (size_t i = 0; i < res_ids.size(); i++)
		{
			const char* name = Res_GetResourceName(res_ids[i]).c_str();
			const char* buffer = static_cast<const char*>(Res_LoadResource(res_ids[i], PU_STATIC));

			OScannerConfig config = {
			    name,  // lumpName
			    false, // semiComments
			    true,  // cComments
			};
			OScanner os = OScanner::openBuffer(config, buffer, buffer + Res_GetResourceSize(res_ids[i]));

			while (os.scan())
			{
				if (os.compareTokenNoCase("flat"))
				{
					parseAnim(os, FLOOR);
				}
				else if (os.compareTokenNoCase("texture"))
				{
					parseAnim(os, WALL);
				}
				else if (os.compareTokenNoCase("switch"))
				{
					// Don't support switchdef yet...
				}
				else if (os.compareTokenNoCase("warp"))
				{
					os.mustScan();
					TextureSearchOrdering search_ordering = FLOOR;
					if (os.compareTokenNoCase("texture"))
						search_ordering = WALL;
					else if (!os.compareTokenNoCase("flat"))
						os.error("Unknown error reading in ANIMDEFS");

					os.mustScan();
					const OString texture_name = OStringToUpper(os.getToken());
					addWarpedTexture(Res_GetTextureResourceId(texture_name, search_ordering, false));
				}
			}

			Res_ReleaseResource(res_ids[i]);
		}
	}
	catch (CRecoverableError&)
	{

	}
}


//
// AnimatedTextureManager::addWarpedTexture
//
void AnimatedTextureManager::addWarpedTexture(const ResourceId res_id)
{
	if (!Res_CheckResource(res_id))
		return;

	AnimatedTextureManager::warp_t warp;
	warp.working_texture = (Texture*)Res_CacheTexture(res_id, PU_STATIC);
	size_t size = Texture::calculateSize(warp.working_texture->mWidth, warp.working_texture->mHeight);
	warp.original_texture = (Texture*)(new uint8_t[size]);
	copyTexture(warp.original_texture, warp.working_texture);
	mWarpedTextures.push_back(warp);
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
		anim_t* anim = &mAnimDefs[i];
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

	// warp textures
	for (size_t i = 0; i < mWarpedTextures.size(); i++)
	{
		Res_WarpTexture(mWarpedTextures[i].working_texture, mWarpedTextures[i].original_texture);
	}
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
// If use_placeholder is true, this will return the resource ID of a placeholder texture
// if the requested texture cannot be found.
//
const ResourceId Res_GetTextureResourceId(const OString& name, TextureSearchOrdering ordering, bool use_placeholder)
{
	if (name.empty() || (ordering == WALL && name.size() == 1 && name.c_str()[0] == '-'))
		return ResourceId::INVALID_ID;

	ResourceNamespace ns = NS_GLOBAL;
	if (ordering == WALL)
		ns = NS_NEWTEXTURES;
	else if (ordering == FLOOR)
		ns = NS_FLATS;
	else if (ordering == PATCH)
		ns = NS_PATCHES;
	else if (ordering == SPRITE)
		ns = NS_SPRITES;
	else if (ordering == GRAPHICS)
		ns = NS_GRAPHICS;

	ResourceId res_id = Res_GetResourceId(name, ns);
	if (!Res_CheckResource(res_id))
	{
		DPrintf("Missing texture %s\n", name.c_str());
		if (use_placeholder)
			res_id = Res_GetResourceId("-MISSING-", NS_GLOBAL);
	}
	return res_id;
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
const Texture* Res_CacheTexture(ResourceId res_id, zoneTag_e tag)
{
	if (!Res_CheckResource(res_id))
		return static_cast<const Texture*>(NULL);
	return static_cast<const Texture*>(Res_LoadResource(res_id, tag));
}


//
// Res_CacheTexture
//
const Texture* Res_CacheTexture(const OString& lump_name, TextureSearchOrdering ordering, zoneTag_e tag)
{
	return Res_CacheTexture(Res_GetTextureResourceId(lump_name, ordering), tag);
}
