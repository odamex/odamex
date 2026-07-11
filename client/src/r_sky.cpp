// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	ID24 SKYDEFS support (fire skies, double skies, scrolling skies)
//	adapted from Rum and Raisin via protobreak, running on top of the
//	resource manager / Texture system.
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
#include "i_system.h"
#include "g_mapinfo.h"

#include "resources/res_main.h"
#include "resources/res_texture.h"

extern fixed_t FocalLengthX;
extern fixed_t freelookviewheight;
extern visplane_t* skyplane;

EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(cl_mouselook)
EXTERN_CVAR(joy_freelook)
EXTERN_CVAR(r_skypalette)
EXTERN_CVAR(r_linearsky)



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

static ResourceId sky_flat_resource_id = ResourceId::INVALID_ID;

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
static angle_t xtoviewangle[MAXWIDTH + 1];
static angle_t linearskyangle[MAXWIDTH + 1];

static const palindex_t* skyposts[MAXWIDTH];

CVAR_FUNC_IMPL(r_stretchsky)
{
	R_InitSkyMap();
}

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
	ResourceId res_id;
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
OHashTable<ResourceId, sky_t*> skyflatlookup;

//
// R_ResourceIdIsSkyFlat
//
// Returns true if the given ResourceId represents the sky.
//
bool R_ResourceIdIsSkyFlat(const ResourceId res_id)
{
	if (res_id == sky_flat_resource_id)
		return true;
	return skyflatlookup.find(res_id) != skyflatlookup.end();
}

//
// cache the Texture for a skytex_t, honoring texture animation
//
static const Texture* R_SkyTexTexture(const skytex_t* skytex)
{
	return Res_CacheTexture(Res_GetAnimatedTextureResourceId(skytex->res_id));
}

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
				xtoviewangle[i]   = static_cast<angle_t>(-static_cast<signed>(tantoangle[slope >> DBITS]));
				linearskyangle[i] = (0.5 - i / static_cast<double>(viewwidth)) * FIXED2DOUBLE(hitan) * ANG90;
			}

			for (int i = t + 1; i <= viewwidth; i++)
			{
				xtoviewangle[i]   = ANG270+tantoangle[dfocus / (i - centerx)];
				linearskyangle[i] = (0.5 - i / static_cast<double>(viewwidth)) * FIXED2DOUBLE(hitan) * ANG90;
			}

			for (int i = 0; i < centerx; i++)
			{
				xtoviewangle[i]   = static_cast<angle_t>(-static_cast<signed>(xtoviewangle[viewwidth-i-1]));
				linearskyangle[i] = static_cast<angle_t>(-static_cast<signed>(linearskyangle[viewwidth-i-1]));
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
//
void R_InitSkyMap()
{
	// [SL] 2011-11-30 - Don't run if we don't know what sky texture to use
	if (gamestate != GS_LEVEL)
		return;

	// Prefer the SKYDEFS sky attached to the default sky flat, if one exists.
	const Texture* defaultskytex = sky1texture;
	auto it = skyflatlookup.find(sky_flat_resource_id);
	if (it != skyflatlookup.end() && it->second)
		defaultskytex = R_SkyTexTexture(&it->second->background);

	fixed_t fskyheight = defaultskytex ? defaultskytex->getScaledHeight() : 0;

	if (fskyheight <= (128 << FRACBITS))
	{
		skytexturemid = 200 / 2 * FRACUNIT;
		skystretch = ((r_stretchsky != 0) && consoleplayer().spectator) ||
		             (r_stretchsky == 1) ||
		             (r_stretchsky == 2 && sv_freelook && (cl_mouselook || joy_freelook));
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
	if (defaultskytex && defaultskytex->mWidthBits >= 7)
		sky1shift -= skystretch;
	if (sky2texture && sky2texture->mWidthBits >= 7)
		sky2shift -= skystretch;

	R_InitXToViewAngle();
}

//
// R_GetSky
//
// Finds or creates the sky definition for the given texture name.
//
static sky_t* R_GetSky(const OLumpName& name, bool create)
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

	const ResourceId tex_res_id = Res_GetTextureResourceId(OStringToUpper(name.c_str()), WALL);
	if (!Res_CheckResource(tex_res_id))
		return nullptr;

	OLumpName skytexname;
	sky_t* sky = Z_Malloc<sky_t>(PU_STATIC);
	memset(sky, 0, sizeof(*sky));
	sky->background.scalex = INT2FIXED(1);
	sky->background.scaley = INT2FIXED(1);
	sky->background.scrolly = INT2FIXED(0);
	if (level.flags & LEVEL_DOUBLESKY)
	{
		sky->background.res_id = Res_GetTextureResourceId(OStringToUpper(level.skypic2.c_str()), WALL);
		sky->background.texture = level.skypic2;
		sky->background.scrollx = level.sky2ScrollDelta & 0xffffff;
		sky->foreground.scrollx = level.sky1ScrollDelta & 0xffffff;
		sky->foreground.res_id = tex_res_id;
		sky->foreground.texture = name;
		sky->foreground.scalex = INT2FIXED(1);
		sky->foreground.scaley = INT2FIXED(1);
		sky->foreground.scrolly = INT2FIXED(0);
		sky->type = skytype_t::DOUBLESKY;
		skytexname = level.skypic2;
	}
	else
	{
		sky->background.res_id = tex_res_id;
		sky->background.texture = name;
		sky->background.scrollx = level.sky1ScrollDelta & 0xffffff;
		sky->type = skytype_t::NORMAL;
		skytexname = name;
	}
	sky->usedefaultmid = true;

	skylookup[skytexname] = sky;
	return sky;
}

//
// R_SkyFlatResourceId
//
// Returns the ResourceId of the default sky flat (F_SKY1, or F_SKY for
// Hexen-format maps).
//
static ResourceId R_SkyFlatResourceId()
{
	if (HexenHack)
		return Res_GetTextureResourceId("F_SKY", FLOOR);
	return Res_GetTextureResourceId(SKYFLATNAME.c_str(), FLOOR);
}

// [EB] adapted from Rum and Raisin r_sky.cpp
void R_InitSkyDefs()
{
	sky_flat_resource_id = R_SkyFlatResourceId();

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
			const ResourceId tex_res_id = Res_GetTextureResourceId(OStringToUpper(skytexname.c_str()), WALL);
			if (!Res_CheckResource(tex_res_id)) return jsonlumpresult_t::PARSEERROR;

			if (!mid.isNumeric()
			   || !scrollx.isNumeric()
			   || !scrolly.isNumeric()
			   || !scalex.isNumeric()
			   || !scaley.isNumeric())
			{
				return jsonlumpresult_t::PARSEERROR;
			}

			sky_t* sky = Z_Malloc<sky_t>(PU_STATIC);
			memset(sky, 0, sizeof(*sky));

			sky->type = skytype;
			sky->usedefaultmid = false;

			static constexpr float_t ticratescale = 1.0 / TICRATE;

			sky->background.res_id  = tex_res_id;
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
				sky->numfireentries = static_cast<int32_t>(firepalette.size());
				byte* output = sky->firepalette = Z_Malloc<byte>(sky->numfireentries, PU_STATIC);
				for (const Json::Value& palentry : firepalette)
				{
					*output++ = palentry.asUInt();
				}
				sky->fireticrate = static_cast<int32_t>((fireupdatetime.asFloat() * TICRATE));
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
				const ResourceId foretex_res_id = Res_GetTextureResourceId(OStringToUpper(foreskytexname.c_str()), WALL);
				if (!Res_CheckResource(foretex_res_id)) return jsonlumpresult_t::PARSEERROR;

				if (!foremid.isNumeric()
				   || !forescrollx.isNumeric()
				   || !forescrolly.isNumeric()
				   || !forescalex.isNumeric()
				   || !forescaley.isNumeric())
				{
					return jsonlumpresult_t::PARSEERROR;
				}

				sky->foreground.res_id  = foretex_res_id;
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
			const ResourceId flat_res_id = Res_GetTextureResourceId(OStringToUpper(flatname.c_str()), FLOOR);
			if (!Res_CheckResource(flat_res_id)) return jsonlumpresult_t::PARSEERROR;

			OLumpName skyname = skyelem.asString();
			sky_t* sky = R_GetSky(skyname, true);

			skyflatlookup[flat_res_id] = sky;
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

static void spreadFire(int src, byte* firepixels, int width)
{
	const byte pixel = firepixels[src];
	const int copyloc0 = src - width;
	if (pixel == 0) {
		if (copyloc0 >= 0)
			firepixels[copyloc0] = 0;
	} else {
		const int rand = static_cast<int>(std::round(M_RandomFloat() * 3.0)) & 3;
		const int copyloc1 = copyloc0 - rand + 1;
		if (copyloc1 >= 0)
			firepixels[copyloc1] = pixel - (rand & 1);
	}
}

static void R_UpdateFireSky(sky_t* sky, bool init = false)
{
	if (gametic % sky->fireticrate != 0 && !init) return;
	const Texture* tex = R_SkyTexTexture(&sky->background);
	if (!tex) return;
	const int width = tex->mWidth;
	const int height = tex->mHeight;
	for (int x = 0 ; x < width; x++)
	{
		for (int y = 1; y < height; y++)
		{
			spreadFire(y * width + x, sky->firetexturedata, width);
		}
	}
	for (int x = 0; x < width; x++)
	{
		// Texture stores its columns contiguously, so writing per-column
		// works the same way the old column-data accessor did.
		palindex_t* coldata = const_cast<palindex_t*>(tex->getColumn(x));
		for (int y = 0; y < height; y++)
		{
			coldata[y] = sky->firepalette[sky->firetexturedata[y * width + x]];
		}
	}
}

static void R_InitFireSky(sky_t* sky)
{
	const Texture* tex = R_SkyTexTexture(&sky->background);
	if (!tex) return;
	const int width = tex->mWidth;
	const int height = tex->mHeight;
	sky->firetexturedata = Z_Malloc<byte>(width * height, PU_LEVEL);
	for (int i = 0 ; i < width*height; i++)
	{
		sky->firetexturedata[i] = 0;
	}
	for (int i = 0 ; i < width; i++)
	{
		sky->firetexturedata[(height - 1) * width + i] = sky->numfireentries - 1;
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

static void R_ActivateSky(sky_t* sky)
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

//
// R_ActivateSkies
//
// Activates (and initializes, e.g. fire skies) every sky whose mapped flat
// is actually used by a sector in the current level.
//
void R_ActivateSkies()
{
	for (auto& [flat_res_id, sky] : skyflatlookup)
	{
		if (!sky)
			continue;

		bool used = false;
		for (int i = 0; i < numsectors && !used; i++)
		{
			used = sectors[i].floor_res_id == flat_res_id ||
			       sectors[i].ceiling_res_id == flat_res_id;
		}

		if (used)
			R_ActivateSky(sky);
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
	if (!skydef)
		return;

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

	sky_flat_resource_id = R_SkyFlatResourceId();
	skyflatlookup[sky_flat_resource_id] = skydef;
}


//
// R_SetSkyTextures
//
// Loads the default sky textures and re-initializes the sky map lookup tables.
//
void R_SetSkyTextures(const char* sky1_name, const char* sky2_name)
{
	sky1texture = Res_CacheTexture(OStringToUpper(sky1_name, 8), WALL);
	sky2texture = Res_CacheTexture(OStringToUpper(sky2_name, 8), WALL);

	if (!sky1texture)
		I_Error("Invalid sky1 texture \"{}\"", OStringToUpper(sky1_name, 8));

	if (sky2texture && sky1texture->mHeight != sky2texture->mHeight)
	{
		PrintFmt(PRINT_HIGH,"Both sky textures must be the same height.\n");
		sky2texture = sky1texture;
	}

	sky1columnoffset = 0;
	sky2columnoffset = 0;
	sky1scrolldelta = level.sky1ScrollDelta;
	sky2scrolldelta = sky2texture ? level.sky2ScrollDelta : 0;

	sky_flat_resource_id = R_SkyFlatResourceId();

	R_InitSkyMap();
}


//
// R_BlastSkyColumn
//
static inline void R_BlastSkyColumn(void (*drawfunc)(void))
{
	if (dcol.yl <= dcol.yh)
	{
		dcol.texturefrac = dcol.texturemid +
		                   FixedMul(((dcol.yl + 1) << FRACBITS) - centeryfrac, dcol.iscale);
		drawfunc();
	}
}

inline void SkyColumnBlaster()
{
	R_BlastSkyColumn(colfunc);
}

inline void SkyForegroundColumnBlaster()
{
	R_BlastSkyColumn(R_DrawSkyForegroundColumn);
}

//
// R_RenderSkyRange
//
// [RH] Can handle parallax skies. Note that the front sky is *not* masked in
// in the normal convention for patches, but uses color 0 as a transparent
// color.
// [ML] 5/11/06 - Removed sky2
// [BC] 7/5/24 - Brought back for real this time
// [EB] SKYDEFS support: scrolling, scaling, fire and double skies
//
void R_RenderSkyRange(visplane_t* pl)
{
	if (pl->minx > pl->maxx)
		return;

	const Texture* frontskytex = NULL;
	const Texture* backskytex = NULL;

	fixed_t front_offset = 0;
	fixed_t back_offset = 0;
	fixed_t frontrow_offset = 0;
	fixed_t backrow_offset = 0;
	angle_t skyflip = 0;
	const angle_t* xtoskyangle = r_linearsky ? linearskyangle : xtoviewangle;

	fixed_t sky1scalex = FRACUNIT;
	fixed_t sky2scalex = FRACUNIT;
	fixed_t sky1scaley = FRACUNIT;
	fixed_t sky2scaley = FRACUNIT;
	fixed_t sky1mid = skytexturemid;
	fixed_t sky2mid = skytexturemid;

	auto skyflat = skyflatlookup.find(pl->res_id);

	if (pl->sky_transfer == PL_SKYFLAT)
	{
		// use sky2
		frontskytex = sky2texture ? sky2texture : sky1texture;
		front_offset = sky2columnoffset;
	}
	else if (pl->sky_transfer & PL_SKYFLAT)
	{
		// MBF's linedef-controlled skies
		uint32_t linenum = (pl->sky_transfer & ~PL_SKYFLAT) - 1;
		if (linenum >= numlines)
			linenum = 0;
		const line_t* line = &lines[linenum];

		// Sky transferred from first sidedef
		const side_t* side = *line->sidenum + sides;

		// Texture comes from upper texture of reference sidedef
		frontskytex = Res_CacheTexture(Res_GetAnimatedTextureResourceId(side->toptexture));

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
	else if (skyflat != skyflatlookup.end() && skyflat->second)
	{
		const sky_t* sky = skyflat->second;
		if (sky->type == skytype_t::DOUBLESKY)
		{
			frontskytex = R_SkyTexTexture(&sky->foreground);
			backskytex = R_SkyTexTexture(&sky->background);
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
			frontskytex = R_SkyTexTexture(&sky->background);
			backskytex = NULL;
			front_offset = sky->background.currx;
			frontrow_offset = sky->background.curry;
			sky1scalex = sky->background.scalex;
			sky1scaley = sky->background.scaley;
			if (!sky->usedefaultmid)
				sky1mid = sky->background.mid;
		}
	}
	else
	{
		// default sky1 (no SKYDEFS mapping present)
		frontskytex = sky1texture;

		if (level.flags & LEVEL_DOUBLESKY)
			backskytex = sky2texture;

		front_offset = sky1columnoffset;
		back_offset = sky2columnoffset;
	}

	if (!frontskytex)
		return;

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

	skyplane = pl;
	dcol.masked = false;

	// Background sky layer (only present when a foreground layer will be
	// composited on top of it).
	if (backskytex)
	{
		dcol.iscale = FixedMul(skyiscale, sky2scaley) >> skystretch;
		dcol.texturemid = sky2mid + backrow_offset;
		dcol.textureheight = backskytex->mHeight << FRACBITS;
		dcol.texturedata = backskytex->mData;
		dcol.argbtexturedata = backskytex->mARGBData;

		for (int x = pl->minx; x <= pl->maxx; x++)
		{
			int colnum = ((((viewangle + xtoskyangle[x]) ^ skyflip) >> sky2shift) + back_offset) >> FRACBITS;
			colnum = FIXED2INT(FixedMul(INT2FIXED(colnum), sky2scalex));
			colnum &= (1 << backskytex->mWidthBits) - 1;
			skyposts[x] = backskytex->getColumn(colnum);
		}

		R_RenderColumnRange(pl->minx, pl->maxx, reinterpret_cast<int*>(pl->top), reinterpret_cast<int*>(pl->bottom),
				skyposts, SkyColumnBlaster, false, 2);
	}

	// Foreground (or only) sky layer.
	dcol.iscale = FixedMul(skyiscale, sky1scaley) >> skystretch;
	dcol.texturemid = sky1mid + frontrow_offset;
	dcol.textureheight = frontskytex->mHeight << FRACBITS;
	dcol.texturedata = frontskytex->mData;
	dcol.argbtexturedata = frontskytex->mARGBData;

	for (int x = pl->minx; x <= pl->maxx; x++)
	{
		int colnum = ((((viewangle + xtoskyangle[x]) ^ skyflip) >> sky1shift) + front_offset) >> FRACBITS;
		colnum = FIXED2INT(FixedMul(INT2FIXED(colnum), sky1scalex));
		colnum &= (1 << frontskytex->mWidthBits) - 1;
		skyposts[x] = frontskytex->getColumn(colnum);
	}

	// When compositing over a background layer, palette index 0 in the
	// foreground texture is treated as transparent (ID24 convention).
	R_RenderColumnRange(pl->minx, pl->maxx, reinterpret_cast<int*>(pl->top), reinterpret_cast<int*>(pl->bottom), skyposts,
			backskytex ? SkyForegroundColumnBlaster : SkyColumnBlaster, false, 2);

	R_ResetDrawFuncs();
}

VERSION_CONTROL (r_sky_cpp, "$Id$")
