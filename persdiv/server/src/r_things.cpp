// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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
//		Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "m_alloc.h"

#include "doomdef.h"
#include "m_swap.h"
#include "m_argv.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"

#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"

#include "doomstat.h"

#include "v_video.h"

#include "cmdlib.h"
#include "s_sound.h"

extern fixed_t FocalLengthX, FocalLengthY;


#define MINZ							(FRACUNIT*4)
#define BASEYCENTER 					(100)

#define MAX_SPRITE_FRAMES 29		// [RH] Macro-ized as in BOOM.
#define SPRITE_NEEDS_INFO	MAXINT

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//	and range check thing_t sprites patches
spritedef_t*	sprites;
int				numsprites;

spriteframe_t	sprtemp[MAX_SPRITE_FRAMES];
int 			maxframe;
static const char*			spritename;

// [RH] skin globals
playerskin_t	*skins;
size_t			numskins;

void R_CacheSprite (spritedef_t *sprite)
{
	int i, r;
	patch_t *patch;

	DPrintf ("cache sprite %s\n",
		sprite - sprites < NUMSPRITES ? sprnames[sprite - sprites] : "");
	for (i = 0; i < sprite->numframes; i++)
	{
		for (r = 0; r < 8; r++)
		{
			if (sprite->spriteframes[i].width[r] == SPRITE_NEEDS_INFO)
			{
				if (sprite->spriteframes[i].lump[r] == -1)
					I_Error ("Sprite %d, rotation %d has no lump", i, r);
				patch = W_CachePatch (sprite->spriteframes[i].lump[r]);
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
static void R_InstallSpriteLump (int lump, unsigned frame, unsigned rotation, BOOL flipped)
{
	if (frame >= MAX_SPRITE_FRAMES || rotation > 8)
		I_FatalError ("R_InstallSpriteLump: Bad frame characters in lump %i", lump);

	if ((int)frame > maxframe)
		maxframe = frame;

	if (rotation == 0)
	{
		// the lump should be used for all rotations
        // false=0, true=1, but array initialised to -1
        // allows doom to have a "no value set yet" boolean value!
		int r;

		for (r = 7; r >= 0; r--)
			if (sprtemp[frame].lump[r] == -1)
			{
				sprtemp[frame].lump[r] = (short)(lump);
				sprtemp[frame].flip[r] = (byte)flipped;
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
	}
	else if (sprtemp[frame].lump[--rotation] == -1)
	{
		// the lump is only used for one rotation
		sprtemp[frame].lump[rotation] = (short)(lump);
		sprtemp[frame].flip[rotation] = (byte)flipped;
		sprtemp[frame].rotate = true;
		sprtemp[frame].width[rotation] = SPRITE_NEEDS_INFO;
	}
}


// [RH] Seperated out of R_InitSpriteDefs()
static void R_InstallSprite (const char *name, int num)
{
	char sprname[5];
	int frame;

	if (maxframe == -1)
	{
		sprites[num].numframes = 0;
		return;
	}

	strncpy (sprname, name, 4);
	sprname[4] = 0;

	maxframe++;

	for (frame = 0 ; frame < maxframe ; frame++)
	{
		switch ((int)sprtemp[frame].rotate)
		{
		  case -1:
			// no rotations were found for that frame at all
			I_FatalError ("R_InstallSprite: No patches found for %s frame %c", sprname, frame+'A');
			break;

		  case 0:
			// only the first rotation is needed
			break;

		  case 1:
			// must have all 8 frames
			{
				int rotation;

				for (rotation = 0; rotation < 8; rotation++)
					if (sprtemp[frame].lump[rotation] == -1)
						I_FatalError ("R_InstallSprite: Sprite %s frame %c is missing rotations",
									  sprname, frame+'A');
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
void R_InitSpriteDefs (const char **namelist)
{
	int i;
	int l;
	int intname;
	int start;
	int end;
	int realsprites;

	// count the number of sprite names
	for (numsprites = 0; namelist[numsprites]; numsprites++)
		;
	// [RH] include skins in the count
	realsprites = numsprites;
	numsprites += numskins - 1;

	if (!numsprites)
		return;

	sprites = (spritedef_t *)Z_Malloc (numsprites * sizeof(*sprites), PU_STATIC, NULL);

	start = firstspritelump - 1;
	end = lastspritelump + 1;

	// scan all the lump names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (i = 0; i < realsprites; i++)
	{
		spritename = namelist[i];
		memset (sprtemp, -1, sizeof(sprtemp));

		maxframe = -1;
		intname = *(int *)namelist[i];

		// scan the lumps,
		//	filling in the frames for whatever is found
		for (l = lastspritelump; l >= firstspritelump; l--)
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

		R_InstallSprite (namelist[i], i);
	}
}

// [RH]
// R_InitSkins
// Reads in everything applicable to a skin. The skins should have already
// been counted and had their identifiers assigned to namespaces.
//
static const char *skinsoundnames[8][2] = {
	{ "dsplpain",	NULL },
	{ "dspldeth",	NULL },
	{ "dspdiehi",	"xdeath1" },
	{ "dsoof",		"land1" },
	{ "dsnoway",	"grunt1" },
	{ "dsslop",		"gibbed" },
	{ "dspunch",	"fist" },
	{ "dsjump",		"jump1" }
};

static char facenames[8][8] = {
	"xxxTR", "xxxTL", "xxxOUCH", "xxxEVL", "xxxKILL", "xxxGOD0",
	"xxxST", { 'x','x','x','D','E','A','D','0' }
};
static const int facelens[8] = {
	5, 5, 7, 6, 7, 7, 5, 8
};

void R_InitSkins (void)
{
	char sndname[128];
	int sndlumps[8];
	char key[10];
	int intname;
	size_t i;
	int j, k, base;
	int stop;
	char *def;

	key[9] = 0;

	for (i = 1; i < numskins; i++)
	{
		for (j = 0; j < 8; j++)
			sndlumps[j] = -1;
		base = W_CheckNumForName ("S_SKIN", skins[i].namespc);
		// The player sprite has 23 frames. This means that the S_SKIN
		// marker needs a minimum of 23 lumps after it (probably more).
		if (base + 23 >= (int)numlumps || base == -1)
			continue;
		def = (char *)W_CacheLumpNum (base, PU_CACHE);
		intname = 0;

		// Data is stored as "key = data".
		while ( (def = COM_Parse (def)) )
		{
			strncpy (key, com_token, 9);
			def = COM_Parse (def);
			if (com_token[0] != '=')
			{
				Printf (PRINT_HIGH, "Bad format for skin %d: %s %s", i, key, com_token);
				break;
			}
			def = COM_Parse (def);
			if (!stricmp (key, "name")) {
				strncpy (skins[i].name, com_token, 16);
			} else if (!stricmp (key, "sprite")) {
				for (j = 3; j >= 0; j--)
					com_token[j] = toupper (com_token[j]);
				intname = *((int *)com_token);
			} else if (!stricmp (key, "face")) {
				for (j = 2; j >= 0; j--)
					skins[i].face[j] = toupper (com_token[j]);
			} else {
				for (j = 0; j < 8; j++) {
					if (!stricmp (key, skinsoundnames[j][0])) {
						// Can't use W_CheckNumForName because skin sounds
						// haven't been assigned a namespace yet.
						for (k = base + 1; k < (int)numlumps &&
										   lumpinfo[k].handle == lumpinfo[base].handle; k++) {
							if (!strnicmp (com_token, lumpinfo[k].name, 8)) {
								//W_SetLumpNamespace (k, skins[i].namespc);
								sndlumps[j] = k;
								break;
							}
						}
						if (sndlumps[j] == -1) {
							// Replacement not found, try finding it in the global namespace
							sndlumps[j] = W_CheckNumForName (com_token);
						}
						break;
					}
				}
				//if (j == 8)
				//	Printf (PRINT_HIGH, "Funny info for skin %i: %s = %s\n", i, key, com_token);
			}
		}

		if (skins[i].name[0] == 0)
			sprintf (skins[i].name, "skin%d", (unsigned)i);

		// Register any sounds this skin provides
		for (j = 0; j < 8; j++) {
			if (sndlumps[j] != -1) {
				if (j > 1) {
					sprintf (sndname, "player/%s/%s", skins[i].name, skinsoundnames[j][1]);
					S_AddSoundLump (sndname, sndlumps[j]);
				} else if (j == 1) {
					int r;

					for (r = 1; r <= 4; r++) {
						sprintf (sndname, "player/%s/death%d", skins[i].name, r);
						S_AddSoundLump (sndname, sndlumps[j]);
					}
				} else {	// j == 0
					int l, r;

					for (l =  1; l <= 4; l++)
						for (r = 1; r <= 2; r++) {
							sprintf (sndname, "player/%s/pain%d_%d", skins[i].name, l*25, r);
							S_AddSoundLump (sndname, sndlumps[j]);
						}
				}
			}
		}

		// Now collect the sprite frames for this skin. If the sprite name was not
		// specified, use whatever immediately follows the specifier lump.
		if (intname == 0) {
			intname = *(int *)(lumpinfo[base+1].name);
			for (stop = base + 2; stop < (int)numlumps &&
								  lumpinfo[stop].handle == lumpinfo[base].handle &&
								  *(int *)lumpinfo[stop].name == intname; stop++)
				;
		} else {
			stop = numlumps;
		}

		memset (sprtemp, -1, sizeof(sprtemp));
		maxframe = -1;

		for (k = base + 1;
			 k < stop && lumpinfo[k].handle == lumpinfo[base].handle;
			 k++) {
			if (*(int *)lumpinfo[k].name == intname)
			{
				R_InstallSpriteLump (k,
									 lumpinfo[k].name[4] - 'A', // denis - fixme - security
									 lumpinfo[k].name[5] - '0',
									 false);

				if (lumpinfo[k].name[6])
					R_InstallSpriteLump (k,
									 lumpinfo[k].name[6] - 'A',
									 lumpinfo[k].name[7] - '0',
									 true);

				//W_SetLumpNamespace (k, skins[i].namespc);
			}
		}
		R_InstallSprite ((char *)&intname, (skins[i].sprite = (spritenum_t)(numsprites - numskins + i)));

		// Now go back and check for face graphics (if necessary)
		if (skins[i].face[0] == 0 || skins[i].face[1] == 0 || skins[i].face[2] == 0) {
			// No face name specified, so this skin doesn't replace it
			skins[i].face[0] = 0;
		} else {
			// Need to go through and find all face graphics for the skin
			// and assign them to the skin's namespace.
			for (j = 0; j < 8; j++)
				strncpy (facenames[j], skins[i].face, 3);

			for (k = base + 1;
				 k < (int)numlumps && lumpinfo[k].handle == lumpinfo[base].handle;
				 k++) {
				for (j = 0; j < 8; j++)
					if (!strncmp (facenames[j], lumpinfo[k].name, facelens[j])) {
						//W_SetLumpNamespace (k, skins[i].namespc);
						break;
					}
			}
		}
	}
	// Grrk. May have changed sound table. Fix it.
	if (numskins > 1)
		S_HashSounds ();
}

// [RH] Find a skin by name
int R_FindSkin (const char *name)
{
	int i;

	for (i = 0; i < (int)numskins; i++)
		if (!strnicmp (skins[i].name, name, 16))
			return i;

	return 0;
}

// [RH] List the names of all installed skins
BEGIN_COMMAND (skins)
{
	int i;

	for (i = 0; i < (int)numskins; i++)
		Printf (PRINT_HIGH, "% 3d %s\n", i, skins[i].name);
}
END_COMMAND (skins)

//
// GAME FUNCTIONS
//
int				MaxVisSprites;
vissprite_t 	*vissprites;
vissprite_t		*vissprite_p;
vissprite_t		*lastvissprite;
int 			newvissprite;



//
// R_InitSprites
// Called at program start.
//
void R_InitSprites (const char **namelist)
{
	unsigned i;

	numskins = 0; // [Toke - skins] Reset skin count

	MaxVisSprites = 128;	// [RH] This is the initial default value. It grows as needed.

    M_Free(vissprites);

	vissprites = (vissprite_t *)Malloc (MaxVisSprites * sizeof(vissprite_t));
	lastvissprite = &vissprites[MaxVisSprites];

	// [RH] Count the number of skins, rename each S_SKIN?? identifier
	//		to just S_SKIN, and assign it a unique namespace.
	for (i = 0; i < numlumps; i++)
	{
		if (!strncmp (lumpinfo[i].name, "S_SKIN", 6))
		{
			numskins++;
			lumpinfo[i].name[6] = lumpinfo[i].name[7] = 0;
			//W_SetLumpNamespace (i, ns_skinbase + numskins);
		}
	}

	// [RH] We always have a default "base" skin.
	numskins++;

	// [RH] Do some preliminary setup
	skins = (playerskin_t *)Z_Malloc (sizeof(*skins) * numskins, PU_STATIC, 0);
	memset (skins, 0, sizeof(*skins) * numskins);
	for (i = 1; i < numskins; i++)
	{
		skins[i].namespc = i + ns_skinbase;
	}

	R_InitSpriteDefs (namelist);
	R_InitSkins ();		// [RH] Finish loading skin data

	// [RH] Set up base skin
	strcpy (skins[0].name, "Base");
	skins[0].face[0] = 'S';
	skins[0].face[1] = 'T';
	skins[0].face[2] = 'F';
	skins[0].sprite = SPR_PLAY;
	skins[0].namespc = ns_global;
}

VERSION_CONTROL (r_things_cpp, "$Id$")

