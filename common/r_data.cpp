// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//		Preparation of data for rendering,
//		generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

#include "z_zone.h"

#include "resources/res_main.h"
#include "resources/res_texture.h"

#include "doomdef.h"
#include "r_local.h"
#include "cmdlib.h"

#include "v_palette.h"
#include "v_video.h"


struct FakeCmap
{
	std::string name;
	argb_t blend_color;
};

static FakeCmap* fakecmaps = NULL;

size_t numfakecmaps;
int firstfakecmap;
shademap_t realcolormaps;


void R_ForceDefaultColormap(const char* name)
{
	const byte* data = (byte*)Res_LoadResource(name, PU_CACHE);
	memcpy(realcolormaps.colormap, data, (NUMCOLORMAPS+1)*256);

#if 0
	// Setup shademap to mirror colormapped colors:
	for (int m = 0; m < (NUMCOLORMAPS+1); ++m)
		for (int c = 0; c < 256; ++c)
			realcolormaps.shademap[m*256+c] = V_Palette.shade(realcolormaps.colormap[m*256+c]);
#else
	BuildDefaultShademap(V_GetDefaultPalette(), realcolormaps);
#endif

	fakecmaps[0].name = StdStringToUpper(name, 8); 	// denis - todo - string limit?
	fakecmaps[0].blend_color = argb_t(0, 255, 255, 255);
}

void R_SetDefaultColormap(const char* name)
{
	if (strnicmp(fakecmaps[0].name.c_str(), name, 8) != 0)
		R_ForceDefaultColormap(name);
}

void R_ReinitColormap()
{
	if (fakecmaps == NULL)
		return;

	std::string name = fakecmaps[0].name;
	if (name.empty())
		name = "COLORMAP";

	R_ForceDefaultColormap(name.c_str());
}


//
// R_ShutdownColormaps
//
// Frees the memory allocated specifically for the colormaps.
//
void R_ShutdownColormaps()
{
	if (realcolormaps.colormap)
	{
		Z_Free(realcolormaps.colormap);
		realcolormaps.colormap = NULL;
	}

	if (realcolormaps.shademap)
	{
		Z_Free(realcolormaps.shademap);
		realcolormaps.shademap = NULL;
	}

	if (fakecmaps)
	{
		delete [] fakecmaps;
		fakecmaps = NULL;
	}

}

//
// R_InitColormaps
//
void R_InitColormaps()
{
	// [RH] Try and convert BOOM colormaps into blending values.
	//		This is a really rough hack, but it's better than
	//		not doing anything with them at all (right?)

	ResourcePathList paths = Res_ListResourceDirectory(colormaps_directory_name);
	numfakecmaps = std::max<int>(paths.size(), 1);

	realcolormaps.colormap = (byte*)Z_Malloc(256*(NUMCOLORMAPS+1)*numfakecmaps, PU_STATIC,0);
	realcolormaps.shademap = (argb_t*)Z_Malloc(256*sizeof(argb_t)*(NUMCOLORMAPS+1)*numfakecmaps, PU_STATIC,0);

	delete[] fakecmaps;
	fakecmaps = new FakeCmap[numfakecmaps];

	R_ForceDefaultColormap("COLORMAP");

	if (numfakecmaps > 1)
	{
		const palette_t* pal = V_GetDefaultPalette();

		for (size_t i = 1; i < paths.size(); i++)
		{
			const ResourcePath& path = paths[i];
			const ResourceId res_id = Res_GetResourceId(path);
			if (Res_GetResourceSize(res_id) >= (NUMCOLORMAPS+1)*256)
			{
				const uint8_t* map = (uint8_t*)Res_LoadResource(res_id, PU_CACHE);
				byte* colormap = realcolormaps.colormap+(NUMCOLORMAPS+1)*256*i;
				argb_t* shademap = realcolormaps.shademap+(NUMCOLORMAPS+1)*256*i;

				// Copy colormap data:
				memcpy(colormap, map, (NUMCOLORMAPS+1)*256);

				int r = pal->basecolors[*map].getr();
				int g = pal->basecolors[*map].getg();
				int b = pal->basecolors[*map].getb();

				fakecmaps[i].name = StdStringToUpper(path.last());

				for (int k = 1; k < 256; k++)
				{
					r = (r + pal->basecolors[map[k]].getr()) >> 1;
					g = (g + pal->basecolors[map[k]].getg()) >> 1;
					b = (b + pal->basecolors[map[k]].getb()) >> 1;
				}
				// NOTE(jsd): This alpha value is used for 32bpp in water areas.
				argb_t color = argb_t(64, r, g, b);
				fakecmaps[i].blend_color = color;

				// Set up shademap for the colormap:
				for (int k = 0; k < 256; ++k)
					shademap[k] = alphablend1a(pal->basecolors[map[0]], color, i * (256 / numfakecmaps));
			}
		}
	}
}


//
// R_ColormapNumForname
//
// [RH] Returns an index into realcolormaps. Multiply it by
//		256*(NUMCOLORMAPS+1) to find the start of the colormap to use.
//
// COLORMAP always returns 0.
//
int R_ColormapNumForName(const char* name)
{
	if (strnicmp(name, "COLORMAP", 8) != 0)
	{
		for (size_t i = 0; i < numfakecmaps; i++)
			if (strnicmp(name, fakecmaps[i].name.c_str(), 8) == 0)
				return i;
	}
	return 0;
}


//
// R_BlendForColormap
//
// Returns a blend value to approximate the given colormap index number.
// Invalid values return the color white with 0% opacity.
//
argb_t R_BlendForColormap(unsigned int index)
{
	if (index > 0 && index < numfakecmaps)
		return fakecmaps[index].blend_color;

	return argb_t(0, 255, 255, 255);
}


//
// R_ColormapForBlend
//
// Returns the colormap index number that has the given blend color value.
//
int R_ColormapForBlend(const argb_t blend_color)
{
	for (unsigned int i = 1; i < numfakecmaps; i++)
		if (fakecmaps[i].blend_color == blend_color)
			return i;
	return 0;
}

//
// R_InitData
// Locates all the lumps
//	that will be used by all views
// Must be called after W_Init.
//
void R_InitData()
{
	// haleyjd 01/28/10: also initialize tantoangle_acc table
	Table_InitTanToAngle();
}


//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
// [RH] Rewrote this using Lee Killough's code in BOOM as an example.

void R_PrecacheLevel()
{
	DPrintf("Level Pre-Cache start\n");

	// Cache floor & ceiling textures
	for (int i = numsectors - 1; i >= 0; i--)
	{
		Res_CacheTexture(sectors[i].floor_res_id);
		Res_CacheTexture(sectors[i].ceiling_res_id);
	}

	// Cache wall textures
	for (int i = numsides - 1; i >= 0; i--)
	{
		Res_CacheTexture(sides[i].toptexture);
		Res_CacheTexture(sides[i].midtexture);
		Res_CacheTexture(sides[i].bottomtexture);
	}

	// TODO: Cache sky textures

	// Cache sprites
	AActor* actor;
	TThinkerIterator<AActor> iterator;
	while ( (actor = iterator.Next ()) )
		R_CacheSprite(sprites + actor->sprite);

	DPrintf("Level Pre-Cache end\n");
}


// Utility function,
//	called by R_PointToAngle.
unsigned int SlopeDiv (unsigned int num, unsigned int den)
{
	unsigned int ans;

	if (den < 512)
		return SLOPERANGE;

	ans = (num << 3) / (den >> 8);

	return ans <= SLOPERANGE ? ans : SLOPERANGE;
}

VERSION_CONTROL (r_data_cpp, "$Id$")

