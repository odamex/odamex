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

#include "m_alloc.h"

#include "doomdef.h"
#include "m_swap.h"
#include "m_argv.h"

#include "i_system.h"
#include "z_zone.h"
#include "w_wad.h"

#include "r_local.h"
#include "p_local.h"

#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"

#include "doomstat.h"

#include "v_video.h"

#include "cmdlib.h"
#include "s_sound.h"

#include "qsort.h"

extern fixed_t FocalLengthX, FocalLengthY;

#define MINZ							(FRACUNIT*4)
#define BASEYCENTER 					(100)

//void R_DrawColumn (void);
//void R_DrawFuzzColumn (void);

static int crosshair_lump;

static void R_InitCrosshair();
static byte crosshair_trans[256];

EXTERN_CVAR (hud_crosshairhealth)
CVAR_FUNC_IMPL (hud_crosshair)
{
	R_InitCrosshair();
}

//
// Sprite rotation 0 is facing the viewer,
//	rotation 1 is one angle turn CLOCKWISE around the axis.
// This is not the same as the angle,
//	which increases counter clockwise (protractor).
//
fixed_t 		pspritexscale;
fixed_t			pspriteyscale;
fixed_t 		pspritexiscale;
//fixed_t		sky1scale;			// [RH] Sky 1 scale factor
									// [ML] 5/11/06 - Removed sky2
int*			spritelights;

// constant arrays
//	used for psprite clipping and initializing clipping
int			*negonearray;
int			*screenheightarray;

#define MAX_SPRITE_FRAMES 29		// [RH] Macro-ized as in BOOM.
#define SPRITE_NEEDS_INFO	MAXINT

EXTERN_CVAR (r_drawplayersprites)
EXTERN_CVAR (r_particles)

EXTERN_CVAR (hud_crosshairdim)
EXTERN_CVAR (hud_crosshairscale) 

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//	and range check thing_t sprites patches
spritedef_t*	sprites;
int				numsprites;

spriteframe_t	sprtemp[MAX_SPRITE_FRAMES];
int 			maxframe;
static const char*		spritename;

// [RH] skin globals
playerskin_t	*skins;
size_t			numskins;

// [RH] particle globals
extern int				NumParticles;
extern int				ActiveParticles;
extern int				InactiveParticles;
extern particle_t		*Particles;
TArray<WORD>			ParticlesInSubsec;

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

	// scan all the lump names for each of the names,
	//	noting the highest frame letter.
	// Just compare 4 characters as ints
	for (i = 0; i < realsprites; i++)
	{
		spritename = (const char *)namelist[i];
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

static void R_InitCrosshair()
{
	int xhairnum = (int)hud_crosshair;

	if (xhairnum)
	{
		if (xhairnum < 0)
			xhairnum = -xhairnum;

		char xhairname[16];
		int xhair;

		sprintf (xhairname, "XHAIR%d", xhairnum);

		if ((xhair = W_CheckNumForName (xhairname)) == -1)
		{
			xhair = W_CheckNumForName ("XHAIR1");
		}

		if(xhair != -1)
			crosshair_lump = xhair;
	}

	// set up translation table for the crosshair's color
	// initialize to default colors
	for (size_t i = 0; i < 256; i++)
		crosshair_trans[i] = i;
}

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

	// set up the crosshair
	R_InitCrosshair();
}



//
// R_ClearSprites
// Called at frame start.
//
void R_ClearSprites (void)
{
	vissprite_p = vissprites;
}


//
// R_NewVisSprite
//
vissprite_t *R_NewVisSprite (void)
{
	if (vissprite_p == lastvissprite) {
		int prevvisspritenum = vissprite_p - vissprites;

		MaxVisSprites *= 2;
		vissprites = (vissprite_t *)Realloc (vissprites, MaxVisSprites * sizeof(vissprite_t));
		lastvissprite = &vissprites[MaxVisSprites];
		vissprite_p = &vissprites[prevvisspritenum];
		DPrintf ("MaxVisSprites increased to %d\n", MaxVisSprites);
	}

	vissprite_p++;
	return vissprite_p-1;
}


//
// R_DrawMaskedColumn
// Used for sprites and masked mid textures.
// Masked means: partly transparent, i.e. stored
//	in posts/runs of opaque pixels.
//
int*			mfloorclip;
int*			mceilingclip;

fixed_t 		spryscale;
fixed_t 		sprtopscreen;

void R_DrawMaskedColumn(tallpost_t *post)
{
	while (!post->end())
	{
		if (post->length == 0)
		{
			post = post->next();
			continue;
		}

		// calculate unclipped screen coordinates for post
		int topscreen = sprtopscreen + spryscale * post->topdelta + 1;

		dc_yl = (topscreen + FRACUNIT) >> FRACBITS;
		dc_yh = (topscreen + spryscale * post->length) >> FRACBITS;

		if (dc_yh >= mfloorclip[dc_x])
			dc_yh = mfloorclip[dc_x] - 1;
		if (dc_yl <= mceilingclip[dc_x])
			dc_yl = mceilingclip[dc_x] + 1;

		dc_texturefrac = dc_texturemid - (post->topdelta << FRACBITS)
			+ (dc_yl*dc_iscale) - FixedMul(centeryfrac-FRACUNIT, dc_iscale);

		if (dc_texturefrac < 0)
		{
			int cnt = (FixedDiv(-dc_texturefrac, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
			dc_yl += cnt;
			dc_texturefrac += cnt * dc_iscale;
		}

		const fixed_t endfrac = dc_texturefrac + (dc_yh-dc_yl)*dc_iscale;
		const fixed_t maxfrac = post->length << FRACBITS;
		
		if (endfrac >= maxfrac)
		{
			int cnt = (FixedDiv(endfrac - maxfrac - 1, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
			dc_yh -= cnt;
		}

		dc_source = post->data();

		if (dc_yl >= 0 && dc_yh < viewheight && dc_yl <= dc_yh)
			colfunc();
	
		post = post->next();
	}
}

//
// R_DrawVisSprite
//	mfloorclip and mceilingclip should also be set.
//
void R_DrawVisSprite (vissprite_t *vis, int x1, int x2)
{
#ifdef RANGECHECK
	unsigned int		patchwidth;
#endif
	fixed_t 			frac;
	patch_t*			patch;

	bool				fuzz_effect = false;
	bool				translated = false;
	bool				lucent = false;

	dc_textureheight = 256 << FRACBITS;

	if (vis->mobjflags & MF_SPECTATOR)
		return;

	if (vis->patch == NO_PARTICLE)
	{
		// [RH] It's a particle
		if (screen->is8bit())
			R_DrawParticleP (vis, x1, x2);
		else
			R_DrawParticleD (vis, x1, x2);
		return;
	}

	patch = W_CachePatch (vis->patch);

	dc_colormap = vis->colormap;

	if (vis->translation)
	{
		translated = true;
		dc_translation = vis->translation;
	}
	else if (vis->mobjflags & MF_TRANSLATION)
	{
		// [RH] MF_TRANSLATION is still available for DeHackEd patches that
		//		used it, but the prefered way to change a thing's colors
		//		is now with the palette field.
		translated = true;
		dc_translation = translationref_t(translationtables + (MAXPLAYERS-1)*256 +
			( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) ));
	}

	if (vis->mobjflags & MF_SHADOW)
	{
		// [RH] I use MF_SHADOW to recognize fuzz effect now instead of
		//		a NULL colormap. This allow proper substition of
		//		translucency with light levels if desired. The original
		//		code used colormap == NULL to indicate shadows.
		dc_translevel = FRACUNIT/5;
		fuzz_effect = true;
	}
	else if (vis->translucency < FRACUNIT)
	{	// [RH] draw translucent column
		lucent = true;
		dc_translevel = vis->translucency;
	}

	// [SL] Select the set of drawing functions to use
	R_ResetDrawFuncs();

	if (fuzz_effect)
		colfunc = fuzzcolfunc;
	else if (lucent && translated)
		R_SetTranslatedLucentDrawFuncs();
	else if (lucent)
		R_SetLucentDrawFuncs();
	else if (translated)
		R_SetTranslatedDrawFuncs();

	dc_iscale = FixedDiv (FRACUNIT, vis->yscale) + 1;
	dc_texturemid = vis->texturemid;
	frac = vis->startfrac;
	spryscale = vis->yscale;
	sprtopscreen = centeryfrac - FixedMul (dc_texturemid, spryscale);
#ifdef RANGECHECK
	patchwidth = (unsigned)(patch->width());
#endif

	if (!r_columnmethod || fuzz_effect) {
		// [RH] The original Doom method of drawing sprites
		int x1 = vis->x1, x2 = vis->x2;
		fixed_t xiscale = vis->xiscale;

		for (dc_x = x1; dc_x <= x2; dc_x++, frac += xiscale)
		{
			unsigned int texturecolumn = frac>>FRACBITS;

#ifdef RANGECHECK
			if (texturecolumn >= patchwidth) {
				DPrintf ("R_DrawSpriteRange: bad texturecolumn (%d)\n", texturecolumn);
				continue;
			}
#endif

			R_DrawMaskedColumn ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[texturecolumn])));
		}
	} else {
		// [RH] Cache-friendly drawer
		int x1 = vis->x1, x2 = vis->x2 + 1;
		fixed_t xiscale = vis->xiscale;
		int stop = x2 & ~3;

		if (x1 < x2) {
			dc_x = x1;

			if (dc_x & 1) {
				R_DrawMaskedColumn ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				dc_x++;
				frac += xiscale;
			}

			if (dc_x & 2) {
				if (dc_x < x2 - 1) {
					rt_initcols();
					R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
					dc_x++;
					frac += xiscale;
					R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
					rt_draw2cols ((dc_x - 1) & 3, dc_x - 1);
					dc_x++;
					frac += xiscale;
				} else if (dc_x == x2 - 1) {
					R_DrawMaskedColumn ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
					dc_x++;
					frac += xiscale;
				}
			}

			while (dc_x < stop) {
				rt_initcols();
				R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				dc_x++;
				frac += xiscale;
				R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				dc_x++;
				frac += xiscale;
				R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				dc_x++;
				frac += xiscale;
				R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				rt_draw4cols (dc_x - 3);
				dc_x++;
				frac += xiscale;
			}

			if (x2 - dc_x == 1) {
				R_DrawMaskedColumn ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
			} else if (x2 - dc_x >= 2) {
				rt_initcols();
				R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				dc_x++;
				frac += xiscale;
				R_DrawMaskedColumnHoriz ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				rt_draw2cols ((dc_x - 1) & 3, dc_x - 1);
				dc_x++;
				frac += xiscale;
				if (++dc_x < x2) {
					R_DrawMaskedColumn ((tallpost_t *)((byte *)patch + LELONG(patch->columnofs[frac>>FRACBITS])));
				}
			}
		}
	}

	R_ResetDrawFuncs();
}



//
// R_ProjectSprite
// Generates a vissprite for a thing if it might be visible.
//
void R_ProjectSprite (AActor *thing, int fakeside)
{
	fixed_t 			tr_x;
	fixed_t 			tr_y;

	fixed_t 			gxt;
	fixed_t 			gyt;

	fixed_t				gzt;				// killough 3/27/98
	fixed_t				gzb;
	fixed_t 			tx;
	fixed_t 			tz;

	fixed_t 			xscale;
	fixed_t				yscale;

	int 				x1;
	int 				x2;

	spritedef_t*		sprdef;
	spriteframe_t*		sprframe;
	int 				lump;

	unsigned			rot;
	BOOL 				flip;

	int 				index;

	vissprite_t*		vis;

	angle_t 			ang;
	fixed_t 			iscale;

	sector_t*			heightsec;			// killough 3/27/98

	if (thing->flags2 & MF2_DONTDRAW || thing->translucency == 0 ||
		(thing->player && thing->player->spectator))
		return;

	// transform the origin point
	tr_x = thing->x - viewx;
	tr_y = thing->y - viewy;

	gxt = FixedMul (tr_x,viewcos);
	gyt = -FixedMul (tr_y,viewsin);

	tz = gxt-gyt;

	// thing is behind view plane?
	if (tz < MINZ)
		return;

	xscale = FixedDiv (FocalLengthX, tz);
	yscale = FixedDiv (FocalLengthY, tz);

	gxt = -FixedMul (tr_x, viewsin);
	gyt = FixedMul (tr_y, viewcos);
	tx = -(gyt+gxt);

	// too far off the side?
	if (abs(tx)>(tz<<2))
		return;

	// decide which patch to use for sprite relative to player
#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= (unsigned)numsprites)
	{
		DPrintf ("R_ProjectSprite: invalid sprite number %i\n", thing->sprite);
		return;
	}
#endif
	sprdef = &sprites[thing->sprite];
#ifdef RANGECHECK
	if ( (thing->frame&FF_FRAMEMASK) >= sprdef->numframes )
	{
		DPrintf ("R_ProjectSprite: invalid sprite frame %i : %i\n ", thing->sprite, thing->frame);
		return;
	}
#endif
	sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

	if (sprframe->rotate)
	{
		// choose a different rotation based on player view
		ang = R_PointToAngle (thing->x, thing->y);
		rot = (ang-thing->angle+(unsigned)(ANG45/2)*9)>>29;
		lump = sprframe->lump[rot];
		flip = (BOOL)sprframe->flip[rot];
	}
	else
	{
		// use single rotation for all views
		lump = sprframe->lump[rot = 0];
		flip = (BOOL)sprframe->flip[0];
	}

	if (sprframe->width[rot] == SPRITE_NEEDS_INFO)
		R_CacheSprite (sprdef);	// [RH] speeds up game startup time

	// calculate edges of the shape
	tx -= sprframe->offset[rot];	// [RH] Moved out of spriteoffset[]
	x1 = (centerxfrac + FixedMul (tx,xscale)) >> FRACBITS;

	// off the right side?
	if (x1 > viewwidth)
		return;

	tx += sprframe->width[rot];	// [RH] Moved out of spritewidth[]
	x2 = ((centerxfrac + FixedMul (tx,xscale)) >> FRACBITS) - 1;

	// off the left side or too small?
	if (x2 < 0 || x2 < x1)
		return;

	gzt = thing->z + sprframe->topoffset[rot];	// [RH] Moved out of spritetopoffset[]
	gzb = thing->z;

	// killough 4/9/98: clip things which are out of view due to height
// [RH] This doesn't work too well with freelook
//	if (thing->z > viewz + FixedDiv(centeryfrac, yscale) ||
//		gzt      < viewz - FixedDiv(centeryfrac-viewheight, yscale))
//		return;

	// killough 3/27/98: exclude things totally separated
	// from the viewer, by either water or fake ceilings
	// killough 4/11/98: improve sprite clipping for underwater/fake ceilings

	heightsec = thing->subsector->sector->heightsec;
	
	if (heightsec && heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC)
		heightsec = NULL;
	
	if (heightsec)	// only clip things which are in special sectors
	{
		if (fakeside == FAKED_AboveCeiling)
		{
			if (gzt < P_CeilingHeight(heightsec))
				return;
		}
		else if (fakeside == FAKED_BelowFloor)
		{
			if (gzb >= P_FloorHeight(heightsec))
				return;
		}
		else
		{
			if (gzt < P_FloorHeight(heightsec))
				return;
			if (gzb >= P_CeilingHeight(heightsec))
				return;
		}
	}

	// store information in a vissprite
	vis = R_NewVisSprite ();

	// killough 3/27/98: save sector for special clipping later
	vis->heightsec = heightsec;

	vis->mobjflags = thing->flags;
	vis->xscale = xscale;
	vis->yscale = yscale;
	vis->gx = thing->x;
	vis->gy = thing->y;
	vis->gz = gzb;
	vis->gzt = gzt;		// killough 3/27/98
	vis->texturemid = gzt - viewz;
	vis->x1 = x1 < 0 ? 0 : x1;
	vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	vis->translation = thing->translation;		// [RH] thing translation table
	vis->translucency = thing->translucency;
	vis->depth = tz;
	vis->FakeFlat = fakeside;
	iscale = FixedDiv (tz, FocalLengthX);

	if (flip)
	{
		vis->startfrac = sprframe->width[rot]-1;	// [RH] Moved out of spritewidth
		vis->xiscale = -iscale;
	}
	else
	{
		vis->startfrac = 0;
		vis->xiscale = iscale;
	}

	if (vis->x1 > x1)
		vis->startfrac += vis->xiscale*(vis->x1-x1);
	vis->patch = lump;

	// get light level
	if (fixedlightlev)
	{
		vis->colormap = basecolormap.with(fixedlightlev);
	}
	else if (fixedcolormap.isValid())
	{
		// fixed map
		vis->colormap = fixedcolormap;
	}
	else if (!foggy && (thing->frame & FF_FULLBRIGHT))
	{
		// full bright
		vis->colormap = basecolormap;	// [RH] Use basecolormap
	}
	else
	{
		// diminished light
		index = (vis->yscale*lightscalexmul)>>LIGHTSCALESHIFT;	// [RH]

		if (index >= MAXLIGHTSCALE)
			index = MAXLIGHTSCALE-1;

		vis->colormap = basecolormap.with(spritelights[index]);	// [RH] Use basecolormap
	}
}


//
// R_AddSprites
// During BSP traversal, this adds sprites by sector.
//
// killough 9/18/98: add lightlevel as parameter, fixing underwater lighting
void R_AddSprites (sector_t *sec, int lightlevel, int fakeside)
{
	AActor *thing;
	int 	lightnum;

	// BSP is traversed by subsector.
	// A sector might have been split into several
	//	subsectors during BSP building.
	// Thus we check whether it was already added.
	if (sec->validcount == validcount)
		return;

	// Well, now it will be done.
	sec->validcount = validcount;

	lightnum = (lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);

	if (lightnum < 0)
		spritelights = scalelight[0];
	else if (lightnum >= LIGHTLEVELS)
		spritelights = scalelight[LIGHTLEVELS-1];
	else
		spritelights = scalelight[lightnum];

	// Handle all things in sector.
	for (thing = sec->thinglist ; thing ; thing = thing->snext)
	{
		R_ProjectSprite (thing, fakeside);
	}
}


//
// R_DrawPSprite
//
void R_DrawPSprite (pspdef_t* psp, unsigned flags)
{
	fixed_t 			tx;
	int 				x1;
	int 				x2;
	spritedef_t*		sprdef;
	spriteframe_t*		sprframe;
	int 				lump;
	BOOL 				flip;
	vissprite_t*		vis;
	vissprite_t 		avis;

	// decide which patch to use
#ifdef RANGECHECK
	if ( (unsigned)psp->state->sprite >= (unsigned)numsprites) {
		DPrintf ("R_DrawPSprite: invalid sprite number %i\n", psp->state->sprite);
		return;
	}
#endif
	sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
	if ( (psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes) {
		DPrintf ("R_DrawPSprite: invalid sprite frame %i : %i\n", psp->state->sprite, psp->state->frame);
		return;
	}
#endif
	sprframe = &sprdef->spriteframes[ psp->state->frame & FF_FRAMEMASK ];

	lump = sprframe->lump[0];
	flip = (BOOL)sprframe->flip[0];

	if (sprframe->width[0] == SPRITE_NEEDS_INFO)
		R_CacheSprite (sprdef);	// [RH] speeds up game startup time

	// calculate edges of the shape
	tx = psp->sx-((320/2)<<FRACBITS);

	tx -= sprframe->offset[0];	// [RH] Moved out of spriteoffset[]
	x1 = (centerxfrac + FixedMul (tx, pspritexscale)) >>FRACBITS;

	// off the right side
	if (x1 > viewwidth)
		return;

	tx += sprframe->width[0];	// [RH] Moved out of spritewidth[]
	x2 = ((centerxfrac + FixedMul (tx, pspritexscale)) >>FRACBITS) - 1;

	// off the left side
	if (x2 < 0)
		return;

	// store information in a vissprite
	vis = &avis;
	vis->mobjflags = flags;

// [RH] +0x6000 helps it meet the screen bottom
//		at higher resolutions while still being in
//		the right spot at 320x200.
// denis - bump to 0x9000
#define WEAPONTWEAK				(0x9000)

	vis->texturemid = (BASEYCENTER<<FRACBITS)+FRACUNIT/2-
		(psp->sy+WEAPONTWEAK-sprframe->topoffset[0]);	// [RH] Moved out of spritetopoffset[]
	vis->x1 = x1 < 0 ? 0 : x1;
	vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	vis->xscale = pspritexscale;
	vis->yscale = pspriteyscale;
	vis->translation = translationref_t();		// [RH] Use default colors
	vis->translucency = FRACUNIT;

	if (flip)
	{
		vis->xiscale = -pspritexiscale;
		vis->startfrac = sprframe->width[0]-1;	// [RH] Moved out of spritewidth[]
	}
	else
	{
		vis->xiscale = pspritexiscale;
		vis->startfrac = 0;
	}

	if (vis->x1 > x1)
		vis->startfrac += vis->xiscale*(vis->x1-x1);

	vis->patch = lump;

	if (fixedlightlev)
	{
		vis->colormap = basecolormap.with(fixedlightlev);
	}
	else if (fixedcolormap.isValid())
	{
		// fixed color
		vis->colormap = fixedcolormap;
	}
	else if (psp->state->frame & FF_FULLBRIGHT)
	{
		// full bright
		vis->colormap = basecolormap;	// [RH] use basecolormap
	}
	else
	{
		// local light
		vis->colormap = basecolormap.with(spritelights[MAXLIGHTSCALE-1]);	// [RH] add basecolormap
	}
	if (camera->player &&
		(camera->player->powers[pw_invisibility] > 4*32
		 || camera->player->powers[pw_invisibility] & 8))
	{
		// shadow draw
		vis->mobjflags = MF_SHADOW;
	}

	// Don't display the weapon sprite if using spectating without spynext
	if (consoleplayer().spectator && displayplayer_id == consoleplayer_id)
		return;

	R_DrawVisSprite (vis, vis->x1, vis->x2);
}



//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites (void)
{
	int 		i;
	int 		lightnum;
	pspdef_t*	psp;
	sector_t*	sec;
	static sector_t tempsec;
	int			floorlight, ceilinglight;

	if(!camera || !camera->subsector)
		return;

	if (!r_drawplayersprites ||
		!camera->player ||
		(consoleplayer().cheats & CF_CHASECAM))
		return;

	sec = R_FakeFlat (camera->subsector->sector, &tempsec, &floorlight,
		&ceilinglight, false);

	// [RH] set foggy flag
	foggy = (level.fadeto || sec->floorcolormap->fade);

	// [RH] set basecolormap
	basecolormap = sec->floorcolormap->maps;

	// get light level
	lightnum = ((floorlight + ceilinglight) >> (LIGHTSEGSHIFT+1))
		+ (foggy ? 0 : extralight);

	if (lightnum < 0)
		spritelights = scalelight[0];
	else if (lightnum >= LIGHTLEVELS)
		spritelights = scalelight[LIGHTLEVELS-1];
	else
		spritelights = scalelight[lightnum];

	// clip to screen bounds
	mfloorclip = screenheightarray;
	mceilingclip = negonearray;

	{
		int centerhack = centery;

		centery = viewheight >> 1;
		centeryfrac = centery << FRACBITS;

		// add all active psprites
		for (i=0, psp=camera->player->psprites;
			 i<NUMPSPRITES;
			 i++,psp++)
		{
			if (psp->state)
				R_DrawPSprite (psp, 0);
		}

		centery = centerhack;
		centeryfrac = centerhack << FRACBITS;
	}
}




//
// R_SortVisSprites
//
// [RH] The old code for this function used a bubble sort, which was far less
//		than optimal with large numbers of sprties. I changed it to use the
//		stdlib qsort() function instead, and now it is a *lot* faster; the
//		more vissprites that need to be sorted, the better the performance
//		gain compared to the old function.
//
static struct vissort_s {
	vissprite_t *sprite;
	fixed_t		depth;
}				*spritesorter;
static int		spritesortersize = 0;
static int		vsprcount;

#define cmp_sprites(a, b) (b->depth < a->depth)

void R_SortVisSprites (void)
{
	int i;

	vsprcount = vissprite_p - vissprites;

	if (!vsprcount)
		return;

	if (spritesortersize < MaxVisSprites)
	{
		spritesorter = (vissort_s *)Realloc (spritesorter, sizeof(struct vissort_s) * MaxVisSprites);
		spritesortersize = MaxVisSprites;
	}

	for (i = 0; i < vsprcount; i++)
	{
		spritesorter[i].sprite = &vissprites[i];
		spritesorter[i].depth = vissprites[i].depth;
	}

    QSORT(vissort_s, spritesorter, vsprcount, cmp_sprites);
}


//
// R_DrawSprite
//
void R_DrawSprite (vissprite_t *spr)
{
	static int			cliptop[MAXWIDTH];
	static int			clipbot[MAXWIDTH];

	drawseg_t*			ds;
	int 				x;
	int 				r1, r2;
	fixed_t 			segscale1, segscale2;

	int					topclip = 0, botclip = viewheight;
	int*				clip1;
	int*				clip2;

	// [RH] Quickly reject sprites with bad x ranges.
	if (spr->x1 > spr->x2)
		return;

	// killough 3/27/98:
	// Clip the sprite against deep water and/or fake ceilings.
	// [RH] rewrote this to be based on which part of the sector is really visible

	if (spr->heightsec && !(spr->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{ 
		if (spr->FakeFlat != FAKED_AboveCeiling)
		{
			fixed_t h = P_FloorHeight(spr->heightsec);
			h = (centeryfrac - FixedMul(h - viewz, spr->yscale)) >> FRACBITS;

			if (spr->FakeFlat == FAKED_BelowFloor)
			{ // seen below floor: clip top
				if (h > topclip)
					topclip = MIN<int>(h, viewheight);
			}
			else
			{ // seen in the middle: clip bottom
				if (h < botclip)
					botclip = MAX<int>(0, h);
			}
		}
		if (spr->FakeFlat != FAKED_BelowFloor)
		{
			fixed_t h = P_CeilingHeight(spr->heightsec);
			h = (centeryfrac - FixedMul(h - viewz, spr->yscale)) >> FRACBITS;

			if (spr->FakeFlat == FAKED_AboveCeiling)
			{ // seen above ceiling: clip bottom
				if (h < botclip)
					botclip = MAX<int>(0, h);
			}
			else
			{ // seen in the middle: clip top
				if (h > topclip)
					topclip = MIN<int>(h, viewheight);
			}
		}
	}

	// initialize the clipping arrays
	int i = spr->x2 - spr->x1 + 1;
	clip1 = clipbot + spr->x1;
	clip2 = cliptop + spr->x1;
	do
	{
		*clip1++ = botclip;
		*clip2++ = topclip;
	} while (--i);

	// Scan drawsegs from end to start for obscuring segs.
	// The first drawseg that has a greater scale is the clip seg.

	// Modified by Lee Killough:
	// (pointer check was originally nonportable
	// and buggy, by going past LEFT end of array):

	for (ds = ds_p ; ds-- > drawsegs ; )  // new -- killough
	{
		// determine if the drawseg obscures the sprite
		if (ds->x1 > spr->x2 || ds->x2 < spr->x1 ||
			(!(ds->silhouette & SIL_BOTH) && !ds->maskedtexturecol) )
		{
			// does not cover sprite
			continue;
		}

		r1 = MAX<int>(ds->x1, spr->x1);
		r2 = MIN<int>(ds->x2, spr->x2);

		segscale1 = MAX<int>(ds->scale1, ds->scale2);
		segscale2 = MIN<int>(ds->scale1, ds->scale2);

		// check if the seg is in front of the sprite
		if (segscale1 < spr->yscale ||
			(segscale2 < spr->yscale && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
		{
			// masked mid texture?
			if (ds->maskedtexturecol)
				R_RenderMaskedSegRange (ds, r1, r2);
			// seg is behind sprite
			continue;
		}

		// clip this piece of the sprite
		// killough 3/27/98: optimized and made much shorter

		for (x = r1; x <= r2; x++)
		{
			if (ds->silhouette & SIL_BOTTOM && clipbot[x] > ds->sprbottomclip[x])
				clipbot[x] = ds->sprbottomclip[x];
			if (ds->silhouette & SIL_TOP && cliptop[x] < ds->sprtopclip[x])
				cliptop[x] = ds->sprtopclip[x];
		}
	}

	// all clipping has been performed, so draw the sprite
	mfloorclip = clipbot;
	mceilingclip = cliptop;
	R_DrawVisSprite (spr, spr->x1, spr->x2);
}



static void R_DrawCrosshair (void)
{
	if(!camera)
		return;

	// Don't draw the crosshair in chasecam mode
	if (camera->player && (camera->player->cheats & CF_CHASECAM))
		return;

    // Don't draw the crosshair in overlay mode
    if (automapactive && viewactive)
        return;
        
	// Don't draw the crosshair in spectator mode
	if (camera->player && camera->player->spectator)
		return;

	if(hud_crosshair && crosshair_lump)
	{
		static const byte crosshair_color = 0xB0;
		if (hud_crosshairhealth)
		{
			byte health_colors[4] = { 0xB0, 0xDF, 0xE7, 0x77 }; 

			if (camera->health > 75)
				crosshair_trans[crosshair_color] = health_colors[3];
			else if (camera->health > 50)
				crosshair_trans[crosshair_color] = health_colors[2];
			else if (camera->health > 25)
				crosshair_trans[crosshair_color] = health_colors[1];
			else
				crosshair_trans[crosshair_color] = health_colors[0];
		}
		else
			crosshair_trans[crosshair_color] = crosshair_color;	// no trans

		V_ColorMap = translationref_t(crosshair_trans);

		if (hud_crosshairdim && hud_crosshairscale)
			screen->DrawTranslatedLucentPatchCleanNoMove (W_CachePatch (crosshair_lump),
				realviewwidth / 2 + viewwindowx,
				realviewheight / 2 + viewwindowy);
        else if (hud_crosshairscale)
			screen->DrawTranslatedPatchCleanNoMove (W_CachePatch (crosshair_lump),
				realviewwidth / 2 + viewwindowx,
				realviewheight / 2 + viewwindowy);
        else if (hud_crosshairdim)
			screen->DrawTranslatedLucentPatch (W_CachePatch (crosshair_lump),
				realviewwidth / 2 + viewwindowx,
				realviewheight / 2 + viewwindowy);
		else
			screen->DrawTranslatedPatch (W_CachePatch (crosshair_lump),
				realviewwidth / 2 + viewwindowx,
				realviewheight / 2 + viewwindowy);
	}
}

//
// R_DrawMasked
//
void R_DrawMasked (void)
{
	drawseg_t		 *ds;
	struct vissort_s *sorttail;

	R_SortVisSprites ();

	if (vsprcount)
	{
		sorttail = spritesorter + vsprcount;
		vsprcount = -vsprcount;
		do
		{
			R_DrawSprite (sorttail[vsprcount].sprite);
		} while (++vsprcount);
	}

	// render any remaining masked mid textures

	// Modified by Lee Killough:
	// (pointer check was originally nonportable
	// and buggy, by going past LEFT end of array):

	//		for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

	for (ds=ds_p ; ds-- > drawsegs ; )	// new -- killough
		if (ds->maskedtexturecol)
			R_RenderMaskedSegRange (ds, ds->x1, ds->x2);

	// draw the psprites on top of everything
	//	but does not draw on side views
	if (!viewangleoffset)
	{
		R_DrawCrosshair (); // [RH] Draw crosshair (if active)
		R_DrawPlayerSprites ();
	}
}

void R_InitParticles (void)
{
	const char *i;

	if ((i = Args.CheckValue ("-numparticles")))
		NumParticles = atoi (i);
	if (NumParticles == 0)
		NumParticles = 4000;
	else if (NumParticles < 100)
		NumParticles = 100;

	if(Particles)
		delete[] Particles;

	Particles = new particle_t[NumParticles * sizeof(particle_t)];
	R_ClearParticles ();
}

void R_ClearParticles (void)
{
	int i;

	memset (Particles, 0, NumParticles * sizeof(particle_t));
	ActiveParticles = NO_PARTICLE;
	InactiveParticles = 0;
	for (i = 0; i < NumParticles-1; i++)
		Particles[i].next = i + 1;
	
	Particles[i].next = NO_PARTICLE;
}

void R_FindParticleSubsectors ()
{
	if (ParticlesInSubsec.Size() < (size_t)numsubsectors)
		ParticlesInSubsec.Reserve(numsubsectors - ParticlesInSubsec.Size());

	// fill the buffer with NO_PARTICLE
	for (int i = 0; i < numsubsectors; i++)
		ParticlesInSubsec[i] = NO_PARTICLE;

	if (!r_particles)
		return;
	
	for (int i = ActiveParticles; i != NO_PARTICLE; i = Particles[i].next)
	{
		subsector_t *ssec = R_PointInSubsector(Particles[i].x, Particles[i].y);
		int ssnum = ssec - subsectors;	
		
		Particles[i].nextinsubsector = ParticlesInSubsec[ssnum];
		ParticlesInSubsec[ssnum] = i;	
	}
}

void R_ProjectParticle (particle_t *particle, const sector_t *sector, int fakeside)
{
	fixed_t 			tr_x;
	fixed_t 			tr_y;
	fixed_t 			gxt;
	fixed_t 			gyt;
	fixed_t				gzt;				// killough 3/27/98
	fixed_t				gzb;
	fixed_t 			tx;
	fixed_t 			tz;
	fixed_t 			xscale;
	int 				x1;
	int 				x2;
	vissprite_t*		vis;
	sector_t*			heightsec = NULL;	// killough 3/27/98
		
	// transform the origin point
	tr_x = particle->x - viewx;
	tr_y = particle->y - viewy;
		
	gxt = FixedMul (tr_x, viewcos); 
	gyt = -FixedMul (tr_y, viewsin);
	
	tz = gxt - gyt; 

	// particle is behind view plane?
	if (tz < MINZ)
		return;
	
	xscale = FixedDiv (FocalLengthX, tz);
		
	gxt = -FixedMul (tr_x, viewsin); 
	gyt = FixedMul (tr_y, viewcos); 
	tx = -(gyt+gxt); 

	// too far off the side?
	if (abs(tx)>(tz<<2))
		return;
	
	// calculate edges of the shape
	x1 = (centerxfrac + FixedMul (tx,xscale)) >> FRACBITS;

	// off the right side?
	if (x1 >= viewwidth)
		return;
	
	x2 = ((centerxfrac + FixedMul (tx+particle->size*(FRACUNIT/4),xscale)) >> FRACBITS);

	// off the left side
	if (x2 < 0)
		return;

	gzt = particle->z+1;
	gzb = particle->z;

	// killough 3/27/98: exclude things totally separated
	// from the viewer, by either water or fake ceilings
	// killough 4/11/98: improve sprite clipping for underwater/fake ceilings

	if (gzt < P_FloorHeight(particle->x, particle->y, sector) ||
		gzb > P_CeilingHeight(particle->x, particle->y, sector))
		return;

	heightsec = sector->heightsec;
	if (heightsec)	// only clip particles which are in special sectors
	{
		if (fakeside == FAKED_AboveCeiling)
		{
			if (gzt < P_CeilingHeight(heightsec))
				return;
		}
		else if (fakeside == FAKED_BelowFloor)
		{
			if (gzb >= P_FloorHeight(heightsec))
				return;
		}
		else
		{
			if (gzt < P_FloorHeight(heightsec))
				return;
			if (gzb >= P_CeilingHeight(heightsec))
				return;
		}
	}

	// store information in a vissprite
	vis = R_NewVisSprite ();
	vis->heightsec = heightsec;
	vis->xscale = xscale;
	vis->yscale = FixedMul (xscale, yaspectmul);
	vis->depth = tz;
	vis->gx = particle->x;
	vis->gy = particle->y;
	vis->gz = gzb;
	vis->gzt = gzt;
	vis->texturemid = FixedMul (yaspectmul, vis->gzt - viewz);
	vis->x1 = x1 < 0 ? 0 : x1;
	vis->x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	vis->translation = translationref_t();
	vis->startfrac = particle->color;
	vis->patch = NO_PARTICLE;
	vis->mobjflags = particle->trans;
	vis->FakeFlat = fakeside;

	if (fixedcolormap.isValid())
	{
		vis->colormap = fixedcolormap;
	}
	else if (sector)
	{
		shaderef_t map;

		if (heightsec && 
			(gzt <= P_FloorHeight(heightsec) || gzb > P_CeilingHeight(heightsec)))
		{
			map = sector->heightsec->floorcolormap->maps;
		}
		else
		{
			map = sector->floorcolormap->maps;
		}


		if (fixedlightlev)
		{
			vis->colormap = map.with(fixedlightlev);
		}
		else
		{
			int index = (vis->yscale*lightscalexmul)>>(LIGHTSCALESHIFT-1);
			int lightnum = (sector->lightlevel >> LIGHTSEGSHIFT)
					+ (foggy ? 0 : extralight);

			if (lightnum < 0)
				lightnum = 0;
			else if (lightnum >= LIGHTLEVELS)
				lightnum = LIGHTLEVELS-1;
			if (index >= MAXLIGHTSCALE) 
				index = MAXLIGHTSCALE-1;

			vis->colormap = map.with(scalelight[lightnum][index]);
		}
	}
	else
	{
		vis->colormap = shaderef_t(&realcolormaps, 0);
		//vis->colormap = shaderef_t(&GetDefaultPalette()->maps, 0);
	}
}

void R_DrawParticleP (vissprite_t *vis, int x1, int x2)
{
	byte color = vis->colormap.index(vis->startfrac);
	int yl = (centeryfrac - FixedMul(vis->texturemid, vis->xscale) + FRACUNIT - 1) >> FRACBITS;
	int yh;
	x1 = vis->x1;
	x2 = vis->x2;

	if (x1 < 0)
		x1 = 0;
	if (x2 < x1)
		x2 = x1;
	if (x2 >= viewwidth)
		x2 = viewwidth - 1;

	yh = yl + (((x2 - x1)<<detailxshift)>>detailyshift);

	// Don't bother clipping each individual column
	if (yh >= mfloorclip[x1])
		yh = mfloorclip[x1]-1;
	if (yl <= mceilingclip[x1])
		yl = mceilingclip[x1]+1;
	if (yh >= mfloorclip[x2])
		yh = mfloorclip[x2]-1;
	if (yl <= mceilingclip[x2])
		yl = mceilingclip[x2]+1;

	// vis->mobjflags holds translucency level (0-255)
	{
		argb_t *bg2rgb;
		int countbase = x2 - x1 + 1;
		int ycount;
		int colsize = ds_colsize;
		int spacing;
		byte *dest;
		argb_t fg;

		ycount = yh - yl;
		if (ycount < 0)
			return;
		ycount++;

		{
			fixed_t fglevel, bglevel;
			argb_t *fg2rgb;

			fglevel = ((vis->mobjflags + 1) << 8) & ~0x3ff;
			bglevel = FRACUNIT-fglevel;
			fg2rgb = Col2RGB8[fglevel>>10];
			bg2rgb = Col2RGB8[bglevel>>10];
			fg = fg2rgb[color];
		}

		spacing = screen->pitch - (countbase << detailxshift);
		dest = ylookup[yl] + columnofs[x1];

		do
		{
			int count = countbase;
			do
			{
				argb_t bg = bg2rgb[*dest];
				bg = (fg+bg) | 0x1f07c1f;
				*dest = RGB32k[0][0][bg & (bg>>15)];
				dest += colsize;
			} while (--count);
			dest += spacing;
		} while (--ycount);
	}
}

void R_DrawParticleD (vissprite_t *vis, int x1, int x2)
{
	argb_t color = vis->colormap.shade(vis->startfrac);
	int yl = (centeryfrac - FixedMul(vis->texturemid, vis->xscale) + FRACUNIT - 1) >> FRACBITS;
	int yh;
	x1 = vis->x1;
	x2 = vis->x2;

	if (x1 < 0)
		x1 = 0;
	if (x2 < x1)
		x2 = x1;
	if (x2 >= viewwidth)
		x2 = viewwidth - 1;

	yh = yl + (((x2 - x1)<<detailxshift)>>detailyshift);

	// Don't bother clipping each individual column
	if (yh >= mfloorclip[x1])
		yh = mfloorclip[x1]-1;
	if (yl <= mceilingclip[x1])
		yl = mceilingclip[x1]+1;
	if (yh >= mfloorclip[x2])
		yh = mfloorclip[x2]-1;
	if (yl <= mceilingclip[x2])
		yl = mceilingclip[x2]+1;

	// vis->mobjflags holds translucency level (0-255)
	{
		unsigned int *bg2rgb;
		int countbase = x2 - x1 + 1;
		int ycount;
		int colsize = ds_colsize;
		int spacing;
		int pitch;
		argb_t *dest;

		ycount = yh - yl;
		if (ycount < 0)
			return;
		ycount++;

		pitch = screen->pitch / sizeof(argb_t);

		int fga, bga;
		{
			fixed_t fglevel, bglevel;

			fglevel = ((vis->mobjflags + 1) << 8) & ~0x3ff;
			bglevel = FRACUNIT-fglevel;
			fga = fglevel >> 8;
			bga = bglevel >> 8;
		}

		spacing = pitch - (countbase << detailxshift);
		dest = (argb_t *)( ylookup[yl] + columnofs[x1] );

		do
		{
			int count = countbase;
			do
			{
				*dest = alphablend2a(*dest, bga, color, fga);
				dest += colsize;
			} while (--count);
			dest += spacing;
		} while (--ycount);
	}
}

VERSION_CONTROL (r_things_cpp, "$Id$")
