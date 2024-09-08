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
#include "m_jsonlump.h"
#include "m_random.h"
#include "r_data.h"
#include "r_draw.h"
#include "r_main.h"
#include "r_sky.h"
#include "w_wad.h"
#include "i_system.h"

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
int 		sky1texture,    sky2texture;
fixed_t		sky1texturemid, sky2texturemid;
fixed_t		skyscale;
int			skystretch;
fixed_t		skyheight;
fixed_t		skyiscale;

int			sky1shift,        sky2shift;
fixed_t		sky2scrollxdelta;
fixed_t		sky2columnoffset;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t xtoviewangle[MAXWIDTH + 1];

CVAR_FUNC_IMPL(r_stretchsky)
{
	R_InitSkyMap ();
}

char SKYFLATNAME[8] = "F_SKY1";


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
		sky1texturemid = 200/2*FRACUNIT;
		skystretch = (r_stretchsky == 1) || consoleplayer().spectator || (r_stretchsky == 2 && sv_freelook && cl_mouselook);
	}
	else
	{
		sky1texturemid = 199<<FRACBITS;//textureheight[sky1texture]-1;
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

enum skytype_t
{
	SKY_NORMAL,
	SKY_FIRE,
	SKY_DOUBLESKY
};

struct skytex_t
{
	fixed_t mid;
	fixed_t scrollx;
	fixed_t scrolly;
	fixed_t scalex;
	fixed_t scaley;
	fixed_t currx;
	fixed_t curry;
	int32_t texnum;
};

struct sky_t
{
	skytype_t type;
	bool      active;

	// Common functionality for all types
	skytex_t background;

	// Fire functionality
	byte*    firepalette;
	int32_t  numfireentries;
	int32_t  fireticrate;

	// With foreground
	skytex_t foreground;
};

OHashTable<std::string, sky_t*> skylookup;
OHashTable<int32_t, sky_t*> skyflatlookup;

sky_t* R_GetSky(const char* name, bool create)
{
	std::string skytexname = name;
	auto found = skylookup.find(name);
	if (found != skylookup.end())
	{
		return found->second;
	}

	if(!create)
	{
		return nullptr;
	}

	int32_t tex = R_TextureNumForName(name);
	if (tex < 0) return nullptr;

	sky_t* sky = (sky_t*)Z_Malloc(sizeof(sky_t), PU_STATIC, nullptr);
	sky->background.scalex = INT2FIXED(1);
	sky->background.scaley = INT2FIXED(1);
	sky->background.scrolly = INT2FIXED(0);
	sky->background.mid = INT2FIXED(100);
	if (level.flags & LEVEL_DOUBLESKY)
	{
		sky->background.texnum = R_TextureNumForName(level.skypic2.c_str());
		sky->background.scrollx = level.sky2ScrollDelta;
		sky->foreground.scrollx = level.sky1ScrollDelta;
		sky->foreground.texnum = tex;
		sky->foreground.scalex = INT2FIXED(1);
		sky->foreground.scaley = INT2FIXED(1);
		sky->foreground.scrolly = INT2FIXED(0);
		sky->foreground.mid = INT2FIXED(100);
	}
	else
	{
		sky->background.texnum = tex;
		sky->background.scrollx = level.sky1ScrollDelta;
	}
	sky->type = level.flags & LEVEL_DOUBLESKY ? SKY_DOUBLESKY : SKY_NORMAL;

	skylookup[name] = sky;
	return sky;
}

// [EB] adapted from Rum n Raisin r_sky.cpp
void R_InitSkyDefs()
{
	skyflatnum = R_FlatNumForName(SKYFLATNAME);

	skyflatlookup[skyflatnum] = nullptr;

	auto ParseSkydef = [](const Json::Value& elem, const JSONLumpVersion& version) -> jsonlumpresult_t
	{
		const Json::Value& skyarray = elem["skies"];
		const Json::Value& flatmappings = elem["flatmapping"];

		if(!(skyarray.isArray() || skyarray.isNull())) return JL_PARSEERROR;
		if(!(flatmappings.isArray() || flatmappings.isNull())) return JL_PARSEERROR;

		for(const Json::Value& skyelem : skyarray)
		{
			const Json::Value& type     = skyelem["type"];

			const Json::Value& skytex   = skyelem["name"];
			const Json::Value& mid      = skyelem["mid"];
			const Json::Value& scrollx  = skyelem["scrollx"];
			const Json::Value& scrolly  = skyelem["scrolly"];
			const Json::Value& scalex   = skyelem["scalex"];
			const Json::Value& scaley   = skyelem["scaley"];

			const Json::Value& fireelem	= skyelem["fire"];
			const Json::Value& foreelem = skyelem["foregroundtex"];

			auto skytype = static_cast<skytype_t>(type.asInt());
			if(skytype < SKY_NORMAL || skytype > SKY_DOUBLESKY) return JL_PARSEERROR;

			std::string skytexname = skytex.asString();
			int32_t tex = R_TextureNumForName(skytexname.c_str());
			if(tex < 0) return JL_PARSEERROR;

			if(!mid.isNumeric()
			   || !scrollx.isNumeric()
			   || !scrolly.isNumeric()
			   || !scalex.isNumeric()
			   || !scaley.isNumeric())
			{
				return JL_PARSEERROR;
			}

			sky_t* sky = (sky_t*)Z_Malloc(sizeof(sky_t), PU_STATIC, nullptr);

			sky->type = skytype;

			constexpr float_t ticratescale = 1.0 / TICRATE;

			sky->background.texnum  = tex;
			sky->background.mid     = FLOAT2FIXED(mid.asFloat());
			sky->background.scrollx = FLOAT2FIXED(scrollx.asFloat() * ticratescale);
			sky->background.scrolly = FLOAT2FIXED(scrolly.asFloat() * ticratescale);
			sky->background.scalex  = FLOAT2FIXED(scalex.asFloat());
			sky->background.scaley  = FLOAT2FIXED(scaley.asFloat());

			if(sky->type == SKY_FIRE)
			{
				if(!fireelem.isObject()) return JL_PARSEERROR;

				const Json::Value& firepalette    = fireelem["palette"];
				const Json::Value& fireupdatetime = fireelem["updatetime"];

				if(!firepalette.isArray()) return JL_PARSEERROR;
				sky->numfireentries = (int32_t)firepalette.size();
				byte* output = sky->firepalette = (byte*)Z_Malloc(sizeof(byte) * sky->numfireentries, PU_STATIC, nullptr);
				for(const Json::Value& palentry : firepalette)
				{
					*output++ = palentry.asUInt();
				}
				sky->fireticrate = (int32_t)(fireupdatetime.asFloat() * TICRATE);
			}
			else if(sky->type == SKY_DOUBLESKY)
			{
				if(!foreelem.isObject()) return JL_PARSEERROR;

				const Json::Value& foreskytex  = foreelem["name"];
				const Json::Value& foremid     = foreelem["mid"];
				const Json::Value& forescrollx = foreelem["scrollx"];
				const Json::Value& forescrolly = foreelem["scrolly"];
				const Json::Value& forescalex  = foreelem["scalex"];
				const Json::Value& forescaley  = foreelem["scaley"];

				std::string foreskytexname = foreskytex.asString();
				int32_t foretex = R_TextureNumForName(foreskytexname.c_str());
				if(foretex < 0) return JL_PARSEERROR;

				if(!foremid.isNumeric()
				   || !forescrollx.isNumeric()
				   || !forescrolly.isNumeric()
				   || !forescalex.isNumeric()
				   || !forescaley.isNumeric())
				{
					return JL_PARSEERROR;
				}

				sky->foreground.texnum  = foretex;
				sky->foreground.mid     = FLOAT2FIXED(foremid.asFloat());
				sky->foreground.scrollx = FLOAT2FIXED(forescrollx.asFloat() * ticratescale);
				sky->foreground.scrolly = FLOAT2FIXED(forescrolly.asFloat() * ticratescale);
				sky->foreground.scalex  = FLOAT2FIXED(forescalex.asFloat());
				sky->foreground.scaley  = FLOAT2FIXED(forescaley.asFloat());
			}
			else
			{
				if(!fireelem.isNull() || !foreelem.isNull()) return JL_PARSEERROR;
			}

			skylookup[skytexname] = sky;
		}

		for(const Json::Value& flatentry : flatmappings)
		{
			const Json::Value& flatelem = flatentry["flat"];
			const Json::Value& skyelem = flatentry["sky"];

			std::string flatname = flatelem.asString();
			int32_t flatnum = R_FlatNumForName(flatname.c_str());
			if(flatnum < 0 || flatnum >= ::numflats) return JL_PARSEERROR;

			std::string skyname = skyelem.asString();
			sky_t* sky = R_GetSky(skyname.c_str(), true);

			skyflatlookup[flatnum] = sky;
		}

		return JL_SUCCESS;
	};

	jsonlumpresult_t result =  M_ParseJSONLump("SKYDEFS", "skydefs", { 1, 0, 0 }, ParseSkydef);
	if (result != JL_SUCCESS && result != JL_NOTFOUND)
		I_Error("R_InitSkyDefs: SKYDEFS JSON error: %s", jsonLumpResultToString(result));
}

void R_ClearSkyDefs()
{
	skylookup.clear();
	skyflatlookup.clear();
}

void spreadFire(int src, byte* firePixels, int FIRE_WIDTH, int numfire)
{
	byte pixel = firePixels[src];
	if(pixel == 0) {
		firePixels[src - FIRE_WIDTH] = 0;
	} else {
		int rand = (int)std::round(M_RandomFloat() * 3.0) & 3;
		firePixels[(src - rand + 1) - FIRE_WIDTH] = pixel - (rand & 1);
	}
}

byte* paletteindices;

static void R_UpdateFireSky(sky_t* sky)
{
	if (gametic % sky->fireticrate != 0) return;
	int texnum = sky->background.texnum;
	texture_t* tex = textures[texnum];
    for (int x = 0 ; x < tex->width; x++)
	{
        for (int y = 1; y < tex->height; y++)
		{
            spreadFire(y * tex->width + x, paletteindices, tex->width, sky->numfireentries);
        }
    }
	byte* coldata;
	for (int x = 0; x < tex->width; x++)
	{
		coldata = R_GetTextureColumnData(texnum, x);
		for (int y = 0; y < tex->height; y++)
		{
			coldata[y] = sky->firepalette[paletteindices[y * tex->width + x]];
		}
	}
}

void R_InitFireSky(sky_t* sky)
{
	std::srand(std::time(nullptr));
	int texnum = sky->background.texnum;
	texture_t* tex = textures[texnum];
	paletteindices = (byte*)Z_Malloc(sizeof(byte) * tex->width * tex->height, PU_STATIC, nullptr);
    for (int i = 0 ; i < tex->width*tex->height; i++)
	{
		paletteindices[i] = 0;
    }
	for (int i = 0 ; i < tex->width; i++)
	{
		paletteindices[(tex->height - 1) * tex->width + i] = sky->numfireentries - 1;
    }
	for (int i = 0; i < 64; i++) {
		R_UpdateFireSky(sky);
	}
}

static void R_UpdateSky(sky_t* sky)
{
	sky->foreground.currx += sky->foreground.scrollx;
	sky->foreground.curry += sky->foreground.scrolly;

	sky->background.currx += sky->background.scrollx;
	sky->background.curry += sky->background.scrolly;

	if(sky->type == SKY_FIRE)
	{
		R_UpdateFireSky(sky);
	}
}


void R_UpdateSkies()
{
	for(auto& skypair : skylookup)
	{
		if(skypair.second->active)
		{
			R_UpdateSky(skypair.second);
		}
	}
}

void R_InitSkiesForLevel()
{
	for(auto& skypair : skylookup)
	{
		// TODO: change to false and correctly set if sky is active elsewhere
		skypair.second->active = true;
		skypair.second->foreground.currx = 0;
		skypair.second->foreground.curry = 0;
		skypair.second->background.currx = 0;
		skypair.second->background.curry = 0;
		if (skypair.second->type == SKY_FIRE)
		{
			R_InitFireSky(skypair.second);
		}
	}
}

void R_SetDefaultSky(const char* sky)
{
	sky_t* skydef = R_GetSky(sky, true);
	skyflatlookup[R_FlatNumForName(SKYFLATNAME)] = skydef;
}

// TODO: in R_PreCacheLevel, check if any of the skyflats are present and activate the corresponding skies
void R_ActivateSky(sky_t* sky)
{
	sky->active = true;
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

bool R_IsSkyFlat(int flatnum)
{
	return skyflatlookup.count(flatnum);
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
	fixed_t frontrow_offset = 0;
	fixed_t backrow_offset = 0;
	angle_t skyflip = 0;

	auto skyflat = skyflatlookup.find(pl->picnum);
	sky_t* sky = nullptr;

	if (skyflat != skyflatlookup.end())
	{
		sky = skyflat->second;
		// use sky1
		if (skyflat->second->type == SKY_DOUBLESKY)
		{
			frontskytex = texturetranslation[sky->foreground.texnum];
			backskytex = texturetranslation[sky->background.texnum];
			front_offset = sky->foreground.currx;
			back_offset = sky->background.currx;
		}
		else
		{
			frontskytex = texturetranslation[sky->background.texnum];
			backskytex = -1;
			front_offset = sky->background.currx;
		}
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
		sky1texturemid = side->rowoffset - 28*FRACUNIT;

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
	dcol.texturemid = sky1texturemid;
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
