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
//	Sky rendering. The DOOM sky is a texture map like any
//	wall, wrapping around. 1024 columns equal 360 degrees.
//	The default sky map is 256 columns and repeats 4 times
//	on a 320 screen.
//	
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

// Needed for FRACUNIT.
#include "m_fixed.h"
#include "r_data.h"
#include "c_cvars.h"
#include "g_level.h"
#include "r_sky.h"
#include "gi.h"
#include "w_wad.h"

extern int *texturewidthmask;

EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(cl_mouselook)

//
// sky mapping
//
int 		skyflatnum;
int 		sky1texture,	sky2texture;
fixed_t		skytexturemid;
fixed_t		skyscale;
int			skystretch;
fixed_t		skyiscale;

int			sky1shift,		sky2shift;
fixed_t		sky1pos=0,		sky1speed=0;
fixed_t		sky2pos=0,		sky2speed=0;

CVAR_FUNC_IMPL(r_stretchsky)
{
	R_InitSkyMap ();
}

char SKYFLATNAME[8] = "F_SKY1";

extern "C" int detailxshift, detailyshift;
extern fixed_t freelookviewheight;

//
//
// R_InitSkyMap
//
// Called whenever the view size changes.
//
// [ML] 5/11/06 - Remove sky2 stuffs
// [ML] 3/16/10 - Bring it back!

void R_InitSkyMap ()
{
	texpatch_t *texpatch;
	patch_t *wpatch;
	int p_height, t_height,i,count;

	if (textureheight == NULL)
		return;

	// [SL] 2011-11-30 - Don't run if we don't know what sky texture to use
	if (gamestate != GS_LEVEL)
		return;

	if (sky2texture && textureheight[sky1texture] != textureheight[sky2texture])
	{
		Printf (PRINT_HIGH,"\x1f+Both sky textures must be the same height.\x1f-\n");
		sky2texture = sky1texture;
	}

	t_height = textures[sky1texture]->height;
	p_height = 0;

	count = textures[sky1texture]->patchcount;
	texpatch = &(textures[sky1texture]->patches[0]);
	
	// Find the tallest patch in the texture
	for(i = 0; i < count; i++, texpatch++)
	{
		wpatch = W_CachePatch(texpatch->patch);
		if(wpatch->height() > p_height)
			p_height = SAFESHORT(wpatch->height());
	}

	textures[sky1texture]->height = MAX(t_height,p_height);
	textureheight[sky1texture] = textures[sky1texture]->height << FRACBITS;
	
	skystretch = 0;

	if (textureheight[sky1texture] <= (128 << FRACBITS))
	{
		skytexturemid = 200/2*FRACUNIT;
		skystretch = (r_stretchsky == 1) || (r_stretchsky == 2 && sv_freelook && cl_mouselook);
	}
	else
	{
		skytexturemid = 199 * FRACUNIT;
	}
	
	if (viewwidth && viewheight)
	{
		skyiscale = (200*FRACUNIT) / (((freelookviewheight<<detailxshift) * viewwidth) / (viewwidth<<detailxshift));
		skyscale = ((((freelookviewheight<<detailxshift) * viewwidth) / (viewwidth<<detailxshift)) << FRACBITS) /(200);

		skyiscale = FixedMul (skyiscale, FixedDiv (FieldOfView, 2048));
		skyscale = FixedMul (skyscale, FixedDiv (2048, FieldOfView));
	}

	// The DOOM sky map is 256*128*4 maps.
	// The Heretic sky map is 256*200*4 maps.
	sky1shift = 22+skystretch-16;
	sky2shift = 22+skystretch-16;	
	if (texturewidthmask[sky1texture] >= 127)
		sky1shift -= skystretch;
	if (texturewidthmask[sky2texture] >= 127)
		sky2shift -= skystretch;
}

VERSION_CONTROL (r_sky_cpp, "$Id$")
