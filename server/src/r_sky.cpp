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
//  Sky rendering. The DOOM sky is a texture map like any
//  wall, wrapping around. A 1024 columns equal 360 degrees.
//  The default sky map is 256 columns and repeats 4 times
//  on a 320 screen?
//  
//-----------------------------------------------------------------------------


#include <stdio.h>

#include "r_data.h"
#include "resources/res_resourceid.h"
#include "resources/res_main.h"

// [ML] 5/11/06 - Remove sky2
int 		skyflatnum;
int 		sky1texture, sky2texture;

fixed_t		sky1pos=0,		sky1speed=0;

char SKYFLATNAME[8] = "F_SKY1";

//
// R_ResourceIdIsSkyFlat
//
// Returns true if the given ResourceId represents the sky.
//
bool R_ResourceIdIsSkyFlat(const ResourceId res_id)
{
	const ResourceId sky_res_id = Res_GetTextureResourceId(SKYFLATNAME, FLOOR);
	return res_id == sky_res_id;
}


//
// R_SetSkyTextures
//
// Loads the sky textures and re-initializes the sky map lookup tables.
//
void R_SetSkyTextures(const char* sky1_name, const char* sky2_name)
{
}


VERSION_CONTROL (r_sky_cpp, "$Id$")

