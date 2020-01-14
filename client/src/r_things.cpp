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
//		Refresh of things, i.e. objects represented by sprites.
//
//-----------------------------------------------------------------------------

#include "m_alloc.h"

#include "doomdef.h"
#include "m_swap.h"
#include "m_argv.h"

#include "i_system.h"
#include "z_zone.h"
#include "resources/res_main.h"
#include "resources/res_texture.h"

#include "r_local.h"
#include "p_local.h"

#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"

#include "doomstat.h"

#include "v_video.h"

#include "cmdlib.h"
#include "s_sound.h"

#include "m_vectors.h"

extern fixed_t FocalLengthX, FocalLengthY;

#define MINZ							(FRACUNIT*4)
#define BASEYCENTER 					(100)

//void R_DrawColumn (void);
//void R_DrawFuzzColumn (void);


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

#define MAX_SPRITE_FRAMES 29		// [RH] Macro-ized as in BOOM.
#define SPRITE_NEEDS_INFO	MAXINT

EXTERN_CVAR (r_drawplayersprites)
EXTERN_CVAR (r_particles)

//
// INITIALIZATION FUNCTIONS
//

// variables used to look up
//	and range check thing_t sprites patches
spritedef_t*	sprites;
int				numsprites;

spriteframe_t	sprtemp[MAX_SPRITE_FRAMES];
int 			maxframe;

static const palindex_t* spriteposts[MAXWIDTH];

// [RH] particle globals
extern int				NumParticles;
extern int				ActiveParticles;
extern int				InactiveParticles;
extern particle_t		*Particles;
TArray<WORD>			ParticlesInSubsec;

void R_CacheSprite(spritedef_t* sprite)
{
	DPrintf ("cache sprite %s\n", sprite - sprites < NUMSPRITES ? sprnames[sprite - sprites] : "");

	for (int i = 0; i < sprite->numframes; i++)
	{
		for (int r = 0; r < 8; r++)
		{
			if (sprite->spriteframes[i].width[r] == SPRITE_NEEDS_INFO)
			{
				if (sprite->spriteframes[i].resource[r] == ResourceId::INVALID_ID)
					I_Error ("Sprite %d, rotation %d has no lump", i, r);

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
static void R_InstallSpriteLump(const ResourceId res_id, uint32_t frame, uint32_t rotation, bool flipped)
{
	if (frame >= MAX_SPRITE_FRAMES || rotation > 8)
		I_FatalError ("R_InstallSpriteLump: Bad frame characters in resource ID %i", (int)res_id);

	if ((int)frame > maxframe)
		maxframe = frame;

	if (rotation == 0)
	{
		// the resource should be used for all rotations
        // false=0, true=1, but array initialised to -1
        // allows doom to have a "no value set yet" boolean value!
		for (int r = 7; r >= 0; r--)
			if (sprtemp[frame].resource[r] == ResourceId::INVALID_ID)
			{
				sprtemp[frame].resource[r] = res_id;
				sprtemp[frame].flip[r] = (byte)flipped;
				sprtemp[frame].rotate = false;
				sprtemp[frame].width[r] = SPRITE_NEEDS_INFO;
			}
	}
	else if (sprtemp[frame].resource[--rotation] == ResourceId::INVALID_ID)
	{
		// the lump is only used for one rotation
		sprtemp[frame].resource[rotation] = res_id;
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
				for (int rotation = 0; rotation < 8; rotation++)
				{
					if (sprtemp[frame].resource[rotation] == ResourceId::INVALID_ID)
						I_FatalError ("R_InstallSprite: Sprite %s frame %c is missing rotations", sprname, frame+'A');
				}
			}
			break;
		}
	}

	// allocate space for the frames present and copy sprtemp to it
	sprites[num].numframes = maxframe;
	sprites[num].spriteframes = (spriteframe_t*)Z_Malloc(maxframe * sizeof(spriteframe_t), PU_STATIC, NULL);
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
void R_InitSpriteDefs(const char **namelist)
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
				const ResourceId res_id = Res_GetTextureResourceId(sprite_paths[l], SPRITE);
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
vissprite_t		*vissprite_p;
vissprite_t		*lastvissprite;
int 			newvissprite;

//
// R_InitSprites
// Called at program start.
//
void R_InitSprites (const char **namelist)
{
	MaxVisSprites = 128;	// [RH] This is the initial default value. It grows as needed.

	M_Free(vissprites);

	vissprites = (vissprite_t *)Malloc (MaxVisSprites * sizeof(vissprite_t));
	lastvissprite = &vissprites[MaxVisSprites];

	R_InitSpriteDefs (namelist);
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
// R_BlastSpriteColum
// Used for sprites
// Masked means: partly transparent, i.e. stored
//	in posts/runs of opaque pixels.
//
const int*		mfloorclip;
const int*		mceilingclip;

fixed_t 		spryscale;
fixed_t 		sprtopscreen;

void R_BlastSpriteColumn(void (*drawfunc)())
{
	// calculate unclipped screen coordinates for post
	int64_t topscreen = sprtopscreen;
	int64_t bottomscreen = topscreen + FixedMul(spryscale, dcol.textureheight);

	dcol.yl = (int)((topscreen + FRACUNIT - 1) >> FRACBITS);
	dcol.yh = (int)((bottomscreen - 1) >> FRACBITS);

	if (mceilingclip[dcol.x] + 1 > dcol.yl)
		// TODO: dcol.texturefrac should take y-scaling of textures into account
		dcol.texturefrac = (mceilingclip[dcol.x] + 1 - dcol.yl) * dcol.iscale;
	else
		dcol.texturefrac = 0;

	dcol.yl = MAX(dcol.yl, mceilingclip[dcol.x] + 1);
	dcol.yh = MIN(dcol.yh, mfloorclip[dcol.x] - 1);

	if (dcol.yl <= dcol.yh)
		drawfunc();
}


void SpriteColumnBlaster()
{
	R_BlastSpriteColumn(colfunc);
}


//
// R_DrawVisSprite
//	mfloorclip and mceilingclip should also be set.
//
void R_DrawVisSprite(vissprite_t *vis, int x1, int x2)
{
	bool				fuzz_effect = false;
	bool				translated = false;
	bool				lucent = false;

	if (vis->yscale <= 0)
		return;

	if (vis->mobjflags & MF_SPECTATOR)
		return;

	if (vis->res_id == NO_PARTICLE)
	{
		R_DrawParticle(vis);
		return;
	}

	dcol.colormap = vis->colormap;

	if (vis->translation)
	{
		translated = true;
		dcol.translation = vis->translation;
	}
	else if (vis->mobjflags & MF_TRANSLATION)
	{
		// [RH] MF_TRANSLATION is still available for DeHackEd patches that
		//		used it, but the prefered way to change a thing's colors
		//		is now with the palette field.
		translated = true;
		dcol.translation = translationref_t(translationtables + (MAXPLAYERS-1)*256 +
			( (vis->mobjflags & MF_TRANSLATION) >> (MF_TRANSSHIFT-8) ));
	}

	if (vis->mobjflags & MF_SHADOW)
	{
		// [RH] I use MF_SHADOW to recognize fuzz effect now instead of
		//		a NULL colormap. This allow proper substition of
		//		translucency with light levels if desired. The original
		//		code used colormap == NULL to indicate shadows.
		fuzz_effect = true;
	}
	else if (vis->translucency < FRACUNIT)
	{	// [RH] draw translucent column
		lucent = true;
		dcol.translevel = vis->translucency;
	}

	// [SL] Select the set of drawing functions to use
	R_ResetDrawFuncs();

	if (fuzz_effect)
		R_SetFuzzDrawFuncs();
	else if (lucent && translated)
		R_SetTranslatedLucentDrawFuncs();
	else if (lucent)
		R_SetLucentDrawFuncs();
	else if (translated)
		R_SetTranslatedDrawFuncs();


	const Texture* texture = vis->texture;
	dcol.textureheight = texture->mHeight << FRACBITS;

	dcol.masked = true;
	dcol.iscale = 0xffffffffu / (unsigned)vis->yscale;
	dcol.texturemid = vis->texturemid;
	spryscale = vis->yscale;
	sprtopscreen = centeryfrac - FixedMul(dcol.texturemid, spryscale);

	// [SL] set up the array that indicates which patch column to use for each screen column
	fixed_t colfrac = vis->startfrac;
	for (int x = vis->x1; x <= vis->x2; x++)
	{
		spriteposts[x] = texture->getColumn(colfrac >> FRACBITS);
		colfrac += vis->xiscale;
	}

	R_RenderColumnRange(vis->x1, vis->x2, negonearray, viewheightarray, spriteposts, SpriteColumnBlaster, false);

	R_ResetDrawFuncs();
}


//
// R_GenerateVisSprite
//
// Helper function that creates a vissprite_t and projects the given world
// coordinates onto the screen. Returns NULL if the projection is completely
// clipped off the screen.
//
static vissprite_t* R_GenerateVisSprite(const sector_t* sector, int fakeside,
		fixed_t x, fixed_t y, fixed_t z, fixed_t height, fixed_t width,
		fixed_t topoffs, fixed_t sideoffs, bool flip)
{
	// translate the sprite edges from world-space to camera-space
	// and store in t1 & t2
	fixed_t tx, ty, t1xold;
	R_RotatePoint(x - viewx, y - viewy, ANG90 - viewangle, tx, ty);

	v2fixed_t t1, t2;
	t1.x = t1xold = tx - sideoffs;
	t2.x = t1.x + width;
	t1.y = t2.y = ty;

	// clip the sprite to the left & right screen edges
	int32_t lclip, rclip;
	if (!R_ClipLineToFrustum(&t1, &t2, NEARCLIP, lclip, rclip))
		return NULL;

	// calculate how much of the sprite was clipped from the left side
	R_ClipLine(&t1, &t2, lclip, rclip, &t1, &t2);
	fixed_t clipped_offset = t1.x - t1xold;

	fixed_t gzt = z + topoffs;
	fixed_t gzb = z;

	// project the sprite edges to determine which columns the sprite occupies
	int x1 = R_ProjectPointX(t1.x, ty);
	int x2 = R_ProjectPointX(t2.x, ty) - 1;
	if (!R_CheckProjectionX(x1, x2))
		return NULL;

	// Entirely above the top of the screen or below the bottom?
	int y1 = R_ProjectPointY(gzt - viewz, ty);
	int y2 = R_ProjectPointY(gzb - viewz, ty) - 1;
	if (!R_CheckProjectionY(y1, y2))
		return NULL;

	// killough 3/27/98: exclude things totally separated
	// from the viewer, by either water or fake ceilings
	// killough 4/11/98: improve sprite clipping for underwater/fake ceilings
	sector_t* heightsec = sector->heightsec;

	if (heightsec && heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC)
		heightsec = NULL;

	if (heightsec)	// only clip things which are in special sectors
	{
		if (fakeside == FAKED_AboveCeiling)
		{
			if (gzt < P_CeilingHeight(heightsec))
				return NULL;
		}
		else if (fakeside == FAKED_BelowFloor)
		{
			if (gzb >= P_FloorHeight(heightsec))
				return NULL;
		}
		else
		{
			if (gzt < P_FloorHeight(heightsec))
				return NULL;
			if (gzb >= P_CeilingHeight(heightsec))
				return NULL;
		}
	}

	// store information in a vissprite
	vissprite_t *vis = R_NewVisSprite();

	// killough 3/27/98: save sector for special clipping later
	vis->heightsec = heightsec;

	vis->xscale = FixedDiv(FocalLengthX, ty);
	vis->yscale = FixedDiv(FocalLengthY, ty);
	vis->xiscale = FixedDiv(ty, FocalLengthX);
	vis->startfrac = clipped_offset;
	vis->gx = x;
	vis->gy = y;
	vis->gzb = gzb;
	vis->gzt = gzt;
	vis->texturemid = gzt - viewz;
	vis->x1 = x1;
	vis->x2 = x2;
	vis->y1 = y1;
	vis->y2 = y2;
	vis->depth = ty;
	vis->FakeFlat = fakeside;
	vis->colormap = basecolormap;
	vis->res_id = ResourceId::INVALID_ID;
	vis->texture = NULL;

	if (flip)
	{
		vis->startfrac = width - 1 - vis->startfrac;
		vis->xiscale *= -1;
	}

	return vis;
}

void R_DrawHitBox(AActor* thing)
{
	v3fixed_t vertices[8];
	const byte color = 0x80;

	// bottom front left
	vertices[0].x = thing->x - thing->radius;
	vertices[0].y = thing->y + thing->radius;
	vertices[0].z = thing->z;

	// bottom front right
	vertices[1].x = thing->x + thing->radius;
	vertices[1].y = thing->y + thing->radius;
	vertices[1].z = thing->z;

	// bottom back left
	vertices[2].x = thing->x - thing->radius;
	vertices[2].y = thing->y - thing->radius;
	vertices[2].z = thing->z;

	// bottom back right
	vertices[3].x = thing->x + thing->radius;
	vertices[3].y = thing->y - thing->radius;
	vertices[3].z = thing->z;

	// top front left
	vertices[4].x = thing->x - thing->radius;
	vertices[4].y = thing->y + thing->radius;
	vertices[4].z = thing->z + thing->height;

	// top front right
	vertices[5].x = thing->x + thing->radius;
	vertices[5].y = thing->y + thing->radius;
	vertices[5].z = thing->z + thing->height;

	// top back left
	vertices[6].x = thing->x - thing->radius;
	vertices[6].y = thing->y - thing->radius;
	vertices[6].z = thing->z + thing->height;

	// top back right
	vertices[7].x = thing->x + thing->radius;
	vertices[7].y = thing->y - thing->radius;
	vertices[7].z = thing->z + thing->height;

	// draw bottom square
	R_DrawLine(&vertices[0], &vertices[1], color);
	R_DrawLine(&vertices[0], &vertices[2], color);
	R_DrawLine(&vertices[2], &vertices[3], color);
	R_DrawLine(&vertices[1], &vertices[3], color);

	// draw top square
	R_DrawLine(&vertices[4], &vertices[5], color);
	R_DrawLine(&vertices[4], &vertices[6], color);
	R_DrawLine(&vertices[6], &vertices[7], color);
	R_DrawLine(&vertices[5], &vertices[7], color);

	// connect the top and bottom squares
	R_DrawLine(&vertices[0], &vertices[4], color);
	R_DrawLine(&vertices[1], &vertices[5], color);
	R_DrawLine(&vertices[2], &vertices[6], color);
	R_DrawLine(&vertices[3], &vertices[7], color);
}



//
// R_ProjectSprite
// Generates a vissprite for a thing if it might be visible.
//
void R_ProjectSprite(AActor *thing, int fakeside)
{
	if (!thing || !thing->subsector || !thing->subsector->sector)
		return;

	if (thing->flags2 & MF2_DONTDRAW || thing->translucency == 0 ||
		(thing->player && thing->player->spectator))
		return;

	// [SL] interpolate the position of thing
	fixed_t thingx, thingy, thingz;

	if (P_AproxDistance2(thing, thing->prevx, thing->prevy) < 128*FRACUNIT)
	{
		// the actor probably did not teleport
		// interpolate between previous and current position
		thingx = thing->prevx + FixedMul(render_lerp_amount, thing->x - thing->prevx);
		thingy = thing->prevy + FixedMul(render_lerp_amount, thing->y - thing->prevy);
		thingz = thing->prevz + FixedMul(render_lerp_amount, thing->z - thing->prevz);
	}
	else
	{
		// the actor just teleported
		// do not interpolate
		thingx = thing->x;
		thingy = thing->y;
		thingz = thing->z;
	}

#ifdef RANGECHECK
	if ((unsigned)thing->sprite >= (unsigned)numsprites)
	{
		DPrintf ("R_ProjectSprite: invalid sprite number %i\n", thing->sprite);
		return;
	}
#endif

	const spritedef_t* sprdef = &sprites[thing->sprite];

#ifdef RANGECHECK
	if ( (thing->frame & FF_FRAMEMASK) >= sprdef->numframes )
	{
		DPrintf ("R_ProjectSprite: invalid sprite frame %i : %i\n ", thing->sprite, thing->frame);
		return;
	}
#endif

	const spriteframe_t* sprframe = &sprdef->spriteframes[thing->frame & FF_FRAMEMASK];

	int frame_index = 0;
	// choose a different rotation based on player view (if supported by the sprite)
	if (sprframe->rotate)
		frame_index = (R_PointToAngle(thingx, thingy) - thing->angle + (unsigned)(ANG45/2)*9) >> 29;

	const ResourceId res_id = sprframe->resource[frame_index];
	const Texture* texture = Res_CacheTexture(res_id);

	bool flip = sprframe->flip[frame_index];
	sector_t* sector = thing->subsector->sector;
	fixed_t topoffs = texture->mOffsetY << FRACBITS;
	fixed_t sideoffs = texture->mOffsetX << FRACBITS;
	fixed_t height = texture->mHeight << FRACBITS;
	fixed_t width = texture->mWidth << FRACBITS;

	vissprite_t* vis = R_GenerateVisSprite(sector, fakeside, thingx, thingy, thingz, height, width, topoffs, sideoffs, flip);

	if (vis == NULL)
		return;

	vis->mobjflags = thing->flags;
	vis->translation = thing->translation;		// [RH] thing translation table
	vis->translucency = thing->translucency;
	vis->res_id = res_id;
	vis->texture = texture;
	vis->mo = thing;

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
		int index = (vis->yscale*lightscalexmul)>>LIGHTSCALESHIFT;	// [RH]
		index = clamp(index, 0, MAXLIGHTSCALE - 1);

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


EXTERN_CVAR(sv_allowmovebob)
EXTERN_CVAR(cl_movebob)

fixed_t P_CalculateWeaponBobX(player_t* player, float scale_amount);
fixed_t P_CalculateWeaponBobY(player_t* player, float scale_amount);

//
// R_DrawPSprite
//
void R_DrawPSprite(pspdef_t* psp, unsigned flags)
{
	// decide which patch to use
#ifdef RANGECHECK
	if ( (unsigned)psp->state->sprite >= (unsigned)numsprites) {
		DPrintf ("R_DrawPSprite: invalid sprite number %i\n", psp->state->sprite);
		return;
	}
#endif
	const spritedef_t* sprdef = &sprites[psp->state->sprite];
#ifdef RANGECHECK
	if ( (psp->state->frame & FF_FRAMEMASK) >= sprdef->numframes) {
		DPrintf ("R_DrawPSprite: invalid sprite frame %i : %i\n", psp->state->sprite, psp->state->frame);
		return;
	}
#endif

	const spriteframe_t* sprframe = &sprdef->spriteframes[psp->state->frame & FF_FRAMEMASK];
	const ResourceId res_id = sprframe->resource[0];
	const Texture* texture = Res_CacheTexture(res_id);

	int frame_index = 0;
	fixed_t topoffs = texture->mOffsetY << FRACBITS;
	fixed_t sideoffs = texture->mOffsetX << FRACBITS;
	fixed_t width = texture->mWidth << FRACBITS;

	// calculate the positional offset due to weapon bobbing
	float bob_amount = ((clientside && sv_allowmovebob) || (clientside && serverside)) ? cl_movebob : 1.0f;
	fixed_t sx = P_CalculateWeaponBobX(&displayplayer(), bob_amount);
	fixed_t sy = P_CalculateWeaponBobY(&displayplayer(), bob_amount);

	// calculate edges of the shape
	fixed_t tx = sx - ((320 / 2) << FRACBITS) - sideoffs;
	int x1 = (centerxfrac + FixedMul(tx, pspritexscale)) >> FRACBITS;

	if (x1 > viewwidth)		// off the right side
		return;

	tx += width;
	int x2 = ((centerxfrac + FixedMul(tx, pspritexscale)) >> FRACBITS) - 1;

	if (x2 < 0)		// off the left side
		return;

	// store information in a vissprite
	vissprite_t vis;
	vis.res_id = res_id;
	vis.texture = texture;
	vis.mobjflags = flags;
	vis.texturemid = (BASEYCENTER << FRACBITS) + topoffs - sy;
	vis.x1 = x1 < 0 ? 0 : x1;
	vis.x2 = x2 >= viewwidth ? viewwidth-1 : x2;
	vis.xscale = pspritexscale;
	vis.yscale = pspriteyscale;
	vis.xiscale = pspritexiscale;
	vis.startfrac = 0;
	vis.translation = translationref_t();		// [RH] Use default colors
	vis.translucency = r_drawplayersprites * FRACUNIT;
	vis.mo = NULL;

	if (sprframe->flip[0])
	{
		vis.startfrac = width - 1;
		vis.xiscale *= -1;
	}

	if (vis.x1 > x1)
		vis.startfrac += vis.xiscale * (vis.x1-x1);

	if (fixedlightlev)
	{
		vis.colormap = basecolormap.with(fixedlightlev);
	}
	else if (fixedcolormap.isValid())
	{
		// fixed color
		vis.colormap = fixedcolormap;
	}
	else if (psp->state->frame & FF_FULLBRIGHT)
	{
		// full bright
		vis.colormap = basecolormap;	// [RH] use basecolormap
	}
	else
	{
		// local light
		vis.colormap = basecolormap.with(spritelights[MAXLIGHTSCALE-1]);	// [RH] add basecolormap
	}
	if (camera->player &&
		(camera->player->powers[pw_invisibility] > 4*32
		 || camera->player->powers[pw_invisibility] & 8))
	{
		// shadow draw
		vis.mobjflags = MF_SHADOW;
	}

	// Don't display the weapon sprite if using spectating without spynext
	if (consoleplayer().spectator && displayplayer_id == consoleplayer_id)
		return;

	R_DrawVisSprite(&vis, vis.x1, vis.x2);
}


//
// R_DrawPlayerSprites
//
void R_DrawPlayerSprites (void)
{
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
	foggy = level.fadeto_color[0] || level.fadeto_color[1] || level.fadeto_color[2] || level.fadeto_color[3]
				|| sec->colormap->fade;

	// [RH] set basecolormap
	basecolormap = sec->colormap->maps;

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
	mfloorclip = viewheightarray;
	mceilingclip = negonearray;

	{
		int centerhack = centery;

		centery = viewheight >> 1;
		centeryfrac = centery << FRACBITS;

		// add all active psprites
		int i;
		for (i=0, psp=camera->player->psprites; i<NUMPSPRITES; i++,psp++)
		{
			if (psp->state)
				R_DrawPSprite(psp, 0);
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

static int				vsprcount;
static vissprite_t**	spritesorter;
static int				spritesorter_size = 0;

static int STACK_ARGS sv_compare(const void *arg1, const void *arg2)
{
	int diff = (*(vissprite_t **)arg1)->depth - (*(vissprite_t **)arg2)->depth;
	if (diff == 0)
		return (*(vissprite_t **)arg2)->gzt - (*(vissprite_t **)arg1)->gzt;
	return diff;
}

void R_SortVisSprites (void)
{
	vsprcount = vissprite_p - vissprites;

	if (!vsprcount)
		return;

	if (spritesorter_size < MaxVisSprites)
	{
		delete [] spritesorter;
		spritesorter = new vissprite_t*[MaxVisSprites];
		spritesorter_size = MaxVisSprites;
	}

	for (int i = 0; i < vsprcount; i++)
		spritesorter[i] = vissprites + i;

	qsort(spritesorter, vsprcount, sizeof(vissprite_t *), sv_compare);
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
		if (ds->x1 > spr->x2 || ds->x2 < spr->x1 || (!(ds->silhouette & SIL_BOTH) && !ds->midposts))
			continue; // does not cover sprite

		r1 = MAX<int>(ds->x1, spr->x1);
		r2 = MIN<int>(ds->x2, spr->x2);

		segscale1 = MAX<int>(ds->scale1, ds->scale2);
		segscale2 = MIN<int>(ds->scale1, ds->scale2);

		// check if the seg is in front of the sprite
		if (segscale1 < spr->yscale || (segscale2 < spr->yscale && !R_PointOnSegSide(spr->gx, spr->gy, ds->curline)))
		{
			// masked mid texture?
			if (ds->midposts)
				R_RenderMaskedSegRange(ds, r1, r2);
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

	#if 0
	EXTERN_CVAR (r_drawhitboxes)
	if (r_drawhitboxes && spr->mo)
		R_DrawHitBox(spr->mo);
	#endif
}



//
// R_DrawMasked
//
void R_DrawMasked (void)
{
	drawseg_t		 *ds;

	R_SortVisSprites ();

	while (vsprcount > 0)
		R_DrawSprite(spritesorter[--vsprcount]);

	// render any remaining masked mid textures

	// Modified by Lee Killough:
	// (pointer check was originally nonportable
	// and buggy, by going past LEFT end of array):

	//		for (ds=ds_p-1 ; ds >= drawsegs ; ds--)    old buggy code

	for (ds=ds_p ; ds-- > drawsegs ; )	// new -- killough
		if (ds->midposts)
			R_RenderMaskedSegRange(ds, ds->x1, ds->x2);

	// draw the psprites on top of everything
	//	but does not draw on side views
	if (!viewangleoffset)
	{
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
	if (sector == NULL)
		return;

	fixed_t x = particle->x;
	fixed_t y = particle->y;
	fixed_t z = particle->z;
	fixed_t height = particle->size*(FRACUNIT/4);
	fixed_t width = particle->size*(FRACUNIT/4);
	fixed_t topoffs = height;
	fixed_t sideoffs = width >> 1;

	vissprite_t* vis = R_GenerateVisSprite(sector, fakeside, x, y, z, height, width, topoffs, sideoffs, false);

	if (vis == NULL)
		return;

	vis->translation = translationref_t();
	vis->startfrac = particle->color;
	vis->res_id = NO_PARTICLE;
	vis->mobjflags = particle->trans;
	vis->mo = NULL;

	// get light level
	if (fixedcolormap.isValid())
	{
		vis->colormap = fixedcolormap;
	}
	else
	{
		shaderef_t map;

		if (vis->heightsec == NULL || vis->FakeFlat == FAKED_Center)
			map = sector->colormap->maps;
		else
			map = vis->heightsec->colormap->maps;

		if (fixedlightlev)
		{
			vis->colormap = map.with(fixedlightlev);
		}
		else
		{
			int index = (vis->yscale*lightscalexmul)>>(LIGHTSCALESHIFT-1);
			int lightnum = (sector->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);

			index = clamp(index, 0, MAXLIGHTSCALE - 1);
			lightnum = clamp(lightnum, 0, LIGHTLEVELS - 1);

			vis->colormap = map.with(scalelight[lightnum][index]);
		}
	}
}

void R_DrawParticle(vissprite_t* vis)
{
	// Don't bother clipping each individual column
	int x1 = vis->x1, x2 = vis->x2;
	int y1 = MAX(vis->y1, MAX(mceilingclip[x1] + 1, mceilingclip[x2] + 1));
	int y2 = MIN(vis->y2, MIN(mfloorclip[x1] - 1, mfloorclip[x2] - 1));

	dspan.x1 = vis->x1;
	dspan.x2 = vis->x2;
	dspan.colormap = vis->colormap;
	// vis->mobjflags holds translucency level (0-255)
	dspan.translevel = (vis->mobjflags + 1) << 8;
	// vis->startfrac holds palette color index
	dspan.color = vis->startfrac;

	for (dspan.y = y1; dspan.y <= y2; dspan.y++)
		R_FillTranslucentSpan();
}

VERSION_CONTROL (r_things_cpp, "$Id$")
