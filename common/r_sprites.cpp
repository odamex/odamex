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
static void R_InstallSpriteLump(const ResourceId res_id, unsigned frame, unsigned rot, bool flipped)
{
	unsigned rotation;

	if (rot <= 9)
		rotation = rot;
	else
		rotation = (rot >= 17) ? rot - 7 : 17;

	if (frame >= MAX_SPRITE_FRAMES || rotation > 16)
		I_FatalError("R_InstallSpriteLump: Bad frame characters in resource {}",
		             Res_GetResourceName(res_id));

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

		return;
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
}


// [RH] Seperated out of R_InitSpriteDefs()
static void R_InstallSprite(const char *name, int32_t num)
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

	for (int frame = 0 ; frame < maxframe ; frame++)
	{
		switch (static_cast<int>(sprtemp[frame].rotate))
		{
		  case -1:
			// no rotations were found for that frame at all
			I_FatalError ("R_InstallSprite: No patches found for {} frame {:c}", sprname, frame+'A');
			break;

		  case 0:
			// only the first rotation is needed
			break;

		  case 1:
			// must have all 16 frames
			{
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

		  	for (int rotation = 0; rotation < 16; ++rotation)
		  	{
				if (!Res_CheckResource(sprtemp[frame].resource[rotation]))
				{
					I_FatalError("R_InstallSprite: Sprite {} frame {:c} is missing rotations",
						sprname, frame + 'A');
				}
		  	}
			}
			break;
		}
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[num].numframes = maxframe;
	sprites[num].spriteframes = Z_Malloc<spriteframe_t>(maxframe, PU_STATIC);
	memcpy (sprites[num].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
	sprites[num].spritenum = num;
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
static void R_InitSpriteDefs(std::vector<spriteinfo_t*>& namelist)
{
	numsprites = namelist.size();

	const ResourcePathList sprite_paths = Res_ListResourceDirectory(sprites_directory_name);

	// scan all the resource names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (int i = 0; i < numsprites; i++)
	{
		memset (sprtemp, -1, sizeof(sprtemp));

		for (int f = 0; f < MAX_SPRITE_FRAMES; f++)
		{
			sprtemp[f].rotate = false;
			for (int r = 0; r < 16; r++)
				sprtemp[f].resource[r] = ResourceId::INVALID_ID;
		}

		maxframe = -1;
		const int intname = *reinterpret_cast<const int*>(namelist[i]->sprite);

		// scan the sprite resources,
		//	filling in the frames for whatever is found
		for (int l = sprite_paths.size() - 1; l >= 0; l--)
		{
			const OString& resource_name = sprite_paths[l].last();
			const char* resource_name_array = resource_name.c_str();
			if (*reinterpret_cast<const int*>(resource_name_array) == intname)
			{
				const ResourceId res_id = Res_GetResourceId(resource_name, NS_SPRITES);
				unsigned frame = resource_name_array[4] - 'A';
				unsigned rotation = resource_name_array[5] - '0';
				R_InstallSpriteLump(res_id, frame, rotation, false);

				// can frame can be flipped?
				if (resource_name.size() > 6 && resource_name_array[6])
				{
					frame = resource_name_array[6] - 'A';
					rotation = resource_name_array[7] - '0';
					R_InstallSpriteLump(res_id, frame, rotation, true);
				}
			}
		}

		R_InstallSprite(namelist[i]->sprite, namelist[i]->spritenum);
	}
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
