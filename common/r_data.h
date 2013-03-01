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
//  Refresh module, data I/O, caching, retrieval of graphics
//  by name.
//
//-----------------------------------------------------------------------------


#ifndef __R_DATA__
#define __R_DATA__

#include "r_defs.h"
#include "r_state.h"


// On the Alpha, accessing the shorts directly if they aren't aligned on a
// 4-byte boundary causes unaligned access warnings. Why it does this it at
// all and only while initing the textures is beyond me.


#ifdef ALPHA
#define SAFESHORT(s)	((short)(((byte *)&(s))[0] + ((byte *)&(s))[1] * 256))
#else
#define SAFESHORT(s)	LESHORT(s)
#endif


// A single patch from a texture definition,
//	basically a rectangular area within
//	the texture rectangle.
typedef struct
{
	// Block origin (always UL),
	// which has already accounted
	// for the internal origin of the patch.
	int 		originx;
	int 		originy;
	int 		patch;
} texpatch_t;


// A maptexturedef_t describes a rectangular texture,
//	which is composed of one or more mappatch_t structures
//	that arrange graphic patches.
typedef struct
{
	// Keep name for switch changing, etc.
	char		name[9];
	short		width;
	short		height;

	// [RH] Use a hash table similar to the one now used
	//		in w_wad.c, thus speeding up level loads.
	//		(possibly quite considerably for larger levels)
	int			index;
	int			next;

	// All the patches[patchcount]
	//	are drawn back to front into the cached texture.
	short		patchcount;
	texpatch_t	patches[1];

} texture_t;


extern texture_t **textures;
extern byte* textureheightmask;
extern fixed_t* texturescalex;
extern fixed_t* texturescaley;

// Retrieve column data for span blitting.
tallpost_t* R_GetColumn(int tex, int col);
byte* R_GetColumnData(int tex, int col);


// I/O, setting up the stuff.
void R_InitData (void);
void R_PrecacheLevel (void);


// Retrieval.
// Floor/ceiling opaque texture tiles,
// lookup by name. For animation?
int R_FlatNumForName (const char *name);
inline int R_FlatNumForName (const byte *name) { return R_FlatNumForName ((const char *)name); }


// Called by P_Ticker for switches and animations,
// returns the texture number for the texture name.
int R_TextureNumForName (const char *name);
int R_CheckTextureNumForName (const char *name);

inline int R_TextureNumForName (const byte *name) { return R_TextureNumForName ((const char *)name); }
inline int R_CheckTextureNumForName (const byte *name) { return R_CheckTextureNumForName ((const char *)name); }

int R_ColormapNumForName(const char *name);		// killough 4/4/98
void R_ReinitColormap();
void R_ForceDefaultColormap (const char *name);
void R_SetDefaultColormap (const char *name);	// [RH] change normal fadetable
unsigned int R_BlendForColormap (int map);		// [RH] return calculated blend for a colormap
extern shademap_t realcolormaps;				// [RH] make the colormaps externally visible
extern size_t numfakecmaps;

int R_FindSkin (const char *name);	// [RH] Find a skin

unsigned int SlopeDiv(unsigned int num, unsigned int den);

#endif


