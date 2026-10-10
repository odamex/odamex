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

#include "r_context.h"

#include <algorithm>

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

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

extern fixed_t FocalLengthY;
extern float xfoc, yfoc;

EXTERN_CVAR(r_clipmaskedspecial)

//
// R_OrthogonalLightnumAdjustment
//
int R_OrthogonalLightnumAdjustment(rendercontext_t& ctx)
{
	// [RH] Only do it if not foggy and allowed
    if (!ctx.bsp.foggy && !(level.flags & LEVEL_EVENLIGHTING))
	{
		if (ctx.bsp.curline->linedef->slopetype == ST_HORIZONTAL)
			return -1;
		else if (ctx.bsp.curline->linedef->slopetype == ST_VERTICAL)
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
	rendercontext_t& ctx,
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
		const double z = z1 + (z2 - z1) * ctx.seg.wallufrac[i];
		const double frac = horizon - z * ctx.seg.wallscaled[i];
		array[i] = std::clamp(static_cast<int>(frac), ceilingclipinitial[0], floorclipinitial[0]);
	}
}


//
// R_BlastMaskedSegColumn
//
static inline void R_BlastMaskedSegColumn(rendercontext_t& ctx, void (*drawfunc)())
{
	// R_PrepWall uses floats to calculate scale1 and scale2, which left
	// the scalestep values vulnerable to floating-point rounding errors.
	// If a wall is tall enough and a resolution big enough, the scalestep
	// can be off enough that by accumulation, it draws a row with no data.
	// Your midtex gap! :)
	ctx.sprite.spryscale = ctx.seg.masked_midscales[ctx.draw.dcol.x];

	if (ctx.draw.dcol.source == NULL || ctx.sprite.spryscale <= 0)
		return;

	ctx.draw.dcol.iscale = 0xffffffffu / static_cast<unsigned>(ctx.sprite.spryscale);

	// R_FillWallHeightArray uses centeryfrac and so should we.
	// Otherwise we can have textures drawing at different
	// heights when mouselook is on.

	// calculate unclipped screen coordinates for the whole dense column
	const int64_t topscreen =
	    static_cast<int64_t>(centeryfrac) - ((static_cast<int64_t>(ctx.draw.dcol.texturemid) * ctx.sprite.spryscale) >> FRACBITS);
	const int64_t bottomscreen =
	    topscreen + ((static_cast<int64_t>(ctx.sprite.spryscale) * ctx.draw.dcol.textureheight) >> FRACBITS);

	int64_t yl = (topscreen - 1) >> FRACBITS;
	int64_t yh = (bottomscreen - 1) >> FRACBITS;

	// iscale is already in the texture's scaled space (spryscale was
	// divided by the y-scale), so this tracks y-scaling automatically.
	int64_t texturefrac = 0;
	if (ctx.sprite.mceilingclip[ctx.draw.dcol.x] + 1 > yl)
		texturefrac = (ctx.sprite.mceilingclip[ctx.draw.dcol.x] + 1 - yl) * ctx.draw.dcol.iscale;

	yl = std::max<int64_t>(yl, std::max(ctx.sprite.mceilingclip[ctx.draw.dcol.x], 0));
	yh = std::min<int64_t>(yh, ctx.sprite.mfloorclip[ctx.draw.dcol.x] - 1);

	if (yl > yh || texturefrac >= ctx.draw.dcol.textureheight)
		return;

	// clamp the texture coordinates so out-of-range rows are not drawn
	const int64_t endfrac = texturefrac + (yh - yl) * ctx.draw.dcol.iscale;
	const int64_t maxfrac = ctx.draw.dcol.textureheight;

	if (endfrac >= maxfrac)
	{
		const int64_t cnt = (endfrac - maxfrac + ctx.draw.dcol.iscale) / ctx.draw.dcol.iscale;
		yh -= cnt;
	}

	if (yl >= 0 && yh < viewheight && yl <= yh)
	{
		ctx.draw.dcol.yl = static_cast<int>(yl);
		ctx.draw.dcol.yh = static_cast<int>(yh);
		ctx.draw.dcol.texturefrac = static_cast<fixed_t>(texturefrac);
		drawfunc();
	}
}


//
// R_BlastSolidSegColumn
//
static inline void R_BlastSolidSegColumn(rendercontext_t& ctx, void (*drawfunc)())
{
	fixed_t scale = ctx.seg.wallscalex[ctx.draw.dcol.x];
	if (scale <= 0)
		return;

	// TODO: move iscale calculation outside this function
	ctx.draw.dcol.iscale = FixedMul(0xffffffffu / static_cast<unsigned>(scale), ctx.seg.wallscaley);
	ctx.draw.dcol.texturefrac = ctx.draw.dcol.texturemid +
	                   FixedMul(((ctx.draw.dcol.yl + 1) << FRACBITS) - centeryfrac, ctx.draw.dcol.iscale);

	if (ctx.draw.dcol.yl <= ctx.draw.dcol.yh)
		drawfunc();
}

inline void SolidColumnBlaster(rendercontext_t& ctx)
{
	R_BlastSolidSegColumn(ctx, ctx.draw.colfunc);
}

inline void MaskedColumnBlaster(rendercontext_t& ctx)
{
	R_BlastMaskedSegColumn(ctx, ctx.draw.colfunc);
}

static inline int R_ColumnRangeMinimumHeight(int start, int stop, const int* top)
{
	int minheight = viewheight - 1;
	for (int x = start; x <= stop; x++)
		minheight = std::min(minheight, top[x]);

	return std::max(minheight, 0);
}

static inline int R_ColumnRangeMaximumHeight(int start, int stop, const int* bottom)
{
	int maxheight = 0;
	for (int x = start; x <= stop; x++)
		maxheight = std::max(maxheight, bottom[x]);

	return std::min(maxheight, viewheight - 1);
}


//
// R_RenderColumnRange
//
//
void R_RenderColumnRange(rendercontext_t& ctx, int start, int stop, const int* top, const int* bottom,
		const palindex_t** posts, void (*colblast)(rendercontext_t&), bool calc_light, int columnmethod)
{
	if (start > stop)
		return;

	if (calc_light)
	{
		if (fixedlightlev)
		{
			ctx.draw.dcol.colormap = ctx.draw.basecolormap.with(fixedlightlev);
			calc_light = false;
		}
		else if (fixedcolormap.isValid())
		{
			ctx.draw.dcol.colormap = fixedcolormap;
			calc_light = false;
		}
		else
		{
			if (!ctx.seg.walllights)
				ctx.seg.walllights = scalelight[0];
		}
	}

	if (columnmethod == 0)
	{
		for (int x = start; x <= stop; x++)
		{
			if (calc_light)
			{
				int light_index = std::clamp(ctx.seg.rw_light >> LIGHTSCALESHIFT, 0, MAXLIGHTSCALE - 1);
				ctx.draw.dcol.colormap = ctx.draw.basecolormap.with(ctx.seg.walllights[light_index]);
				ctx.seg.rw_light += ctx.seg.rw_lightstep;
			}

			ctx.draw.dcol.x = x;
			ctx.draw.dcol.yl = std::max(0, top[x]);
			ctx.draw.dcol.yh = std::min(viewheight -1, bottom[x]);
			ctx.draw.dcol.source = posts[x];
			colblast(ctx);
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
				const int index = std::clamp(ctx.seg.rw_light >> LIGHTSCALESHIFT, 0, MAXLIGHTSCALE - 1);
				light_lookup[x] = ctx.seg.walllights[index];
				ctx.seg.rw_light += ctx.seg.rw_lightstep;
			}
		}

		for (int bx = start; bx <= stop; bx = (bx & ~BLOCKMASK) + BLOCKSIZE)
		{
			const int blockstartx = bx;
			const int blockstopx = std::min((bx & ~BLOCKMASK) + BLOCKSIZE - 1, stop);

			const int miny = R_ColumnRangeMinimumHeight(blockstartx, blockstopx, top);
			const int maxy = R_ColumnRangeMaximumHeight(blockstartx, blockstopx, bottom);

			for (int by = miny; by <= maxy; by = (by & ~BLOCKMASK) + BLOCKSIZE)
			{
				const int blockstarty = by;
				const int blockstopy = std::min((by & ~BLOCKMASK) + BLOCKSIZE - 1, viewheight - 1);

				for (int x = blockstartx; x <= blockstopx; x++)
				{
					if (calc_light)
						ctx.draw.dcol.colormap = ctx.draw.basecolormap.with(light_lookup[x]);

					ctx.draw.dcol.x = x;
					ctx.draw.dcol.yl = std::max(top[x], blockstarty);
					ctx.draw.dcol.yh = std::min(bottom[x], blockstopy);
					ctx.draw.dcol.source = posts[x];
					colblast(ctx);
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
void R_RenderSolidSegRange(rendercontext_t& ctx, int start, int stop)
{
	static int lower[MAXWIDTH];
	const int count = stop - start + 1;
	const int initial_light = ctx.seg.rw_light;

	if (start > stop)
		return;

	ctx.draw.dcol.masked = false;

	// clip the front of the walls to the ceiling and floor
	for (int x = start; x <= stop; x++)
	{
		ctx.seg.walltopf[x] = std::max(ctx.seg.walltopf[x], ctx.plane.ceilingclip[x]);
		ctx.seg.wallbottomf[x] = std::min(ctx.seg.wallbottomf[x], ctx.plane.floorclip[x]);
	}

	// mark ceiling-plane areas
	if (ctx.seg.markceiling)
	{
		for (int x = start; x <= stop; x++)
		{
			const int top = std::max(ctx.plane.ceilingclip[x], 0);
			const int bottom = std::min({ctx.seg.walltopf[x] - 1, ctx.plane.floorclip[x] - 1, viewheight - 1});

			if (top <= bottom)
			{
				ctx.plane.ceilingplane->top[x] = top;
				ctx.plane.ceilingplane->bottom[x] = bottom;
			}
		}
	}

	// mark floor-plane areas
	if (ctx.seg.markfloor)
	{
		for (int x = start; x <= stop; x++)
		{
			const int top = std::max({ctx.seg.wallbottomf[x], ctx.plane.ceilingclip[x], 0});
			const int bottom = std::min(ctx.plane.floorclip[x] - 1, viewheight - 1);

			if (top <= bottom)
			{
				ctx.plane.floorplane->top[x] = top;
				ctx.plane.floorplane->bottom[x] = bottom;
			}
		}
	}

	if (ctx.seg.midtexture)		// 1-sided line
	{
		// draw the middle wall tier
		for (int x = start; x <= stop; x++)
			lower[x] = ctx.seg.wallbottomf[x] - 1;

		ctx.seg.rw_light = initial_light;

		ctx.seg.wallscaley = ctx.seg.midtexture->mScaleY;
		ctx.draw.dcol.textureheight = ctx.seg.midtexture->mHeight << FRACBITS;
		ctx.draw.dcol.texturemid = FixedMul(ctx.seg.rw_midtexturemid, ctx.seg.wallscaley) + ctx.bsp.curline->sidedef->rowoffset;
		ctx.draw.dcol.texturedata = ctx.seg.midtexture->mData;
		ctx.draw.dcol.argbtexturedata = ctx.seg.midtexture->mARGBData;

		R_RenderColumnRange(ctx, start, stop, ctx.seg.walltopf, lower, ctx.seg.midposts, SolidColumnBlaster, true, 0);

		// indicate that no further drawing can be done in this column
		memcpy(&ctx.plane.ceilingclip[start], &floorclipinitial[start], count * sizeof(ctx.plane.ceilingclip[0]));
		memcpy(&ctx.plane.floorclip[start], &ceilingclipinitial[start], count * sizeof(ctx.plane.floorclip[0]));
	}
	else			// 2-sided line
	{
		if (ctx.seg.toptexture)
		{
			// draw the upper wall tier
			ctx.seg.rw_light = initial_light;

			for (int x = start; x <= stop; x++)
			{
				ctx.seg.walltopb[x] = std::max(std::min(ctx.seg.walltopb[x], ctx.plane.floorclip[x]), ctx.seg.walltopf[x]);
				lower[x] = ctx.seg.walltopb[x] - 1;
			}

			ctx.seg.wallscaley = ctx.seg.toptexture->mScaleY;
			ctx.draw.dcol.textureheight = ctx.seg.toptexture->mHeight << FRACBITS;
			ctx.draw.dcol.texturemid = FixedMul(ctx.seg.rw_toptexturemid, ctx.seg.wallscaley) + ctx.bsp.curline->sidedef->rowoffset;
			ctx.draw.dcol.texturedata = ctx.seg.toptexture->mData;
			ctx.draw.dcol.argbtexturedata = ctx.seg.toptexture->mARGBData;

			R_RenderColumnRange(ctx, start, stop, ctx.seg.walltopf, lower, ctx.seg.topposts, SolidColumnBlaster, true, 0);

			memcpy(&ctx.plane.ceilingclip[start], ctx.seg.walltopb + start, count * sizeof(ctx.plane.ceilingclip[0]));
		}
		else if (ctx.seg.markceiling)
		{
			// no upper wall
			memcpy(&ctx.plane.ceilingclip[start], ctx.seg.walltopf + start, count * sizeof(ctx.plane.ceilingclip[0]));
		}

		if (ctx.seg.bottomtexture)
		{
			// draw the lower wall tier
			ctx.seg.rw_light = initial_light;

			for (int x = start; x <= stop; x++)
			{
				ctx.seg.wallbottomb[x] = std::min(std::max(ctx.seg.wallbottomb[x], ctx.plane.ceilingclip[x]), ctx.seg.wallbottomf[x]);
				lower[x] = ctx.seg.wallbottomf[x] - 1;
			}

			ctx.seg.wallscaley = ctx.seg.bottomtexture->mScaleY;
			ctx.draw.dcol.textureheight = ctx.seg.bottomtexture->mHeight << FRACBITS;
			ctx.draw.dcol.texturemid = FixedMul(ctx.seg.rw_bottomtexturemid, ctx.seg.wallscaley) + ctx.bsp.curline->sidedef->rowoffset;
			ctx.draw.dcol.texturedata = ctx.seg.bottomtexture->mData;
			ctx.draw.dcol.argbtexturedata = ctx.seg.bottomtexture->mARGBData;

			R_RenderColumnRange(ctx, start, stop, ctx.seg.wallbottomb, lower, ctx.seg.bottomposts, SolidColumnBlaster, true, 0);

			memcpy(&ctx.plane.floorclip[start], ctx.seg.wallbottomb + start, count * sizeof(ctx.plane.floorclip[0]));
		}
		else if (ctx.seg.markfloor)
		{
			// no lower wall
			memcpy(&ctx.plane.floorclip[start], ctx.seg.wallbottomf + start, count * sizeof(ctx.plane.floorclip[0]));
		}

		if (ctx.seg.maskedtexture)
		{
			// save texturecol for backdrawing of masked mid texture
			for (int x = start; x <= stop; x++)
			{
				int colnum = ctx.seg.maskedtexture->wrapColumn(FixedMul(ctx.seg.texoffs[x], ctx.seg.maskedtexture->mScaleX) >> FRACBITS);
				ctx.seg.masked_midposts[x] = ctx.seg.maskedtexture->getColumn(colnum);
			}
		}
	}

	for (int x = start; x <= stop; x++)
	{
		// cph - if we completely blocked further sight through this column,
		// add this info to the solid columns array
		if ((ctx.seg.markceiling || ctx.seg.markfloor) && (ctx.plane.floorclip[x] <= ctx.plane.ceilingclip[x]))
		{
			ctx.bsp.solidcol[x] = 1;
			ctx.seg.didsolidcol = true;
		}
	}
}


//
// R_RenderMaskedSegRange
//
// Renders a masked seg
//
void R_RenderMaskedSegRange(rendercontext_t& ctx, drawseg_t* ds, int x1, int x2)
{
	sector_t	tempsec;		// killough 4/13/98

	ctx.draw.dcol.color = (ctx.draw.dcol.color + 4) & 0xFF;	// color if using r_drawflat
	ctx.draw.dcol.masked = true;

	// Calculate light table.
	// Use different light tables
	//	 for horizontal / vertical / diagonal. Diagonal?
	// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
	ctx.bsp.curline = ds->curline;

	// killough 4/11/98: draw translucent 2s normal textures
	// [RH] modified because we don't use user-definable
	//		translucency maps
	if (ctx.bsp.curline->linedef->lucency < 240)
	{
		R_SetLucentDrawFuncs();
		ctx.draw.dcol.translevel = ctx.bsp.curline->linedef->lucency << 8;
	}
	else
	{
		R_ResetDrawFuncs();
	}

	ctx.bsp.frontsector = ctx.bsp.curline->frontsector;
	ctx.bsp.backsector = ctx.bsp.curline->backsector;

	const Texture* texture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(ctx.bsp.curline->sidedef->midtexture));
	fixed_t texheight = FixedMul(texture->mHeight << FRACBITS, texture->mScaleY);

	// find texture positioning
	if (ctx.bsp.curline->linedef->flags & ML_DONTPEGBOTTOM)
		// offset by the world-space height of one tile (texel height / y-scale)
		ctx.draw.dcol.texturemid = std::max(ctx.bsp.frontsector->floortexz, ctx.bsp.backsector->floortexz) +
		                  FixedDiv(texture->mHeight << FRACBITS, texture->mScaleY);
	else
		ctx.draw.dcol.texturemid = std::min(ctx.bsp.frontsector->ceilingtexz, ctx.bsp.backsector->ceilingtexz);

	ctx.draw.dcol.texturemid = FixedMul(ctx.draw.dcol.texturemid - viewz, texture->mScaleY) +
	                  ctx.bsp.curline->sidedef->rowoffset;
	
	int64_t topscreenclip = static_cast<int64_t>(centeryfrac) << FRACBITS;
	int64_t botscreenclip = static_cast<int64_t>(centeryfrac - (viewheight << FRACBITS)) << FRACBITS;
 
	// top of texture entirely below screen?
	if (static_cast<int64_t>(ctx.draw.dcol.texturemid) * ds->scale1 <= botscreenclip &&
		static_cast<int64_t>(ctx.draw.dcol.texturemid) * ds->scale2 <= botscreenclip)
		return;

	// bottom of texture entirely above screen?
	if (static_cast<int64_t>(ctx.draw.dcol.texturemid - texheight) * ds->scale1 > topscreenclip &&
		static_cast<int64_t>(ctx.draw.dcol.texturemid - texheight) * ds->scale2 > topscreenclip)
		return;

	ctx.draw.basecolormap = ctx.bsp.frontsector->colormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	int lightnum = (R_FakeFlat(ctx, ctx.bsp.frontsector, &tempsec, NULL, NULL, false)->lightlevel >> LIGHTSEGSHIFT) + (ctx.bsp.foggy ? 0 : extralight);
	lightnum += R_OrthogonalLightnumAdjustment(ctx);

	ctx.seg.walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
		lightnum <  0 ? scalelight[0] : scalelight[lightnum];

	ctx.seg.masked_midposts = ds->midposts;
	ctx.seg.masked_midscales = ds->midscales;

	ctx.seg.rw_lightstep = ds->lightstep;
	ctx.seg.rw_light = ds->light + (x1 - ds->x1) * ctx.seg.rw_lightstep;

	ctx.sprite.mfloorclip = ds->sprbottomclip;
	ctx.sprite.mceilingclip = ds->sprtopclip;

	ctx.draw.dcol.textureheight = texture->mHeight << FRACBITS;
	ctx.draw.dcol.texturedata = texture->mData;
	ctx.draw.dcol.argbtexturedata = texture->mARGBData;

	// [SL] pre-calculate scaling for each column
	if (ctx.seg.masked_midscales)
	{
		memcpy(ctx.seg.wallscalex + x1, ctx.seg.masked_midscales + x1, (x2 - x1 + 1) * sizeof(*ctx.seg.wallscalex));
	}
	else
	{
		ctx.seg.rw_scalestep = FixedDiv(ds->scalestep, texture->mScaleY);
		fixed_t scale = FixedDiv(ds->scale1, texture->mScaleY) + (x1 - ds->x1) * ctx.seg.rw_scalestep;
		for (int x = x1; x <= x2; x++)
		{
			ctx.seg.wallscalex[x] = scale;
			scale += ctx.seg.rw_scalestep;
		}
	}

	// draw the columns
	R_RenderColumnRange(ctx, x1, x2, negonearray, viewheightarray, ds->midposts, MaskedColumnBlaster, true, 0);

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
void R_PrepWall(rendercontext_t& ctx, fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2,
                fixed_t tx1, fixed_t ty1, fixed_t tx2, fixed_t ty2, int start, int stop)
{
	const int width = stop - start + 1;
	if (width <= 0)
		return;

	// Calculate distance from lineseg start to start of clipped lineseg
	vertex_t *v1;			// determine which vertex of the linedef should be used for texture alignment
	if (ctx.bsp.curline->linedef->sidenum[0] == ctx.bsp.curline->sidedef - sides)
		v1 = ctx.bsp.curline->linedef->v1;
	else
		v1 = ctx.bsp.curline->linedef->v2;
	fixed_t segoffs = R_LineLength(v1->x, v1->y, px1, py1) + ctx.bsp.curline->sidedef->textureoffset;

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
		depth = std::clamp(depth, mindepth, maxdepth);

		const double scale = yfoc / depth;
		ctx.seg.wallscaled[i] = scale;
		ctx.seg.wallscalex[i] = DOUBLE2FIXED(scale);

		const double uunits = ((depth * raydx - cx1) * wdx + (depth - cy1) * wdy) * invseglen;
		ctx.seg.wallufrac[i] = uunits * invseglen;
		ctx.seg.texoffs[i] = segoffs +
		             static_cast<fixed_t>(static_cast<int64_t>(uunits * 65536.0));
	}

	ctx.seg.rw_scalestep = FLOAT2FIXED((ctx.seg.wallscaled[stop] - ctx.seg.wallscaled[start]) / width);

	// get the z coordinates of the line's vertices on each side of the line
	ctx.seg.rw_frontcz1 = P_CeilingHeight(px1, py1, ctx.bsp.frontsector);
	ctx.seg.rw_frontfz1 = P_FloorHeight(px1, py1, ctx.bsp.frontsector);
	ctx.seg.rw_frontcz2 = P_CeilingHeight(px2, py2, ctx.bsp.frontsector);
	ctx.seg.rw_frontfz2 = P_FloorHeight(px2, py2, ctx.bsp.frontsector);

	// calculate the upper and lower heights of the walls in the front
	R_FillWallHeightArray(ctx, ctx.seg.walltopf, start, stop, ctx.seg.rw_frontcz1, ctx.seg.rw_frontcz2);
	R_FillWallHeightArray(ctx, ctx.seg.wallbottomf, start, stop, ctx.seg.rw_frontfz1, ctx.seg.rw_frontfz2);

	ctx.seg.rw_hashigh = ctx.seg.rw_haslow = false;

	if (ctx.bsp.backsector)
	{
		ctx.seg.rw_backcz1 = P_CeilingHeight(px1, py1, ctx.bsp.backsector);
		ctx.seg.rw_backfz1 = P_FloorHeight(px1, py1, ctx.bsp.backsector);
		ctx.seg.rw_backcz2 = P_CeilingHeight(px2, py2, ctx.bsp.backsector);
		ctx.seg.rw_backfz2 = P_FloorHeight(px2, py2, ctx.bsp.backsector);

		// calculate the upper and lower heights of the walls in the back
		R_FillWallHeightArray(ctx, ctx.seg.walltopb, start, stop, ctx.seg.rw_backcz1, ctx.seg.rw_backcz2);
		R_FillWallHeightArray(ctx, ctx.seg.wallbottomb, start, stop, ctx.seg.rw_backfz1, ctx.seg.rw_backfz2);

		static constexpr fixed_t tolerance = FRACUNIT / 2;

		// determine if an upper texture is showing
		ctx.seg.rw_hashigh	= (P_CeilingHeight(ctx.bsp.curline->v1->x, ctx.bsp.curline->v1->y, ctx.bsp.frontsector) - tolerance >
					   P_CeilingHeight(ctx.bsp.curline->v1->x, ctx.bsp.curline->v1->y, ctx.bsp.backsector)) ||
					  (P_CeilingHeight(ctx.bsp.curline->v2->x, ctx.bsp.curline->v2->y, ctx.bsp.frontsector) - tolerance>
					   P_CeilingHeight(ctx.bsp.curline->v2->x, ctx.bsp.curline->v2->y, ctx.bsp.backsector));

		// determine if a lower texture is showing
		ctx.seg.rw_haslow	= (P_FloorHeight(ctx.bsp.curline->v1->x, ctx.bsp.curline->v1->y, ctx.bsp.frontsector) + tolerance <
					   P_FloorHeight(ctx.bsp.curline->v1->x, ctx.bsp.curline->v1->y, ctx.bsp.backsector)) ||
					  (P_FloorHeight(ctx.bsp.curline->v2->x, ctx.bsp.curline->v2->y, ctx.bsp.frontsector) + tolerance <
					   P_FloorHeight(ctx.bsp.curline->v2->x, ctx.bsp.curline->v2->y, ctx.bsp.backsector));

		// hack to allow height changes in outdoor areas (sky hack)
		// copy back ceiling height array to front ceiling height array
		if (R_ResourceIdIsSkyFlat(ctx.bsp.frontsector->ceiling_res_id) &&
			R_ResourceIdIsSkyFlat(ctx.bsp.backsector->ceiling_res_id))
			memcpy(ctx.seg.walltopf+start, ctx.seg.walltopb+start, width*sizeof(*ctx.seg.walltopb));
	}

	// Cache the wall textures
	ctx.seg.toptexture = ctx.seg.midtexture = ctx.seg.bottomtexture = ctx.seg.maskedtexture = NULL;

	if (!ctx.bsp.backsector)
		ctx.seg.midtexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(ctx.bsp.curline->sidedef->midtexture));

	if (ctx.seg.rw_hashigh)
		ctx.seg.toptexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(ctx.bsp.curline->sidedef->toptexture));

	if (ctx.seg.rw_haslow)
		ctx.seg.bottomtexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(ctx.bsp.curline->sidedef->bottomtexture));

	// determine which texture posts will be used for each screen
	// column in this range.
	for (int i = start; i <= stop; i++)
	{
		const fixed_t colfrac = ctx.seg.texoffs[i];

		if (ctx.seg.toptexture)
		{
			int colnum = ctx.seg.toptexture->wrapColumn(FixedMul(colfrac, ctx.seg.toptexture->mScaleX) >> FRACBITS);
			ctx.seg.topposts[i] = ctx.seg.toptexture->getColumn(colnum);
		}
		if (ctx.seg.midtexture)
		{
			int colnum = ctx.seg.midtexture->wrapColumn(FixedMul(colfrac, ctx.seg.midtexture->mScaleX) >> FRACBITS);
			ctx.seg.midposts[i] = ctx.seg.midtexture->getColumn(colnum);
		}
		if (ctx.seg.bottomtexture)
		{
			int colnum = ctx.seg.bottomtexture->wrapColumn(FixedMul(colfrac, ctx.seg.bottomtexture->mScaleX) >> FRACBITS);
			ctx.seg.bottomposts[i] = ctx.seg.bottomtexture->getColumn(colnum);
		}
	}
}


//
// R_StoreWallRange
// A wall segment will be drawn
//	between start and stop pixels (inclusive).
//
void R_StoreWallRange(rendercontext_t& ctx, int start, int stop)
{
#ifdef RANGECHECK
	if (start >= viewwidth || start > stop)
		I_FatalError("Bad R_StoreWallRange: {} to {}", start , stop);
#endif

	const int count = stop - start + 1;
	if (count <= 0)
		return;

	R_ReallocDrawSegs(ctx);	// don't overflow and crash

	ctx.bsp.sidedef = ctx.bsp.curline->sidedef;
	ctx.bsp.linedef = ctx.bsp.curline->linedef;

	// mark the segment as visible for auto map
	ctx.bsp.linedef->flags |= ML_MAPPED;

	ctx.bsp.ds_p->x1 = start;
	ctx.bsp.ds_p->x2 = stop;
	ctx.bsp.ds_p->curline = ctx.bsp.curline;

	// calculate scale at both ends and step
	ctx.bsp.ds_p->scale1 = ctx.seg.rw_scale = ctx.seg.wallscalex[start];
	ctx.bsp.ds_p->scale2 = ctx.seg.wallscalex[stop];
	ctx.bsp.ds_p->scalestep = ctx.seg.rw_scalestep;

	ctx.bsp.ds_p->light = ctx.seg.rw_light = ctx.seg.rw_scale * lightscalexmul;
 	ctx.bsp.ds_p->lightstep = ctx.seg.rw_lightstep = ctx.seg.rw_scalestep * lightscalexmul;

	// calculate texture boundaries
	//	and decide if floor / ceiling marks are needed
	ctx.seg.maskedtexture = NULL;
	ctx.bsp.ds_p->midposts = NULL;
	ctx.bsp.ds_p->midscales = NULL;

	if (!ctx.bsp.backsector)
	{
		// single sided line

		// a single sided line is terminal, so it must mark ends
		ctx.seg.markfloor = ctx.seg.markceiling = true;

		if (ctx.bsp.linedef->flags & ML_DONTPEGBOTTOM)
		{
			// bottom of texture at bottom
			if (ctx.seg.midtexture)
			{
				// world-space height of one tile: texel height divided by y-scale
				fixed_t texheight = FixedDiv(ctx.seg.midtexture->mHeight << FRACBITS, ctx.seg.midtexture->mScaleY);
				ctx.seg.rw_midtexturemid = ctx.bsp.frontsector->floortexz - viewz + texheight;
			}
		}
		else
		{
			// top of texture at top
			const fixed_t fc = ctx.bsp.frontsector->ceilingtexz;
			ctx.seg.rw_midtexturemid = fc - viewz;
		}

		ctx.bsp.ds_p->silhouette = SIL_BOTH;
		ctx.bsp.ds_p->sprtopclip = viewheightarray;
		ctx.bsp.ds_p->sprbottomclip = negonearray;
	}
	else
	{
		// two sided line
		ctx.bsp.ds_p->sprtopclip = ctx.bsp.ds_p->sprbottomclip = NULL;
		ctx.bsp.ds_p->silhouette = 0;

		if (ctx.bsp.doorclosed)
		{
			// clip all sprites behind this closed door (or otherwise solid line)
			ctx.bsp.ds_p->silhouette = SIL_BOTH;
			ctx.bsp.ds_p->sprtopclip = viewheightarray;
			ctx.bsp.ds_p->sprbottomclip = negonearray;
		}
		else
		{
			// determine sprite clipping for non-solid line segs
			if (ctx.seg.rw_frontfz1 > ctx.seg.rw_backfz1 || ctx.seg.rw_frontfz2 > ctx.seg.rw_backfz2 ||
				ctx.seg.rw_backfz1 > viewz || ctx.seg.rw_backfz2 > viewz ||
				!P_IsPlaneLevel(&ctx.bsp.backsector->floorplane))	// backside sloping?
				ctx.bsp.ds_p->silhouette |= SIL_BOTTOM;

			if (ctx.seg.rw_frontcz1 < ctx.seg.rw_backcz1 || ctx.seg.rw_frontcz2 < ctx.seg.rw_backcz2 ||
				ctx.seg.rw_backcz1 < viewz || ctx.seg.rw_backcz2 < viewz ||
				!P_IsPlaneLevel(&ctx.bsp.backsector->ceilingplane))	// backside sloping?
				ctx.bsp.ds_p->silhouette |= SIL_TOP;
		}

		if (ctx.bsp.doorclosed)
		{
			ctx.seg.markceiling = ctx.seg.markfloor = true;
		}
		else if (ctx.draw.spanfunc == R_FillSpan)
		{
			ctx.seg.markfloor = ctx.seg.markceiling = (ctx.bsp.frontsector != ctx.bsp.backsector);
		}
		else
		{
			ctx.seg.markfloor =
				  !P_IdenticalPlanes(&ctx.bsp.backsector->floorplane, &ctx.bsp.frontsector->floorplane)
				|| ctx.bsp.backsector->lightlevel != ctx.bsp.frontsector->lightlevel
				|| ctx.bsp.backsector->floor_res_id != ctx.bsp.frontsector->floor_res_id

				// killough 3/7/98: Add checks for (x,y) offsets
				|| ctx.bsp.backsector->floor_xoffs != ctx.bsp.frontsector->floor_xoffs
				|| (ctx.bsp.backsector->floor_yoffs + ctx.bsp.backsector->base_floor_yoffs) !=
				   (ctx.bsp.frontsector->floor_yoffs + ctx.bsp.frontsector->base_floor_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through deep water
				|| ctx.bsp.frontsector->heightsec

				// killough 4/17/98: draw floors if different light levels
				|| ctx.bsp.backsector->floorlightsec != ctx.bsp.frontsector->floorlightsec

				// [EB] check for special too for DSDA-compatibility on MBF21
				|| (r_clipmaskedspecial && ctx.bsp.backsector->special != ctx.bsp.frontsector->special)

				// [RH] Add checks for colormaps
				|| ctx.bsp.backsector->colormap != ctx.bsp.frontsector->colormap

				|| ctx.bsp.backsector->floor_xscale != ctx.bsp.frontsector->floor_xscale
				|| ctx.bsp.backsector->floor_yscale != ctx.bsp.frontsector->floor_yscale

				|| (ctx.bsp.backsector->floor_angle + ctx.bsp.backsector->base_floor_angle) !=
				   (ctx.bsp.frontsector->floor_angle + ctx.bsp.frontsector->base_floor_angle)
				;

			// Sky hack
			// MBF sky transfers split the visplane in 2, so in order for sky
			// transfer skyhack to work, we need to identify both sectors' sky
			const bool ceilingskyhack =
				!R_ResourceIdIsSkyFlat(ctx.bsp.frontsector->ceiling_res_id) || !R_ResourceIdIsSkyFlat(ctx.bsp.backsector->ceiling_res_id);

			ctx.seg.markceiling =
				  (ceilingskyhack &&
				   !P_IdenticalPlanes(&ctx.bsp.backsector->ceilingplane, &ctx.bsp.frontsector->ceilingplane))
				|| ctx.bsp.backsector->lightlevel != ctx.bsp.frontsector->lightlevel
				|| ctx.bsp.backsector->ceiling_res_id != ctx.bsp.frontsector->ceiling_res_id

				// killough 3/7/98: Add checks for (x,y) offsets
				|| ctx.bsp.backsector->ceiling_xoffs != ctx.bsp.frontsector->ceiling_xoffs
				|| (ctx.bsp.backsector->ceiling_yoffs + ctx.bsp.backsector->base_ceiling_yoffs) !=
				   (ctx.bsp.frontsector->ceiling_yoffs + ctx.bsp.frontsector->base_ceiling_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through fake ceilings
				|| (ctx.bsp.frontsector->heightsec && !R_ResourceIdIsSkyFlat(ctx.bsp.frontsector->ceiling_res_id))

				// killough 4/17/98: draw ceilings if different light levels
				|| ctx.bsp.backsector->ceilinglightsec != ctx.bsp.frontsector->ceilinglightsec

				// [RH] Add check for colormaps
				|| ctx.bsp.backsector->colormap != ctx.bsp.frontsector->colormap

				|| ctx.bsp.backsector->ceiling_xscale != ctx.bsp.frontsector->ceiling_xscale
				|| ctx.bsp.backsector->ceiling_yscale != ctx.bsp.frontsector->ceiling_yscale

				|| (ctx.bsp.backsector->ceiling_angle + ctx.bsp.backsector->base_ceiling_angle) !=
				   (ctx.bsp.frontsector->ceiling_angle + ctx.bsp.frontsector->base_ceiling_angle)
				;
		}

		if (ctx.seg.rw_hashigh)
		{
			// top texture

			if (ctx.bsp.linedef->flags & ML_DONTPEGTOP)
			{
				// top of texture at top
				ctx.seg.rw_toptexturemid = ctx.bsp.frontsector->ceilingtexz - viewz;
			}
			else if (ctx.seg.toptexture)
			{
				// bottom of texture
				// world-space height of one tile: texel height divided by y-scale
				fixed_t texheight = FixedDiv(ctx.seg.toptexture->mHeight << FRACBITS, ctx.seg.toptexture->mScaleY);
				ctx.seg.rw_toptexturemid = ctx.bsp.backsector->ceilingtexz - viewz + texheight;
			}
		}

		if (ctx.seg.rw_haslow)
		{
			// bottom texture

			if (ctx.bsp.linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom, top of texture at top
				ctx.seg.rw_bottomtexturemid = ctx.bsp.frontsector->ceilingtexz - viewz;
			}
			else
			{
				// top of texture at top
				ctx.seg.rw_bottomtexturemid = ctx.bsp.backsector->floortexz - viewz;
			}
		}

		// allocate space for masked texture tables
		ctx.seg.maskedtexture = Res_CacheTexture(Res_GetAnimatedTextureResourceId(ctx.bsp.sidedef->midtexture));
		if (ctx.seg.maskedtexture)
		{
			ctx.bsp.ds_p->midposts = ctx.seg.masked_midposts = ctx.seg.masked_midposts_pool.alloc(count) - start;

			// save the per-column scales, pre-scaled into the
			// midtexture's y-scale space, for the masked pass
			fixed_t* midscales = ctx.seg.midscales_pool.alloc(count) - start;
			if (ctx.seg.maskedtexture->mScaleY == FRACUNIT)
			{
				memcpy(midscales + start, ctx.seg.wallscalex + start, count * sizeof(*midscales));
			}
			else
			{
				for (int x = start; x <= stop; x++)
					midscales[x] = FixedDiv(ctx.seg.wallscalex[x], ctx.seg.maskedtexture->mScaleY);
			}
			ctx.bsp.ds_p->midscales = midscales;
		}

		// [SL] additional fix for sky hack
		if (R_ResourceIdIsSkyFlat(ctx.bsp.frontsector->ceiling_res_id) && R_ResourceIdIsSkyFlat(ctx.bsp.backsector->ceiling_res_id))
			ctx.seg.toptexture = NULL;
	}

	// [SL] 2012-01-24 - Horizon line extends to infinity by scaling the wall
	// height to 0

	if (ctx.bsp.curline->is_horizon)
	{
		ctx.seg.rw_scale = ctx.bsp.ds_p->scale1 = ctx.bsp.ds_p->scale2 = ctx.seg.rw_scalestep = ctx.bsp.ds_p->light = ctx.seg.rw_light = 0;
		ctx.seg.midtexture = ctx.seg.toptexture = ctx.seg.bottomtexture = ctx.seg.maskedtexture = NULL;

		for (int n = start; n <= stop; n++)
			ctx.seg.walltopf[n] = ctx.seg.wallbottomf[n] = FIXED2FLOAT(centeryfrac);
	}

	ctx.seg.segtextured = (static_cast<bool>(ctx.seg.midtexture) | static_cast<bool>(ctx.seg.toptexture)) |
	              ((static_cast<bool>(ctx.seg.bottomtexture) | static_cast<bool>(ctx.seg.maskedtexture)));

	if (ctx.seg.segtextured)
	{
		// calculate light table
		//	use different light tables
		//	for horizontal / vertical / diagonal
		// OPTIMIZE: get rid of LIGHTSEGSHIFT globally
		if (!fixedcolormap.isValid())
		{
			int lightnum = (ctx.bsp.frontsector->lightlevel >> LIGHTSEGSHIFT)
					+ (ctx.bsp.foggy ? 0 : extralight);

			lightnum += R_OrthogonalLightnumAdjustment(ctx);

			lightnum = std::clamp(lightnum, 0, LIGHTLEVELS - 1);
			ctx.seg.walllights = scalelight[lightnum];
		}
	}

	// if a floor / ceiling plane is on the wrong side
	//	of the view plane, it is definitely invisible
	//	and doesn't need to be marked.

	// killough 3/7/98: add deep water check
	if (ctx.bsp.frontsector->heightsec == NULL ||
		(ctx.bsp.frontsector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{
		// above view plane?
		if (P_FloorHeight(viewx, viewy, ctx.bsp.frontsector) >= viewz)
			ctx.seg.markfloor = false;
		// below view plane?
		if (P_CeilingHeight(viewx, viewy, ctx.bsp.frontsector) <= viewz && !R_ResourceIdIsSkyFlat(ctx.bsp.frontsector->ceiling_res_id))
			ctx.seg.markceiling = false;	
	}

	// render it
	if (ctx.seg.markceiling && ctx.plane.ceilingplane)
		ctx.plane.ceilingplane = R_CheckPlane(ctx, ctx.plane.ceilingplane, start, stop);
	else
		ctx.seg.markceiling = false;

	if (ctx.seg.markfloor && ctx.plane.floorplane)
		ctx.plane.floorplane = R_CheckPlane(ctx, ctx.plane.floorplane, start, stop);
	else
		ctx.seg.markfloor = false;

	ctx.seg.didsolidcol = false;

	R_RenderSolidSegRange(ctx, start, stop);

	// [SL] save full clipping info for masked midtextures
	// cph - if a column was made solid by this wall, we _must_ save full clipping info
	if (ctx.seg.maskedtexture || (ctx.bsp.backsector && ctx.seg.didsolidcol))
		ctx.bsp.ds_p->silhouette = SIL_BOTH;

    // save sprite clipping info
	if ((ctx.bsp.ds_p->silhouette & SIL_TOP) && ctx.bsp.ds_p->sprtopclip == NULL)
	{
		int* topclip = ctx.seg.sprclip_pool.alloc(count) - start;
		memcpy(topclip + start, ctx.plane.ceilingclip.get() + start, count * sizeof(*topclip));
		ctx.bsp.ds_p->sprtopclip = topclip;
	}

	if ((ctx.bsp.ds_p->silhouette & SIL_BOTTOM) && ctx.bsp.ds_p->sprbottomclip == NULL)
	{
		int* bottomclip = ctx.seg.sprclip_pool.alloc(count) - start;
		memcpy(bottomclip + start, ctx.plane.floorclip.get() + start, count * sizeof(*bottomclip));
		ctx.bsp.ds_p->sprbottomclip = bottomclip;
	}

	ctx.bsp.ds_p++;
}


void R_ClearOpenings(rendercontext_t& ctx)
{
	ctx.seg.masked_midposts_pool.clear();
	ctx.seg.sprclip_pool.clear();
	ctx.seg.midscales_pool.clear();
}

VERSION_CONTROL (r_segs_cpp, "$Id$")
