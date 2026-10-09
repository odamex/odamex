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
#include <array>
#include <nonstd/bit.hpp>

#include "m_mempool.h"

#include "i_system.h"


#include "i_video.h"
#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_video.h"

#include "m_vectors.h"

#include "p_mapformat.h"

#include "p_lnspec.h"

// a pool of bytes allocated for sprite clipping arrays
Pool<tallpost_t*> masked_midposts_pool(4096);
Pool<int> sprclip_pool(4096);
Pool<fixed_t> midscales_pool(4096);

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

static bool		segtextured;	// True if any of the segs textures might be visible.
static bool		markfloor;		// False if the back side is the same plane.
static bool		markceiling;
static bool		didsolidcol;
static int		maskedtexture;
static int		toptexture;
static int		bottomtexture;
static int		midtexture;

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

static tallpost_t* topposts[MAXWIDTH];
static tallpost_t* midposts[MAXWIDTH];
static tallpost_t* bottomposts[MAXWIDTH];

static fixed_t wallscaley;
static fixed_t wallscalex[MAXWIDTH];
static int texoffs[MAXWIDTH];

extern fixed_t FocalLengthY;
extern float yfoc;
fixed_t R_FogLight2Shade(int lightlevel);

static tallpost_t** masked_midposts;
static const fixed_t* masked_midscales;

EXTERN_CVAR(r_clipmaskedspecial)
EXTERN_CVAR(r_fogboundary)

namespace
{

//
// R_IsFogBoundary
//
// Returns true if the seg between front and back is a boundary between
// two different fogs, in which case the fog volume on the front side is
// shaded over the view during the masked pass so it appears to fill the
// open space.
//
// As in ZDoom, only segs whose front (viewer-side) sector is
// foggy qualify -- a fog volume seen from clear air is covered by the far
// faces of the volume, whose front sector is the fog.
//
bool R_IsFogBoundary(const sector_t* front, const sector_t* back)
{
	if (!r_fogboundary || !front->colormap || !back->colormap)
		return false;

	const argb_t frontfade = front->colormap->fade;
	const argb_t backfade = back->colormap->fade;

	// the front sector must actually have fog
	if ((frontfade.getr() | frontfade.getg() | frontfade.getb()) == 0)
		return false;

	// fogs of identical color don't need a boundary
	if (frontfade.getr() == backfade.getr() &&
		frontfade.getg() == backfade.getg() &&
		frontfade.getb() == backfade.getb())
		return false;

	// don't fog the boundary if one ceiling is sky
	return !R_IsSkyFlat(front->ceilingpic) || !R_IsSkyFlat(back->ceilingpic);
}

fixed_t R_FogBoundaryVisMul();

bool      wall_foglight;
fixed_t   wall_fogshade;
fixed_t   wall_fogvismul;  // converts rw_light into ZDoom vis units

//
// R_SetWallFogLight
//
// Enables ZDoom-style fog shading of wall tiers when the front sector has a
// fog fade.
//
// Must be called wherever walllights is selected for wall drawing.
//
void R_SetWallFogLight(const sector_t* frontsec, int lightlevel)
{
	wall_foglight = false;

	if (!frontsec || !frontsec->colormap)
		return;

	const argb_t fade = frontsec->colormap->fade;
	if ((fade.getr() | fade.getg() | fade.getb()) == 0)
		return;

	wall_foglight = true;
	wall_fogshade = R_FogLight2Shade(lightlevel);
	wall_fogvismul = FixedDiv(R_FogBoundaryVisMul(), INT2FIXED(lightscalexmul));
}

//
// R_WallColormapLevel
//
// Returns the colormap level for the current wall column's light.
//
inline int R_WallColormapLevel()
{
	if (wall_foglight)
		return std::clamp((wall_fogshade - FixedMul(rw_light, wall_fogvismul)) >> FRACBITS,
		             0, NUMCOLORMAPS - 1);

	const int index = std::clamp(rw_light >> LIGHTSCALESHIFT, 0, MAXLIGHTSCALE - 1);
	return walllights[index];
}

} // namespace

//
// R_TexScaleX
//
// Scales a value by the horizontal scaling value for texnum
//
static inline fixed_t R_TexScaleX(fixed_t x, int texnum)
{
	return FixedMul(x, texturescalex[texnum]);
}

//
// R_TexScaleY
//
// Scales a value by the vertical scaling value for texnum
//
static inline fixed_t R_TexScaleY(fixed_t y, int texnum)
{
	return FixedMul(y, texturescaley[texnum]);
}

//
// R_TexInvScaleX
//
// Scales a value by the inverse of the horizontal scaling value for texnum
//
//static inline fixed_t R_TexInvScaleX(fixed_t x, int texnum) // unused
//{
//	return FixedDiv(x, texturescalex[texnum]);
//}

//
// R_TexInvScaleY
//
// Scales a value by the inverse of the vertical scaling value for texnum
//
static inline fixed_t R_TexInvScaleY(fixed_t y, int texnum)
{
	return FixedDiv(y, texturescaley[texnum]);
}

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
	fixed_t val1, fixed_t val2,
	float scale1, float scale2)
{
	if (start > stop)
		return;

	const float h1 = FIXED2FLOAT(val1 - viewz) * scale1;
	const float h2 = FIXED2FLOAT(val2 - viewz) * scale2;

	const float step = (h2 - h1) / (stop - start + 1);
	float frac = float(centery) - h1;

	for (int i = start; i <= stop; i++)
	{
		array[i] = std::clamp((int)frac, ceilingclipinitial[0], floorclipinitial[0]);
		frac -= step;
	}
}

//
// R_BlastMaskedSegColumn
//
static inline void R_BlastMaskedSegColumn(void (*drawfunc)())
{
	tallpost_t* post = dcol.post;

	// R_PrepWall uses floats to calculate scale1 and scale2, which left
	// the scalestep values vulnerable to floating-point rounding errors.
	// If a wall is tall enough and a resolution big enough, the scalestep
	// can be off enough that by accumulation, it draws a row with no data.
	// Your midtex gap! :)
	//
	// Instead we get spryscale from a value precalculated in R_StoreWallRange.
	spryscale = masked_midscales[dcol.x];

	if (post != NULL && spryscale > 0)
	{
		// R_FillWallHeightArray uses centery and so should we.
		// Otherwise we can have textures drawing at different
		// heights when mouselook is on.
		sprtopscreen = (centery << FRACBITS) - FixedMul(dcol.texturemid, spryscale);
		dcol.iscale = 0xffffffffu / (unsigned)spryscale;

		while (!post->end())
		{
			// calculate unclipped screen coordinates for post
			const int topscreen = sprtopscreen + spryscale * post->topdelta;

			dcol.yl = topscreen >> FRACBITS;
			dcol.yh = (topscreen + spryscale * post->length) >> FRACBITS;

			dcol.yl = MAX(dcol.yl, MAX(mceilingclip[dcol.x], 0));
			dcol.yh = MIN(dcol.yh, mfloorclip[dcol.x] - 1);

			dcol.texturefrac = dcol.texturemid - (post->topdelta << FRACBITS)
				+ (dcol.yl * dcol.iscale) - FixedMul((centery << FRACBITS) - FRACUNIT, dcol.iscale);

			if (dcol.texturefrac < 0)
			{
				const int cnt = R_PixelCeil(-dcol.texturefrac, dcol.iscale);
				dcol.yl += cnt;
				dcol.texturefrac += cnt * dcol.iscale;
			}

			const fixed_t endfrac = dcol.texturefrac + (dcol.yh - dcol.yl) * dcol.iscale;
			const fixed_t maxfrac = post->length << FRACBITS;

			if (endfrac >= maxfrac)
			{
				const int cnt = R_PixelCeil(endfrac - maxfrac + 1, dcol.iscale);
				dcol.yh -= cnt;
			}

			dcol.source = post->data();

			if (dcol.yl >= 0 && dcol.yh < viewheight && dcol.yl <= dcol.yh)
				drawfunc();

			post = post->next();
		}

		masked_midposts[dcol.x] = NULL;
	}
}

//
// R_BlastSolidSegColumn
//
static inline void R_BlastSolidSegColumn(void (*drawfunc)())
{
	if (wallscalex[dcol.x] <= 0)
		return;

	if (dcol.post->length != dcol.textureheight >> FRACBITS)
	{
		tallpost_t* srcpost = dcol.post;

		int destpostlen = 0;

		static byte* destpostraw[512];
		tallpost_t* destpost = (tallpost_t*) destpostraw;

		// a 512 pixel tall post can overflow destpostraw
		// because of the 4 byte header and 4 byte footer.
		// so rather than increase it to 513 and imply that we
		// handle 513px at a time, clamp it instead.
		const int maxcount = static_cast<int>(sizeof(destpostraw)) - 8;
		int count = MIN(dcol.textureheight >> FRACBITS, maxcount);

		destpost->topdelta = 0;

		while (destpostlen < count)
		{
			int remaining = count - destpostlen; // pixels remaining to be replenished

			if (srcpost->topdelta == destpostlen)
			{
				// clamp to remaining to ensure malformed post lengths
				// don't crash
				const int copylen = MIN<int>(srcpost->length, remaining);
				memcpy(destpost->data() + destpostlen, srcpost->data(), copylen);
				destpostlen += copylen;
			}
			else
			{
				int curmidtexdelta = abs((srcpost->end() ? remaining : srcpost->topdelta) - destpostlen);
				int translen = curmidtexdelta > remaining ? remaining : curmidtexdelta;
				memset(destpost->data() + destpostlen, 0,
				       translen);
				destpostlen += translen;
			}

			if (!srcpost->end() && !srcpost->next()->end() && destpostlen >= srcpost->topdelta + srcpost->length)
			{
				srcpost = srcpost->next();
			}

			destpost->length = destpostlen;
		}

		// finish the post up.
		destpost->next()->length = 0;
		destpost->next()->writeend();

		dcol.post = destpost;
	}

	dcol.iscale = FixedMul(0xffffffffu / unsigned(wallscalex[dcol.x]), wallscaley);
	dcol.source = dcol.post->data();
	dcol.texturefrac = dcol.texturemid + FixedMul((dcol.yl - centery + 1) << FRACBITS, dcol.iscale);

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

inline void R_ColumnSetup(int x, int* top, int* bottom, tallpost_t** posts, bool calc_light)
{
	if (calc_light)
	{
		dcol.colormap = basecolormap.with(R_WallColormapLevel());
	}

	dcol.yl = MAX(top[x], 0);
	dcol.yh = MIN(bottom[x], viewheight - 1);
	dcol.post = posts[x];
}


static inline int R_ColumnRangeMinimumHeight(int start, int stop, int* top)
{
	int minheight = viewheight - 1;
	for (int x = start; x <= stop; x++)
		minheight = MIN(minheight, top[x]);

	return MAX(minheight, 0);
}

static inline int R_ColumnRangeMaximumHeight(int start, int stop, int* bottom)
{
	int maxheight = 0;
	for (int x = start; x <= stop; x++)
		maxheight = MAX(maxheight, bottom[x]);

	return MIN(maxheight, viewheight - 1);
}

//
// R_RenderColumnRange
//
// Renders a range of columns to the screen.
// If r_columnmethod is enabled, the columns are renderd using a temporary
// buffer to write the columns horizontally and then blit to the screen.
// Writing columns horizontally utilizes the cache much better than writing
// columns vertically to the screen buffer.
//
// [RH] This is a cache optimized version of R_RenderSegLoop(). It first
//		draws columns into a temporary buffer with a pitch of 4 and then
//		copies them to the framebuffer using a bunch of byte, word, and
//		longword moves. This may seem like a lot of extra work just to
//		draw columns to the screen (and it is), but it's actually faster
//		than drawing them directly to the screen like R_RenderSegLoop1().
//		On a Pentium II 300, using this code with rendering functions in
//		C is about twice as fast as using R_RenderSegLoop1() with an
//		assembly rendering function.
//
void R_RenderColumnRange(int start, int stop, int* top, int* bottom,
		tallpost_t** posts, void (*colblast)(), bool calc_light, int columnmethod)
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
		for (dcol.x = start; dcol.x <= stop; dcol.x++)
		{
			R_ColumnSetup(dcol.x, top, bottom, posts, calc_light);
			colblast();
			rw_light += rw_lightstep;
		}
	}
	else if (columnmethod == 2)
	{
		#define BLOCKBITS 6
		#define BLOCKSIZE (1 << BLOCKBITS)
		#define BLOCKMASK (BLOCKSIZE - 1)

		// pre-calculate the color map number for lighting for each screen column
		static int light_lookup[MAXWIDTH];
		if (calc_light)
		{
			for (int x = start; x <= stop; x++)
			{
				light_lookup[x] = R_WallColormapLevel();
				rw_light += rw_lightstep;
			}
		}

		// [SL] Render the range of columns in 64x64 pixel blocks, aligned to a grid
		// on the screen. This is to make better use of spatial locality in the cache.
		for (int bx = start; bx <= stop; bx = (bx & ~BLOCKMASK) + BLOCKSIZE)
		{
			const int blockstartx = bx;
			const int blockstopx = MIN((bx & ~BLOCKMASK) + BLOCKSIZE - 1, stop);

			const int miny = R_ColumnRangeMinimumHeight(blockstartx, blockstopx, top);
			const int maxy = R_ColumnRangeMaximumHeight(blockstartx, blockstopx, bottom);

			for (int by = miny; by <= maxy; by = (by & ~BLOCKMASK) + BLOCKSIZE)
			{
				const int blockstarty = by;
				const int blockstopy = (by & ~BLOCKMASK) + BLOCKSIZE - 1;

				for (int x = blockstartx; x <= blockstopx; x++)
				{
					if (calc_light)
						dcol.colormap = basecolormap.with(light_lookup[x]);

					dcol.x = x;
					dcol.yl = MAX(top[x], blockstarty);
					dcol.yh = MIN(bottom[x], blockstopy);
					dcol.post = posts[x];
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

	static constexpr int columnmethod = 2;

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

		wallscaley = texturescaley[midtexture];
		dcol.textureheight = textureheight[midtexture];
		dcol.texturemid = R_TexScaleY(rw_midtexturemid, midtexture) + sidedef->rowoffset;

		R_RenderColumnRange(start, stop, walltopf, lower, midposts,
					SolidColumnBlaster, true, columnmethod);

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

			wallscaley = texturescaley[toptexture];
			dcol.textureheight = textureheight[toptexture];
			dcol.texturemid = R_TexScaleY(rw_toptexturemid, toptexture) + sidedef->rowoffset;

			R_RenderColumnRange(start, stop, walltopf, lower, topposts,
						SolidColumnBlaster, true, columnmethod);

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

			wallscaley = texturescaley[bottomtexture];
			dcol.textureheight = textureheight[bottomtexture];
			dcol.texturemid = R_TexScaleY(rw_bottomtexturemid, bottomtexture) + sidedef->rowoffset;

			R_RenderColumnRange(start, stop, wallbottomb, lower, bottomposts,
						SolidColumnBlaster, true, columnmethod);

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
				const int colnum = (R_TexScaleX(texoffs[x], maskedtexture) + curline->sidedef->textureoffset) >> FRACBITS;
				masked_midposts[x] = R_GetTextureColumn(maskedtexture, colnum);
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


// ============================================================================
//
// Fog boundary rendering
//
// When two adjacent sectors have different fog colors, the fog in the
// nearer sector needs to be drawn over the opening between them, or the fog
// will appear to stop at untextured two-sided lines. This is done by
// re-shading the framebuffer pixels inside the opening with the front
// sector's fog colormap, using the same distance-based light the wall
// tiers of this seg were drawn with.
//
// Adapted from ZDoom 1.23's R_DrawFogBoundary.
//
// ============================================================================

namespace
{

std::array<int, MAXHEIGHT> fogboundary_spanend;
// NOLINTNEXTLINE(bugprone-throwing-static-initialization) - default constructor cannot throw
shaderef_t fogboundary_colormap;
int fogboundary_level;

// current fog parameters, from the front sector's dynamic colormap
// NOLINTBEGIN(bugprone-throwing-static-initialization) - default constructor cannot throw
argb_t fogboundary_fade;
argb_t fogboundary_lightcolor;
// NOLINTEND(bugprone-throwing-static-initialization)

// precomputed 32bpp blend factors for the current colormap level
int fogboundary_fogmul;
int fogboundary_fogaddr, fogboundary_fogaddg, fogboundary_fogaddb;

//
// R_SetFogBoundaryColormap
//
// Selects the colormap level used to shade the fog boundary and precomputes
// the blending constants for direct (32bpp) rendering.
//
void R_SetFogBoundaryColormap(int level)
{
	fogboundary_level = level;
	fogboundary_colormap = basecolormap.with(level);

	if (I_GetPrimarySurface()->getBitsPerPixel() != 8)
	{
		// mirror BuildColoredLights: lerp toward the fade color by
		// level/NUMCOLORMAPS, then scale by the light color
		const argb_t fade = V_GammaCorrect(fogboundary_fade);
		fogboundary_fogmul = NUMCOLORMAPS - level;
		fogboundary_fogaddr = (fade.getr() * level) + (NUMCOLORMAPS / 2);
		fogboundary_fogaddg = (fade.getg() * level) + (NUMCOLORMAPS / 2);
		fogboundary_fogaddb = (fade.getb() * level) + (NUMCOLORMAPS / 2);
	}
}

//
// R_FogBoundaryRow
//
// Re-shades the framebuffer pixels in the given row from x1 to x2 (inclusive)
// with the current fog boundary colormap.
//
// the start of framebuffer row y, as pixels of type T
template <typename T>
T* R_FogBoundaryDest(int y)
{
	return nonstd::bit_cast<T*>(dcol.destination) +
	       (static_cast<ptrdiff_t>(y) * dcol.pitch_in_pixels);
}

void R_FogBoundaryRow(int y, int x1, int x2)
{
	if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
	{
		auto* dest = R_FogBoundaryDest<palindex_t>(y);
		for (int x = x1; x <= x2; x++)
			dest[x] = fogboundary_colormap.index(dest[x]);
	}
	else
	{
		auto* dest = R_FogBoundaryDest<argb_t>(y);
		const int lr = fogboundary_lightcolor.getr();
		const int lg = fogboundary_lightcolor.getg();
		const int lb = fogboundary_lightcolor.getb();
		const bool whitelight = (lr & lg & lb) == 255;

		for (int x = x1; x <= x2; x++)
		{
			const argb_t c = dest[x];
			int r = ((c.getr() * fogboundary_fogmul) + fogboundary_fogaddr) / NUMCOLORMAPS;
			int g = ((c.getg() * fogboundary_fogmul) + fogboundary_fogaddg) / NUMCOLORMAPS;
			int b = ((c.getb() * fogboundary_fogmul) + fogboundary_fogaddb) / NUMCOLORMAPS;

			if (!whitelight)
			{
				r = r * lr / 255;
				g = g * lg / 255;
				b = b * lb / 255;
			}

			dest[x] = argb_t(255, r, g, b);
		}
	}
}

void R_DrawFogBoundarySection(int y, int y2, int x1)
{
	for (; y < y2; y++)
		R_FogBoundaryRow(y, x1, fogboundary_spanend[y]);
}

// current shade for the fog boundary in ZDoom's light units, set by
// R_RenderFogBoundary; colormap level = (shade - vis) >> FRACBITS
fixed_t fogboundary_shade;

} // namespace

//
// R_FogLight2Shade
//
// ZDoom's LIGHT2SHADE: converts a sector light level (0-255) into a
// shade value that a distance-based visibility term is subtracted from.
// Unlike the vanilla scalelight tables this is linear and uses the light
// level at full resolution, which is what ZDoom shades its fog with.
//
fixed_t R_FogLight2Shade(int lightlevel)
{
	constexpr int LIGHT_BIAS = 12;		// ZDoom offsets the light level by this
	constexpr int LIGHT_RANGE = 128;	// light levels per NUMCOLORMAPS shades
	return (NUMCOLORMAPS * 2 * FRACUNIT) -
	       ((lightlevel + LIGHT_BIAS) * (FRACUNIT * NUMCOLORMAPS / LIGHT_RANGE));
}

namespace
{

//
// R_FogBoundaryVisMul
//
// The factor that converts a wall column's scale (as computed by
// R_PrepWall) into ZDoom's per-column visibility.
// 
// This is ZDoom's r_WallVisibility (with the default vis of 8.0)
// divided by InvZtoScale, since Odamex's wall scale already is
// InvZtoScale / z.
//
fixed_t R_FogBoundaryVisMul()
{
	constexpr int64_t BASE_HEIGHT = 200;	// the 320x200 view the constants assume
	const IWindowSurface* surface = I_GetPrimarySurface();
	const int64_t sw = surface->getWidth();
	const int64_t sh = surface->getHeight();

	// the tangent of the 4:3 field of view
	// because it's what zdoom does
	const fixed_t basetan = finetangent[(FINEANGLES / 4) + (FieldOfView / 2)];
	const fixed_t vis = FixedMul(8 << FRACBITS, basetan);
	return static_cast<fixed_t>(vis * (sw * BASE_HEIGHT) / (viewwidth * sh));
}

inline int R_FogBoundaryLightLevel(fixed_t vis)
{
	if (fixedlightlev)
		return fixedlightlev;

	return std::clamp((fogboundary_shade - vis) >> FRACBITS, 0, NUMCOLORMAPS - 1);
}

//
// R_DrawFogBoundary
//
// This is essentially the same as R_MapVisPlane but with an extra step
// to create new horizontal spans whenever the light changes enough that
// we need to use a new colormap.
//
void R_DrawFogBoundary(int x1, int x2, const int* uclip, const int* dclip)
{
	fixed_t light = rw_light + (rw_lightstep * (x2 - x1));
	int x = x2;
	int t2 = std::clamp(uclip[x], 0, viewheight);
	int b2 = std::clamp(dclip[x], 0, viewheight);
	int rcolormap = R_FogBoundaryLightLevel(light);
	int lcolormap;

	for (int y = t2; y < b2; y++)
		fogboundary_spanend[y] = x;

	R_SetFogBoundaryColormap(rcolormap);

	for (--x; x >= x1; --x)
	{
		int t1 = std::clamp(uclip[x], 0, viewheight);
		int b1 = std::clamp(dclip[x], 0, viewheight);
		const int xr = x + 1;
		int stop;

		light -= rw_lightstep;
		lcolormap = R_FogBoundaryLightLevel(light);
		if (lcolormap != rcolormap)
		{
			if (t2 < b2 && rcolormap != 0)
			{
				// Colormap 0 is always the identity map, so rendering
				// it is just a waste of time.
				R_DrawFogBoundarySection(t2, b2, xr);
			}
			t2 = std::min(t2, t1);
			b2 = std::max(b2, b1);
			for (int y = t2; y < b2; y++)
				fogboundary_spanend[y] = x;

			rcolormap = lcolormap;
			R_SetFogBoundaryColormap(rcolormap);
		}
		else
		{
			if (rcolormap != 0)
			{
				stop = MIN(t1, b2);
				while (t2 < stop)
				{
					R_FogBoundaryRow(t2, xr, fogboundary_spanend[t2]);
					t2++;
				}
				stop = MAX(b1, t2);
				while (b2 > stop)
				{
					--b2;
					R_FogBoundaryRow(b2, xr, fogboundary_spanend[b2]);
				}
			}
			else
			{
				t2 = MAX(t2, MIN(t1, b2));
				b2 = MIN(b2, MAX(b1, t2));
			}

			stop = MIN(t2, b1);
			while (t1 < stop)
				fogboundary_spanend[t1++] = x;
			stop = MAX(b2, t2);
			while (b1 > stop)
				fogboundary_spanend[--b1] = x;
		}

		t2 = std::clamp(uclip[x], 0, viewheight);
		b2 = std::clamp(dclip[x], 0, viewheight);
	}

	if (t2 < b2 && rcolormap != 0)
		R_DrawFogBoundarySection(t2, b2, x1);
}

//
// R_RenderFogBoundary
//
// Sets up lighting and shades the opening of a fog boundary drawseg.
//
// the clip array the fog was drawn against, poisoned by the caller after
// the whole masked range (fog + midtexture) is finished
int* fog_used_topclip;

void R_RenderFogBoundary(drawseg_t* ds, int x1, int x2)
{
	// the fog would be re-shaded by a fixed colormap anyway (invulnerability),
	// so don't bother drawing it
	if (fixedcolormap.isValid())
		return;

	sector_t tempsec;	// killough 4/13/98

	fogboundary_fade = frontsector->colormap->fade;
	fogboundary_lightcolor = frontsector->colormap->color;
	basecolormap = frontsector->colormap->maps;	// [RH] Set basecolormap

	// The front sector is foggy by definition here, so don't apply
	// gun flash extralight or the fake contrast for orthogonal lines
	fogboundary_shade = R_FogLight2Shade(
			R_FakeFlat(frontsector, &tempsec, nullptr, nullptr, false)->lightlevel);

	// walk ZDoom's per-column visibility term instead of Odamex's
	// scalelight index -- it is linear in the wall column scale
	const fixed_t vismul = R_FogBoundaryVisMul();
	rw_lightstep = FixedMul(ds->scalestep, vismul);
	rw_light = FixedMul(ds->scale1, vismul) + ((x1 - ds->x1) * rw_lightstep);

	fog_used_topclip = ds->sprtopclip;

	R_DrawFogBoundary(x1, x2, ds->sprtopclip, ds->sprbottomclip);
}

//
// R_RenderMaskedTextureRange
//
// Renders the masked midtexture of a seg
//
void R_RenderMaskedTextureRange(drawseg_t* ds, int x1, int x2)
{
	sector_t	tempsec;		// killough 4/13/98

	dcol.color = (dcol.color + 4) & 0xFF;	// color if using r_drawflat

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

	const int texnum = texturetranslation[curline->sidedef->midtexture];
	const fixed_t texheight = R_TexScaleY(textureheight[texnum], texnum);

	// find texture positioning
	if (curline->linedef->flags & ML_DONTPEGBOTTOM)
		dcol.texturemid = MAX(P_FloorHeight(frontsector), P_FloorHeight(backsector)) + R_TexInvScaleY(textureheight[texnum], texnum);
	else
		dcol.texturemid = MIN(P_CeilingHeight(frontsector), P_CeilingHeight(backsector));

	dcol.texturemid = R_TexScaleY(dcol.texturemid - viewz, texnum) + curline->sidedef->rowoffset;

	const int64_t topscreenclip = int64_t(centery) << 2*FRACBITS;
	const int64_t botscreenclip = int64_t(centery - viewheight) << 2*FRACBITS;

	// top of texture entirely below screen?
	if (int64_t(dcol.texturemid) * ds->scale1 <= botscreenclip &&
		int64_t(dcol.texturemid) * ds->scale2 <= botscreenclip)
		return;

	// bottom of texture entirely above screen?
	if (int64_t(dcol.texturemid - texheight) * ds->scale1 > topscreenclip &&
		int64_t(dcol.texturemid - texheight) * ds->scale2 > topscreenclip)
		return;

	basecolormap = frontsector->colormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	const int masked_lightlevel =
		R_FakeFlat(frontsector, &tempsec, nullptr, nullptr, false)->lightlevel;

	int lightnum = (masked_lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);

	lightnum += R_OrthogonalLightnumAdjustment();

	walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
		lightnum <  0 ? scalelight[0] : scalelight[lightnum];

	// foggy sectors shade their masked textures with ZDoom's fog curve
	R_SetWallFogLight(frontsector, masked_lightlevel);

	masked_midposts = ds->midposts;
	masked_midscales = ds->midscales;

	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	dcol.textureheight = 0;

	// draw the columns
	// TODO: change negonearray to the actual top/bottom
	R_RenderColumnRange(x1, x2, negonearray, viewheightarray, ds->midposts,
			MaskedColumnBlaster, true, 0);
}

} // namespace

//
// R_RenderMaskedSegRange
//
// Renders a masked seg: first any fog boundary the seg forms, then its
// masked midtexture (either may be absent).
//
void R_RenderMaskedSegRange(drawseg_t* ds, int x1, int x2)
{
	// the psprite pass expects basecolormap to still refer to the view's
	// sector after the masked pass, so restore it when done
	const shaderef_t saved_basecolormap = basecolormap;

	curline = ds->curline;
	frontsector = curline->frontsector;
	backsector = curline->backsector;

	// Draw fog partition
	fog_used_topclip = nullptr;
	if (ds->fogboundary)
		R_RenderFogBoundary(ds, x1, x2);

	if (ds->midposts)
		R_RenderMaskedTextureRange(ds, x1, x2);

	if (fog_used_topclip)
	{
		// mark these columns as done so the fog isn't blended in a
		// second time when this drawseg is revisited
		for (int x = x1; x <= x2; x++)
			fog_used_topclip[x] = viewheight;
	}

	basecolormap = saved_basecolormap;
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
void R_PrepWall(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2, fixed_t dist1, fixed_t dist2, int start, int stop)
{
	const int width = stop - start + 1;
	if (width <= 0)
		return;

	const int toptexture = texturetranslation[curline->sidedef->toptexture];
	const int midtexture = texturetranslation[curline->sidedef->midtexture];
	const int bottomtexture = texturetranslation[curline->sidedef->bottomtexture];

	// clipped lineseg length
	const fixed_t seglen = R_LineLength(px1, py1, px2, py2);

	// distance from lineseg start to start of clipped lineseg
	const fixed_t segoffs = curline->offset + R_LineLength(curline->v1->x, curline->v1->y, px1, py1);

	const fixed_t mindist = NEARCLIP;
	static constexpr fixed_t maxdist = 16384*FRACUNIT;
	dist1 = std::clamp(dist1, mindist, maxdist);
	dist2 = std::clamp(dist2, mindist, maxdist);

	// calculate texture coordinates at the line's endpoints
	const float scale1 = yfoc / FIXED2FLOAT(dist1);
	const float scale2 = yfoc / FIXED2FLOAT(dist2);

	// [SL] Quick note on texture mapping: we can not linearly interpolate along the length of the seg
	// as it will yield evenly spaced texels instead of correct perspective (taking depth Z into account).
	// We also can not linearly interpolate Z, but we can linearly interpolate 1/Z (scale), so we linearly
	// interpolate the texture coordinates u / Z and then divide by 1/Z to get the correct u for each column.

	const float scalestep = (scale2 - scale1) / width;
	const float uinvzstep = FIXED2FLOAT(seglen) * scale2 / width;

	// determine which texture posts will be used for each screen
	// column in this range and calculate the scaling factor for
	// each column.

	fixed_t textureoffset = curline->sidedef->textureoffset;

	float uinvz = 0.0f;
	float curscale = scale1;
	for (int i = start; i <= stop; i++)
	{
		wallscalex[i] = FLOAT2FIXED(curscale);

		const fixed_t colfrac = segoffs + FLOAT2FIXED(uinvz / curscale);
		texoffs[i] = colfrac;

		if (toptexture)
		{
			const int colnum = (R_TexScaleX(colfrac, toptexture) + textureoffset) >> FRACBITS;
			topposts[i] = R_GetTextureColumn(toptexture, colnum);
		}
		if (midtexture)
		{
			const int colnum = (R_TexScaleX(colfrac, midtexture) + textureoffset) >> FRACBITS;
			midposts[i] = R_GetTextureColumn(midtexture, colnum);
		}
		if (bottomtexture)
		{
			const int colnum = (R_TexScaleX(colfrac, bottomtexture) + textureoffset) >> FRACBITS;
			bottomposts[i] = R_GetTextureColumn(bottomtexture, colnum);
		}

		uinvz += uinvzstep;
		curscale += scalestep;
	}

	// get the z coordinates of the line's vertices on each side of the line
	rw_frontcz1 = P_CeilingHeight(px1, py1, frontsector);
	rw_frontfz1 = P_FloorHeight(px1, py1, frontsector);
	rw_frontcz2 = P_CeilingHeight(px2, py2, frontsector);
	rw_frontfz2 = P_FloorHeight(px2, py2, frontsector);

	// calculate the upper and lower heights of the walls in the front
	R_FillWallHeightArray(walltopf, start, stop, rw_frontcz1, rw_frontcz2, scale1, scale2);
	R_FillWallHeightArray(wallbottomf, start, stop, rw_frontfz1, rw_frontfz2, scale1, scale2);

	rw_hashigh = rw_haslow = false;

	if (backsector)
	{
		rw_backcz1 = P_CeilingHeight(px1, py1, backsector);
		rw_backfz1 = P_FloorHeight(px1, py1, backsector);
		rw_backcz2 = P_CeilingHeight(px2, py2, backsector);
		rw_backfz2 = P_FloorHeight(px2, py2, backsector);

		// calculate the upper and lower heights of the walls in the back
		R_FillWallHeightArray(walltopb, start, stop, rw_backcz1, rw_backcz2, scale1, scale2);
		R_FillWallHeightArray(wallbottomb, start, stop, rw_backfz1, rw_backfz2, scale1, scale2);

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
		if (R_IsSkyFlat(frontsector->ceilingpic) && R_IsSkyFlat(backsector->ceilingpic))
			memcpy(walltopf+start, walltopb+start, width*sizeof(*walltopb));
	}

	rw_scalestep = FLOAT2FIXED(scalestep);
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
	midtexture = toptexture = bottomtexture = maskedtexture = 0;
	ds_p->midposts = NULL;
	ds_p->midscales = NULL;
	ds_p->fogboundary = false;

	if (!backsector)
	{
		// single sided line
		midtexture = texturetranslation[sidedef->midtexture];

		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = true;

		if (linedef->flags & ML_DONTPEGBOTTOM)
		{
			// bottom of texture at bottom
			const fixed_t texheight = R_TexInvScaleY(textureheight[midtexture], midtexture);
			rw_midtexturemid = P_FloorHeight(frontsector) - viewz + texheight;
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
				|| backsector->floorpic != frontsector->floorpic

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
				!R_IsSkyFlat(frontsector->ceilingpic) || !R_IsSkyFlat(backsector->ceilingpic);

			markceiling =
				  (ceilingskyhack &&
				   !P_IdenticalPlanes(&backsector->ceilingplane, &frontsector->ceilingplane))
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->ceilingpic != frontsector->ceilingpic

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->ceiling_xoffs != frontsector->ceiling_xoffs
				|| (backsector->ceiling_yoffs + backsector->base_ceiling_yoffs) !=
				   (frontsector->ceiling_yoffs + frontsector->base_ceiling_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through fake ceilings
				|| (frontsector->heightsec && !R_IsSkyFlat(frontsector->ceilingpic))

				// killough 4/17/98: draw ceilings if different light levels
				|| backsector->ceilinglightsec != frontsector->ceilinglightsec

				// [RH] Add check for colormaps
				|| backsector->colormap != frontsector->colormap

				|| backsector->ceiling_xscale != frontsector->ceiling_xscale
				|| backsector->ceiling_yscale != frontsector->ceiling_yscale

				|| (backsector->ceiling_angle + backsector->base_ceiling_angle) !=
				   (frontsector->ceiling_angle + frontsector->base_ceiling_angle)
				;
		}


		if (rw_hashigh)
		{
			// top texture
			toptexture = texturetranslation[sidedef->toptexture];
			if (linedef->flags & ML_DONTPEGTOP)
			{
				// top of texture at top
				const fixed_t fc = P_CeilingHeight(frontsector);
				rw_toptexturemid = fc - viewz;
			}
			else
			{
				// bottom of texture
				const fixed_t texheight = R_TexInvScaleY(textureheight[toptexture], toptexture);
				rw_toptexturemid = P_CeilingHeight(backsector) - viewz + texheight;
			}
		}

		if (rw_haslow)
		{
			// bottom texture
			bottomtexture = texturetranslation[sidedef->bottomtexture];

			if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom, top of texture at top
				const fixed_t fc = P_CeilingHeight(frontsector);
				rw_bottomtexturemid = fc - viewz;
			}
			else
			{
				// top of texture at top
				const fixed_t bf = P_FloorHeight(backsector);
				rw_bottomtexturemid = bf - viewz;
			}
		}

		// mark segs between fogs of different density so the fog can
		// be drawn over the opening during the masked pass (a closed door
		// already draws its wall tiers with the front sector's fog)
		if (!doorclosed)
			ds_p->fogboundary = R_IsFogBoundary(frontsector, backsector);

		// allocate space for masked texture tables
		if (sidedef->midtexture)
		{
			// masked midtexture
			maskedtexture = texturetranslation[sidedef->midtexture];
			ds_p->midposts = masked_midposts = masked_midposts_pool.alloc(count) - start;

			// save the per-column scales, pre-scaled into the
			// midtexture's y-scale space, for the masked pass
			fixed_t* midscales = midscales_pool.alloc(count) - start;
			if (texturescaley[maskedtexture] == FRACUNIT)
			{
				memcpy(midscales + start, wallscalex + start, count * sizeof(*midscales));
			}
			else
			{
				for (int x = start; x <= stop; x++)
					midscales[x] = R_TexInvScaleY(wallscalex[x], maskedtexture);
			}
			ds_p->midscales = midscales;
		}

		// [SL] additional fix for sky hack
		if ((R_IsSkyFlat(frontsector->ceilingpic) && R_IsSkyFlat(backsector->ceilingpic)))
			toptexture = 0;
	}

	// [SL] 2012-01-24 - Horizon line extends to infinity by scaling the wall
	// height to 0

	if (curline->is_horizon)
	{
		rw_scale = ds_p->scale1 = ds_p->scale2 = rw_scalestep = ds_p->light = rw_light = 0;
		midtexture = toptexture = bottomtexture = maskedtexture = 0;
		ds_p->fogboundary = false;

		for (int n = start; n <= stop; n++)
			walltopf[n] = wallbottomf[n] = centery;
	}

	segtextured = (midtexture | toptexture) | (bottomtexture | maskedtexture);

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

			lightnum = std::clamp(lightnum, 0, LIGHTLEVELS - 1);
			walllights = scalelight[lightnum];
		}

		// foggy sectors shade their walls with ZDoom's fog curve
		R_SetWallFogLight(frontsector, frontsector->lightlevel);
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
		if (P_CeilingHeight(viewx, viewy, frontsector) <= viewz &&
			!R_IsSkyFlat(frontsector->ceilingpic))
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
	if (maskedtexture || ds_p->fogboundary || (backsector && didsolidcol))
		ds_p->silhouette = SIL_BOTH;

    // save sprite clipping info
	if ((ds_p->silhouette & SIL_TOP) && ds_p->sprtopclip == NULL)
	{
		ds_p->sprtopclip = sprclip_pool.alloc(count) - start;
		memcpy(ds_p->sprtopclip + start, &ceilingclip[start], count * sizeof(*ds_p->sprtopclip));
	}

	if ((ds_p->silhouette & SIL_BOTTOM) && ds_p->sprbottomclip == NULL)
	{
		ds_p->sprbottomclip = sprclip_pool.alloc(count) - start;
		memcpy(ds_p->sprbottomclip + start, &floorclip[start], count * sizeof(*ds_p->sprbottomclip));
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
