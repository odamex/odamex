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
#include "w_wad.h"

#include "v_video.h"

#include "s_sound.h"

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
				if (sprite->spriteframes[i].lump[r] == -1)
					I_Error("Sprite {}, rotation {} has no lump", i, r);

				patch_t* patch = W_CachePatch(sprite->spriteframes[i].lump[r]);
				sprite->spriteframes[i].width[r] = patch->width()<<FRACBITS;
				sprite->spriteframes[i].height[r] = patch->height()<<FRACBITS;
				sprite->spriteframes[i].offset[r] = patch->leftoffset()<<FRACBITS;
				sprite->spriteframes[i].topoffset[r] = patch->topoffset()<<FRACBITS;
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
// Returns false if the lump name does not decode to a usable frame and
// rotation. Only tolerant callers see that, it is otherwise fatal.
//
bool R_InstallSpriteLump(int lump, unsigned frame, unsigned rot, bool flipped,
                         bool tolerant)
{
	unsigned rotation;

	if (rot <= 9)
		rotation = rot;
	else
		rotation = (rot >= 17) ? rot - 7 : 17;

	if (frame >= MAX_SPRITE_FRAMES || rotation > 16)
	{
		if (!tolerant)
			I_FatalError("R_InstallSpriteLump: Bad frame characters in lump {}: {}", lump, W_GetOLumpName(lump));

		return false;
	}

	if (static_cast<int>(frame) > maxframe)
		maxframe = frame;

	if (rotation == 0)
	{
		// the lump should be used for all rotations
        // false=0, true=1, but array initialised to -1
        // allows doom to have a "no value set yet" boolean value!
		for (int r = 14; r >= 0; r -= 2)
		{
			if (sprtemp[frame].lump[r] == -1)
			{
				sprtemp[frame].lump[r] = lump;
				sprtemp[frame].flip[r] = flipped;
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
		}

		return true;
	}

	rotation = (rotation <= 8 ? (rotation - 1) * 2 : (rotation - 9) * 2 + 1);

	if (sprtemp[frame].lump[rotation] == -1)
	{
		// the lump is only used for one rotation
		sprtemp[frame].lump[rotation] = lump;
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
			if (strict && sprtemp[frame].lump[0] == -1)
				return SPRITE_NO_PATCHES;

			continue;
		}

		// must have all 16 frames
		for (int rotation = 0; rotation < 16; rotation += 2)
		{
			if (sprtemp[frame].lump[rotation + 1] == -1)
			{
				sprtemp[frame].lump[rotation + 1] = sprtemp[frame].lump[rotation];
				sprtemp[frame].flip[rotation + 1] = sprtemp[frame].flip[rotation];
				sprtemp[frame].width[rotation + 1] = SPRITE_NEEDS_INFO;
			}

			if (sprtemp[frame].lump[rotation] == -1)
			{
				sprtemp[frame].lump[rotation] = sprtemp[frame].lump[rotation + 1];
				sprtemp[frame].flip[rotation] = sprtemp[frame].flip[rotation + 1];
				sprtemp[frame].width[rotation] = SPRITE_NEEDS_INFO;
			}
		}

		for (const int rotationlump : sprtemp[frame].lump)
		{
			if (rotationlump == -1)
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
// Resets sprtemp/maxframe and fills them in from every lump between first and
// last whose name matches the given four character sprite name.
//
// Returns false if a matching lump had a name that does not decode to a frame
// and rotation, which only tolerant callers see.
//
bool R_ScanSpriteLumps(const char* sprite, int first, int last, bool tolerant)
{
	memset (sprtemp, -1, sizeof(sprtemp));

	for (spriteframe_t& frame : sprtemp)
		frame.rotate = false;

	maxframe = -1;
	const int intname = NameToInt(sprite);
	bool wellformed = true;

	// scan the lumps,
	//	filling in the frames for whatever is found
	for (int l = last; l >= first; l--)
	{
		if (NameToInt(lumpinfo[l].name.c_str()) == intname && lumpinfo[l].size > 0)
		{
			if (!R_InstallSpriteLump (l,
								 lumpinfo[l].name[SPR_FRAME_CHAR] - 'A', // denis - fixme - security
								 lumpinfo[l].name[SPR_ROTATION_CHAR] - '0',
								 false, tolerant))
				wellformed = false;

			if (lumpinfo[l].name[SPR_FLIPFRAME_CHAR])
				if (!R_InstallSpriteLump (l,
								 lumpinfo[l].name[SPR_FLIPFRAME_CHAR] - 'A',
								 lumpinfo[l].name[SPR_FLIPROTATION_CHAR] - '0',
								 true, tolerant))
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

	// scan all the lump names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (int i = 0; i < numsprites; i++)
	{
		R_ScanSpriteLumps(namelist[i]->sprite, firstspritelump, lastspritelump, false);
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
	const int startlump = W_CheckNumForName("S_START");
	const int endlump = W_CheckNumForName("S_END");

	if (startlump == -1 || endlump == -1 || endlump <= startlump)
		return "";

	for (auto it = sprnames.begin(); it != sprnames.end(); ++it)
	{
		if (!R_ScanSpriteLumps(it->second.data(), startlump + 1, endlump - 1, true))
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

