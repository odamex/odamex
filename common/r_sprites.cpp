// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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
#include "w_wad.h"

#include "v_video.h"

#include "s_sound.h"

#define SPRITE_NEEDS_INFO	MAXINT

//
// INITIALIZATION FUNCTIONS
//
spritedef_t* sprites;
int numsprites;

spriteframe_t sprtemp[MAX_SPRITE_FRAMES];
int maxframe;

void R_CacheSprite(spritedef_t *sprite)
{
	DPrintf ("cache sprite %s\n",
		sprite - sprites < NUMSPRITES ? sprnames[sprite - sprites] : "");
	for (int i = 0; i < sprite->numframes; i++)
	{
		for (int r = 0; r < 16; r++)
		{
			if (sprite->spriteframes[i].width[r] == SPRITE_NEEDS_INFO)
			{
				if (sprite->spriteframes[i].lump[r] == -1)
					I_Error ("Sprite %d, rotation %d has no lump", i, r);
				patch_t* patch = W_CachePatch(sprite->spriteframes[i].lump[r]);
				sprite->spriteframes[i].width[r] = patch->width()<<FRACBITS;
				sprite->spriteframes[i].offset[r] = patch->leftoffset()<<FRACBITS;
				sprite->spriteframes[i].topoffset[r] = patch->topoffset()<<FRACBITS;
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
static void R_InstallSpriteLump(int lump, unsigned frame, unsigned rot, BOOL flipped)
{
	unsigned rotation;

	if (rot <= 9)
		rotation = rot;
	else
		rotation = (rot >= 17) ? rot - 7 : 17;
	
	if (frame >= MAX_SPRITE_FRAMES || rotation > 16)
		I_FatalError("R_InstallSpriteLump: Bad frame characters in lump %i", lump);

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
				sprtemp[frame].lump[r] = static_cast<short>(lump);
				sprtemp[frame].flip[r] = static_cast<byte>(flipped);
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
		}
		
		return;
	}

	rotation = (rotation <= 8 ? (rotation - 1) * 2 : (rotation - 9) * 2 + 1);
	
	if (sprtemp[frame].lump[rotation] == -1)
	{
		// the lump is only used for one rotation
		sprtemp[frame].lump[rotation] = static_cast<short>(lump);
		sprtemp[frame].flip[rotation] = static_cast<byte>(flipped);
		sprtemp[frame].rotate = true;
		sprtemp[frame].width[rotation] = SPRITE_NEEDS_INFO;
	}
}


// [RH] Seperated out of R_InitSpriteDefs()
static void R_InstallSprite(const char *name, int num)
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
			I_FatalError ("R_InstallSprite: No patches found for %s frame %c", sprname, frame+'A');
			break;

		  case 0:
			// only the first rotation is needed
			break;

		  case 1:
			// must have all 16 frames
			{
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

		  	for (int rotation = 0; rotation < 16; ++rotation)
		  	{
				if (sprtemp[frame].lump[rotation] == -1)
				{
					I_FatalError("R_InstallSprite: Sprite %s frame %c is missing rotations",
						sprname, frame + 'A');
				}
		  	}
			}
			break;
		}
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[num].numframes = maxframe;
	sprites[num].spriteframes = (spriteframe_t *)
		Z_Malloc (maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
	memcpy (sprites[num].spriteframes, sprtemp, maxframe * sizeof(spriteframe_t));
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

	// scan all the lump names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (int i = 0; i < numsprites; i++)
	{
		memset (sprtemp, -1, sizeof(sprtemp));

                for (int i = 0; i < MAX_SPRITE_FRAMES; i++) {
                    sprtemp[i].rotate = false;
                }

		maxframe = -1;
		const int intname = *(int *)namelist[i];

		// scan the lumps,
		//	filling in the frames for whatever is found
		for (int l = lastspritelump; l >= firstspritelump; l--)
		{
			if (*(int *)lumpinfo[l].name == intname)
			{
				R_InstallSpriteLump (l,
									 lumpinfo[l].name[4] - 'A', // denis - fixme - security
									 lumpinfo[l].name[5] - '0',
									 false);

				if (lumpinfo[l].name[6])
					R_InstallSpriteLump (l,
									 lumpinfo[l].name[6] - 'A',
									 lumpinfo[l].name[7] - '0',
									 true);
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

