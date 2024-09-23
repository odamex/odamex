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
//	Sky rendering. The DOOM sky is a texture map like any
//	wall, wrapping around. 1024 columns equal 360 degrees.
//	The default sky map is 256 columns and repeats 4 times
//	on a 320 screen.
//	
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "m_fixed.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_sky.h"
#include "w_wad.h"

extern int *texturewidthmask;
extern fixed_t FocalLengthX;
extern fixed_t freelookviewheight;

EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(cl_mouselook)
EXTERN_CVAR(r_skypalette)


//
// sky mapping
//
int 		skyflatnum;
int 		sky1texture,	sky2texture;
fixed_t		skytexturemid;
fixed_t		skyscale;
int			skystretch;
fixed_t		skyheight;
fixed_t		skyiscale;

int			sky1shift,		sky2shift;
fixed_t		sky1scrolldelta,	sky2scrolldelta;
fixed_t		sky1columnoffset,	sky2columnoffset;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t xtoviewangle[MAXWIDTH + 1];

CVAR_FUNC_IMPL(r_stretchsky)
{
	R_InitSkyMap ();
}

OLumpName SKYFLATNAME = "F_SKY1";


static tallpost_t* skyposts[MAXWIDTH];
static byte compositeskybuffer[MAXWIDTH][512]; // holds doublesky composite sky to blit to the screen


//
// R_InitXToViewAngle
//
// Now generate xtoviewangle for sky texture mapping.
// [RH] Do not generate viewangletox, because texture mapping is no
// longer done with trig, so it's not needed.
//
static void R_InitXToViewAngle()
{
	static int last_viewwidth = -1;
	static fixed_t last_focx = -1;

	if (viewwidth != last_viewwidth || FocalLengthX != last_focx)
	{
		if (centerx > 0)
		{
			const fixed_t hitan = finetangent[FINEANGLES/4+CorrectFieldOfView/2];
			const int t = std::min<int>((FocalLengthX >> FRACBITS) + centerx, viewwidth);
			const fixed_t slopestep = hitan / centerx;
			const fixed_t dfocus = FocalLengthX >> DBITS;

			for (int i = centerx, slope = 0; i <= t; i++, slope += slopestep)
				xtoviewangle[i] = (angle_t)-(signed)tantoangle[slope >> DBITS];

			for (int i = t + 1; i <= viewwidth; i++)
				xtoviewangle[i] = ANG270+tantoangle[dfocus / (i - centerx)];

			for (int i = 0; i < centerx; i++)
				xtoviewangle[i] = (angle_t)(-(signed)xtoviewangle[viewwidth-i-1]);
		}
		else
		{
			memset(xtoviewangle, 0, sizeof(angle_t) * viewwidth + 1);
		}

		last_viewwidth = viewwidth;
		last_focx = FocalLengthX;
	}
}


//
//
// R_InitSkyMap
//
// Called whenever the view size changes.
//
// [ML] 5/11/06 - Remove sky2 stuffs
// [ML] 3/16/10 - Bring it back!

void R_GenerateLookup(int texnum, int *const errors); // from r_data.cpp

void R_InitSkyMap()
{
	fixed_t fskyheight;

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
	
	fskyheight = textureheight[sky1texture];

	if (fskyheight <= (128 << FRACBITS))
	{
		skytexturemid = 200/2*FRACUNIT;
		skystretch = (r_stretchsky == 1) || consoleplayer().spectator || (r_stretchsky == 2 && sv_freelook && cl_mouselook);
	}
	else
	{
		skytexturemid = 199<<FRACBITS;//textureheight[sky1texture]-1;
		skystretch = 0;
	}
	skyheight = fskyheight << skystretch;

	if (viewwidth && viewheight)
	{
		skyiscale = (200*FRACUNIT) / ((freelookviewheight * viewwidth) / viewwidth);
		skyscale = (((freelookviewheight * viewwidth) / viewwidth) << FRACBITS) /(200);

		skyiscale = FixedMul(skyiscale, FixedDiv(FieldOfView, 2048));
		skyscale = FixedMul(skyscale, FixedDiv(2048, FieldOfView));
	}

	// The DOOM sky map is 256*128*4 maps.
	// The Heretic sky map is 256*200*4 maps.
	sky1shift = 22+skystretch-16;
	sky2shift = 22+skystretch-16;	
	if (texturewidthmask[sky1texture] >= 127)
		sky1shift -= skystretch;
	if (texturewidthmask[sky2texture] >= 127)
		sky2shift -= skystretch;

	R_InitXToViewAngle();
}


//
// R_BlastSkyColumn
//
static inline void R_BlastSkyColumn(void (*drawfunc)(void))
{
	if (dcol.yl <= dcol.yh)
	{
		dcol.source = dcol.post->data();
		dcol.texturefrac = dcol.texturemid + (dcol.yl - centery + 1) * dcol.iscale;
		drawfunc();
	}
}

inline void SkyColumnBlaster()
{
	R_BlastSkyColumn(colfunc);
}

inline bool R_PostDataIsTransparent(byte* data)
{
	if (*data == '\0')
	{
		return true;
	}
	return false;
}



//
// R_RenderSkyRange
//
// [RH] Can handle parallax skies. Note that the front sky is *not* masked in
// in the normal convention for patches, but uses color 0 as a transparent
// color.
// [ML] 5/11/06 - Removed sky2
// [BC] 7/5/24 - Brought back for real this time
//
void R_RenderSkyRange(visplane_t* pl)
{
	if (pl->minx > pl->maxx)
		return;

	int columnmethod = 2;
	int frontskytex, backskytex;
	fixed_t front_offset = 0;
	fixed_t back_offset = 0;
	angle_t skyflip = 0;

	if (pl->picnum == skyflatnum )
	{
		// use sky1
		frontskytex = texturetranslation[sky1texture];
		if (level.flags & LEVEL_DOUBLESKY)
		{
			backskytex = texturetranslation[sky2texture];
		}
		else
		{
			backskytex = -1;
		}
		front_offset = sky1columnoffset;
		back_offset = sky2columnoffset;
	}
	else if (pl->picnum == int(PL_SKYFLAT))
	{
		// use sky2
		frontskytex = texturetranslation[sky2texture];
		backskytex = -1;
		front_offset = sky2columnoffset;
	}
	else
	{
		// MBF's linedef-controlled skies
		short picnum = (pl->picnum & ~PL_SKYFLAT) - 1;
		const line_t* line = &lines[picnum < numlines ? picnum : 0];

		// Sky transferred from first sidedef
		const side_t* side = *line->sidenum + sides;

		// Texture comes from upper texture of reference sidedef
		frontskytex = texturetranslation[side->toptexture];
		backskytex = -1;

		// Horizontal offset is turned into an angle offset,
		// to allow sky rotation as well as careful positioning.
		// However, the offset is scaled very small, so that it
		// allows a long-period of sky rotation.
		front_offset = (-side->textureoffset) >> 6;

		// Vertical offset allows careful sky positioning.
		skytexturemid = side->rowoffset - 28*FRACUNIT;

		// We sometimes flip the picture horizontally.
		//
		// Doom always flipped the picture, so we make it optional,
		// to make it easier to use the new feature, while to still
		// allow old sky textures to be used.
		skyflip = line->args[2] ? 0u : ~0u;
	}

	R_ResetDrawFuncs();

	const palette_t* pal = V_GetDefaultPalette();

	dcol.iscale = skyiscale >> skystretch;
	dcol.texturemid = skytexturemid;
	dcol.textureheight = textureheight[frontskytex]; // both skies are forced to be the same height anyway
	dcol.texturefrac = dcol.texturemid + (dcol.yl - centery) * dcol.iscale;
	skyplane = pl;

	// set up the appropriate colormap for the sky
	if (fixedlightlev)
	{
		dcol.colormap = shaderef_t(&pal->maps, fixedlightlev);
	}
	else if (fixedcolormap.isValid() && r_skypalette)
	{
		dcol.colormap = fixedcolormap;
	}
	else
	{
		// [SL] 2011-06-28 - Emulate vanilla Doom's handling of skies
		// when the player has the invulnerability powerup
		dcol.colormap = shaderef_t(&pal->maps, 0);
	}

	// determine which texture posts will be used for each screen
	// column in this range.
	for (int x = pl->minx; x <= pl->maxx; x++)
	{
		int sky1colnum = ((((viewangle + xtoviewangle[x]) ^ skyflip) >> sky1shift) + front_offset) >> FRACBITS;
		tallpost_t* skypost = R_GetTextureColumn(frontskytex, sky1colnum);
		if (backskytex == -1)
		{
			skyposts[x] = skypost;
		}
		else
		{
			// create composite of both skies
			int sky2colnum = ((((viewangle + xtoviewangle[x]) ^ skyflip) >> sky2shift) + back_offset) >> FRACBITS;
			tallpost_t* skypost2 = R_GetTextureColumn(backskytex, sky2colnum);

			int count = MIN<int> (512, MIN (textureheight[backskytex], textureheight[frontskytex]) >> FRACBITS);
			int destpostlen = 0;

			BYTE* composite = compositeskybuffer[x];
			tallpost_t* destpost = (tallpost_t*)composite;

			tallpost_t* orig = destpost; // need a pointer to the og element to return!
			
			// here's the skinny...

			// we need to grab sky1 as long as it has a valid topdelta and length
			// in a lot of cases there's gaps in length and topdelta because of transparency
			// its up to us to find these gaps and put in sky2 when needed

			// but when we grab from sky1, we ALSO have to check every byte if it's index 0.
			// if it is, we need to grab the same byte from sky2 instead.
			// this allows non-converted sky patches, like those in hexen.wad or tlsxctf1.wad
			// to work

			// the finished tallpost should be the same length as count, and 0 topdelta, with a endpost after.
			
			destpost->topdelta = 0;

			while (destpostlen < count)
			{
				int remaining = count - destpostlen; // pixels remaining to be replenished

				if (skypost->topdelta == destpostlen)
				{
					// valid first sky, grab its data and advance the pixel
					memcpy(destpost->data() + destpostlen, skypost->data(), skypost->length);
					// before advancing, check the data we just inserted, and input sky2 if its index 0
					for (int i = 0; i < skypost->length; i++)
					{
						if (R_PostDataIsTransparent(destpost->data() + destpostlen + i))
						{
							// use sky2 for this pixel
						
							memcpy(destpost->data() + destpostlen + i,
							       skypost2->data() + destpostlen + i, 1);
						}
					}
					destpostlen += skypost->length;
				}
				else
				{
					// sky1 top delta is less than current pixel, lets get sky2 up to the pixel
					int cursky1delta = abs((skypost->end() ? remaining : skypost->topdelta) - destpostlen);
					int sky2len = cursky1delta > remaining ? remaining : cursky1delta;
					memcpy(destpost->data() + destpostlen, skypost2->data() + destpostlen,
					       sky2len);
					destpostlen += sky2len;
				}

				if (!skypost->next()->end() && destpostlen >= skypost->topdelta + skypost->length)
				{
					skypost = skypost->next();
				}

				if (!skypost2->next()->end() && destpostlen >= skypost2->topdelta + skypost2->length)
				{
					skypost2 = skypost2->next();
				}

				destpost->length = destpostlen;
			}

			// finish the post up.
			destpost = destpost->next();
			destpost->length = 0;
			destpost->writeend();

			skyposts[x] = orig;
		}
	}

	R_RenderColumnRange(pl->minx, pl->maxx, (int*)pl->top, (int*)pl->bottom,
			skyposts, SkyColumnBlaster, false, columnmethod);

	R_ResetDrawFuncs();
}

VERSION_CONTROL (r_sky_cpp, "$Id$")
