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

#include <algorithm>

#include <stdlib.h>
#include <math.h>

#include "z_zone.h"
#include "w_wad.h"
#include "m_mempool.h"

#include "p_local.h"
#include "r_local.h"
#include "r_context.h"
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
static constexpr float flatwidth = 64.0f;
static constexpr float flatheight = 64.0f;

// Visplane headers come from dense blocks so that walking a hash chain or the
// free list stays within a few pages. Their column spans are allocated
// separately; those are only touched when a plane is actually drawn.
// Now, visplane state is kept within its render context instead of global.
// ----------------------------------------------------------------------------
static auto&	visplanes = ::rctx.plane.visplanes;
static auto&	freetail = ::rctx.plane.freetail;
static auto&	freehead = ::rctx.plane.freehead;

static auto&	visplane_blocks = ::rctx.plane.visplane_blocks;
static auto&	visplane_block_used = ::rctx.plane.visplane_block_used;
static auto&	visplane_spans = ::rctx.plane.visplane_spans;

namespace
{

} // namespace

// Depth of the portal view currently being rendered.
// 0 = the main view.
// Portal planes created at depth N are rendered as portal passes at depth
// N+1, up to r_portalrecursions, beyond that they draw as regular sky.
static int r_PortalDepth;

EXTERN_CVAR(r_portalrecursions)

// Is this actor a stacked-sector portal anchor?
static bool R_IsStackPoint(const AActor* mo)
{
	return mo && (mo->type == MT_UPPERSTACK || mo->type == MT_LOWERSTACK);
}

// Alpha of the boundary flat drawn over a stack portal's content: 0 = invisible
// flat, 255 = fully opaque.
static int R_StackFlatAlpha(const AActor* mo)
{
	const AActor* local = mo->tracer;

	// Unpaired boundary: nothing to see through, so draw the flat as it is.
	if (local == nullptr)
		return 255;

	return std::clamp(static_cast<int>(local->args[0]), 0, 255);
}

// Can the view see through this boundary? A flat at full opacity hides
// whatever is behind it, so the plane just draws normally.
bool R_IsStackBoundary(const AActor* mo)
{
	return R_IsStackPoint(mo) && R_StackFlatAlpha(mo) < 255;
}

// Stack points of the portal passes being rendered, innermost last.
std::vector<AActor*> r_ActiveStackPortals;

// Is either end of this boundary's pair already being rendered?
// If so, prevent it from being entered again.
bool R_IsStackPairActive(const AActor* mo)
{
	const auto samepair = [mo](const AActor* active)
	{
		const AActor* mate = active->tracer;
		return active == mo || (mate != nullptr && mate == mo);
	};

	return std::ranges::any_of(r_ActiveStackPortals, samepair);
}

// Does this plane render a portal pass, or only its own boundary flat?
bool R_IsStackPortal(const AActor* mo)
{
	return R_IsStackBoundary(mo) && !R_IsStackPairActive(mo);
}


// killough -- hash function for visplanes
// Empirically verified to be fairly uniform:

#define visplane_hash(picnum,lightlevel,secplane) \
  (static_cast<unsigned>((picnum)*3+(lightlevel)+(secplane.d)*7) & (MAXVISPLANES-1))

//
// Clip values are the solid pixel bounding the range.
//	floorclip starts out SCREENHEIGHT-1
//	ceilingclip starts out 0
//
std::unique_ptr<int[]> floorclipinitial;
std::unique_ptr<int[]> ceilingclipinitial;

//
// spanstart holds the start of a plane span
// initialized to 0 at start
//
static auto& spanstart = ::rctx.plane.spanstart;

//
// texture mapping
//
extern fixed_t FocalLengthX, FocalLengthY;
extern float xfoc, yfoc;
extern float focratio, ifocratio;

static auto&      planezlight = ::rctx.plane.planezlight;
static auto&      plight = ::rctx.plane.plight;
static auto&      shade = ::rctx.plane.shade;

std::unique_ptr<fixed_t[]> yslope;

static auto&      pl_xscale = ::rctx.plane.pl_xscale;
static auto&      pl_yscale = ::rctx.plane.pl_yscale;
static auto&      pl_viewsin = ::rctx.plane.pl_viewsin;
static auto&      pl_viewcos = ::rctx.plane.pl_viewcos;
static auto&      pl_viewxtrans = ::rctx.plane.pl_viewxtrans;
static auto&      pl_viewytrans = ::rctx.plane.pl_viewytrans;
static auto&      pl_xstepscale = ::rctx.plane.pl_xstepscale;
static auto&      pl_ystepscale = ::rctx.plane.pl_ystepscale;
static auto&      pl_planeheight = ::rctx.plane.pl_planeheight;


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

// Slope plane state now lives in a render context instead of global.
static auto&      a = ::rctx.plane.slope_a;
static auto&      b = ::rctx.plane.slope_b;
static auto&      c = ::rctx.plane.slope_c;
static auto&      ixscale = ::rctx.plane.ixscale;
static auto&      iyscale = ::rctx.plane.iyscale;

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
		if (lightdist >= 0.0 && lightdist < static_cast<double>(MAXLIGHTZ) * static_cast<double>(1 << LIGHTZSHIFT))
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
		if (visplane_block_used == VISPLANE_BLOCK)
		{
			visplane_blocks.push_back(
					static_cast<visplane_t*>(M_Calloc(VISPLANE_BLOCK, sizeof(visplane_t))));
			visplane_block_used = 0;
		}
		check = &visplane_blocks.back()[visplane_block_used++];

		// one slot ahead of top for its [-1] entry, then top and bottom each
		// spanning the surface width plus the sentinel column past maxx
		const size_t width = I_GetSurfaceWidth();
		unsigned int* spans =
				static_cast<unsigned int*>(M_Calloc(2 * width + 4, sizeof(*spans)));
		visplane_spans.push_back(spans);

		check->top = spans + 1;
		check->bottom = check->top + width + 2;
	}
	else
		if (!(freetail = freetail->next))
			freehead = &freetail;

	check->next = visplanes[hash];
	visplanes[hash] = check;
	return check;
}


//
// R_PlaneMatches
//
// The chain walk's acceptance test, factored out so callers share one predicate
// rather than a second copy of it.
//
static forceinline bool R_PlaneMatches(
		const visplane_t* check,
		bool isskybox,
		const plane_t& secplane,
		ResourceId res_id,
		uint32_t sky_transfer,
		int lightlevel,
		fixed_t xoffs, fixed_t yoffs,
		fixed_t xscale, fixed_t yscale,
		angle_t angle,
		const AActor::AActorPtr& skybox)
{
	if (isskybox)
		return skybox == check->skybox;

	return P_IdenticalPlanes(&secplane, &check->secplane) &&
		res_id == check->res_id &&
		sky_transfer == check->sky_transfer &&
		lightlevel == check->lightlevel &&
		skybox == check->skybox &&	// boundary flats draw at their own alpha
		xoffs == check->xoffs &&	// killough 2/28/98: Add offset checks
		yoffs == check->yoffs &&
		basecolormap == check->colormap &&	// [RH] Add colormap check
		xscale == check->xscale &&
		yscale == check->yscale &&
		angle == check->angle;
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
		isskybox = R_ResourceIdIsSkyFlat(res_id) && (skybox != NULL) &&
		           r_PortalDepth < r_portalrecursions.asInt();
	}
	else
	{
		isskybox = R_IsStackPortal(skybox) &&
		           r_PortalDepth < r_portalrecursions.asInt();
	}

	// New visplane algorithm uses hash table -- killough
	hash = isskybox ? MAXVISPLANES : visplane_hash(res_id, lightlevel, secplane);

	for (check = visplanes[hash]; check; check = check->next) // killough
	{
		if (R_PlaneMatches(check, isskybox, secplane, res_id, sky_transfer,
				lightlevel, xoffs, yoffs, xscale, yscale, angle, skybox))
			return check;
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

	// Nothing to initialise: the range is empty, and R_CheckPlane marks
	// columns free as it adds them.

	return check;
}

//
// R_MarkPlaneColumnsFree
//
// Marks [first, last] as covering no rows yet. R_CheckPlane's free test reads
// top[x] == viewheight, so only columns being ADDED to a plane's range need it;
// those already inside [minx, maxx] were marked when they were added.
//
// Replaces initialising the full surface width every time a plane is taken into
// use -- a plane covers a small part of the screen and the rest was never read.
// Measured at 5.52 MB/frame on a heavy scene.
//
static forceinline void R_MarkPlaneColumnsFree(visplane_t* pl, int first, int last)
{
	if (first > last)
		return;

	for (int x = first; x <= last; x++)
		pl->top[x] = static_cast<unsigned int>(viewheight);
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
		// use the same visplane, marking the columns the union adds
		if (pl->maxx < pl->minx)
		{
			// empty plane -- the whole union is new
			R_MarkPlaneColumnsFree(pl, unionl, unionh);
		}
		else
		{
			R_MarkPlaneColumnsFree(pl, unionl, pl->minx - 1);
			R_MarkPlaneColumnsFree(pl, pl->maxx + 1, unionh);
		}

		pl->minx = unionl;
		pl->maxx = unionh;
	}
	else
	{
		// make a new visplane
		unsigned hash;

		if (((R_ResourceIdIsSkyFlat(pl->res_id) && pl->skybox != NULL) ||
		     R_IsStackPortal(pl->skybox)) &&
		    r_PortalDepth < r_portalrecursions.asInt())
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
		R_MarkPlaneColumnsFree(pl, start, stop);
		pl->minx = start;
		pl->maxx = stop;
	}
	return pl;
}

//
// R_MakeSpans
//
// Classic Doom span emission over the plane's own columns. The sentinel at each
// end is what opens every row at minx and flushes every row at maxx.
//
void R_MakeSpans(visplane_t* pl, void(*spanfunc)(int, int, int))
{
	const int minx = pl->minx;
	const int maxx = pl->maxx;

	const unsigned int savedleft = pl->top[minx-1];
	const unsigned int savedright = pl->top[maxx+1];
	pl->top[minx-1] = viewheight;
	pl->top[maxx+1] = viewheight;

	for (int x = minx; x <= maxx + 1; x++)
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

	pl->top[minx-1] = savedleft;
	pl->top[maxx+1] = savedright;
}


// ============================================================================
//
// Column-major level planes
//
// A level plane drawn as spans writes one pixel per column, so each pixel lands
// on its own cache line -- and under a column-sliced multithreaded renderer,
// adjacent workers false-share a line at every slice boundary.
//
// The same surface can be walked a column at a time with no per-pixel divide.
// R_MapLevelPlane computes
//
//   distance(y) = pl_planeheight * FIXED2DOUBLE(yslope[y])
//   ustep       = pl_xstepscale * distance / xfoc
//   ufrac(x,y)  = pl_viewxtrans + pl_viewcos*distance*pl_xscale + (x-centerx)*ustep
//
// and collecting that on distance for a fixed x leaves
//
//   u(y) = pl_viewxtrans + Eu(x) * yslope[y]
//   Eu(x) = pl_planeheight * ( pl_viewcos*pl_xscale + (x-centerx)*pl_xstepscale/xfoc)
//   Ev(x) = pl_planeheight * (-pl_viewsin*pl_yscale + (x-centerx)*pl_ystepscale/xfoc)
//
// which is the same value, factored the other way. Eu/Ev are affine in x, so
// one multiply-add per column; the per-pixel work is one yslope[] load and two
// multiply-adds, with no division.
//
// A visplane already stores the per-column extents this consumes, top[x] and
// bottom[x]; R_MakeSpans only ever existed to turn those back into rows.
//
// ============================================================================

namespace
{

// Per-y colormap offsets for the plane being drawn, as the relative planezlight
// value already multiplied by 256. A level plane's colormap is a pure function
// of screen y, so one table serves every column: cbase[lightoff[y] + c] is
// exactly basecolormap.with(rel).index(c), given that the shaderef_t
// constructor sets m_colormap = colors->colormap + 256 * mapnum.
std::vector<uint16_t> plane_lightoff;

// True when every row of the plane resolved to the same colormap, so the
// drawers can drop the table lookup entirely.
bool plane_constlight;

} // namespace

//
// R_BuildPlaneLighting
//
// Fills plane_lightoff over [miny, maxy] by running the identical selection
// R_MapLevelPlane runs, rather than exploiting its monotonicity -- so bit
// identity rests on the same arithmetic, and the degenerate horizon row
// reproduces today's output for free.
//
static shaderef_t R_BuildPlaneLighting(int miny, int maxy)
{
	if (plane_lightoff.size() < static_cast<size_t>(viewheight))
		plane_lightoff.resize(viewheight);

	// fixedlightlev is tested first because r_main.cpp sets both it and
	// fixedcolormap when the player's fixedcolormap is in [1, NUMCOLORMAPS).
	if (fixedlightlev)
	{
		plane_constlight = true;
		return basecolormap.with(fixedlightlev);
	}

	if (fixedcolormap.isValid())
	{
		plane_constlight = true;
		return fixedcolormap;
	}

	uint16_t first = 0;
	bool uniform = true;

	for (int y = miny; y <= maxy; y++)
	{
		const double distance = pl_planeheight * FIXED2DOUBLE(yslope[y]);

		unsigned int index = MAXLIGHTZ - 1;
		const double lightdist = distance * 65536.0;
		if (lightdist >= 0.0 && lightdist < static_cast<double>(MAXLIGHTZ) * static_cast<double>(1 << LIGHTZSHIFT))
			index = static_cast<unsigned int>(lightdist) >> LIGHTZSHIFT;

		const uint16_t off = static_cast<uint16_t>(planezlight[index] << 8);
		plane_lightoff[y] = off;

		if (y == miny)
			first = off;
		else if (off != first)
			uniform = false;
	}

	plane_constlight = uniform;

	// When every row landed in one band the table is dead weight, so resolve
	// it here and let the drawers take the constant-light template.
	return uniform ? basecolormap.with(first >> 8) : basecolormap;
}

//
// R_DrawLevelPlaneColumns
//
void R_DrawLevelPlaneColumns(visplane_t* pl)
{
	// The plane's own row range, so the lighting table costs O(rows covered)
	// rather than O(viewheight) per plane.
	int miny = viewheight, maxy = -1;

	for (int x = pl->minx; x <= pl->maxx; x++)
	{
		// An untouched column has top[x] == viewheight against a stale bottom[x],
		// and bottom is only ever written as min(..., viewheight-1), so yl > yh
		// and it drops out. Same reason R_RenderColumnRange tolerates it.
		const int yl = std::max<int>(pl->top[x], 0);
		const int yh = std::min<int>(pl->bottom[x], viewheight - 1);
		if (yl > yh)
			continue;

		miny = std::min(miny, yl);
		maxy = std::max(maxy, yh);
	}

	if (maxy < miny)
		return;

	const shaderef_t light = R_BuildPlaneLighting(miny, maxy);

	dpcol.lightoff = plane_constlight ? NULL : plane_lightoff.data();

	dpcol.cbase = light.m_colormap;
	dpcol.sbase = light.m_shademap;

	// Already 16.16: FIXED2DOUBLE's 1/65536 and the fixed conversion's 65536
	// cancel, so eu/ev multiply the raw yslope entry. The base is wrapped to keep
	// the sum in range.
	dpcol.ubase = (pl_viewxtrans - 65536.0 * floor(pl_viewxtrans / 65536.0)) * 65536.0;
	dpcol.vbase = (pl_viewytrans - 65536.0 * floor(pl_viewytrans / 65536.0)) * 65536.0;

	const double eu_base = pl_planeheight * pl_viewcos * pl_xscale;
	const double ev_base = pl_planeheight * -pl_viewsin * pl_yscale;
	const double eu_step = pl_planeheight * pl_xstepscale / xfoc;
	const double ev_step = pl_planeheight * pl_ystepscale / xfoc;

	for (int x = pl->minx; x <= pl->maxx; x++)
	{
		const int yl = std::max<int>(pl->top[x], 0);
		const int yh = std::min<int>(pl->bottom[x], viewheight - 1);
		if (yl > yh)
			continue;

		// From absolute x, not accumulated, so two visplane splits of one surface
		// cannot drift apart.
		const double dx = x - centerx;

		dpcol.eu = eu_base + dx * eu_step;
		dpcol.ev = ev_base + dx * ev_step;
		dpcol.x = x;
		dpcol.yl = yl;
		dpcol.yh = yh;

		levelcolfunc();
	}
}


// Per-column range of four-row groups lying wholly inside the visplane, as
// group indices (screen row y0 = g * 4). Sized to the view, reused across
// planes; group_first > group_last means the column has no full group.
std::vector<int> group_first;
std::vector<int> group_last;

// First column of the run currently open at each group, or -1 for none. Indexed
// by group, so viewheight/4 entries.
std::vector<int> group_runstart;

//
// R_DrawLevelPlaneGroups
//
// The same surface as R_DrawLevelPlaneColumns, drawn four screen rows at a time
// marching in x instead of one column at a time marching in y. See
// drawplanegroup_t in common/r_draw.h for why that is the form that vectorizes.
//
// Two passes. The first records, per column, which groups lie wholly inside the
// plane, and hands the leftover rows at the top and bottom of each column to the
// existing scalar drawer -- at most three each, since a group is four rows. The
// second walks the groups, and within each one walks x emitting maximal runs of
// columns that cover all four of its rows.
//
void R_DrawLevelPlaneGroups(visplane_t* pl)
{
	int miny = viewheight, maxy = -1;

	for (int x = pl->minx; x <= pl->maxx; x++)
	{
		const int yl = std::max<int>(pl->top[x], 0);
		const int yh = std::min<int>(pl->bottom[x], viewheight - 1);
		if (yl > yh)
			continue;

		miny = std::min(miny, yl);
		maxy = std::max(maxy, yh);
	}

	if (maxy < miny)
		return;

	const shaderef_t light = R_BuildPlaneLighting(miny, maxy);

	dpcol.lightoff = plane_constlight ? NULL : plane_lightoff.data();
	dpcol.cbase = light.m_colormap;
	dpcol.sbase = light.m_shademap;

	dpcol.ubase = (pl_viewxtrans - 65536.0 * floor(pl_viewxtrans / 65536.0)) * 65536.0;
	dpcol.vbase = (pl_viewytrans - 65536.0 * floor(pl_viewytrans / 65536.0)) * 65536.0;

	const double eu_base = pl_planeheight * pl_viewcos * pl_xscale;
	const double ev_base = pl_planeheight * -pl_viewsin * pl_yscale;
	const double eu_step = pl_planeheight * pl_xstepscale / xfoc;
	const double ev_step = pl_planeheight * pl_ystepscale / xfoc;

	// Everything the kernel needs that is per-plane rather than per-group.
	dpgroup.source = dpcol.source;
	dpgroup.destination = dpcol.destination;
	dpgroup.colstep = dpcol.colstep;
	dpgroup.umask = dpcol.umask;
	dpgroup.vmask = dpcol.vmask;
	dpgroup.ushift = dpcol.ushift;
	dpgroup.vshift = dpcol.vshift;

	if (group_first.size() < static_cast<size_t>(viewwidth))
	{
		group_first.resize(viewwidth);
		group_last.resize(viewwidth);
	}

	int gmin = viewheight, gmax = -1;

	// ---- pass 1: group bounds per column, and the fringe rows scalar
	for (int x = pl->minx; x <= pl->maxx; x++)
	{
		const int yl = std::max<int>(pl->top[x], 0);
		const int yh = std::min<int>(pl->bottom[x], viewheight - 1);
		if (yl > yh)
		{
			// An empty interval, so the sweep below opens and closes nothing for
			// this column. first > last is the only property it relies on.
			group_first[x] = 0;
			group_last[x] = -1;
			continue;
		}

		const int g0 = (yl + 3) >> 2;			// first group wholly at or below yl
		const int g1 = ((yh + 1) >> 2) - 1;		// last group wholly at or above yh
		group_first[x] = g0;
		group_last[x] = g1;

		// From absolute x, not accumulated, matching R_DrawLevelPlaneColumns.
		const double dx = x - centerx;
		dpcol.eu = eu_base + dx * eu_step;
		dpcol.ev = ev_base + dx * ev_step;
		dpcol.x = x;

		if (g1 >= g0)
		{
			gmin = std::min(gmin, g0);
			gmax = std::max(gmax, g1);

			if (yl < (g0 << 2))
			{
				dpcol.yl = yl;
				dpcol.yh = (g0 << 2) - 1;
				levelcolfunc();
			}

			if (((g1 + 1) << 2) <= yh)
			{
				dpcol.yl = (g1 + 1) << 2;
				dpcol.yh = yh;
				levelcolfunc();
			}
		}
		else
		{
			// Shorter than a group straddling it, so all of it is fringe.
			dpcol.yl = yl;
			dpcol.yh = yh;
			levelcolfunc();
		}
	}

	if (gmax < gmin)
		return;

	// Fills dpgroup for one group's run and draws it. The per-lane constants are
	// built here rather than hoisted per group -- at 386 runs a frame that is free,
	// and it lets the sweep emit runs in x order instead of group order.
	const auto emit = [&](int g, int xa, int xb)
	{
		const int y0 = g << 2;
		dpgroup.y0 = y0;
		dpgroup.xa = xa;
		dpgroup.xb = xb;

		// Anchored with the scalar drawer's own expression, so the first column of
		// every run is bit-identical to it and the DDA cannot drift beyond one run.
		// The step is R_MapLevelPlane's ustep, which in 16.16 is exactly
		// eu_step * yslope[y]: per row, and independent of x. That is the trick.
		const double eu = eu_base + (xa - centerx) * eu_step;
		const double ev = ev_base + (xa - centerx) * ev_step;

		for (int i = 0; i < 4; i++)
		{
			const double Y = static_cast<double>(yslope[y0 + i]);

			dpgroup.ustep[i] = static_cast<dsfixed_t>(static_cast<int64_t>(eu_step * Y));
			dpgroup.vstep[i] = static_cast<dsfixed_t>(static_cast<int64_t>(ev_step * Y));
			dpgroup.ufrac[i] = static_cast<dsfixed_t>(static_cast<int64_t>(dpcol.ubase + eu * Y));
			dpgroup.vfrac[i] = static_cast<dsfixed_t>(static_cast<int64_t>(dpcol.vbase + ev * Y));

			// NULL whenever the plane resolved to one colormap, including
			// fixedlightlev and fixedcolormap -- there R_BuildPlaneLighting returns
			// early without filling the table and folds the offset into the
			// shaderef_t. Adding it again would double-apply it and read past the
			// shademap.
			const unsigned int off = dpcol.lightoff ? dpcol.lightoff[y0 + i] : 0u;
			dpgroup.shade[i] = dpcol.sbase + off;
			dpgroup.cmap[i] = dpcol.cbase + off;
		}

		R_DrawLevelGroup();
	};

	// ---- pass 2: one sweep in x, emitting each group's run as it closes
	//
	// Replaces a rescan of [minx, maxx] per group, which cost O(width x groups) to
	// emit O(covered pairs) of work -- measured at 0.47 scan steps per vectorized
	// pixel. Group intervals are contiguous, so only the groups at an interval's
	// ends can open or close between adjacent columns: the event count is bounded
	// by the boundary's vertical variation, not by width x height. R_MakeSpans,
	// rotated a quarter turn.
	const size_t needed = static_cast<size_t>(viewheight / 4) + 1;
	if (group_runstart.size() < needed)
		group_runstart.resize(needed);

	for (int g = gmin; g <= gmax; g++)
		group_runstart[g] = -1;

	// The previous column's interval, empty so the first column opens cleanly.
	int pg0 = 0, pg1 = -1;

	for (int x = pl->minx; x <= pl->maxx + 1; x++)
	{
		// One past the end is an empty column, so every open run closes.
		const int g0 = (x <= pl->maxx) ? group_first[x] : 0;
		const int g1 = (x <= pl->maxx) ? group_last[x] : -1;

		// Groups leaving the interval. The two ranges cannot overlap: for a
		// non-empty [g0, g1] we have g0 - 1 < g1 + 1, and for an empty one
		// the first range is itself empty.
		for (int g = pg0; g <= std::min(pg1, g0 - 1); g++)
			emit(g, group_runstart[g], x - 1);
		for (int g = std::max(pg0, g1 + 1); g <= pg1; g++)
			emit(g, group_runstart[g], x - 1);

		// Groups entering it, by the same decomposition.
		for (int g = g0; g <= std::min(g1, pg0 - 1); g++)
			group_runstart[g] = x;
		for (int g = std::max(g0, pg1 + 1); g <= g1; g++)
			group_runstart[g] = x;

		pg0 = g0;
		pg1 = g1;
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

	const double xscale = FIXED2DOUBLE(pl->xscale);
	const double yscale = FIXED2DOUBLE(pl->yscale);
	const double scaledflatwidth = flatwidth / (xscale != 0.0 ? xscale : 1.0);
	const double scaledflatheight = flatheight / (yscale != 0.0 ? yscale : 1.0);

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
		const angle_t rotation = 0u - pl->angle;
		const double sinang = sin((rotation + ANG90) * ANGLE_TO_RAD);
		const double cosang = cos((rotation + ANG90) * ANGLE_TO_RAD);

		if (map_format.getZDoom())
		{
			px = yoffs * cosang - xoffs * sinang;
			pz = xoffs * cosang + yoffs * sinang;
		}
		else
		{
			px = -xoffs;
			pz = yoffs;
		}
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
	float slopevis = 8.0 * slopetan * 16.0 * 320.0 / static_cast<float>(I_GetSurfaceWidth());

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
	pl_viewxtrans = pl_viewx * pl_xscale;
	pl_viewytrans = pl_viewy * pl_yscale;

	basecolormap = pl->colormap;	// [RH] set basecolormap

	// [SL] 2012-02-05 - Plane's height should be constant for all (x,y)
	// so just use (0, 0) when calculating the plane's z height
	pl_planeheight = FIXED2DOUBLE(abs(P_PlaneZ(0, 0, &pl->secplane) - viewz));

	const int light = std::clamp((pl->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight), 0, LIGHTLEVELS - 1);
	planezlight = zlight[light];

	// A NULL levelcolfunc forces spans -- r_drawflat and nodrawers -- which also
	// keeps R_StoreWallRange's `spanfunc == R_FillSpan` render-mode test correct.
	//
	// ARGB flats force spans too: no column drawer reads dpcol.argbsource, so such
	// a texture would be sampled through the palette instead of in true colour.
	// Those spans store one pixel per cache line, which makes a PNG flat on a
	// full-screen floor the known weak spot of the column-major layout.
	const bool columns = levelcolfunc != NULL && dpcol.argbsource == NULL;

	// R_DrawLevelGroup is NULL on an 8bpp surface, and the group path draws
	// opaque flats only -- a translucent portal boundary sets levelcolfunc to
	// the translucent column drawer, which has no group form yet.
	if (columns && R_DrawLevelGroup != NULL && levelcolfunc == R_DrawLevelColumn)
		R_DrawLevelPlaneGroups(pl);
	else if (columns)
		R_DrawLevelPlaneColumns(pl);
	else
		R_MakeSpans(pl, R_MapLevelPlane);
}


//
// R_DrawSingleFlatPlane
//
// Draws one visplane textured with its (regular, non-sky) flat, using
// whatever spanfunc is currently selected. Factored out of R_DrawPlanes so
// portal boundary flats can be re-drawn blended over portal content.
//
static void R_DrawSingleFlatPlane(visplane_t* pl)
{
	// regular flat
	dspan.color += 4;	// [RH] color if r_drawflat is 1

	const ResourceId res_id = Res_GetAnimatedTextureResourceId(pl->res_id);
	const Texture* cached = Res_CacheTexture(res_id, PU_STATIC);
	if (cached == NULL)
		return;

	// The span drawers below tile with power-of-two masks, which a wall texture
	// put on a plane need not have, so sample a resized copy of those instead.
	const Texture* texture = Res_PlaneTexture(res_id, cached);

	dspan.source = dpcol.source = texture->mData;
	// the 32bpp drawers sample the native ARGB plane when the
	// texture carries one (NULL otherwise)
	dspan.argbsource = dpcol.argbsource = texture->mARGBData;

	// [SL] Note that the texture orientation differs from typical Doom span
	// drawers since flats are stored in column major format now. The roles
	// of ufrac and vfrac have been reversed to accomodate this.
	dspan.umask = dpcol.umask = texture->mWidthMask << texture->mHeightBits;
	dspan.vmask = dpcol.vmask = texture->mHeightMask;
	dspan.ushift = dpcol.ushift = FRACBITS - texture->mHeightBits;
	dspan.vshift = dpcol.vshift = FRACBITS;

	// Warped flats are now handled elsewhere

	pl->top[pl->maxx+1] = viewheight;
	pl->top[pl->minx-1] = viewheight;

	if (P_IsPlaneLevel(&pl->secplane))
		R_DrawLevelPlane(pl);
	else
		R_DrawSlopedPlane(pl);

	Z_ChangeTag(cached, PU_CACHE);
}

//
// R_DrawStackFlatBlend
//
// Blends a stacked sector portal plane's own flat over the portal content
// just rendered into it, at the alpha given by the stack thing's arg0.
// Runs with the discovering pass's view restored, since the plane's texture
// mapping is in that view's space.
//
static void R_DrawStackFlatBlend(visplane_t* pl)
{
	if (pl->maxx < pl->minx || !R_IsStackPoint(pl->skybox))
		return;

	const int alpha = R_StackFlatAlpha(pl->skybox);
	if (alpha == 0)
		return;

	// A sky-pic'd portal plane has no flat to blend.
	if (R_ResourceIdIsSkyFlat(Res_GetAnimatedTextureResourceId(pl->res_id)) ||
	    (pl->sky_transfer & PL_SKYFLAT))
		return;

	dspan.translevel = dpcol.translevel = (alpha << FRACBITS) / 255;
	spanfunc = R_DrawTranslucentSpan;
	spanslopefunc = R_DrawTranslucentSlopeSpan;
	levelcolfunc = R_DrawTranslucentLevelColumn;

	R_DrawSingleFlatPlane(pl);

	R_ResetDrawFuncs();
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
			else if (R_IsStackBoundary(pl->skybox) && R_StackFlatAlpha(pl->skybox) > 0)
			{
				R_DrawStackFlatBlend(pl);
			}
			else
			{
				R_DrawSingleFlatPlane(pl);
			}
		}
	}
}

//==========================================================================
//
// R_DrawPortals
//
// Draws recursive skybox views and then frees them.
// Just note that these are NOT linked portals or even stacked sectors.
// The current iteration just handles recursive skyboxes, that's it.
//
// The process:
//   1. Move the camera to coincide with the SkyViewpoint.
//   2. Clear out the old planes. (They have already been drawn.)
//   3. Clear a window out of the ClipSegs just large enough for the plane.
//   4. Pretend the existing vissprites and drawsegs aren't there.
//   5. Create a drawseg at 0 distance to clip sprites to the visplane. It
//      doesn't need to be associated with a line in the map, since there
//      will never be any sprites in front of it.
//   6. Render the BSP, then planes, then portal visplanes recursively
//      (BSP, then planes).
//   7. The final portal level renders masked textures and sprites,
//      then each portal level above it will do the same.
//   8. The final level renders sprites and masked textures,
//      and frees the memory used for all portal visplanes.
//   9. If the recursion level exceeds r_portalrecursions, all portal planes
//      draw sky instead of rendering the portal view.
//
//==========================================================================

static void R_RenderPortalView(visplane_t* pl);

//
// R_DrawDiscoveredPortals
//
// Renders every portal plane queued in visplanes[MAXVISPLANES] by the pass
// (or main view) currently being rendered, then frees them.
//
static void R_DrawDiscoveredPortals()
{
	if (visplanes[MAXVISPLANES] == NULL)
		return;

	// Detach the queue, passes below accumulate their own portal discoveries.
	visplane_t* queue = visplanes[MAXVISPLANES];
	visplanes[MAXVISPLANES] = NULL;

	visplane_t* pl;

	r_PortalDepth++;

	if (r_PortalDepth <= r_portalrecursions.asInt())
	{
		for (pl = queue; pl != NULL; pl = pl->next)
		{
			R_RenderPortalView(pl);

			// The discovering view is restored now, overlay the boundary
			// flat translucently if the stack thing asks for it.
			R_DrawStackFlatBlend(pl);
		}
	}
	else
	{
		// Shouldn't be reachable (creation is suppressed at the limit), but
		// never leave portal planes undrawn: render them as regular sky.
		for (pl = queue; pl != NULL; pl = pl->next)
		{
			if (pl->maxx >= pl->minx)
				R_RenderSkyRange(pl);
		}
	}

	r_PortalDepth--;

	// Free the processed planes.
	for (*freehead = queue; *freehead;)
		freehead = &(*freehead)->next;
}

//
// R_RenderPortalView
//
// Renders a single portal pass into the window described by the visplane,
// saving and restoring the view state around it so passes can nest.
//
static void R_RenderPortalView(visplane_t* pl)
{
	if (pl->maxx < pl->minx)
		return;

	fixed_t savedx = viewx;
	fixed_t savedy = viewy;
	fixed_t savedz = viewz;
	angle_t savedangle = viewangle;
	ptrdiff_t savedvissprite_p = vissprite_p - vissprites;
	ptrdiff_t savedfirstvissprite = firstvissprite - vissprites;
	ptrdiff_t savedds_p = ds_p - drawsegs;
	ptrdiff_t savedfirstdrawseg = firstdrawseg - drawsegs;
	AActor* savedcamera = camera;
	bool pushedstackportal = false;

	int i;

	AActor* sky = pl->skybox;

	if (R_IsStackPoint(sky))
	{
		AActor* mate = sky->tracer;

		if (mate == NULL)
			return; // mate was destroyed at runtime, skip the pass

		viewx = savedx + sky->x - mate->x;
		viewy = savedy + sky->y - mate->y;
		viewz = savedz;
		camera = sky;
		r_ActiveStackPortals.push_back(sky);
		pushedstackportal = true;
	}
	else
	{
		viewx = sky->x;
		viewy = sky->y;
		viewz = sky->z;
		camera = sky;
		R_SetViewAngle(savedangle + sky->angle);
	}
	validcount++; // Make sure we see all sprites

	R_ClearPlanes(false);
	R_ClearClipSegs();

	// Set up ceiling/floor clip arrays for this visplane.
	for (i = pl->minx; i <= pl->maxx; i++)
	{
		if (std::cmp_equal(pl->top[i], viewheight))
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
	R_DrawDiscoveredPortals();
	R_DrawMasked();

	firstvissprite = vissprites + savedfirstvissprite;
	vissprite_p = vissprites + savedvissprite_p;
	firstdrawseg = drawsegs + savedfirstdrawseg;
	ds_p = drawsegs + savedds_p;

	camera = savedcamera;

	if (pushedstackportal)
		r_ActiveStackPortals.pop_back();
	viewx = savedx;
	viewy = savedy;
	viewz = savedz;
	R_SetViewAngle(savedangle);
}

void R_DrawPortals()
{
	if (visplanes[MAXVISPLANES] == NULL)
		return;

	// A render aborted mid-pass can leave entries behind.
	r_ActiveStackPortals.clear();

	// Don't let gun flashes brighten portal views
	int savedextralight = extralight;
	extralight = 0;

	R_DrawDiscoveredPortals();

	extralight = savedextralight;
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

	// Free all visplanes and let them be re-allocated as needed. Headers are
	// owned by their block and spans by their own allocation, so every chain
	// is dropped wholesale rather than walked.
	for (unsigned int* spans : visplane_spans)
		M_Free(spans);
	visplane_spans.clear();

	for (visplane_t* block : visplane_blocks)
		M_Free(block);
	visplane_blocks.clear();
	visplane_block_used = VISPLANE_BLOCK;

	freetail = NULL;
	freehead = &freetail;

	// includes the portal bucket at MAXVISPLANES, whose planes would otherwise
	// be left pointing at freed blocks
	for (int i = 0; i <= MAXVISPLANES; i++)
		visplanes[i] = NULL;

	return true;
}

VERSION_CONTROL (r_plane_cpp, "$Id$")
