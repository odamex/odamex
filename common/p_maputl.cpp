// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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


#include "odamex.h"

#include "m_bbox.h"

#include "p_local.h"
#include "p_mobj.h"
#include "r_data.h"
#include "m_random.h"

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
	m_actor (mo),
	m_capacity (INLINE_BLOCKS),
	m_next (m_inlinenext.data()),
	m_prev (m_inlineprev.data())
{
	clear();
}

AActor::ActorBlockMapListNode::ActorBlockMapListNode(const ActorBlockMapListNode& other) :
	m_actor (other.m_actor),
	m_capacity (INLINE_BLOCKS),
	m_next (m_inlinenext.data()),
	m_prev (m_inlineprev.data())
{
	copyFrom(other);
}

AActor::ActorBlockMapListNode& AActor::ActorBlockMapListNode::operator=(const ActorBlockMapListNode& other)
{
	if (this != &other)
	{
		m_actor = other.m_actor;
		copyFrom(other);
	}
	return *this;
}

AActor::ActorBlockMapListNode::~ActorBlockMapListNode()
{
	if (m_next != m_inlinenext.data())
	{
		delete[] m_next;
		delete[] m_prev;
	}
}

// Grows the link storage to hold at least blockcnt entries.  Only legal
// while the node is unlinked, since other actors' m_prev entries can point
// into this node's m_next array while it is linked.
void AActor::ActorBlockMapListNode::ensureCapacity(int blockcnt)
{
	if (blockcnt <= m_capacity)
		return;

	if (m_next != m_inlinenext.data())
	{
		delete[] m_next;
		delete[] m_prev;
	}

	m_next = new AActor*[blockcnt];
	m_prev = new AActor**[blockcnt];
	m_capacity = blockcnt;
}

void AActor::ActorBlockMapListNode::copyFrom(const ActorBlockMapListNode& other)
{
	m_originx   = other.m_originx;
	m_originy   = other.m_originy;
	m_blockcntx = other.m_blockcntx;
	m_blockcnty = other.m_blockcnty;

	int blockcnt = m_blockcntx * m_blockcnty;
	blockcnt = std::max(blockcnt, 1); // index 0 must always be a valid slot

	ensureCapacity(blockcnt);

	for (int i = 0; i < blockcnt; i++)
	{
		m_next[i] = other.m_next[i];
		m_prev[i] = other.m_prev[i];
	}
}

void AActor::ActorBlockMapListNode::Link()
{
	int left    = (m_actor->x - m_actor->radius - blockmap.originx()) >> MAPBLOCKSHIFT;
	int right   = (m_actor->x + m_actor->radius - blockmap.originx()) >> MAPBLOCKSHIFT;
	int top     = (m_actor->y - m_actor->radius - blockmap.originy()) >> MAPBLOCKSHIFT;
	int bottom  = (m_actor->y + m_actor->radius - blockmap.originy()) >> MAPBLOCKSHIFT;

	if (!co_blockmapfix)
	{
		// originally Doom only used the block containing the center point
		// of the actor even if the actor overlapped into other blocks
		top = bottom = (m_actor->y - blockmap.originy()) >> MAPBLOCKSHIFT;
		left = right = (m_actor->x - blockmap.originx()) >> MAPBLOCKSHIFT;
	}

	// do not ignore actors only *partially* outside blockmap
	// e.g. do not ignore an actor just because its left edge is off the left
	// side of the blockmap - its *right* edge must be off the left side as well
	if (right >= 0 && left < blockmap.width() && bottom >= 0 && top < blockmap.height())
	{
		// however, need to clamp a partially off-limits actor to the grid
		left = std::max(left, 0);
		if (right >= blockmap.width()) right = blockmap.width() - 1;
		top = std::max(top, 0);
		if (bottom >= blockmap.height()) bottom = blockmap.height() - 1;

		m_originx = left;
		m_originy = top;
		m_blockcntx = right - left + 1;
		m_blockcnty = bottom - top + 1;

		ensureCapacity(m_blockcntx * m_blockcnty);

		// [SL] 2012-05-15 - Add the actor to the blocklinks list for all of the
		// blockmaps it overlaps, not just the blockmap for the actor's center point.
		for (int bmy = top; bmy <= bottom; bmy++)
		{
			for (int bmx = left; bmx <= right; bmx++)
			{
				// killough 8/11/98: simpler scheme using pointer-to-pointer prev
				// pointers, allows head nodes to be treated like everything else

				AActor** headptr   = &blocklinks[(bmy * blockmap.width()) + bmx];
				AActor*  headactor = *headptr;

				const size_t thisidx = getIndex(bmx, bmy);

				if ((m_next[thisidx] = headactor))
				{
					const size_t nextidx = headactor->bmapnode.getIndex(bmx, bmy);
					headactor->bmapnode.m_prev[nextidx] = & m_next[thisidx];
				}

				m_prev[thisidx] = headptr;
				*headptr = m_actor;
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
	for (int bmy = m_originy; bmy < m_originy + m_blockcnty; bmy++)
	{
		for (int bmx = m_originx; bmx < m_originx + m_blockcntx; bmx++)
		{
			// killough 8/11/98: simpler scheme using pointers-to-pointers for prev
			// pointers, allows head node pointers to be treated like everything else
			//
			// Also more robust, since it doesn't depend on current position for
			// unlinking. Old method required computing head node based on position
			// at time of unlinking, assuming it was the same position as during
			// linking.

			size_t thisidx = getIndex(bmx, bmy);

			AActor*  nextactor = m_next[thisidx];
			AActor** prevactor = m_prev[thisidx];

			if (prevactor && (*prevactor = nextactor))
			{
				size_t nextidx = nextactor->bmapnode.getIndex(bmx, bmy);
				nextactor->bmapnode.m_prev[nextidx] = prevactor;
			}
		}
	}
}

void AActor::ActorBlockMapListNode::clear()
{
	m_originx = 0;
	m_originy = 0;
	m_blockcntx = 0;
	m_blockcnty = 0;
	std::fill(m_next, m_next + m_capacity, nullptr);
	std::fill(m_prev, m_prev + m_capacity, nullptr);
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

fixed_t P_AproxDistance2 (const fixed_t *pos_array, fixed_t x, fixed_t y)
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

fixed_t P_AproxDistance2 (const AActor *mo, fixed_t x, fixed_t y)
{
	if (mo)
		return P_AproxDistance2(&mo->x, x, y);
	else
		return 0;
}

fixed_t P_AproxDistance2 (const AActor *a, const AActor *b)
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
int P_BoxOnLineSide (const std::span<const fixed_t, 4> tmbox, const line_t *ld)
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

		return static_cast<fixed_t>(num / den);
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

	opentop = std::min<fixed_t>(fc, bc);

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
		else if (refx != limits::MINFIXED)
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
bool P_ActorInFOV(const AActor* origin, const AActor* mo , float f, fixed_t dist)
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

//
// RoughMonsterCheck
// Searches though the surrounding mapblocks for monsters/players
// based on Hexen's P_RoughMonsterSearch
//
// This allows friendlies (and hostiles) to target each other
//
// distance is in MAPBLOCKUNITS

AActor* RoughMonsterCheck(AActor* mo, int index, angle_t fov)
{
	// TODO: get rid of mod here
	const int bx = index % blockmap.width();
	const int by = index / blockmap.width();
	for (AActor* link = blocklinks[index]; link != nullptr; link = link->bmapnode.Next(bx, by))
	{
		// skip non-shootable actors
		if (!(link->flags & MF_SHOOTABLE))
			continue;

		// skip yourself
		if (link == mo)
			continue;

		// skip barrels and other shootable but not alive things
		if (!sentient(link))
			continue;

		// Don't target things friendly to you.
		if (P_IsFriendlyThing(mo, link))
			continue;

		// Don't target players or spectators (done elsewhere)
		if (link->player || (link->player && link->player->spectator))
			continue;

		// skip actors outside of specified FOV
		if (fov > 0 && !P_CheckFov(mo, link, fov))
			continue;

		// skip actors not in line of sight
		if (!P_CheckSight(mo, link))
			continue;

		// all good! return it.
		return link;
	}

	// couldn't find a valid target
	return NULL;
}

//
// RoughTracerCheck
// Searches though the surrounding mapblocks for monsters/players
// based on Hexen's P_RoughMonsterSearch
//
// Special logic to handle tracers (actor->target is owner of tracer)
//
// distance is in MAPBLOCKUNITS

AActor* RoughTracerCheck(AActor* mo, int index, angle_t fov)
{
	// TODO: get rid of mod here
	const int bx = index % blockmap.width();
	const int by = index / blockmap.width();
	for (AActor* link = blocklinks[index]; link != nullptr; link = link->bmapnode.Next(bx, by))
	{
		// skip non-shootable actors
		if (!(link->flags & MF_SHOOTABLE))
			continue;

		// skip the projectile's owner
		if (link == mo->target)
			continue;

		// [Blair] Don't target friendlies
		if (P_IsFriendlyThing(mo->target, link))
			continue;

		// [Blair] Don't target spectators
		if (link->player && link->player->spectator)
			continue;

		// [Blair] Don't target teammates
		if (mo->target->player && link->player &&
			P_AreTeammates(*mo->target->player, *link->player))
			continue;

		// skip actors outside of specified FOV
		if (fov > 0 && !P_CheckFov(mo, link, fov))
			continue;

		// skip actors not in line of sight
		if (!P_CheckSight(mo, link))
			continue;

		// all good! return it.
		return link;
	}

	// couldn't find a valid target
	return NULL;
}

AActor* P_RoughTargetSearch(AActor* mo, angle_t fov, int distance, AActor* (*searchFunc)(AActor*, int, angle_t))
{
	int blockIndex;
	int firstStop;
	int secondStop;
	int thirdStop;
	int finalStop;
	AActor* target;

	const int startX = (mo->x - blockmap.originx()) >> MAPBLOCKSHIFT;
	const int startY = (mo->y - blockmap.originy()) >> MAPBLOCKSHIFT;

	if (startX >= 0 && startX < blockmap.width() && startY >= 0 && startY < blockmap.height())
	{
		target = searchFunc(mo, (startY * blockmap.width()) + startX, fov);
		if (target)
		{ // found a target right away
			return target;
		}
	}
	for (int count = 1; count <= distance; count++)
	{
		int blockX = startX - count;
		int blockY = startY - count;

		if (blockY < 0)
		{
			blockY = 0;
		}
		else if (blockY >= blockmap.height())
		{
			blockY = blockmap.height() - 1;
		}
		if (blockX < 0)
		{
			blockX = 0;
		}
		else if (blockX >= blockmap.width())
		{
			blockX = blockmap.width() - 1;
		}
		blockIndex = (blockY * blockmap.width()) + blockX;
		firstStop = startX + count;
		if (firstStop < 0)
		{
			continue;
		}
		if (firstStop >= blockmap.width())
		{
			firstStop = blockmap.width() - 1;
		}
		secondStop = startY + count;
		if (secondStop < 0)
		{
			continue;
		}
		if (secondStop >= blockmap.height())
		{
			secondStop = blockmap.height() - 1;
		}
		thirdStop = (secondStop * blockmap.width()) + blockX;
		secondStop = (secondStop * blockmap.width()) + firstStop;
		firstStop += blockY * blockmap.width();
		finalStop = blockIndex;

		// Trace the first block section (along the top)
		for (; blockIndex <= firstStop; blockIndex++)
		{
			target = searchFunc(mo, blockIndex, fov);
			if (target)
			{
				return target;
			}
		}
		// Trace the second block section (right edge)
		for (blockIndex--; blockIndex <= secondStop; blockIndex += blockmap.width())
		{
			target = searchFunc(mo, blockIndex, fov);
			if (target)
			{
				return target;
			}
		}
		// Trace the third block section (bottom edge)
		for (blockIndex -= blockmap.width(); blockIndex >= thirdStop; blockIndex--)
		{
			target = searchFunc(mo, blockIndex, fov);
			if (target)
			{
				return target;
			}
		}
		// Trace the final block section (left edge)
		for (blockIndex++; blockIndex > finalStop; blockIndex -= blockmap.width())
		{
			target = searchFunc(mo, blockIndex, fov);
			if (target)
			{
				return target;
			}
		}
	}
	return nullptr;
}

VERSION_CONTROL (p_maputl_cpp, "$Id$")
