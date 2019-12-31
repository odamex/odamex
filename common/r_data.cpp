// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
// Revision 1.3  1997/01/29 20:10
// DESCRIPTION:
//		Preparation of data for rendering,
//		generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------

#include "i_system.h"
#include "z_zone.h"
#include "m_alloc.h"

#include "m_swap.h"

#include "w_wad.h"
#include "resources/res_main.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#include "cmdlib.h"

#include "r_data.h"

#include "v_palette.h"
#include "v_video.h"

#include <ctype.h>
#include <cstddef>

#include <algorithm>

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
//

int 			firstflat;
int 			lastflat;
int				numflats;

int 			firstspritelump;
int 			lastspritelump;
int				numspritelumps;

int				numtextures;
texture_t** 	textures;

int*			texturewidthmask;

// needed for texture pegging
fixed_t*		textureheight;
static int*		texturecompositesize;
static short** 	texturecolumnlump;
static unsigned **texturecolumnofs;
static byte**	texturecomposite;
fixed_t*		texturescalex;
fixed_t*		texturescaley;

// for global animation
bool*			flatwarp;
byte**			warpedflats;
int*			flatwarpedwhen;
int*			flattranslation;

int*			texturetranslation;


//
// R_InitTextures
// Initializes the texture list
//	with the textures from the world map.
//
void R_InitTextures (void)
{
	maptexture_t*		mtexture;
	texture_t*			texture;
	mappatch_t* 		mpatch;
	texpatch_t* 		patch;

	int					i;
	int 				j;

	int*				maptex;
	int*				maptex2;
	int*				maptex1;

	int*				patchlookup;

	int 				totalwidth;
	int					nummappatches;
	int 				offset;
	int 				maxoff;
	int 				maxoff2;
	int					numtextures1;
	int					numtextures2;

	int*				directory;

	int					errors = 0;


	// Load the patch names from pnames.lmp.
	{
		const ResourceId pnames_res_id = Res_GetResourceId("PNAMES", global_directory_name);
		char *names = (char*)Res_LoadResource(pnames_res_id, PU_STATIC);
		char *name_p = names+4;

		nummappatches = LELONG ( *((int *)names) );
		patchlookup = new int[nummappatches];

		for (i = 0; i < nummappatches; i++)
		{
			patchlookup[i] = W_CheckNumForName (name_p + i*8);
			if (patchlookup[i] == -1)
			{
				// killough 4/17/98:
				// Some wads use sprites as wall patches, so repeat check and
				// look for sprites this time, but only if there were no wall
				// patches found. This is the same as allowing for both, except
				// that wall patches always win over sprites, even when they
				// appear first in a wad. This is a kludgy solution to the wad
				// lump namespace problem.

				patchlookup[i] = W_CheckNumForName (name_p + i*8, ns_sprites);
			}
		}

		Res_ReleaseResource(pnames_res_id);
	}

	// Load the map texture definitions from textures.lmp.
	// The data is contained in one or two lumps,
	//	TEXTURE1 for shareware, plus TEXTURE2 for commercial.
	maptex = maptex1 = (int*)Res_LoadResource("TEXTURE1", PU_STATIC);
	numtextures1 = LELONG(*maptex);
	maxoff = Res_GetResourceSize("TEXTURE1", global_directory_name);
	directory = maptex+1;

	if (Res_CheckResource("TEXTURE2", global_directory_name))
	{
		maptex2 = (int*)Res_LoadResource("TEXTURE2", PU_STATIC);
		numtextures2 = LELONG(*maptex2);
		maxoff2 = Res_GetResourceSize("TEXTURE2", global_directory_name);
	}
	else
	{
		maptex2 = NULL;
		numtextures2 = 0;
		maxoff2 = 0;
	}

	// denis - fix memory leaks
	for (i = 0; i < numtextures; i++)
	{
		delete[] texturecolumnlump[i];
		delete[] texturecolumnofs[i];
	}

	// denis - fix memory leaks
	delete[] textures;
	delete[] texturecolumnlump;
	delete[] texturecolumnofs;
	delete[] texturecomposite;
	delete[] texturecompositesize;
	delete[] texturewidthmask;
	delete[] textureheight;
	delete[] texturescalex;
	delete[] texturescaley;

	numtextures = numtextures1 + numtextures2;

	textures = new texture_t *[numtextures];
	texturecolumnlump = new short *[numtextures];
	texturecolumnofs = new unsigned int *[numtextures];
	texturecomposite = new byte *[numtextures];
	texturecompositesize = new int[numtextures];
	texturewidthmask = new int[numtextures];
	textureheight = new fixed_t[numtextures];
	texturescalex = new fixed_t[numtextures];
	texturescaley = new fixed_t[numtextures];

	totalwidth = 0;

	for (i = 0; i < numtextures; i++, directory++)
	{
		if (i == numtextures1)
		{
			// Start looking in second texture file.
			maptex = maptex2;
			maxoff = maxoff2;
			directory = maptex+1;
		}

		offset = LELONG(*directory);

		if (offset > maxoff)
			I_FatalError ("R_InitTextures: bad texture directory");

		mtexture = (maptexture_t *) ( (byte *)maptex + offset);

		texture = textures[i] = (texture_t *)
			Z_Malloc (sizeof(texture_t)
					  + sizeof(texpatch_t)*(SAFESHORT(mtexture->patchcount)-1),
					  PU_STATIC, 0);

		texture->width = SAFESHORT(mtexture->width);
		texture->height = SAFESHORT(mtexture->height);
		texture->patchcount = SAFESHORT(mtexture->patchcount);

		strncpy (texture->name, mtexture->name, 9); // denis - todo string limit?
		std::transform(texture->name, texture->name + strlen(texture->name), texture->name, toupper);

		mpatch = &mtexture->patches[0];
		patch = &texture->patches[0];

		for (j=0 ; j<texture->patchcount ; j++, mpatch++, patch++)
		{
			patch->originx = LESHORT(mpatch->originx);
			patch->originy = LESHORT(mpatch->originy);
			patch->patch = patchlookup[LESHORT(mpatch->patch)];
			if (patch->patch == -1)
			{
				Printf (PRINT_HIGH, "R_InitTextures: Missing patch in texture %s\n", texture->name);
				errors++;
			}
		}
		texturecolumnlump[i] = new short[texture->width];
		texturecolumnofs[i] = new unsigned int[texture->width];

		for (j = 1; j*2 <= texture->width; j <<= 1)
			;
		texturewidthmask[i] = j-1;

		textureheight[i] = texture->height << FRACBITS;
			
		// [RH] Special for beta 29: Values of 0 will use the tx/ty cvars
		// to determine scaling instead of defaulting to 8. I will likely
		// remove this once I finish the betas, because by then, users
		// should be able to actually create scaled textures.
		texturescalex[i] = mtexture->scalex ? mtexture->scalex << (FRACBITS - 3) : FRACUNIT;
		texturescaley[i] = mtexture->scaley ? mtexture->scaley << (FRACBITS - 3) : FRACUNIT;

		totalwidth += texture->width;
	}
	delete[] patchlookup;

	Res_ReleaseResource("TEXTURE1");
	if (Res_CheckResource("TEXTURE2", global_directory_name))
		Res_ReleaseResource("TEXTURE2");

	if (errors)
		I_FatalError ("%d errors in R_InitTextures.", errors);

	// [RH] Setup hash chains. Go from back to front so that if
	//		duplicates are found, the first one gets used instead
	//		of the last (thus mimicing the original behavior
	//		of R_CheckTextureNumForName().
	for (i = 0; i < numtextures; i++)
		textures[i]->index = -1;

	for (i = numtextures - 1; i >= 0; i--)
	{
		j = 0; //W_LumpNameHash (textures[i]->name) % (unsigned) numtextures;
		textures[i]->next = textures[j]->index;
		textures[j]->index = i;
	}

	if (clientside)		// server doesn't need to load patches ever
	{
		// Precalculate whatever possible.
		//for (i = 0; i < numtextures; i++)
		//	R_GenerateLookup (i, &errors);
	}

//	if (errors)
//		I_FatalError ("%d errors encountered during texture generation.", errors);

	// Create translation table for global animation.

	delete[] texturetranslation;

	texturetranslation = new int[numtextures+1];

	for (i = 0; i < numtextures; i++)
		texturetranslation[i] = i;
}



//
// R_InitFlats
//
void R_InitFlats (void)
{
	int i;

	firstflat = W_GetNumForName ("F_START") + 1;
	lastflat = W_GetNumForName ("F_END") - 1;

	if(firstflat >= lastflat)
		I_Error("no flats");

	numflats = lastflat - firstflat + 1;

	delete[] flattranslation;

	// Create translation table for global animation.
	flattranslation = new int[numflats+1];

	for (i = 0; i < numflats; i++)
		flattranslation[i] = i;

	delete[] flatwarp;

	flatwarp = new bool[numflats+1];
	memset (flatwarp, 0, sizeof(bool) * (numflats+1));

	delete[] warpedflats;

	warpedflats = new byte *[numflats+1];
	memset (warpedflats, 0, sizeof(byte *) * (numflats+1));

	delete[] flatwarpedwhen;

	flatwarpedwhen = new int[numflats+1];
	memset (flatwarpedwhen, 0xff, sizeof(int) * (numflats+1));
}


//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//	so the sprite does not need to be cached completely
//	just for having the header info ready during rendering.
//
void R_InitSpriteLumps (void)
{
	firstspritelump = W_GetNumForName ("S_START") + 1;
	lastspritelump = W_GetNumForName ("S_END") - 1;

	numspritelumps = lastspritelump - firstspritelump + 1;

	if(firstspritelump > lastspritelump)
		I_Error("no sprite lumps");

	// [RH] Rather than maintaining separate spritewidth, spriteoffset,
	//		and spritetopoffset arrays, this data has now been moved into
	//		the sprite frame definition and gets initialized by
	//		R_InstallSpriteLump(), so there really isn't anything to do here.
}


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
	int lastfakecmap = W_CheckNumForName("C_END");
	firstfakecmap = W_CheckNumForName("C_START");

	if (firstfakecmap == -1 || lastfakecmap == -1)
		numfakecmaps = 1;
	else
	{
		if (firstfakecmap > lastfakecmap)
			I_Error("no fake cmaps");

		numfakecmaps = lastfakecmap - firstfakecmap;
	}

	realcolormaps.colormap = (byte*)Z_Malloc(256*(NUMCOLORMAPS+1)*numfakecmaps, PU_STATIC,0);
	realcolormaps.shademap = (argb_t*)Z_Malloc(256*sizeof(argb_t)*(NUMCOLORMAPS+1)*numfakecmaps, PU_STATIC,0);

	delete[] fakecmaps;
	fakecmaps = new FakeCmap[numfakecmaps];

	R_ForceDefaultColormap("COLORMAP");

	if (numfakecmaps > 1)
	{
		const palette_t* pal = V_GetDefaultPalette();

		for (unsigned i = ++firstfakecmap, j = 1; j < numfakecmaps; i++, j++)
		{
			if (W_LumpLength(i) >= (NUMCOLORMAPS+1)*256)
			{
				byte* map = (byte*)W_CacheLumpNum(i, PU_CACHE);
				byte* colormap = realcolormaps.colormap+(NUMCOLORMAPS+1)*256*j;
				argb_t* shademap = realcolormaps.shademap+(NUMCOLORMAPS+1)*256*j;

				// Copy colormap data:
				memcpy(colormap, map, (NUMCOLORMAPS+1)*256);

				int r = pal->basecolors[*map].getr();
				int g = pal->basecolors[*map].getg();
				int b = pal->basecolors[*map].getb();

				char name[9];
				W_GetLumpName(name, i);
				fakecmaps[j].name = StdStringToUpper(name, 8);

				for (int k = 1; k < 256; k++)
				{
					r = (r + pal->basecolors[map[k]].getr()) >> 1;
					g = (g + pal->basecolors[map[k]].getg()) >> 1;
					b = (b + pal->basecolors[map[k]].getb()) >> 1;
				}
				// NOTE(jsd): This alpha value is used for 32bpp in water areas.
				argb_t color = argb_t(64, r, g, b);
				fakecmaps[j].blend_color = color;

				// Set up shademap for the colormap:
				for (int k = 0; k < 256; ++k)
					shademap[k] = alphablend1a(pal->basecolors[map[0]], color, j * (256 / numfakecmaps));
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
		int lump = W_CheckNumForName(name, ns_colormaps);
		
		if (lump != -1)
			return lump - firstfakecmap + 1;
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
	R_InitTextures();
	R_InitFlats();
	R_InitSpriteLumps();

	// haleyjd 01/28/10: also initialize tantoangle_acc table
	Table_InitTanToAngle();
}



//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (const char* name)
{
	int i = W_CheckNumForName (name, ns_flats);

	if (i == -1)	// [RH] Default flat for not found ones
		i = W_CheckNumForName ("-NOFLAT-", ns_flats);

	if (i == -1) {
		char namet[9];

		strncpy (namet, name, 8);
		namet[8] = 0;

		I_Error ("R_FlatNumForName: %s not found", namet);
	}

	return i - firstflat;
}


//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
// [RH] Rewrote this using Lee Killough's code in BOOM as an example.

void R_PrecacheLevel (void)
{
	byte *hitlist;
	int i;

	if (demoplayback)
		return;

	{
		int size = (numflats > numsprites) ? numflats : numsprites;

		hitlist = new byte[(numtextures > size) ? numtextures : size];
	}

	// Precache flats.
	/*
	memset (hitlist, 0, numflats);

	for (i = numsectors - 1; i >= 0; i--)
		hitlist[sectors[i].floor_res_id] = hitlist[sectors[i].ceiling_res_id] = 1;

	for (i = numflats - 1; i >= 0; i--)
		if (hitlist[i])
			W_CacheLumpNum (firstflat + i, PU_CACHE);
	*/

	// Precache textures.
	memset (hitlist, 0, numtextures);

	/*
	for (i = numsides - 1; i >= 0; i--)
	{
		hitlist[sides[i].toptexture] =
			hitlist[sides[i].midtexture] =
			hitlist[sides[i].bottomtexture] = 1;
	}
	*/

	// Sky texture is always present.
	// Note that F_SKY1 is the name used to
	//	indicate a sky floor/ceiling as a flat,
	//	while the sky texture is stored like
	//	a wall texture, with an episode dependend
	//	name.
	//
	// [RH] Possibly two sky textures now.
	// [ML] 5/11/06 - Not anymore!

	/*
	hitlist[sky1texture] = 1;
	hitlist[sky2texture] = 1;
	*/

	/*
	for (i = numtextures - 1; i >= 0; i--)
	{
		if (hitlist[i])
		{
			int j;
			texture_t *texture = textures[i];

			for (j = texture->patchcount - 1; j > 0; j--)
				W_CachePatch(texture->patches[j].patch, PU_CACHE);
		}
	}
	*/

	// Precache sprites.
	memset (hitlist, 0, numsprites);

	{
		AActor *actor;
		TThinkerIterator<AActor> iterator;

		while ( (actor = iterator.Next ()) )
			hitlist[actor->sprite] = 1;
	}

	for (i = numsprites - 1; i >= 0; i--)
	{
		if (hitlist[i])
			R_CacheSprite (sprites + i);
	}

	delete[] hitlist;
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

