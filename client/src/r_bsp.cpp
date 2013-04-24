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
//		BSP traversal, handling of LineSegs for rendering.
//
//-----------------------------------------------------------------------------


#include "m_alloc.h"
#include "doomdef.h"
#include "m_bbox.h"
#include "i_system.h"
#include "r_main.h"
#include "r_plane.h"
#include "r_draw.h"
#include "r_things.h"
#include "p_local.h"

// State.
#include "doomstat.h"
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

int				MaxDrawSegs;
drawseg_t		*drawsegs;
drawseg_t*		ds_p;

// Floor and ceiling heights at the end points of a seg_t
fixed_t			rw_backcz1, rw_backcz2;
fixed_t			rw_backfz1, rw_backfz2;
fixed_t			rw_frontcz1, rw_frontcz2;
fixed_t			rw_frontfz1, rw_frontfz2;

int rw_start, rw_stop;

static BYTE		FakeSide;

void R_StoreWallRange(int start, int stop);

//
// R_ClearDrawSegs
//
void R_ClearDrawSegs (void)
{
	if (!drawsegs)
	{
		MaxDrawSegs = 256;		// [RH] Default. Increased as needed.
		drawsegs = (drawseg_t *)Malloc (MaxDrawSegs * sizeof(drawseg_t));
	}
	ds_p = drawsegs;
}

//
// ClipWallSegment
// Clips the given range of columns
// and includes it in the new clip list.
//
//
// 1/11/98 killough: Since a type "short" is sufficient, we
// should use it, since smaller arrays fit better in cache.
//

typedef struct {
	short first, last;		// killough
} cliprange_t;

// 1/11/98: Lee Killough
//
// This fixes many strange venetian blinds crashes, which occurred when a scan
// line had too many "posts" of alternating non-transparent and transparent
// regions. Using a doubly-linked list to represent the posts is one way to
// do it, but it has increased overhead and poor spatial locality, which hurts
// cache performance on modern machines. Since the maximum number of posts
// theoretically possible is a function of screen width, a static limit is
// okay in this case. It used to be 32, which was way too small.
//
// This limit was frequently mistaken for the visplane limit in some Doom
// editing FAQs, where visplanes were said to "double" if a pillar or other
// object split the view's space into two pieces horizontally. That did not
// have anything to do with visplanes, but it had everything to do with these
// clip posts.

// newend is one past the last valid seg
static cliprange_t *newend;
static cliprange_t solidsegs[MAXWIDTH / 2 + 2];

//
// R_ClipSolidWallSegment
// Does handle solid walls,
//	e.g. single sided LineDefs (middle texture)
//	that entirely block the view.
//
void
R_ClipSolidWallSegment
( int			first,
  int			last )
{
	cliprange_t *next, *start;

	// Find the first range that touches the range
	//	(adjacent pixels are touching).
	start = solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start), so insert a new clippost.
			R_StoreWallRange (first, last);

			// 1/11/98 killough: performance tuning using fast memmove
			memmove (start+1, start, (++newend-start)*sizeof(*start));
			start->first = first;
			start->last = last;
			return;
		}

		// There is a fragment above *start.
		R_StoreWallRange (first, start->first - 1);

		// Now adjust the clip size.
		start->first = first;
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	next = start;
	while (last >= (next+1)->first-1)
	{
		// There is a fragment between two posts.
		R_StoreWallRange (next->last + 1, (next+1)->first - 1);
		next++;

		if (last <= next->last)
		{
			// Bottom is contained in next. Adjust the clip size.
			start->last = next->last;
			goto crunch;
		}
	}

	// There is a fragment after *next.
	R_StoreWallRange (next->last + 1, last);
	// Adjust the clip size.
	start->last = last;

	// Remove start+1 to next from the clip list,
	// because start now covers their area.
  crunch:
	if (next == start)
	{
		// Post just extended past the bottom of one post.
		return;
	}


	while (next++ != newend)
	{
		// Remove a post.
		*++start = *next;
	}

	newend = start+1;
}



//
// R_ClipPassWallSegment
// Clips the given range of columns,
//	but does not includes it in the clip list.
// Does handle windows,
//	e.g. LineDefs with upper and lower texture.
//
void
R_ClipPassWallSegment
( int	first,
  int	last )
{
	cliprange_t *start;

	// Find the first range that touches the range
	//	(adjacent pixels are touching).
	start = solidsegs;
	while (start->last < first-1)
		start++;

	if (first < start->first)
	{
		if (last < start->first-1)
		{
			// Post is entirely visible (above start).
			R_StoreWallRange (first, last);
			return;
		}

		// There is a fragment above *start.
		R_StoreWallRange (first, start->first - 1);
	}

	// Bottom contained in start?
	if (last <= start->last)
		return;

	while (last >= (start+1)->first-1)
	{
		// There is a fragment between two posts.
		R_StoreWallRange (start->last + 1, (start+1)->first - 1);
		start++;

		if (last <= start->last)
			return;
	}

	// There is a fragment after *next.
	R_StoreWallRange (start->last + 1, last);
}



//
// R_ClearClipSegs
//
void R_ClearClipSegs (void)
{
	solidsegs[0].first = -0x7fff;	// new short limit --  killough
	solidsegs[0].last = -1;
	solidsegs[1].first = viewwidth;
	solidsegs[1].last = 0x7fff;		// new short limit --  killough
	newend = solidsegs+2;
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
					 BOOL back)
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

	if (sec->heightsec && !(sec->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC))
	{
		if (!camera || !camera->subsector || !camera->subsector->sector)
			return sec;
		sector_t *heightsec = camera->subsector->sector->heightsec;

		const sector_t *s = sec->heightsec;

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
				return sec;
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
			if (rw_frontcz1 <= P_FloorHeight(curline->v1->x, curline->v1->y, s) &&
				rw_frontcz2 <= P_FloorHeight(curline->v2->x, curline->v2->y, s))
			{
				// Check that the window is actually visible
				for (int z = rw_start; z < rw_stop; z++)
				{
					if (floorclip[z] > ceilingclip[z])
					{
						doorunderwater = true;
						r_fakingunderwater = true;
						break;
					}
				}
			}
		}

		if (underwater || doorunderwater)
		{
			tempsec->floorplane = sec->floorplane;
			tempsec->ceilingplane = s->floorplane;
			P_InvertPlane(&tempsec->ceilingplane);
			P_ChangeCeilingHeight(tempsec, -1);
			tempsec->floorcolormap = s->floorcolormap;
			tempsec->ceilingcolormap = s->ceilingcolormap;
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

			tempsec->ceilingcolormap	= s->ceilingcolormap;
			tempsec->floorcolormap		= s->floorcolormap;

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
	}
	return sec;
}


//
// R_AddLine
// Clips the given segment
// and adds any visible pieces to the line list.
//
void R_AddLine (seg_t *line)
{
	int 			x1;
	int 			x2;
	angle_t 		angle1;
	angle_t 		angle2;
	angle_t 		span;
	angle_t 		tspan;
	static sector_t tempsec;	// killough 3/8/98: ceiling/water hack

	curline = line;

	// [RH] Color if not texturing line
	dc_color = ((line - segs) & 31) * 4;

	// OPTIMIZE: quickly reject orthogonal back sides.
	angle1 = R_PointToAngle (line->v1->x, line->v1->y);
	angle2 = R_PointToAngle (line->v2->x, line->v2->y);

	// Clip to view edges.
	// OPTIMIZE: make constant out of 2*clipangle (FIELDOFVIEW).
	span = angle1 - angle2;

	// Back side? I.e. backface culling?
	if (span >= ANG180)
		return;

	// Global angle needed by segcalc.
	rw_angle1 = angle1;
	angle1 -= viewangle;
	angle2 -= viewangle;

	tspan = angle1 + clipangle;
	if (tspan > 2*clipangle)
	{
		// Totally off the left edge?
		if (tspan - 2*clipangle >= span)
			return;

		angle1 = clipangle;
	}
	tspan = clipangle - angle2;
	if (tspan > 2*clipangle)
	{
		// Totally off the left edge?
		if (tspan - 2*clipangle >= span)
			return;
		angle2 = (unsigned) (-(int)clipangle);
	}

	// The seg is in the view range, but not necessarily visible.
	angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;

	// killough 1/31/98: Here is where "slime trails" can SOMETIMES occur:
	x1 = viewangletox[angle1];
	x2 = viewangletox[angle2];

	// Does not cross a pixel?
	if (x1 >= x2)	// killough 1/31/98 -- change == to >= for robustness
		return;

	rw_start = x1;
	rw_stop = x2 - 1;
	
	rw_frontcz1 = P_CeilingHeight(line->v1->x, line->v1->y, frontsector);
	rw_frontfz1 = P_FloorHeight(line->v1->x, line->v1->y, frontsector);
	rw_frontcz2 = P_CeilingHeight(line->v2->x, line->v2->y, frontsector);
	rw_frontfz2 = P_FloorHeight(line->v2->x, line->v2->y, frontsector);	
	
	backsector = line->backsector;
	// Single sided line?
	if (!backsector)
		goto clipsolid;

	// killough 3/8/98, 4/4/98: hack for invisible ceilings / deep water
	backsector = R_FakeFlat (backsector, &tempsec, NULL, NULL, true);
		
	rw_backcz1 = P_CeilingHeight(line->v1->x, line->v1->y, backsector);
	rw_backfz1 = P_FloorHeight(line->v1->x, line->v1->y, backsector);
	rw_backcz2 = P_CeilingHeight(line->v2->x, line->v2->y, backsector);
	rw_backfz2 = P_FloorHeight(line->v2->x, line->v2->y, backsector);

	// [SL] Check for closed doors or other scenarios that would make this
	// line seg solid.
	//
	// This fixes the automap floor height bug -- killough 1/18/98:
	// killough 4/7/98: optimize: save result in doorclosed for use in r_segs.c
	if (!(line->linedef->flags & ML_TWOSIDED) ||
		 (rw_backcz1 <= rw_frontfz1 && rw_backcz2 <= rw_frontfz2) ||
		 (rw_backfz1 >= rw_frontcz1 && rw_backfz2 >= rw_frontcz2) ||

		// handle a case where the backsector slopes one direction
		// and the frontsector slopes the opposite:
		(rw_backcz1 <= rw_frontfz1 && rw_backfz1 >= rw_frontcz1) ||
		(rw_backcz2 <= rw_frontfz2 && rw_backfz2 >= rw_frontcz2) ||

		// if door is closed because back is shut:
		((rw_backcz1 <= rw_backfz1 && rw_backcz2 <= rw_backfz2) &&
		
		// preserve a kind of transparent door/lift special effect:
		((rw_backcz1 >= rw_frontcz1 && rw_backcz2 >= rw_frontcz2) ||
		 line->sidedef->toptexture) &&
		
		((rw_backfz1 <= rw_frontfz1 && rw_backfz2 <= rw_frontfz2) ||
		 line->sidedef->bottomtexture) &&

		// properly render skies (consider door "open" if both ceilings are sky):
		 (backsector->ceilingpic !=skyflatnum || 
		  frontsector->ceilingpic!=skyflatnum)))
	{
		doorclosed = true;
		goto clipsolid;
	}
	else
	{
		doorclosed = false;
	}

	// Window.
	if (!P_IdenticalPlanes(&frontsector->ceilingplane, &backsector->ceilingplane) ||
		!P_IdenticalPlanes(&frontsector->floorplane, &backsector->floorplane))
		goto clippass;

	// Reject empty lines used for triggers
	//	and special events.
	// Identical floor and ceiling on both sides,
	// identical light levels on both sides,
	// and no middle texture.
	if (backsector->lightlevel == frontsector->lightlevel
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
		&& backsector->floorcolormap == frontsector->floorcolormap
		&& backsector->ceilingcolormap == frontsector->ceilingcolormap

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

  clippass:
	R_ClipPassWallSegment (x1, x2-1);
	return;

  clipsolid:
	R_ClipSolidWallSegment (x1, x2-1);
}


//
// R_CheckBBox
// Checks BSP node/subtree bounding box.
// Returns true
//	if some part of the bbox might be visible.
//
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


static BOOL R_CheckBBox (fixed_t *bspcoord)	// killough 1/28/98: static
{
	int 				boxx;
	int 				boxy;
	int 				boxpos;

	fixed_t 			x1;
	fixed_t 			y1;
	fixed_t 			x2;
	fixed_t 			y2;

	angle_t 			angle1;
	angle_t 			angle2;
	angle_t 			span;
	angle_t 			tspan;

	cliprange_t*		start;

	int 				sx1;
	int 				sx2;

	// Find the corners of the box
	// that define the edges from current viewpoint.
	if (viewx <= bspcoord[BOXLEFT])
		boxx = 0;
	else if (viewx < bspcoord[BOXRIGHT])
		boxx = 1;
	else
		boxx = 2;

	if (viewy >= bspcoord[BOXTOP])
		boxy = 0;
	else if (viewy > bspcoord[BOXBOTTOM])
		boxy = 1;
	else
		boxy = 2;

	boxpos = (boxy<<2)+boxx;
	if (boxpos == 5)
		return true;

	x1 = bspcoord[checkcoord[boxpos][0]];
	y1 = bspcoord[checkcoord[boxpos][1]];
	x2 = bspcoord[checkcoord[boxpos][2]];
	y2 = bspcoord[checkcoord[boxpos][3]];

	// check clip list for an open space
	angle1 = R_PointToAngle (x1, y1) - viewangle;
	angle2 = R_PointToAngle (x2, y2) - viewangle;

	span = angle1 - angle2;

	// Sitting on a line?
	if (span >= ANG180)
		return true;

	tspan = angle1 + clipangle;

	if (tspan > 2*clipangle)
	{
		tspan -= 2*clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle1 = clipangle;
	}
	tspan = clipangle - angle2;
	if (tspan > 2*clipangle)
	{
		tspan -= 2*clipangle;

		// Totally off the left edge?
		if (tspan >= span)
			return false;

		angle2 = (unsigned)(-(int)clipangle);
	}

	// Find the first clippost
	//	that touches the source post
	//	(adjacent pixels are touching).
	angle1 = (angle1+ANG90)>>ANGLETOFINESHIFT;
	angle2 = (angle2+ANG90)>>ANGLETOFINESHIFT;
	sx1 = viewangletox[angle1];
	sx2 = viewangletox[angle2];

	// Does not cross a pixel.
	if (sx1 == sx2)
		return false;
	sx2--;

	start = solidsegs;
	while (start->last < sx2)
		start++;

        if (sx1 >= start->first
            && sx2 <= start->last)
	{
		// The clippost contains the new span.
		return false;
	}

	return true;
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

	basecolormap = frontsector->ceilingcolormap->maps;

	ceilingplane = P_CeilingHeight(camera) > viewz ||
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

	basecolormap = frontsector->floorcolormap->maps;	// [RH] set basecolormap

	// killough 3/7/98: Add (x,y) offsets to flats, add deep water check
	// killough 3/16/98: add floorlightlevel
	// killough 10/98: add support for skies transferred from sidedefs
	floorplane = P_FloorHeight(camera) < viewz || // killough 3/7/98
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
	foggy = level.fadeto || frontsector->floorcolormap->fade
						 || frontsector->ceilingcolormap->fade;

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
		{
			R_AddLine (*polySeg++);
		}
	}
	
	while (count--)
	{
		R_AddLine (line++);
	}
}




//
// RenderBSPNode
// Renders all subsectors below a given node,
//	traversing subtree recursively.
// Just call with BSP root.
// killough 5/2/98: reformatted, removed tail recursion

void R_RenderBSPNode (int bspnum)
{
	while (!(bspnum & NF_SUBSECTOR))  // Found a subsector?
	{
		node_t *bsp = &nodes[bspnum];

		// Decide which side the view point is on.
		int side = R_PointOnSide(viewx, viewy, bsp);

		// Recursively divide front space.
		R_RenderBSPNode(bsp->children[side]);

		// Possibly divide back space.
		if (!R_CheckBBox(bsp->bbox[side^1]))
			return;

		bspnum = bsp->children[side^1];
	}
	R_Subsector(bspnum == -1 ? 0 : bspnum & ~NF_SUBSECTOR);
}

VERSION_CONTROL (r_bsp_cpp, "$Id$")

