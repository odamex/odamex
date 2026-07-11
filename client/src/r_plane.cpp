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
//	Here is a core component: drawing the floors and ceilings,
//	while maintaining a per column clipping list only.
//	Moreover, the sky areas have to be determined.
//
//		MAXVISPLANES is no longer a limit on the number of visplanes,
//		but a limit on the number of hash slots; larger numbers mean
//		better performance usually but after a point they are wasted,
//		and memory and time overheads creep in.
//
//													-Lee Killough
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <stdlib.h>
#include <math.h>

#include "z_zone.h"
#include "w_wad.h"
#include "m_mempool.h"

#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "p_mapformat.h"

#include "m_alloc.h"
#include "i_video.h"
#include "v_video.h"

#include "m_vectors.h"

#include "resources/res_texture.h"

planefunction_t 		floorfunc;
planefunction_t 		ceilingfunc;

// Here comes the obnoxious "visplane".
#define MAXVISPLANES 128    /* must be a power of 2 */

static constexpr float flatwidth = 64.0f;
static constexpr float flatheight = 64.0f;

static visplane_t		*visplanes[MAXVISPLANES + 1];	// killough
static visplane_t		*freetail;					// killough
static visplane_t		**freehead = &freetail;		// killough

static bool r_InSkyBox;

visplane_t 				*floorplane;
visplane_t 				*ceilingplane;
visplane_t				*skyplane;

// killough -- hash function for visplanes
// Empirically verified to be fairly uniform:

#define visplane_hash(picnum,lightlevel,secplane) \
  (static_cast<unsigned>((picnum)*3+(lightlevel)+(secplane.d)*7) & (MAXVISPLANES-1))

//
// Clip values are the solid pixel bounding the range.
//	floorclip starts out SCREENHEIGHT-1
//	ceilingclip starts out 0
//
std::unique_ptr<int[]> floorclip;
std::unique_ptr<int[]> ceilingclip;
std::unique_ptr<int[]> floorclipinitial;
std::unique_ptr<int[]> ceilingclipinitial;

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
std::unique_ptr<int[]> spanstart;

//
// texture mapping
//
extern fixed_t FocalLengthX, FocalLengthY;
extern float xfoc, yfoc;
extern float focratio, ifocratio;
extern Pool<int> sprclip_pool;

int*					planezlight;
float					plight, shade;

std::unique_ptr<fixed_t[]> yslope;

static double			pl_xscale, pl_yscale;
static double			pl_viewsin, pl_viewcos;
static double			pl_viewxtrans, pl_viewytrans;
static double			pl_xstepscale, pl_ystepscale;
static double			pl_planeheight;


//
// R_DoubleToDsFixed
//
// Converts a double to 16.16 fixed point with the same low-32-bit wrapping
// the fixed-point pipeline had, avoiding the undefined behavior of casting
// an out-of-range double directly to a 32-bit integer.
//
static inline dsfixed_t R_DoubleToDsFixed(double value)
{
	return static_cast<dsfixed_t>(static_cast<int64_t>(value * 65536.0));
}

v3float_t				a, b, c;
float					ixscale, iyscale;

//
// R_InitPlanes
// Only at game startup.
//
void R_InitPlanes (void)
{
	// Doh!
}

//
// R_MapSlopedPlane
//
// Calculates the vectors a, b, & c, which are used to texture map a sloped
// plane.
//
// Based in part on R_MapSlope() and R_SlopeLights() from Eternity Engine,
// written by SoM/Quasar
//
void R_MapSlopedPlane(int y, int x1, int x2)
{
	int len = x2 - x1 + 1;
	if (len <= 0)
		return;

	// center of the view plane
	// use the sub-pixel view center (centeryfrac) so the mapping tracks
	// the exact y-shear from mouselook instead of snapping to whole pixels
	v3float_t s;
	s.x = x1 - centerx;
	s.y = static_cast<float>(y + 1 - FIXED2DOUBLE(centeryfrac));
	s.z = xfoc;

	dspan.iu = M_DotProductVec3f(&s, &a) * flatwidth;
	dspan.iv = M_DotProductVec3f(&s, &b) * flatheight;
	dspan.id = M_DotProductVec3f(&s, &c);

	dspan.iustep = a.x * flatwidth;
	dspan.ivstep = b.x * flatheight;
	dspan.idstep = c.x;

	// From R_SlopeLights, Eternity Engine
	float id = dspan.id + dspan.idstep * (x2 - x1);
	float map1 = 256.0f - (shade - plight * dspan.id);
	float map2 = 256.0f - (shade - plight * id);

	if (fixedlightlev)
	{
		for (int i = 0; i < len; i++)
			dspan.slopelighting[i] = basecolormap.with(fixedlightlev);
	}
	else if (fixedcolormap.isValid())
	{
		for (int i = 0; i < len; i++)
			dspan.slopelighting[i] = fixedcolormap;
	}
	else
	{
		fixed_t mapstart = FLOAT2FIXED((256.0f - map1) / 256.0f * NUMCOLORMAPS);
		fixed_t mapend = FLOAT2FIXED((256.0f - map2) / 256.0f * NUMCOLORMAPS);
		fixed_t map = mapstart;
		fixed_t step = 0;

		step = (mapend - mapstart) / len;

		for (int i = 0; i < len; i++)
		{
			int index = static_cast<int>(map >> FRACBITS) + 1;
			index -= (foggy ? 0 : extralight << 2);

			if (index < 0)
				dspan.slopelighting[i] = basecolormap;
			else if (index >= NUMCOLORMAPS)
				dspan.slopelighting[i] = basecolormap.with((NUMCOLORMAPS - 1));
			else
				dspan.slopelighting[i] = basecolormap.with(index);

			map += step;
		}
	}

   	dspan.y = y;
	dspan.x1 = x1;
	dspan.x2 = x2;

	spanslopefunc();
}


//
// R_MapLevelPlane
//
// [SL] 2012-11-09 - Based loosely on R_MapPlane() from PrBoom+ to increase
// the accuracy of texture-mapping visplanes with the same textures.
//
// e6y
//
// [RH]Instead of using the xtoviewangle array, I calculated the fractional values
// at the middle of the screen, then used the calculated ds_xstep and ds_ystep
// to step from those to the proper texture coordinate to start drawing at.
// That way, the texture coordinate is always calculated by its position
// on the screen and not by its position relative to the edge of the visplane.
//
// Visplanes with the same texture now match up far better than before.
//
void R_MapLevelPlane(int y, int x1, int x2)
{
	const double distance = pl_planeheight * FIXED2DOUBLE(yslope[y]);

	const double slope = distance / xfoc;

	const double ustep = pl_xstepscale * slope;
	const double vstep = pl_ystepscale * slope;

	double ufrac = pl_viewxtrans + pl_viewcos * distance * pl_xscale +
				(x1 - centerx) * ustep;
	double vfrac = pl_viewytrans - pl_viewsin * distance * pl_yscale +
				(x1 - centerx) * vstep;

	// Wrap up into a FRACUNIT at most before converting back to fixed point.
	ufrac -= 65536.0 * floor(ufrac / 65536.0);
	vfrac -= 65536.0 * floor(vfrac / 65536.0);

	dspan.ustep = R_DoubleToDsFixed(ustep);
	dspan.vstep = R_DoubleToDsFixed(vstep);
	dspan.ufrac = R_DoubleToDsFixed(ufrac);
	dspan.vfrac = R_DoubleToDsFixed(vfrac);

	if (fixedlightlev)
		dspan.colormap = basecolormap.with(fixedlightlev);
	else if (fixedcolormap.isValid())
		dspan.colormap = fixedcolormap;
	else
	{
		// Determine lighting based on the span's distance from the viewer.
		unsigned int index = MAXLIGHTZ - 1;
		const double lightdist = distance * 65536.0;
		if (lightdist >= 0.0 && lightdist < double(MAXLIGHTZ) * double(1 << LIGHTZSHIFT))
			index = static_cast<unsigned int>(lightdist) >> LIGHTZSHIFT;

		dspan.colormap = basecolormap.with(planezlight[index]);
	}

	dspan.y = y;
	dspan.x1 = x1;
	dspan.x2 = x2;

	spanfunc();
}

//
// R_ClearPlanes
// At begining of frame.
//
void R_ClearPlanes(bool fullclear)
{
	for (int i = 0; i < MAXVISPLANES; i++)	// new code -- killough
		for (*freehead = visplanes[i], visplanes[i] = NULL; *freehead; )
			freehead = &(*freehead)->next;

	if (fullclear)
	{
		// opening / clipping determination
		memcpy(floorclip.get(), floorclipinitial.get(), viewwidth * sizeof(floorclip[0]));
		memcpy(ceilingclip.get(), ceilingclipinitial.get(), viewwidth * sizeof(ceilingclip[0]));
	}
}

//
// New function, by Lee Killough
// [RH] top and bottom buffers get allocated immediately
//		after the visplane.
//
static visplane_t *new_visplane(unsigned hash)
{
	visplane_t *check = freetail;

	if (!check)
	{
		check = static_cast<visplane_t*>(M_Calloc(1, sizeof(*check) + sizeof(*check->top)*2*I_GetSurfaceWidth()));
		check->bottom = &check->top[I_GetSurfaceWidth() + 2];
	}
	else
		if (!(freetail = freetail->next))
			freehead = &freetail;
	check->next = visplanes[hash];
	visplanes[hash] = check;
	return check;
}


//
// R_FindPlane
//
// killough 2/28/98: Add offsets
//
visplane_t* R_FindPlane(
		const plane_t& secplane,
		ResourceId res_id,
		uint32_t sky_transfer,
		int lightlevel,
		fixed_t xoffs, fixed_t yoffs,
		fixed_t xscale, fixed_t yscale,
		angle_t angle,
		AActor::AActorPtr skybox)
{
	visplane_t *check;
	unsigned hash;						// killough
	bool isskybox;


	if (R_ResourceIdIsSkyFlat(res_id) || (sky_transfer & PL_SKYFLAT))  // killough 10/98
	{
		lightlevel = 0;		// most skies map together
		isskybox = R_ResourceIdIsSkyFlat(res_id) && (skybox != NULL) && !r_InSkyBox;
	}
	else
	{
		isskybox = false;
	}

	// New visplane algorithm uses hash table -- killough
	hash = isskybox ? MAXVISPLANES : visplane_hash(res_id, lightlevel, secplane);

	for (check = visplanes[hash]; check; check = check->next) // killough
	{
		if (isskybox)
		{
			if (skybox == check->skybox)
			{
				return check;
			}
		}
		else if (P_IdenticalPlanes(&secplane, &check->secplane) &&
			res_id == check->res_id &&
			sky_transfer == check->sky_transfer &&
			lightlevel == check->lightlevel &&
			xoffs == check->xoffs &&	// killough 2/28/98: Add offset checks
			yoffs == check->yoffs &&
			basecolormap == check->colormap &&	// [RH] Add colormap check
			xscale == check->xscale &&
			yscale == check->yscale &&
			angle == check->angle
			)
		{
			return check;
		}
	}

	check = new_visplane (hash);		// killough

	memcpy(&check->secplane, &secplane, sizeof(secplane));
	check->res_id = res_id;
	check->sky_transfer = sky_transfer;
	check->lightlevel = lightlevel;
	check->xoffs = xoffs;				// killough 2/28/98: Save offsets
	check->yoffs = yoffs;
	check->xscale = xscale;
	check->yscale = yscale;
	check->angle = angle;
	check->colormap = basecolormap;		// [RH] Save colormap
	check->skybox = skybox;
	check->minx = viewwidth;			// Was SCREENWIDTH -- killough 11/98
	check->maxx = -1;

	memcpy(check->top, viewheightarray, viewwidth * sizeof(*check->top));

	return check;
}

//
// R_CheckPlane
//
visplane_t* R_CheckPlane(visplane_t* pl, int start, int stop)
{
    int		intrl;
    int		intrh;
    int		unionl;
    int		unionh;
    int		x;

	if (start < pl->minx)
	{
		intrl = pl->minx;
		unionl = start;
	}
	else
	{
		unionl = pl->minx;
		intrl = start;
	}

	if (stop > pl->maxx)
	{
		intrh = pl->maxx;
		unionh = stop;
	}
	else
	{
		unionh = pl->maxx;
		intrh = stop;
	}

	for (x = intrl ; x <= intrh && pl->top[x] == static_cast<unsigned int>(viewheight); x++)
		;

	if (x > intrh)
	{
		// use the same visplane
		pl->minx = unionl;
		pl->maxx = unionh;
	}
	else
	{
		// make a new visplane
		unsigned hash;

		if (R_ResourceIdIsSkyFlat(pl->res_id) && pl->skybox != NULL && !r_InSkyBox)
		{
			hash = MAXVISPLANES;
		}
		else
		{
			hash = visplane_hash(pl->res_id, pl->lightlevel, pl->secplane);
		}
		visplane_t *new_pl = new_visplane (hash);

		new_pl->secplane = pl->secplane;
		new_pl->res_id = pl->res_id;
		new_pl->sky_transfer = pl->sky_transfer;
		new_pl->lightlevel = pl->lightlevel;
		new_pl->xoffs = pl->xoffs;			// killough 2/28/98
		new_pl->yoffs = pl->yoffs;
		new_pl->xscale = pl->xscale;
		new_pl->yscale = pl->yscale;
		new_pl->angle = pl->angle;
		new_pl->colormap = pl->colormap;	// [RH] Copy colormap
		new_pl->skybox = pl->skybox;
		pl = new_pl;
		pl->minx = start;
		pl->maxx = stop;
		memcpy(pl->top, viewheightarray, viewwidth * sizeof(*pl->top));
	}
	return pl;
}

//
// R_MakeSpans
//
void R_MakeSpans(visplane_t *pl, void(*spanfunc)(int, int, int))
{
	for (int x = pl->minx; x <= pl->maxx + 1; x++)
	{
		unsigned int t1 = pl->top[x-1];
		unsigned int b1 = pl->bottom[x-1];
		unsigned int t2 = pl->top[x];
		unsigned int b2 = pl->bottom[x];

		for (; t1 < t2 && t1 <= b1; t1++)
			spanfunc(t1, spanstart[t1], x-1);
		for (; b1 > b2 && b1 >= t1; b1--)
			spanfunc(b1, spanstart[b1], x-1);
		while (t2 < t1 && t2 <= b2)
			spanstart[t2++] = x;
		while (b2 > b1 && b2 >= t2)
			spanstart[b2--] = x;
	}
}

//
// R_DrawSlopedPlane
//
// Calculates the vectors a, b, & c, which are used to texture map a sloped
// plane.
//
// Based in part on R_CalcSlope() from Eternity Engine, written by SoM.
//
void R_DrawSlopedPlane(visplane_t *pl)
{
	const double xoffs = FIXED2DOUBLE(pl->xoffs);
	const double yoffs = FIXED2DOUBLE(pl->yoffs);
	const double scaledflatwidth = flatwidth * FIXED2DOUBLE(pl->xscale);
	const double scaledflatheight = flatheight * FIXED2DOUBLE(pl->yscale);

	// world-space points on the plane (x, z horizontal, y = height)
	double px, py, pz, tx, ty, tz, sx, sy, sz;

	// [SL] optimize when the texture rotation angle is zero (most of the time)
	if (pl->angle == 0)
	{
		// Point p is the anchor point of the texture.  It starts out as the
		// map coordinate (0, 0, planez(0,0)) but texture offset gets applied
		px = -xoffs;
		pz = yoffs;
		py = P_PlaneZ(px, pz, &pl->secplane);

		// Point t is the point along the plane (texwidth, 0, planez(texwidth, 0)) with texture
		// offset applied
		tx = px - scaledflatwidth;
		tz = pz;
		ty = P_PlaneZ(tx, tz, &pl->secplane);

		// Point s is the point along the plane (0, texheight, planez(0, texheight)) with texture
		// offset applied
		sx = px;
		sz = pz + scaledflatheight;
		sy = P_PlaneZ(sx, sz, &pl->secplane);
	}
	else
	{
		const double sinang = sin((pl->angle + ANG90) * ANGLE_TO_RAD);
		const double cosang = cos((pl->angle + ANG90) * ANGLE_TO_RAD);

		// Point p is the anchor point of the texture.  It starts out as the
		// map coordinate (0, 0, planez(0,0)) but texture offset and rotation get applied
		px = -yoffs * cosang - xoffs * sinang;
		pz = -xoffs * cosang + yoffs * sinang;
		py = P_PlaneZ(px, pz, &pl->secplane);

		// Point t is the point along the plane (texwidth, 0, planez(texwidth, 0)) with texture
		// offset and rotation applied
		tx = px - scaledflatwidth * sinang;
		tz = pz + scaledflatwidth * cosang;
		ty = P_PlaneZ(tx, tz, &pl->secplane);

		// Point s is the point along the plane (0, texheight, planez(0, texheight)) with texture
		// offset and rotation applied
		sx = px + scaledflatheight * cosang;
		sz = pz + scaledflatheight * sinang;
		sy = P_PlaneZ(sx, sz, &pl->secplane);
	}

	// Translate the points to their position relative to viewx, viewy and
	// rotate them based on viewangle (exact trig, see M_TranslateVec3f for
	// the coordinate-system conventions)
	const double rotrad = (ANG90 - viewangle) * ANGLE_TO_RAD;
	const double rcos = cos(rotrad);
	const double rsin = sin(rotrad);
	const double viewxd = FIXED2DOUBLE(viewx);
	const double viewyd = FIXED2DOUBLE(viewy);
	const double viewzd = FIXED2DOUBLE(viewz);

	auto translate = [&](double& x, double& y, double& z)
	{
		const double dx = x - viewxd;
		const double dy = viewzd - y;
		const double dz = z - viewyd;

		x = dx * rcos - dz * rsin;
		z = dz * rcos + dx * rsin;
		y = dy;
	};

	translate(px, py, pz);
	translate(tx, ty, tz);
	translate(sx, sy, sz);

	// Subtract p from t and s, making t and s into direction vectors
	tx -= px; ty -= py; tz -= pz;
	sx -= px; sy -= py; sz -= pz;

	// a = p cross s, b = t cross p, c = t cross s, each scaled by half and
	// with the y component corrected for the aspect ratio of the view
	a.x = static_cast<float>(0.5 * (py * sz - pz * sy));
	a.y = static_cast<float>(0.5 * (pz * sx - px * sz) * ifocratio);
	a.z = static_cast<float>(0.5 * (px * sy - py * sx));

	b.x = static_cast<float>(0.5 * (ty * pz - tz * py));
	b.y = static_cast<float>(0.5 * (tz * px - tx * pz) * ifocratio);
	b.z = static_cast<float>(0.5 * (tx * py - ty * px));

	c.x = static_cast<float>(0.5 * (ty * sz - tz * sy));
	c.y = static_cast<float>(0.5 * (tz * sx - tx * sz) * ifocratio);
	c.z = static_cast<float>(0.5 * (tx * sy - ty * sx));

	// (SoM) More help from randy. I was totally lost on this...
	float scalenumer = FIXED2FLOAT(finetangent[FINEANGLES/4+CorrectFieldOfView/2]);
	float ixscale = scalenumer / flatwidth;
	float iyscale = scalenumer / flatheight;

	const double zat = P_PlaneZ(viewxd, viewyd, &pl->secplane);

	angle_t fovang = ANG(consoleplayer().fov / 2.0f);
	float slopetan = FIXED2FLOAT(finetangent[fovang >> ANGLETOFINESHIFT]);
	float slopevis = 8.0 * slopetan * 16.0 * 320.0 / float(I_GetSurfaceWidth());

	plight = (slopevis * ixscale * iyscale) / (zat - viewzd);
	shade = 256.0 * 2.0 - (pl->lightlevel + 16.0) * 256.0 / 128.0;

	basecolormap = pl->colormap;	// [RH] set basecolormap

	R_MakeSpans(pl, R_MapSlopedPlane);
}


//
// R_DrawLevelPlane
//
void R_DrawLevelPlane(visplane_t *pl)
{
	// viewx/viewy rotated by the texture rotation angle
	double pl_viewx, pl_viewy;

	// texture scaling factor
	pl_xscale = FIXED2DOUBLE(pl->xscale);
	pl_yscale = FIXED2DOUBLE(pl->yscale);

	const angle_t rotation = viewangle + pl->angle;
	pl_viewsin = sin(rotation * ANGLE_TO_RAD);
	pl_viewcos = cos(rotation * ANGLE_TO_RAD);

	const double xoffs = FIXED2DOUBLE(pl->xoffs);
	const double yoffs = FIXED2DOUBLE(pl->yoffs);
	const double viewx_d = FIXED2DOUBLE(viewx);
	const double viewy_d = FIXED2DOUBLE(viewy);

	if (pl->angle == 0)
	{
		pl_viewx = xoffs + viewx_d;
		pl_viewy = yoffs - viewy_d;
	}
	else
	{
		const double pl_cos = cos(pl->angle * ANGLE_TO_RAD);
		const double pl_sin = sin(pl->angle * ANGLE_TO_RAD);

		if (map_format.getZDoom())
		{
			pl_viewx = xoffs + viewx_d * pl_cos - viewy_d * pl_sin;
			pl_viewy = yoffs - (viewx_d * pl_sin + viewy_d * pl_cos);
		}
		else
		{
			pl_viewx = (viewx_d + xoffs) * pl_cos - (viewy_d - yoffs) * pl_sin;
			pl_viewy = -((viewx_d + xoffs) * pl_sin + (viewy_d - yoffs) * pl_cos);
		}
	}

	// cache a calculation used by R_MapLevelPlane
	pl_xstepscale = pl_viewsin * pl_xscale;
	pl_ystepscale = pl_viewcos * pl_yscale;

	// cache a calculation used by R_MapLevelPlane
	pl_viewxtrans = (pl_viewx + xoffs) * pl_xscale;
	pl_viewytrans = (pl_viewy + yoffs) * pl_yscale;

	basecolormap = pl->colormap;	// [RH] set basecolormap

	// [SL] 2012-02-05 - Plane's height should be constant for all (x,y)
	// so just use (0, 0) when calculating the plane's z height
	pl_planeheight = FIXED2DOUBLE(abs(P_PlaneZ(0, 0, &pl->secplane) - viewz));

	int light = clamp((pl->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight), 0, LIGHTLEVELS - 1);
	planezlight = zlight[light];

	R_MakeSpans(pl, R_MapLevelPlane);
}


//
// R_DrawPlanes
//
// At the end of each frame.
//
void R_DrawPlanes()
{
	R_ResetDrawFuncs();
	dspan.color = 3;
	
	for (int i = 0; i < MAXVISPLANES; i++)
	{
		for (visplane_t* pl = visplanes[i]; pl; pl = pl->next)
		{
			if (pl->minx > pl->maxx)
				continue;

			const ResourceId res_id = Res_GetAnimatedTextureResourceId(pl->res_id);
			if (R_ResourceIdIsSkyFlat(res_id) || (pl->sky_transfer & PL_SKYFLAT))
			{
				R_RenderSkyRange(pl);
			}
			else
			{
				// regular flat
				dspan.color += 4;	// [RH] color if r_drawflat is 1

				const Texture* texture = Res_CacheTexture(res_id, PU_STATIC);
				dspan.source = texture->mData;
				// the 32bpp drawers sample the native ARGB plane when the
				// texture carries one (NULL otherwise)
				dspan.argbsource = texture->mARGBData;

				// [SL] Note that the texture orientation differs from typical Doom span
				// drawers since flats are stored in column major format now. The roles
				// of ufrac and vfrac have been reversed to accomodate this.
				dspan.umask = texture->mWidthMask << texture->mHeightBits;
				dspan.vmask = texture->mHeightMask;
				dspan.ushift = FRACBITS - texture->mHeightBits;
				dspan.vshift = FRACBITS;
										   
				#if 0
				// TODO: Remove "useflatnum" and implement warped flats
				int useflatnum = 0;
				// [RH] warp a flat if desired
				if (flatwarp[useflatnum])
				{
					if (warpedflats[useflatnum] && flatwarpedwhen[useflatnum] == level.time)
					{
						Z_ChangeTag(dspan.source, PU_CACHE);
						dspan.source = warpedflats[useflatnum];
						Z_ChangeTag(dspan.source, PU_STATIC);
					}
					else
					{
						if (!warpedflats[useflatnum])
							warpedflats[useflatnum] = Z_Malloc<byte>(64*64, PU_STATIC, &warpedflats[useflatnum]);

						static byte buffer[64];
						int timebase = level.time*23;

						flatwarpedwhen[useflatnum] = level.time;
						byte *warped = warpedflats[useflatnum];

						for (int x = 63; x >= 0; x--)
						{
							int yt, yf = (finesine[(timebase + ((x+17) << 7))&FINEMASK]>>13) & 63;
							const byte *source = dspan.source + x;
							byte *dest = warped + x;
							for (yt = 64; yt; yt--, yf = (yf+1)&63, dest += 64)
								*dest = *(source + (yf << 6));
						}
						timebase = level.time*32;
						for (int y = 63; y >= 0; y--)
						{
							int xt, xf = (finesine[(timebase + (y << 7))&FINEMASK]>>13) & 63;
							const byte *source = warped + (y << 6);
							byte *dest = buffer;
							for (xt = 64; xt; xt--, xf = (xf+1) & 63)
								*dest++ = *(source+xf);
							memcpy (warped + (y << 6), buffer, 64);
						}
						Z_ChangeTag (dspan.source, PU_CACHE);
						dspan.source = warped;
					}
				}
				#endif	// if 0
				
				pl->top[pl->maxx+1] = viewheight;
				pl->top[pl->minx-1] = viewheight;

				if (P_IsPlaneLevel(&pl->secplane))
					R_DrawLevelPlane(pl);
				else
					R_DrawSlopedPlane(pl);
					
				Z_ChangeTag((void*)texture, PU_CACHE);
			}
		}
	}
}

//==========================================================================
//
// R_DrawSkyBoxes
//
// Draws any recorded sky boxes and then frees them.
//
// The process:
//   1. Move the camera to coincide with the SkyViewpoint.
//   2. Clear out the old planes. (They have already been drawn.)
//   3. Clear a window out of the ClipSegs just large enough for the plane.
//   4. Pretend the existing vissprites and drawsegs aren't there.
//   5. Create a drawseg at 0 distance to clip sprites to the visplane. It
//      doesn't need to be associated with a line in the map, since there
//      will never be any sprites in front of it.
//   6. Render the BSP, then planes, then masked stuff.
//   7. Restore the previous vissprites and drawsegs.
//   8. Repeat for any other sky boxes.
//   9. Put the camera back where it was to begin with.
//
//==========================================================================

void R_DrawSkyBoxes()
{
	if (visplanes[MAXVISPLANES] == NULL)
		return;

	int savedextralight = extralight;
	fixed_t savedx = viewx;
	fixed_t savedy = viewy;
	fixed_t savedz = viewz;
	angle_t savedangle = viewangle;
	ptrdiff_t savedvissprite_p = vissprite_p - vissprites;
	ptrdiff_t savedds_p = ds_p - drawsegs;
	AActor* savedcamera = camera;

	int i;
	visplane_t* pl;

	// Don't draw sky boxes inside sky boxes.
	r_InSkyBox = true;

	// Don't let gun flashes brighten the sky box
	extralight = 0;

	for (pl = visplanes[MAXVISPLANES]; pl != NULL; pl = pl->next)
	{
		if (pl->maxx < pl->minx)
			continue;

		AActor* sky = pl->skybox;

		viewx = sky->x;
		viewy = sky->y;
		viewz = sky->z;
		camera = sky;
		R_SetViewAngle(savedangle + sky->angle);
		validcount++; // Make sure we see all sprites

		R_ClearPlanes(false);
		R_ClearClipSegs();

		// Set up ceiling/floor clip arrays for this visplane.
		for (i = pl->minx; i <= pl->maxx; i++)
		{
			if (pl->top[i] == static_cast<unsigned int>(viewheight))
			{
				ceilingclip[i] = viewheight;
				floorclip[i] = -1;
			}
			else
			{
				ceilingclip[i] = pl->top[i];
				floorclip[i] = pl->bottom[i] + 1;
			}
		}

		// Create a drawseg to clip sprites to the sky plane.
		R_ReallocDrawSegs();
		ds_p->x1 = 0;
		ds_p->x2 = viewwidth - 1;
		ds_p->silhouette = SIL_BOTH;
		ds_p->midposts = NULL;
		ds_p->midscales = NULL;
		ds_p->curline = NULL;

		// [RK] Allocate full width clip arrays.
		int* bottomclip = sprclip_pool.alloc(viewwidth);
		int* topclip = sprclip_pool.alloc(viewwidth);

		// [RK] Copy visplane clip values into the arrays.
		memcpy(bottomclip, floorclip.get(), viewwidth * sizeof(*bottomclip));
		memcpy(topclip, ceilingclip.get(), viewwidth * sizeof(*topclip));

		ds_p->sprbottomclip = bottomclip;
		ds_p->sprtopclip = topclip;

		firstvissprite = vissprite_p;
		firstdrawseg = ds_p++;

		R_RenderBSPNode(numnodes - 1);
		R_DrawPlanes();
		R_DrawMasked();

		firstvissprite = vissprites;
		vissprite_p = vissprites + savedvissprite_p;
		firstdrawseg = drawsegs;
		ds_p = drawsegs + savedds_p;
	}

	camera = savedcamera;
	viewx = savedx;
	viewy = savedy;
	viewz = savedz;
	extralight = savedextralight;
	R_SetViewAngle(savedangle);

	r_InSkyBox = false;

	for (*freehead = visplanes[MAXVISPLANES], visplanes[MAXVISPLANES] = NULL; *freehead;)
		freehead = &(*freehead)->next;
}

//
// R_PlaneInitData
//
bool R_PlaneInitData(IWindowSurface* surface)
{
	int surface_width = surface->getWidth();
	int surface_height = surface->getHeight();

	floorclip = std::make_unique<int[]>(surface_width);
	ceilingclip = std::make_unique<int[]>(surface_width);
	floorclipinitial = std::make_unique<int[]>(surface_width);
	ceilingclipinitial = std::make_unique<int[]>(surface_width);

	for (int i = 0; i < surface_width; i++)
	{
		ceilingclipinitial[i] = -1;
		floorclipinitial[i] = viewheight;
	}

	spanstart = std::make_unique<int[]>(surface_height);
	yslope = std::make_unique<fixed_t[]>(surface_height);

	// Free all visplanes and let them be re-allocated as needed.
	visplane_t* pl = freetail;

	while (pl)
	{
		visplane_t *next = pl->next;
		M_Free(pl);
		pl = next;
	}
	freetail = NULL;
	freehead = &freetail;

	for (int i = 0; i < MAXVISPLANES; i++)
	{
		pl = visplanes[i];
		visplanes[i] = NULL;
		while (pl)
		{
			visplane_t *next = pl->next;
			M_Free(pl);
			pl = next;
		}
	}

	return true;
}

VERSION_CONTROL (r_plane_cpp, "$Id$")
