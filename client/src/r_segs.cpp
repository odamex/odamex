// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2013 by The Odamex Team.
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

static fixed_t wallscalex[MAXWIDTH];
static int texoffs[MAXWIDTH];

extern fixed_t FocalLengthY;
extern float yfoc;

static int  	*maskedtexturecol;


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
static inline fixed_t R_TexInvScaleX(fixed_t x, int texnum)
{
	return FixedDiv(x, texturescalex[texnum]);
}

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
// R_SetTextureParams
//
// Sets dc_source, dc_texturefrac
//
static inline void R_SetTextureParams(int texnum, fixed_t texcol, fixed_t mid)
{
	dc_texturefrac = R_TexScaleY(mid + FixedMul((dc_yl - centery + 1) << FRACBITS, dc_iscale), texnum);
	dc_source = R_GetColumnData(texnum, R_TexScaleX(texcol, texnum) >> FRACBITS);
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

	float h1 = FIXED2FLOAT(val1 - viewz) * scale1;
	float h2 = FIXED2FLOAT(val2 - viewz) * scale2;
	
	float step = (h2 - h1) / (stop - start + 1);
	float frac = float(centery) - h1;

	for (int i = start; i <= stop; i++)
	{
		array[i] = clamp((int)frac, ceilingclipinitial[0], floorclipinitial[0]);
		frac -= step;
	}
}

typedef enum {
	COL_SOLIDSEG,
	COL_MASKEDSEG,
	COL_SKY
} coltype_t;

//
// R_BlastMaskedSegColumn
//
static void BlastMaskedSegColumn(void (*drawfunc)())
{
	if (maskedtexturecol[dc_x] != MAXINT && spryscale > 0)
	{
		int texnum = texturetranslation[curline->sidedef->midtexture];
		tallpost_t* post = (tallpost_t*)R_GetColumn(texnum, maskedtexturecol[dc_x]);

		// calculate lighting
		if (!fixedcolormap.isValid())
		{
			unsigned int index = MIN(rw_light >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1);
			dc_colormap = basecolormap.with(walllights[index]);
		}

		sprtopscreen = centeryfrac - FixedMul(dc_texturemid, spryscale);
		dc_iscale = 0xffffffffu / (unsigned)spryscale;

		// draw the texture
		while (!post->end())
		{
			// skip over blank posts
			if (post->length == 0)
			{
				post = post->next();
				continue;
			}

			// calculate unclipped screen coordinates for post
			int topscreen = sprtopscreen + spryscale * post->topdelta + 1;

			dc_yl = (topscreen + FRACUNIT) >> FRACBITS;
			dc_yh = (topscreen + spryscale * post->length) >> FRACBITS;

			if (dc_yh >= mfloorclip[dc_x])
				dc_yh = mfloorclip[dc_x] - 1;
			if (dc_yl <= mceilingclip[dc_x])
				dc_yl = mceilingclip[dc_x] + 1;

			dc_texturefrac = dc_texturemid - (post->topdelta << FRACBITS)
				+ (dc_yl*dc_iscale) - FixedMul(centeryfrac-FRACUNIT, dc_iscale);

			if (dc_texturefrac < 0)
			{
				int cnt = (FixedDiv(-dc_texturefrac, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
				dc_yl += cnt;
				dc_texturefrac += cnt * dc_iscale;
			}

			const fixed_t endfrac = dc_texturefrac + (dc_yh-dc_yl)*dc_iscale;
			const fixed_t maxfrac = post->length << FRACBITS;
			
			if (endfrac >= maxfrac)
			{
				int cnt = (FixedDiv(endfrac - maxfrac - 1, dc_iscale) + FRACUNIT - 1) >> FRACBITS;
				dc_yh -= cnt;
			}

			dc_source = post->data();

			if (dc_yl >= 0 && dc_yh < viewheight && dc_yl <= dc_yh)
				drawfunc();
		
			post = post->next();
		}

		maskedtexturecol[dc_x] = MAXINT;
	}

	spryscale += rw_scalestep;
	rw_light += rw_lightstep;
}


//
// R_BlastSolidSegColumn
//
static void R_BlastSolidSegColumn(void (*drawfunc)())
{
	rw_scale = wallscalex[dc_x];
	if (rw_scale > 0)
		dc_iscale = 0xffffffffu / (unsigned)rw_scale;

	fixed_t texturecolumn = texoffs[dc_x];

	walltopf[dc_x] = MAX(walltopf[dc_x], ceilingclip[dc_x]);
	wallbottomf[dc_x] = MIN(wallbottomf[dc_x], floorclip[dc_x]);

	// mark ceiling-plane areas
	if (markceiling)
	{
		int top = ceilingclip[dc_x];
		if (top < 0)
			top = 0;

		int bottom = MIN(walltopf[dc_x], floorclip[dc_x]) - 1;
		if (bottom >= viewheight)
			bottom = viewheight - 1;

		if (top <= bottom)
		{
			ceilingplane->top[dc_x] = top;
			ceilingplane->bottom[dc_x] = bottom;
		}
	}

	// mark floor-plane areas
	if (markfloor)
	{
		int top = MAX(wallbottomf[dc_x], ceilingclip[dc_x]);
		if (top < 0)
			top = 0;

		int bottom = floorclip[dc_x] - 1;
		if (bottom >= viewheight)
			bottom = viewheight - 1;

		if (top <= bottom)
		{
			floorplane->top[dc_x] = top;
			floorplane->bottom[dc_x] = bottom;
		}
	}

	// draw the wall tiers
	if (midtexture)
	{
		dc_yl = MAX(walltopf[dc_x], 0);
		dc_yh = MIN(wallbottomf[dc_x] - 1, viewheight - 1);

		if (dc_yl <= dc_yh)
		{
			R_SetTextureParams(midtexture, texturecolumn, rw_midtexturemid);
			drawfunc();
		}

		// indicate that no further drawing can be done in this column
		ceilingclip[dc_x] = floorclipinitial[dc_x];
		floorclip[dc_x] = ceilingclipinitial[dc_x];

	}
	else
	{
		if (toptexture)
		{
			walltopb[dc_x] = MAX(MIN(walltopb[dc_x], floorclip[dc_x]), walltopf[dc_x]);

			dc_yl = MAX(walltopf[dc_x], 0);
			dc_yh = MIN(walltopb[dc_x] - 1, viewheight - 1);

			if (dc_yl <= dc_yh)
			{			
				R_SetTextureParams(toptexture, texturecolumn, rw_toptexturemid);
				drawfunc();
			}

			ceilingclip[dc_x] = walltopb[dc_x];
		}
		else if (markceiling)
		{
			// no top wall
			ceilingclip[dc_x] = walltopf[dc_x];
		}
		
		if (bottomtexture)
		{
			wallbottomb[dc_x] = MIN(MAX(wallbottomb[dc_x], ceilingclip[dc_x]), wallbottomf[dc_x]);

			dc_yl = MAX(wallbottomb[dc_x], 0);
			dc_yh = MIN(wallbottomf[dc_x] - 1, viewheight - 1);

			if (dc_yl <= dc_yh)
			{
				R_SetTextureParams(bottomtexture, texturecolumn, rw_bottomtexturemid);
				drawfunc();
			}

			floorclip[dc_x] = wallbottomb[dc_x];
		}
		else if (markfloor)
		{
			// no bottom wall
			floorclip[dc_x] = wallbottomf[dc_x];
		}

		if (maskedtexture)
		{
			// save texturecol for backdrawing of masked mid texture
			maskedtexturecol[dc_x] = R_TexScaleX(texturecolumn, maskedtexture) >> FRACBITS;
		}
	}

	// cph - if we completely blocked further sight through this column,
	// add this info to the solid columns array
	if ((markceiling || markfloor) && (floorclip[dc_x] <= ceilingclip[dc_x]))
		solidcol[dc_x] = 1;

	rw_light += rw_lightstep;
}


EXTERN_CVAR(r_skypalette)
static int		skytex;
static angle_t	skyflip;
static int		frontpos;

//
// R_BlastSkyColumn
//
static void R_BlastSkyColumn(void (*drawfunc)(void))
{
	dc_yl = skyplane->top[dc_x];
	dc_yh = skyplane->bottom[dc_x];

	if (dc_yl >= 0 && dc_yl <= dc_yh)
	{
		int angle = ((((viewangle + xtoviewangle[dc_x])^skyflip)>>sky1shift) + frontpos)>>16;

		dc_texturefrac = dc_texturemid + (dc_yl - centery + 1) * dc_iscale;
		dc_source = R_GetColumnData(skytex, angle);
		drawfunc();
	}
}

inline void SolidColumnBlaster()
{
	R_BlastSolidSegColumn(colfunc);
}

inline void SolidHColumnBlaster()
{
	R_BlastSolidSegColumn(hcolfunc_pre);
}

inline void MaskedColumnBlaster()
{
	BlastMaskedSegColumn(colfunc);
}

inline void MaskedHColumnBlaster()
{
	BlastMaskedSegColumn(hcolfunc_pre);
}

inline void SkyColumnBlaster()
{
	R_BlastSkyColumn(colfunc);
}

inline void SkyHColumnBlaster()
{
	R_BlastSkyColumn(hcolfunc_pre);
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

void R_RenderColumnRange(int start, int stop, int coltype, bool columnmethod)
{
	if (start > stop)
		return;

	void (*colblast)() = NULL;
	void (*hcolblast)() = NULL;

	if (coltype == COL_SOLIDSEG)
	{
		colblast = SolidColumnBlaster;
		hcolblast = SolidHColumnBlaster;
	}
	else if (coltype == COL_MASKEDSEG)
	{
		colblast = MaskedColumnBlaster;
		hcolblast = MaskedHColumnBlaster;
	}
	else if (coltype == COL_SKY)
	{
		colblast = SkyColumnBlaster;
		hcolblast = SkyHColumnBlaster;
	}
	
	dc_x = start;

	bool calc_light = false;
	if (coltype != COL_SKY)
	{
		if (fixedlightlev)
		{
			dc_colormap = basecolormap.with(fixedlightlev);
		}
		else if (fixedcolormap.isValid())
		{
			dc_colormap = fixedcolormap;	
		}
		else
		{
			if (!walllights)
				walllights = scalelight[0];
			calc_light = true;
		}
	}

	if (columnmethod == false)
	{
		for (dc_x = start; dc_x <= stop; dc_x++)
		{
			if (calc_light)
			{
				int index = MIN(rw_light >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1);
				dc_colormap = basecolormap.with(walllights[index]);
			}

			colblast();
		}
	}
	else
	{
		int blockend = (stop + 1) & ~3;

		// blit until dc_x is DWORD aligned
		while ((dc_x < blockend) && (dc_x & 3))
		{
			if (calc_light)
			{
				int index = MIN(rw_light >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1);
				dc_colormap = basecolormap.with(walllights[index]);
			}

			colblast();	
			dc_x++;
		}

		// blit in DWORD blocks to a temporary buffer horizontally, with
		// the columns interleaved, eg write to buf[0], buf[4], buf[8]
		while (dc_x < blockend)
		{
			if (calc_light)
			{
				int index = MIN(rw_light >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1);
				dc_colormap = basecolormap.with(walllights[index]);
			}

			rt_initcols();
			hcolblast();
			dc_x++;
			hcolblast();
			dc_x++;
			hcolblast();
			dc_x++;
			hcolblast();
			dc_x++;
			rt_draw4cols(dc_x - 4);
		}

		// blit any remaining pixels
		while (dc_x <= stop)
		{
			if (calc_light)
			{
				int index = MIN(rw_light >> LIGHTSCALESHIFT, MAXLIGHTSCALE - 1);
				dc_colormap = basecolormap.with(walllights[index]);
			}

			colblast();	
			dc_x++;
		}
	}
}

//
// R_RenderSegLoop
//
// Renders a solid seg
//
void R_RenderSegRange(int start, int stop)
{
	R_RenderColumnRange(start, stop, COL_SOLIDSEG, r_columnmethod);
}


//
// R_RenderMaskedSegRange
//
// Renders a masked seg
//
void R_RenderMaskedSegRange(drawseg_t* ds, int x1, int x2)
{
	int 		lightnum;
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

	int texnum = texturetranslation[curline->sidedef->midtexture];
	fixed_t texheight = R_TexScaleY(textureheight[texnum], texnum);

	// find texture positioning
	if (curline->linedef->flags & ML_DONTPEGBOTTOM)
		dc_texturemid = MAX<fixed_t>(P_FloorHeight(frontsector), P_FloorHeight(backsector)) + texheight;
	else
		dc_texturemid = MIN<fixed_t>(P_CeilingHeight(frontsector), P_CeilingHeight(backsector));

	dc_texturemid = R_TexScaleY(dc_texturemid - viewz + curline->sidedef->rowoffset, texnum);
	
	int64_t topscreenclip = int64_t(centery) << 2*FRACBITS;
	int64_t botscreenclip = int64_t(centery - viewheight) << 2*FRACBITS;
 
	// top of texture entirely below screen?
	if (int64_t(dc_texturemid) * ds->scale1 <= botscreenclip &&
		int64_t(dc_texturemid) * ds->scale2 <= botscreenclip)
		return;
 
	// bottom of texture entirely above screen?
	if (int64_t(dc_texturemid - texheight) * ds->scale1 > topscreenclip &&
		int64_t(dc_texturemid - texheight) * ds->scale2 > topscreenclip)
		return;

	basecolormap = frontsector->floorcolormap->maps;	// [RH] Set basecolormap

	// killough 4/13/98: get correct lightlevel for 2s normal textures
	lightnum = (R_FakeFlat(frontsector, &tempsec, NULL, NULL, false)
			->lightlevel >> LIGHTSEGSHIFT) + (foggy ? 0 : extralight);

	lightnum += R_OrthogonalLightnumAdjustment();

	walllights = lightnum >= LIGHTLEVELS ? scalelight[LIGHTLEVELS-1] :
		lightnum <  0 ? scalelight[0] : scalelight[lightnum];

	maskedtexturecol = ds->maskedtexturecol;

	rw_scalestep = R_TexInvScaleY(ds->scalestep, texnum);
	spryscale = R_TexInvScaleY(ds->scale1, texnum) + (x1 - ds->x1) * rw_scalestep;

	rw_lightstep = ds->lightstep;
	rw_light = ds->light + (x1 - ds->x1) * rw_lightstep;

	mfloorclip = ds->sprbottomclip;
	mceilingclip = ds->sprtopclip;

	// draw the columns
	R_RenderColumnRange(x1, x2, COL_MASKEDSEG, r_columnmethod);
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

	if (pl->picnum == skyflatnum)
	{
		// use sky1
		skytex = sky1texture;
		skyflip = 0;
	}
	else if (pl->picnum == int(PL_SKYFLAT))
	{
		// use sky2
		skytex = sky2texture;
		skyflip = 0;
	}
	else
	{
		// MBF's linedef-controlled skies
		short picnum = (pl->picnum & ~PL_SKYFLAT) - 1;
		const line_t* line = &lines[picnum < numlines ? picnum : 0];

		// Sky transferred from first sidedef
		const side_t* side = *line->sidenum + sides;

		// Texture comes from upper texture of reference sidedef
		skytex = texturetranslation[side->toptexture];

		// Horizontal offset is turned into an angle offset,
		// to allow sky rotation as well as careful positioning.
		// However, the offset is scaled very small, so that it
		// allows a long-period of sky rotation.
		frontpos = (-side->textureoffset) >> 6;

		// Vertical offset allows careful sky positioning.
		dc_texturemid = side->rowoffset - 28*FRACUNIT;

		// We sometimes flip the picture horizontally.
		//
		// Doom always flipped the picture, so we make it optional,
		// to make it easier to use the new feature, while to still
		// allow old sky textures to be used.
		skyflip = line->args[2] ? 0u : ~0u;
	}

	R_ResetDrawFuncs();

	palette_t *pal = GetDefaultPalette();

	// set up the appropriate colormap for the sky
	if (fixedlightlev)
	{
		dc_colormap = shaderef_t(&pal->maps, fixedlightlev);
	}
	else if (fixedcolormap.isValid() && r_skypalette)
	{
		dc_colormap = fixedcolormap;
	}
	else
	{
		// [SL] 2011-06-28 - Emulate vanilla Doom's handling of skies
		// when the player has the invulnerability powerup
		dc_colormap = shaderef_t(&pal->maps, 0);
	}

	dc_iscale = skyiscale >> skystretch;
	dc_texturemid = skytexturemid;
	skyplane = pl;

	R_RenderColumnRange(pl->minx, pl->maxx, COL_SKY, r_columnmethod);
				
	R_ResetDrawFuncs();
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
	size_t need = 4*(stop - start + 1) + pos;

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

static fixed_t R_LineLength(fixed_t px1, fixed_t py1, fixed_t px2, fixed_t py2)
{
	float dx = FIXED2FLOAT(px2 - px1);
	float dy = FIXED2FLOAT(py2 - py1);

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
	int width = stop - start + 1;
	if (width <= 0)
		return;

	// determine which vertex of the linedef should be used for texture alignment
	vertex_t *v1;
	if (curline->linedef->sidenum[0] == curline->sidedef - sides)
		v1 = curline->linedef->v1;
	else
		v1 = curline->linedef->v2;

	// clipped lineseg length
	fixed_t seglen = R_LineLength(px1, py1, px2, py2);

	// distance from lineseg start to start of clipped lineseg
	fixed_t segoffs = R_LineLength(v1->x, v1->y, px1, py1) + curline->sidedef->textureoffset;

	const fixed_t mindist = NEARCLIP;
	const fixed_t maxdist = 16384*FRACUNIT;
	dist1 = clamp(dist1, mindist, maxdist);
	dist2 = clamp(dist2, mindist, maxdist);

	// calculate texture coordinates at the line's endpoints
	float scale1 = yfoc / FIXED2FLOAT(dist1);
	float scale2 = yfoc / FIXED2FLOAT(dist2);

	// [SL] Quick note on texture mapping: we can not linearly interpolate along the length of the seg
	// as it will yield evenly spaced texels instead of correct perspective (taking depth Z into account).
	// We also can not linearly interpolate Z, but we can linearly interpolate 1/Z (scale), so we linearly
	// interpolate the texture coordinates u / Z and then divide by 1/Z to get the correct u for each column.

	float scalestep = (scale2 - scale1) / width;
	float uinvzstep = FIXED2FLOAT(seglen) * scale2 / width;

	// fill the texture column array
	float uinvz = 0.0f;
	float curscale = scale1;
	for (int i = start; i <= stop; i++)
	{
		wallscalex[i] = FLOAT2FIXED(curscale);
		texoffs[i] = segoffs + FLOAT2FIXED(uinvz / curscale);

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
	
		const fixed_t tolerance = FRACUNIT/2;

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
		if (frontsector->ceilingpic == skyflatnum && backsector->ceilingpic == skyflatnum)
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
		I_FatalError ("Bad R_StoreWallRange: %i to %i", start , stop);
#endif

	if (start > stop)
		return;

	int count = stop - start + 1;

	R_ReallocDrawSegs();	// don't overflow and crash

	sidedef = curline->sidedef;
	linedef = curline->linedef;

	// mark the segment as visible for auto map
	linedef->flags |= ML_MAPPED;

	ds_p->x1 = start;
	ds_p->x2 = stop;
	ds_p->curline = curline;

	// killough: remove limits on openings
	R_AdjustOpenings(start, stop);

	// calculate scale at both ends and step
	ds_p->scale1 = rw_scale = wallscalex[start];
	ds_p->scale2 = wallscalex[stop];
	ds_p->scalestep = rw_scalestep;

	ds_p->light = rw_light = rw_scale * lightscalexmul;
 	ds_p->lightstep = rw_lightstep = rw_scalestep * lightscalexmul;

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
			fixed_t texheight = R_TexScaleY(textureheight[midtexture], midtexture); 
			rw_midtexturemid = P_FloorHeight(frontsector) - viewz + texheight;
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


		if (rw_hashigh)
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
				fixed_t texheight = R_TexScaleY(textureheight[toptexture], toptexture);
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
			ds_p->maskedtexturecol = maskedtexturecol = lastopening - start;
			lastopening += count; 
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
		if (!fixedcolormap.isValid())
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
		ceilingplane = R_CheckPlane(ceilingplane, start, stop);
	else
		markceiling = false;

	if (markfloor && floorplane)
		floorplane = R_CheckPlane(floorplane, start, stop);
	else
		markfloor = false;

	R_RenderSegRange(start, stop);

    // save sprite clipping info
    if ( ((ds_p->silhouette & SIL_TOP) || maskedtexture) && !ds_p->sprtopclip)
	{
		memcpy(lastopening, ceilingclip+start, count * sizeof(*lastopening));
		ds_p->sprtopclip = lastopening - start;
		lastopening += count;
	}

    if ( ((ds_p->silhouette & SIL_BOTTOM) || maskedtexture) && !ds_p->sprbottomclip)
	{
		memcpy(lastopening, floorclip+start, count * sizeof(*lastopening));
		ds_p->sprbottomclip = lastopening - start;
		lastopening += count;
	}

	if (maskedtexture && !(ds_p->silhouette & SIL_TOP))
		ds_p->silhouette |= SIL_TOP;
	if (maskedtexture && !(ds_p->silhouette & SIL_BOTTOM))
		ds_p->silhouette |= SIL_BOTTOM;

	ds_p++;
}


VERSION_CONTROL (r_segs_cpp, "$Id$")

