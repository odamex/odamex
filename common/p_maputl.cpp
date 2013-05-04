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
//		Movement/collision utility functions,
//		as used by function in p_map.c.
//		BLOCKMAP Iterator functions,
//		and some PIT_* functions to use for iteration.
//
//-----------------------------------------------------------------------------


#include "m_bbox.h"

#include "doomdef.h"
#include "doomstat.h"
#include "p_local.h"
#include "r_data.h"

// State.
#include "r_state.h"

EXTERN_CVAR (co_blockmapfix)
EXTERN_CVAR (co_zdoomphys)

//
//
// P_PointOnSide
//
// Traverse BSP (sub) tree, check point against partition plane.
// Returns side 0 (front) or 1 (back).
//
// killough 5/2/98: reformatted
// [SL] This is a version for the physics code so that R_PointOnSide
// may be changed without affecting compatibility.
//

static int P_PointOnSide(fixed_t x, fixed_t y, node_t *node)
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
// P_PointInSubsector
//
//

subsector_t* P_PointInSubsector(fixed_t x, fixed_t y)
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
		side = P_PointOnSide (x, y, node);
		nodenum = node->children[side];
	}

	return &subsectors[nodenum & ~NF_SUBSECTOR];
}


AActor::ActorBlockMapListNode::ActorBlockMapListNode(AActor *mo) :
	actor(mo)
{
	clear();
}

void AActor::ActorBlockMapListNode::Link()
{
	int left    = (actor->x - actor->radius - bmaporgx) >> MAPBLOCKSHIFT;
	int right   = (actor->x + actor->radius - bmaporgx) >> MAPBLOCKSHIFT;
	int top     = (actor->y - actor->radius - bmaporgy) >> MAPBLOCKSHIFT;
	int bottom  = (actor->y + actor->radius - bmaporgy) >> MAPBLOCKSHIFT;

	if (!co_blockmapfix)
	{
		// originally Doom only used the block containing the center point
		// of the actor even if the actor overlapped into other blocks
		top = bottom = (actor->y - bmaporgy) >> MAPBLOCKSHIFT;
		left = right = (actor->x - bmaporgx) >> MAPBLOCKSHIFT;
	}

	// do not ignore actors only *partially* outside blockmap
	// e.g. do not ignore an actor just because its left edge is off the left
	// side of the blockmap - its *right* edge must be off the left side as well
	if (right >= 0 && left < bmapwidth && bottom >= 0 && top < bmapheight)
	{
		// however, need to clamp a partially off-limits actor to the grid
		if (left < 0) left = 0;
		if (right >= bmapwidth) right = bmapwidth - 1;
		if (top < 0) top = 0;
		if (bottom >= bmapheight) bottom = bmapheight - 1;

		originx = left;
		originy = top;
		blockcntx = right - left + 1;
		blockcnty = bottom - top + 1;

		// [SL] 2012-05-15 - Add the actor to the blocklinks list for all of the
		// blockmaps it overlaps, not just the blockmap for the actor's center point.
		for (int bmy = top; bmy <= bottom; bmy++)
		{
			for (int bmx = left; bmx <= right; bmx++)
			{
				// killough 8/11/98: simpler scheme using pointer-to-pointer prev
				// pointers, allows head nodes to be treated like everything else

				AActor **headptr = &blocklinks[bmy * bmapwidth + bmx];
				AActor *headactor = *headptr;

				size_t thisidx = getIndex(bmx, bmy);
				
		        if ((next[thisidx] = headactor))
		        {
		        	size_t nextidx = headactor->bmapnode.getIndex(bmx, bmy);
					headactor->bmapnode.prev[nextidx] = &next[thisidx];
				}
				
		        prev[thisidx] = headptr;
		        *headptr = actor;
			}
		}
	}
	else
	{
		clear();
	}
}

void AActor::ActorBlockMapListNode::Unlink()
{
	for (int bmy = originy; bmy < originy + blockcnty; bmy++)
	{
		for (int bmx = originx; bmx < originx + blockcntx; bmx++)
		{
			// killough 8/11/98: simpler scheme using pointers-to-pointers for prev
			// pointers, allows head node pointers to be treated like everything else
			//
			// Also more robust, since it doesn't depend on current position for
			// unlinking. Old method required computing head node based on position
			// at time of unlinking, assuming it was the same position as during
			// linking.

			size_t thisidx = getIndex(bmx, bmy);
			
			AActor *nextactor = next[thisidx];
			AActor **prevactor = prev[thisidx];
			
			if (prevactor && (*prevactor = nextactor))
			{
				size_t nextidx = nextactor->bmapnode.getIndex(bmx, bmy);
				nextactor->bmapnode.prev[nextidx] = prevactor;
			}
		}
	}
}

AActor* AActor::ActorBlockMapListNode::Next(int bmx, int bmy)
{
	if (bmx < 0 || bmx >= bmapwidth || bmy < 0 || bmy >= bmapheight)
		return NULL;

	return next[getIndex(bmx, bmy)];
}

void AActor::ActorBlockMapListNode::clear()
{
	originx = originy = 0;
	blockcntx = blockcnty = 0;
	memset(prev, 0, sizeof(prev));
	memset(next, 0, sizeof(next));
}

size_t AActor::ActorBlockMapListNode::getIndex(int bmx, int bmy)
{
	if (!co_blockmapfix)
		return 0;

	// range check
	if (bmx < originx || bmx > originx + blockcntx - 1 ||
		bmy < originy || bmy > originy + blockcnty - 1)
		return 0;
		
	return (bmy - originy) * BLOCKSX + bmx - originx;
}


//
// P_AproxDistance
// Gives an estimation of distance (not exact)
//

fixed_t P_AproxDistance (fixed_t dx, fixed_t dy)
{
	dx = abs(dx);
	dy = abs(dy);
	if (dx < dy)
		return dx+dy-(dx>>1);
	return dx+dy-(dy>>1);
}

fixed_t P_AproxDistance2 (fixed_t *pos_array, fixed_t x, fixed_t y)
{
	if (pos_array)
	{
		fixed_t adx = abs(pos_array[0] - x);
		fixed_t ady = abs(pos_array[1] - y);
		// From _GG1_ p.428. Appox. eucledian distance fast.
		return adx + ady - ((adx < ady ? adx : ady)>>1);
	}
	else
		return 0;
}

fixed_t P_AproxDistance2 (AActor *mo, fixed_t x, fixed_t y)
{
	if (mo)
		return P_AproxDistance2(&mo->x, x, y);
	else
		return 0;
}

fixed_t P_AproxDistance2 (AActor *a, AActor *b)
{
	if (a && b)
		return P_AproxDistance2(&a->x, b->x, b->y);
	else
		return 0;
}

//
// P_PointOnLineSide
// Returns 0 (front) or 1 (back)
//
int P_PointOnLineSide (fixed_t x, fixed_t y, const line_t *line)
{
	if (co_zdoomphys)
	{
		// Make use of vector cross product
		return	int64_t(y - line->v1->y) * int64_t(line->dx) +
				int64_t(line->v1->x - x) * int64_t(line->dy) >= 0;
	}
	else
	{
		if (!line->dx)
		{
			return (x <= line->v1->x) ? (line->dy > 0) : (line->dy < 0);
		}
		else if (!line->dy)
		{
			return (y <= line->v1->y) ? (line->dx < 0) : (line->dx > 0);
		}
		else
		{
			return FixedMul (line->dy >> FRACBITS, x - line->v1->x)
				   <= FixedMul (y - line->v1->y , line->dx >> FRACBITS);
		}
	}
}



//
// P_BoxOnLineSide
// Considers the line to be infinite
// Returns side 0 or 1, -1 if box crosses the line.
//
int P_BoxOnLineSide (const fixed_t *tmbox, const line_t *ld)
{
	int p1 = 0;
	int p2 = 0;

	switch (ld->slopetype)
	{
	  case ST_HORIZONTAL:
		p1 = tmbox[BOXTOP] > ld->v1->y;
		p2 = tmbox[BOXBOTTOM] > ld->v1->y;
		if (ld->dx < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;

	  case ST_VERTICAL:
		p1 = tmbox[BOXRIGHT] < ld->v1->x;
		p2 = tmbox[BOXLEFT] < ld->v1->x;
		if (ld->dy < 0)
		{
			p1 ^= 1;
			p2 ^= 1;
		}
		break;

	  case ST_POSITIVE:
		p1 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXTOP], ld);
		p2 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXBOTTOM], ld);
		break;

	  case ST_NEGATIVE:
		p1 = P_PointOnLineSide (tmbox[BOXRIGHT], tmbox[BOXTOP], ld);
		p2 = P_PointOnLineSide (tmbox[BOXLEFT], tmbox[BOXBOTTOM], ld);
		break;
	}

	return (p1 == p2) ? p1 : -1;
}


//
// P_PointOnDivlineSide
// Returns 0 (front) or 1 (back).
//
int P_PointOnDivlineSide (fixed_t x, fixed_t y, const divline_t *line)
{
	if (co_zdoomphys)
	{
		// Make use of vector cross product
		return	int64_t(y - line->y) * int64_t(line->dx) +
				int64_t(line->x - x) * int64_t(line->dy) >= 0;
	}
	else
	{
		if (!line->dx)
		{
			return (x <= line->x) ? (line->dy > 0) : (line->dy < 0);
		}
		else if (!line->dy)
		{
			return (y <= line->y) ? (line->dx < 0) : (line->dx > 0);
		}
		else
		{
			fixed_t dx = (x - line->x);
			fixed_t dy = (y - line->y);

			// try to quickly decide by looking at sign bits
			if ((line->dy ^ line->dx ^ dx ^ dy) & 0x80000000)
			{	// (left is negative)
				return ((line->dy ^ dx) & 0x80000000) ? 1 : 0;
			}
			else
			{	// if (left >= right), return 1, 0 otherwise
				return FixedMul (dy >> 8, line->dx >> 8) >= FixedMul (line->dy >> 8, dx >> 8);
			}
		}
	}
}



//
// P_MakeDivline
//
void P_MakeDivline (const line_t *li, divline_t *dl)
{
	dl->x = li->v1->x;
	dl->y = li->v1->y;
	dl->dx = li->dx;
	dl->dy = li->dy;
}



//
// P_InterceptVector
// Returns the fractional intercept point along the first divline.
// This is only called by the addthings and addlines traversers.
//
fixed_t P_InterceptVector (const divline_t *v2, const divline_t *v1)
{
	if (co_zdoomphys)
	{
		// [RH] Use 64 bit ints, so long divlines don't overflow
		int64_t den =
				(int64_t(v1->dy) * int64_t(v2->dx) -
				 int64_t(v1->dx) * int64_t(v2->dy)) >> FRACBITS;

		if (den == 0)
			return 0;		// parallel

		int64_t num =
				int64_t(v1->x - v2->x) * int64_t(v1->dy) +
				int64_t(v2->y - v1->y) * int64_t(v1->dx);

		return (fixed_t)(num / den);
	}
	else
	{
		fixed_t den = FixedMul (v1->dy>>8,v2->dx) - FixedMul(v1->dx>>8,v2->dy);

		if (den == 0)
			return 0;

		fixed_t num =
			FixedMul ( (v1->x - v2->x)>>8 ,v1->dy )
			+FixedMul ( (v2->y - v1->y)>>8, v1->dx );

		fixed_t frac = FixedDiv (num , den);

		return frac;
	}
}


//
// P_LineOpening
// Sets opentop and openbottom to the window
// through a two sided line.
// OPTIMIZE: keep this precalculated
//
fixed_t opentop;
fixed_t openbottom;
fixed_t openrange;
fixed_t lowfloor;
sector_t *openbottomsec;

void P_LineOpening (const line_t *linedef, fixed_t x, fixed_t y, fixed_t refx, fixed_t refy)
{
	if (linedef->sidenum[1] == R_NOSIDE)
	{
		// single sided line
		openrange = 0;
		return;
	}

	sector_t *front = linedef->frontsector;
	sector_t *back = linedef->backsector;

	fixed_t fc = P_CeilingHeight(x, y, front);
	fixed_t ff = P_FloorHeight(x, y, front);
	fixed_t bc = P_CeilingHeight(x, y, back);
	fixed_t bf = P_FloorHeight(x, y, back);

	opentop = MIN<fixed_t>(fc, bc);

	bool fflevel = P_IsPlaneLevel(&front->floorplane);
	bool bflevel = P_IsPlaneLevel(&back->floorplane);

	bool usefront = (ff > bf);

	// [RH] fudge a bit for actors that are moving across lines
	// bordering a slope/non-slope that meet on the floor. Note
	// that imprecisions in the plane equation mean there is a
	// good chance that even if a slope and non-slope look like
	// they line up, they won't be perfectly aligned.

	if ((!fflevel || !bflevel) && abs(ff - bf) < 256)
	{
		if (fflevel)
			usefront = true;
		else if (bflevel)
			usefront = false;
		else if (refx != MINFIXED)
			usefront = !P_PointOnLineSide(refx, refy, linedef);
	}

	if (usefront)
	{
		openbottom = ff;
		lowfloor = bf;
		openbottomsec = front;
	}
	else
	{
		openbottom = bf;
		lowfloor = ff;
		openbottomsec = back;
	}

	openrange = opentop - openbottom;
}

//
// THING POSITION SETTING
//

//
// P_UnsetThingPosition
// Unlinks a thing from block map and sectors.
// On each position change, BLOCKMAP and other
// lookups maintaining lists of things inside
// these structures need to be updated.
//
void AActor::UnlinkFromWorld ()
{
	sector_list = NULL;

	if(!subsector)
		return;

	if (!(flags & MF_NOSECTOR))
	{
		// invisible things don't need to be in sector list
		// unlink from subsector

		// killough 8/11/98: simpler scheme using pointers-to-pointers for prev
		// pointers, allows head node pointers to be treated like everything else
		AActor **prev = sprev;
		AActor  *next = snext;
		if ((*prev = next))  // unlink from sector list
			next->sprev = prev;

		// phares 3/14/98
		//
		// Save the sector list pointed to by touching_sectorlist.
		// In P_SetThingPosition, we'll keep any nodes that represent
		// sectors the Thing still touches. We'll add new ones then, and
		// delete any nodes for sectors the Thing has vacated. Then we'll
		// put it back into touching_sectorlist. It's done this way to
		// avoid a lot of deleting/creating for nodes, when most of the
		// time you just get back what you deleted anyway.
		//
		// If this Thing is being removed entirely, then the calling
		// routine will clear out the nodes in sector_list.

		sector_list = touching_sectorlist;
		touching_sectorlist = NULL; //to be restored by P_SetThingPosition
	}

	if ( !(flags & MF_NOBLOCKMAP) )
	{
		bmapnode.Unlink();
	}

	subsector = NULL;
}


//
// P_SetThingPosition
// Links a thing into both a block and a subsector based on it's x y.
// Sets thing->subsector properly
//
void AActor::LinkToWorld ()
{
	// link into subsector
	subsector = P_PointInSubsector (x, y);

	if (!subsector)
		return;

	if ( !(flags & MF_NOSECTOR) )
	{
		// invisible things don't go into the sector links
		// killough 8/11/98: simpler scheme using pointer-to-pointer prev
		// pointers, allows head nodes to be treated like everything else
		AActor **link = &subsector->sector->thinglist;
		AActor *next = *link;
		if ((snext = next))
			next->sprev = &snext;
		sprev = link;
		*link = this;

		// phares 3/16/98
		//
		// If sector_list isn't NULL, it has a collection of sector
		// nodes that were just removed from this Thing.

		// Collect the sectors the object will live in by looking at
		// the existing sector_list and adding new nodes and deleting
		// obsolete ones.

		// When a node is deleted, its sector links (the links starting
		// at sector_t->touching_thinglist) are broken. When a node is
		// added, new sector links are created.

		P_CreateSecNodeList (this, x, y);
		touching_sectorlist = sector_list;	// Attach to thing
		sector_list = NULL;		// clear for next time
    }

	// link into blockmap
	if ( !(flags & MF_NOBLOCKMAP) )
	{
		bmapnode.Link();
	}
}

void AActor::SetOrigin (fixed_t ix, fixed_t iy, fixed_t iz)
{
	UnlinkFromWorld ();
	x = ix;
	y = iy;
	z = iz;
	LinkToWorld ();
}


//
// BLOCK MAP ITERATORS
// For each line/thing in the given mapblock,
// call the passed PIT_* function.
// If the function returns false,
// exit with false without checking anything else.
//


//
// P_BlockLinesIterator
// The validcount flags are used to avoid checking lines
// that are marked in multiple mapblocks,
// so increment validcount before the first call
// to P_BlockLinesIterator, then make one or more calls
// to it.
//
extern polyblock_t **PolyBlockMap;

BOOL P_BlockLinesIterator (int x, int y, BOOL(*func)(line_t*))
{
	if (x<0 || y<0 || x>=bmapwidth || y>=bmapheight)
		return true;

	int offset = *(blockmap + (bmapwidth*y + x));
	int *list = blockmaplump + offset;
	
	/* [RH] Polyobj stuff from Hexen --> */
	polyblock_t *polyLink;

	offset = y*bmapwidth + x;
	if (PolyBlockMap)
	{
		polyLink = PolyBlockMap[offset];
		
		while (polyLink)
		{
			if (polyLink->polyobj && polyLink->polyobj->validcount != validcount)
			{
				int i;
				seg_t **tempSeg = polyLink->polyobj->segs;
				polyLink->polyobj->validcount = validcount;

				for (i = polyLink->polyobj->numsegs; i; i--, tempSeg++)
				{
					if ((*tempSeg)->linedef->validcount != validcount)
					{
						(*tempSeg)->linedef->validcount = validcount;
						if (!func ((*tempSeg)->linedef))
							return false;
					}
				}
			}
			polyLink = polyLink->next;
		}
	}
	/* <-- Polyobj stuff from Hexen */	

	// [RH] Get past starting 0 (from BOOM)
	// denis - not so fast, this breaks doom1.wad 1.9 demo1
	//list++;

	for (; *list != -1; list++)
	{
		line_t *ld = &lines[*list];

		if (ld->validcount != validcount) {
			ld->validcount = validcount;

			if ( !func(ld) )
				return false;
		}
	}

	return true;		// everything was checked
}


//
// P_BlockThingsIterator
//
BOOL P_BlockThingsIterator (int x, int y, BOOL(*func)(AActor*), AActor *actor)
{
	if (x<0 || y<0 || x>=bmapwidth || y>=bmapheight)
		return true;
	else
 	{
		AActor *mobj = (actor != NULL ? actor : blocklinks[y*bmapwidth+x]);
		while (mobj)
 		{
 			if (!func (mobj))
 				return false;

			mobj = mobj->bmapnode.Next(x, y);
		}
	}
	return true;
}



//
// INTERCEPT ROUTINES
//
// denis - make intercepts array resizeable
TArray<intercept_t> intercepts;

divline_t		trace;
BOOL 			earlyout;
int 			ptflags;

//
// PIT_AddLineIntercepts.
// Looks for lines in the given block
// that intercept the given trace
// to add to the intercepts list.
//
// A line is crossed if its endpoints
// are on opposite sides of the trace.
// Returns true if earlyout and a solid line hit.
//
BOOL PIT_AddLineIntercepts (line_t *ld)
{
	int 				s1;
	int 				s2;
	fixed_t 			frac;
	divline_t			dl;

	// avoid precision problems with two routines
	if ( trace.dx > FRACUNIT*16
		 || trace.dy > FRACUNIT*16
		 || trace.dx < -FRACUNIT*16
		 || trace.dy < -FRACUNIT*16)
	{
		s1 = P_PointOnDivlineSide (ld->v1->x, ld->v1->y, &trace);
		s2 = P_PointOnDivlineSide (ld->v2->x, ld->v2->y, &trace);
	}
	else
	{
		s1 = P_PointOnLineSide (trace.x, trace.y, ld);
		s2 = P_PointOnLineSide (trace.x+trace.dx, trace.y+trace.dy, ld);
	}

	if (s1 == s2)
		return true;	// line isn't crossed

	// hit the line
	P_MakeDivline (ld, &dl);
	frac = P_InterceptVector (&trace, &dl);

	if (frac < 0)
		return true;	// behind source

	// try to early out the check
	if (earlyout
		&& frac < FRACUNIT
		&& !ld->backsector)
	{
		return false;	// stop checking
	}


	intercept_t intercept;
	intercept.frac = frac;
	intercept.isaline = true;
	intercept.d.line = ld;
	intercepts.Push(intercept);

	return true;		// continue
}



//
// PIT_AddThingIntercepts
//
BOOL PIT_AddThingIntercepts (AActor* thing)
{
	fixed_t 		x1;
	fixed_t 		y1;
	fixed_t 		x2;
	fixed_t 		y2;

	int 			s1;
	int 			s2;

	BOOL 			tracepositive;

	divline_t		dl;

	fixed_t 		frac;

	tracepositive = (trace.dx ^ trace.dy)>0;

	// check a corner to corner crossection for hit
	if (tracepositive)
	{
		x1 = thing->x - thing->radius;
		y1 = thing->y + thing->radius;

		x2 = thing->x + thing->radius;
		y2 = thing->y - thing->radius;
	}
	else
	{
		x1 = thing->x - thing->radius;
		y1 = thing->y - thing->radius;

		x2 = thing->x + thing->radius;
		y2 = thing->y + thing->radius;
	}

	s1 = P_PointOnDivlineSide (x1, y1, &trace);
	s2 = P_PointOnDivlineSide (x2, y2, &trace);

	if (s1 == s2)
		return true;			// line isn't crossed

	dl.x = x1;
	dl.y = y1;
	dl.dx = x2-x1;
	dl.dy = y2-y1;

	frac = P_InterceptVector (&trace, &dl);

	if (frac < 0)
		return true;			// behind source

	intercept_t intercept;
	intercept.frac = frac;
	intercept.isaline = false;
	intercept.d.thing = thing;
	intercepts.Push(intercept);

	return true;				// keep going
}


//
// P_TraverseIntercepts
// Returns true if the traverser function returns true
// for all lines.
//
BOOL P_TraverseIntercepts (traverser_t func, fixed_t maxfrac)
{
	size_t 				count = intercepts.Size();
	fixed_t 			dist;
	size_t		scan;
	intercept_t*		in = 0;

	while (count--)
	{
		dist = MAXINT;
		for (scan = 0 ; scan < intercepts.Size(); scan++)
		{
			if (intercepts[scan].frac < dist)
			{
				dist = intercepts[scan].frac;
				in = &intercepts[scan];
			}
		}

		if (dist > maxfrac)
			return true;		// checked everything in range


		if ( !func (in) )
			return false;		// don't bother going farther

		in->frac = MAXINT;
	}

	return true;				// everything was traversed
}




//
// P_PathTraverse
// Traces a line from x1,y1 to x2,y2,
// calling the traverser function for each.
// Returns true if the traverser function returns true
// for all lines.
//
BOOL P_PathTraverse (fixed_t x1, fixed_t y1, fixed_t x2, fixed_t y2, int flags, BOOL (*trav) (intercept_t *))
{
	fixed_t 	xt1;
	fixed_t 	yt1;
	fixed_t 	xt2;
	fixed_t 	yt2;

	fixed_t 	xstep;
	fixed_t 	ystep;

	fixed_t 	partial;

	fixed_t 	xintercept;
	fixed_t 	yintercept;

	int 		mapx;
	int 		mapy;

	int 		mapxstep;
	int 		mapystep;

	int 		count;

	earlyout = flags & PT_EARLYOUT;

	validcount++;

	intercepts.Clear();

	if ( ((x1-bmaporgx)&(MAPBLOCKSIZE-1)) == 0)
		x1 += FRACUNIT; // don't side exactly on a line

	if ( ((y1-bmaporgy)&(MAPBLOCKSIZE-1)) == 0)
		y1 += FRACUNIT; // don't side exactly on a line

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

	// Step through map blocks.
	// Count is present to prevent a round off error
	// from skipping the break.
	mapx = xt1;
	mapy = yt1;

	for (count = 0 ; count < 64 ; count++)
	{
		if (flags & PT_ADDLINES)
		{
			if (!P_BlockLinesIterator (mapx, mapy,PIT_AddLineIntercepts))
				return false;	// early out
		}

		if (flags & PT_ADDTHINGS)
		{
			if (!P_BlockThingsIterator (mapx, mapy,PIT_AddThingIntercepts))
				return false;	// early out
		}

		if (mapx == xt2 && mapy == yt2)
		{
			break;
		}

		if ( (yintercept >> FRACBITS) == mapy)
		{
			yintercept += ystep;
			mapx += mapxstep;
		}
		else if ( (xintercept >> FRACBITS) == mapx)
		{
			xintercept += xstep;
			mapy += mapystep;
		}

	}
	// go through the sorted list
	return P_TraverseIntercepts ( trav, FRACUNIT );
}

//
// P_PointToAngle
//
// To get a global angle from cartesian coordinates,
//  the coordinates are flipped until they are in
//  the first octant of the coordinate system, then
//  the y (<=x) is scaled and divided by x to get a
//  tangent (slope) value which is looked up in the
//  tantoangle[] table. The +1 size of tantoangle[]
//  is to handle the case when x==y without additional
//  checking.
//
// killough 5/2/98: reformatted, cleaned up
// haleyjd 01/28/10: restored to Vanilla and made some modifications;
//                   added P_ version for use by gamecode.
//
angle_t P_PointToAngle(fixed_t xo, fixed_t yo, fixed_t x, fixed_t y)
{
	x -= xo;
	y -= yo;

	if((x | y) == 0)
		return 0;

	if(x >= 0)
	{
		if (y >= 0)
		{
			if(x > y)
			{
				// octant 0
				return p_tantoangle[SlopeDiv(y, x)];
			}
			else
			{
				// octant 1
				return ANG90 - 1 - p_tantoangle[SlopeDiv(x, y)];
			}
		}
		else
		{
			y = -y;

			if(x > y)
			{
				// octant 8
				return 0 - p_tantoangle[SlopeDiv(y, x)];
			}
			else
			{
				// octant 7
				return ANG270 + p_tantoangle[SlopeDiv(x, y)];
			}
		}
	}
	else
	{
		x = -x;

		if(y >= 0)
		{
			if(x > y)
			{
				// octant 3
				return ANG180 - 1 - p_tantoangle[SlopeDiv(y, x)];
			}
			else
			{
				// octant 2
				return ANG90 + p_tantoangle[SlopeDiv(x, y)];
			}
		}
		else
		{
			y = -y;

			if(x > y)
			{
				// octant 4
				return ANG180 + p_tantoangle[SlopeDiv(y, x)];
			}
			else
			{
				// octant 5
				return ANG270 - 1 - p_tantoangle[SlopeDiv(x, y)];
			}
		}
	}

	return 0;
}

//
// P_ActorInFOV
//
// Returns true if the actor mo is in the field-of-view of the actor origin,
// with FOV specified by f (0.0 - 180.0) and within a maximum distance specified
// by dist.
//
bool P_ActorInFOV(AActor* origin, AActor* mo , float f, fixed_t dist)
{
	if (f <= 0.0f)
		return false;
	if (f > 180.0f)
		f = 180.0f;

	if (!mo)
		return false;

	// check that the actors are within a radius of dist of each other
	// (A very cheap calculation)
	if (P_AproxDistance2(origin, mo) > dist)
		return false;

	// check that the actor mo is in front of origin's field of view
	// (Not so expensive...)

	// transform and rotate so that tx and ty represent mo's location with respect
	// to the direction origin is looking
	fixed_t tx, ty;
	R_RotatePoint(mo->x - origin->x, mo->y - origin->y, ANG90 - origin->angle, tx, ty);

	// mo is behind origin?
	if (ty < 4*FRACUNIT)
		return false;

	// calculate the angle from the direction origin is facing to mo
	float ang;

	tx = abs(tx);		// just to make calculations simplier
	if (tx > ty)
		ang = 360.0f * (ANG90 - 1 - tantoangle_acc[SlopeDiv(ty, tx)]) / ANG360;
	else
		ang = 360.0f * tantoangle_acc[SlopeDiv(tx, ty)] / ANG360;

	// is the actor mo within the FOV specified by f?
	if (ang > f / 2.0f)
		return false;

	// check to see if the actor mo is hidden behind walls, etc
	// (A very expensive calculation)
	if (!P_CheckSightEdges(origin, mo, 0.0))
			return false;

	return true;
}

VERSION_CONTROL (p_maputl_cpp, "$Id$")

