// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2025 by The Odamex Team.
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
EXTERN_CVAR(r_linearsky)


//
// sky mapping
//
fixed_t		defaultskytexturemid;
fixed_t		skyscale;
int			skystretch;
fixed_t		skyheight;
fixed_t		skyiscale;

int			sky1shift,        sky2shift;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t xtoviewangle[MAXWIDTH + 1];
static angle_t linearskyangle[MAXWIDTH + 1];

CVAR_FUNC_IMPL(r_stretchsky)
{
	R_InitSkyMap ();
}

static tallpost_t* skyposts[MAXWIDTH];
static byte transparentskybuffer[MAXWIDTH][512]; // holds foreground sky with transparency to blit to the screen

enum class skytype_t
{
	NORMAL,
	FIRE,
	DOUBLESKY
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
	OLumpName texture;

	// for interpolation
	fixed_t prevx;
	fixed_t prevy;
	fixed_t savedx;
	fixed_t savedy;
};

struct sky_t
{
	skytype_t type;
	bool      active;

	// Common functionality for all types
	skytex_t background;

	// Fire functionality
	byte*    firepalette;
	byte*    firetexturedata;
	int32_t  numfireentries;
	int32_t  fireticrate;

	// With foreground
	skytex_t foreground;

	bool usedefaultmid;
};

OHashTable<OLumpName, sky_t*> skylookup;
OHashTable<int32_t, sky_t*> skyflatlookup;

/**
 * @brief Used by OInterpolation::beginGameInterpolation
 */
void R_InterpolateSkyDefs(fixed_t amount)
{
	for (const auto& [_, sky] : skylookup)
	{
		if (!sky->active) continue;

		// Perform interp for any active scrolling skies
		skytex_t* background = &sky->background;
		skytex_t* foreground = &sky->foreground;

		if (gamestate == GS_LEVEL)
		{
			fixed_t newbackgroundxoffset = background->prevx +
			                    FixedMul(amount, background->currx - background->prevx);
			fixed_t newbackgroundyoffset = background->prevy +
			                    FixedMul(amount, background->curry - background->prevy);

			background->savedx = background->currx;
			background->savedy = background->curry;

			background->currx = newbackgroundxoffset;
			background->curry = newbackgroundyoffset;

			fixed_t newforegroundxoffset = foreground->prevx +
			                    FixedMul(amount, foreground->currx - foreground->prevx);
			fixed_t newforegroundyoffset = foreground->prevy +
			                    FixedMul(amount, foreground->curry - foreground->prevy);

			foreground->savedx = foreground->currx;
			foreground->savedy = foreground->curry;

			foreground->currx = newforegroundxoffset;
			foreground->curry = newforegroundyoffset;
		}
		else
		{
			background->savedx = 0;
			background->savedy = 0;

			foreground->savedx = 0;
			foreground->savedy = 0;
		}
	}
}

/**
 * @brief Used by OInterpolation::ticInterpolation
 */
void R_TicSkyDefInterpolation()
{
	for (const auto& [_, sky] : skylookup)
	{
		if (!sky->active) continue;

		skytex_t* background = &sky->background;
		skytex_t* foreground = &sky->foreground;

		if (gamestate == GS_LEVEL)
		{
			background->prevx = background->currx;
			background->prevy = background->curry;
			foreground->prevx = foreground->currx;
			foreground->prevy = foreground->curry;
		}
		else
		{
			background->prevx = 0;
			background->prevy = 0;
			foreground->prevx = 0;
			foreground->prevy = 0;
		}
	}
}

/**
 * @brief Used by OInterpolation::endGameInterpolation
 */
void R_RestoreSkyDefs()
{
	for (const auto& [_, sky] : skylookup)
	{
		if (!sky->active) continue;

		sky->background.currx = sky->background.savedx;
		sky->background.curry = sky->background.savedy;
		sky->foreground.currx = sky->foreground.savedx;
		sky->foreground.curry = sky->foreground.savedy;
	}
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
			{
				xtoviewangle[i]   = (angle_t)-(signed)tantoangle[slope >> DBITS];
				linearskyangle[i] = (0.5 - i / (double)viewwidth) * FIXED2DOUBLE(hitan) * ANG90;
			}

			for (int i = t + 1; i <= viewwidth; i++)
			{
				xtoviewangle[i]   = ANG270+tantoangle[dfocus / (i - centerx)];
				linearskyangle[i] = (0.5 - i / (double)viewwidth) * FIXED2DOUBLE(hitan) * ANG90;
			}

			for (int i = 0; i < centerx; i++)
			{
				xtoviewangle[i]   = (angle_t)(-(signed)xtoviewangle[viewwidth-i-1]);
				linearskyangle[i] = (angle_t)(-(signed)linearskyangle[viewwidth-i-1]);
			}
		}
		else
		{
			memset(xtoviewangle, 0, sizeof(angle_t) * viewwidth + 1);
			memset(linearskyangle, 0, sizeof(angle_t) * viewwidth + 1);
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
	if (textureheight == NULL)
		return;

	// [SL] 2011-11-30 - Don't run if we don't know what sky texture to use
	if (gamestate != GS_LEVEL)
		return;

	sky_t* defaultsky = skyflatlookup[skyflatnum];

	const fixed_t fskyheight = textureheight[defaultsky->background.texnum];

	if (fskyheight <= (128 << FRACBITS))
	{
		defaultskytexturemid = 200/2*FRACUNIT;
		skystretch = (r_stretchsky == 1) || consoleplayer().spectator || (r_stretchsky == 2 && sv_freelook && cl_mouselook);
	}
	else
	{
		defaultskytexturemid = 199<<FRACBITS;
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

sky_t* R_GetSky(const OLumpName& name, bool create)
{
	auto found = skylookup.find(name);
	if (found != skylookup.end())
	{
		return found->second;
	}

	if (!create)
	{
		return nullptr;
	}

	int32_t tex = R_TextureNumForName(name);
	if (tex < 0) return nullptr;

	OLumpName skytexname;
	sky_t* sky = (sky_t*)Z_Malloc(sizeof(sky_t), PU_STATIC, nullptr);
	sky->background.scalex = INT2FIXED(1);
	sky->background.scaley = INT2FIXED(1);
	sky->background.scrolly = INT2FIXED(0);
	if (level.flags & LEVEL_DOUBLESKY)
	{
		sky->background.texnum = R_TextureNumForName(level.skypic2);
		sky->background.texture = level.skypic2;
		sky->background.scrollx = level.sky2ScrollDelta & 0xffffff;
		sky->foreground.scrollx = level.sky1ScrollDelta & 0xffffff;
		sky->foreground.texnum = tex;
		sky->foreground.texture = name;
		sky->foreground.scalex = INT2FIXED(1);
		sky->foreground.scaley = INT2FIXED(1);
		sky->foreground.scrolly = INT2FIXED(0);
		sky->type = skytype_t::DOUBLESKY;
		skytexname = level.skypic2;
	}
	else
	{
		sky->background.texnum = tex;
		sky->background.texture = name;
		sky->background.scrollx = level.sky1ScrollDelta & 0xffffff;
		sky->type = skytype_t::NORMAL;
		skytexname = name;
	}
	sky->usedefaultmid = true;

	skylookup[skytexname] = sky;
	return sky;
}

// [EB] adapted from Rum and Raisin r_sky.cpp
void R_InitSkyDefs()
{
	skyflatnum = R_FlatNumForName(SKYFLATNAME);

	skyflatlookup[skyflatnum] = nullptr;

	auto ParseSkydef = [](const Json::Value& elem, const JSONLumpVersion& version) -> jsonlumpresult_t
	{
		const Json::Value& skyarray = elem["skies"];
		const Json::Value& flatmappings = elem["flatmapping"];

		if (!(skyarray.isArray() || skyarray.isNull())) return jsonlumpresult_t::PARSEERROR;
		if (!(flatmappings.isArray() || flatmappings.isNull())) return jsonlumpresult_t::PARSEERROR;

		for (const Json::Value& skyelem : skyarray)
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
			if (skytype < skytype_t::NORMAL || skytype > skytype_t::DOUBLESKY) return jsonlumpresult_t::PARSEERROR;

			OLumpName skytexname = skytex.asString();
			int32_t tex = R_TextureNumForName(skytexname);
			if (tex < 0) return jsonlumpresult_t::PARSEERROR;

			if (!mid.isNumeric()
			   || !scrollx.isNumeric()
			   || !scrolly.isNumeric()
			   || !scalex.isNumeric()
			   || !scaley.isNumeric())
			{
				return jsonlumpresult_t::PARSEERROR;
			}

			sky_t* sky = (sky_t*)Z_Malloc(sizeof(sky_t), PU_STATIC, nullptr);

			sky->type = skytype;
			sky->usedefaultmid = false;

			static constexpr float_t ticratescale = 1.0 / TICRATE;

			sky->background.texnum  = tex;
			sky->background.texture = skytexname;
			sky->background.mid     = FLOAT2FIXED(mid.asFloat());
			sky->background.scrollx = FLOAT2FIXED(scrollx.asFloat() * ticratescale);
			sky->background.scrolly = FLOAT2FIXED(scrolly.asFloat() * ticratescale);
			sky->background.scalex  = FLOAT2FIXED(1.0f / scalex.asFloat());
			sky->background.scaley  = FLOAT2FIXED(1.0f / scaley.asFloat());

			if (sky->type == skytype_t::FIRE)
			{
				if (!fireelem.isObject()) return jsonlumpresult_t::PARSEERROR;

				const Json::Value& firepalette    = fireelem["palette"];
				const Json::Value& fireupdatetime = fireelem["updatetime"];

				if (!firepalette.isArray()) return jsonlumpresult_t::PARSEERROR;
				sky->numfireentries = (int32_t)firepalette.size();
				byte* output = sky->firepalette = (byte*)Z_Malloc(sizeof(byte) * sky->numfireentries, PU_STATIC, nullptr);
				for (const Json::Value& palentry : firepalette)
				{
					*output++ = palentry.asUInt();
				}
				sky->fireticrate = (int32_t)(fireupdatetime.asFloat() * TICRATE);
			}
			else if (sky->type == skytype_t::DOUBLESKY)
			{
				if (!foreelem.isObject()) return jsonlumpresult_t::PARSEERROR;

				const Json::Value& foreskytex  = foreelem["name"];
				const Json::Value& foremid     = foreelem["mid"];
				const Json::Value& forescrollx = foreelem["scrollx"];
				const Json::Value& forescrolly = foreelem["scrolly"];
				const Json::Value& forescalex  = foreelem["scalex"];
				const Json::Value& forescaley  = foreelem["scaley"];

				OLumpName foreskytexname = foreskytex.asString();
				int32_t foretex = R_TextureNumForName(foreskytexname);
				if (foretex < 0) return jsonlumpresult_t::PARSEERROR;

				if (!foremid.isNumeric()
				   || !forescrollx.isNumeric()
				   || !forescrolly.isNumeric()
				   || !forescalex.isNumeric()
				   || !forescaley.isNumeric())
				{
					return jsonlumpresult_t::PARSEERROR;
				}

				sky->foreground.texnum  = foretex;
				sky->foreground.texture = foreskytexname;
				sky->foreground.mid     = FLOAT2FIXED(foremid.asFloat());
				sky->foreground.scrollx = FLOAT2FIXED(forescrollx.asFloat() * ticratescale);
				sky->foreground.scrolly = FLOAT2FIXED(forescrolly.asFloat() * ticratescale);
				sky->foreground.scalex  = FLOAT2FIXED(1.0f / forescalex.asFloat());
				sky->foreground.scaley  = FLOAT2FIXED(1.0f / forescaley.asFloat());
			}
			else
			{
				if (!fireelem.isNull() || !foreelem.isNull()) return jsonlumpresult_t::PARSEERROR;
			}

			skylookup[skytexname] = sky;
		}

		for (const Json::Value& flatentry : flatmappings)
		{
			const Json::Value& flatelem = flatentry["flat"];
			const Json::Value& skyelem = flatentry["sky"];

			OLumpName flatname = flatelem.asString();
			int32_t flatnum = R_FlatNumForName(flatname);
			if (flatnum < 0 || flatnum >= ::numflats) return jsonlumpresult_t::PARSEERROR;

			OLumpName skyname = skyelem.asString();
			sky_t* sky = R_GetSky(skyname, true);

			skyflatlookup[flatnum] = sky;
		}

		return jsonlumpresult_t::SUCCESS;
	};

	jsonlumpresult_t result =  M_ParseJSONLump("SKYDEFS", "skydefs", { 1, 0, 0 }, ParseSkydef);
	if (result != jsonlumpresult_t::SUCCESS && result != jsonlumpresult_t::NOTFOUND)
		I_Error("R_InitSkyDefs: SKYDEFS JSON error: {}", M_JSONLumpResultToString(result));
}

void R_ClearSkyDefs()
{
	skylookup.clear();
	skyflatlookup.clear();
}

void spreadFire(int src, byte* firepixels, int width)
{
	const byte pixel = firepixels[src];
	const int copyloc0 = src - width;
	if (pixel == 0) {
		if (copyloc0 >= 0)
			firepixels[copyloc0] = 0;
	} else {
		const int rand = (int)std::round(M_RandomFloat() * 3.0) & 3;
		const int copyloc1 = copyloc0 - rand + 1;
		if (copyloc1 >= 0)
			firepixels[copyloc1] = pixel - (rand & 1);
	}
}

static void R_UpdateFireSky(sky_t* sky, bool init = false)
{
	if (gametic % sky->fireticrate != 0 && !init) return;
	const int texnum = sky->background.texnum;
	texture_t* tex = textures[texnum];
	for (int x = 0 ; x < tex->width; x++)
	{
		for (int y = 1; y < tex->height; y++)
		{
			spreadFire(y * tex->width + x, sky->firetexturedata, tex->width);
		}
	}
	byte* coldata;
	for (int x = 0; x < tex->width; x++)
	{
		coldata = R_GetTextureColumnData(texnum, x);
		for (int y = 0; y < tex->height; y++)
		{
			coldata[y] = sky->firepalette[sky->firetexturedata[y * tex->width + x]];
		}
	}
}

void R_InitFireSky(sky_t* sky)
{
	int texnum = sky->background.texnum;
	const texture_t* tex = textures[texnum];
	sky->firetexturedata = (byte*)Z_Malloc(sizeof(byte) * tex->width * tex->height, PU_LEVEL, nullptr);
    for (int i = 0 ; i < tex->width*tex->height; i++)
	{
		sky->firetexturedata[i] = 0;
    }
	for (int i = 0 ; i < tex->width; i++)
	{
		sky->firetexturedata[(tex->height - 1) * tex->width + i] = sky->numfireentries - 1;
    }
	for (int i = 0; i < 64; i++) {
		R_UpdateFireSky(sky, true);
	}
}

static void R_UpdateSky(sky_t* sky)
{
	sky->foreground.currx += sky->foreground.scrollx;
	sky->foreground.curry += sky->foreground.scrolly;

	sky->background.currx += sky->background.scrollx;
	sky->background.curry += sky->background.scrolly;

	if (sky->type == skytype_t::FIRE)
	{
		R_UpdateFireSky(sky);
	}
}


void R_UpdateSkies()
{
	for (auto& [_, sky] : skylookup)
	{
		if (sky->active)
		{
			R_UpdateSky(sky);
		}
	}
}

void R_ActivateSky(sky_t* sky)
{
	if (sky->type == skytype_t::FIRE)
	{
		R_InitFireSky(sky);
	}
	if (sky->type == skytype_t::DOUBLESKY)
	{
		auto skypair = skylookup.find(sky->foreground.texture);
		if (skypair != skylookup.end())
		{
			R_ActivateSky(skypair->second);
		}
	}
	sky->active = true;
}

void R_ActivateSkies(const byte* hitlist, std::vector<int>& skytextures)
{
	for (auto& [flat, sky] : skyflatlookup)
	{
		if (hitlist[flat])
			R_ActivateSky(sky);

		skytextures.push_back(sky->background.texnum);
		if (sky->type == skytype_t::DOUBLESKY)
		{
			skytextures.push_back(sky->foreground.texnum);
			auto foreskypair = skylookup.find(sky->foreground.texture);
			if (foreskypair != skylookup.end())
			{
				R_ActivateSky(foreskypair->second);
			}
		}
	}
}

void R_InitSkiesForLevel()
{
	for (auto& [_, sky] : skylookup)
	{
		sky->active = false;
		sky->foreground.currx = 0;
		sky->foreground.curry = 0;
		sky->background.currx = 0;
		sky->background.curry = 0;
		sky->foreground.prevx = 0;
		sky->foreground.prevy = 0;
		sky->background.prevx = 0;
		sky->background.prevy = 0;
		sky->foreground.savedx = 0;
		sky->foreground.savedy = 0;
		sky->background.savedx = 0;
		sky->background.savedy = 0;
	}
}

void R_SetDefaultSky(const OLumpName& sky)
{
	sky_t* skydef = R_GetSky(sky, true);
	// make sure that if mapinfo sets a scroll speed we use that
	// to not mess up wads without skydefs that reuse textures with different scroll speeds
	// setting a scroll speed in mapinfo and in a skydef is undefined behavior
	if (level.flags & LEVEL_DOUBLESKY)
	{
		if (level.sky1ScrollDelta != 0)
		{
			skydef->foreground.scrollx = level.sky1ScrollDelta;
		}
		if (level.sky2ScrollDelta != 0)
		{
			skydef->background.scrollx = level.sky2ScrollDelta;
		}
	}
	else
	{
		if (level.sky1ScrollDelta != 0)
		{
			skydef->background.scrollx = level.sky1ScrollDelta;
		}
	}
	skyflatlookup[R_FlatNumForName(SKYFLATNAME)] = skydef;
}

//
// R_BlastSkyColumn
//
static inline void R_BlastSkyColumn(void (*drawfunc)())
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
	return flatnum == skyflatnum || skyflatlookup.count(flatnum);
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

	static constexpr int columnmethod = 2;
	int frontskytex, backskytex;
	fixed_t front_offset = 0;
	fixed_t back_offset = 0;
	fixed_t frontrow_offset = 0;
	fixed_t backrow_offset = 0;
	angle_t skyflip = 0;
	const angle_t* xtoskyangle = r_linearsky.asBool() ? linearskyangle : xtoviewangle;

	auto skyflat = skyflatlookup.find(pl->picnum);

	fixed_t sky1scalex = INT2FIXED(1);
	fixed_t sky2scalex = INT2FIXED(1);
	fixed_t sky1scaley = INT2FIXED(1);
	fixed_t sky2scaley = INT2FIXED(1);
	fixed_t sky1mid = defaultskytexturemid;
	fixed_t sky2mid = defaultskytexturemid;

	if (skyflat != skyflatlookup.end())
	{
		const sky_t* sky = skyflat->second;
		// use sky1
		if (skyflat->second->type == skytype_t::DOUBLESKY)
		{
			frontskytex = texturetranslation[sky->foreground.texnum];
			backskytex = texturetranslation[sky->background.texnum];
			front_offset = sky->foreground.currx;
			back_offset = sky->background.currx;
			frontrow_offset = sky->foreground.curry;
			backrow_offset = sky->background.curry;
			sky1scalex = sky->foreground.scalex;
			sky2scalex = sky->background.scalex;
			sky1scaley = sky->foreground.scaley;
			sky2scaley = sky->background.scaley;
			if (!sky->usedefaultmid)
			{
				sky1mid = sky->foreground.mid;
				sky2mid = sky->background.mid;
			}
		}
		else
		{
			frontskytex = texturetranslation[sky->background.texnum];
			backskytex = -1;
			front_offset = sky->background.currx;
			frontrow_offset = sky->background.curry;
			sky1scalex = sky->background.scalex;
			sky1scaley = sky->background.scaley;
			if (!sky->usedefaultmid)
				sky1mid = sky->background.mid;
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
		const int picnum = (pl->picnum & ~PL_SKYFLAT) - 1;
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
		sky1mid = side->rowoffset - 28*FRACUNIT;

		// We sometimes flip the picture horizontally.
		//
		// Doom always flipped the picture, so we make it optional,
		// to make it easier to use the new feature, while to still
		// allow old sky textures to be used.
		skyflip = line->args[2] ? 0u : ~0u;
	}

	R_ResetDrawFuncs();

	const palette_t* pal = V_GetDefaultPalette();

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

	if (backskytex != -1)
	{
		dcol.iscale = FixedMul(skyiscale, sky2scaley) >> skystretch;
		dcol.texturemid = sky2mid + backrow_offset;
		dcol.textureheight = textureheight[backskytex];
		dcol.texturefrac = dcol.texturemid + (dcol.yl - centery) * dcol.iscale;
		skyplane = pl;

		for (int x = pl->minx; x <= pl->maxx; x++)
		{
			int sky2colnum = ((((viewangle + xtoskyangle[x]) ^ skyflip) >> sky2shift) + back_offset) >> FRACBITS;
			sky2colnum = FIXED2INT(FixedMul(INT2FIXED(sky2colnum), sky2scalex));
			tallpost_t* skypost = R_GetTextureColumn(backskytex, sky2colnum);
			skyposts[x] = skypost;
		}

		R_RenderColumnRange(pl->minx, pl->maxx, (int*)pl->top, (int*)pl->bottom,
				skyposts, SkyColumnBlaster, false, columnmethod);
	}

	dcol.iscale = FixedMul(skyiscale, sky1scaley) >> skystretch;
	dcol.texturemid = sky1mid + frontrow_offset;
	dcol.textureheight = textureheight[frontskytex];
	dcol.texturefrac = dcol.texturemid + (dcol.yl - centery) * dcol.iscale;

	if (backskytex == -1)
	{
		for (int x = pl->minx; x <= pl->maxx; x++)
		{
			int sky1colnum = ((((viewangle + xtoskyangle[x]) ^ skyflip) >> sky1shift) + front_offset) >> FRACBITS;
			sky1colnum = FIXED2INT(FixedMul(INT2FIXED(sky1colnum), sky1scalex));
			tallpost_t* skypost = R_GetTextureColumn(frontskytex, sky1colnum);
			skyposts[x] = skypost;
		}

		R_RenderColumnRange(pl->minx, pl->maxx, (int*)pl->top, (int*)pl->bottom,
			skyposts, SkyColumnBlaster, false, columnmethod);
	}
	else
	{
		for (int x = pl->minx; x <= pl->maxx; x++)
		{
			int sky1colnum = ((((viewangle + xtoskyangle[x]) ^ skyflip) >> sky1shift) + front_offset) >> FRACBITS;
			sky1colnum = FIXED2INT(FixedMul(INT2FIXED(sky1colnum), sky1scalex));
			tallpost_t* skypost = R_GetTextureColumn(frontskytex, sky1colnum);

			int count = MIN<int> (512, textureheight[frontskytex] >> FRACBITS);
			int destpostlen = 0;

			tallpost_t* destpost = (tallpost_t*)transparentskybuffer[x];

			tallpost_t* orig = destpost;

			destpost->topdelta = 0;

			while (destpostlen < count)
			{
				int remaining = count - destpostlen; // pixels remaining to be replenished

				if (skypost->topdelta == destpostlen)
				{
					// valid first sky, grab its data and advance the pixel
					memcpy(destpost->data() + destpostlen, skypost->data(), skypost->length);
					destpostlen += skypost->length;
				}
				else
				{
					// sky1 top delta is less than current pixel, lets get palette 0 up to the pixel
					int cursky1delta = abs((skypost->end() ? remaining : skypost->topdelta) - destpostlen);
					int translen = cursky1delta > remaining ? remaining : cursky1delta;
					memset(destpost->data() + destpostlen, 0,
					       translen);
					destpostlen += translen;
				}

				if (!skypost->end() && !skypost->next()->end() && destpostlen >= skypost->topdelta + skypost->length)
				{
					skypost = skypost->next();
				}

				destpost->length = destpostlen;
			}

			// finish the post up.
			destpost = destpost->next();
			destpost->length = 0;
			destpost->writeend();

			skyposts[x] = orig;
		}

		R_SetSkyForegroundDrawFuncs();

		R_RenderColumnRange(pl->minx, pl->maxx, (int*)pl->top, (int*)pl->bottom,
			skyposts, SkyColumnBlaster, false, columnmethod);
	}

	R_ResetDrawFuncs();
}

VERSION_CONTROL (r_sky_cpp, "$Id$")
