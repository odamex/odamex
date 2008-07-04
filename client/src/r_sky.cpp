// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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

EXTERN_CVAR(allowfreelook)

extern int *texturewidthmask;

//
// sky mapping
//
int 		skyflatnum;
int 		skytexture;					//		[ML] 5/11/06 - remove sky2 remenants
fixed_t		skytexturemid;
fixed_t		skyscale;
int			skystretch;
fixed_t		skyheight;					
fixed_t		skyiscale;

int			skyshift;					//		[ML] 5/11/06 - remove sky2 remenants
fixed_t		skypos=0,		skyspeed=0;


EXTERN_CVAR (r_stretchsky)

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

void R_InitSkyMap ()
{
	if (textureheight == NULL)
		return;

	if (textureheight[skytexture] <= (128 << FRACBITS))
	{
                skytexturemid = 100*FRACUNIT;
                skystretch = (r_stretchsky && allowfreelook);
	}
	else
	{
		skytexturemid = 100*FRACUNIT;
		skystretch = 0;
	}
	skyheight = textureheight[skytexture] << skystretch;

	if (viewwidth && viewheight)
	{
		skyiscale = (200*FRACUNIT) / (((freelookviewheight<<detailxshift) * viewwidth) / (viewwidth<<detailxshift));
		skyscale = ((((freelookviewheight<<detailxshift) * viewwidth) / (viewwidth<<detailxshift)) << FRACBITS) /(200);

		skyiscale = FixedMul (skyiscale, FixedDiv (clipangle, ANG45));
		skyscale = FixedMul (skyscale, FixedDiv (ANG45, clipangle));
	}

	// The sky map is 256*128*4 maps.
	skyshift = 22+skystretch-16;
	if (texturewidthmask[skytexture] >= 256*2-1)
		skyshift -= skystretch;
}

VERSION_CONTROL (r_sky_cpp, "$Id$")


