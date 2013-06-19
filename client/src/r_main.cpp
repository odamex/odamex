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

int negonearray[MAXWIDTH];
int viewheightarray[MAXWIDTH];

void R_SpanInitData ();

extern int *walllights;

// [RH] Defined in d_main.cpp
extern dyncolormap_t NormalLight;
extern bool r_fakingunderwater;

EXTERN_CVAR (r_columnmethod)
EXTERN_CVAR (r_flashhom)
EXTERN_CVAR (r_viewsize)
EXTERN_CVAR (r_widescreen)
EXTERN_CVAR (sv_allowwidescreen)

static float	LastFOV = 0.0f;
fixed_t			FocalLengthX;
fixed_t			FocalLengthY;
float			xfoc;		// FIXEDFLOAT(FocalLengthX)
float			yfoc;		// FIXEDFLOAT(FocalLengthY)
fixed_t			fovtan;
float			focratio;
float			ifocratio;
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
void (*spanfunc) (void);
void (*spanslopefunc) (void);

void (*hcolfunc_pre) (void);
void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
void (*hcolfunc_post4) (int sx, int yl, int yh);

// [AM] Number of fineangles in a default 90 degree FOV at a 4:3 resolution.
int FieldOfView = 2048;
int CorrectFieldOfView = 2048;

//
//
// R_PointOnSide
//
// Determines which side of a line the point (x, y) is on.
// Returns side 0 (front) or 1 (back) 
//
int R_PointOnSide(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh)
{
	return int64_t(xh - xl) * (y - yl) - int64_t(yh - yl) * (x - xl) >= 0;
}

//
//
// R_PointOnSide
//
// Traverse BSP (sub) tree, check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
// [SL] 2013-02-06 - Changed to use cross product a la ZDoom
//
int R_PointOnSide(fixed_t x, fixed_t y, const node_t *node)
{
	return R_PointOnSide(x, y, node->x, node->y, node->x + node->dx, node->y + node->dy); 
}

//
//
// R_PointOnSegSide
//
// Same, except takes a lineseg as input instead of a linedef
//
// killough 5/2/98: reformatted
// [SL] 2013-02-06 - Changed to use cross product a la ZDoom
//
int R_PointOnSegSide (fixed_t x, fixed_t y, const seg_t *line)
{
	return R_PointOnSide(x, y, line->v1->x, line->v1->y, line->v2->x, line->v2->y);
}


//
// R_PointOnLine
//
// Determines if a point is sitting on a line.
//
bool R_PointOnLine(fixed_t x, fixed_t y, fixed_t xl, fixed_t yl, fixed_t xh, fixed_t yh)
{
	return int64_t(xh - xl) * (y - yl) - int64_t(yh - yl) * (x - xl) == 0;
}


//
// R_PointToAngle
//
// To get a global angle from cartesian coordinates, the coordinates are
// flipped until they are in the first octant of the coordinate system,
// then the y (<=x) is scaled and divided by x to get a tangent (slope)
// value which is looked up in the tantoangle[] table.
//
// This version is from prboom-plus
//
angle_t R_PointToAngle2(fixed_t viewx, fixed_t viewy, fixed_t x, fixed_t y)
{
	return (y -= viewy, (x -= viewx) || y) ?
		x >= 0 ?
			y >= 0 ?
				(x > y) ? tantoangle[SlopeDiv(y,x)] :						// octant 0
					ANG90-1-tantoangle[SlopeDiv(x,y)] :						// octant 1
				x > (y = -y) ? 0-tantoangle[SlopeDiv(y,x)] :				// octant 8
						ANG270+tantoangle[SlopeDiv(x,y)] :					// octant 7
			y >= 0 ? (x = -x) > y ? ANG180-1-tantoangle[SlopeDiv(y,x)] :	// octant 3
							ANG90 + tantoangle[SlopeDiv(x,y)] :				// octant 2
				(x = -x) > (y = -y) ? ANG180+tantoangle[ SlopeDiv(y,x)] :	// octant 4
								ANG270-1-tantoangle[SlopeDiv(x,y)] :		// octant 5
		0;
}

//
// R_PointToAngle - wrapper around R_PointToAngle2
//
angle_t R_PointToAngle(fixed_t x, fixed_t y)
{
	return R_PointToAngle2(viewx, viewy, x, y);
}

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
fixed_t R_PointToDist(fixed_t x, fixed_t y)
{
	fixed_t dx = abs(x - viewx);
	fixed_t dy = abs(y - viewy);

	if (dy > dx)
	{
		fixed_t t = dx;
		dx = dy;
		dy = t;
	}

	return FixedDiv(dx, finecosine[tantoangle[FixedDiv(dy, dx) >> DBITS] >> ANGLETOFINESHIFT]);
}

//
//
// R_PointToDist2
//
//
fixed_t R_PointToDist2(fixed_t dx, fixed_t dy)
{
	dx = abs (dx);
	dy = abs (dy);

	if (dy > dx)
	{
		fixed_t t = dx;
		dx = dy;
		dy = t;
	}

	return FixedDiv(dx, finecosine[tantoangle[FixedDiv(dy, dx) >> DBITS] >> ANGLETOFINESHIFT]);
}


void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
	int index = ang >> ANGLETOFINESHIFT;
	
	tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
	ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}

//
// R_ClipLineToFrustum
//
// Clips the endpoints of a line to the view frustum.
// (px1, py1) and (px2, py2) are the endpoints of the line
// clipdist is the distance from the viewer to the near plane of the frustum.
//
// Returns false if the line is entirely clipped.
//
bool R_ClipLineToFrustum(fixed_t &px1, fixed_t &py1, fixed_t &px2, fixed_t &py2, fixed_t clipdist)
{
	fixed_t den, t;

	// Clip portions of the line that are behind the view plane
	if (py1 < clipdist)
	{      
		// reject the line entirely if the whole thing is behind the view plane.
		if (py2 < clipdist)
			return false;

		// clip the line at the point where t1.y == clipdist
		t = FixedDiv(clipdist - py1, py2 - py1);
		px1 = px1 + FixedMul(t, px2 - px1);
		py1 = clipdist;
	}

	if (py2 < clipdist)
	{
		// clip the line at the point where t2.y == clipdist
		t = FixedDiv(clipdist - py1, py2 - py1);
		px2 = px1 + FixedMul(t, px2 - px1);
		py2 = clipdist;
	}

	// clip line at right edge of the screen

	// is the entire line off the right side of the screen?
	if (px1 > FixedMul(fovtan, py1))
		return false;

	den = px2 - px1 - FixedMul(fovtan, py2 - py1);	
	if (den == 0)
		return false;

	t = FixedDiv(FixedMul(fovtan, py1) - px1, den);

	if (t > 0 && t < FRACUNIT)
	{
		px2 = px1 + FixedMul(t, px2 - px1);
		py2 = py1 + FixedMul(t, py2 - py1);
	}

	// clip line at left edge of the screen

	// is the entire line off the left side of the screen?
	if (px2 < FixedMul(-fovtan, py2))
		return false;

	den = px2 - px1 - FixedMul(-fovtan, py2 - py1);
	if (den == 0)
		return false;

	t = FixedDiv(FixedMul(-fovtan, py1) - px1, den);

	if (t > 0 && t < FRACUNIT)
	{
		px1 = px1 + FixedMul(t, px2 - px1);
		py1 = py1 + FixedMul(t, py2 - py1);
	}

	return true;
}

//
// R_ProjectPointX
//
// Returns the screen column that a point in camera-space projects to.
//
int R_ProjectPointX(fixed_t x, fixed_t y)
{
	if (y > 0)
		return FIXED2INT(centerxfrac + int64_t(FocalLengthX) * int64_t(x) / int64_t(y));
	else
		return centerx;
}

//
// R_ProjectPointY
//
// Returns the screen row that a point in camera-space projects to.
//
int R_ProjectPointY(fixed_t z, fixed_t y)
{
	if (y > 0)
		return FIXED2INT(centeryfrac - int64_t(FocalLengthY) * int64_t(z) / int64_t(y));
	else
		return centery;
}

//
// R_CheckProjectionX
//
// Clamps the columns x1 and x2 to the viewing screen and returns false if
// the entire projection is off the screen.
//
bool R_CheckProjectionX(int &x1, int &x2)
{
	x1 = MAX<int>(x1, 0);
	x2 = MIN<int>(x2, viewwidth - 1);

	if (x1 > x2 || x2 < 0 || x1 >= viewwidth)
		return false;
	return true;
}

//
// R_CheckProjectionY
//
// Clamps the rows y1 and y2 to the viewing screen and returns false if
// the entire projection is off the screen.
//
bool R_CheckProjectionY(int &y1, int &y2)
{
	y1 = MAX<int>(y1, 0);
	y2 = MIN<int>(y2, viewheight - 1);

	if (y1 > y2 || y2 < 0 || y1 >= viewheight)
		return false;
	return true;
}


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
	fovtan = hitan; 

	// Calc focallength so FieldOfView angles covers viewwidth.
	FocalLengthX = FixedDiv (centerxfrac, hitan);
	FocalLengthY = FixedDiv (FixedMul (centerxfrac, yaspectmul), hitan);
	xfoc = FIXED2FLOAT(FocalLengthX);
	yfoc = FIXED2FLOAT(FocalLengthY);

	focratio = yfoc / xfoc;
	ifocratio = xfoc / yfoc;

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
/*
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
*/

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

	for (int i = 0; i < screen->width; i++)
	{
		negonearray[i] = -1;
		viewheightarray[i] = (int)viewheight;
	}

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

	// allocate for the array that indicates if a screen column is fully solid
	delete [] solidcol;
	solidcol = new byte[viewwidth];

	R_PlaneInitData ();

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
// R_Init
//
//

void R_Init (void)
{
	R_InitData ();
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
// R_ViewShear
//
// Sets centeryfrac, centery, and fills the yslope array based on the given
// pitch. This allows the y-shearing freelook approximation.
//
static void R_ViewShear(angle_t pitch)
{
	fixed_t dy = FixedMul(FocalLengthY, finetangent[(ANG90 - pitch) >> ANGLETOFINESHIFT]);

	centeryfrac = (viewheight << (FRACBITS - 1)) + dy;
	centery = centeryfrac >> FRACBITS;

	int e = viewheight, i = 0;
	fixed_t focus = FocalLengthY;
	fixed_t den;

	if (i < centery)
	{
		den = centeryfrac - (i << FRACBITS) - FRACUNIT / 2;

		if (e <= centery)
		{
			do {
				yslope[i] = FixedDiv(focus, den);
				den -= FRACUNIT;
			} while (++i < e);
		}
		else
		{
			do {
				yslope[i] = FixedDiv(focus, den);
				den -= FRACUNIT;
			} while (++i < centery);

			den = (i << FRACBITS) - centeryfrac + FRACUNIT / 2;

			do {
				yslope[i] = FixedDiv(focus, den);
				den += FRACUNIT;
			} while (++i < e);
		}
	}
	else
	{
		den = (i << FRACBITS) - centeryfrac + FRACUNIT / 2;

		do {
			yslope[i] = FixedDiv(focus, den);
			den += FRACUNIT;
		} while (++i < e);
	}
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

	if (!camera || !camera->subsector)
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
	static angle_t last_pitch = 0xFFFFFFFF;
	if (camera->pitch != last_pitch)
		R_ViewShear(camera->pitch);

	// [RH] Hack to make windows into underwater areas possible
	r_fakingunderwater = false;

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
	colfunc = R_FillColumn;
	// palettized version of hcolfunc_pre is ok because the columns are written
	// to a temporary 8bpp buffer, which is later copied to the screen with the
	// appropriate 8bpp or 32bpp function. 
	hcolfunc_pre = R_FillColumnHorizP;
	hcolfunc_post1 = rt_copy1col;
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
	hcolfunc_post4 = rt_copy4cols;
	spanfunc = spanslopefunc = R_BlankSpan;
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
		colfunc = R_DrawColumn;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_map1col;
		hcolfunc_post4 = rt_map4cols;
		spanfunc = R_DrawSpan;
		spanslopefunc = R_DrawSlopeSpan;
	}
}

void R_SetFuzzDrawFuncs()
{
	if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = R_DrawFuzzColumn;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_map1col;
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
		colfunc = R_DrawTranslucentColumn;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_lucent1col;
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
		colfunc = R_DrawTranslatedColumn;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_tlate1col;
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
		colfunc = R_DrawTlatedLucentColumn;
		hcolfunc_pre = R_DrawColumnHoriz;
		hcolfunc_post1 = rt_tlatelucent1col;
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
	R_ClearOpenings();
	R_ClearPlanes ();
	R_ClearSprites ();

	R_ResetDrawFuncs();

	// [SL] fill the screen with a blinking solid color to make HOM more visible
	if (r_flashhom)
	{
		int color = gametic & 8 ? 0 : 200;

		int x1 = viewwindowx;
		int y1 = viewwindowy;
		int x2 = viewwindowx + viewwidth - 1;
		int y2 = viewwindowy + viewheight - 1; 

		if (screen->is8bit())
			screen->Clear(x1, y1, x2, y2, color);
		else
			screen->Clear(x1, y1, x2, y2, basecolormap.shade(color));
	}

	// [RH] Setup particles for this frame
	R_FindParticleSubsectors();

    // [Russell] - From zdoom 1.22 source, added camera pointer check
	// Never draw the player unless in chasecam mode
	if (camera && camera->player && !(player->cheats & CF_CHASECAM))
	{
		int flags2_backup = camera->flags2;
		camera->flags2 |= MF2_DONTDRAW;
		R_RenderBSPNode(numnodes - 1);
		camera->flags2 = flags2_backup; 
	}
	else
		R_RenderBSPNode(numnodes - 1);	// The head node is the last node output.

	R_DrawPlanes ();

	R_DrawMasked ();

	// [RH] Apply detail mode doubling
	R_DetailDouble ();

	// NOTE(jsd): Full-screen status color blending:
	extern int BlendA, BlendR, BlendG, BlendB;
	if (BlendA != 0)
	{
		unsigned int blend_rgb = MAKERGB(newgamma[BlendR], newgamma[BlendG], newgamma[BlendB]);
		r_dimpatchD(screen, blend_rgb, BlendA, 0, 0, screen->width, screen->height);
	}
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
	// in r_draw.c
	extern byte **ylookup;
	extern int *columnofs;

	// [Russell] - Possible bug, ylookup is 2 star.
    M_Free(ylookup);
    M_Free(columnofs);
    M_Free(xtoviewangle);

	ylookup = (byte **)M_Malloc (screen->height * sizeof(byte *));
	columnofs = (int *)M_Malloc (screen->width * sizeof(int));

	// These get set in R_ExecuteSetViewSize()
	xtoviewangle = (angle_t *)M_Malloc (sizeof(angle_t) * (screen->width + 1));

	// GhostlyDeath -- Clean up the buffers
	memset(ylookup, 0, screen->height * sizeof(byte*));
	memset(columnofs, 0, screen->width * sizeof(int));

    memset(xtoviewangle, 0, screen->width * sizeof(angle_t) + 1);

	R_InitFuzzTable ();
	R_OldBlend = ~0;
}

VERSION_CONTROL (r_main_cpp, "$Id$")


