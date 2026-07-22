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
//		All the clipping: columns, horizontal spans, sky columns.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <math.h>

#include "m_mempool.h"

#include "i_system.h"


#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_video.h"

#include "m_vectors.h"

#include "p_mapformat.h"

#include "p_lnspec.h"

#include "r_sky.h"
#include "resources/res_texture.h"

// a pool of bytes allocated for sprite clipping arrays
Pool<const palindex_t*> masked_midposts_pool(4096);
Pool<int> sprclip_pool(4096);
Pool<fixed_t> midscales_pool(4096);

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

static bool		segtextured;	// True if any of the segs textures might be visible.
static bool		markfloor;		// False if the back side is the same plane.
static bool		markceiling;
static bool		didsolidcol;


static const Texture*	toptexture;
static const Texture*	bottomtexture;
static const Texture*	midtexture;
static const Texture*	maskedtexture;

int*			walllights;

//
// regular wall
//
fixed_t			rw_light;		// [RH] Use different scaling for lights
fixed_t			rw_lightstep;

static fixed_t	rw_scale;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;

extern fixed_t	rw_frontcz1, rw_frontcz2;
extern fixed_t	rw_frontfz1, rw_frontfz2;
extern fixed_t	rw_backcz1, rw_backcz2;
extern fixed_t	rw_backfz1, rw_backfz2;
static bool		rw_hashigh, rw_haslow;

static int walltopf[MAXWIDTH];
static int walltopb[MAXWIDTH];
static int wallbottomf[MAXWIDTH];
static int wallbottomb[MAXWIDTH];

static const palindex_t* topposts[MAXWIDTH];
static const palindex_t* midposts[MAXWIDTH];
static const palindex_t* bottomposts[MAXWIDTH];

static const palindex_t** masked_midposts;

// y-scale of the texture tier currently being drawn by the solid column blaster
static fixed_t wallscaley = FRACUNIT;
static fixed_t wallscalex[MAXWIDTH];
static int texoffs[MAXWIDTH];

// per-column scale and wall-parameter values computed by R_PrepWall
static double wallscaled[MAXWIDTH];
static double wallufrac[MAXWIDTH];

extern fixed_t FocalLengthY;
extern float xfoc, yfoc;

static const fixed_t* masked_midscales;

EXTERN_CVAR(r_clipmaskedspecial)

//
// R_OrthogonalLightnumAdjustment
//
int R_OrthogonalLightnumAdjustment()
{
	// [RH] Only do it if not foggy and allowed
    if (!foggy && !(level.flags & LEVEL_EVENLIGHTING))
	{
		if (curline->linedef->slopetype == ST_HORIZONTAL)
			return -1;
		else if (curline->linedef->slopetype == ST_VERTICAL)
			return 1;
	}

	return 0;	// no adjustment for diagonal lines
}


//
// R_FillWallHeightArray
//
// Calculates the wall-texture screen coordinates for a span of columns.
//
static void R_FillWallHeightArray(
	int *array,
	int start, int stop,
	fixed_t val1, fixed_t val2)
{
	if (start > stop)
		return;

	const double z1 = FIXED2DOUBLE(val1 - viewz);
	const double z2 = FIXED2DOUBLE(val2 - viewz);

	const double horizon = FIXED2DOUBLE(centeryfrac);

	for (int i = start; i <= stop; i++)
	{
		const double z = z1 + (z2 - z1) * wallufrac[i];
		const double frac = horizon - z * wallscaled[i];
		array[i] = clamp(static_cast<int>(frac), ceilingclipinitial[0], floorclipinitial[0]);
	}
}


//
// R_BlastMaskedSegColumn
//
static inline void R_BlastMaskedSegColumn(void (*drawfunc)())
{
	// R_PrepWall uses floats to calculate scale1 and scale2, which left
	// the scalestep values vulnerable to floating-point rounding errors.
	// If a wall is tall enough and a resolution big enough, the scalestep
	// can be off enough that by accumulation, it draws a row with no data.
	// Your midtex gap! :)
	spryscale = masked_midscales[dcol.x];

	if (dcol.source == NULL || spryscale <= 0)
		return;

	dcol.iscale = 0xffffffffu / static_cast<unsigned>(spryscale);

	// R_FillWallHeightArray uses centeryfrac and so should we.
	// Otherwise we can have textures drawing at different
	// heights when mouselook is on.

	// calculate unclipped screen coordinates for the whole dense column
	const int64_t topscreen =
	    static_cast<int64_t>(centeryfrac) - ((static_cast<int64_t>(dcol.texturemid) * spryscale) >> FRACBITS);
	const int64_t bottomscreen =
	    topscreen + ((static_cast<int64_t>(spryscale) * dcol.textureheight) >> FRACBITS);

	int64_t yl = (topscreen - 1) >> FRACBITS;
	int64_t yh = (bottomscreen - 1) >> FRACBITS;

	// iscale is already in the texture's scaled space (spryscale was
	// divided by the y-scale), so this tracks y-scaling automatically.
	int64_t texturefrac = 0;
	if (mceilingclip[dcol.x] + 1 > yl)
		texturefrac = (mceilingclip[dcol.x] + 1 - yl) * dcol.iscale;

	yl = MAX<int64_t>(yl, MAX(mceilingclip[dcol.x], 0));
	yh = MIN<int64_t>(yh, mfloorclip[dcol.x] - 1);

	if (yl > yh || texturefrac >= dcol.textureheight)
		return;

	// clamp the texture coordinates so out-of-range rows are not drawn
	const int64_t endfrac = texturefrac + (yh - yl) * dcol.iscale;
	const int64_t maxfrac = dcol.textureheight;

	if (endfrac >= maxfrac)
	{
		const int64_t cnt = (endfrac - maxfrac + dcol.iscale) / dcol.iscale;
		yh -= cnt;
	}

	if (yl >= 0 && yh < viewheight && yl <= yh)
	{
		dcol.yl = static_cast<int>(yl);
		dcol.yh = static_cast<int>(yh);
		dcol.texturefrac = static_cast<fixed_t>(texturefrac);
		drawfunc();
	}
}


//
// R_BlastSolidSegColumn
//
static inline void R_BlastSolidSegColumn(void (*drawfunc)())
{
	fixed_t scale = wallscalex[dcol.x];
	if (scale <= 0)
		return;

	// TODO: move iscale calculation outside this function
	dcol.iscale = FixedMul(0xffffffffu / static_cast<unsigned>(scale), wallscaley);
	dcol.texturefrac = dcol.texturemid +
	                   FixedMul(((dcol.yl + 1) << FRACBITS) - centeryfrac, dcol.iscale);

	if (dcol.yl <= dcol.yh)
		drawfunc();
}

inline void SolidColumnBlaster()
{
	R_BlastSolidSegColumn(colfunc);
}

inline void MaskedColumnBlaster()
{
	R_BlastMaskedSegColumn(colfunc);
}

inline void R_ColumnSetup(int x, const int* top, const int* bottom, const palindex_t** posts, bool calc_light)
{
	if (calc_light)
	{
		const int index = clamp(rw_light >> LIGHTSCALESHIFT, 0, MAXLIGHTSCALE - 1);
		dcol.colormap = basecolormap.with(walllights[index]);
	}

	dcol.yl = MAX(top[x], 0);
	dcol.yh = MIN(bottom[x], viewheight - 1);
	dcol.source = posts[x];
}


static inline int R_ColumnRangeMinimumHeight(int start, int stop, const int* top)
{
	int minheight = viewheight - 1;
	for (int x = start; x <= stop; x++)
		minheight = MIN(minheight, top[x]);

	return MAX(minheight, 0);
}

static inline int R_ColumnRangeMaximumHeight(int start, int stop, const int* bottom)
{
	int maxheight = 0;
	for (int x = start; x <= stop; x++)
		maxheight = MAX(maxheight, bottom[x]);

	return MIN(maxheight, viewheight - 1);
}


//
// R_RenderColumnRange
//
//
void R_RenderColumnRange(int start, int stop, const int* top, const int* bottom,
		const palindex_t** posts, void (*colblast)(), bool calc_light, int columnmethod)
{
	if (start > stop)
		return;

	if (calc_light)
	{
		if (fixedlightlev)
		{
			dcol.colormap = basecolormap.with(fixedlightlev);
			calc_light = false;
		}
		else if (fixedcolormap.isValid())
		{
			dcol.colormap = fixedcolormap;
			calc_light = false;
		}
		else
		{
			if (!walllights)
				walllights = scalelight[0];
		}
	}

	if (columnmethod == 0)
	{
		for (int x = start; x <= stop; x++)
		{
			if (calc_light)
			{
				int light_index = clamp(rw_light >> LIGHTSCALESHIFT, 0, MAXLIGHTSCALE - 1);
				dcol.colormap = basecolormap.with(walllights[light_index]);
				rw_light += rw_lightstep;
			}

			dcol.x = x;
			dcol.yl = MAX(0, top[x]);
			dcol.yh = MIN(viewheight -1, bottom[x]);
			dcol.source = posts[x];
			colblast();
		}
	}
	else if (columnmethod == 2)
	{
		// [SL] Render the range of columns in 64x64 pixel blocks, aligned to a grid
		// on the screen. This is to make better use of spatial locality in the cache.
		#define BLOCKBITS 6
		#define BLOCKSIZE (1 << BLOCKBITS)
		#define BLOCKMASK (BLOCKSIZE - 1)

		// pre-calculate the color map number for lighting for each screen column
		static int light_lookup[MAXWIDTH];
		if (calc_light)
		{
			for (int x = start; x <= stop; x++)
			{
				int index = clamp(rw_light >> LIGHTSCALESHIFT, 0, MAXLIGHTSCALE - 1);
				light_lookup[x] = walllights[index];
				rw_light += rw_lightstep;
			}
		}

		for (int bx = start; bx <= stop; bx = (bx & ~BLOCKMASK) + BLOCKSIZE)
		{
			const int blockstartx = bx;
			const int blockstopx = MIN((bx & ~BLOCKMASK) + BLOCKSIZE - 1, stop);

			const int miny = R_ColumnRangeMinimumHeight(blockstartx, blockstopx, top);
			const int maxy = R_ColumnRangeMaximumHeight(blockstartx, blockstopx, bottom);

			for (int by = miny; by <= maxy; by = (by & ~BLOCKMASK) + BLOCKSIZE)
			{
				const int blockstarty = by;
				const int blockstopy = MIN((by & ~BLOCKMASK) + BLOCKSIZE - 1, viewheight - 1);

				for (int x = blockstartx; x <= blockstopx; x++)
				{
					if (calc_light)
						dcol.colormap = basecolormap.with(light_lookup[x]);

					dcol.x = x;
					dcol.yl = MAX(top[x], blockstarty);
					dcol.yh = MIN(bottom[x], blockstopy);
					dcol.source = posts[x];
					colblast();
				}
			}
		}
	}
}

//
// R_RenderSolidSegRange
//
// Clips each of the three possible seg tiers of the column (top, mid, and bottom),
// sets the appropriate drawcolumn variables and calls R_RenderColumnRange for each
// tier to render the range of columns.
//
// The clipping of the seg tiers also vertically clips the ceiling and floor
// planes.
//
void R_RenderSolidSegRange(int start, int stop)
{
	static int lower[MAXWIDTH];
	const int count = stop - start + 1;
	const int initial_light = rw_light;

	if (start > stop)
		return;

	// render solid seg tiers in 64x64 screen-space blocks for cache locality
	static constexpr int columnmethod = 2;

	dcol.masked = false;

	// clip the front of the walls to the ceiling and floor
	for (int x = start; x <= stop; x++)
	{
		walltopf[x] = MAX(walltopf[x], ceilingclip[x]);
		wallbottomf[x] = MIN(wallbottomf[x], floorclip[x]);
	}

	// mark ceiling-plane areas
	if (markceiling)
	{
		for (int x = start; x <= stop; x++)
		{
			const int top = MAX(ceilingclip[x], 0);
			const int bottom = MIN(MIN(walltopf[x], floorclip[x]) - 1, viewheight - 1);

			if (top <= bottom)
			{
				ceilingplane->top[x] = top;
				ceilingplane->bottom[x] = bottom;
			}
		}
	}

	// mark floor-plane areas
	if (markfloor)
	{
		for (int x = start; x <= stop; x++)
		{
			const int top = MAX(MAX(wallbottomf[x], ceilingclip[x]), 0);
			const int bottom = MIN(floorclip[x] - 1, viewheight - 1);

			if (top <= bottom)
			{
				floorplane->top[x] = top;
				floorplane->bottom[x] = bottom;
			}
		}
	}

	if (midtexture)		// 1-sided line
	{
		// draw the middle wall tier
		for (int x = start; x <= stop; x++)
			lower[x] = wallbottomf[x] - 1;

		rw_light = initial_light;

		wallscaley = midtexture->mScaleY;
		dcol.textureheight = midtexture->mHeight << FRACBITS;
		dcol.texturemid = FixedMul(rw_midtexturemid, wallscaley) + curline->sidedef->rowoffset;
		dcol.texturedata = midtexture->mData;
		dcol.argbtexturedata = midtexture->mARGBData;

		R_RenderColumnRange(start, stop, walltopf, lower, midposts, SolidColumnBlaster, true, columnmethod);

		// indicate that no further drawing can be done in this column
		memcpy(&ceilingclip[start], &floorclipinitial[start], count * sizeof(ceilingclip[0]));
		memcpy(&floorclip[start], &ceilingclipinitial[start], count * sizeof(floorclip[0]));
	}
	else			// 2-sided line
	{
		if (toptexture)
		{
			// draw the upper wall tier
			rw_light = initial_light;

			for (int x = start; x <= stop; x++)
			{
				walltopb[x] = MAX(MIN(walltopb[x], floorclip[x]), walltopf[x]);
				lower[x] = walltopb[x] - 1;
			}

			wallscaley = toptexture->mScaleY;
			dcol.textureheight = toptexture->mHeight << FRACBITS;
			dcol.texturemid = FixedMul(rw_toptexturemid, wallscaley) + curline->sidedef->rowoffset;
			dcol.texturedata = toptexture->mData;
			dcol.argbtexturedata = toptexture->mARGBData;

			R_RenderColumnRange(start, stop, walltopf, lower, topposts, SolidColumnBlaster, true, columnmethod);

			memcpy(&ceilingclip[start], walltopb + start, count * sizeof(ceilingclip[0]));
		}
		else if (markceiling)
		{
			// no upper wall
			memcpy(&ceilingclip[start], walltopf + start, count * sizeof(ceilingclip[0]));
		}

		if (bottomtexture)
		{
			// draw the lower wall tier
			rw_light = initial_light;

			for (int x = start; x <= stop; x++)
			{
				wallbottomb[x] = MIN(MAX(wallbottomb[x], ceilingclip[x]), wallbottomf[x]);
				lower[x] = wallbottomf[x] - 1;
			}

			wallscaley = bottomtexture->mScaleY;
			dcol.textureheight = bottomtexture->mHeight << FRACBITS;
			dcol.texturemid = FixedMul(rw_bottomtexturemid, wallscaley) + curline->sidedef->rowoffset;
			dcol.texturedata = bottomtexture->mData;
			dcol.argbtexturedata = bottomtexture->mARGBData;

			R_RenderColumnRange(start, stop, wallbottomb, lower, bottomposts, SolidColumnBlaster, true, columnmethod);

			memcpy(&floorclip[start], wallbottomb + start, count * sizeof(floorclip[0]));
		}
		else if (markfloor)
		{
			// no lower wall
			memcpy(&floorclip[start], wallbottomf + start, count * sizeof(floorclip[0]));
		}

		if (maskedtexture)
		{
			// save texturecol for backdrawing of masked mid texture
			for (int x = start; x <= stop; x++)
			{
				int colnum = maskedtexture->wrapColumn(FixedMul(texoffs[x], maskedtexture->mScaleX) >> FRACBITS);
				masked_midposts[x] = maskedtexture->getColumn(colnum);
			}
		}
	}

	for (int x = start; x <= stop; x++)
	{
		// cph - if we completely blocked further sight through this column,
		// add this info to the solid columns array
		if ((markceiling || markfloor) && (floorclip[x] <= ceilingclip[x]))
		{
			solidcol[x] = 1;
			didsolidcol = true;
		}
	}
}


//
// R_RenderMaskedSegRange
//
// Renders a masked seg
//
void R_RenderMaskedSegRange(drawseg_t* ds, int x1, int x2)
{
	sector_t	tempsec;		// killough 4/13/98

	dcol.color = (dcol.color + 4) & 0xFF;	// color if using r_drawflat
	dcol.masked = true;

	// Calculate light table.
	// Use different light tables
	//	 for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	curline = ds->curline;

	// killough 4/11/98: draw translucent 2s normal textures
	// [RH] modified because we don't use user-definable
	//		translucency maps
	if (curline->linedef->lucency < 240)
	{
		R_SetLucentDrawFuncs();
		dcol.translevel = curline->linedef->lucency << 8;
	}
	else
	{
		R_ResetDrawFuncs();
	}

	frontsector = curline->frontsector;
	backsector = curline->backsector;

	const Texture* texture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(curline->sidedef->midtexture));
	fixed_t texheight = FixedMul(texture->mHeight << FRACBITS, texture->mScaleY);

	// find texture positioning
	if (curline->linedef->flags & ML_DONTPEGBOTTOM)
		// offset by the world-space height of one tile (texel height / y-scale)
		dcol.texturemid = MAX(P_FloorHeight(frontsector), P_FloorHeight(backsector)) +
		                  FixedDiv(texture->mHeight << FRACBITS, texture->mScaleY);
	else
		dcol.texturemid = MIN(P_CeilingHeight(frontsector), P_CeilingHeight(backsector));

	dcol.texturemid = FixedMul(dcol.texturemid - viewz, texture->mScaleY) +
	                  curline->sidedef->rowoffset;
	
	int64_t topscreenclip = static_cast<int64_t>(centeryfrac) << FRACBITS;
	int64_t botscreenclip = static_cast<int64_t>(centeryfrac - (viewheight << FRACBITS)) << FRACBITS;
 
	// top of texture entirely below screen?
	if (static_cast<int64_t>(dcol.texturemid) * ds->scale1 <= botscreenclip &&
		static_cast<int64_t>(dcol.texturemid) * ds->scale2 <= botscreenclip)
		return;

	// bottom of texture entirely above screen?
	if (static_cast<int64_t>(dcol.texturemid - texheight) * ds->scale1 > topscreenclip &&
		static_cast<int64_t>(dcol.texturemid - texheight) * ds->scale2 > topscreenclip)
		return;

	basecolormap = frontsector->colormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	int lightnum = (R_FakeFlat(frontsector, &tempsec, NULL, NULL, false)->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);
	lightnum += R_OrthogonalLightnumAdjustment();

	walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
		lightnum <  0 ? scalelight[0] : scalelight[lightnum];

	masked_midposts = ds->midposts;
	masked_midscales = ds->midscales;

	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	dcol.textureheight = texture->mHeight << FRACBITS;
	dcol.texturedata = texture->mData;
	dcol.argbtexturedata = texture->mARGBData;

	// [SL] pre-calculate scaling for each column
	if (masked_midscales)
	{
		memcpy(wallscalex + x1, masked_midscales + x1, (x2 - x1 + 1) * sizeof(*wallscalex));
	}
	else
	{
		rw_scalestep = FixedDiv(ds->scalestep, texture->mScaleY);
		fixed_t scale = FixedDiv(ds->scale1, texture->mScaleY) + (x1 - ds->x1) * rw_scalestep;
		for (int x = x1; x <= x2; x++)
		{
			wallscalex[x] = scale;
			scale += rw_scalestep;
		}
	}

	// draw the columns
	R_RenderColumnRange(x1, x2, negonearray, viewheightarray, ds->midposts, MaskedColumnBlaster, true, 0);

	// Mark these columns as having been drawn by setting the midpost ptr to NULL for each column
	memset(ds->midposts + x1, 0, (x2 - x1 + 1) * sizeof(ds->midposts));
}


static constexpr fixed_t R_LineLength(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2)
{
	const float dx = FIXED2FLOAT(px2 - px1);
	const float dy = FIXED2FLOAT(py2 - py1);

	return FLOAT2FIXED(sqrt(dx*dx + dy*dy));
}


//
// R_PrepWall
//
// Prepares a lineseg for rendering. It fills the walltopf, wallbottomf,
// walltopb, and wallbottomb arrays with the top and bottom pixel heights
// of the wall for the span from start to stop.
//
// It also fills in the wallscalex and texoffs arrays with the vertical
// scaling for each column and the horizontal texture offset for each column
// respectively.
//
void R_PrepWall(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2,
                fixed_t tx1, fixed_t ty1, fixed_t tx2, fixed_t ty2, int start, int stop)
{
	const int width = stop - start + 1;
	if (width <= 0)
		return;

	// Calculate distance from lineseg start to start of clipped lineseg
	vertex_t *v1;			// determine which vertex of the linedef should be used for texture alignment
	if (curline->linedef->sidenum[0] == curline->sidedef - sides)
		v1 = curline->linedef->v1;
	else
		v1 = curline->linedef->v2;
	fixed_t segoffs = R_LineLength(v1->x, v1->y, px1, py1) + curline->sidedef->textureoffset;

	// clipped lineseg endpoints in camera space
	const double cx1 = FIXED2DOUBLE(tx1), cy1 = FIXED2DOUBLE(ty1);
	const double cx2 = FIXED2DOUBLE(tx2), cy2 = FIXED2DOUBLE(ty2);
	const double wdx = cx2 - cx1, wdy = cy2 - cy1;

	// camera space is a rigid rotation of world space, so this is also the
	// world-space length of the clipped seg
	const double seglen = sqrt(wdx * wdx + wdy * wdy);
	const double invseglen = seglen > 0.0 ? 1.0 / seglen : 0.0;

	// constant of the wall's line equation: cross(P, W) = cross(P1, W)
	const double wallconst = cx1 * wdy - cy1 * wdx;

	const double mindepth = FIXED2DOUBLE(NEARCLIP);
	const double maxdepth = 16384.0;

	for (int i = start; i <= stop; i++)
	{
		// view ray through the center of screen column i
		const double raydx = (i + 0.5 - centerx) / xfoc;
		const double den = raydx * wdy - wdx;
		double depth = den != 0.0 ? wallconst / den : maxdepth;
		depth = clamp(depth, mindepth, maxdepth);

		const double scale = yfoc / depth;
		wallscaled[i] = scale;
		wallscalex[i] = DOUBLE2FIXED(scale);

		const double uunits = ((depth * raydx - cx1) * wdx + (depth - cy1) * wdy) * invseglen;
		wallufrac[i] = uunits * invseglen;
		texoffs[i] = segoffs +
		             static_cast<fixed_t>(static_cast<int64_t>(uunits * 65536.0));
	}

	rw_scalestep = FLOAT2FIXED((wallscaled[stop] - wallscaled[start]) / width);

	// get the z coordinates of the line's vertices on each side of the line
	rw_frontcz1 = P_CeilingHeight(px1, py1, frontsector);
	rw_frontfz1 = P_FloorHeight(px1, py1, frontsector);
	rw_frontcz2 = P_CeilingHeight(px2, py2, frontsector);
	rw_frontfz2 = P_FloorHeight(px2, py2, frontsector);

	// calculate the upper and lower heights of the walls in the front
	R_FillWallHeightArray(walltopf, start, stop, rw_frontcz1, rw_frontcz2);
	R_FillWallHeightArray(wallbottomf, start, stop, rw_frontfz1, rw_frontfz2);

	rw_hashigh = rw_haslow = false;

	if (backsector)
	{
		rw_backcz1 = P_CeilingHeight(px1, py1, backsector);
		rw_backfz1 = P_FloorHeight(px1, py1, backsector);
		rw_backcz2 = P_CeilingHeight(px2, py2, backsector);
		rw_backfz2 = P_FloorHeight(px2, py2, backsector);

		// calculate the upper and lower heights of the walls in the back
		R_FillWallHeightArray(walltopb, start, stop, rw_backcz1, rw_backcz2);
		R_FillWallHeightArray(wallbottomb, start, stop, rw_backfz1, rw_backfz2);

		static constexpr fixed_t tolerance = FRACUNIT / 2;

		// determine if an upper texture is showing
		rw_hashigh	= (P_CeilingHeight(curline->v1->x, curline->v1->y, frontsector) - tolerance >
					   P_CeilingHeight(curline->v1->x, curline->v1->y, backsector)) ||
					  (P_CeilingHeight(curline->v2->x, curline->v2->y, frontsector) - tolerance>
					   P_CeilingHeight(curline->v2->x, curline->v2->y, backsector));

		// determine if a lower texture is showing
		rw_haslow	= (P_FloorHeight(curline->v1->x, curline->v1->y, frontsector) + tolerance <
					   P_FloorHeight(curline->v1->x, curline->v1->y, backsector)) ||
					  (P_FloorHeight(curline->v2->x, curline->v2->y, frontsector) + tolerance <
					   P_FloorHeight(curline->v2->x, curline->v2->y, backsector));

		// hack to allow height changes in outdoor areas (sky hack)
		// copy back ceiling height array to front ceiling height array
		if (R_ResourceIdIsSkyFlat(frontsector->ceiling_res_id) &&
			R_ResourceIdIsSkyFlat(backsector->ceiling_res_id))
			memcpy(walltopf+start, walltopb+start, width*sizeof(*walltopb));
	}

	// Cache the wall textures
	toptexture = midtexture = bottomtexture = maskedtexture = NULL;

	if (!backsector)
		midtexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(curline->sidedef->midtexture));

	if (rw_hashigh)
		toptexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(curline->sidedef->toptexture));

	if (rw_haslow)
		bottomtexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(curline->sidedef->bottomtexture));

	// determine which texture posts will be used for each screen
	// column in this range.
	for (int i = start; i <= stop; i++)
	{
		const fixed_t colfrac = texoffs[i];

		if (toptexture)
		{
			int colnum = toptexture->wrapColumn(FixedMul(colfrac, toptexture->mScaleX) >> FRACBITS);
			topposts[i] = toptexture->getColumn(colnum);
		}
		if (midtexture)
		{
			int colnum = midtexture->wrapColumn(FixedMul(colfrac, midtexture->mScaleX) >> FRACBITS);
			midposts[i] = midtexture->getColumn(colnum);
		}
		if (bottomtexture)
		{
			int colnum = bottomtexture->wrapColumn(FixedMul(colfrac, bottomtexture->mScaleX) >> FRACBITS);
			bottomposts[i] = bottomtexture->getColumn(colnum);
		}
	}
}


//
// R_StoreWallRange
// A wall segment will be drawn
//	between start and stop pixels (inclusive).
//
void R_StoreWallRange(int start, int stop)
{
#ifdef RANGECHECK
	if (start >= viewwidth || start > stop)
		I_FatalError("Bad R_StoreWallRange: {} to {}", start , stop);
#endif

	const int count = stop - start + 1;
	if (count <= 0)
		return;

	R_ReallocDrawSegs();	// don't overflow and crash

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// mark the segment as visible for auto map
	linedef->flags |= ML_MAPPED;

	ds_p->x1 = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;

	// calculate scale at both ends and step
	ds_p->scale1 = rw_scale = wallscalex[start];
	ds_p->scale2 = wallscalex[stop];
	ds_p->scalestep = rw_scalestep;

	ds_p->light = rw_light = rw_scale * lightscalexmul;
 	ds_p->lightstep = rw_lightstep = rw_scalestep * lightscalexmul;

	// calculate texture boundaries
	//	and decide if floor / ceiling marks are needed
	maskedtexture = NULL;
	ds_p->midposts = NULL;
	ds_p->midscales = NULL;

	if (!backsector)
	{
		// single sided line

		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = true;

		if (linedef->flags & ML_DONTPEGBOTTOM)
		{
			// bottom of texture at bottom
			if (midtexture)
			{
				// world-space height of one tile: texel height divided by y-scale
				fixed_t texheight = FixedDiv(midtexture->mHeight << FRACBITS, midtexture->mScaleY);
				rw_midtexturemid = P_FloorHeight(frontsector) - viewz + texheight;
			}
		}
		else
		{
			// top of texture at top
			const fixed_t fc = P_CeilingHeight(frontsector);
			rw_midtexturemid = fc - viewz;
		}

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = viewheightarray;
		ds_p->sprbottomclip = negonearray;
	}
	else
	{
		// two sided line
		ds_p->sprtopclip = ds_p->sprbottomclip = NULL;
		ds_p->silhouette = 0;

		extern bool doorclosed;
		if (doorclosed)
		{
			// clip all sprites behind this closed door (or otherwise solid line)
			ds_p->silhouette = SIL_BOTH;
			ds_p->sprtopclip = viewheightarray;
			ds_p->sprbottomclip = negonearray;
		}
		else
		{
			// determine sprite clipping for non-solid line segs
			if (rw_frontfz1 > rw_backfz1 || rw_frontfz2 > rw_backfz2 ||
				rw_backfz1 > viewz || rw_backfz2 > viewz ||
				!P_IsPlaneLevel(&backsector->floorplane))	// backside sloping?
				ds_p->silhouette |= SIL_BOTTOM;

			if (rw_frontcz1 < rw_backcz1 || rw_frontcz2 < rw_backcz2 ||
				rw_backcz1 < viewz || rw_backcz2 < viewz ||
				!P_IsPlaneLevel(&backsector->ceilingplane))	// backside sloping?
				ds_p->silhouette |= SIL_TOP;
		}

		if (doorclosed)
		{
			markceiling = markfloor = true;
		}
		else if (spanfunc == R_FillSpan)
		{
			markfloor = markceiling = (frontsector != backsector);
		}
		else
		{
			markfloor =
				  !P_IdenticalPlanes(&backsector->floorplane, &frontsector->floorplane)
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->floor_res_id != frontsector->floor_res_id

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->floor_xoffs != frontsector->floor_xoffs
				|| (backsector->floor_yoffs + backsector->base_floor_yoffs) !=
				   (frontsector->floor_yoffs + frontsector->base_floor_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through deep water
				|| frontsector->heightsec

				// killough 4/17/98: draw floors if different light levels
				|| backsector->floorlightsec != frontsector->floorlightsec

				// [EB] check for special too for DSDA-compatibility on MBF21
				|| (r_clipmaskedspecial && backsector->special != frontsector->special)

				// [RH] Add checks for colormaps
				|| backsector->colormap != frontsector->colormap

				|| backsector->floor_xscale != frontsector->floor_xscale
				|| backsector->floor_yscale != frontsector->floor_yscale

				|| (backsector->floor_angle + backsector->base_floor_angle) !=
				   (frontsector->floor_angle + frontsector->base_floor_angle)
				;

			// Sky hack
			// MBF sky transfers split the visplane in 2, so in order for sky
			// transfer skyhack to work, we need to identify both sectors' sky
			const bool ceilingskyhack =
				!R_ResourceIdIsSkyFlat(frontsector->ceiling_res_id) || !R_ResourceIdIsSkyFlat(backsector->ceiling_res_id);

			markceiling =
				  (ceilingskyhack &&
				   !P_IdenticalPlanes(&backsector->ceilingplane, &frontsector->ceilingplane))
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->ceiling_res_id != frontsector->ceiling_res_id

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->ceiling_xoffs != frontsector->ceiling_xoffs
				|| (backsector->ceiling_yoffs + backsector->base_ceiling_yoffs) !=
				   (frontsector->ceiling_yoffs + frontsector->base_ceiling_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through fake ceilings
				|| (frontsector->heightsec && !R_ResourceIdIsSkyFlat(frontsector->ceiling_res_id))

				// killough 4/17/98: draw ceilings if different light levels
				|| backsector->ceilinglightsec != frontsector->ceilinglightsec

				// [RH] Add check for colormaps
				|| backsector->colormap != frontsector->colormap

				|| backsector->ceiling_xscale != frontsector->ceiling_xscale
				|| backsector->ceiling_yscale != frontsector->ceiling_yscale

				|| (backsector->ceiling_angle + backsector->base_ceiling_angle) !=
				   (frontsector->ceiling_angle + frontsector->base_ceiling_angle)
				;

			// Sky hack
			markceiling = markceiling &&
				(!R_ResourceIdIsSkyFlat(frontsector->ceiling_res_id) || !R_ResourceIdIsSkyFlat(backsector->ceiling_res_id));
		}

		if (rw_hashigh)
		{
			// top texture

			if (linedef->flags & ML_DONTPEGTOP)
			{
				// top of texture at top
				rw_toptexturemid = P_CeilingHeight(frontsector) - viewz;
			}
			else if (toptexture)
			{
				// bottom of texture
				// world-space height of one tile: texel height divided by y-scale
				fixed_t texheight = FixedDiv(toptexture->mHeight << FRACBITS, toptexture->mScaleY);
				rw_toptexturemid = P_CeilingHeight(backsector) - viewz + texheight;
			}
		}

		if (rw_haslow)
		{
			// bottom texture

			if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom, top of texture at top
				rw_bottomtexturemid = P_CeilingHeight(frontsector) - viewz;
			}
			else
			{
				// top of texture at top
				rw_bottomtexturemid = P_FloorHeight(backsector) - viewz;
			}
		}

		// allocate space for masked texture tables
		maskedtexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(sidedef->midtexture));
		if (maskedtexture)
		{
			ds_p->midposts = masked_midposts = masked_midposts_pool.alloc(count) - start;

			// save the per-column scales, pre-scaled into the
			// midtexture's y-scale space, for the masked pass
			fixed_t* midscales = midscales_pool.alloc(count) - start;
			if (maskedtexture->mScaleY == FRACUNIT)
			{
				memcpy(midscales + start, wallscalex + start, count * sizeof(*midscales));
			}
			else
			{
				for (int x = start; x <= stop; x++)
					midscales[x] = FixedDiv(wallscalex[x], maskedtexture->mScaleY);
			}
			ds_p->midscales = midscales;
		}

		// [SL] additional fix for sky hack
		if (R_ResourceIdIsSkyFlat(frontsector->ceiling_res_id) && R_ResourceIdIsSkyFlat(backsector->ceiling_res_id))
			toptexture = NULL;
	}

	// [SL] 2012-01-24 - Horizon line extends to infinity by scaling the wall
	// height to 0

	if (curline->is_horizon)
	{
		rw_scale = ds_p->scale1 = ds_p->scale2 = rw_scalestep = ds_p->light = rw_light = 0;
		midtexture = toptexture = bottomtexture = maskedtexture = NULL;

		for (int n = start; n <= stop; n++)
			walltopf[n] = wallbottomf[n] = FIXED2FLOAT(centeryfrac);
	}

	segtextured = (static_cast<bool>(midtexture) | static_cast<bool>(toptexture)) |
	              ((static_cast<bool>(bottomtexture) | static_cast<bool>(maskedtexture)));

	if (segtextured)
	{
		// calculate light table
		//	use different light tables
		//	for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!fixedcolormap.isValid())
		{
			int lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)
					+ (foggy ? 0 : extralight);

			lightnum += R_OrthogonalLightnumAdjustment();

			lightnum = clamp(lightnum, 0, LIGHTLEVELS - 1);
			walllights = scalelight[lightnum];
		}
	}

	// if a floor / ceiling plane is on the wrong side
	//	of the view plane, it is definitely invisible
	//	and doesn't need to be marked.

	// killough 3/7/98: add deep water check
	if (frontsector->heightsec == NULL ||
		(frontsector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{
		// above view plane?
		if (P_FloorHeight(viewx, viewy, frontsector) >= viewz)
			markfloor = false;
		// below view plane?
		if (P_CeilingHeight(viewx, viewy, frontsector) <= viewz && !R_ResourceIdIsSkyFlat(frontsector->ceiling_res_id))
			markceiling = false;	
	}

	// render it
	if (markceiling && ceilingplane)
		ceilingplane = R_CheckPlane(ceilingplane, start, stop);
	else
		markceiling = false;

	if (markfloor && floorplane)
		floorplane = R_CheckPlane(floorplane, start, stop);
	else
		markfloor = false;

	didsolidcol = false;

	R_RenderSolidSegRange(start, stop);

	// [SL] save full clipping info for masked midtextures
	// cph - if a column was made solid by this wall, we _must_ save full clipping info
	if (maskedtexture || (backsector && didsolidcol))
		ds_p->silhouette = SIL_BOTH;

    // save sprite clipping info
	if ((ds_p->silhouette & SIL_TOP) && ds_p->sprtopclip == NULL)
	{
		int* topclip = sprclip_pool.alloc(count) - start;
		memcpy(topclip + start, ceilingclip.get() + start, count * sizeof(*topclip));
		ds_p->sprtopclip = topclip;
	}

	if ((ds_p->silhouette & SIL_BOTTOM) && ds_p->sprbottomclip == NULL)
	{
		int* bottomclip = sprclip_pool.alloc(count) - start;
		memcpy(bottomclip + start, floorclip.get() + start, count * sizeof(*bottomclip));
		ds_p->sprbottomclip = bottomclip;
	}

	ds_p++;
}


void R_ClearOpenings()
{
	masked_midposts_pool.clear();
	sprclip_pool.clear();
	midscales_pool.clear();
}

VERSION_CONTROL (r_segs_cpp, "$Id$")
