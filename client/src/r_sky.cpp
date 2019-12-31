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
//	Sky rendering. The DOOM sky is a texture map like any
//	wall, wrapping around. 1024 columns equal 360 degrees.
//	The default sky map is 256 columns and repeats 4 times
//	on a 320 screen.
//	
//
//-----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include "doomstat.h"
#include "m_fixed.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "c_cvars.h"
#include "g_level.h"
#include "r_sky.h"
#include "gi.h"
#include "w_wad.h"

#include "resources/res_main.h"
#include "resources/res_texture.h"

extern fixed_t FocalLengthX;
extern fixed_t freelookviewheight;

EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(cl_mouselook)
EXTERN_CVAR(r_skypalette)

static const palindex_t* skyposts[MAXWIDTH];

//
// sky mapping
//
static const Texture* sky1texture;
static const Texture* sky2texture;

fixed_t		skytexturemid;
fixed_t		skyscale;
int			skystretch;
fixed_t		skyheight;
fixed_t		skyiscale;

int			sky1shift,		sky2shift;
fixed_t		sky1pos=0,		sky1speed=0;
fixed_t		sky2pos=0,		sky2speed=0;

char SKYFLATNAME[8] = "F_SKY1";

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t xtoviewangle[MAXWIDTH + 1];

CVAR_FUNC_IMPL(r_stretchsky)
{
	R_InitSkyMap();
}


//
// R_ResourceIdIsSkyFlat
//
// Returns true if the given ResourceId represents the sky.
//
bool R_ResourceIdIsSkyFlat(const ResourceId res_id)
{
	const ResourceId sky_res_id = Res_GetResourceId(SKYFLATNAME, flats_directory_name);
	return res_id == sky_res_id;
}


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
//
void R_InitSkyMap()
{
	// [SL] 2011-11-30 - Don't run if we don't know what sky texture to use
	if (gamestate != GS_LEVEL)
		return;

	fixed_t fskyheight = sky1texture ? sky1texture->getScaledHeight() : 0;
	if (fskyheight <= (128 << FRACBITS))
	{
		skytexturemid = 200/2*FRACUNIT;
		skystretch = (r_stretchsky == 1) || consoleplayer().spectator || (r_stretchsky == 2 && sv_freelook && cl_mouselook);
	}
	else
	{
		skytexturemid = 199 << FRACBITS;
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
	if (sky1texture && sky1texture->mWidthBits >= 7)
		sky1shift -= skystretch;
	if (sky2texture && sky2texture->mWidthBits >= 7)
		sky2shift -= skystretch;

	R_InitXToViewAngle();
}


//
// R_SetSkyTextures
//
// Loads the sky textures and re-initializes the sky map lookup tables.
//
void R_SetSkyTextures(const char* sky1_name, const char* sky2_name)
{
	sky1texture = sky2texture = NULL;
	const ResourceId sky1_res_id = Res_GetTextureResourceId(sky1_name, WALL);
	if (sky1_res_id != ResourceId::INVALID_ID)
		sky1texture = Res_CacheTexture(sky1_res_id);
	const ResourceId sky2_res_id = Res_GetTextureResourceId(sky2_name, WALL);
	if (sky2_res_id != ResourceId::INVALID_ID)
		sky2texture = Res_CacheTexture(sky2_res_id);

	if (sky2texture && sky1texture->mHeight != sky2texture->mHeight)
	{
		Printf(PRINT_HIGH,"Both sky textures must be the same height.\n");
		sky2texture = sky1texture;
	}

	R_InitSkyMap();
}


//
// R_BlastSkyColumn
//
static inline void R_BlastSkyColumn(void (*drawfunc)(void))
{
	if (dcol.yl <= dcol.yh)
	{
		dcol.texturefrac = dcol.texturemid + (dcol.yl - centery + 1) * dcol.iscale;
		drawfunc();
	}
}

inline void SkyColumnBlaster()
{
	R_BlastSkyColumn(colfunc);
}

//
// R_RenderSkyRange
//
// [RH] Can handle parallax skies. Note that the front sky is *not* masked in
// in the normal convention for patches, but uses color 0 as a transparent
// color.
// [ML] 5/11/06 - Removed sky2
//
void R_RenderSkyRange(visplane_t* pl)
{
	if (pl->minx > pl->maxx)
		return;

	const Texture* texture = NULL;

	fixed_t front_offset = 0;
	angle_t skyflip = 0;

	if (R_ResourceIdIsSkyFlat(pl->res_id))
	{
		if ((pl->sky & PL_SKYFLAT) == 0)
		{
			texture = sky1texture;
		}
		else if ((pl->sky & ~PL_SKYFLAT) == 0)
		{
			texture = sky2texture;
		}
		else
		{
			// MBF's linedef-controlled skies
			const line_t* line = &lines[(pl->sky & ~PL_SKYFLAT) - 1];

			// Sky transferred from first sidedef
			const side_t* side = &sides[line->sidenum[0]];

			// Texture comes from upper texture of reference sidedef
			if (side->toptexture != ResourceId::INVALID_ID)
				texture = Res_CacheTexture(side->toptexture);

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
	}

	R_ResetDrawFuncs();

	const palette_t* pal = V_GetDefaultPalette();

	skyplane = pl;

	if (texture)
	{
		dcol.masked = false;
		dcol.iscale = skyiscale >> skystretch;
		dcol.texturemid = skytexturemid;
		dcol.textureheight = texture->mHeight << FRACBITS;

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
			int colnum = ((((viewangle + xtoviewangle[x]) ^ skyflip) >> sky1shift) + front_offset) >> FRACBITS;
			colnum &= (1 << texture->mWidthBits) - 1;
			skyposts[x] = texture->getColumn(colnum);
		}

		R_RenderColumnRange(pl->minx, pl->maxx, (int*)pl->top, (int*)pl->bottom, skyposts, SkyColumnBlaster, false);
	}
				
	R_ResetDrawFuncs();
}

VERSION_CONTROL (r_sky_cpp, "$Id$")
