// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2012 by The Odamex Team.
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



#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "m_alloc.h"

#include "i_system.h"

#include "doomdef.h"
#include "doomstat.h"

#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "v_video.h"

#include "vectors.h"
#include <math.h>

#include "p_lnspec.h"

// OPTIMIZE: closed two sided lines as single sided

// killough 1/6/98: replaced globals with statics where appropriate

static BOOL		segtextured;	// True if any of the segs textures might be visible.
static BOOL		markfloor;		// False if the back side is the same plane.
static BOOL		markceiling;
static BOOL		maskedtexture;
static int		toptexture;
static int		bottomtexture;
static int		midtexture;

int*			walllights;

//
// regular wall
//
fixed_t			rw_light;		// [RH] Use different scaling for lights
fixed_t			rw_lightstep;

static int		rw_x;
static int		rw_stopx;
static fixed_t	rw_scale;
static fixed_t	rw_scalestep;
static fixed_t	rw_midtexturemid;
static fixed_t	rw_toptexturemid;
static fixed_t	rw_bottomtexturemid;

extern fixed_t	rw_frontcz1, rw_frontcz2;
extern fixed_t	rw_frontfz1, rw_frontfz2;
extern fixed_t	rw_backcz1, rw_backcz2;
extern fixed_t	rw_backfz1, rw_backfz2;

static int walltopf[MAXWIDTH];
static int walltopb[MAXWIDTH];
static int wallbottomf[MAXWIDTH];
static int wallbottomb[MAXWIDTH];
static int masktop[MAXWIDTH];
static int maskbottom[MAXWIDTH];

static fixed_t wallscalex[MAXWIDTH];
static int texoffs[MAXWIDTH];

extern fixed_t FocalLengthY;
extern double yfoc;

static int  	*maskedtexturecol;

void (*R_RenderSegLoop)(void);
void R_ColumnToPointOnSeg(int column, line_t *line, fixed_t &x, fixed_t &y);

//
// R_ScaledTextureHeight
//
// Returns the texture height after y-scaling has been applied
//
static fixed_t R_ScaledTextureHeight(int texnum)
{
	return FixedDiv(textureheight[texnum], texturescaley[texnum]);
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
	double scale1, double scale2)
{
	double h1 = FIXED2DOUBLE(val1 - viewz) * scale1;
	double h2 = FIXED2DOUBLE(val2 - viewz) * scale2;
	
	double step = 0.0;
	if (stop > start)
		step = (h2 - h1) / (stop - start + 1);

	double frac = double(centery) - h1;

	for (int i = start; i <= stop; i++)
	{
		array[i] = clamp((int)(frac + 0.5), -1, screen->height);
		frac -= step;
	}
}

//
// BlastMaskedColumn
//
static void BlastMaskedColumn (void (*blastfunc)(tallpost_t *post), int texnum)
{
	if (maskedtexturecol[dc_x] != MAXINT && spryscale > 0)
	{
		// calculate lighting
		if (!fixedcolormap)
		{
			unsigned index = rw_light >> LIGHTSCALESHIFT;	// [RH]

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE-1;

			dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
		}

		if (maskbottom[dc_x] >= 0 && masktop[dc_x] < viewheight)
		{
			sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
			dc_iscale = 0xffffffffu / (unsigned)spryscale;

			// killough 1/25/98: here's where Medusa came in, because
			// it implicitly assumed that the column was all one patch.
			// Originally, Doom did not construct complete columns for
			// multipatched textures, so there were no header or trailer
			// bytes in the column referred to below, which explains
			// the Medusa effect. The fix is to construct true columns
			// when forming multipatched textures (see r_data.c).

			// draw the texture

			blastfunc (R_GetColumn(texnum, maskedtexturecol[dc_x]));
			maskedtexturecol[dc_x] = MAXINT;
		}
	}
	spryscale += rw_scalestep;
	rw_light += rw_lightstep;
}

//
// R_RenderMaskedSegRange
//
void
R_RenderMaskedSegRange
( drawseg_t*	ds,
  int		x1,
  int		x2 )
{
	int 		lightnum;
	int 		texnum;
	sector_t	tempsec;		// killough 4/13/98

	dc_color = (dc_color + 4) & 0xFF;	// color if using r_drawflat

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
		dc_translevel = curline->linedef->lucency << 8;
	}
	else
	{
		R_ResetDrawFuncs();
	}

	frontsector = curline->frontsector;
	backsector = curline->backsector;

	texnum = texturetranslation[curline->sidedef->midtexture];

	// find texture positioning
	fixed_t top, bottom;
	if (curline->linedef->flags & ML_DONTPEGBOTTOM)
		top = MAX<fixed_t>(P_FloorHeight(frontsector), P_FloorHeight(backsector)) + R_ScaledTextureHeight(texnum);
	else
		top = MIN<fixed_t>(P_CeilingHeight(frontsector), P_CeilingHeight(backsector));
	bottom = top - R_ScaledTextureHeight(texnum);

	dc_texturemid = FixedMul(top - viewz + curline->sidedef->rowoffset, texturescaley[texnum]);
	
	// project the top and bottom of the texture along the length of the seg
	double fscale1 = FIXED2DOUBLE(ds->scale1);
	double fscale2 = FIXED2DOUBLE(ds->scale2);
	R_FillWallHeightArray(masktop, ds->x1, ds->x2, top, top, fscale1, fscale2);
	R_FillWallHeightArray(maskbottom, ds->x1, ds->x2, bottom, bottom, fscale1, fscale2);	

	// bottom of texture entirely above screen?
	if (maskbottom[x1] < 0 && maskbottom[x2] < 0)
		return;
	// top of texture entirely below screen?
	if (masktop[x1] >= viewheight && masktop[x2] >= viewheight)
		return;

	basecolormap = frontsector->floorcolormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	lightnum = (R_FakeFlat(frontsector, &tempsec, NULL, NULL, false)
			->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);

	lightnum += R_OrthogonalLightnumAdjustment();

	walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
		lightnum <  0 ? scalelight[0] : scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;

	// [SL] 2013-01-02 - Adjust the scaling variables for the texture
	// y-scaling factor
	rw_scalestep = FixedDiv(ds->scalestep, texturescaley[texnum]);
	spryscale = FixedDiv(ds->scale1, texturescaley[texnum]) + (x1 - ds->x1) * rw_scalestep;

	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;
	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	if (fixedlightlev)
		dc_colormap = basecolormap + fixedlightlev;
	else if (fixedcolormap)
		dc_colormap = fixedcolormap;

	// draw the columns
	if (!r_columnmethod)
	{
		for (dc_x = x1; dc_x <= x2; dc_x++)
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
	}
	else
	{
		// [RH] Draw them through the temporary buffer
		int stop = (++x2) & ~3;

		if (x1 >= x2)
			return;

		dc_x = x1;

		if (dc_x & 1) {
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
			dc_x++;
		}

		if (dc_x & 2) {
			if (dc_x < x2 - 1) {
				rt_initcols();
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
				dc_x++;
				BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
				rt_draw2cols ((dc_x - 1) & 3, dc_x - 1);
				dc_x++;
			} else if (dc_x == x2 - 1) {
				BlastMaskedColumn (R_DrawMaskedColumn, texnum);
				dc_x++;
			}
		}

		while (dc_x < stop) {
			rt_initcols();
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			rt_draw4cols (dc_x - 3);
			dc_x++;
		}

		if (x2 - dc_x == 1) {
			BlastMaskedColumn (R_DrawMaskedColumn, texnum);
		} else if (x2 - dc_x >= 2) {
			rt_initcols();
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			dc_x++;
			BlastMaskedColumn (R_DrawMaskedColumnHoriz, texnum);
			rt_draw2cols ((dc_x - 1) & 3, dc_x - 1);
			if (++dc_x < x2) {
				BlastMaskedColumn (R_DrawMaskedColumn, texnum);
			}
		}
	}
}

//
// R_SetTextureParams
//
// Sets dc_source, dc_texturefrac
//
static void R_SetTextureParams(int texnum, fixed_t texcol, fixed_t mid)
{
	const fixed_t scalex = texturescalex[texnum];
	const fixed_t scaley = texturescaley[texnum];

	dc_texturefrac = FixedMul(scaley, mid + FixedMul((dc_yl - centery + 1) << FRACBITS, dc_iscale));

	dc_source = R_GetColumnData(texnum, FIXED2INT(FixedMul(scalex, texcol)));
}

//
// R_RenderSegLoop
// Draws zero, one, or two textures (and possibly a masked
//	texture) for walls.
// Can draw or mark the starting pixel of floor and ceiling
//	textures.
// CALLED: CORE LOOPING ROUTINE.
//

static void BlastColumn (void (*blastfunc)())
{
	rw_scale = wallscalex[rw_x];

	// mark floor / ceiling areas
	walltopf[rw_x] = MAX(walltopf[rw_x], ceilingclip[rw_x]);

	if (markceiling)
	{
		int top = ceilingclip[rw_x];
		int bottom = MIN(walltopf[rw_x], floorclip[rw_x]);

		if (top < bottom)
		{
			ceilingplane->top[rw_x] = top;
			ceilingplane->bottom[rw_x] = bottom - 1;
		}
	}

	wallbottomf[rw_x] = MIN(wallbottomf[rw_x], floorclip[rw_x]);

	if (markfloor)
	{
		int top = MAX(wallbottomf[rw_x], ceilingclip[rw_x]);
		int bottom = floorclip[rw_x];

		if (top < bottom)
		{
			floorplane->top[rw_x] = top;
			floorplane->bottom[rw_x] = bottom - 1;
		}
	}

	fixed_t texturecolumn = texoffs[rw_x];

	if (rw_scale > 0)
		dc_iscale = 0xffffffffu / (unsigned)rw_scale;

	// draw the wall tiers
	if (midtexture)
	{
		// single sided line
		dc_yl = walltopf[rw_x];
		dc_yh = wallbottomf[rw_x] - 1;

		R_SetTextureParams(midtexture, texturecolumn, rw_midtexturemid);

		blastfunc ();
		ceilingclip[rw_x] = viewheight - 1;
		floorclip[rw_x] = 0;
	}
	else
	{
		// two sided line
		if (toptexture)
		{
			walltopb[rw_x] = MAX(MIN(walltopb[rw_x], floorclip[rw_x]), walltopf[rw_x]);
			dc_yl = walltopf[rw_x];
			dc_yh = walltopb[rw_x] - 1;

			R_SetTextureParams(toptexture, texturecolumn, rw_toptexturemid);
			blastfunc();

			ceilingclip[rw_x] = walltopb[rw_x];		
		}
		else if (markceiling)
		{
			// no top wall
			ceilingclip[rw_x] = walltopf[rw_x];
		}

		if (bottomtexture)
		{
			wallbottomb[rw_x] = MIN(MAX(wallbottomb[rw_x], ceilingclip[rw_x]), wallbottomf[rw_x]);
			dc_yl = wallbottomb[rw_x];
			dc_yh = wallbottomf[rw_x] - 1;

			R_SetTextureParams(bottomtexture, texturecolumn, rw_bottomtexturemid);
			blastfunc();

			floorclip[rw_x] = wallbottomb[rw_x];
		}
		else if (markfloor)	
		{
			// no bottom wall
			floorclip[rw_x] = wallbottomf[rw_x];
		}

		if (maskedtexture)
		{
			// save texturecol
			//	for backdrawing of masked mid texture
			maskedtexturecol[rw_x] = FixedMul(texturescalex[maskedtexture], texturecolumn) / FRACUNIT; 
		}
	}

	rw_light += rw_lightstep;
}


// [RH] This is DOOM's original R_RenderSegLoop() with most of the work
//		having been split off into a separate BlastColumn() function. It
//		just draws the columns straight to the frame buffer, and at least
//		on a Pentium II, can be slower than R_RenderSegLoop2().
void R_RenderSegLoop1 (void)
{
	if (fixedlightlev)
		dc_colormap = basecolormap + fixedlightlev;
	else if (fixedcolormap)
		dc_colormap = fixedcolormap;
	else if (!walllights)
		walllights = scalelight[0];

	for ( ; rw_x < rw_stopx ; rw_x++)
	{
		dc_x = rw_x;
		if (!fixedcolormap)
		{
			// calculate lighting
			unsigned index = rw_light >> LIGHTSCALESHIFT;

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE-1;

			dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
		}
		BlastColumn (colfunc);
	}
}

// [RH] This is a cache optimized version of R_RenderSegLoop(). It first
//		draws columns into a temporary buffer with a pitch of 4 and then
//		copies them to the framebuffer using a bunch of byte, word, and
//		longword moves. This may seem like a lot of extra work just to
//		draw columns to the screen (and it is), but it's actually faster
//		than drawing them directly to the screen like R_RenderSegLoop1().
//		On a Pentium II 300, using this code with rendering functions in
//		C is about twice as fast as using R_RenderSegLoop1() with an
//		assembly rendering function.

void R_RenderSegLoop2 (void)
{
	int stop = rw_stopx & ~3;

	if (rw_x >= rw_stopx)
		return;

	if (fixedlightlev)
		dc_colormap = basecolormap + fixedlightlev;
	else if (fixedcolormap)
		dc_colormap = fixedcolormap;
	else {
		// calculate lighting
		unsigned index = rw_light >> LIGHTSCALESHIFT;

		if (index >= MAXLIGHTSCALE)
			index = MAXLIGHTSCALE-1;

		if (!walllights)
			walllights = scalelight[0];

		dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
	}

	if (rw_x & 1) {
		dc_x = rw_x;
		BlastColumn (colfunc);
		rw_x++;
	}

	if (rw_x & 2) {
		if (rw_x < rw_stopx - 1) {
			rt_initcols();
			dc_x = 0;
			BlastColumn (hcolfunc_pre);
			rw_x++;
			dc_x = 1;
			BlastColumn (hcolfunc_pre);
			rt_draw2cols (0, rw_x - 1);
			rw_x++;
		} else if (rw_x == rw_stopx - 1) {
			dc_x = rw_x;
			BlastColumn (colfunc);
			rw_x++;
		}
	}

	while (rw_x < stop) {
		if (!fixedcolormap) {
			// calculate lighting
			unsigned index = rw_light >> LIGHTSCALESHIFT;

			if (index >= MAXLIGHTSCALE)
				index = MAXLIGHTSCALE-1;

			dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
		}
		rt_initcols();
		dc_x = 0;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 1;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 2;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 3;
		BlastColumn (hcolfunc_pre);
		rt_draw4cols (rw_x - 3);
		rw_x++;
	}

	if (!fixedcolormap) {
		// calculate lighting
		unsigned index = rw_light >> LIGHTSCALESHIFT;

		if (index >= MAXLIGHTSCALE)
			index = MAXLIGHTSCALE-1;

		dc_colormap = walllights[index] + basecolormap;	// [RH] add basecolormap
	}

	if (rw_stopx - rw_x == 1) {
		dc_x = rw_x;
		BlastColumn (colfunc);
		rw_x++;
	} else if (rw_stopx - rw_x >= 2) {
		rt_initcols();
		dc_x = 0;
		BlastColumn (hcolfunc_pre);
		rw_x++;
		dc_x = 1;
		BlastColumn (hcolfunc_pre);
		rt_draw2cols (0, rw_x - 1);
		if (++rw_x < rw_stopx) {
			dc_x = rw_x;
			BlastColumn (colfunc);
			rw_x++;
		}
	}
}

extern int *openings;
extern size_t maxopenings;

//
// R_AdjustOpenings
//
// killough 1/6/98, 2/1/98: remove limit on openings
// [SL] 2012-01-21 - Moved into its own function
static void R_AdjustOpenings(int start, int stop)
{
	ptrdiff_t pos = lastopening - openings;
	size_t need = (rw_stopx - start)*4 + pos;

	if (need > maxopenings)
	{
		drawseg_t *ds;
		int *oldopenings = openings;
		int *oldlast = lastopening;

		do
			maxopenings = maxopenings ? maxopenings*2 : 16384;
		while (need > maxopenings);
		
		openings = (int *)Realloc (openings, maxopenings * sizeof(*openings));
		lastopening = openings + pos;
		DPrintf ("MaxOpenings increased to %u\n", maxopenings);

		// [RH] We also need to adjust the openings pointers that
		//		were already stored in drawsegs.
		for (ds = drawsegs; ds < ds_p; ds++) {
#define ADJUST(p) if (ds->p + ds->x1 >= oldopenings && ds->p + ds->x1 <= oldlast)\
				  ds->p = ds->p - oldopenings + openings;
			ADJUST (maskedtexturecol);
			ADJUST (sprtopclip);
			ADJUST (sprbottomclip);
		}
#undef ADJUST
	}
}

//
// R_PrepWall
//
// Prepares a lineseg for rendering. It fills the walltopf, wallbottomf,
// walltopb, and wallbottomb arrays with the top and bottom pixel heights
// of the wall for the span from start to stop.
//
// lclip1 and lclip2 are percentanges of the left and right edge of the
// line that have been clipped off by R_AddLine.
//
void R_PrepWall(seg_t *line, int start, int stop, fixed_t lclip1, fixed_t lclip2)
{
	// [SL] Points v1 and v2 represent the original line's endpoints after clipping
	v2fixed_t v1, v2;
	R_ClipEndPoints(line->v1->x, line->v1->y, line->v2->x, line->v2->y,
					lclip1, lclip2, v1.x, v1.y, v2.x, v2.y);

	// clipped lineseg length
	fixed_t seglen = FixedMul(line->length, lclip2 - lclip1);

	// distance from lineseg start to start of clipped lineseg
	fixed_t segoffs = FixedMul(line->length, lclip1) + line->sidedef->textureoffset + line->offset;

	// Find distance in camera space from the vertices to the camera
	fixed_t dist1 = FixedMul(v1.x - viewx, finesine[(ANG90 - viewangle) >> ANGLETOFINESHIFT]) + 
					FixedMul(v1.y - viewy, finecosine[(ANG90 - viewangle) >> ANGLETOFINESHIFT]);
	fixed_t dist2 = FixedMul(v2.x - viewx, finesine[(ANG90 - viewangle) >> ANGLETOFINESHIFT]) + 
					FixedMul(v2.y - viewy, finecosine[(ANG90 - viewangle) >> ANGLETOFINESHIFT]);

	const fixed_t mindist = 0.25*FRACUNIT;
	const fixed_t maxdist = 16384*FRACUNIT;
	dist1 = clamp(dist1, mindist, maxdist);
	dist2 = clamp(dist2, mindist, maxdist);

	// calculate texture coordinates at the line's endpoints
	double scale1 = yfoc / FIXED2DOUBLE(dist1);
	double scale2 = yfoc / FIXED2DOUBLE(dist2);

	// [SL] Quick note on texture mapping: we can not linearly interpolate along the length of the seg
	// as it will yield evenly spaced texels instead of correct perspective (taking depth Z into account).
	// We also can not linearly interpolate Z, but we can linearly interpolate 1/Z (scale), so we linearly
	// interpolate the texture coordinates u / Z and then divide by 1/Z to get the correct u for each column.
	double uinvz = 0.0;
	double uinvz2 = FIXED2DOUBLE(seglen - FRACUNIT) * scale2;

	double scalestep = 0.0, uinvzstep = 0.0;
	if (stop > start)
	{
		scalestep = (scale2 - scale1) / (stop - start + 1);
		uinvzstep = uinvz2 / (stop - start + 1);
	}

	// fill the texture column array
	double curscale = scale1;
	for (int i = start; i <= stop; i++)
	{
		wallscalex[i] = DOUBLE2FIXED(curscale);
		texoffs[i] = segoffs + DOUBLE2FIXED(uinvz / curscale);

		uinvz += uinvzstep;
		curscale += scalestep;
	}

	// get the z coordinates of the line's vertices on each side of the line
	rw_frontcz1 = P_CeilingHeight(v1.x, v1.y, frontsector);
	rw_frontfz1 = P_FloorHeight(v1.x, v1.y, frontsector);
	rw_frontcz2 = P_CeilingHeight(v2.x, v2.y, frontsector);
	rw_frontfz2 = P_FloorHeight(v2.x, v2.y, frontsector);

	// calculate the upper and lower heights of the walls in the front
	R_FillWallHeightArray(walltopf, start, stop, rw_frontcz1, rw_frontcz2, scale1, scale2);
	R_FillWallHeightArray(wallbottomf, start, stop, rw_frontfz1, rw_frontfz2, scale1, scale2);

	if (backsector)
	{
		rw_backcz1 = P_CeilingHeight(v1.x, v1.y, backsector);
		rw_backfz1 = P_FloorHeight(v1.x, v1.y, backsector);
		rw_backcz2 = P_CeilingHeight(v2.x, v2.y, backsector);
		rw_backfz2 = P_FloorHeight(v2.x, v2.y, backsector);

		// calculate the upper and lower heights of the walls in the back
		R_FillWallHeightArray(walltopb, start, stop, rw_backcz1, rw_backcz2, scale1, scale2);
		R_FillWallHeightArray(wallbottomb, start, stop, rw_backfz1, rw_backfz2, scale1, scale2);

		// hack to allow height changes in outdoor areas (sky hack)
		// copy back ceiling height array to front ceiling height array
		if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
			memcpy(walltopf+start, walltopb+start, (stop-start+1)*sizeof(*walltopb));
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
		I_FatalError ("Bad R_StoreWallRange: %i to %i", start , stop);
#endif

	// don't overflow and crash
	if (ds_p == &drawsegs[MaxDrawSegs])
	{ // [RH] Grab some more drawsegs
		size_t newdrawsegs = MaxDrawSegs ? MaxDrawSegs*2 : 32;
		drawsegs = (drawseg_t *)Realloc (drawsegs, newdrawsegs * sizeof(drawseg_t));
		ds_p = drawsegs + MaxDrawSegs;
		MaxDrawSegs = newdrawsegs;
		DPrintf ("MaxDrawSegs increased to %d\n", MaxDrawSegs);
	}

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// mark the segment as visible for auto map
	linedef->flags |= ML_MAPPED;

	ds_p->x1 = rw_x = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;
	rw_stopx = stop+1;

	// killough: remove limits on openings
	R_AdjustOpenings(start, stop);

	// calculate scale at both ends and step
	ds_p->scale1 = rw_scale = wallscalex[start];
	ds_p->light = rw_light = rw_scale * lightscalexmul;
 
	if (stop > start)
	{
		ds_p->scale2 = wallscalex[stop];
		ds_p->scalestep = rw_scalestep =
			(ds_p->scale2 - rw_scale) / (stop-start);
		ds_p->lightstep = rw_lightstep = rw_scalestep * lightscalexmul;
	}
	else
	{
		ds_p->scale2 = ds_p->scale1;
		ds_p->scalestep = rw_scalestep = 0;
		ds_p->lightstep = rw_lightstep = 0;
	}

	// calculate texture boundaries
	//	and decide if floor / ceiling marks are needed
	midtexture = toptexture = bottomtexture = maskedtexture = 0;
	ds_p->maskedtexturecol = NULL;

	if (!backsector)
	{
		// single sided line
		midtexture = texturetranslation[sidedef->midtexture];

		// a single sided line is terminal, so it must mark ends
		markfloor = markceiling = true;

		if (linedef->flags & ML_DONTPEGBOTTOM)
		{
			// bottom of texture at bottom
			fixed_t ff = P_FloorHeight(frontsector);
			rw_midtexturemid = ff - viewz + R_ScaledTextureHeight(sidedef->midtexture);
		}
		else
		{
			// top of texture at top
			fixed_t fc = P_CeilingHeight(frontsector);
			rw_midtexturemid = fc - viewz;
		}

		rw_midtexturemid += sidedef->rowoffset;

		ds_p->silhouette = SIL_BOTH;
		ds_p->sprtopclip = screenheightarray;
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
			ds_p->sprtopclip = screenheightarray;
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

		if (spanfunc == R_FillSpan)
		{
			markfloor = markceiling = frontsector != backsector;
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

				// [RH] Add checks for colormaps
				|| backsector->floorcolormap != frontsector->floorcolormap

				|| backsector->floor_xscale != frontsector->floor_xscale
				|| backsector->floor_yscale != frontsector->floor_yscale

				|| (backsector->floor_angle + backsector->base_floor_angle) !=
				   (frontsector->floor_angle + frontsector->base_floor_angle)
				;

			markceiling = 
				  !P_IdenticalPlanes(&backsector->ceilingplane, &frontsector->ceilingplane)
				|| backsector->lightlevel != frontsector->lightlevel
				|| backsector->ceilingpic != frontsector->ceilingpic

				// killough 3/7/98: Add checks for (x,y) offsets
				|| backsector->ceiling_xoffs != frontsector->ceiling_xoffs
				|| (backsector->ceiling_yoffs + backsector->base_ceiling_yoffs) !=
				   (frontsector->ceiling_yoffs + frontsector->base_ceiling_yoffs)

				// killough 4/15/98: prevent 2s normals
				// from bleeding through fake ceilings
				|| (frontsector->heightsec && frontsector->ceilingpic != skyflatnum)

				// killough 4/17/98: draw ceilings if different light levels
				|| backsector->ceilinglightsec != frontsector->ceilinglightsec

				// [RH] Add check for colormaps
				|| backsector->ceilingcolormap != frontsector->ceilingcolormap

				|| backsector->ceiling_xscale != frontsector->ceiling_xscale
				|| backsector->ceiling_yscale != frontsector->ceiling_yscale

				|| (backsector->ceiling_angle + backsector->base_ceiling_angle) !=
				   (frontsector->ceiling_angle + frontsector->base_ceiling_angle)
				;
				
			// Sky hack
			markceiling = markceiling &&
				(frontsector->ceilingpic != skyflatnum || backsector->ceilingpic != skyflatnum);
		}


		if (doorclosed)
			markceiling = markfloor = true;

		if (rw_backcz1 < rw_frontcz1 || rw_backcz2 < rw_frontcz2)
		{
			// top texture
			toptexture = texturetranslation[sidedef->toptexture];
			if (linedef->flags & ML_DONTPEGTOP)
			{
				// top of texture at top
				fixed_t fc = P_CeilingHeight(frontsector);
				rw_toptexturemid = fc - viewz;
			}
			else
			{
				// bottom of texture
				fixed_t bc = P_CeilingHeight(backsector);
				rw_toptexturemid = bc - viewz + R_ScaledTextureHeight(sidedef->toptexture);
			}
		}
		if (rw_backfz1 > rw_frontfz1 || rw_backfz2 > rw_frontfz2)
		{
			// bottom texture
			bottomtexture = texturetranslation[sidedef->bottomtexture];

			if (linedef->flags & ML_DONTPEGBOTTOM)
			{
				// bottom of texture at bottom, top of texture at top
				fixed_t fc = P_CeilingHeight(frontsector);
				rw_bottomtexturemid = fc - viewz;
			}
			else
			{
				// top of texture at top
				fixed_t bf = P_FloorHeight(backsector);
				rw_bottomtexturemid = bf - viewz;
			}
		}

		rw_toptexturemid += sidedef->rowoffset;
		rw_bottomtexturemid += sidedef->rowoffset;

		// allocate space for masked texture tables
		if (sidedef->midtexture)
		{
			// masked midtexture
			maskedtexture = sidedef->midtexture;
			ds_p->maskedtexturecol = maskedtexturecol = lastopening - rw_x;
			lastopening += rw_stopx - rw_x;
		}

		// [SL] additional fix for sky hack
		if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
			toptexture = 0;
	}

	// [SL] 2012-01-24 - Horizon line extends to infinity by scaling the wall
	// height to 0
	if (curline->linedef->special == Line_Horizon)
	{
		rw_scale = ds_p->scale1 = ds_p->scale2 = rw_scalestep = ds_p->light = rw_light = 0;
		midtexture = toptexture = bottomtexture = maskedtexture = 0;

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
		if (!fixedcolormap)
		{
			int lightnum = (frontsector->lightlevel >> LIGHTSEGSHIFT)
					+ (foggy ? 0 : extralight);

			lightnum += R_OrthogonalLightnumAdjustment();

			if (lightnum < 0)
				walllights = scalelight[0];
			else if (lightnum >= LIGHTLEVELS)
				walllights = scalelight[LIGHTLEVELS-1];
			else
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
		if (P_CeilingHeight(viewx, viewy, frontsector) <= viewz &&
			frontsector->ceilingpic != skyflatnum)   
			markceiling = false;	
	}

	// render it
	if (markceiling && ceilingplane)
		ceilingplane = R_CheckPlane(ceilingplane, rw_x, rw_stopx-1);
	else
		markceiling = 0;

	if (markfloor && floorplane)
		floorplane = R_CheckPlane(floorplane, rw_x, rw_stopx-1);
	else
		markfloor = 0;

	R_RenderSegLoop ();

    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture)
		 && !ds_p->sprtopclip)
	{
		memcpy (lastopening, ceilingclip+start, sizeof(*lastopening)*(rw_stopx-start));
		ds_p->sprtopclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture)
		 && !ds_p->sprbottomclip)
	{
		memcpy (lastopening, floorclip+start, sizeof(*lastopening)*(rw_stopx-start));
		ds_p->sprbottomclip = lastopening - start;
		lastopening += rw_stopx - start;
	}

	if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
		ds_p->silhouette |= SIL_TOP;
	if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
		ds_p->silhouette |= SIL_BOTTOM;

	ds_p++;
}


VERSION_CONTROL (r_segs_cpp, "$Id$")

