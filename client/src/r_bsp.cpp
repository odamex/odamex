// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//		BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <math.h>
#include "m_alloc.h"
#include "m_bbox.h"
#include "i_system.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_draw.h"
#include "r_things.h"
#include "p_local.h"
#include "m_vectors.h"

// State.
#include "r_state.h"
#include "v_palette.h"

EXTERN_CVAR (r_particles)

seg_t*			curline;
side_t* 		sidedef;
line_t* 		linedef;
sector_t*		frontsector;
sector_t*		backsector;

// killough 4/7/98: indicates doors closed wrt automap bugfix:
bool			doorclosed;

bool			r_fakingunderwater;
bool			r_underwater;

// Floor and ceiling heights at the end points of a seg_t
fixed_t			rw_backcz1, rw_backcz2;
fixed_t			rw_backfz1, rw_backfz2;
fixed_t			rw_frontcz1, rw_frontcz2;
fixed_t			rw_frontfz1, rw_frontfz2;

int rw_start, rw_stop;

static BYTE		FakeSide;

const fixed_t NEARCLIP = 2*FRACUNIT;

drawseg_t*		ds_p;
drawseg_t*		drawsegs;
unsigned		maxdrawsegs;

// CPhipps -
// Instead of clipsegs, let's try using an array with one entry for each column,
// indicating whether it's blocked by a solid wall yet or not.
// e6y: resolution limitation is removed
byte			solidcol[MAXWIDTH];

//
// R_ClearClipSegs
//
void R_ClearClipSegs (void)
{
	memset(solidcol, 0, viewwidth);
}

//
// R_ReallocDrawSegs
//
// [SL] From prboom-plus. Moved out of R_StoreWallRange()
void R_ReallocDrawSegs(void)
{
	if (ds_p == drawsegs+maxdrawsegs)		// killough 1/98 -- fix 2s line HOM
	{
		unsigned pos = ds_p - drawsegs;	// jff 8/9/98 fix from ZDOOM1.14a
		unsigned newmax = maxdrawsegs ? maxdrawsegs*2 : 128; // killough
		drawsegs = (drawseg_t*)Realloc(drawsegs, newmax*sizeof(*drawsegs));
		ds_p = drawsegs + pos;				// jff 8/9/98 fix from ZDOOM1.14a
		maxdrawsegs = newmax;
		DPrintf("MaxDrawSegs increased to %d\n", maxdrawsegs);
	}
}

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs(void)
{
	ds_p = drawsegs;
}

//
// R_ClipWallSegment
//
// [SL] From prboom-plus. Modified for clarity.
//
// Calls R_StoreWallRange for each span of non-solid range of columns that
// is within the range of first to (and including) last. Non-solid columns
// are those which have not yet had a 1s lineseg drawn to them. If makesolid
// is specified, any range of non-solid columns found will be marked as solid.
//
static void R_ClipWallSegment(int first, int last, bool makesolid)
{
	while (first <= last)
	{
		if (solidcol[first])
		{
			// find the first remaining non-solid column
			// if all columns remaining are solid, we're done
			byte* p = (byte*)memchr(solidcol + first, 0, last - first + 1);
			if (p == NULL)
				return; 

			first = p - solidcol;
		}
		else
		{
			int to;
			// find where the span of non-solid columns ends
			byte* p = (byte*)memchr(solidcol + first, 1, last - first + 1);
			if (p == NULL)
				to = last;
			else
				to = p - solidcol - 1;

			// set the range for this wall to the range of non-solid columns
			R_StoreWallRange(first, to);

			// mark the  columns as solid
			if (makesolid)
				memset(solidcol + first, 1, to - first + 1);

			first = to + 1;
		}
	}
}

bool CopyPlaneIfValid (plane_t *dest, const plane_t *source, const plane_t *opp)
{
	bool copy = false;

	// If the planes do not have matching slopes, then always copy them
	// because clipping would require creating new sectors.
	if (source->a != dest->a || source->b != dest->b || source->c != dest->c)
	{
		copy = true;
	}
	else if (opp->a != -dest->a || opp->b != -dest->b || opp->c != -dest->c)
	{
		if (source->d < dest->d)
		{
			copy = true;
		}
	}
	else if (source->d < dest->d && source->d > -opp->d)
	{
		copy = true;
	}

	if (copy)
	{
		memcpy(dest, source, sizeof(plane_t));
	}

	return copy;
}

//
// killough 3/7/98: Hack floor/ceiling heights for deep water etc.
//
// If player's view height is underneath fake floor, lower the
// drawn ceiling to be just under the floor height, and replace
// the drawn floor and ceiling textures, and light level, with
// the control sector's.
//
// Similar for ceiling, only reflected.
//
// killough 4/11/98, 4/13/98: fix bugs, add 'back' parameter
//

sector_t *R_FakeFlat(sector_t *sec, sector_t *tempsec,
					 int *floorlightlevel, int *ceilinglightlevel,
					 bool back)
{
	// [RH] allow per-plane lighting
	if (floorlightlevel != NULL)
	{
		*floorlightlevel = sec->floorlightsec == NULL ?
			sec->lightlevel : sec->floorlightsec->lightlevel;
	}

	if (ceilinglightlevel != NULL)
	{
		*ceilinglightlevel = sec->ceilinglightsec == NULL ? // killough 4/11/98
			sec->lightlevel : sec->ceilinglightsec->lightlevel;
	}

	FakeSide = FAKED_Center;

	if (!sec->heightsec || sec->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC)
		return sec;
	if (!camera || !camera->subsector || !camera->subsector->sector)
		return sec;

	const sector_t* s = sec->heightsec;
	sector_t *heightsec = camera->subsector->sector->heightsec;

	bool underwater = r_fakingunderwater ||
		(heightsec && viewz <= P_FloorHeight(viewx, viewy, heightsec));
	bool doorunderwater = false;
	int diffTex = (s->MoreFlags & SECF_CLIPFAKEPLANES);

	// Replace sector being drawn with a copy to be hacked
	*tempsec = *sec;

	// Replace floor and ceiling height with control sector's heights.
	if (diffTex)
	{
		if (CopyPlaneIfValid (&tempsec->floorplane, &s->floorplane, &sec->ceilingplane))
			tempsec->floorpic = s->floorpic;
		else if (s->MoreFlags & SECF_FAKEFLOORONLY)
		{
			if (underwater)
			{
				tempsec->colormap = s->colormap;

				if (!(s->MoreFlags & SECF_NOFAKELIGHT))
				{
					tempsec->lightlevel = s->lightlevel;

					if (floorlightlevel != NULL)
					{
						*floorlightlevel = s->floorlightsec == NULL ?
							s->lightlevel : s->floorlightsec->lightlevel;
					}

					if (ceilinglightlevel != NULL)
					{
						*ceilinglightlevel = s->ceilinglightsec == NULL ?
							s->lightlevel : s->ceilinglightsec->lightlevel;
					}
				}

				FakeSide = FAKED_BelowFloor;
				return tempsec;
			}
			return sec;
		}
	}
	else
	{
		tempsec->floorplane = s->floorplane;
	}

	if (!(s->MoreFlags & SECF_FAKEFLOORONLY))
	{
		if (diffTex)
		{
			if (CopyPlaneIfValid (&tempsec->ceilingplane, &s->ceilingplane, &sec->floorplane))
				tempsec->ceilingpic = s->ceilingpic;
		}
		else
		{
			tempsec->ceilingplane  = s->ceilingplane;
		}
	}

	fixed_t refceilz = P_CeilingHeight(viewx, viewy, s);
	fixed_t orgceilz = P_CeilingHeight(viewx, viewy, sec);

	// [RH] Allow viewing underwater areas through doors/windows that
	// are underwater but not in a water sector themselves.
	// Only works if you cannot see the top surface of any deep water
	// sectors at the same time.

	if (back && !r_fakingunderwater && curline->frontsector->heightsec == NULL)
	{
		fixed_t fcz1 = P_CeilingHeight(curline->v1->x, curline->v1->y, frontsector);
		fixed_t fcz2 = P_CeilingHeight(curline->v2->x, curline->v2->y, frontsector);

		if (fcz1 <= P_FloorHeight(curline->v1->x, curline->v1->y, s) &&
			fcz2 <= P_FloorHeight(curline->v2->x, curline->v2->y, s))
		{
			// will any columns of this window be visible or will they be blocked
			// by 1s lines and closed doors?
			if (memchr(solidcol + rw_start, 0, rw_stop - rw_start + 1) != NULL)
			{	
				doorunderwater = true;
				r_fakingunderwater = true;
			}
		}
	}

	if (underwater || doorunderwater)
	{
		tempsec->floorplane = sec->floorplane;
		tempsec->ceilingplane = s->floorplane;
		P_InvertPlane(&tempsec->ceilingplane);
		P_ChangeCeilingHeight(tempsec, -1);
		tempsec->colormap = s->colormap;
	}

	// killough 11/98: prevent sudden light changes from non-water sectors:
	if ((underwater && !back) || doorunderwater)
	{					// head-below-floor hack
		tempsec->floorpic			= diffTex ? sec->floorpic : s->floorpic;
		tempsec->floor_xoffs		= s->floor_xoffs;
		tempsec->floor_yoffs		= s->floor_yoffs;
		tempsec->floor_xscale		= s->floor_xscale;
		tempsec->floor_yscale		= s->floor_yscale;
		tempsec->floor_angle		= s->floor_angle;
		tempsec->base_floor_angle	= s->base_floor_angle;
		tempsec->base_floor_yoffs	= s->base_floor_yoffs;

		tempsec->ceilingplane		= s->floorplane;
		P_InvertPlane(&tempsec->ceilingplane);
		P_ChangeCeilingHeight(tempsec, -1);
		if (s->ceilingpic == skyflatnum)
		{
			tempsec->floorplane			= tempsec->ceilingplane;
			P_InvertPlane(&tempsec->floorplane);
			P_ChangeFloorHeight(tempsec, +1);
			tempsec->ceilingpic			= tempsec->floorpic;
			tempsec->ceiling_xoffs		= tempsec->floor_xoffs;
			tempsec->ceiling_yoffs		= tempsec->floor_yoffs;
			tempsec->ceiling_xscale		= tempsec->floor_xscale;
			tempsec->ceiling_yscale		= tempsec->floor_yscale;
			tempsec->ceiling_angle		= tempsec->floor_angle;
			tempsec->base_ceiling_angle	= tempsec->base_floor_angle;
			tempsec->base_ceiling_yoffs	= tempsec->base_floor_yoffs;
		}
		else
		{
			tempsec->ceilingpic			= diffTex ? s->floorpic : s->ceilingpic;
			tempsec->ceiling_xoffs		= s->ceiling_xoffs;
			tempsec->ceiling_yoffs		= s->ceiling_yoffs;
			tempsec->ceiling_xscale		= s->ceiling_xscale;
			tempsec->ceiling_yscale		= s->ceiling_yscale;
			tempsec->ceiling_angle		= s->ceiling_angle;
			tempsec->base_ceiling_angle	= s->base_ceiling_angle;
			tempsec->base_ceiling_yoffs	= s->base_ceiling_yoffs;
		}

		if (!(s->MoreFlags & SECF_NOFAKELIGHT))
		{
			tempsec->lightlevel = s->lightlevel;

			if (floorlightlevel != NULL)
			{
				*floorlightlevel = s->floorlightsec == NULL ?
					s->lightlevel : s->floorlightsec->lightlevel;
			}

			if (ceilinglightlevel != NULL)
			{
				*ceilinglightlevel = s->ceilinglightsec == NULL ?
					s->lightlevel : s->ceilinglightsec->lightlevel;
			}
		}
		FakeSide = FAKED_BelowFloor;
	}
	else if (heightsec && viewz >= P_CeilingHeight(viewx, viewy, heightsec) &&
			 orgceilz > refceilz && !(s->MoreFlags & SECF_FAKEFLOORONLY))
	{	// Above-ceiling hack
		tempsec->ceilingplane		= s->ceilingplane;
		tempsec->floorplane			= s->ceilingplane;
		P_InvertPlane(&tempsec->floorplane);
		P_ChangeFloorHeight(tempsec, +1);

		tempsec->colormap			= s->colormap;

		tempsec->ceilingpic = diffTex ? sec->ceilingpic : s->ceilingpic;
		tempsec->floorpic											= s->ceilingpic;
		tempsec->floor_xoffs		= tempsec->ceiling_xoffs		= s->ceiling_xoffs;
		tempsec->floor_yoffs		= tempsec->ceiling_yoffs		= s->ceiling_yoffs;
		tempsec->floor_xscale		= tempsec->ceiling_xscale		= s->ceiling_xscale;
		tempsec->floor_yscale		= tempsec->ceiling_yscale		= s->ceiling_yscale;
		tempsec->floor_angle		= tempsec->ceiling_angle		= s->ceiling_angle;
		tempsec->base_floor_angle	= tempsec->base_ceiling_angle	= s->base_ceiling_angle;
		tempsec->base_floor_yoffs	= tempsec->base_ceiling_yoffs	= s->base_ceiling_yoffs;

		if (s->floorpic != skyflatnum)
		{
			tempsec->ceilingplane	= sec->ceilingplane;
			tempsec->floorpic		= s->floorpic;
			tempsec->floor_xoffs	= s->floor_xoffs;
			tempsec->floor_yoffs	= s->floor_yoffs;
			tempsec->floor_xscale	= s->floor_xscale;
			tempsec->floor_yscale	= s->floor_yscale;
			tempsec->floor_angle	= s->floor_angle;
		}

		if (!(s->MoreFlags & SECF_NOFAKELIGHT))
		{
			tempsec->lightlevel  = s->lightlevel;

			if (floorlightlevel != NULL)
			{
				*floorlightlevel = s->floorlightsec == NULL ?
					s->lightlevel : s->floorlightsec->lightlevel;
			}

			if (ceilinglightlevel != NULL)
			{
				*ceilinglightlevel = s->ceilinglightsec == NULL ?
					s->lightlevel : s->ceilinglightsec->lightlevel;
			}
		}
		FakeSide = FAKED_AboveCeiling;
	}
	sec = tempsec;					// Use other sector

	return sec;
}


//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
void R_AddLine (seg_t *line)
{
	curline = line;

	// skip this line if it's not facing the camera
	if (R_PointOnSegSide(viewx, viewy, line) != 0)
		return;

	dcol.color = ((line - segs) & 31) * 4;	// [RH] Color if not texturing line

	// translate the line seg endpoints from world-space to camera-space
	// and store in (t1.x, t1.y) and (t2.x, t2.y)
	v2fixed_t t1, t2;
	R_RotatePoint(line->v1->x - viewx, line->v1->y - viewy, ANG90 - viewangle, t1.x, t1.y);
	R_RotatePoint(line->v2->x - viewx, line->v2->y - viewy, ANG90 - viewangle, t2.x, t2.y);

	// Clip the line seg to the viewing window
	int32_t lclip, rclip;
	if (!R_ClipLineToFrustum(&t1, &t2, NEARCLIP, lclip, rclip))
		return;

	// apply the view frustum clipping to t1 & t2
	R_ClipLine(&t1, &t2, lclip, rclip, &t1, &t2);

	// project the line endpoints to determine which columns the line seg occupies
	int x1 = R_ProjectPointX(t1.x, t1.y);
	int x2 = R_ProjectPointX(t2.x, t2.y) - 1;
	if (!R_CheckProjectionX(x1, x2))
		return;

	rw_start = x1;
	rw_stop = x2;

	// clip the line seg endpoints in world-space
	// and store in (w1.x, w1.y) and (w2.x, w2.y)
	v2fixed_t w1, w2;
	R_ClipLine(line->v1, line->v2, lclip, rclip, &w1, &w2);

	// killough 3/8/98, 4/4/98: hack for invisible ceilings / deep water
	static sector_t tempsec;
	backsector = line->backsector ? R_FakeFlat(line->backsector, &tempsec, NULL, NULL, true) : NULL;

	R_PrepWall(w1.x, w1.y, w2.x, w2.y, t1.y, t2.y, x1, x2);

	// [SL] Check for single-sided line, closed doors or other scenarios that
	// would make this line seg solid.
	//
	// This fixes the automap floor height bug -- killough 1/18/98:
	// killough 4/7/98: optimize: save result in doorclosed for use in r_segs.c

	if (!backsector || !(line->linedef->flags & ML_TWOSIDED) ||
		(rw_backcz1 <= rw_frontfz1 && rw_backcz2 <= rw_frontfz2) ||
		(rw_backfz1 >= rw_frontcz1 && rw_backfz2 >= rw_frontcz2) ||

		// if door is closed because back is shut:
		((rw_backcz1 <= rw_backfz1 && rw_backcz2 <= rw_backfz2) &&
		
		// preserve a kind of transparent door/lift special effect:
		((rw_backcz1 >= rw_frontcz1 && rw_backcz2 >= rw_frontcz2) ||
		 line->sidedef->toptexture) &&
		
		((rw_backfz1 <= rw_frontfz1 && rw_backfz2 <= rw_frontfz2) ||
		 line->sidedef->bottomtexture) &&

		// properly render skies (consider door "open" if both ceilings are sky):
		(backsector->ceilingpic !=skyflatnum || frontsector->ceilingpic!=skyflatnum)))
	{
		doorclosed = true;
		R_ClipWallSegment(x1, x2, true);
		return;
	}

	// Reject empty lines used for triggers and special events.
	// Identical floor and ceiling on both sides,
	// identical light levels on both sides, and no middle texture.
	if (P_IdenticalPlanes(&frontsector->ceilingplane, &backsector->ceilingplane)
		&& P_IdenticalPlanes(&frontsector->floorplane, &backsector->floorplane)
		&& backsector->lightlevel == frontsector->lightlevel
		&& backsector->floorpic == frontsector->floorpic
		&& backsector->ceilingpic == frontsector->ceilingpic
		&& curline->sidedef->midtexture == 0

		// killough 3/7/98: Take flats offsets into account:
		&& backsector->floor_xoffs == frontsector->floor_xoffs
		&& (backsector->floor_yoffs + backsector->base_floor_yoffs) == (frontsector->floor_yoffs + backsector->base_floor_yoffs)
		&& backsector->ceiling_xoffs == frontsector->ceiling_xoffs
		&& (backsector->ceiling_yoffs + backsector->base_ceiling_yoffs) == (frontsector->ceiling_yoffs + frontsector->base_ceiling_yoffs)

		// killough 4/16/98: consider altered lighting
		&& backsector->floorlightsec == frontsector->floorlightsec
		&& backsector->ceilinglightsec == frontsector->ceilinglightsec

		// [RH] Also consider colormaps
		&& backsector->colormap == frontsector->colormap

		// [RH] and scaling
		&& backsector->floor_xscale == frontsector->floor_xscale
		&& backsector->floor_yscale == frontsector->floor_yscale
		&& backsector->ceiling_xscale == frontsector->ceiling_xscale
		&& backsector->ceiling_yscale == frontsector->ceiling_yscale

		// [RH] and rotation
		&& (backsector->floor_angle + backsector->base_floor_angle) == (frontsector->floor_angle + frontsector->base_floor_angle)
		&& (backsector->ceiling_angle + backsector->base_ceiling_angle) == (frontsector->ceiling_angle + frontsector->base_ceiling_angle)
		)
	{
		return;
	}

	doorclosed = false;
	R_ClipWallSegment(x1, x2, false);
}


static const int checkcoord[12][4] = // killough -- static const
{
	{3,0,2,1},
	{3,0,2,0},
	{3,1,2,0},
	{0},
	{2,0,2,1},
	{0,0,0,0},
	{3,1,3,0},
	{0},
	{2,0,3,1},
	{2,1,3,1},
	{2,1,3,0}
};

//
// R_CheckBBox
//
// Checks BSP node/subtree bounding box.
// Returns true if some part of the bbox might be visible.
//
// killough 1/28/98: static
// CPhipps - const parameter, reformatted
// [SL] Rewritten to use R_ClipLineToFrustum to determine if any part of the
//      bbox's two diagonals would be drawn in a non-solid screen column.
//
static bool R_CheckBBox(const fixed_t *bspcoord)
{
	const fixed_t clipdist = 0;
	v2fixed_t t1, t2;

	// Find the corners of the box that define the edges from current viewpoint
	int boxpos = (viewx <= bspcoord[BOXLEFT] ? 0 : viewx < bspcoord[BOXRIGHT ] ? 1 : 2) +
				(viewy >= bspcoord[BOXTOP ] ? 0 : viewy > bspcoord[BOXBOTTOM] ? 4 : 8);

	if (boxpos == 5)
		return true;

	fixed_t xl = bspcoord[checkcoord[boxpos][0]];
	fixed_t yl = bspcoord[checkcoord[boxpos][1]];
	fixed_t xh = bspcoord[checkcoord[boxpos][2]];
	fixed_t yh = bspcoord[checkcoord[boxpos][3]];

	// translate the bounding box vertices from world-space to camera-space
	// and store in (t1.x, t1.y) and (t2.x, t2.y)
	R_RotatePoint(xl - viewx, yl - viewy, ANG90 - viewangle, t1.x, t1.y);
	R_RotatePoint(xh - viewx, yh - viewy, ANG90 - viewangle, t2.x, t2.y);

	v2fixed_t box_pts[4][2];
	// top line of box
	box_pts[0][0].x = t1.x;	box_pts[0][0].y = t1.y;	box_pts[0][1].x = t2.x;	box_pts[0][1].y = t1.y;
	// bottom line of box
	box_pts[1][0].x = t1.x;	box_pts[1][0].y = t2.y;	box_pts[1][1].x = t2.x;	box_pts[1][1].y = t2.y;
	// left line of box;
	box_pts[2][0].x = t1.x;	box_pts[2][0].y = t1.y;	box_pts[2][1].x = t1.x;	box_pts[2][1].y = t2.y;
	// right line of box
	box_pts[3][0].x = t2.x;	box_pts[3][0].y = t1.y;	box_pts[3][1].x = t2.x;	box_pts[3][1].y = t2.y;

	// Check each of the four sides of the bounding box to see if
	// any part is visible. Find the maximum range of columns the bounding box
	// will project onto the screen.
	for (int i = 0; i < 4; i++)
	{
		v2fixed_t p1 = box_pts[i][0];
		v2fixed_t p2 = box_pts[i][1];
		
		if (R_PointOnLine(0, 0, p1.x, p1.y, p2.x, p2.y))
			return true;

		int32_t lclip, rclip;
		if (R_ClipLineToFrustum(&p1, &p2, clipdist, lclip, rclip))
		{
			R_ClipLine(&p1, &p2, lclip, rclip, &p1, &p2);
			int x1 = R_ProjectPointX(p1.x, p1.y);
			int x2 = R_ProjectPointX(p2.x, p2.y) - 1;
			if (R_CheckProjectionX(x1, x2))
			{
				if (memchr(solidcol + x1, 0, x2 - x1 + 1) != NULL)	
					return true;
			}
		}
	}

	return false;
}

//
// R_Subsector
// Determine floor/ceiling planes.
// Add sprites of things in sector.
// Draw one or more line segments.
//
void R_Subsector (int num)
{
	int 		 count;
	seg_t*		 line;
	subsector_t *sub;
	sector_t     tempsec;				// killough 3/7/98: deep water hack
	int          floorlightlevel;		// killough 3/16/98: set floor lightlevel
	int          ceilinglightlevel;		// killough 4/11/98

#ifdef RANGECHECK
    if (num>=numsubsectors)
		I_Error ("R_Subsector: ss %i with numss = %i",
				 num,
				 numsubsectors);
#endif

	sub = &subsectors[num];
	frontsector = sub->sector;
	count = sub->numlines;
	line = &segs[sub->firstline];

	// killough 3/8/98, 4/4/98: Deep water / fake ceiling effect
	frontsector = R_FakeFlat(frontsector, &tempsec, &floorlightlevel,
						   &ceilinglightlevel, false);	// killough 4/11/98

	basecolormap = frontsector->colormap->maps;

	ceilingplane = P_CeilingHeight(viewx, viewy, frontsector) > viewz ||
		frontsector->ceilingpic == skyflatnum ||
		(frontsector->heightsec && 
		!(frontsector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) && 
		frontsector->heightsec->floorpic == skyflatnum) ?
		R_FindPlane(frontsector->ceilingplane,		// killough 3/8/98
					frontsector->ceilingpic == skyflatnum &&  // killough 10/98
						frontsector->sky & PL_SKYFLAT ? frontsector->sky :
						frontsector->ceilingpic,
					ceilinglightlevel,				// killough 4/11/98
					frontsector->ceiling_xoffs,		// killough 3/7/98
					frontsector->ceiling_yoffs + frontsector->base_ceiling_yoffs,
					frontsector->ceiling_xscale,
					frontsector->ceiling_yscale,
					frontsector->ceiling_angle + frontsector->base_ceiling_angle
					) : NULL;

	// killough 3/7/98: Add (x,y) offsets to flats, add deep water check
	// killough 3/16/98: add floorlightlevel
	// killough 10/98: add support for skies transferred from sidedefs
	floorplane = P_FloorHeight(viewx, viewy, frontsector) < viewz || // killough 3/7/98
		(frontsector->heightsec &&
		!(frontsector->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
		frontsector->heightsec->ceilingpic == skyflatnum) ?
		R_FindPlane(frontsector->floorplane,
					frontsector->floorpic == skyflatnum &&  // killough 10/98
						frontsector->sky & PL_SKYFLAT ? frontsector->sky :
						frontsector->floorpic,
					floorlightlevel,				// killough 3/16/98
					frontsector->floor_xoffs,		// killough 3/7/98
					frontsector->floor_yoffs + frontsector->base_floor_yoffs,
					frontsector->floor_xscale,
					frontsector->floor_yscale,
					frontsector->floor_angle + frontsector->base_floor_angle
					) : NULL;

	// [RH] set foggy flag
	foggy = level.fadeto_color[0] || level.fadeto_color[1] || level.fadeto_color[2] ||
	        level.fadeto_color[3] || frontsector->colormap->fade.getr() ||
	        frontsector->colormap->fade.getg() || frontsector->colormap->fade.getb();

	// killough 9/18/98: Fix underwater slowdown, by passing real sector
	// instead of fake one. Improve sprite lighting by basing sprite
	// lightlevels on floor & ceiling lightlevels in the surrounding area.
	R_AddSprites (sub->sector, (floorlightlevel + ceilinglightlevel) / 2, FakeSide);

	// [RH] Add particles
	if (r_particles)
	{
		for (WORD i = ParticlesInSubsec[num]; i != NO_PARTICLE; i = Particles[i].nextinsubsector)
			R_ProjectParticle(Particles + i, subsectors[num].sector, FakeSide);
	}		

	if (sub->poly)
	{ // Render the polyobj in the subsector first
		int polyCount = sub->poly->numsegs;
		seg_t **polySeg = sub->poly->segs;
		while (polyCount--)
			R_AddLine (*polySeg++);
	}
	
	while (count--)
		R_AddLine (line++);
}


//
// RenderBSPNode
//
// Renders all subsectors below a given node, traversing subtree recursively.
// Just call with BSP root.
// killough 5/2/98: reformatted, removed tail recursion
//
void R_RenderBSPNode (int bspnum)
{
	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		node_t *bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		int frontside = R_PointOnSide(viewx, viewy, bsp);
		int backside = frontside ^ 1;

		// Recursively divide front space.
		R_RenderBSPNode(bsp->children[frontside]);

		// Possibly divide back space.
		if (!R_CheckBBox(bsp->bbox[backside]))
			return;

		bspnum = bsp->children[backside];
	}

	R_Subsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}


VERSION_CONTROL (r_bsp_cpp, "$Id$")
