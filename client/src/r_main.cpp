// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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
#include "m_vectors.h"
#include "f_wipe.h"
#include "am_map.h"

void R_BeginInterpolation(fixed_t amount);
void R_EndInterpolation();
void R_InterpolateCamera(fixed_t amount);

#define DISTMAP			2

int negonearray[MAXWIDTH];
int viewheightarray[MAXWIDTH];

void R_SpanInitData ();

extern int *walllights;

// [RH] Defined in d_main.cpp
extern dyncolormap_t NormalLight;
extern bool r_fakingunderwater;

EXTERN_CVAR (r_flashhom)
EXTERN_CVAR (r_viewsize)
EXTERN_CVAR (sv_allowwidescreen)
EXTERN_CVAR (vid_320x200)
EXTERN_CVAR (vid_640x400)

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
int				centery;

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

static bool		setsizeneeded = true;
int				setblocks;

fixed_t			freelookviewheight;

// [RH] Base blending values (for e.g. underwater)
static argb_t sector_blend_color(0, 255, 255, 255);

// [SL] Current color blending values (including palette effects)
fargb_t blend_color(0.0f, 255.0f, 255.0f, 255.0f);

void (*colfunc) (void);
void (*spanfunc) (void);
void (*spanslopefunc) (void);

// [AM] Number of fineangles in a default 90 degree FOV at a 4:3 resolution.
int FieldOfView = 2048;
int CorrectFieldOfView = 2048;

fixed_t			render_lerp_amount;

static void R_InitViewWindow();


//
// R_ForceViewWindowResize
//
// Tells the renderer to recalculate all of the viewing window dimensions
// and any lookup tables dependent on those dimensions prior to rendering
// the next frame.
//
void R_ForceViewWindowResize()
{
	setsizeneeded = true;
}


//
// R_GetRenderingSurface
//
// Returns a pointer to the surface that the rendered scene is drawn on.
//
IWindowSurface* R_GetRenderingSurface()
{
	if ((vid_320x200 || vid_640x400) && I_GetEmulatedSurface() != NULL)
		return I_GetEmulatedSurface();
	else
		return I_GetPrimarySurface();
}


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
int R_PointOnSegSide(fixed_t x, fixed_t y, const seg_t *line)
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
// R_ClipLine
//
// Clips the endpoints of the line defined by in1 & in2 and stores the
// results in out1 & out2. The clipped left endpoint is lclip percent
// of the way between the left and right endpoints and similarly the
// right endpoint is rclip percent of the way between the left and right
// endpoints
//
void R_ClipLine(const v2fixed_t* in1, const v2fixed_t* in2, 
				int32_t lclip, int32_t rclip,
				v2fixed_t* out1, v2fixed_t* out2)
{
	const fixed_t dx = in2->x - in1->x;
	const fixed_t dy = in2->y - in1->y;
	const fixed_t x = in1->x;
	const fixed_t y = in1->y;
	out1->x = x + FixedMul30(lclip, dx);
	out2->x = x + FixedMul30(rclip, dx);
	out1->y = y + FixedMul30(lclip, dy);
	out2->y = y + FixedMul30(rclip, dy);
}

void R_ClipLine(const vertex_t* in1, const vertex_t* in2,
				int32_t lclip, int32_t rclip,
				v2fixed_t* out1, v2fixed_t* out2)
{
	R_ClipLine((const v2fixed_t*)in1, (const v2fixed_t*)in2, lclip, rclip, out1, out2);
}

//
// R_ClipLineToFrustum
//
// Clips the endpoints of a line to the view frustum.
// v1 and v2 are the endpoints of the line
// clipdist is the distance from the viewer to the near plane of the frustum.
// The left endpoint of the line should be clipped lclip percent of the way
// between the left and right endpoints and similarly the right endpoint
// should be clipped rclip percent of the way between the left and right
// endpoints.
//
// Returns false if the line is entirely clipped.
//
bool R_ClipLineToFrustum(const v2fixed_t* v1, const v2fixed_t* v2, fixed_t clipdist, int32_t& lclip, int32_t& rclip)
{
	static const int32_t CLIPUNIT = 1 << 30;
	v2fixed_t p1 = *v1, p2 = *v2;

	lclip = 0;
	rclip = CLIPUNIT; 

	// Clip portions of the line that are behind the view plane
	if (p1.y < clipdist)
	{      
		// reject the line entirely if the whole thing is behind the view plane.
		if (p2.y < clipdist)
			return false;

		// clip the line at the point where p1.y == clipdist
		lclip = FixedDiv30(clipdist - p1.y, p2.y - p1.y);
	}

	if (p2.y < clipdist)
	{
		// clip the line at the point where p2.y == clipdist
		rclip = FixedDiv30(clipdist - p1.y, p2.y - p1.y);
	}

	int32_t unclipped_amount = rclip - lclip;

	// apply the clipping against the 'y = clipdist' plane to p1 & p2
	R_ClipLine(v1, v2, lclip, rclip, &p1, &p2);

	// [SL] A note on clipping to the screen edges:
	// With a 90-degree FOV, if p1.x < -p1.y, then the left point
	// is off the left side of the screen. Similarly, if p2.x > p2.y,
	// then the right point is off the right side of the screen.
	// We use yc1 and yc2 instead of p1.y and p2.y because they are
	// adjusted to work with the current FOV rather than just 90-degrees.
	fixed_t yc1 = FixedMul(fovtan, p1.y);
	fixed_t yc2 = FixedMul(fovtan, p2.y);

	// is the entire line off the left side or the right side of the screen?
	if ((p1.x < -yc1 && p2.x < -yc2) || (p1.x > yc1 && p2.x > yc2))
		return false;

	// is the left vertex off the left side of the screen?
	if (p1.x < -yc1)
	{
		// clip line at left edge of the screen
		fixed_t den = p2.x - p1.x + yc2 - yc1;
		if (den == 0)
			return false;

		int32_t t = FixedDiv30(-yc1 - p1.x, den);
		lclip += FixedMul30(t, unclipped_amount);
	}

	// is the right vertex off the right side of the screen?
	if (p2.x > yc2)
	{
		// clip line at right edge of the screen
		fixed_t den = p2.x - p1.x - yc2 + yc1;	
		if (den == 0)
			return false;

		int32_t t = FixedDiv30(yc1 - p1.x, den);
		rclip -= FixedMul30(CLIPUNIT - t, unclipped_amount);
	}

	if (lclip > rclip)
		return false;

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
	x1 = MAX(x1, 0);
	x2 = MIN(x2, viewwidth - 1);
	return (x1 <= x2);
}

//
// R_CheckProjectionY
//
// Clamps the rows y1 and y2 to the viewing screen and returns false if
// the entire projection is off the screen.
//
bool R_CheckProjectionY(int &y1, int &y2)
{
	y1 = MAX(y1, 0);
	y2 = MIN(y2, viewheight - 1);
	return (y1 <= y2);
}


//
// R_DrawPixel
//
static inline void R_DrawPixel(int x, int y, byte color)
{
	byte* dest = dcol.destination + y * dcol.pitch_in_pixels + x;
	*dest = color;
}


//
// R_DrawLine
//
// Draws a colored line between the two endpoints given in world coordinates.
//
void R_DrawLine(const v3fixed_t* inpt1, const v3fixed_t* inpt2, byte color)
{
	// convert from world-space to camera-space
	v3fixed_t pt1, pt2;
	R_RotatePoint(inpt1->x - viewx, inpt1->y - viewy, ANG90 - viewangle, pt1.x, pt1.y);
	R_RotatePoint(inpt2->x - viewx, inpt2->y - viewy, ANG90 - viewangle, pt2.x, pt2.y);
	pt1.z = inpt1->z - viewz;
	pt2.z = inpt2->z - viewz;

	if (pt1.x > pt2.x)
	{
		std::swap(pt1.x, pt2.x);
		std::swap(pt1.y, pt2.y);
		std::swap(pt1.z, pt2.z);
	}

	// convert from camera-space to screen-space
	int lclip, rclip;

	if (!R_ClipLineToFrustum((v2fixed_t*)&pt1, (v2fixed_t*)&pt2, NEARCLIP, lclip, rclip))
		return;

	R_ClipLine((v2fixed_t*)&pt1, (v2fixed_t*)&pt2, lclip, rclip, (v2fixed_t*)&pt1, (v2fixed_t*)&pt2);

	int x1 = clamp(R_ProjectPointX(pt1.x, pt1.y), 0, viewwidth - 1);
	int x2 = clamp(R_ProjectPointX(pt2.x, pt2.y), 0, viewwidth - 1);
	int y1 = clamp(R_ProjectPointY(pt1.z, pt1.y), 0, viewheight - 1);
	int y2 = clamp(R_ProjectPointY(pt2.z, pt2.y), 0, viewheight - 1);

	// draw the line to the framebuffer
	int dx = x2 - x1;
	int dy = y2 - y1;

	int ax = 2 * (dx<0 ? -dx : dx);
	int sx = dx < 0 ? -1 : 1;

	int ay = 2 * (dy < 0 ? -dy : dy);
	int sy = dy < 0 ? -1 : 1;

	int x = x1;
	int y = y1;
	int d;

	if (ax > ay)
	{
		d = ay - ax/2;

		while (1)
		{
			R_DrawPixel(x, y, color);
			if (x == x2)
				return;

			if (d >= 0)
			{
				y += sy;
				d -= ax;
			}
			x += sx;
			d += ay;
		}
	}
	else
	{
		d = ax - ay/2;
		while (1)
		{
			R_DrawPixel(x, y, color);
			if (y == y2)
				return;		

			if (d >= 0)
			{
				x += sx;
				d -= ay;
			}
			y += sy;
			d += ax;
		}
	}
}



//
//
// R_SetViewSize
//
// Do not really change anything here, because it might be in the middle
// of a refresh. The change will take effect next refresh.
//
//
void R_SetViewSize(int blocks)
{
	R_ForceViewWindowResize();
	setblocks = blocks;
}


//
//
// CVAR screenblocks
//
// Selects the size of the visible window
//
//

CVAR_FUNC_IMPL(screenblocks)
{
	R_SetViewSize((int)var);
}


//
// R_Init
//
//
void R_Init()
{
	R_InitData();
	R_SetViewSize((int)screenblocks);
	R_InitPlanes();
	R_InitTranslationTables();

	R_InitParticles();	// [RH] Setup particle engine

	framecount = 0;
}


//
// R_Shutdown
//
void STACK_ARGS R_Shutdown()
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
// R_SetSectorBlend
//
// Sets the blend color for the camera's current sector to the given color.
//
void R_SetSectorBlend(const argb_t color)
{
	sector_blend_color = color;
}


//
// R_ClearSectorBlend
//
// Sets the blend color for the camera's current sector to white with 0% opacity.
//
void R_ClearSectorBlend()
{
	sector_blend_color.seta(0);
	sector_blend_color.setr(255);
	sector_blend_color.setg(255);
	sector_blend_color.setb(255);
}


//
// R_GetSectorBlend
//
// Returns the blend color for the camera's current sector.
//
argb_t R_GetSectorBlend()
{
	return sector_blend_color;
}


//
//
// R_SetupFrame
//
//
void R_SetupFrame (player_t *player)
{
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
		viewangle = viewangleoffset + camera->angle;
	}
	else
	{
		if (render_lerp_amount < FRACUNIT)
		{
			R_InterpolateCamera(render_lerp_amount);
		}
		else
		{
			viewx = camera->x;
			viewy = camera->y;
			viewz = camera->player ? camera->player->viewz : camera->z;
			viewangle = viewangleoffset + camera->angle;
		}
	}

	if (camera->player && camera->player->xviewshift && !paused)
	{
		int intensity = camera->player->xviewshift;
		viewx += ((M_Random() % (intensity << 2)) - (intensity << 1)) << FRACBITS;
		viewy += ((M_Random() % (intensity << 2)) - (intensity << 1)) << FRACBITS;
	}

	extralight = camera == player->mo ? player->extralight : 0;

	viewsin = finesine[viewangle>>ANGLETOFINESHIFT];
	viewcos = finecosine[viewangle>>ANGLETOFINESHIFT];

	// [SL] Change to a different sector blend color (or colormap in 8bpp mode)
	// if entering a heightsec (via TransferHeight line special)
	if (camera->subsector->sector->heightsec &&
		!(camera->subsector->sector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{
		const sector_t* sec = camera->subsector->sector->heightsec;

		argb_t new_sector_blend_color;
		if (viewz < P_FloorHeight(viewx, viewy, sec))
			new_sector_blend_color = sec->bottommap;
		else if (viewz > P_CeilingHeight(viewx, viewy, sec))
			new_sector_blend_color = sec->topmap;
		else
			new_sector_blend_color = sec->midmap;

		// [RH] Don't override testblend unless entering a sector with a
		//		blend different from the previous sector's. Same goes with
		//		NormalLight's maps pointer.
		if (new_sector_blend_color != R_GetSectorBlend())
		{
			R_SetSectorBlend(new_sector_blend_color);

			// use colormap lumps in 8bpp mode instead of blends
			int colormap_num = 0;
			if (I_GetPrimarySurface()->getBitsPerPixel() == 8)
				colormap_num = R_ColormapForBlend(new_sector_blend_color);

			NormalLight.maps = shaderef_t(&realcolormaps, (NUMCOLORMAPS+1) * colormap_num);
		}
	}
	else
	{
		R_ClearSectorBlend();
		NormalLight.maps = shaderef_t(&realcolormaps, 0);
	}

	fixedcolormap = shaderef_t();
	fixedlightlev = 0;
	const palette_t* pal = V_GetDefaultPalette();

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
	fixed_t pitch = camera->prevpitch + FixedMul(render_lerp_amount, camera->pitch - camera->prevpitch);
	R_ViewShear(pitch); 

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
	spanfunc = spanslopefunc = R_BlankSpan;
}

//
// R_ResetDrawFuncs
//
void R_ResetDrawFuncs()
{
	if (nodrawers)
	{
		R_SetBlankDrawFuncs();
	}
	else if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = R_DrawColumn;
		spanfunc = R_DrawSpan;
		spanslopefunc = R_DrawSlopeSpan;
	}
}

void R_SetFuzzDrawFuncs()
{
	if (nodrawers)
	{
		R_SetBlankDrawFuncs();
	}
	else if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = R_DrawFuzzColumn;
		spanfunc = R_DrawSpan;
		spanslopefunc = R_DrawSlopeSpan;
	}
}

void R_SetLucentDrawFuncs()
{
	if (nodrawers)
	{
		R_SetBlankDrawFuncs();
	}
	else if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = R_DrawTranslucentColumn;
	}
}

void R_SetTranslatedDrawFuncs()
{
	if (nodrawers)
	{
		R_SetBlankDrawFuncs();
	}
	else if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = R_DrawTranslatedColumn;
	}
}

void R_SetTranslatedLucentDrawFuncs()
{
	if (nodrawers)
	{
		R_SetBlankDrawFuncs();
	}
	else if (r_drawflat)
	{
		R_SetFlatDrawFuncs();
	}
	else
	{
		colfunc = R_DrawTlatedLucentColumn;
	}
}


//
// R_RenderPlayerView
//
void R_RenderPlayerView(player_t* player)
{
	// Recalculate the viewing window dimensions, if needed.
	if (setsizeneeded)
	{
		R_InitViewWindow();
		ST_ForceRefresh();
		setsizeneeded = false;
	}

	if (!viewactive)
		return;

	R_SetupFrame(player);

	// Clear buffers.
	R_ClearClipSegs();
	R_ClearDrawSegs();
	R_ClearOpenings();
	R_ClearPlanes();
	R_ClearSprites();

	R_ResetDrawFuncs();

	IWindowSurface* surface = R_GetRenderingSurface();

	// [SL] fill the screen with a blinking solid color to make HOM more visible
	if (r_flashhom)
	{
		argb_t color = gametic & 8 ? argb_t(0, 0, 0) : argb_t(0, 0, 255);
		int x1 = viewwindowx, y1 = viewwindowy;
		int x2 = viewwindowx + viewwidth - 1, y2 = viewwindowy + viewheight - 1; 

		surface->getDefaultCanvas()->Clear(x1, y1, x2, y2, color);
	}

	R_BeginInterpolation(render_lerp_amount);

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

	R_DrawPlanes();

	R_DrawMasked();

	// NOTE(jsd): Full-screen status color blending:
	int blend_alpha = int(blend_color.geta() * 255.0f);
	if (surface->getBitsPerPixel() == 32 && blend_alpha > 0)
	{
		r_dimpatchD(surface, V_GammaCorrect(blend_color), blend_alpha,
						viewwindowx, viewwindowy, viewwidth, viewheight);
	}

	R_EndInterpolation();
}


//
// R_InitLightTables
//
// Calculates the values for the scalelight and the zlight tables for seg
// and plane lighting.
//
static void R_InitLightTables(int surface_width, int surface_height)
{
	// Calculate the light levels to use for each level / scale combination.
	// [RH] This just stores indices into the colormap rather than pointers to a specific one.
	for (int i = 0; i < LIGHTLEVELS; i++)
	{
		int startmap = ((LIGHTLEVELS-1-i)*2) * NUMCOLORMAPS/LIGHTLEVELS;
		for (int j = 0; j < MAXLIGHTSCALE; j++)
		{
			int level = startmap - (j * surface_width) / ((viewwidth * DISTMAP));
			scalelight[i][j] = clamp(level, 0, NUMCOLORMAPS - 1);
		}
	}

	// Calculate the light levels to use for each level / distance combination.
	for (int i = 0; i < LIGHTLEVELS; i++)
	{
		int startmap = ((LIGHTLEVELS-1-i)*2)*NUMCOLORMAPS/LIGHTLEVELS;
		for (int j=0 ; j<MAXLIGHTZ ; j++)
		{
			int scale = FixedDiv(160*FRACUNIT, (j+1) << LIGHTZSHIFT);
			scale >>= LIGHTSCALESHIFT-LIGHTSCALEMULBITS;
			zlight[i][j] = clamp(startmap - scale/DISTMAP, 0, NUMCOLORMAPS - 1);
		}
	}

	lightscalexmul = (320 * (1 << LIGHTSCALEMULBITS)) / surface_height;
}


//
// R_ViewWidth
//
int R_ViewWidth(int width, int height)
{
	if (setblocks == 10 || setblocks == 11 || setblocks == 12)
		return width;
	else
		return (setblocks * width / 10) & ~15;
}


//
// R_ViewHeight
//
int R_ViewHeight(int width, int height)
{
	if (setblocks == 11 || setblocks == 12)
		return height;
	else if (setblocks == 10)
		return ST_StatusBarY(width, height);
	else
		return (setblocks * ST_StatusBarY(width, height) / 10) & ~7;
}


//
// R_ViewWindowX
//
int R_ViewWindowX(int width, int height)
{
	return (width - R_ViewWidth(width, height)) / 2;
}


//
// R_ViewWindowY
//
int R_ViewWindowY(int width, int height)
{
	if (setblocks == 10 || setblocks == 11 || setblocks == 12)
		return 0;
	else
		return (ST_StatusBarY(width, height) - R_ViewHeight(width, height)) / 2;
}


//
// R_InitViewWindow
//
// Initializes the renderer variables and tables that are dependent upon
// the surface dimensions and/or the size of the viewing window.
//
// This replaces R_ExecuteSetViewSize and R_MultiresInit.
//
static void R_InitViewWindow()
{
	IWindowSurface* surface = R_GetRenderingSurface();
	int surface_width = surface->getWidth(), surface_height = surface->getHeight();

	// using a 320x200/640x400 surface or video mode
	bool protected_res = vid_320x200 || vid_640x400
					|| I_IsProtectedResolution(I_GetVideoWidth(), I_GetVideoHeight());

	surface->lock();

	// Calculate viewwidth & viewheight based on the amount of window border 
	viewwidth = R_ViewWidth(surface_width, surface_height);
	viewheight = R_ViewHeight(surface_width, surface_height);
	viewwindowx = R_ViewWindowX(surface_width, surface_height);
	viewwindowy = R_ViewWindowY(surface_width, surface_height);

	if (setblocks == 10 || setblocks == 11 || setblocks == 12)
		freelookviewheight = surface_height; 
	else
		freelookviewheight = ((setblocks * surface_height) / 10) & ~7;

	centerx = viewwidth / 2;
	centery = viewheight / 2;
	centerxfrac = (viewwidth << FRACBITS) / 2;
	centeryfrac = (viewheight << FRACBITS) / 2;

	// calculate the vertical stretching factor to emulate 320x200
	// it's a 5:4 ratio = (320 / 200) / (4 / 3)
	if (protected_res)
		yaspectmul = FRACUNIT;
	else
		yaspectmul = 320 * 3 * FRACUNIT / (200 * 4);

	// Calculate FieldOfView and CorrectFieldOfView
	float desired_fov = 90.0f;
	if (consoleplayer().camera && consoleplayer().camera->player)
		desired_fov = clamp(consoleplayer().camera->player->fov, 45.0f, 135.0f);

	FieldOfView = int(desired_fov * FINEANGLES / 360.0f);

	if (V_UseWidescreen() || V_UseLetterBox())
	{
		float am = (3.0f * I_GetSurfaceWidth()) / (4.0f * I_GetSurfaceHeight());
		float radfov = desired_fov * PI / 180.0f;
		float widefov = (2 * atan(am * tan(radfov / 2))) * 180.0f / PI;
		CorrectFieldOfView = int(widefov * FINEANGLES / 360.0f);
	}
	else
 	{
		CorrectFieldOfView = FieldOfView;
 	}

	// Calculate focal length so CorrectFieldOfView angles covers viewwidth.
	fovtan = finetangent[FINEANGLES/4 + CorrectFieldOfView/2];
	FocalLengthX = FixedDiv(centerxfrac, fovtan);
	FocalLengthY = FixedDiv(FixedMul(centerxfrac, yaspectmul), fovtan);
	xfoc = FIXED2FLOAT(FocalLengthX);
	yfoc = FIXED2FLOAT(FocalLengthY);

	focratio = yfoc / xfoc;
	ifocratio = xfoc / yfoc;

	// psprite scales
	// [AM] Using centerxfrac will make our sprite too fat, so we
	//      generate a corrected 4:3 screen width based on our
	//      height, then generate the x-scale based on that.
	int cswidth, crvwidth;
	if (protected_res)
		cswidth = 320 * surface_height / 200;
	else
		cswidth = 4 * surface_height / 3;

	if (setblocks < 10)
		crvwidth = ((setblocks * cswidth) / 10) & ~15;
	else
		crvwidth = cswidth;

	pspritexscale = ((crvwidth / 2) * FRACUNIT) / 160;
	pspriteyscale = FixedMul(pspritexscale, yaspectmul);
	pspritexiscale = FixedDiv(FRACUNIT, pspritexscale);

	// Precalculate all row offsets.
	for (int i = 0; i < surface_width; i++)
	{
		negonearray[i] = -1;
		viewheightarray[i] = (int)viewheight;
	}

	R_InitLightTables(surface_width, surface_height);

	R_PlaneInitData(surface);
	R_InitSkyMap();

	dcol.destination = dspan.destination = surface->getBuffer(viewwindowx, viewwindowy);
	dcol.pitch_in_pixels = dspan.pitch_in_pixels = surface->getPitchInPixels();

	surface->unlock();

	char temp_str[16];
	sprintf(temp_str, "%d x %d", viewwidth, viewheight);
	r_viewsize.ForceSet(temp_str);

	// [SL] clear many renderer variables
	R_ExitLevel();
}


//
// R_BorderVisible
//
// Returns true if the border surrounding the view window should be drawn.
//
bool R_BorderVisible()
{
	return setblocks < 10 && !AM_ClassicAutomapVisible();
}


//
// R_StatusBarVisible
//
// Returns true if the status bar should be drawn below the view window.
//
bool R_StatusBarVisible()
{
	return setblocks <= 10 || AM_ClassicAutomapVisible();
}



//
// R_ExitLevel
//
// Resets internal renderer variables at the end of a level (or the start of
// a new level). This includes clearing any sector blends.
//
void R_ExitLevel()
{
	R_ClearSectorBlend();
	NormalLight.maps = shaderef_t(&realcolormaps, 0);

	blend_color = fargb_t(0.0f, 255.0f, 255.0f, 255.0f);
	V_ForceBlend(blend_color);

	r_underwater = false;
}

VERSION_CONTROL (r_main_cpp, "$Id$")


