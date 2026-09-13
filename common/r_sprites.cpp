// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//		Loading sprites, skins.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "r_sprites.h"

#include "m_alloc.h"

#include "i_system.h"
#include "z_zone.h"

#include "v_video.h"

#include "s_sound.h"

#include "resources/res_main.h"
#include "resources/res_texture.h"

#define SPRITE_NEEDS_INFO	limits::MAXINT

//
// INITIALIZATION FUNCTIONS
//
OHashTable<int32_t, spritedef_t> sprites;
int numsprites;

spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
int maxframe;

// [CMB] This function assumes that sprnames has the correct sprites in order
void R_CacheSprite(const spritedef_t *sprite)
{
	auto it = sprnames.find(sprite->spritenum);
	DPrintFmt("cache sprite {}\n",
		it != sprnames.end() ? it->second : "");
	for (int i = 0; i < sprite->numframes; i++)
	{
		for (int r = 0; r < 16; r++)
		{
			if (sprite->spriteframes[i].width[r] == SPRITE_NEEDS_INFO)
			{
				if (!Res_CheckResource(sprite->spriteframes[i].resource[r]))
					I_Error("Sprite {}, rotation {} has no resource", i, r);

				const ResourceId res_id = sprite->spriteframes[i].resource[r];
				const Texture* texture = Res_CacheTexture(res_id, PU_CACHE);

				sprite->spriteframes[i].height[r] = texture->mHeight << FRACBITS;
				sprite->spriteframes[i].width[r] = texture->mWidth << FRACBITS;
				sprite->spriteframes[i].offset[r] = texture->mOffsetX << FRACBITS;
				sprite->spriteframes[i].topoffset[r] = texture->mOffsetY << FRACBITS;
			}
		}
	}
}

namespace
{

constexpr size_t SPR_FRAME_CHAR = 4;
constexpr size_t SPR_ROTATION_CHAR = 5;
constexpr size_t SPR_FLIPFRAME_CHAR = 6;
constexpr size_t SPR_FLIPROTATION_CHAR = 7;

//
// NameToInt
//
// Packs the four character sprite name into an int so names can be compared in
// one go.
//
int NameToInt(const char* name)
{
	int packed = 0;
	memcpy(&packed, name, sizeof(packed));
	return packed;
}

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
// [RH] Removed checks for coexistance of rotation 0 with other
//		rotations and made it look more like BOOM's version.
//
bool R_InstallSpriteLump(const ResourceId res_id, unsigned frame, unsigned rot,
                         bool flipped, bool tolerant)
{
	unsigned rotation;

	if (rot <= 9)
		rotation = rot;
	else
		rotation = (rot >= 17) ? rot - 7 : 17;

	if (frame >= MAX_SPRITE_FRAMES || rotation > 16)
	{
		if (!tolerant)
			I_FatalError("R_InstallSpriteLump: Bad frame characters in resource {}",
			             Res_GetResourceName(res_id));

		return false;
	}

	if (static_cast<int>(frame) > maxframe)
		maxframe = frame;

	if (rotation == 0)
	{
		// the resource should be used for all rotations
        // false=0, true=1, but array initialised to -1
        // allows doom to have a "no value set yet" boolean value!
		for (int r = 14; r >= 0; r -= 2)
		{
			if (!Res_CheckResource(sprtemp[frame].resource[r]))
			{
				sprtemp[frame].resource[r] = res_id;
				sprtemp[frame].flip[r] = flipped;
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
		}

		return true;
	}

	rotation = (rotation <= 8 ? (rotation - 1) * 2 : (rotation - 9) * 2 + 1);

	if (!Res_CheckResource(sprtemp[frame].resource[rotation]))
	{
		// the resource is only used for one rotation
		sprtemp[frame].resource[rotation] = res_id;
		sprtemp[frame].flip[rotation] = flipped;
		sprtemp[frame].rotate = true;
		sprtemp[frame].width[rotation] = SPRITE_NEEDS_INFO;
	}

	return true;
}


enum spritecheck_t
{
	SPRITE_COMPLETE,
	SPRITE_NO_PATCHES,
	SPRITE_MISSING_ROTATIONS
};

//
// R_CheckSpriteFrames
//
// Applies the frame completeness rules to the first numframes entries of
// sprtemp, filling in mirrored rotations as it goes.
//
// When strict is set, a non-rotating frame that never had a lump installed is
// reported too. R_InstallSprite lets those thru -- its only an issue when
// displaying the sprite -- but for judging an unofficial IWAD means every
// sprite needs one.
//
// Returns SPRITE_COMPLETE if every frame is usable, otherwise it will return
// the failure reason.
//
spritecheck_t R_CheckSpriteFrames(int numframes, bool strict, int& badframe)
{
	for (int frame = 0; frame < numframes; frame++)
	{
		badframe = frame;

		if (!sprtemp[frame].rotate)
		{
			// only the first rotation is needed
			if (strict && !Res_CheckResource(sprtemp[frame].resource[0]))
				return SPRITE_NO_PATCHES;

			continue;
		}

		// must have all 16 frames
		for (int rotation = 0; rotation < 16; rotation += 2)
		{
			if (!Res_CheckResource(sprtemp[frame].resource[rotation + 1]))
			{
				sprtemp[frame].resource[rotation + 1] = sprtemp[frame].resource[rotation];
				sprtemp[frame].flip[rotation + 1] = sprtemp[frame].flip[rotation];
				sprtemp[frame].width[rotation + 1] = SPRITE_NEEDS_INFO;
			}

			if (!Res_CheckResource(sprtemp[frame].resource[rotation]))
			{
				sprtemp[frame].resource[rotation] = sprtemp[frame].resource[rotation + 1];
				sprtemp[frame].flip[rotation] = sprtemp[frame].flip[rotation + 1];
				sprtemp[frame].width[rotation] = SPRITE_NEEDS_INFO;
			}
		}

		for (const ResourceId rotation_res_id : sprtemp[frame].resource)
		{
			if (!Res_CheckResource(rotation_res_id))
				return SPRITE_MISSING_ROTATIONS;
		}
	}

	return SPRITE_COMPLETE;
}


// [RH] Seperated out of R_InitSpriteDefs()
void R_InstallSprite(const char *name, int32_t num)
{
	if (maxframe == -1)
	{
		sprites[num].numframes = 0;
		return;
	}

	char sprname[5];
	strncpy (sprname, name, 4);
	sprname[4] = 0;

	maxframe++;

	int badframe = 0;
	switch (R_CheckSpriteFrames(maxframe, false, badframe))
	{
	  case SPRITE_NO_PATCHES:
		// no rotations were found for that frame at all
		I_FatalError ("R_InstallSprite: No patches found for {} frame {:c}", sprname, badframe+'A');
		break;

	  case SPRITE_MISSING_ROTATIONS:
		I_FatalError("R_InstallSprite: Sprite {} frame {:c} is missing rotations",
			sprname, badframe + 'A');
		break;

	  default:
		break;
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[num].numframes = maxframe;
	sprites[num].spriteframes = Z_Malloc<spriteframe_t>(maxframe, PU_STATIC);
	memcpy (sprites[num].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
	sprites[num].spritenum = num;
}


//
// R_ScanSpriteLumps
//
// Resets sprtemp/maxframe and fills them in from every sprite resource whose
// name matches the given four character sprite name.
//
// Returns false if a matching lump had a name that does not decode to a frame
// and rotation, which only tolerant callers see.
//
bool R_ScanSpriteLumps(const ResourcePathList& sprite_paths, const char* sprite,
                       bool tolerant)
{
	for (int f = 0; f < MAX_SPRITE_FRAMES; f++)
	{
		sprtemp[f].rotate = false;
		for (int r = 0; r < 16; r++)
		{
			sprtemp[f].resource[r] = ResourceId::INVALID_ID;
			sprtemp[f].width[r] = -1;
		}
	}

	maxframe = -1;
	const int intname = *reinterpret_cast<const int*>(sprite);
	bool wellformed = true;

	// scan the sprite resources,
	//	filling in the frames for whatever is found
	for (int l = sprite_paths.size() - 1; l >= 0; l--)
	{
		const OString& resource_name = sprite_paths[l].last();

		// Archive and directory resources may carry long file names;
		// only classic 8-character lump names can encode sprite frame
		// and rotation characters.
		if (resource_name.size() > 8 || resource_name.find(".") != std::string::npos)
			continue;

		const char* resource_name_array = resource_name.c_str();
		if (*reinterpret_cast<const int*>(resource_name_array) != intname)
			continue;

		const ResourceId res_id = Res_GetResourceId(resource_name, NS_SPRITES);
		unsigned frame = resource_name_array[4] - 'A';
		unsigned rotation = resource_name_array[5] - '0';

		if (!R_InstallSpriteLump(res_id, frame, rotation, false, tolerant))
			wellformed = false;

		// can frame can be flipped?
		if (resource_name.size() > 6 && resource_name_array[6])
		{
			frame = resource_name_array[6] - 'A';
			rotation = resource_name_array[7] - '0';

			if (!R_InstallSpriteLump(res_id, frame, rotation, true, tolerant))
				wellformed = false;
		}
	}

	return wellformed;
}

//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//	(4 chars exactly) to be used.
// Builds the sprite rotation matrices to account
//	for horizontally flipped sprites.
// Will report an error if the lumps are inconsistent.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//	a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//	letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
void R_InitSpriteDefs(std::vector<spriteinfo_t*>& namelist)
{
	numsprites = namelist.size();

	const ResourcePathList sprite_paths = Res_ListResourceDirectory(sprites_directory_name);

	// scan all the resource names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (int i = 0; i < numsprites; i++)
	{
		R_ScanSpriteLumps(sprite_paths, namelist[i]->sprite, false);
		R_InstallSprite(namelist[i]->sprite, namelist[i]->spritenum);
	}
}

} // namespace

//
// R_FindIncompleteSprite
//
// Checks every sprite the game knows about against the lumps currently loaded,
// looking for frames that are missing patches or rotations. 
//
// Never fatal, so its safe to call when judging whether a WAD will run
// standalone (aka a standalone IWAD).
//
// Returns the first offender, or an empty string if they all check out.
//
std::string R_FindIncompleteSprite()
{
	const ResourcePathList sprite_paths = Res_ListResourceDirectory(sprites_directory_name);

	if (sprite_paths.empty())
		return "";

	for (auto it = sprnames.begin(); it != sprnames.end(); ++it)
	{
		if (!R_ScanSpriteLumps(sprite_paths, it->second.data(), true))
			return fmt::format("sprite {} has a malformed lump name", it->second);

		if (maxframe == -1)
			continue; // sprite is absent entirely, which is fine

		int badframe = 0;
		if (R_CheckSpriteFrames(maxframe + 1, true, badframe) != SPRITE_COMPLETE)
			return fmt::format("sprite {} frame {:c} is incomplete", it->second,
			                   badframe + 'A');
	}

	return "";
}

//
// GAME FUNCTIONS
//
int				MaxVisSprites;
vissprite_t 	*vissprites;
vissprite_t		*firstvissprite;
vissprite_t		*lastvissprite;



//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(std::vector<spriteinfo_t*>& sprites)
{
	MaxVisSprites = 128;	// [RH] This is the initial default value. It grows as needed.

	M_Free(vissprites);

	firstvissprite = vissprites = static_cast<vissprite_t*>(M_Malloc(MaxVisSprites * sizeof(vissprite_t)));
	lastvissprite = &vissprites[MaxVisSprites];

	R_InitSpriteDefs (sprites);
}

VERSION_CONTROL (r_sprites_cpp, "$Id$")
