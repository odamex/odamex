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


// I/O, setting up the stuff.
void R_InitData (void);
void R_PrecacheLevel (void);

void R_InitColormaps();
void R_ShutdownColormaps();

int R_ColormapNumForName(const char *name);		// killough 4/4/98
void R_ReinitColormap();
void R_ForceDefaultColormap (const char *name);
void R_SetDefaultColormap (const char *name);	// [RH] change normal fadetable

argb_t R_BlendForColormap(unsigned int mapnum);		// [RH] return calculated blend for a colormap
int R_ColormapForBlend(const argb_t blend_color);	// [SL] return colormap that has the blend color

extern shademap_t realcolormaps;				// [RH] make the colormaps externally visible
extern size_t numfakecmaps;

unsigned int SlopeDiv(unsigned int num, unsigned int den);

#endif