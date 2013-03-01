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
//	Rendering of moving objects, sprites.
//
//-----------------------------------------------------------------------------


#ifndef __R_THINGS__
#define __R_THINGS__

// [RH] Particle details
struct particle_s {
	fixed_t	x,y,z;
	fixed_t velx,vely,velz;
	fixed_t accx,accy,accz;
	byte	ttl;
	byte	trans;
	byte	size;
	byte	fade;
	int		color;
	WORD	next;
	WORD	nextinsubsector;
};
typedef struct particle_s particle_t;

extern int	NumParticles;
extern int	ActiveParticles;
extern int	InactiveParticles;
extern particle_t *Particles;
extern TArray<WORD>     ParticlesInSubsec;

const WORD NO_PARTICLE = 0xffff;

#ifdef _MSC_VER
__inline particle_t *NewParticle (void)
{
	particle_t *result = NULL;
	if (InactiveParticles != NO_PARTICLE) {
		result = Particles + InactiveParticles;
		InactiveParticles = result->next;
		result->next = ActiveParticles;
		ActiveParticles = result - Particles;
	}
	return result;
}
#else
particle_t *NewParticle (void);
#endif
void R_InitParticles (void);
void R_ClearParticles (void);
void R_DrawParticleP (vissprite_t *, int, int);
void R_DrawParticleD (vissprite_t *, int, int);
void R_ProjectParticle (particle_t *, const sector_t* sector, int fakeside);
void R_FindParticleSubsectors();

extern int MaxVisSprites;

extern vissprite_t		*vissprites;
extern vissprite_t* 	vissprite_p;
extern vissprite_t		vsprsortedhead;

// Constant arrays used for psprite clipping
//	and initializing clipping.
extern int			*negonearray;
extern int			*screenheightarray;

// vars for R_DrawMaskedColumn
extern int*			mfloorclip;
extern int*			mceilingclip;
extern fixed_t		spryscale;
extern fixed_t		sprtopscreen;

extern fixed_t		pspritexscale;
extern fixed_t		pspriteyscale;
extern fixed_t		pspritexiscale;


void R_DrawMaskedColumn(tallpost_t* post);


void R_CacheSprite (spritedef_t *sprite);
void R_SortVisSprites (void);
void R_AddSprites (sector_t *sec, int lightlevel, int fakeside);
void R_AddPSprites (void);
void R_DrawSprites (void);
void R_InitSprites (const char** namelist);
void R_ClearSprites (void);
void R_DrawMasked (void);

#endif

