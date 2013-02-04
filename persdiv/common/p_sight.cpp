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
//	LineOfSight/Visibility checks, uses REJECT Lookup Table.
//  [RH] Switched over to Hexen's p_sight.c
//  [ML] 9/10/10 - With their powers combined...
//
//-----------------------------------------------------------------------------


#include "doomdef.h"

#include "i_system.h"
#include "p_local.h"
#include "m_random.h"
#include "m_bbox.h"
#include "vectors.h"

// State.
#include "r_state.h"

//
// P_CheckSight
//
fixed_t		sightzstart;		// eye z of looker
fixed_t		topslope;
fixed_t		bottomslope;		// slopes to top and bottom of target

divline_t	strace;			// from t1 to t2
fixed_t		t2x;
fixed_t		t2y;

int		sightcounts[2];
int		sightcounts2[3];

extern bool HasBehavior;
EXTERN_CVAR (co_zdoomphys)

/*
==============
=
= PTR_SightTraverse
=
==============
*/

bool PTR_SightTraverse (intercept_t *in)
{
	line_t  *li;
	fixed_t slope;

	if (!in->isaline)
		I_Error ("PTR_SightTraverse: non-line intercept\n");

	li = in->d.line;
	
	if (!li->backsector)
        return false;

//
// crosses a two sided line
//
	fixed_t crossx = trace.x + FixedMul(trace.dx, in->frac);
	fixed_t crossy = trace.y + FixedMul(trace.dy, in->frac);	
	P_LineOpening(li, crossx, crossy);

	if (openbottom >= opentop)		// quick test for totally closed doors
		return false;	// stop

	if (P_FloorHeight(crossx, crossy, li->frontsector) !=
		P_FloorHeight(crossx, crossy, li->backsector))
	{
		slope = FixedDiv (openbottom - sightzstart , in->frac);
		if (slope > bottomslope)
			bottomslope = slope;
	}

	if (P_CeilingHeight(crossx, crossy, li->frontsector) !=
		P_CeilingHeight(crossx, crossy, li->backsector))
	{
		slope = FixedDiv (opentop - sightzstart , in->frac);
		if (slope < topslope)
			topslope = slope;
	}

	if (topslope <= bottomslope)
		return false;	// stop

	return true;	// keep going
}

/*
==================
=
= P_SightBlockLinesIterator
=
===================
*/

bool P_SightBlockLinesIterator (int x, int y)
{
	int offset;
	int *list;
	line_t *ld;
	int s1, s2;
	divline_t dl;
	
	polyblock_t *polyLink;
	seg_t **segList;
	int i;
	extern polyblock_t **PolyBlockMap;

	offset = y*bmapwidth+x;

	polyLink = PolyBlockMap[offset];
	
	while(polyLink)
	{
		if(polyLink->polyobj)
		{ // only check non-empty links
			if(polyLink->polyobj->validcount != validcount)
			{
				segList = polyLink->polyobj->segs;
				for(i = 0; i < polyLink->polyobj->numsegs; i++, segList++)
				{
					ld = (*segList)->linedef;
					if(ld->validcount == validcount)
					{
						continue;
					}
					ld->validcount = validcount;
					s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
					s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
					if (s1 == s2)
						continue;		// line isn't crossed
					P_MakeDivline (ld, &dl);
					s1 = P_PointOnDivlineSide (trace.x, trace.y, &dl);
					s2 = P_PointOnDivlineSide (trace.x+trace.dx, trace.y+trace.dy, &dl);
					if (s1 == s2)
						continue;		// line isn't crossed

				// try to early out the check
					if (!ld->backsector)
						return false;	// stop checking

				// store the line for later intersection testing
					intercept_t intercept;
					intercept.d.line = ld;
					intercept.isaline = true;
					intercepts.Push(intercept);
				}
				polyLink->polyobj->validcount = validcount;
			}
		}
		polyLink = polyLink->next;
	}

	offset = *(blockmap + (bmapwidth*y + x));

	for (list = blockmaplump + offset; *list != -1; list++)
	{
		ld = &lines[*list];
		if (ld->validcount == validcount)
			continue;				// line has already been checked
		ld->validcount = validcount;

		s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
		s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
		if (s1 == s2)
			continue;				// line isn't crossed
		P_MakeDivline (ld, &dl);
		s1 = P_PointOnDivlineSide (trace.x, trace.y, &dl);
		s2 = P_PointOnDivlineSide (trace.x+trace.dx, trace.y+trace.dy, &dl);
		if (s1 == s2)
			continue;				// line isn't crossed

	// try to early out the check
		if (!ld->backsector)
			return false;	// stop checking

	// store the line for later intersection testing
       	intercept_t intercept;
       	intercept.d.line = ld;
		intercept.isaline = true;
       	intercepts.Push(intercept);
	}

	return true;			// everything was checked
}
/*
====================
=
= P_SightTraverseIntercepts
=
= Returns true if the traverser function returns true for all lines
====================
*/

bool P_SightTraverseIntercepts ( void )
{
	size_t  count = intercepts.Size();
	fixed_t dist;
	size_t	scan;
	intercept_t *in = 0;
	divline_t dl;
//
// calculate intercept distance
//
	for (scan = 0 ; scan < intercepts.Size(); scan++)
	{
		if (!intercepts[scan].isaline)
			I_Error ("P_SightTraverseIntercepts: non-line intercept\n");

		P_MakeDivline (intercepts[scan].d.line, &dl);
		intercepts[scan].frac = P_InterceptVector (&trace, &dl);
	}

//
// go through in order
//
	while (count--)
	{
		dist = MAXINT;
		for (scan = 0 ; scan < intercepts.Size(); scan++)
			if (intercepts[scan].frac < dist)
			{
				dist = intercepts[scan].frac;
				in = &intercepts[scan];
			}

		if ( !PTR_SightTraverse (in) )
			return false;					// don't bother going farther
			
		in->frac = MAXINT;
	}

	return true;			// everything was traversed
}

/*
==================
=
= P_SightPathTraverse
=
= Traces a line from x1,y1 to x2,y2, calling the traverser function for each
= Returns true if the traverser function returns true for all lines
==================
*/

bool P_SightPathTraverse (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2)
{
	fixed_t xt1,yt1,xt2,yt2;
	fixed_t xstep,ystep;
	fixed_t partial;
	fixed_t xintercept, yintercept;
	int mapx, mapy, mapxstep, mapystep;
	int count;

	validcount++;
	intercepts.Clear();

	if ( ((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
		x1 += FRACUNIT;							// don't side exactly on a line
	if ( ((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
		y1 += FRACUNIT;							// don't side exactly on a line
	trace.x = x1;
	trace.y = y1;
	trace.dx = x2 - x1;
	trace.dy = y2 - y1;

	x1 -= bmaporgx;
	y1 -= bmaporgy;
	xt1 = x1>>MAPBLOCKSHIFT;
	yt1 = y1>>MAPBLOCKSHIFT;

	x2 -= bmaporgx;
	y2 -= bmaporgy;
	xt2 = x2>>MAPBLOCKSHIFT;
	yt2 = y2>>MAPBLOCKSHIFT;

// points should never be out of bounds, but check once instead of
// each block
	if (xt1<0 || yt1<0 || xt1>=bmapwidth || yt1>=bmapheight
	||  xt2<0 || yt2<0 || xt2>=bmapwidth || yt2>=bmapheight)
		return false;

	if (xt2 > xt1)
	{
		mapxstep = 1;
		partial = FRACUNIT - ((x1>>MAPBTOFRAC)&(FRACUNIT-1));
		ystep = FixedDiv (y2-y1,abs(x2-x1));
	}
	else if (xt2 < xt1)
	{
		mapxstep = -1;
		partial = (x1>>MAPBTOFRAC)&(FRACUNIT-1);
		ystep = FixedDiv (y2-y1,abs(x2-x1));
	}
	else
	{
		mapxstep = 0;
		partial = FRACUNIT;
		ystep = 256*FRACUNIT;
	}
	yintercept = (y1>>MAPBTOFRAC) + FixedMul (partial, ystep);


	if (yt2 > yt1)
	{
		mapystep = 1;
		partial = FRACUNIT - ((y1>>MAPBTOFRAC)&(FRACUNIT-1));
		xstep = FixedDiv (x2-x1,abs(y2-y1));
	}
	else if (yt2 < yt1)
	{
		mapystep = -1;
		partial = (y1>>MAPBTOFRAC)&(FRACUNIT-1);
		xstep = FixedDiv (x2-x1,abs(y2-y1));
	}
	else
	{
		mapystep = 0;
		partial = FRACUNIT;
		xstep = 256*FRACUNIT;
	}
	xintercept = (x1>>MAPBTOFRAC) + FixedMul (partial, xstep);


//
// step through map blocks
// Count is present to prevent a round off error from skipping the break
	mapx = xt1;
	mapy = yt1;


	for (count = 0 ; count < 64 ; count++)
	{
		if (!P_SightBlockLinesIterator (mapx, mapy))
		{
			sightcounts2[1]++;
			return false;	// early out
		}

		if (mapxstep == 0 && mapystep == 0)
			break;

		if ( (yintercept >> FRACBITS) == mapy)
		{
			yintercept += ystep;
			mapx += mapxstep;
			if (mapx == xt2)
				mapxstep = 0;
		}
		else if ( (xintercept >> FRACBITS) == mapx)
		{
			xintercept += xstep;
			mapy += mapystep;
			if (mapy == yt2)
				mapystep = 0;
		}

	}


//
// couldn't early out, so go through the sorted list
//
	sightcounts2[2]++;

	return P_SightTraverseIntercepts ( );
}

/*
=====================
=
= P_CheckSight
=
= Returns true if a straight line between t1 and t2 is unobstructed
= look from eyes of t1 to t2
=
=====================
*/

bool P_CheckSightZDoom(const AActor *t1, const AActor *t2)
{
	if(!t1 || !t2 || !t1->subsector || !t2->subsector)
		return false;

	const sector_t *s1 = t1->subsector->sector;
	const sector_t *s2 = t2->subsector->sector;
	int pnum = (s1 - sectors) * numsectors + (s2 - sectors);

	//
	// check for trivial rejection
	//
	if (!rejectempty && rejectmatrix[pnum>>3] & (1 << (pnum & 7))) {
		sightcounts2[0]++;
		return false;			// can't possibly be connected
	}
	//
	// check precisely
	//
	// killough 4/19/98: make fake floors and ceilings block monster view

	fixed_t s1_floorheight_t1 = P_FloorHeight(t1->x, t1->y, s1->heightsec);
	fixed_t s1_floorheight_t2 = P_FloorHeight(t2->x, t2->y, s1->heightsec);
	fixed_t s1_ceilingheight_t1 = P_CeilingHeight(t1->x, t1->y, s1->heightsec);
	fixed_t s1_ceilingheight_t2 = P_CeilingHeight(t2->x, t2->y, s1->heightsec);
	fixed_t s2_floorheight_t1 = P_FloorHeight(t1->x, t1->y, s2->heightsec);
	fixed_t s2_floorheight_t2 = P_FloorHeight(t2->x, t2->y, s2->heightsec);
	fixed_t s2_ceilingheight_t1 = P_CeilingHeight(t1->x, t1->y, s2->heightsec);
	fixed_t s2_ceilingheight_t2 = P_CeilingHeight(t2->x, t2->y, s2->heightsec);

	if ((s1->heightsec && !(s1->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
		((t1->z + t1->height <= s1_floorheight_t1 &&
		  t2->z >= s1_floorheight_t2) ||
		 (t1->z >= s1_ceilingheight_t1 &&
		  t2->z + t1->height <= s1_ceilingheight_t2)))
		||
		(s2->heightsec && !(s2->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
		 ((t2->z + t2->height <= s2_floorheight_t2 &&
		   t1->z >= s2_floorheight_t1) ||
		  (t2->z >= s2_ceilingheight_t2 &&
		   t1->z + t2->height <= s2_ceilingheight_t1))))
		return false;

	validcount++;

	sightzstart = t1->z + t1->height - (t1->height >> 2);
	bottomslope = (t2->z) - sightzstart;
	topslope = bottomslope + t2->height;

	return P_SightPathTraverse (t1->x, t1->y, t2->x, t2->y);
}

/*
=====================
=
= [denis] P_CheckSightEdgesZDoom
=
= Returns true if a straight line between t1 and t2 is unobstructed
= look from eyes of t1 to any part of t2
=
=====================
*/

bool P_CheckSightEdgesZDoom(const AActor *t1, const AActor *t2, float radius_boost)
{
	const sector_t *s1 = t1->subsector->sector;
	const sector_t *s2 = t2->subsector->sector;
	int pnum = (s1 - sectors) * numsectors + (s2 - sectors);

	//
	// check for trivial rejection
	//
	if (!rejectempty && rejectmatrix[pnum>>3] & (1 << (pnum & 7))) {
		sightcounts2[0]++;
		return false;                   // can't possibly be connected
	}

	//
	// check precisely
	//
	// killough 4/19/98: make fake floors and ceilings block monster view

	fixed_t s1_floorheight_t1 = P_FloorHeight(t1->x, t1->y, s1->heightsec);
	fixed_t s1_floorheight_t2 = P_FloorHeight(t2->x, t2->y, s1->heightsec);
	fixed_t s1_ceilingheight_t1 = P_CeilingHeight(t1->x, t1->y, s1->heightsec);
	fixed_t s1_ceilingheight_t2 = P_CeilingHeight(t2->x, t2->y, s1->heightsec);
	fixed_t s2_floorheight_t1 = P_FloorHeight(t1->x, t1->y, s2->heightsec);
	fixed_t s2_floorheight_t2 = P_FloorHeight(t2->x, t2->y, s2->heightsec);
	fixed_t s2_ceilingheight_t1 = P_CeilingHeight(t1->x, t1->y, s2->heightsec);
	fixed_t s2_ceilingheight_t2 = P_CeilingHeight(t2->x, t2->y, s2->heightsec);

	if ((s1->heightsec && !(s1->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
		((t1->z + t1->height <= s1_floorheight_t1 &&
		  t2->z >= s1_floorheight_t2) ||
		 (t1->z >= s1_ceilingheight_t1 &&
		  t2->z + t1->height <= s1_ceilingheight_t2)))
		||
		(s2->heightsec && !(s2->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
		 ((t2->z + t2->height <= s2_floorheight_t2 &&
		   t1->z >= s2_floorheight_t1) ||
		  (t2->z >= s2_ceilingheight_t2 &&
		   t1->z + t2->height <= s2_ceilingheight_t1))))
		return false;

	sightzstart = t1->z + t1->height - (t1->height >> 2);
	bottomslope = (t2->z) - sightzstart;
	topslope = bottomslope + t2->height;

	// d = normalized euclidian distance between points
	// r = normalized vector perpendicular to d
	// w = r scaled by the radius of mobj t2
	// thereby, "t2->[x,y] + or - w[x,y]" gives you the edges of t2 from t1's point of view
	// this function used to only check the middle of t2
	v3double_t d, r, w;
	M_SetVec3(&d, t1->x - t2->x, t1->y - t2->y, 0);
	M_NormalizeVec3(&d, &d);
	M_SetVec3(&r, -d.y, d.x, 0.0);
	M_ScaleVec3(&w, &r, FIXED2FLOAT(t2->radius));

	return P_SightPathTraverse (t1->x, t1->y, t2->x, t2->y)
		|| P_SightPathTraverse(t1->x, t1->y, t2->x + FLOAT2FIXED(w.x), t2->y + FLOAT2FIXED(w.y))
		|| P_SightPathTraverse(t1->x, t1->y, t2->x - FLOAT2FIXED(w.x), t2->y - FLOAT2FIXED(w.y));
}

/////////////////////////////////////////////////////////////////////////////
//  Vanilla Sight Checking
/////////////////////////////////////////////////////////////////////////////

//
// P_DivlineSide
// Returns side 0 (front), 1 (back), or 2 (on).
//
int
P_DivlineSide
( fixed_t	x,
  fixed_t	y,
  divline_t*	node )
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	left;
    fixed_t	right;
	
    if (!node->dx)
    {
		if (x==node->x)
			return 2;
		
		if (x <= node->x)
			return node->dy > 0;
		
		return node->dy < 0;
    }
    
    if (!node->dy)
    {
		if (x==node->y)
			return 2;
		
		if (y <= node->y)
			return node->dx < 0;
		
		return node->dx > 0;
    }
	
    dx = (x - node->x);
    dy = (y - node->y);
	
    left =  (node->dy>>FRACBITS) * (dx>>FRACBITS);
    right = (dy>>FRACBITS) * (node->dx>>FRACBITS);
	
    if (right < left)
		return 0;	// front side
    
    if (left == right)
		return 2;
    return 1;		// back side
}


//
// P_InterceptVector2
// Returns the fractional intercept point
// along the first divline.
// This is only called by the addthings and addlines traversers.
//
fixed_t
P_InterceptVector2
( divline_t*	v2,
  divline_t*	v1 )
{
    fixed_t	frac;
    fixed_t	num;
    fixed_t	den;
	
    den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);
	
    if (den == 0)
		return 0;
    //	I_Error ("P_InterceptVector: parallel");
    
    num = FixedMul ( (v1->x - v2->x)>>8 ,v1->dy) + 
		FixedMul ( (v2->y - v1->y)>>8 , v1->dx);
    frac = FixedDiv (num , den);
	
    return frac;
}

//
// P_CrossSubsector
// Returns true
//  if strace crosses the given subsector successfully.
//
bool P_CrossSubsector (int num)
{
    seg_t*		seg;
    line_t*		line;
    int			s1;
    int			s2;
    int			count;
    subsector_t*	sub;
    sector_t*		front;
    sector_t*		back;
    fixed_t		opentop;
    fixed_t		openbottom;
    divline_t		divl;
    vertex_t*		v1;
    vertex_t*		v2;
    fixed_t		frac;
    fixed_t		slope;
	
#ifdef RANGECHECK
    if (num>=numsubsectors)
		I_Error ("P_CrossSubsector: ss %i with numss = %i",
				 num,
				 numsubsectors);
#endif
	
    sub = &subsectors[num];
    
    // check lines
    count = sub->numlines;
    seg = &segs[sub->firstline];
	
    for ( ; count ; seg++, count--)
    {
		line = seg->linedef;
		
		// allready checked other side?
		if (line->validcount == validcount)
			continue;
		
		line->validcount = validcount;
		
		v1 = line->v1;
		v2 = line->v2;
		s1 = P_DivlineSide (v1->x,v1->y, &strace);
		s2 = P_DivlineSide (v2->x, v2->y, &strace);
		
		// line isn't crossed?
		if (s1 == s2)
			continue;
		
		divl.x = v1->x;
		divl.y = v1->y;
		divl.dx = v2->x - v1->x;
		divl.dy = v2->y - v1->y;
		s1 = P_DivlineSide (strace.x, strace.y, &divl);
		s2 = P_DivlineSide (t2x, t2y, &divl);
		
		// line isn't crossed?
		if (s1 == s2)
			continue;	
		
		// stop because it is not two sided anyway
		// might do this after updating validcount?
		if ( !(line->flags & ML_TWOSIDED) )
			return false;
		
		// crosses a two sided line
		front = seg->frontsector;
		back = seg->backsector;

		frac = P_InterceptVector2 (&strace, &divl);
		
		// no wall to block sight with?
		fixed_t crossx = divl.x + FixedMul(frac, divl.dx);
		fixed_t crossy = divl.y + FixedMul(frac, divl.dy);

		fixed_t ff = P_FloorHeight(crossx, crossy, front);
		fixed_t fc = P_CeilingHeight(crossx, crossy, front);
		fixed_t bf = P_FloorHeight(crossx, crossy, back);
		fixed_t bc = P_CeilingHeight(crossx, crossy, back);

		if (ff == bf && fc == bc)
			continue;	
		
		// possible occluder
		// because of ceiling height differences
		if (fc < bc)
			opentop = fc;
		else
			opentop = bc;
		
		// because of ceiling height differences
		if (ff > bf)
			openbottom = ff;
		else
			openbottom = bf;
		
		// quick test for totally closed doors
		if (openbottom >= opentop)	
			return false;		// stop
		
		if (ff != bf)
		{
			slope = FixedDiv (openbottom - sightzstart , frac);
			if (slope > bottomslope)
				bottomslope = slope;
		}
		
		if (fc != bc)
		{
			slope = FixedDiv (opentop - sightzstart , frac);
			if (slope < topslope)
				topslope = slope;
		}
		
		if (topslope <= bottomslope)
			return false;		// stop				
    }
    // passed the subsector ok
    return true;		
}



//
// P_CrossBSPNode
// Returns true
//  if strace crosses the given node successfully.
//
bool P_CrossBSPNode (int bspnum)
{
    node_t*	bsp;
    int		side;
	
    if (bspnum & NF_SUBSECTOR)
    {
		if (bspnum == -1)
			return P_CrossSubsector (0);
		else
			return P_CrossSubsector (bspnum&(~NF_SUBSECTOR));
    }
	
    bsp = &nodes[bspnum];
    
    // decide which side the start point is on
    side = P_DivlineSide (strace.x, strace.y, (divline_t *)bsp);
    if (side == 2)
		side = 0;	// an "on" should cross both sides
	
    // cross the starting side
    if (!P_CrossBSPNode (bsp->children[side]) )
		return false;
	
    // the partition plane is crossed here
    if (side == P_DivlineSide (t2x, t2y,(divline_t *)bsp))
    {
		// the line doesn't touch the other side
		return true;
    }
    
    // cross the ending side		
    return P_CrossBSPNode (bsp->children[side^1]);
}


//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
bool P_CheckSightDoom(const AActor* t1, const AActor* t2)
{
    int		s1;
    int		s2;
    int		pnum;
    int		bytenum;
    int		bitnum;

	if(!t1 || !t2 || !t1->subsector || !t2->subsector)
		return false;
    
    // First check for trivial rejection.
	
    // Determine subsector entries in REJECT table.
    s1 = (t1->subsector->sector - sectors);
    s2 = (t2->subsector->sector - sectors);
    pnum = s1*numsectors + s2;
    bytenum = pnum>>3;
    bitnum = 1 << (pnum&7);
	
    // Check in REJECT table.
    if (!rejectempty && rejectmatrix[bytenum]&bitnum)
    {
		sightcounts[0]++;
		
		// can't possibly be connected
		return false;	
    }
	
    // An unobstructed LOS is possible.
    // Now look from eyes of t1 to any part of t2.
    sightcounts[1]++;
	
    validcount++;
	
    sightzstart = t1->z + t1->height - (t1->height>>2);
    topslope = (t2->z+t2->height) - sightzstart;
    bottomslope = (t2->z) - sightzstart;
	
    strace.x = t1->x;
    strace.y = t1->y;
    t2x = t2->x;
    t2y = t2->y;
    strace.dx = t2->x - t1->x;
    strace.dy = t2->y - t1->y;
	
    // the head node is the last node output
    return P_CrossBSPNode (numnodes-1);	
}

//
// P_CheckSight
// Returns true
//  if a straight line between t1 and t2 is unobstructed.
// Uses REJECT.
//
static bool P_CheckSightDoom
( fixed_t x1, fixed_t y1, fixed_t z1, fixed_t h1,
  fixed_t x2, fixed_t y2, fixed_t z2, fixed_t h2 )
{
    int		s1;
    int		s2;
    int		pnum;
    int		bytenum;
    int		bitnum;
    
    // First check for trivial rejection.
	
    // Determine subsector entries in REJECT table.
    s1 = (R_PointInSubsector(x1, y1)->sector - sectors);
    s2 = (R_PointInSubsector(x2, y2)->sector - sectors);
    pnum = s1*numsectors + s2;
    bytenum = pnum>>3;
    bitnum = 1 << (pnum&7);
	
    // Check in REJECT table.
    if (!rejectempty && rejectmatrix[bytenum]&bitnum)
    {
		sightcounts[0]++;
		
		// can't possibly be connected
		return false;	
    }
	
    // An unobstructed LOS is possible.
    // Now look from eyes of t1 to any part of t2.
    sightcounts[1]++;
	
    validcount++;
	
    sightzstart = z1 + h1 - (h1>>2);
    topslope = (z2+h2) - sightzstart;
    bottomslope = (z2) - sightzstart;
	
    strace.x = x1;
    strace.y = y1;
    t2x = x2;
    t2y = y2;
    strace.dx = x2 - x1;
    strace.dy = y2 - y1;
	
    // the head node is the last node output
    return P_CrossBSPNode (numnodes-1);	
}

bool P_CheckSight(const AActor* t1, const AActor* t2)
{
	if (co_zdoomphys || HasBehavior)
		return P_CheckSightZDoom(t1, t2);
	else
		return P_CheckSightDoom(t1, t2);
}

//
// denis - P_CheckSightEdgesDoom
// Returns true if a straight line between the eyes of t1 and
// any part of t2 is unobstructed.
// Uses REJECT.
//
bool P_CheckSightEdgesDoom
( const AActor*	t1,
  const AActor*	t2,
  float radius_boost )
{
// denis - todo - make this more efficient!!!
// denis - todo - three well placed objects can cause this function to incorrectly return false
// d = normalized euclidian distance between points
// r = normalized vector perpendicular to d
// w = r scaled by the radius of mobj t2
// thereby, "t2->[x,y] + or - w[x,y]" gives you the edges of t2 from t1's point of view
// this function used to only check the middle of t2
	v3double_t d, r, w;
	M_SetVec3(&d, t1->x - t2->x, t1->y - t2->y, 0);
	M_NormalizeVec3(&d, &d);
	M_SetVec3(&r, -d.y, d.x, 0.0);
	M_ScaleVec3(&w, &r, FIXED2FLOAT(t2->radius) + radius_boost);

	bool contact = false;

	contact |= P_CheckSightDoom(t1->x, t1->y, t1->z, t1->height,
							t2->x, t2->y, t2->z, t2->height);

	contact |= P_CheckSightDoom(t1->x, t1->y, t1->z, t1->height,
							t2->x - FLOAT2FIXED(w.x), t2->y - FLOAT2FIXED(w.y), t2->z, t2->height);

	contact |= P_CheckSightDoom(t1->x, t1->y, t1->z, t1->height,
							t2->x + FLOAT2FIXED(w.x), t2->y + FLOAT2FIXED(w.y), t2->z, t2->height);

	return contact;
}	

bool P_CheckSightEdges(const AActor* t1, const AActor* t2, float radius_boost)
{
	if (co_zdoomphys || HasBehavior)
		return P_CheckSightEdgesZDoom(t1, t2, radius_boost);
	else
		return P_CheckSightEdgesDoom(t1, t2, radius_boost);
}

VERSION_CONTROL (p_sight_cpp, "$Id$")

