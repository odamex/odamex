// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"


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

fixed_t			dc_textureheight;


//
// R_CalculateNewPatchSize
//
// Helper function for converting raw patches that use post_t into patches
// that use tallpost_t. Returns the lump size of the converted patch.
//
size_t R_CalculateNewPatchSize(patch_t *patch, size_t length)
{
	if (!patch)
		return 0;

	// sanity check to see if the postofs array fits in the patch lump
	if (length < patch->width() * sizeof(unsigned int))
		return 0;

	int numposts = 0, numpixels = 0;
	unsigned int *postofs = (unsigned int *)((byte*)patch + 8);

	for (int i = 0; i < patch->width(); i++)
	{
		size_t ofs = LELONG(postofs[i]);

		// check that the offset is valid
		if (ofs >= length)
			return 0;

		post_t *post = (post_t*)((byte*)patch + ofs);

		while (post->topdelta != 0xFF)
		{
			if (ofs + post->length >= length)
				return 0;		// patch is corrupt

			numposts++;
			numpixels += post->length;
			post = (post_t*)((byte*)post + post->length + 4);
		}
	}

	// 8 byte patch header
	// 4 * width bytes for column offset table
	// 4 bytes per post for post header
	// 1 byte per pixel
	// 2 bytes per column for termination
	return 8 + 4 * patch->width() + 4 * numposts + numpixels + 2 * patch->width();
}

//
// R_ConvertPatch
//
// Converts a patch that uses post_t posts into a patch that uses tallpost_t.
//
void R_ConvertPatch(patch_t *newpatch, patch_t *rawpatch)
{
	if (!rawpatch || !newpatch)
		return;

	memcpy(newpatch, rawpatch, 8);		// copy the patch header

	unsigned *rawpostofs = (unsigned*)((byte*)rawpatch + 8);
	unsigned *newpostofs = (unsigned*)((byte*)newpatch + 8);

	unsigned curofs = 8 + 4 * rawpatch->width();	// keep track of the column offset

	for (int i = 0; i < rawpatch->width(); i++)
	{
		int abs_offset = 0;
			
		newpostofs[i] = LELONG(curofs);		// write the new offset for this column
		post_t *rawpost = (post_t*)((byte*)rawpatch + LELONG(rawpostofs[i]));
		tallpost_t *newpost = (tallpost_t*)((byte*)newpatch + curofs);

		while (rawpost->topdelta != 0xFF)
		{
			// handle DeePsea tall patches where topdelta is treated as a relative
			// offset instead of an absolute offset
			if (rawpost->topdelta <= abs_offset)
				abs_offset += rawpost->topdelta;
			else
				abs_offset = rawpost->topdelta;
				
			// watch for column overruns
			int length = rawpost->length;
			if (abs_offset + length > rawpatch->height())
				length = rawpatch->height() - abs_offset;

			// copy the pixels in the post
			memcpy(newpost->data(), (byte*)(rawpost) + 3, length);
				
			newpost->topdelta = abs_offset;
			newpost->length = length;

			curofs += newpost->length + 4;
			rawpost = (post_t*)((byte*)rawpost + rawpost->length + 4);
			newpost = newpost->next();
		}

		newpost->writeend();
		curofs += 2;
	}
}

//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//	it counts the number of composite columns
//	required in the texture and allocates space
//	for a column directory and any new columns.
// The directory will simply point inside other patches
//	if there is only one patch in a given column,
//	but any columns with multiple patches
//	will have new column_ts generated.
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//

void R_DrawColumnInCache(const tallpost_t *post, byte *cache,
						  int originy, int cacheheight, byte *marks)
{
	while (!post->end())
	{
		int count = post->length;
		int position = post->topdelta + originy;

		if (position < 0)
		{
			count += position;
			position = 0;
		}

		if (position + count > cacheheight)
			count = cacheheight - position;

		if (count > 0)
		{
			memcpy(cache + position, post->data(), count);

			// killough 4/9/98: remember which cells in column have been drawn,
			// so that column can later be converted into a series of posts, to
			// fix the Medusa bug.

			memset(marks + position, 0xFF, count);
		}

		post = post->next();
	}
}

//
// R_GenerateComposite
// Using the texture definition,
//	the composite texture is created from the patches,
//	and each column is cached.
//
// Rewritten by Lee Killough for performance and to fix Medusa bug

void R_GenerateComposite (int texnum)
{
	byte *block = (byte *)Z_Malloc (texturecompositesize[texnum], PU_STATIC,
						   (void **) &texturecomposite[texnum]);
	texture_t *texture = textures[texnum];

	// Composite the columns together.
	texpatch_t *texpatch = texture->patches;
	short *collump = texturecolumnlump[texnum];

	// killough 4/9/98: marks to identify transparent regions in merged textures
	byte *marks = new byte[texture->width * texture->height];
	memset(marks, 0, texture->width * texture->height);

	for (int i = texture->patchcount; --i >=0; texpatch++)
	{
		patch_t *patch = W_CachePatch(texpatch->patch);
		int x1 = texpatch->originx, x2 = x1 + patch->width();
		const int *cofs = patch->columnofs-x1;
		if (x1<0)
			x1 = 0;
		if (x2 > texture->width)
			x2 = texture->width;

		for (; x1 < x2 ; x1++)
		{
			if (collump[x1] == -1)			// Column has multiple patches?
			{
				// killough 1/25/98, 4/9/98: Fix medusa bug.
				tallpost_t *srcpost = (tallpost_t*)((byte*)patch + LELONG(cofs[x1]));
				tallpost_t *destpost = (tallpost_t*)(block + texturecolumnofs[texnum][x1]);

				R_DrawColumnInCache(srcpost, destpost->data(), texpatch->originy, texture->height,
									marks + x1 * texture->height);
			}
		}
	}

	// killough 4/9/98: Next, convert multipatched columns into true columns,
	// to fix Medusa bug while still allowing for transparent regions.

	byte *tmpdata = new byte[texture->height];		// temporary post data
	for (int i = 0; i < texture->width; i++)
	{
		if (collump[i] != -1)	// process only multipatched columns
			continue;

		tallpost_t *post = (tallpost_t *)(block + texturecolumnofs[texnum][i]);
		const byte *mark = marks + i * texture->height;
		int j = 0;

		// save column in temporary so we can shuffle it around
		memcpy(tmpdata, post->data(), texture->height);

		// reconstruct the column by scanning transparency marks
		while (true)
		{
			while (j < texture->height && !mark[j]) // skip transparent pixels
				j++;
			if (j >= texture->height) 				// if at end of column
			{
				post->writeend();					// end-of-column marker
				break;
			}

			post->topdelta = j;						// starting offset of post

			// count opaque pixels
			for (post->length = 0; j < texture->height && mark[j]; j++)
				post->length++;

			// copy opaque pixels from the temporary back into the column
			memcpy(post->data(), tmpdata + post->topdelta, post->length);	
			post = post->next();
		}
	}

	delete [] marks;
	delete [] tmpdata;

	// Now that the texture has been built in column cache,
	// it is purgable from zone memory.

	Z_ChangeTag(block, PU_CACHE);
}

//
// R_GenerateLookup
//
// Rewritten by Lee Killough for performance and to fix Medusa bug
//

static void R_GenerateLookup(int texnum, int *const errors)
{
	const texture_t *texture = textures[texnum];

	// Composited texture not created yet.

	short *collump = texturecolumnlump[texnum];

	// killough 4/9/98: keep count of posts in addition to patches.
	// Part of fix for medusa bug for multipatched 2s normals.
	unsigned short *patchcount = new unsigned short[texture->width];
	unsigned short *postcount = new unsigned short[texture->width];

	memset(patchcount, 0, sizeof(unsigned short) * texture->width);	
	memset(postcount, 0, sizeof(unsigned short) * texture->width);	

	const texpatch_t *texpatch = texture->patches;

	for (int i = 0; i < texture->patchcount; i++)
	{
		const int patchnum = texpatch->patch;
		const patch_t *patch = W_CachePatch(patchnum);
		int x1 = texpatch++->originx, x2 = x1 + patch->width(), x = x1;
		const int *cofs = patch->columnofs-x1;

		if (x2 > texture->width)
			x2 = texture->width;
		if (x1 < 0)
			x = 0;
		for (; x < x2; x++)
		{
			// killough 4/9/98: keep a count of the number of posts in column,
			// to fix Medusa bug while allowing for transparent multipatches.

			const tallpost_t *post = (tallpost_t*)((byte*)patch + LELONG(cofs[x]));
	
			// NOTE: this offset will be rewritten later if a composite is generated
			// for this texture (eg, there's more than one patch)	
			texturecolumnofs[texnum][x] = (byte *)post - (byte *)patch;

			patchcount[x]++;
			collump[x] = patchnum;

			while (!post->end())
			{
				postcount[x]++;
				post = post->next();
			}
		}
	}

	// Now count the number of columns that are covered by more than one patch.
	// Fill in the lump / offset, so columns with only a single patch are all done.

	texturecomposite[texnum] = 0;
	int csize = 0;

	// [RH] Always create a composite texture for multipatch textures
	// or tall textures in order to keep things simpler.	
	bool needcomposite = (texture->patchcount > 1 || texture->height > 254);

	// [SL] Check for columns without patches.
	// If a texture has columns without patches, generate a composite for
	// the texture, which will create empty posts and prevent crashes.
	for (int x = 0; x < texture->width && !needcomposite; x++)
	{
		if (patchcount[x] == 0)
			needcomposite = true;
	}

	if (needcomposite)
	{
		int x = texture->width;
		while (--x >= 0)
		{
			// killough 1/25/98, 4/9/98:
			//
			// Fix Medusa bug, by adding room for column header
			// and trailer bytes for each post in merged column.
			// For now, just allocate conservatively 4 bytes
			// per post per patch per column, since we don't
			// yet know how many posts the merged column will
			// require, and it's bounded above by this limit.

			collump[x] = -1;				// mark lump as multipatched

			texturecolumnofs[texnum][x] = csize;

			// 4 header bytes per post + column height + 2 byte terminator
			csize += 4 * postcount[x] + 2 + texture->height;
		}
	}
	
	texturecompositesize[texnum] = csize;
	
	delete [] patchcount;
	delete [] postcount;
}

//
// R_GetColumn
//
tallpost_t* R_GetColumn(int tex, int col)
{
	int lump;
	int ofs;

	col &= texturewidthmask[tex];
	lump = texturecolumnlump[tex][col];
	ofs = texturecolumnofs[tex][col];
	dc_textureheight = textureheight[tex];

	if (lump > 0)
		return (tallpost_t*)((byte *)W_CachePatch(lump,PU_CACHE) + ofs);

	if (!texturecomposite[tex])
		R_GenerateComposite (tex);

	return (tallpost_t*)(texturecomposite[tex] + ofs);
}


byte* R_GetColumnData(int tex, int col)
{
	return R_GetColumn(tex, col)->data();
}


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
		char *names = (char *)W_CacheLumpName ("PNAMES", PU_STATIC);
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
		Z_Free (names);
	}

	// Load the map texture definitions from textures.lmp.
	// The data is contained in one or two lumps,
	//	TEXTURE1 for shareware, plus TEXTURE2 for commercial.
	maptex = maptex1 = (int *)W_CacheLumpName ("TEXTURE1", PU_STATIC);
	numtextures1 = LELONG(*maptex);
	maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
	directory = maptex+1;

	if (W_CheckNumForName ("TEXTURE2") != -1)
	{
		maptex2 = (int *)W_CacheLumpName ("TEXTURE2", PU_STATIC);
		numtextures2 = LELONG(*maptex2);
		maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
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

	Z_Free (maptex1);
	if (maptex2)
		Z_Free (maptex2);

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
		for (i = 0; i < numtextures; i++)
			R_GenerateLookup (i, &errors);
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


static struct FakeCmap {
	char name[9];
	unsigned int blend;
} *fakecmaps;
size_t numfakecmaps;
int firstfakecmap;
shademap_t realcolormaps;
int lastusedcolormap;

void R_ForceDefaultColormap(const char *name)
{
	byte *data = (byte *)W_CacheLumpName (name, PU_CACHE);

	memcpy (realcolormaps.colormap, data, (NUMCOLORMAPS+1)*256);

#if 0
	// Setup shademap to mirror colormapped colors:
	for (int m = 0; m < (NUMCOLORMAPS+1); ++m)
		for (int c = 0; c < 256; ++c)
			realcolormaps.shademap[m*256+c] = V_Palette.shade(realcolormaps.colormap[m*256+c]);
#else
	BuildDefaultShademap (GetDefaultPalette(), realcolormaps);
#endif

	strncpy (fakecmaps[0].name, name, 9); // denis - todo - string limit?
	std::transform(fakecmaps[0].name, fakecmaps[0].name + strlen(fakecmaps[0].name), fakecmaps[0].name, toupper);
	fakecmaps[0].blend = 0;
}

void R_SetDefaultColormap (const char *name)
{
	if (strnicmp (fakecmaps[0].name, name, 8))
	{
		R_ForceDefaultColormap(name);
	}
}

void R_ReinitColormap()
{
	if (fakecmaps == NULL)
		return;

	const char *name = fakecmaps[0].name;

	if (name[0] == 0)
		name = "COLORMAP";

	R_ForceDefaultColormap(name);
}

//
// R_InitColormaps
//
void R_InitColormaps (void)
{
	// [RH] Try and convert BOOM colormaps into blending values.
	//		This is a really rough hack, but it's better than
	//		not doing anything with them at all (right?)
	int lastfakecmap = W_CheckNumForName ("C_END");
	firstfakecmap = W_CheckNumForName ("C_START");

	if (firstfakecmap == -1 || lastfakecmap == -1)
		numfakecmaps = 1;
	else
	{
		if(firstfakecmap > lastfakecmap)
			I_Error("no fake cmaps");

		numfakecmaps = lastfakecmap - firstfakecmap;
	}

	realcolormaps.colormap = (byte *)Z_Malloc (256*(NUMCOLORMAPS+1)*numfakecmaps,PU_STATIC,0);
	realcolormaps.shademap = (argb_t *)Z_Malloc (256*sizeof(argb_t)*(NUMCOLORMAPS+1)*numfakecmaps,PU_STATIC,0);
	fakecmaps = (FakeCmap *)Z_Malloc (sizeof(*fakecmaps) * numfakecmaps, PU_STATIC, 0);

	fakecmaps[0].name[0] = 0;
	R_ForceDefaultColormap ("COLORMAP");

	if (numfakecmaps > 1)
	{
		int i;
		size_t j;
		palette_t *pal = GetDefaultPalette ();
		shaderef_t defpal = shaderef_t(&pal->maps, 0);

		for (i = ++firstfakecmap, j = 1; j < numfakecmaps; i++, j++)
		{
			if (W_LumpLength (i) >= (NUMCOLORMAPS+1)*256)
			{
				int k, r, g, b;
				byte *map = (byte *)W_CacheLumpNum (i, PU_CACHE);

				byte  *colormap = realcolormaps.colormap+(NUMCOLORMAPS+1)*256*j;
				argb_t *shademap = realcolormaps.shademap+(NUMCOLORMAPS+1)*256*j;

				// Copy colormap data:
				memcpy (colormap, map, (NUMCOLORMAPS+1)*256);

				if(pal->basecolors)
				{
					r = RPART(pal->basecolors[*map]);
					g = GPART(pal->basecolors[*map]);
					b = BPART(pal->basecolors[*map]);

					W_GetLumpName (fakecmaps[j].name, i);
					for (k = 1; k < 256; k++) {
						r = (r + RPART(pal->basecolors[map[k]])) >> 1;
						g = (g + GPART(pal->basecolors[map[k]])) >> 1;
						b = (b + BPART(pal->basecolors[map[k]])) >> 1;
					}
					// NOTE(jsd): This alpha value is used for 32bpp in water areas.
					fakecmaps[j].blend = MAKEARGB (64, r, g, b);

					// Set up shademap for the colormap:
					for (k = 0; k < 256; ++k)
					{
						argb_t c = pal->basecolors[map[0]];
						shademap[k] = alphablend1a(c, MAKERGB(r,g,b), j * (256 / numfakecmaps));
					}
				}
				else
				{
					// Set up shademap for the colormap:
					for (k = 0; k < 256; ++k)
					{
						shademap[k] = defpal.shade(colormap[k]);
					}
				}
			}
		}
	}
}

// [RH] Returns an index into realcolormaps. Multiply it by
//		256*(NUMCOLORMAPS+1) to find the start of the colormap to use.
//		WATERMAP is an exception and returns a blending value instead.
int R_ColormapNumForName (const char *name)
{
	int lump, blend = 0;

	if (strnicmp (name, "COLORMAP", 8))
	{	// COLORMAP always returns 0
		if (-1 != (lump = W_CheckNumForName (name, ns_colormaps)) )
			blend = lump - firstfakecmap + 1;
		else if (!strnicmp (name, "WATERMAP", 8))
			blend = MAKEARGB (128,0,0x4f,0xa5);
	}

	return blend;
}

unsigned int R_BlendForColormap (int map)
{
	return APART(map) ? map :
		   (unsigned)map < numfakecmaps ? fakecmaps[map].blend : 0;
}

//
// R_InitData
// Locates all the lumps
//	that will be used by all views
// Must be called after W_Init.
//
void R_InitData (void)
{
	R_InitColormaps ();
	R_InitTextures ();
	R_InitFlats ();
	R_InitSpriteLumps ();

	// haleyjd 01/28/10: also initialize tantoangle_acc table
	Table_InitTanToAngle ();
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
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int R_CheckTextureNumForName (const char *name)
{
	unsigned char uname[9];
	int  i;

	// "NoTexture" marker.
	if (name[0] == '-')
		return 0;

	// [RH] Use a hash table instead of linear search
	strncpy ((char *)uname, name, 9); // denis - todo - string limit?
	std::transform(uname, uname + sizeof(uname), uname, toupper);

	i = textures[/*W_LumpNameHash (uname) % (unsigned) numtextures*/0]->index; // denis - todo - replace with map<>

	while (i != -1) {
		if (!strncmp (textures[i]->name, (char *)uname, 8))
			break;
		i = textures[i]->next;
	}

	return i;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//	aborts with error message.
//
int R_TextureNumForName (const char *name)
{
	int i;

	i = R_CheckTextureNumForName (name);

	if (i==-1) {
		char namet[9];
		strncpy (namet, name, 8);
		namet[8] = 0;
		//I_Error ("R_TextureNumForName: %s not found", namet);
		// [RH] Return empty texture if it wasn't found.
		Printf (PRINT_HIGH, "Texture %s not found\n", namet);
		return 0;
	}

	return i;
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
	memset (hitlist, 0, numflats);

	for (i = numsectors - 1; i >= 0; i--)
		hitlist[sectors[i].floorpic] = hitlist[sectors[i].ceilingpic] = 1;

	for (i = numflats - 1; i >= 0; i--)
		if (hitlist[i])
			W_CacheLumpNum (firstflat + i, PU_CACHE);

	// Precache textures.
	memset (hitlist, 0, numtextures);

	for (i = numsides - 1; i >= 0; i--)
	{
		hitlist[sides[i].toptexture] =
			hitlist[sides[i].midtexture] =
			hitlist[sides[i].bottomtexture] = 1;
	}

	// Sky texture is always present.
	// Note that F_SKY1 is the name used to
	//	indicate a sky floor/ceiling as a flat,
	//	while the sky texture is stored like
	//	a wall texture, with an episode dependend
	//	name.
	//
	// [RH] Possibly two sky textures now.
	// [ML] 5/11/06 - Not anymore!

	hitlist[sky1texture] = 1;
	hitlist[sky2texture] = 1;

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

// [ML] 11/4/06: Moved here from v_video.cpp
// [ML] 12/6/11: Moved from v_draw.cpp, not sure where to put it now...
/*
===============
BestColor
(borrowed from Quake2 source: utils3/qdata/images.c)
===============
*/
byte BestColor (const argb_t *palette, const int r, const int g, const int b, const int numcolors)
{
	int		i;
	int		dr, dg, db;
	int		bestdistortion, distortion;
	int		bestcolor;

//
// let any color go to 0 as a last resort
//
	bestdistortion = 256*256*4;
	bestcolor = 0;

	for (i = 0; i < numcolors; i++)
	{
		dr = r - RPART(palette[i]);
		dg = g - GPART(palette[i]);
		db = b - BPART(palette[i]);
		distortion = dr*dr + dg*dg + db*db;
		if (distortion < bestdistortion)
		{
			if (!distortion)
				return i;		// perfect match

			bestdistortion = distortion;
			bestcolor = i;
		}
	}

	return bestcolor;
}

byte BestColor2 (const argb_t *palette, const argb_t color, const int numcolors)
{
	return BestColor(palette, RPART(color), GPART(color), BPART(color), numcolors);
}

VERSION_CONTROL (r_data_cpp, "$Id$")

