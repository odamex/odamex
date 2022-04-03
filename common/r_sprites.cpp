// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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

#include"r_sprites.h"

#include "m_alloc.h"

#include "i_system.h"
#include "z_zone.h"

#include "v_video.h"

#include "s_sound.h"
#include "resources/res_texture.h"

#define SPRITE_NEEDS_INFO	MAXINT

//
// INITIALIZATION FUNCTIONS
//
spritedef_t* sprites;
int numsprites;

spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
int maxframe;

void R_CacheSprite(spritedef_t* sprite)
{
	DPrintf("cache sprite %s\n",
	        sprite - sprites < NUMSPRITES ? sprnames[sprite - sprites] : "");

	for (int i = 0; i < sprite->numframes; i++)
	{
		for (int r = 0; r < 8; r++)
		{
			if (sprite->spriteframes[i].width[r] == SPRITE_NEEDS_INFO)
			{
				if (sprite->spriteframes[i].resource[r] == ResourceId::INVALID_ID)
					I_Error("Sprite %d, rotation %d has no lump", i, r);

				const ResourceId res_id = sprite->spriteframes[i].resource[r];
				const Texture* texture = Res_CacheTexture(res_id, PU_CACHE);

				sprite->spriteframes[i].width[r] = texture->mWidth << FRACBITS;
				sprite->spriteframes[i].offset[r] = texture->mOffsetX << FRACBITS;
				sprite->spriteframes[i].topoffset[r] = texture->mOffsetY << FRACBITS;
			}
		}
	}
}

//
// R_InstallSpriteLump
// Local function for R_InitSprites.
//
// [RH] Removed checks for coexistance of rotation 0 with other
//		rotations and made it look more like BOOM's version.
//
static void R_InstallSpriteLump(const ResourceId res_id, uint32_t frame, uint32_t rot, bool flipped)
{
	if (frame >= MAX_SPRITE_FRAMES || rot > 8)
		I_FatalError ("R_InstallSpriteLump: Bad frame characters in resource ID %i", (int)res_id);

	if (static_cast<int>(frame) > maxframe)
		maxframe = frame;

	if (rot == 0)
	{
		// the resource should be used for all rotations
        // false=0, true=1, but array initialised to -1
        // allows doom to have a "no value set yet" boolean value!
		for (int r = 7; r >= 0; r--)
		{
			if (sprtemp[frame].resource[r] == ResourceId::INVALID_ID)
			{
				sprtemp[frame].resource[r] = res_id;
				sprtemp[frame].flip[r] = (byte)flipped;
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
		}
		
		return;
	}
	else if (sprtemp[frame].resource[--rot] == ResourceId::INVALID_ID)
	{
		// the lump is only used for one rotation
		sprtemp[frame].resource[rot] = res_id;
		sprtemp[frame].flip[rot] = (byte)flipped;
		sprtemp[frame].rotate = true;
		sprtemp[frame].width[rot] = SPRITE_NEEDS_INFO;
	}
}


// [RH] Seperated out of R_InitSpriteDefs()
static void R_InstallSprite(const char* name, int num)
{
	char sprname[5];
	int frame;

	if (maxframe == -1)
	{
		sprites[num].numframes = 0;
		return;
	}

	strncpy(sprname, name, 4);
	sprname[4] = 0;

	maxframe++;

	for (frame = 0; frame < maxframe; frame++)
	{
		switch ((int)sprtemp[frame].rotate)
		{
		case -1:
			// no rotations were found for that frame at all
			I_FatalError("R_InstallSprite: No patches found for %s frame %c", sprname,
			             frame + 'A');
			break;

		case 0:
			// only the first rotation is needed
			break;

		case 1:
			// must have all 8 frames
			{
				for (int rotation = 0; rotation < 8; rotation++)
				{
					if (sprtemp[frame].resource[rotation] == ResourceId::INVALID_ID)
						I_FatalError(
						    "R_InstallSprite: Sprite %s frame %c is missing rotations",
						    sprname, frame + 'A');
				}
			}
			break;
		}
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[num].numframes = maxframe;
	sprites[num].spriteframes =
	    (spriteframe_t*)Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy(sprites[num].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
}


//
// R_InitSpriteDefs
// Pass a null terminated list of sprite names
//	(4 chars exactly) to be used.
// Builds the sprite rotation matrices to account
//	for horizontally flipped sprites.
// Will report an error if the lumps are inconsistant.
// Only called at startup.
//
// Sprite lump names are 4 characters for the actor,
//	a letter for the frame, and a number for the rotation.
// A sprite that is flippable will have an additional
//	letter/number appended.
// The rotation character can be 0 to signify no rotations.
//
static void R_InitSpriteDefs(const char **namelist)
{
	// count the number of sprite names
	for (numsprites = 0; namelist[numsprites]; numsprites++)
		;

	if (!numsprites)
		return;

	sprites = (spritedef_t *)Z_Malloc(numsprites * sizeof(*sprites), PU_STATIC, NULL);

	const ResourcePathList sprite_paths = Res_ListResourceDirectory(sprites_directory_name);

	// scan all the lump names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (int i = 0; i < numsprites; i++)
	{
		memset(sprtemp, -1, sizeof(sprtemp));
		maxframe = -1;

		for (int frame = 0; frame < MAX_SPRITE_FRAMES; frame++)
			for (int r = 0; r < 8; r++)
				sprtemp[frame].resource[r] = ResourceId::INVALID_ID;

		// convert the first 4 chars of sprite name to an int for fast comparisons
		uint32_t actor_id = *(uint32_t*)namelist[i];

		// scan the lumps,
		//	filling in the frames for whatever is found
		for (int l = sprite_paths.size() - 1; l >= 0; l--) 
		{
			const char* res_name = sprite_paths[l].last().c_str();
			uint32_t frame_actor_id = *(uint32_t*)res_name;
			if (frame_actor_id == actor_id)
			{
				const ResourceId res_id = Res_GetResourceId(sprite_paths[l]);
				uint32_t frame = res_name[4] - 'A';
				uint32_t rotation = res_name[5] - '0';
				R_InstallSpriteLump(res_id, frame, rotation, false);

				// can frame can be flipped?
				if (res_name[6] != '\0')
				{
			    	frame = res_name[6] - 'A';
					rotation = res_name[7] - '0';
					R_InstallSpriteLump(res_id, frame, rotation, true);
				}
			}
		}

		R_InstallSprite(namelist[i], i);
	}
}



//
// GAME FUNCTIONS
//
int				MaxVisSprites;
vissprite_t 	*vissprites;
vissprite_t		*lastvissprite;



//
// R_InitSprites
// Called at program start.
//
void R_InitSprites(const char **namelist)
{
	MaxVisSprites = 128;	// [RH] This is the initial default value. It grows as needed.

	M_Free(vissprites);

	vissprites = (vissprite_t *)Malloc(MaxVisSprites * sizeof(vissprite_t));
	lastvissprite = &vissprites[MaxVisSprites];

	R_InitSpriteDefs (namelist);
}

VERSION_CONTROL (r_sprites_cpp, "$Id$")

