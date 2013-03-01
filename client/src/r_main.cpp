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
//	Rendering main loop and setup functions,
//	 utility functions (BSP, geometry, trigonometry).
//	See tables.c, too.
//
//-----------------------------------------------------------------------------

#include "m_alloc.h"
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include "doomdef.h"
#include "d_net.h"
#include "doomstat.h"
#include "m_random.h"
#include "m_bbox.h"
#include "p_local.h"
#include "r_local.h"
#include "r_sky.h"
#include "st_stuff.h"
#include "c_cvars.h"
#include "v_video.h"
#include "stats.h"
#include "z_zone.h"
#include "i_video.h"
#include "vectors.h"

#define DISTMAP			2

void R_SpanInitData ();

extern int *walllights;

// [RH] Defined in d_main.cpp
extern dyncolormap_t NormalLight;
extern bool r_fakingunderwater;

EXTERN_CVAR (r_viewsize)
EXTERN_CVAR (r_widescreen)
EXTERN_CVAR (sv_allowwidescreen)

static float	LastFOV = 0.0f;
fixed_t			FocalLengthX;
fixed_t			FocalLengthY;
double			focratio;
double			ifocratio;
int 			viewangleoffset = 0;

// increment every time a check is made
int 			validcount = 1;

// [RH] colormap currently drawing with
shaderef_t		basecolormap;
int				fixedlightlev;
shaderef_t		fixedcolormap;

int 			centerx;
extern "C" {int	centery; }

fixed_t 		centerxfrac;
fixed_t 		centeryfrac;
fixed_t			yaspectmul;

// just for profiling purposes
int 			framecount;
int 			linecount;
int 			loopcount;

fixed_t 		viewx;
fixed_t 		viewy;
fixed_t 		viewz;

angle_t 		viewangle;

fixed_t 		viewcos;
fixed_t 		viewsin;

AActor			*camera;	// [RH] camera to draw from. doesn't have to be a player

//
// precalculated math tables
//
angle_t 		clipangle;

// The viewangletox[viewangle + FINEANGLES/4] lookup
// maps the visible view angles to screen X coordinates,
// flattening the arc to a flat projection plane.
// There will be many angles mapped to the same X.
int 			viewangletox[FINEANGLES/2];

// The xtoviewangleangle[] table maps a screen pixel
// to the lowest viewangle that maps back to x ranges
// from clipangle to -clipangle.
angle_t 		*xtoviewangle;

const fixed_t	*finecosine = &finesine[FINEANGLES/4];

int				scalelight[LIGHTLEVELS][MAXLIGHTSCALE];
int				scalelightfixed[MAXLIGHTSCALE];
int				zlight[LIGHTLEVELS][MAXLIGHTZ];

// [RH] used to keep hires modes dark enough
int				lightscalexmul;
int				lightscaleymul;

// bumped light from gun blasts
int 			extralight;

// [RH] ignore extralight and fullbright
BOOL			foggy;

BOOL			setsizeneeded;
int				setblocks, setdetail = -1;

fixed_t			freelookviewheight;

unsigned int	R_OldBlend = ~0;

void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*lucentcolfunc) (void);
void (*transcolfunc) (void);
void (*tlatedlucentcolfunc) (void);
void (*spanfunc) (void);
void (*spanslopefunc) (void);

void (*hcolfunc_pre) (void);
void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
void (*hcolfunc_post2) (int hx, int sx, int yl, int yh);
void (*hcolfunc_post4) (int sx, int yl, int yh);

static int lastcenteryfrac;
// [AM] Number of fineangles in a default 90 degree FOV at a 4:3 resolution.
int FieldOfView = 2048;
int CorrectFieldOfView = 2048;

//
//
// R_PointOnSide
//
// Traverse BSP (sub) tree, check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
//
//

int R_PointOnSide (fixed_t x, fixed_t y, node_t *node)
{
	if (!node->dx)
		return x <= node->x ? node->dy > 0 : node->dy < 0;

	if (!node->dy)
		return y <= node->y ? node->dx < 0 : node->dx > 0;

	x -= node->x;
	y -= node->y;

	// Try to quickly decide by looking at sign bits.
	if ((node->dy ^ node->dx ^ x ^ y) < 0)
		return (node->dy ^ x) < 0;  // (left is negative)
	return FixedMul (y, node->dx >> FRACBITS) >= FixedMul (node->dy >> FRACBITS, x);
}

//
//
// R_PointOnSegSide
//
// Same, except takes a lineseg as input instead of a linedef
//
// killough 5/2/98: reformatted
//
//

int R_PointOnSegSide (fixed_t x, fixed_t y, seg_t *line)
{
	fixed_t lx = line->v1->x;
	fixed_t ly = line->v1->y;
	fixed_t ldx = line->v2->x - lx;
	fixed_t ldy = line->v2->y - ly;

	if (!ldx)
		return x <= lx ? ldy > 0 : ldy < 0;

	if (!ldy)
		return y <= ly ? ldx < 0 : ldx > 0;

	x -= lx;
	y -= ly;

	// Try to quickly decide by looking at sign bits.
	if ((ldy ^ ldx ^ x ^ y) < 0)
		return (ldy ^ x) < 0;          // (left is negative)
	return FixedMul (y, ldx >> FRACBITS) >= FixedMul (ldy >> FRACBITS, x);
}

#define R_P2ATHRESHOLD (INT_MAX / 4)
//
// R_PointToAngle
//
// To get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system,
// then the y (<=x) is scaled and divided by x to get a tangent (slope)
// value which is looked up in the tantoangle[] table.
//

angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{
	x -= viewx;
	y -= viewy;

	if((x | y) == 0)
		return 0;

	if(x < R_P2ATHRESHOLD && x > -R_P2ATHRESHOLD &&
		y < R_P2ATHRESHOLD && y > -R_P2ATHRESHOLD)
	{
		if(x >= 0)
		{
			if (y >= 0)
			{
				if(x > y)
				{
					// octant 0
					return tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 1
					return ANG90 - 1 - tantoangle_acc[SlopeDiv(x, y)];
				}
			}
			else // y < 0
			{
				y = -y;

				if(x > y)
				{
					// octant 8
					return 0 - tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 7
					return ANG270 + tantoangle_acc[SlopeDiv(x, y)];
				}
			}
		}
		else // x < 0
		{
			x = -x;

			if(y >= 0)
			{
				if(x > y)
				{
					// octant 3
					return ANG180 - 1 - tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 2
					return ANG90 + tantoangle_acc[SlopeDiv(x, y)];
				}
			}
			else // y < 0
			{
				y = -y;

				if(x > y)
				{
					// octant 4
					return ANG180 + tantoangle_acc[SlopeDiv(y, x)];
				}
				else
				{
					// octant 5
					return ANG270 - 1 - tantoangle_acc[SlopeDiv(x, y)];
				}
			}
		}
	}
	else
	{
      return (angle_t)(atan2((double)y, (double)x) * (ANG180 / PI));
	}

   return 0;
}

//
// R_PointToAngle - wrapper around R_PointToAngle2
//
angle_t
R_PointToAngle
( fixed_t	x,
  fixed_t	y )
{
    return R_PointToAngle2 (viewx, viewy, x, y);
}

//
// R_InitPointToAngle
//
/*
void R_InitPointToAngle (void)
{
	int i;
	float f;
//
// slope (tangent) to angle lookup
//
	for (i = 0; i <= SLOPERANGE; i++)
	{
		f = atan((float)i/SLOPERANGE)/(3.141592657*2); // denis - vanilla
		tantoangle[i] = (angle_t)(0xffffffff*f);
	}
}
*/

//
// R_PointToDist
//
// Returns the distance from the viewpoint to some other point. In a
// floating point environment, we'd probably be better off using the
// Pythagorean Theorem to determine the result.
//
// killough 5/2/98: simplified
// [RH] Simplified further [sin (t + 90 deg) == cos (t)]
//
fixed_t
R_PointToDist
( fixed_t	x,
  fixed_t	y )
{
	fixed_t dx = abs(x - viewx);
	fixed_t dy = abs(y - viewy);

	if (dy > dx)
	{
		fixed_t t = dx;
		dx = dy;
		dy = t;
	}

	return FixedDiv (dx, finecosine[tantoangle[FixedDiv (dy, dx) >> DBITS] >> ANGLETOFINESHIFT]);
}

//
//
// R_PointToDist2
//
//

fixed_t R_PointToDist2 (fixed_t dx, fixed_t dy)
{
	dx = abs (dx);
	dy = abs (dy);

	if (dy > dx)
	{
		fixed_t t = dx;
		dx = dy;
		dy = t;
	}

	return FixedDiv (dx, finecosine[tantoangle[FixedDiv (dy, dx) >> DBITS] >> ANGLETOFINESHIFT]);
}

//
//
// R_ScaleFromGlobalAngle
//
// Returns the texture mapping scale for the current line (horizontal span)
// at the given angle. rw_distance must be calculated first.
//
//

fixed_t R_ScaleFromGlobalAngle (angle_t visangle)
{
	angle_t anglea = ANG90 + (visangle - viewangle);
	angle_t angleb = ANG90 + (visangle - rw_normalangle);
	// both sines are always positive
	fixed_t num = FixedMul (FocalLengthY, finesine[angleb>>ANGLETOFINESHIFT]);
	fixed_t den = FixedMul (rw_distance, finesine[anglea>>ANGLETOFINESHIFT]);

	static const fixed_t maxscale = 256 << FRACBITS;
	static const fixed_t minscale = 64;

	if (den == 0)
		return maxscale;

	fixed_t scale = FixedDiv(num, den);
	if (scale > maxscale)
		scale = maxscale;
	else if (scale < minscale)
		scale = minscale;

	return scale;
}



void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
	int index = ang >> ANGLETOFINESHIFT;
	
	tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
	ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}

//
// R_ColumnToPointOnSeg
//
// Sets the x and y parameters to their respective values at the point
// where line intersects with a ray drawn through the column on the screen
// projection.  Uses parametric line equations to find the point of intercept.
//
void R_ColumnToPointOnSeg(int column, line_t *line, fixed_t &x, fixed_t &y)
{
	// Transform the end points of the seg in relation to the camera position
	fixed_t dx1 = line->v1->x - viewx;
	fixed_t dy1 = line->v1->y - viewy;
	fixed_t dx2 = line->v2->x - viewx;
	fixed_t dy2 = line->v2->y - viewy;

	// Rotate the end points by viewangle
	angle_t rotation_ang1 = (angle_t)(-(int)viewangle);
	fixed_t x1, y1, x2, y2;
	R_RotatePoint(dx1, dy1, rotation_ang1, x1, y1);
	R_RotatePoint(dx2, dy2, rotation_ang1, x2, y2);

	// (fx1, fy1) and (fx2, fy2) are the end points for the lineseg
	float fx1 = FIXED2FLOAT(x1), fx2 = FIXED2FLOAT(x2);
	float fy1 = FIXED2FLOAT(y1), fy2 = FIXED2FLOAT(y2);

	// (fx3, fy3) and (fx4, fy4) are two points on a line from the origin
	// through the screen column
	angle_t ang = xtoviewangle[column];
	float fx3 = 0.0f, fx4 = FIXED2FLOAT(finecosine[ang >> ANGLETOFINESHIFT]);
	float fy3 = 0.0f, fy4 = FIXED2FLOAT(finesine[ang >> ANGLETOFINESHIFT]);

	// (xint, yint) is the point where the line from the viewpoint intersects
	// the lineseg
	fixed_t xint, yint;

	float tnum = (fx4 - fx3) * (fy1 - fy3) - (fy4 - fy3) * (fx1 - fx3);
	float tden = (fy4 - fy3) * (fx2 - fx1) - (fx4 - fx3) * (fy2 - fy1);

	if (fabs(tden) > 0.0f)
	{
		float t = tnum / tden;

		xint = FLOAT2FIXED(fx1 + t * (fx2 - fx1));
		yint = FLOAT2FIXED(fy1 + t * (fy2 - fy1));
	}
	else
	{
		// the lineseg and the line from viewpoint are parallel
		// just give the closest endpoint of the lineseg
		float dist1 = sqrtf(fx1 * fx1 + fy1 * fy1);
		float dist2 = sqrtf(fx2 * fx2 + fy2 * fy2);
		if (dist1 <= dist2)
		{
			xint = FLOAT2FIXED(fx1);
			yint = FLOAT2FIXED(fy1);
		}
		else
		{
			xint = FLOAT2FIXED(fx2);
			yint = FLOAT2FIXED(fy2);
		}
	}

	// rotate (x, y) back by +viewangle to translate back to camera space
	angle_t rotation_ang2 = viewangle;
	R_RotatePoint(xint, yint, rotation_ang2, x, y);
	x += viewx;
	y += viewy;
} 

//
//
// R_InitTables
//
//
#if 0
// [Russell] - Calling this function can desync demos (tnt demo1 msvc being a
// prime example)
void R_InitTables (void)
{
	int i;
	float a, fv;

	// viewangle tangent table
	for (i = 0; i < FINEANGLES/2; i++)
	{
		a = (i-FINEANGLES/4+0.5)*PI*2/FINEANGLES;
		fv = FRACUNIT*tan (a);
		finetangent[i] = (fixed_t)fv;
	}

	// finesine table
	for (i = 0; i < 5*FINEANGLES/4; i++)
	{
		// OPTIMIZE: mirror...
		a = (i+0.5)*PI*2/FINEANGLES;
		finesine[i] = (fixed_t)(FRACUNIT * sin (a));
	}
}
#endif

//
//
// R_InitTextureMapping
//
//

void R_InitTextureMapping (void)
{
	int i, t, x;

	// Use tangent table to generate viewangletox: viewangletox will give
	// the next greatest x after the view angle.

	const fixed_t hitan = finetangent[FINEANGLES/4+CorrectFieldOfView/2];
	const fixed_t lotan = finetangent[FINEANGLES/4-CorrectFieldOfView/2];
	const int highend = viewwidth + 1;

	// Calc focallength so FieldOfView angles covers viewwidth.
	FocalLengthX = FixedDiv (centerxfrac, hitan);
	FocalLengthY = FixedDiv (FixedMul (centerxfrac, yaspectmul), hitan);

	focratio = double(FocalLengthY) / double(FocalLengthX);
	ifocratio = double(FocalLengthX) / double(FocalLengthY);

	for (i = 0; i < FINEANGLES/2; i++)
	{
		fixed_t tangent = finetangent[i];

		if (tangent > hitan)
			t = -1;
		else if (tangent < lotan)
			t = highend;
		else
		{
			t = (centerxfrac - FixedMul (tangent, FocalLengthX) + FRACUNIT - 1) >> FRACBITS;

			if (t < -1)
				t = -1;
			else if (t > highend)
				t = highend;
		}
		viewangletox[i] = t;
	}

	// Scan viewangletox[] to generate xtoviewangle[]:
	//	xtoviewangle will give the smallest view angle
	//	that maps to x.
	for (x = 0; x <= viewwidth; x++)
	{
		i = 0;
		while (viewangletox[i] > x)
			i++;
		xtoviewangle[x] = (i<<ANGLETOFINESHIFT)-ANG90;
	}

	// Take out the fencepost cases from viewangletox.
	for (i = 0; i < FINEANGLES/2; i++)
	{
		t = FixedMul (finetangent[i], FocalLengthX);
		t = centerx - t;

		if (viewangletox[i] == -1)
			viewangletox[i] = 0;
		else if (viewangletox[i] == highend)
			viewangletox[i]--;
	}

	clipangle = xtoviewangle[0];
}

// Changes the field of view.
void R_SetFOV(float fov, bool force = false)
{
	if (fov == LastFOV && !force)
		return;

	if (fov < 1)
		fov = 1;
	else if (fov > 179)
		fov = 179;

	LastFOV = fov;
	FieldOfView = static_cast<int>(fov * static_cast<float>(FINEANGLES) / 360.0f);
	float am = (static_cast<float>(screen->width) / screen->height) / (4.0f / 3.0f);
	if (R_GetWidescreen() >= WIDE_TRUE && am > 1.0f)
	{
		// [AM] The FOV is corrected to fit the wider screen.
		float radfov = fov * PI / 180.0f;
		float widefov = (2 * atan(am * tan(radfov / 2))) * 180.0f / PI;
		CorrectFieldOfView = static_cast<int>(widefov * static_cast<float>(FINEANGLES) / 360.0f);
	}
	else
	{
		// [AM] The FOV is left as-is for the wider screen.
		CorrectFieldOfView = FieldOfView;
	}
	setsizeneeded = true;
}

//
//
// R_GetFOV
//
// Returns the current field of view in degrees
//
//

float R_GetFOV (void)
{
	return LastFOV;
}

// [AM] Always grab the correct widescreen setting based on a
//      combination of r_widescreen and serverside forcing.
int R_GetWidescreen()
{
	if ((r_widescreen.asInt() < WIDE_TRUE) || !multiplayer ||
	    (multiplayer && sv_allowwidescreen))
	{
		return r_widescreen.asInt();
	}

	return r_widescreen.asInt() - WIDE_TRUE;
}

//
//
// R_InitLightTables
//
// Only inits the zlight table, because the scalelight table changes with
// view size.
//
// [RH] This now sets up indices into a colormap rather than pointers into
// the colormap, because the colormap can vary by sector, but the indices
// into it don't.
//
//

void R_InitLightTables (void)
{
	int i;
	int j;
	int level;
	int startmap;
	int scale;

	// Calculate the light levels to use
	//	for each level / distance combination.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j=0 ; j<MAXLIGHTZ ; j++)
		{
			scale = FixedDiv (160*FRACUNIT, (j+1)<<LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT-LIGHTSCALEMULBITS;
			level = startmap - scale/DISTMAP;

			if (level < 0)
				level = 0;
			else if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			zlight[i][j] = level;
		}
	}

	lightscalexmul = ((320<<detailyshift) * (1<<LIGHTSCALEMULBITS)) / screen->width;
}

//
//
// R_SetViewSize
//
// Do not really change anything here, because it might be in the middle
// of a refresh. The change will take effect next refresh.
//
//

void R_SetViewSize (int blocks)
{
	setsizeneeded = true;
	setblocks = blocks;
}

//
//
// CVAR r_detail
//
// Selects a pixel doubling mode
//
//

CVAR_FUNC_IMPL (r_detail)
{
	static BOOL badrecovery = false;

	if (badrecovery)
	{
		badrecovery = false;
		return;
	}

	if (var < 0.0 || var > 3.0)
	{
		Printf (PRINT_HIGH, "Bad detail mode. (Use 0-3)\n");
		badrecovery = true;
		var.Set ((float)((detailyshift << 1)|detailxshift));
		return;
	}

	setdetail = (int)var;
	setsizeneeded = true;
}

CVAR_FUNC_IMPL (r_widescreen)
{
	if (var.asInt() < 0 || var.asInt() > 3)
	{
		Printf(PRINT_HIGH, "Invalid widescreen setting.\n");
		var.RestoreDefault();
	}
	setmodeneeded = true;
}

//
//
// R_ExecuteSetViewSize
//
//

void R_ExecuteSetViewSize (void)
{
	int i, j;
	int level;
	int startmap;
	int virtheight, virtwidth;

	setsizeneeded = false;

	if (setdetail >= 0)
	{
		if (!r_columnmethod)
		{
			R_RenderSegLoop = R_RenderSegLoop1;
			// [RH] x-doubling only works with the standard column drawer
			detailxshift = setdetail & 1;
		}
		else
		{
			R_RenderSegLoop = R_RenderSegLoop2;
			detailxshift = 0;
		}
		detailyshift = (setdetail >> 1) & 1;
		setdetail = -1;
	}

	if (setblocks == 11 || setblocks == 12)
	{
		realviewwidth = screen->width;
		freelookviewheight = realviewheight = screen->height;
	}
	else if (setblocks == 10) {
		realviewwidth = screen->width;
		realviewheight = ST_Y;
		freelookviewheight = screen->height;
	}
	else
	{
		realviewwidth = ((setblocks*screen->width)/10) & (~(15>>(screen->is8bit() ? 0 : 2)));
		realviewheight = ((setblocks*ST_Y)/10)&~7;
		freelookviewheight = ((setblocks*screen->height)/10)&~7;
	}

	viewwidth = realviewwidth >> detailxshift;
	viewheight = realviewheight >> detailyshift;
	freelookviewheight >>= detailyshift;

	{
		char temp[16];

		sprintf (temp, "%d x %d", viewwidth, viewheight);
		r_viewsize.ForceSet (temp);
	}

	lastcenteryfrac = 1<<30;
	centery = viewheight/2;
	centerx = viewwidth/2;
	centerxfrac = centerx<<FRACBITS;
	centeryfrac = centery<<FRACBITS;

	virtwidth = screen->width >> detailxshift;
	virtheight = screen->height >> detailyshift;

	if (R_GetWidescreen() != WIDE_STRETCH)
		yaspectmul = (78643 << detailxshift) >> detailyshift ; // [AM] Force correct aspect ratio
	else
		yaspectmul = (fixed_t)(65536.0f*(320.0f*(float)virtheight/(200.0f*(float)virtwidth)));

	colfunc = basecolfunc = R_DrawColumn;
	lucentcolfunc = R_DrawTranslucentColumn;
	fuzzcolfunc = R_DrawFuzzColumn;
	transcolfunc = R_DrawTranslatedColumn;
	tlatedlucentcolfunc = R_DrawTlatedLucentColumn;
	spanfunc = R_DrawSpan;
	spanslopefunc = R_DrawSlopeSpan;

	// [RH] Horizontal column drawers
	hcolfunc_pre = R_DrawColumnHoriz;
	hcolfunc_post1 = rt_map1col;
	hcolfunc_post2 = rt_map2cols;
	hcolfunc_post4 = rt_map4cols;

	R_InitBuffer (viewwidth, viewheight);
	R_InitTextureMapping ();

	// psprite scales
	if (R_GetWidescreen() != WIDE_STRETCH)
	{
		// [AM] Using centerxfrac will make our sprite too fat, so we
		//      generate a corrected 4:3 screen width based on our
		//      height, then generate the x-scale based on that.
		int cswidth, crvwidth;
		cswidth = (4 * screen->height) / 3;
		if (setblocks < 10)
			crvwidth = ((setblocks * cswidth) / 10) & (~(15 >> (screen->is8bit() ? 0 : 2)));
		else
			crvwidth = cswidth;
		pspritexscale = (((crvwidth >> detailxshift) / 2) << FRACBITS) / 160;
	}
	else
	{
		pspritexscale = centerxfrac / 160;
	}
	pspriteyscale = FixedMul(pspritexscale, yaspectmul);
	pspritexiscale = FixedDiv(FRACUNIT, pspritexscale);

	// [RH] Sky height fix for screens not 200 (or 240) pixels tall
	R_InitSkyMap ();

	// thing clipping
	for (i = 0; i < viewwidth; i++)
		screenheightarray[i] = (int)viewheight;

	// planes
	for (i = 0; i < viewwidth; i++)
	{
		fixed_t cosadj = abs (finecosine[xtoviewangle[i]>>ANGLETOFINESHIFT]);
		distscale[i] = FixedDiv (FRACUNIT, cosadj);
	}

	// Calculate the light levels to use for each level / scale combination.
	// [RH] This just stores indices into the colormap rather than pointers to a specific one.
	for (i = 0; i < LIGHTLEVELS; i++)
	{
		startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (j = 0; j < MAXLIGHTSCALE; j++)
		{
			level = startmap - (j*(screen->width>>detailxshift))/((viewwidth*DISTMAP));
			if (level < 0)
				level = 0;
			else if (level >= NUMCOLORMAPS)
				level = NUMCOLORMAPS-1;

			scalelight[i][j] = level;
		}
	}

	// [RH] Initialize z-light tables here
	R_InitLightTables ();
}

//
//
// CVAR screenblocks
//
// Selects the size of the visible window
//
//

CVAR_FUNC_IMPL (screenblocks)
{
	if (var > 12.0)
		var.Set (12.0);
	else if (var < 3.0)
		var.Set (3.0);
	else
		R_SetViewSize ((int)var);
}

//
//
// CVAR r_columnmethod
//
// Selects which version of the seg renderers to use.
//
//

// [ML] Disabled 16/3/06, now always 0 (Original)
// [Russell] Reenabled 14/3/07, fixes smudging of graphics
//BEGIN_CUSTOM_CVAR (r_columnmethod, "1", CVAR_ARCHIVE)
//{
    /*
	if (var != 0 && var != 1)
		var.Set (1);
	else
		// Trigger the change
		r_detail.Callback ();
    */
//}
//END_CUSTOM_CVAR (r_columnmethod)
EXTERN_CVAR (r_columnmethod)
//
//
// R_Init
//
//

void R_Init (void)
{
	R_InitData ();
	//R_InitPointToAngle ();
//	R_InitTables ();
	// viewwidth / viewheight are set by the defaults

	R_SetViewSize ((int)screenblocks);
	R_InitPlanes ();
	R_InitLightTables ();
	R_InitTranslationTables ();

	R_InitParticles ();	// [RH] Setup particle engine

	framecount = 0;
}

// R_Shutdown
void STACK_ARGS R_Shutdown (void)
{
    R_FreeTranslationTables();
}

//
//
// R_PointInSubsector
//
//

subsector_t *R_PointInSubsector (fixed_t x, fixed_t y)
{
	node_t *node;
	int side;
	int nodenum;

	// single subsector is a special case
	if (!numnodes)
		return subsectors;

	nodenum = numnodes-1;

	while (! (nodenum & NF_SUBSECTOR) )
	{
		node = &nodes[nodenum];
		side = R_PointOnSide (x, y, node);
		nodenum = node->children[side];
	}

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}

//
//
// R_SetupFrame
//
//

void R_SetupFrame (player_t *player)
{
	unsigned int newblend;

	camera = player->camera;	// [RH] Use camera instead of viewplayer

	if(!camera || !camera->subsector)
		return;

	if (player->cheats & CF_CHASECAM)
	{
		// [RH] Use chasecam view
		P_AimCamera (camera);
		viewx = CameraX;
		viewy = CameraY;
		viewz = CameraZ;
	}
	else
	{
		viewx = camera->x;
		viewy = camera->y;
		viewz = camera->player ? camera->player->viewz : camera->z;
	}

	viewangle = camera->angle + viewangleoffset;

	if (camera->player && camera->player->xviewshift && !paused)
	{
		int intensity = camera->player->xviewshift;
		viewx += ((M_Random() % (intensity<<2))
					-(intensity<<1))<<FRACBITS;
		viewy += ((M_Random()%(intensity<<2))
					-(intensity<<1))<<FRACBITS;
	}

	extralight = camera == player->mo ? player->extralight : 0;

	viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
	viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

	// killough 3/20/98, 4/4/98: select colormap based on player status
	// [RH] Can also select a blend

	if (camera->subsector->sector->heightsec &&
		!(camera->subsector->sector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{
		const sector_t *s = camera->subsector->sector->heightsec;
		newblend = viewz < P_FloorHeight(viewx, viewy, s) ? s->bottommap : 
					viewz > P_CeilingHeight(viewx, viewy, s) ? s->topmap : s->midmap;

		if (!screen->is8bit())
			newblend = R_BlendForColormap (newblend);
		else if (APART(newblend) == 0 && newblend >= numfakecmaps)
			newblend = 0;
	}
	else
	{
		newblend = 0;
	}

	// [RH] Don't override testblend unless entering a sector with a
	//		blend different from the previous sector's. Same goes with
	//		NormalLight's maps pointer.
	if (R_OldBlend != newblend)
	{
		R_OldBlend = newblend;
		if (APART(newblend))
		{
			BaseBlendR = RPART(newblend);
			BaseBlendG = GPART(newblend);
			BaseBlendB = BPART(newblend);
			BaseBlendA = APART(newblend) / 255.0f;
			NormalLight.maps = shaderef_t(&realcolormaps, 0);
		}
		else
		{
			NormalLight.maps = shaderef_t(&realcolormaps, (NUMCOLORMAPS+1)*newblend);
			BaseBlendR = BaseBlendG = BaseBlendB = 0;
			BaseBlendA = 0.0f;
		}

		V_SetBlend(BaseBlendR, BaseBlendG, BaseBlendB, (int)(BaseBlendA * 256));
	}

	fixedcolormap = shaderef_t();
	fixedlightlev = 0;
	palette_t *pal = GetDefaultPalette();

	if (camera == player->mo && player->fixedcolormap)
	{
		if (player->fixedcolormap < NUMCOLORMAPS)
		{
			fixedlightlev = player->fixedcolormap;
			fixedcolormap = shaderef_t(&pal->maps, 0);
		}
		else
		{
			fixedcolormap = shaderef_t(&pal->maps, player->fixedcolormap);
		}

		walllights = scalelightfixed;

		memset (scalelightfixed, 0, MAXLIGHTSCALE*sizeof(*scalelightfixed));
	}

	// [RH] freelook stuff
	{
		fixed_t dy = FixedMul (FocalLengthY, finetangent[(ANG90-camera->pitch)>>ANGLETOFINESHIFT]);

		centeryfrac = (viewheight << (FRACBITS-1)) + dy;
		centery = centeryfrac >> FRACBITS;

		//centeryfrac &= 0xffff0000;
		int i = lastcenteryfrac - centeryfrac;
		if (i != 0)
		{
			int e;

			if (i & ((FRACUNIT-1) == 0))	// Unlikely, but possible
			{
				i >>= FRACBITS;
				if (abs (i) < viewheight)
				{
					fixed_t *from, *to;
					if (i > 0)
					{
//						memmove (yslope, yslope + i, (viewheight - i) * sizeof(fixed_t));
						int index = 0;
						from = yslope + i;
						to = yslope;
						i = e = viewheight - i;
						do
						{
							*(to + index) = *(from + index);
							index++;
						} while (--e);
						e = viewheight;
					}
					else
					{
//						memmove (yslope - i, yslope, (viewheight + i) * sizeof(fixed_t));
						from = yslope;
						to = yslope - i;
						e = viewheight + i - 1;
						do
						{
							*(to + e) = *(from + e);
						} while (--e >= 0);
						e = -i;
						i = 0;
					}
				}
				else
				{
					i = 0;
					e = viewheight;
				}
			}
			else
			{
				i = 0;
				e = viewheight;
			}

			{
				fixed_t focus = FocalLengthY;
				fixed_t den;
				if (i < centery)
				{
					den = centeryfrac - (i << FRACBITS) - FRACUNIT/2;
					if (e <= centery)
					{
						do {
							yslope[i] = FixedDiv (focus, den);
							den -= FRACUNIT;
						} while (++i < e);
					}
					else
					{
						do {
							yslope[i] = FixedDiv (focus, den);
							den -= FRACUNIT;
						} while (++i < centery);
						den = (i << FRACBITS) - centeryfrac + FRACUNIT/2;
						do {
							yslope[i] = FixedDiv (focus, den);
							den += FRACUNIT;
						} while (++i < e);
					}
				}
				else
				{
					den = (i << FRACBITS) - centeryfrac + FRACUNIT/2;
					do {
						yslope[i] = FixedDiv (focus, den);
						den += FRACUNIT;
					} while (++i < e);
				}
			}
		}
		lastcenteryfrac = centeryfrac;
	}

	framecount++;
	validcount++;
}

//
// R_SetFlatDrawFuncs
//
// Sets the drawing function pointers to functions that floodfill with
// flat colors instead of texture mapping.
//
void R_SetFlatDrawFuncs()
{
	colfunc = R_FillColumnP;
	hcolfunc_pre = R_FillColumnHorizP;
	hcolfunc_post1 = rt_copy1col;
	hcolfunc_post2 = rt_copy2cols;
	hcolfunc_post4 = rt_copy4cols;
	spanfunc = R_FillSpan;
	spanslopefunc = R_FillSpan;
}

//
// R_SetBlankDrawFuncs
//
// Sets the drawing function pointers to functions that draw nothing.
// These can be used instead of the flat color functions for lucent midtex.
//
void R_SetBlankDrawFuncs()
{
	colfunc = R_BlankColumn;
	hcolfunc_pre = R_BlankColumn; 
	hcolfunc_post1 = rt_copy1col;
	hcolfunc_post2 = rt_copy2cols;
	hcolfunc_post4 = rt_copy4cols;
	spanfunc = spanslopefunc = R_BlankColumn;
}

//
// R_ResetDrawFuncs
//
void R_ResetDrawFuncs()
{
	if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = basecolfunc;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_map1col;
		hcolfunc_post2 = rt_map2cols;
		hcolfunc_post4 = rt_map4cols;
		spanfunc = R_DrawSpan;
		spanslopefunc = R_DrawSlopeSpan;
	}
}

void R_SetLucentDrawFuncs()
{
	if (r_drawflat)
	{
		R_SetBlankDrawFuncs();
	}
	else
	{
		colfunc = lucentcolfunc;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_lucent1col;
		hcolfunc_post2 = rt_lucent2cols;
		hcolfunc_post4 = rt_lucent4cols;
	}
}

void R_SetTranslatedDrawFuncs()
{
	if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = transcolfunc;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_tlate1col;
		hcolfunc_post2 = rt_tlate2cols;
		hcolfunc_post4 = rt_tlate4cols;
	}
}

void R_SetTranslatedLucentDrawFuncs()
{
	if (r_drawflat)
	{
		R_SetBlankDrawFuncs();
	}
	else
	{
		colfunc = tlatedlucentcolfunc;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_tlatelucent1col;
		hcolfunc_post2 = rt_tlatelucent2cols;
		hcolfunc_post4 = rt_tlatelucent4cols;
	}
}

//
//
// R_RenderView
//
//

void R_RenderPlayerView (player_t *player)
{
	R_SetupFrame (player);

	// Clear buffers.
	R_ClearClipSegs ();
	R_ClearDrawSegs ();
	R_ClearPlanes ();
	R_ClearSprites ();

	R_ResetDrawFuncs();

	// [RH] Hack to make windows into underwater areas possible
	r_fakingunderwater = false;

	// [RH] Setup particles for this frame
	R_FindParticleSubsectors();

    // [Russell] - From zdoom 1.22 source, added camera pointer check
	// Never draw the player unless in chasecam mode
	if (camera && camera->player && !(player->cheats & CF_CHASECAM))
	{
		camera->flags2 |= MF2_DONTDRAW;
		R_RenderBSPNode (numnodes - 1);
		camera->flags2 &= ~MF2_DONTDRAW;
	}
	else
		R_RenderBSPNode (numnodes-1);	// The head node is the last node output.

	R_DrawPlanes ();

	R_DrawMasked ();

	// [RH] Apply detail mode doubling
	R_DetailDouble ();

	// NOTE(jsd): Full-screen status color blending:
	extern int BlendA, BlendR, BlendG, BlendB;
	if (BlendA != 0)
		r_dimpatchD(screen, MAKERGB(newgamma[BlendR], newgamma[BlendG], newgamma[BlendB]), BlendA, 0, 0, screen->width, screen->height);
}

//
//
// R_MultiresInit
//
// Called from V_SetResolution()
//
//

void R_MultiresInit (void)
{
	int i;

	// in r_draw.c
	extern byte **ylookup;
	extern int *columnofs;

	// [Russell] - Possible bug, ylookup is 2 star.
    M_Free(ylookup);
    M_Free(columnofs);
    M_Free(negonearray);
    M_Free(screenheightarray);
    M_Free(xtoviewangle);

	ylookup = (byte **)M_Malloc (screen->height * sizeof(byte *));
	columnofs = (int *)M_Malloc (screen->width * sizeof(int));

	// Moved from R_InitSprites()
	negonearray = (int *)M_Malloc (sizeof(int) * screen->width);

	// These get set in R_ExecuteSetViewSize()
	screenheightarray = (int *)M_Malloc (sizeof(int) * screen->width);
	xtoviewangle = (angle_t *)M_Malloc (sizeof(angle_t) * (screen->width + 1));

	// GhostlyDeath -- Clean up the buffers
	memset(ylookup, 0, screen->height * sizeof(byte*));
	memset(columnofs, 0, screen->width * sizeof(int));

	for(i = 0; i < screen->width; i++)
	{
		negonearray[i] = -1;
	}
    memset(screenheightarray, 0, screen->width * sizeof(int));
    memset(xtoviewangle, 0, screen->width * sizeof(angle_t) + 1);

	R_InitFuzzTable ();
	R_PlaneInitData ();
	R_OldBlend = ~0;
}

VERSION_CONTROL (r_main_cpp, "$Id$")


