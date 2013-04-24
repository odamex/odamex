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
#include "c_cvars.h"
#include "v_video.h"
#include "z_zone.h"

#define DISTMAP			2

void R_SpanInitData ();

extern int *walllights;
extern dyncolormap_t NormalLight;

// [Russell] - Server expects these to exist
// [Russell] - Doesn't get used serverside
byte *translationtables;

fixed_t			FocalLengthX;
fixed_t			FocalLengthY;
int 			viewangleoffset;

// increment every time a check is made
int 			validcount = 1;

// [RH] colormap currently drawing with
lighttable_t	*basecolormap;
int				fixedlightlev;
lighttable_t	*fixedcolormap;

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

int				lightscalexmul;	// [RH] used to keep hires modes dark enough
int				lightscaleymul;

// bumped light from gun blasts
int			extralight;

// [RH] ignore extralight and fullbright
BOOL		foggy;

fixed_t			freelookviewheight;

unsigned int	R_OldBlend = ~0;

void (*colfunc) (void);
void (*basecolfunc) (void);
void (*fuzzcolfunc) (void);
void (*lucentcolfunc) (void);
void (*transcolfunc) (void);
void (*tlatedlucentcolfunc) (void);
void (*spanfunc) (void);

void (*hcolfunc_pre) (void);
void (*hcolfunc_post1) (int hx, int sx, int yl, int yh);
void (*hcolfunc_post2) (int hx, int sx, int yl, int yh);
void (*hcolfunc_post4) (int sx, int yl, int yh);

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

void R_RotatePoint(fixed_t x, fixed_t y, angle_t ang, fixed_t &tx, fixed_t &ty)
{
	int index = ang >> ANGLETOFINESHIFT;
	
	tx = FixedMul(x, finecosine[index]) - FixedMul(y, finesine[index]);
	ty = FixedMul(x, finesine[index]) + FixedMul(y, finecosine[index]);
}

//
//
// R_InitTables
//
//
#if 0
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
// R_Init
//
//

void R_Init (void)
{
	R_InitData ();
	//R_InitPointToAngle ();
	//R_InitTables ();

	framecount = 0;
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


VERSION_CONTROL (r_main_cpp, "$Id$")

