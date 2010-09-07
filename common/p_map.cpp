// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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
//		Movement, collision handling.
//		Shooting and aiming.
//
//-----------------------------------------------------------------------------


#include "vectors.h"

#include "m_alloc.h"
#include "m_bbox.h"
#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"
#include "p_lnspec.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"

#include "z_zone.h"

fixed_t 		tmbbox[4];
static AActor  *tmthing;
static int 		tmflags;
static fixed_t	tmx;
static fixed_t	tmy;
static fixed_t	tmz;	// [RH] Needed for third dimension of teleporters
static int		pe_x;	// Pain Elemental position for Lost Soul checks	// phares
static int		pe_y;	// Pain Elemental position for Lost Soul checks	// phares
static int		ls_x;	// Lost Soul position for Lost Soul checks		// phares
static int		ls_y;	// Lost Soul position for Lost Soul checks		// phares

// If "floatok" true, move would be ok
// if within "tmfloorz - tmceilingz".
BOOL 			floatok;

fixed_t 		tmfloorz;
fixed_t 		tmceilingz;
fixed_t 		tmdropoffz;

//Added by MC: So bot will know what kind of sector it's entering.
sector_t*		tmsector;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t* 		ceilingline;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
// [RH] MaxSpecialCross	grows as needed
int				MaxSpecialCross = 0;

line_t** 		spechit;
int 			numspechit;

AActor *onmobj; // generic global onmobj...used for landing on pods/players
AActor *BlockingMobj;

// Temporary holder for thing_sectorlist threads
msecnode_t* sector_list = NULL;		// phares 3/16/98

EXTERN_CVAR(co_allowdropoff)
EXTERN_CVAR (co_realactorheight)

//
// TELEPORT MOVE
//

//
// PIT_StompThing
//
static BOOL StompAlwaysFrags;

BOOL PIT_StompThing (AActor *thing)
{
	fixed_t blockdist;

	if (!(thing->flags & MF_SHOOTABLE))
		return true;

	if (thing->player && thing->player->spectator)
		return true;

	if (tmthing->player && tmthing->player->spectator)
		return true;

	// don't clip against self
	if (thing == tmthing)
		return true;

	blockdist = thing->radius + tmthing->radius;

	if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
	{
		// didn't hit it
		return true;
	}

	// [RH] Z-Check
/*	if (tmz > thing->z + thing->height)
		return true;        // overhead
	if (tmz+tmthing->height < thing->z)
		return true;        // underneath
*/

	// monsters don't stomp things except on boss level
	if (StompAlwaysFrags) {
		P_DamageMobj (thing, tmthing, tmthing, 10000, MOD_TELEFRAG);
		return true;
	}
	return false;
}


//
// P_TeleportMove
//
// [RH] Added telefrag parameter: When true, anything in the spawn spot
//		will always be telefragged, and the move will be successful.
//		Added z parameter. Originally, the thing's z was set *after* the
//		move was made, so the height checking I added for 1.13 could
//		potentially erroneously indicate the move was okay if the thing
//		was being teleported between two non-overlapping height ranges.
BOOL P_TeleportMove (AActor *thing, fixed_t x, fixed_t y, fixed_t z, BOOL telefrag)
{
	int 				xl;
	int 				xh;
	int 				yl;
	int 				yh;
	int 				bx;
	int 				by;

	subsector_t*		newsubsec;

	// kill anything occupying the position
	tmthing = thing;

	tmx = x;
	tmy = y;
	tmz = z;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector (x,y);
	ceilingline = NULL;

	// The base floor/ceiling is from the subsector
	// that contains the point.
	// Any contacted lines the step closer together
	// will adjust them.
	tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = newsubsec->sector->ceilingheight;

	validcount++;
	numspechit = 0;

	StompAlwaysFrags = tmthing->player || (level.flags & LEVEL_MONSTERSTELEFRAG) || telefrag;

	if (!(tmthing->player && tmthing->player->spectator))
	{
		// stomp on any things contacted
		xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
		xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
		yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
		yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

		for (bx=xl ; bx<=xh ; bx++)
			for (by=yl ; by<=yh ; by++)
				if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
					return false;
	}

	// the move is ok,
	// so link the thing into its new position
	thing->SetOrigin (x, y, z);
	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;

	return true;
}

//
// killough 8/28/98:
//
// P_GetFriction()
//
// Returns the friction associated with a particular mobj.

int P_GetFriction (const AActor *mo, int *frictionfactor)
{
	int friction = ORIG_FRICTION;
	int movefactor = ORIG_FRICTION_FACTOR;
	const msecnode_t *m;
	const sector_t *sec;

	if (mo->flags2 & MF2_FLY)
	{
		friction = FRICTION_FLY;
	}
	else if (!(mo->flags & MF_NOGRAVITY) && mo->waterlevel > 1 ||
		(mo->waterlevel == 1 && (mo->z > mo->floorz + 6*FRACUNIT)))
	{
		friction = mo->subsector->sector->friction;
		movefactor = mo->subsector->sector->movefactor >> 1;
	}
	else if (!(mo->flags & (MF_NOCLIP|MF_NOGRAVITY)))
	{	// When the object is straddling sectors with the same
		// floorheight that have different frictions, use the lowest
		// friction value (muddy has precedence over icy).

		for (m = mo->touching_sectorlist; m; m = m->m_tnext)
			if ((sec = m->m_sector)->special & FRICTION_MASK &&
				(sec->friction < friction || friction == ORIG_FRICTION) &&
				(mo->z <= sec->floorheight ||
				(sec->heightsec &&
				mo->z <= sec->heightsec->floorheight)))
			  friction = sec->friction, movefactor = sec->movefactor;
	}

	if (frictionfactor)
		*frictionfactor = movefactor;

	return friction;
}

// phares 3/19/98
// P_GetMoveFactor() returns the value by which the x,y
// movements are multiplied to add to player movement.
//
// killough 8/28/98: rewritten

int P_GetMoveFactor (const AActor *mo, int *frictionp)
{
	int movefactor, friction;

	// If the floor is icy or muddy, it's harder to get moving. This is where
	// the different friction factors are applied to 'trying to move'. In
	// p_mobj.c, the friction factors are applied as you coast and slow down.

	if ((friction = P_GetFriction(mo, &movefactor)) < ORIG_FRICTION)
	{
		// phares 3/11/98: you start off slowly, then increase as
		// you get better footing

		int momentum = P_AproxDistance(mo->momx,mo->momy);

		if (momentum > MORE_FRICTION_MOMENTUM<<2)
			movefactor <<= 3;
		else if (momentum > MORE_FRICTION_MOMENTUM<<1)
			movefactor <<= 2;
		else if (momentum > MORE_FRICTION_MOMENTUM)
			movefactor <<= 1;
	}

	if (frictionp)
		*frictionp = friction;

	return movefactor;
}

//
// MOVEMENT ITERATOR FUNCTIONS
//

//																	// phares
// PIT_CrossLine													//   |
// Checks to see if a PE->LS trajectory line crosses a blocking		//   V
// line. Returns false if it does.
//
// tmbbox holds the bounding box of the trajectory. If that box
// does not touch the bounding box of the line in question,
// then the trajectory is not blocked. If the PE is on one side
// of the line and the LS is on the other side, then the
// trajectory is blocked.
//
// Currently this assumes an infinite line, which is not quite
// correct. A more correct solution would be to check for an
// intersection of the trajectory and the line, but that takes
// longer and probably really isn't worth the effort.
//

static // killough 3/26/98: make static
BOOL PIT_CrossLine (line_t* ld)
{
	if (!(ld->flags & ML_TWOSIDED) ||
		(ld->flags & (ML_BLOCKING|ML_BLOCKMONSTERS|ML_BLOCKEVERYTHING)))
		if (!(tmbbox[BOXLEFT]   > ld->bbox[BOXRIGHT]  ||
			  tmbbox[BOXRIGHT]  < ld->bbox[BOXLEFT]   ||
			  tmbbox[BOXTOP]    < ld->bbox[BOXBOTTOM] ||
			  tmbbox[BOXBOTTOM] > ld->bbox[BOXTOP]))
			if (P_PointOnLineSide(pe_x,pe_y,ld) != P_PointOnLineSide(ls_x,ls_y,ld))
				return(false);  // line blocks trajectory				//   ^
	return(true); // line doesn't block trajectory					//   |
}																	// phares

//
// PIT_CheckLine
// Adjusts tmfloorz and tmceilingz as lines are contacted
//

// [RH] Moved this out of PIT_CheckLine()
static void AddSpecialLine (line_t *ld)
{
	if (ld->special)
	{
		// [RH] Grow the spechit array as needed
		if (numspechit >= MaxSpecialCross)
		{
			MaxSpecialCross = MaxSpecialCross ? MaxSpecialCross * 2 : 8;
			spechit = (line_t **)Realloc (spechit, MaxSpecialCross * sizeof(*spechit));
			DPrintf ("MaxSpecialCross increased to %d\n", MaxSpecialCross);
		}
		spechit[numspechit++] = ld;
	}
}

static // killough 3/26/98: make static
BOOL PIT_CheckLine (line_t *ld)
{
	if (tmbbox[BOXRIGHT] <= ld->bbox[BOXLEFT]
		|| tmbbox[BOXLEFT] >= ld->bbox[BOXRIGHT]
		|| tmbbox[BOXTOP] <= ld->bbox[BOXBOTTOM]
		|| tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP] )
		return true;

	if (P_BoxOnLineSide (tmbbox, ld) != -1)
		return true;


    // A line has been hit

    // The moving thing's destination position will cross
    // the given line.
    // If this should not be allowed, return false.
    // If the line is special, keep track of it
    // to process later if the move is proven ok.
    // NOTE: specials are NOT sorted by order,
    // so two special lines that are only 8 pixels apart
    // could be crossed in either order.

	if (!ld->backsector)
		return false;		// one sided line

    if (!(tmthing->flags & MF_MISSILE) )
    {
		if ( ld->flags & ML_BLOCKING )
			return false;	// explicitly blocking everything

		if ( !tmthing->player && ld->flags & ML_BLOCKMONSTERS )
			return false;	// block monsters only
    }

	// set openrange, opentop, openbottom
	P_LineOpening (ld);

	// adjust floor / ceiling heights
	if (opentop < tmceilingz)
	{
		tmceilingz = opentop;
		ceilingline = ld;
	}

	if (openbottom > tmfloorz)
		tmfloorz = openbottom;

	if (lowfloor < tmdropoffz)
		tmdropoffz = lowfloor;

	// if contacted a special line, add it to the list
	if (ld->special)
		AddSpecialLine (ld);

	return true;
}

EXTERN_CVAR(sv_unblockplayers)

//
// PIT_CheckThing
//
static // killough 3/26/98: make static
BOOL PIT_CheckThing (AActor *thing)
{
	fixed_t blockdist;
	BOOL 	solid;
	int 	damage;

	// don't clip against self
	if (thing == tmthing)
		return true;

	if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)) )
		return true;	// can't hit thing


	// GhostlyDeath -- Spectators go through everything!
	if (thing->player && thing->player->spectator)
		return true;
	// and vice versa
	if (tmthing->player && tmthing->player->spectator)
		return true;

    if (tmthing->player && thing->player && sv_unblockplayers)
        return true;

	blockdist = thing->radius + tmthing->radius;
	if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
	{
		// didn't hit thing
		return true;
	}
	      
	if ((tmthing->flags2 & MF2_PASSMOBJ) && co_realactorheight)
	{
		// check if a mobj passed over/under another object
		if (/*!(thing->flags & MF_SPECIAL) &&*/
			((tmthing->z >= thing->z + thing->height ||
			  tmthing->z + tmthing->height < thing->z)))
			return true;
	}
	
    // check for skulls slamming into things
	if (tmthing->flags & MF_SKULLFLY)
	{
		damage = ((P_Random(tmthing)%8)+1) * tmthing->info->damage;
		P_DamageMobj (thing, tmthing, tmthing, damage, MOD_UNKNOWN);
		tmthing->flags &= ~MF_SKULLFLY;
		tmthing->momx = tmthing->momy = tmthing->momz = 0;
		P_SetMobjState (tmthing, tmthing->info->spawnstate);
		return false;			// stop moving
	}

    // missiles can hit other things
	if (tmthing->flags & MF_MISSILE)
	{
		// see if it went over / under
		if (tmthing->z > thing->z + thing->height)
			return true;				// overhead
		if (tmthing->z+tmthing->height < thing->z)
			return true;				// underneath

		if (tmthing->target && (
			tmthing->target->type == thing->type ||
			(tmthing->target->type == MT_KNIGHT && thing->type == MT_BRUISER)||
			(tmthing->target->type == MT_BRUISER && thing->type == MT_KNIGHT) ) )
		{
			// Don't hit same species as originator.
			if (thing == tmthing->target)
				return true;

			// [RH] DeHackEd infighting is here.
			if (!deh.Infight && !thing->player)
			{
				// Hit same species as originator, explode, no damage
				return false;
			}
		}

		if (!(thing->flags & MF_SHOOTABLE))
		{
			// didn't do any damage
			return !(thing->flags & MF_SOLID);
		}

		// damage / explode
		if (tmthing->info->damage)
		{
			damage = ((P_Random(tmthing)%8)+1) * tmthing->info->damage;
			{
				// [RH] figure out the means of death
				int mod;

				switch (tmthing->type) {
					case MT_ROCKET:
						mod = MOD_ROCKET;
						break;
					case MT_PLASMA:
						mod = MOD_PLASMARIFLE;
						break;
					case MT_BFG:
						mod = MOD_BFG_BOOM;
						break;
					default:
						mod = MOD_UNKNOWN;
						break;
				}
				P_DamageMobj (thing, tmthing, tmthing->target, damage, mod);
			}
		}

		// don't traverse any more
		return false;
	}

	// check for special pickup
	if (thing->flags & MF_SPECIAL)
	{
		solid = thing->flags & MF_SOLID;
		if (tmthing->flags&MF_PICKUP)
		{
			// can remove thing
			P_TouchSpecialThing (thing, tmthing);
		}
		return !solid;
	}

		// killough 3/16/98: Allow non-solid moving objects to move through solid
	// ones, by allowing the moving thing (tmthing) to move if it's non-solid,
	// despite another solid thing being in the way.
	// killough 4/11/98: Treat no-clipping things as not blocking

	//return !((thing->flags & MF_SOLID && !(thing->flags & MF_NOCLIP))
	//		 && (tmthing->flags & MF_SOLID));

	return !(thing->flags & MF_SOLID);	// old code -- killough
}

// This routine checks for Lost Souls trying to be spawned		// phares
// across 1-sided lines, impassible lines, or "monsters can't	//   |
// cross" lines. Draw an imaginary line between the PE			//   V
// and the new Lost Soul spawn spot. If that line crosses
// a 'blocking' line, then disallow the spawn. Only search
// lines in the blocks of the blockmap where the bounding box
// of the trajectory line resides. Then check bounding box
// of the trajectory vs. the bounding box of each blocking
// line to see if the trajectory and the blocking line cross.
// Then check the PE and LS to see if they're on different
// sides of the blocking line. If so, return true, otherwise
// false.

BOOL Check_Sides(AActor* actor, int x, int y)
{
	int bx,by,xl,xh,yl,yh;

	pe_x = actor->x;
	pe_y = actor->y;
	ls_x = x;
	ls_y = y;

	// Here is the bounding box of the trajectory

	tmbbox[BOXLEFT]   = pe_x < x ? pe_x : x;
	tmbbox[BOXRIGHT]  = pe_x > x ? pe_x : x;
	tmbbox[BOXTOP]    = pe_y > y ? pe_y : y;
	tmbbox[BOXBOTTOM] = pe_y < y ? pe_y : y;

	// Determine which blocks to look in for blocking lines

	xl = (tmbbox[BOXLEFT]   - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT]  - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP]    - bmaporgy)>>MAPBLOCKSHIFT;

	// xl->xh, yl->yh determine the mapblock set to search

	validcount++; // prevents checking same line twice
	for (bx = xl ; bx <= xh ; bx++)
		for (by = yl ; by <= yh ; by++)
		if (!P_BlockLinesIterator(bx,by,PIT_CrossLine))
			return true;										//   ^
	return(false);												//   |
}																// phares


//---------------------------------------------------------------------------
//
// PIT_CheckOnmobjZ
//
//---------------------------------------------------------------------------

BOOL PIT_CheckOnmobjZ (AActor *thing)
{
	fixed_t blockdist;

	if(!(thing->flags&(MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)))
	{ // Can't hit thing
		return(true);
	}
	blockdist = thing->radius+tmthing->radius;
	if(abs(thing->x-tmx) >= blockdist || abs(thing->y-tmy) >= blockdist)
	{ // Didn't hit thing
		return(true);
	}
	if(thing == tmthing)
	{ // Don't clip against self
		return(true);
	}
	if(tmthing->z > thing->z+thing->height)
	{
		return(true);
	}
	else if(tmthing->z+tmthing->height < thing->z)
	{ // under thing
		return(true);
	}
	if(thing->flags&MF_SOLID)
	{
		onmobj = thing;
	}
	return(!(thing->flags&MF_SOLID));
}


//
// MOVEMENT CLIPPING
//


BOOL P_TestMobjLocation (AActor *mobj)
{
	int flags;

	flags = mobj->flags;
	mobj->flags &= ~MF_PICKUP;
	if (P_CheckPosition(mobj, mobj->x, mobj->y))
	{ // XY is ok, now check Z
		mobj->flags = flags;
		if ((mobj->z < mobj->floorz)
			|| (mobj->z + mobj->height > mobj->ceilingz))
		{ // Bad Z
			return false;
		}
		return true;
	}
	mobj->flags = flags;
	return false;
}

//
// P_CheckPosition
// This is purely informative, nothing is modified
// (except things picked up).
//
// in:
//	a AActor (can be valid or invalid)
//	a position to be checked
//	 (doesn't need to be related to the AActor->x,y)
//
// during:
//	special things are touched if MF_PICKUP
//	early out on solid lines?
//
// out:
//	newsubsec
//	floorz
//	ceilingz
//	tmdropoffz = the lowest point contacted (monsters won't move to a dropoff)
//	speciallines[]
//	numspeciallines
//  AActor *BlockingMobj = pointer to thing that blocked position (NULL if not
//   blocked, or blocked by a line).
bool P_CheckPosition (AActor *thing, fixed_t x, fixed_t y)
{
	int xl, xh;
	int yl, yh;
	int bx, by;
	subsector_t *newsubsec;

	tmthing = thing;
	tmflags = thing->flags;

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector (x,y);
	ceilingline = NULL;

	if(!newsubsec)
		return false;

// The base floor / ceiling is from the subsector that contains the point.
// Any contacted lines the step closer together will adjust them.
	tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = newsubsec->sector->ceilingheight;

	//Added by MC: Fill the tmsector.
	tmsector = newsubsec->sector;

	validcount++;
	numspechit = 0;

	if (tmflags & MF_NOCLIP && !(tmflags & MF_SKULLFLY))
		return true;

	// Check things first, possibly picking things up.
	// The bounding box is extended by MAXRADIUS
	// because DActors are grouped into mapblocks
	// based on their origin point, and can overlap
	// into adjacent blocks by up to MAXRADIUS units.
	xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;
	
	BlockingMobj = NULL;
	for (bx=xl ; bx<=xh ; bx++)
		for (by=yl ; by<=yh ; by++)
			if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
				return false;
	
	// check lines
	if (tmflags & MF_NOCLIP)
		return true;
		    
    BlockingMobj = NULL;
	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for (bx=xl ; bx<=xh ; bx++)
		for (by=yl ; by<=yh ; by++)
			if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
				return false;

	return true;
}

//
// P_CheckOnmobj(AActor *thing)
// Checks if the new Z position is legal
//
AActor *P_CheckOnmobj (AActor *thing)
{
	int	xl,xh,yl,yh,bx,by;
	subsector_t *newsubsec;
	fixed_t x, y;
	byte oldmo[sizeof(AActor)];
	//AActor oldmo (*thing);	// save the old mobj before the fake zmovement

	memcpy (oldmo, &thing->x, (byte *)&thing[1] - (byte *)&thing->x);

	x = thing->x;
	y = thing->y;
	tmthing = thing;
	tmflags = thing->flags;
	P_FakeZMovement (tmthing);

	tmx = x;
	tmy = y;

	tmbbox[BOXTOP] = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT] = x + tmthing->radius;
	tmbbox[BOXLEFT] = x - tmthing->radius;

	newsubsec = R_PointInSubsector (x, y);
	ceilingline = NULL;

//
// the base floor / ceiling is from the subsector that contains the
// point. Any contacted lines the step closer together will adjust them
//
	tmfloorz = tmdropoffz = newsubsec->sector->floorheight;
	tmceilingz = newsubsec->sector->ceilingheight;

	//Added by MC: Fill the tmsector.
	tmsector = newsubsec->sector;

	validcount++;
	numspechit = 0;

	if ( tmflags & MF_NOCLIP )
		return NULL;

//
// check things first, possibly picking things up
// the bounding box is extended by MAXRADIUS because DActors are grouped
// into mapblocks based on their origin point, and can overlap into adjacent
// blocks by up to MAXRADIUS units
//
	xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

	for (bx=xl ; bx<=xh ; bx++)
		for (by=yl ; by<=yh ; by++)
			if (!P_BlockThingsIterator(bx,by,PIT_CheckOnmobjZ))
			{
				//*tmthing = oldmo;
				memcpy (&tmthing->x, oldmo, (byte *)&thing[1] - (byte *)&thing->x);
				return onmobj;
			}
	//*tmthing = oldmo;
	memcpy (&tmthing->x, oldmo, (byte *)&thing[1] - (byte *)&thing->x);
	return NULL;
}

//
// P_FakeZMovement
// Fake the zmovement so that we can check if a move is legal
//
void P_FakeZMovement(AActor *mo)
{
	int dist;
	int delta;
//
// adjust height
//
	mo->z += mo->momz;
	if (mo->flags&MF_FLOAT && mo->target)
	{		// float down towards target if too close
		if (!(mo->flags&MF_SKULLFLY) && !(mo->flags&MF_INFLOAT))
		{
			dist = P_AproxDistance(mo->x-mo->target->x, mo->y-mo->target->y);
			delta = (mo->target->z+(mo->height>>1))-mo->z;
			if (delta < 0 && dist < -(delta*3))
				mo->z -= FLOATSPEED;
			else if (delta > 0 && dist < (delta*3))
				mo->z += FLOATSPEED;
		}
	}
	if (mo->player && mo->flags2&MF2_FLY && !(mo->z <= mo->floorz))
	{
		mo->z += finesine[(FINEANGLES/80*level.time)&FINEMASK]/8;
	}

//
// clip movement
//
	if(mo->z <= mo->floorz)
	{ // Hit the floor
		mo->z = mo->floorz;
		if(mo->momz < 0)
		{
			mo->momz = 0;
		}
		if(mo->flags & MF_SKULLFLY)
		{ // The skull slammed into something
			mo->momz = -mo->momz;
		}
	}

	if (mo->z + mo->height > mo->ceilingz)
	{		// hit the ceiling
		if (mo->momz > 0)
			mo->momz = 0;
		mo->z = mo->ceilingz - mo->height;
		if (mo->flags & MF_SKULLFLY)
		{		// the skull slammed into something
			mo->momz = -mo->momz;
		}
	}
}

//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
BOOL P_TryMove (AActor *thing, fixed_t x, fixed_t y, bool dropoff)
{
	fixed_t 	oldx;
	fixed_t 	oldy;
	int 		side;
	int 		oldside;
	line_t* 	ld;

	floatok = false;
    if (!P_CheckPosition (thing, x, y))
    {
         // solid wall or thing        
         if (!BlockingMobj)
             return false;
         else
         {
             if (BlockingMobj->player || !thing->player)
                 return false;
                 
             else if (BlockingMobj->z+BlockingMobj->height-thing->z 
                 > 24*FRACUNIT 
                 || (BlockingMobj->subsector->sector->ceilingheight
                 -(BlockingMobj->z+BlockingMobj->height) < thing->height)
                 || (tmceilingz-(BlockingMobj->z+BlockingMobj->height) 
                 < thing->height))
             {
                 return false;
            }
        }
        if (!(tmthing->flags2 & MF2_PASSMOBJ))
            return false;             
    }

	if (!(thing->flags & MF_NOCLIP) && !(thing->player && thing->player->spectator))
	{
		if (tmceilingz - tmfloorz < thing->height)
			return false;		// doesn't fit

		floatok = true;

		if (!(thing->flags & MF_TELEPORT)
			&& tmceilingz - thing->z < thing->height && !(thing->flags2 & MF2_FLY))
		{
			return false;		// mobj must lower itself to fit
		}
		
		if (thing->flags2 & MF2_FLY)
		{
			// When flying, slide up or down blocking lines until the actor
			// is not blocked.
			if (thing->z+thing->height > tmceilingz)
			{
				thing->momz = -8*FRACUNIT;
				return false;
			}
			else if (thing->z < tmfloorz && tmfloorz-tmdropoffz > 24*FRACUNIT)
			{
				thing->momz = 8*FRACUNIT;
				return false;
			}
		}
				
		if (!(thing->flags & MF_TELEPORT) && tmfloorz-thing->z > 24*FRACUNIT)
		{
			// too big a step up
			return false;
		}
		
		// killough 3/15/98: Allow certain objects to drop off
		// [Spleen] Unless co_allowdropoff is true, monsters can now get pushed or thrusted off of ledges like in BOOM
		if (!((thing->flags&(MF_DROPOFF|MF_FLOAT)) || (dropoff && co_allowdropoff))
			&& tmfloorz - tmdropoffz > 24*FRACUNIT)
			return false;	// don't stand over a dropoff
	}

	// the move is ok,
	// so link the thing into its new position
	thing->UnlinkFromWorld ();

	oldx = thing->x;
	oldy = thing->y;
	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->x = x;
	thing->y = y;

	thing->LinkToWorld ();

	// if any special lines were hit, do the effect
	if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
	{
		while (numspechit--)
		{
			// see if the line was crossed
			ld = spechit[numspechit];
			side = P_PointOnLineSide (thing->x, thing->y, ld);
			oldside = P_PointOnLineSide (oldx, oldy, ld);
			if (side != oldside)
			{
				if(ld->special)
					P_CrossSpecialLine (ld-lines, oldside, thing);
			}
		}
	}

	return true;
}


//
// P_ThingHeightClip
// Takes a valid thing and adjusts the thing->floorz,
// thing->ceilingz, and possibly thing->z.
// This is called for all nearby monsters
// whenever a sector changes height.
// If the thing doesn't fit,
// the z will be set to the lowest value
// and false will be returned.
//
BOOL P_ThingHeightClip (AActor* thing)
{
    BOOL             onfloor;

    onfloor = (thing->z == thing->floorz);

    P_CheckPosition (thing, thing->x, thing->y);
    // what about stranding a monster partially off an edge?

    thing->floorz = tmfloorz;
    thing->ceilingz = tmceilingz;

    if (onfloor)
    {
        // walking monsters rise and fall with the floor
        thing->z = thing->floorz;
    }
    else
    {
        // don't adjust a floating monster unless forced to
        if (thing->z+thing->height > thing->ceilingz)
            thing->z = thing->ceilingz - thing->height;
    }

    if (thing->ceilingz - thing->floorz < thing->height)
        return false;

    return true;
}


//
// SLIDE MOVE
// Allows the player to slide along any angled walls.
//
fixed_t 		bestslidefrac;
fixed_t 		secondslidefrac;

line_t* 		bestslideline;
line_t* 		secondslideline;

AActor* 		slidemo;

fixed_t 		tmxmove;
fixed_t 		tmymove;

//
// P_HitSlideLine
// Adjusts the xmove / ymove
// so that the next move will slide along the wall.
// If the floor is icy, then you can bounce off a wall.				// phares
//
void P_HitSlideLine (line_t* ld)
{
	int 	side;

	angle_t lineangle;
	angle_t moveangle;
	angle_t deltaangle;

	fixed_t movelen;
	fixed_t newlen;
	BOOL	icyfloor;	// is floor icy?							// phares
																	//   |
	// Under icy conditions, if the angle of approach to the wall	//   V
	// is more than 45 degrees, then you'll bounce and lose half
	// your momentum. If less than 45 degrees, you'll slide along
	// the wall. 45 is arbitrary and is believable.

	// Check for the special cases of horz or vert walls.

	// killough 10/98: only bounce if hit hard (prevents wobbling)
	icyfloor =
		(P_AproxDistance(tmxmove, tmymove) > 4*FRACUNIT) &&
		slidemo->z <= slidemo->floorz &&		  // killough 8/28/98: calc friction on demand
		P_GetFriction (slidemo, NULL) > ORIG_FRICTION;

	if (ld->slopetype == ST_HORIZONTAL)
	{
		if (icyfloor && (abs(tmymove) > abs(tmxmove)))
		{
			tmxmove /= 2; // absorb half the momentum
			tmymove = -tmymove/2;
			S_Sound (slidemo, CHAN_VOICE, "*grunt1", 1, ATTN_IDLE); // oooff!
		}
		else
			tmymove = 0; // no more movement in the Y direction
		return;
	}

	if (ld->slopetype == ST_VERTICAL)
	{
		if (icyfloor && (abs(tmxmove) > abs(tmymove)))
		{
			tmxmove = -tmxmove/2; // absorb half the momentum
			tmymove /= 2;
			S_Sound (slidemo, CHAN_VOICE, "*grunt1", 1, ATTN_IDLE); // oooff!	//   ^
		}																		//   |
		else																	// phares
			tmxmove = 0; // no more movement in the X direction
		return;
	}

	// The wall is angled. Bounce if the angle of approach is		// phares
	// less than 45 degrees.										// phares

	side = P_PointOnLineSide (slidemo->x, slidemo->y, ld);

	lineangle = P_PointToAngle (0,0, ld->dx, ld->dy);

	if (side == 1)
		lineangle += ANG180;

	moveangle = P_PointToAngle (0,0, tmxmove, tmymove);
	deltaangle = moveangle-lineangle;

	if (icyfloor && (deltaangle > ANG45) && (deltaangle < ANG90+ANG45))
	{
		moveangle += 10;	// prevents sudden path reversal due to		// phares
							// rounding error							//   |
		deltaangle = moveangle-lineangle;								//   V
		movelen = P_AproxDistance (tmxmove, tmymove);

		moveangle = lineangle - deltaangle;
		movelen /= 2; // absorb
		S_Sound (slidemo, CHAN_VOICE, "*grunt1", 1, ATTN_IDLE); // oooff!
		moveangle >>= ANGLETOFINESHIFT;
		tmxmove = FixedMul (movelen, finecosine[moveangle]);
		tmymove = FixedMul (movelen, finesine[moveangle]);
	}																//   ^
	else															//   |
	{																// phares
		if (deltaangle > ANG180)
			deltaangle += ANG180;

		lineangle >>= ANGLETOFINESHIFT;
		deltaangle >>= ANGLETOFINESHIFT;

		movelen = P_AproxDistance (tmxmove, tmymove);
		newlen = FixedMul (movelen, finecosine[deltaangle]);

		tmxmove = FixedMul (newlen, finecosine[lineangle]);
		tmymove = FixedMul (newlen, finesine[lineangle]);
	}																// phares
}


//
// PTR_SlideTraverse
//
BOOL PTR_SlideTraverse (intercept_t* in)
{
	line_t* 	li;

	if (!in->isaline)
		I_Error ("PTR_SlideTraverse: not a line?");

	li = in->d.line;

	if ( ! (li->flags & ML_TWOSIDED) )
	{
		if (P_PointOnLineSide (slidemo->x, slidemo->y, li))
		{
			// don't hit the back side
			return true;
		}
		goto isblocking;
	}

	// set openrange, opentop, openbottom
	P_LineOpening (li);

	if (openrange < slidemo->height)
		goto isblocking;				// doesn't fit

	if (opentop - slidemo->z < slidemo->height)
		goto isblocking;				// mobj is too high

	if (openbottom - slidemo->z > 24*FRACUNIT )
		goto isblocking;				// too big a step up

	// this line doesn't block movement
	return true;

	// the line does block movement,
	// see if it is closer than best so far
  isblocking:
	if (in->frac < bestslidefrac)
	{
		secondslidefrac = bestslidefrac;
		secondslideline = bestslideline;
		bestslidefrac = in->frac;
		bestslideline = li;
	}

	return false;		// stop
}



//
// P_SlideMove
// The momx / momy move is bad, so try to slide
// along a wall.
// Find the first line hit, move flush to it,
// and slide along it
//
// This is a kludgy mess.
//
void P_SlideMove (AActor *mo)
{
    fixed_t		leadx;
    fixed_t		leady;
    fixed_t		trailx;
    fixed_t		traily;
    fixed_t		newx;
    fixed_t		newy;
    int			hitcount;

	slidemo = mo;
	hitcount = 3;

  retry:
	if (!--hitcount)
		goto stairstep; 		// don't loop forever

	// trace along the three leading corners
	if (mo->momx > 0)
	{
		leadx = mo->x + mo->radius;
		trailx = mo->x - mo->radius;
	}
	else
	{
		leadx = mo->x - mo->radius;
		trailx = mo->x + mo->radius;
	}

	if (mo->momy > 0)
	{
		leady = mo->y + mo->radius;
		traily = mo->y - mo->radius;
	}
	else
	{
		leady = mo->y - mo->radius;
		traily = mo->y + mo->radius;
	}

	bestslidefrac = FRACUNIT+1;

	P_PathTraverse ( leadx, leady, leadx+mo->momx, leady+mo->momy,
					 PT_ADDLINES, PTR_SlideTraverse );
	P_PathTraverse ( trailx, leady, trailx+mo->momx, leady+mo->momy,
					 PT_ADDLINES, PTR_SlideTraverse );
	P_PathTraverse ( leadx, traily, leadx+mo->momx, traily+mo->momy,
					 PT_ADDLINES, PTR_SlideTraverse );

	// move up to the wall
	if (bestslidefrac == FRACUNIT+1)
	{
		// the move must have hit the middle, so stairstep
	  stairstep:
		// killough 3/15/98: Allow objects to drop off ledges
		if (!P_TryMove (mo, mo->x, mo->y + mo->momy, true))
		{
			P_TryMove (mo, mo->x + mo->momx, mo->y, true);
		}
		return;
	}

	// fudge a bit to make sure it doesn't hit
	bestslidefrac -= 0x800;
	if (bestslidefrac > 0)
	{
		newx = FixedMul (mo->momx, bestslidefrac);
		newy = FixedMul (mo->momy, bestslidefrac);

		// killough 3/15/98: Allow objects to drop off ledges
		if (!P_TryMove (mo, mo->x+newx, mo->y+newy, true))
			goto stairstep;
	}

	// Now continue along the wall.
	bestslidefrac = FRACUNIT-(bestslidefrac+0x800);	// remanider
	if (bestslidefrac > FRACUNIT)
		bestslidefrac = FRACUNIT;
	else if (bestslidefrac <= 0)
		return;

	tmxmove = FixedMul (mo->momx, bestslidefrac);
	tmymove = FixedMul (mo->momy, bestslidefrac);

	P_HitSlideLine (bestslideline); 	// clip the moves

	mo->momx = tmxmove;
	mo->momy = tmymove;

	// killough 10/98: affect the bobbing the same way (but not voodoo dolls)
	/*if (mo->player && mo->player->mo == mo)
	{
		if (abs(mo->player->momx) > abs(tmxmove))
			mo->player->momx = tmxmove;
		if (abs(mo->player->momy) > abs(tmymove))
			mo->player->momy = tmymove;
	}*/

	if (!P_TryMove (mo, mo->x+tmxmove, mo->y+tmymove, true))
	{
		goto retry;
	}
}

//
// P_LineAttack
//
AActor* 		linetarget; 	// who got hit (or NULL)
AActor* 		shootthing;

// Height if not aiming up or down
// ???: use slope for monsters?
fixed_t 		shootz;

int 			la_damage;
fixed_t 		attackrange;

fixed_t 		aimslope;

// slopes to top and bottom of target
// killough 4/20/98: make static instead of using ones in p_sight.c

static fixed_t	topslope;
static fixed_t	bottomslope;


//
// PTR_AimTraverse
// Sets linetaget and aimslope when a target is aimed at.
//
BOOL PTR_AimTraverse (intercept_t* in)
{
	line_t* 			li;
	AActor* 			th;
	fixed_t 			slope;
	fixed_t 			thingtopslope;
	fixed_t 			thingbottomslope;
	fixed_t 			dist;

	if (in->isaline)
	{
		li = in->d.line;

		if ( !(li->flags & ML_TWOSIDED) )
			return false;				// stop

		// Crosses a two sided line.
		// A two sided line will restrict
		// the possible target ranges.
		P_LineOpening (li);

		if (openbottom >= opentop)
			return false;				// stop

		dist = FixedMul (attackrange, in->frac);

		if (li->frontsector->floorheight != li->backsector->floorheight)
		{
			slope = FixedDiv (openbottom - shootz , dist);
			if (slope > bottomslope)
				bottomslope = slope;
		}

		if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
		{
			slope = FixedDiv (opentop - shootz , dist);
			if (slope < topslope)
				topslope = slope;
		}

		if (topslope <= bottomslope)
			return false;				// stop

		return true;					// shot continues
	}

	// shoot a thing
	th = in->d.thing;
	if (th == shootthing)
		return true;					// can't shoot self

	if (!(th->flags&MF_SHOOTABLE))
		return true;					// corpse or something

	// GhostlyDeath -- dont autoaim on spectators
	if ((th->player && th->player->spectator))
		return true;

	// check angles to see if the thing can be aimed at
	dist = FixedMul (attackrange, in->frac);
	thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

	if (thingtopslope < bottomslope)
		return true;					// shot over the thing

	thingbottomslope = FixedDiv (th->z - shootz, dist);

	if (thingbottomslope > topslope)
		return true;					// shot under the thing

	// this thing can be hit!
	if (thingtopslope > topslope)
		thingtopslope = topslope;

	if (thingbottomslope < bottomslope)
		thingbottomslope = bottomslope;

	aimslope = (thingtopslope+thingbottomslope)/2;
	linetarget = th;

	return false;						// don't go any farther
}


//
// PTR_ShootTraverse
//
BOOL PTR_ShootTraverse (intercept_t* in)
{
	fixed_t x, y, z;
	fixed_t frac;
	line_t *li;
	AActor *th;
	fixed_t dist, slope;
	fixed_t thingtopslope, thingbottomslope;

	if (in->isaline)
	{
		li = in->d.line;

		// [RH] Technically, this could improperly cause an impact line to
		//		activate even if the shot continues, but it's necessary
		//		to have it here for Hexen compatibility.
		if (li->special)
			P_ShootSpecialLine (shootthing, li);

		// position a bit closer
		frac = in->frac - FixedDiv (4*FRACUNIT,attackrange);
		z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

		if (!(li->flags & ML_TWOSIDED))
			goto hitline;

		// crosses a two sided line
		P_LineOpening (li);

//		if (z >= opentop || z <= openbottom)
//			goto hitline;
		dist = FixedMul (attackrange, in->frac);

		if (li->frontsector->floorheight != li->backsector->floorheight)
		{
			slope = FixedDiv (openbottom - shootz , dist);
			if (slope > aimslope)
				goto hitline;
		}

		if (li->frontsector->ceilingheight != li->backsector->ceilingheight)
		{
			slope = FixedDiv (opentop - shootz , dist);
			if (slope < aimslope)
				goto hitline;
		}

		// shot continues

		// [RH] set opentop and openbottom for P_LineAttack
	/*	if (P_PointOnLineSide (trace.x, trace.y, li)) {
			opentop = li->frontsector->ceilingheight;
			openbottom = li->frontsector->floorheight;
		} else {
			opentop = li->backsector->ceilingheight;
			openbottom = li->backsector->floorheight;
		}*/
		return true;


		// hit line
	  hitline:
		// position a bit closer
			frac = in->frac - FixedDiv (4*FRACUNIT,attackrange);
		x = trace.x + FixedMul (trace.dx, frac);
		y = trace.y + FixedMul (trace.dy, frac);
		z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

		if (li->frontsector->ceilingpic == skyflatnum)
		{
	    // don't shoot the sky!
			if (z > li->frontsector->ceilingheight)
				return false;

	    // it's a sky hack wall
			if	(li->backsector && li->backsector->ceilingpic == skyflatnum)
				return false;
		}


		// Spawn bullet puffs.
		P_SpawnPuff (x,y,z,0,0);

		// don't go any farther
		return false;
	}

	// shoot a thing
	th = in->d.thing;
	if (th == shootthing)
		return true;			// can't shoot self

	if (!(th->flags & MF_SHOOTABLE))
		return true;			// corpse or something

	// GhostlyDeath -- Don't shoot spectators!
	if ((th->player && th->player->spectator))
		return true;

	// check angles to see if the thing can be aimed at
	dist = FixedMul (attackrange, in->frac);
	thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

	if (thingtopslope < aimslope)
		return true;			// shot over the thing

	thingbottomslope = FixedDiv (th->z - shootz, dist);

	if (thingbottomslope > aimslope)
		return true;			// shot under the thing


	// hit thing
	// position a bit closer
	frac = in->frac - FixedDiv (10*FRACUNIT,attackrange);

	x = trace.x + FixedMul (trace.dx, frac);
	y = trace.y + FixedMul (trace.dy, frac);
	z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

	// Spawn bullet puffs or blod spots,
	// depending on target type.
	if ((in->d.thing->flags & MF_NOBLOOD))
		P_SpawnPuff (x,y,z, P_PointToAngle (0, 0, trace.dx, trace.dy) - ANG180, 2);
	else
		P_SpawnBlood (x,y,z, P_PointToAngle (0, 0, trace.dx, trace.dy) - ANG180, la_damage);

	if (la_damage) {
		// [RH] try and figure out means of death;
		int mod = MOD_UNKNOWN;

		if (shootthing->player) {
			switch (shootthing->player->readyweapon) {
				case wp_fist:
					mod = MOD_FIST;
					break;
				case wp_pistol:
					mod = MOD_PISTOL;
					break;
				case wp_shotgun:
					mod = MOD_SHOTGUN;
					break;
				case wp_chaingun:
					mod = MOD_CHAINGUN;
					break;
				case wp_supershotgun:
					mod = MOD_SSHOTGUN;
					break;
				case wp_chainsaw:
					mod = MOD_CHAINSAW;
					break;
				default:
					break;
			}
		}

		P_DamageMobj (th, shootthing, shootthing, la_damage, mod);
	}

	// don't go any farther
	return false;
}

EXTERN_CVAR(sv_freelook)

//
// P_AimLineAttack
//
fixed_t P_AimLineAttack (AActor *t1, angle_t angle, fixed_t distance)
{
	fixed_t x2;
	fixed_t y2;

	angle >>= ANGLETOFINESHIFT;
	shootthing = t1;

	x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
	y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
	shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;

	// can't shoot outside view angles
	if(sv_freelook)
	{
		// [RH] Technically, this is now correct for an engine with true 6 DOF
		// instead of one which implements y-shearing, like we currently do.
		angle_t topangle = t1->pitch - ANG(32);
		angle_t bottomangle = t1->pitch + ANG(32);

		if (topangle < ANG180)
			topslope = finetangent[FINEANGLES/2-1];
		else
			topslope = finetangent[FINEANGLES/4-((signed)topangle>>ANGLETOFINESHIFT)];

		if (bottomangle >= ANG180)
			bottomslope = finetangent[0];
		else
			bottomslope = finetangent[FINEANGLES/4-((signed)bottomangle>>ANGLETOFINESHIFT)];
	}
	else
	{
		topslope = 100*FRACUNIT/160;
		bottomslope = -100*FRACUNIT/160;
	}

	attackrange = distance;
	linetarget = NULL;

	P_PathTraverse (t1->x, t1->y, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_AimTraverse);

	if (linetarget)
		return aimslope;

	return 0;
}


//
// P_LineAttack
// If damage == 0, it is just a test trace
// that will leave linetarget set.
//
void P_LineAttack (AActor *t1, angle_t angle, fixed_t distance,
				   fixed_t slope, int damage)
{
	fixed_t x2, y2;

	angle >>= ANGLETOFINESHIFT;
	shootthing = t1;
	la_damage = damage;
	x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
	y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
	shootz = t1->z + (t1->height>>1) + 8*FRACUNIT;
	attackrange = distance;
	aimslope = slope;

	// [RH] If any lines are crossed in the shot's path, these will be changed
	opentop = t1->subsector->sector->ceilingheight;
	openbottom = t1->subsector->sector->floorheight;

	if (P_PathTraverse (t1->x, t1->y, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_ShootTraverse)) {
		// [RH] No lines or things were hit in the shot's path, but	it
		//		might still hit the ceiling or floor before it gets out
		//		of range, so we might still need to show a bullet puff.
		// denis - no, this desyncs demos
		/*fixed_t frac;
		fixed_t z = shootz + FixedMul (distance, slope);
		int updown;

		opentop -= mobjinfo[MT_PUFF].height;
		if (z < openbottom) {
			// hit floor
			frac = FixedDiv (openbottom - shootz, z - shootz);
			z = openbottom;
			x2 = t1->x + FixedMul (x2 - t1->x, frac);
			y2 = t1->y + FixedMul (y2 - t1->y, frac);
			updown = 0;
		} else if (z > opentop) {
			// hit ceiling
			subsector_t *subsector;

			frac = FixedDiv (opentop - shootz, z - shootz);
			z = opentop;
			x2 = t1->x + FixedMul (x2 - t1->x, frac);
			y2 = t1->y + FixedMul (y2 - t1->y, frac);
			updown = 1;
			if ( (subsector = R_PointInSubsector (x2, y2)) ) {
				if (subsector->sector->ceilingpic == skyflatnum)
					return;	// disappeared in the clouds
			} else
				return;	// outside map
		} else {
			// hit nothing
			return;
		}
		P_SpawnPuff (x2, y2, z, P_PointToAngle (0, 0, trace.dx, trace.dy) - ANG180, updown);*/
	}
}

//
// [RH] PTR_CameraTraverse
//
fixed_t CameraX, CameraY, CameraZ;
#define CAMERA_DIST	0x1000	// Minimum distance between camera and walls

BOOL PTR_CameraTraverse (intercept_t* in)
{
	fixed_t z;
	fixed_t frac;
	line_t *li;

	// ignore mobjs
	if (!in->isaline)
		return true;

	frac = in->frac - CAMERA_DIST;
	z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

	li = in->d.line;

	if (!(li->flags & ML_TWOSIDED))
		goto hitline;

	// crosses a two sided line
	P_LineOpening (li);

	if (z >= opentop || z <= openbottom)
		goto hitline;

	return true;

	// hit line
  hitline:
	// If the trace went below/above the floor/ceiling, stop
	// in the right place and not on a wall.
	{
		fixed_t ceilingheight, floorheight;

		if (!li->backsector || !P_PointOnLineSide (trace.x, trace.y, li)) {
			ceilingheight = li->frontsector->ceilingheight - CAMERA_DIST;
			floorheight = li->frontsector->floorheight + CAMERA_DIST;
		} else {
			ceilingheight = li->backsector->ceilingheight - CAMERA_DIST;
			floorheight = li->backsector->floorheight + CAMERA_DIST;
		}

		if (z < floorheight) {
			frac = FixedDiv (FixedMul (floorheight - shootz, frac), z - shootz);
			z = floorheight;
		} else if (z > ceilingheight) {
			frac = FixedDiv (FixedMul (ceilingheight - shootz, frac), z - shootz);
			z = ceilingheight;
		}
	}

	CameraX = trace.x + FixedMul (trace.dx, frac);
	CameraY = trace.y + FixedMul (trace.dy, frac);
	CameraZ = z;

	// don't go any farther
	return false;
}

//
// [RH] P_AimCamera
//
EXTERN_CVAR (chase_height)
EXTERN_CVAR (chase_dist)

void P_AimCamera (AActor *t1)
{
	fixed_t distance = (fixed_t)(chase_dist * FRACUNIT);
	angle_t angle = (t1->angle - ANG180) >> ANGLETOFINESHIFT;
	fixed_t x2 = t1->x + (distance>>FRACBITS)*finecosine[angle];
	fixed_t y2 = t1->y + (distance>>FRACBITS)*finesine[angle];
	subsector_t *subsector;

	shootthing = t1;
	shootz = t1->z + t1->height + (fixed_t)(chase_height * FRACUNIT);
	attackrange = distance;
	aimslope = finetangent[FINEANGLES/4+(t1->pitch>>ANGLETOFINESHIFT)];

	CameraZ = shootz + (fixed_t)(chase_dist * aimslope);
	subsector = R_PointInSubsector (x2, y2);
	if (subsector) {
		fixed_t ceilingheight = subsector->sector->ceilingheight - CAMERA_DIST;
		fixed_t floorheight = subsector->sector->floorheight + CAMERA_DIST;
		fixed_t frac = FRACUNIT;

		if (CameraZ < floorheight) {
			frac = FixedDiv (floorheight - shootz, CameraZ - shootz);
			CameraZ = floorheight;
		} else if (CameraZ > ceilingheight) {
			frac = FixedDiv (ceilingheight - shootz, CameraZ - shootz);
			CameraZ = ceilingheight;
		}

		CameraX = t1->x + FixedMul (x2 - t1->x, frac);
		CameraY = t1->y + FixedMul (y2 - t1->y, frac);
	} else {
		CameraX = x2;
		CameraY = y2;
	}

	P_PathTraverse (t1->x, t1->y, x2, y2, PT_ADDLINES, PTR_CameraTraverse);
}


//
// USE LINES
//
AActor *usething;

BOOL PTR_UseTraverse (intercept_t *in)
{
	if (!in->d.line->special)
	{
		P_LineOpening (in->d.line);
		if (openrange <= 0)
		{
			UV_SoundAvoidPlayer (usething, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);

			// can't use through a wall
			return false;
		}

		// not a special line, but keep checking
		return true;
	}

	int side = (P_PointOnLineSide (usething->x, usething->y, in->d.line) == 1);

    P_UseSpecialLine (usething, in->d.line, side);

	//WAS can't use more than one special line in a row
	//jff 3/21/98 NOW multiple use allowed with enabling line flag
	//[RH] And now I've changed it again. If the line is of type
	//	   SPAC_USE, then it eats the use. Everything else passes
	//	   it through, including SPAC_USETHROUGH.
	//[ML] And NOW (8/16/10) it checks whether it's use or NOT the passthrough flags
	// (passthru on a cross or use line).  This may get augmented/changed even more in the future.
	return (GET_SPAC(in->d.line->flags) == SPAC_USE || 
            (GET_SPAC(in->d.line->flags) != SPAC_CROSSTHROUGH && 
             GET_SPAC(in->d.line->flags) != SPAC_USETHROUGH)) ? false : true;
}

// Returns false if a "oof" sound should be made because of a blocking
// linedef. Makes 2s middles which are impassable, as well as 2s uppers
// and lowers which block the player, cause the sound effect when the
// player tries to activate them. Specials are excluded, although it is
// assumed that all special linedefs within reach have been considered
// and rejected already (see P_UseLines).
//
// by Lee Killough
//

BOOL PTR_NoWayTraverse (intercept_t *in)
{
	line_t *ld = in->d.line;					// This linedef

	return ld->special || !(					// Ignore specials
		ld->flags & (ML_BLOCKING) || (			// Always blocking
		P_LineOpening(ld),						// Find openings
		openrange <= 0 ||						// No opening
		openbottom > usething->z+24*FRACUNIT ||	// Too high it blocks
		opentop < usething->z+usething->height	// Too low it blocks
	)
	);
}

//
// P_UseLines
// Looks for special lines in front of the player to activate.
//
void P_UseLines (player_t *player)
{
	int 		angle;
	fixed_t 	x1;
	fixed_t 	y1;
	fixed_t 	x2;
	fixed_t 	y2;

	// GhostlyDeath -- Spectators can't use special lines
	if (player->spectator)
		return;

	usething = player->mo;

	//Added by MC: Check if bot and use special activating (spin round) if it is.
	angle = player->mo->angle >> ANGLETOFINESHIFT;

	x1 = player->mo->x;
	y1 = player->mo->y;
	x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
	y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];

	// old code:
	//
	// P_PathTraverse ( x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse );
	//
	// This added test makes the "oof" sound work on 2s lines -- killough:

	if (P_PathTraverse (x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse))
		if (!P_PathTraverse (x1, y1, x2, y2, PT_ADDLINES, PTR_NoWayTraverse))
			UV_SoundAvoidPlayer (usething, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
}

//
// RADIUS ATTACK
//
AActor* 		bombsource;
AActor* 		bombspot;
int 			bombdamage;
int				bombmod;

//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
// [RH] Now it knows about vertical distances and
//      can thrust things vertically, too.
//

//
// PIT_RadiusAttack
// "bombsource" is the creature
// that caused the explosion at "bombspot".
//
BOOL PIT_RadiusAttack (AActor *thing)
{
    fixed_t	dx;
    fixed_t	dy;
    fixed_t	dist;

    if (!(thing->flags & MF_SHOOTABLE) )
	return true;

    // Boss spider and cyborg
    // take no damage from concussion.
    if (thing->type == MT_CYBORG
	|| thing->type == MT_SPIDER)
	return true;

    dx = abs(thing->x - bombspot->x);
    dy = abs(thing->y - bombspot->y);

    dist = dx>dy ? dx : dy;
    dist = (dist - thing->radius) >> FRACBITS;

    if (dist < 0)
	dist = 0;

    if (dist >= bombdamage)
	return true;	// out of range

    if ( P_CheckSight (thing, bombspot) )
    {
		// must be in direct path
		P_DamageMobj (thing, bombspot, bombsource, bombdamage - dist, bombmod);
    }

    return true;
}

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack (AActor *spot, AActor *source, int damage, int mod)
{
	int 		x;
	int 		y;

	int 		xl;
	int 		xh;
	int 		yl;
	int 		yh;

	fixed_t 	dist;

	dist = (damage+MAXRADIUS)<<FRACBITS;
	yh = (spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT;
	yl = (spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT;
	xh = (spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT;
	xl = (spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT;
	bombspot = spot;
	bombsource = source;
	bombdamage = damage;
	bombmod = mod;

	for (y=yl ; y<=yh ; y++)
		for (x=xl ; x<=xh ; x++)
			P_BlockThingsIterator (x, y, PIT_RadiusAttack);
}



//
// SECTOR HEIGHT CHANGING
// After modifying a sectors floor or ceiling height,
// call this routine to adjust the positions
// of all things that touch the sector.
//
// If anything doesn't fit anymore, true will be returned.
// If crunch is true, they will take damage
//  as they are being crushed.
// If Crunch is false, you should set the sector height back
//  the way it was and call P_ChangeSector again
//  to undo the changes.
//
bool	crushchange;
bool 	nofit;

//
// PIT_ChangeSector
//
BOOL PIT_ChangeSector (AActor *thing)
{
	if (P_ThingHeightClip (thing))
	{
		// keep checking
		return true;
	}

	// GhostlyDeath -- if it's a spectator, keep checking
	if (thing->player && thing->player->spectator)
		return true;

	// crunch bodies to giblets
	if (thing->health <= 0)
	{
		P_SetMobjState (thing, S_GIBS);

		// [Nes] - Classic demo compatability: Ghost monster bug.
		if ((demoplayback || demorecording) && democlassic) {
			thing->height = 0;
			thing->radius = 0;
		}

		// keep checking
		return true;
	}

	// crunch dropped items
	if (thing->flags & MF_DROPPED)
	{
		thing->Destroy ();

		// keep checking
		return true;
	}

	if (! (thing->flags & MF_SHOOTABLE) )
	{
		// assume it is bloody gibs or something
		return true;
	}

	nofit = true;

	if (crushchange && !(level.time&3) )
	{
		P_DamageMobj (thing, NULL, NULL, 10, MOD_CRUSH);

		// spray blood in a random direction
		if (!(thing->flags&MF_NOBLOOD))
		{
			AActor *mo = new AActor (thing->x,
									 thing->y,
									 thing->z + thing->height/2, MT_BLOOD);

			mo->momx = (P_Random (mo) - P_Random (mo)) << 12;
			mo->momy = (P_Random (mo) - P_Random (mo)) << 12;
		}
	}

	// keep checking (crush other things)
	return true;
}

//
// P_ChangeSector	[RH] Was P_CheckSector in BOOM
// jff 3/19/98 added to just check monsters on the periphery
// of a moving sector instead of all in bounding box of the
// sector. Both more accurate and faster.
//

bool P_ChangeSector (sector_t *sector, bool crunch)
{
	int x, y;

	nofit = false;
	crushchange = crunch;

    // re-check heights for all things near the moving sector
    for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
	for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
	    P_BlockThingsIterator (x, y, PIT_ChangeSector);

	return nofit;
}


// phares 3/21/98
//
// Maintain a freelist of msecnode_t's to reduce memory allocs and frees.

msecnode_t *headsecnode = NULL;

// P_GetSecnode() retrieves a node from the freelist. The calling routine
// should make sure it sets all fields properly.

msecnode_t *P_GetSecnode()
{
	msecnode_t *node;

	if (headsecnode)
	{
		node = headsecnode;
		headsecnode = headsecnode->m_snext;
	}
	else
		node = (msecnode_t *)Z_Malloc (sizeof(*node), PU_LEVEL, NULL);
	return node;
}

// P_PutSecnode() returns a node to the freelist.

void P_PutSecnode (msecnode_t *node)
{
	node->m_snext = headsecnode;
	headsecnode = node;
}

// phares 3/16/98
//
// P_AddSecnode() searches the current list to see if this sector is
// already there. If not, it adds a sector node at the head of the list of
// sectors this object appears in. This is called when creating a list of
// nodes that will get linked in later. Returns a pointer to the new node.

msecnode_t *P_AddSecnode (sector_t *s, AActor *thing, msecnode_t *nextnode)
{
	msecnode_t *node;

	node = nextnode;
	while (node)
	{
		if (node->m_sector == s)	// Already have a node for this sector?
		{
			node->m_thing = thing;	// Yes. Setting m_thing says 'keep it'.
			return nextnode;
		}
		node = node->m_tnext;
	}

	// Couldn't find an existing node for this sector. Add one at the head
	// of the list.

	node = P_GetSecnode();

	// killough 4/4/98, 4/7/98: mark new nodes unvisited.
	node->visited = 0;

	node->m_sector = s; 			// sector
	node->m_thing  = thing; 		// mobj
	node->m_tprev  = NULL;			// prev node on Thing thread
	node->m_tnext  = nextnode;		// next node on Thing thread
	if (nextnode)
		nextnode->m_tprev = node;	// set back link on Thing

	// Add new node at head of sector thread starting at s->touching_thinglist

	node->m_sprev  = NULL;			// prev node on sector thread
	node->m_snext  = s->touching_thinglist; // next node on sector thread
	if (s->touching_thinglist)
		node->m_snext->m_sprev = node;
	s->touching_thinglist = node;
	return node;
}


// P_DelSecnode() deletes a sector node from the list of
// sectors this object appears in. Returns a pointer to the next node
// on the linked list, or NULL.

msecnode_t *P_DelSecnode (msecnode_t *node)
{
	msecnode_t* tp;  // prev node on thing thread
	msecnode_t* tn;  // next node on thing thread
	msecnode_t* sp;  // prev node on sector thread
	msecnode_t* sn;  // next node on sector thread

	if (node)
	{
		// Unlink from the Thing thread. The Thing thread begins at
		// sector_list and not from AActor->touching_sectorlist.

		tp = node->m_tprev;
		tn = node->m_tnext;
		if (tp)
			tp->m_tnext = tn;
		if (tn)
			tn->m_tprev = tp;

		// Unlink from the sector thread. This thread begins at
		// sector_t->touching_thinglist.

		sp = node->m_sprev;
		sn = node->m_snext;
		if (sp)
			sp->m_snext = sn;
		else
			node->m_sector->touching_thinglist = sn;
		if (sn)
			sn->m_sprev = sp;

		// Return this node to the freelist

		P_PutSecnode(node);
		return tn;
	}
	return NULL;
} 														// phares 3/13/98

// Delete an entire sector list

void P_DelSeclist (msecnode_t *node)
{
	while (node)
		node = P_DelSecnode (node);
}


// phares 3/14/98
//
// PIT_GetSectors
// Locates all the sectors the object is in by looking at the lines that
// cross through it. You have already decided that the object is allowed
// at this location, so don't bother with checking impassable or
// blocking lines.

BOOL PIT_GetSectors (line_t *ld)
{
	if (tmbbox[BOXRIGHT]	  <= ld->bbox[BOXLEFT]	 ||
			tmbbox[BOXLEFT]   >= ld->bbox[BOXRIGHT]  ||
			tmbbox[BOXTOP]	  <= ld->bbox[BOXBOTTOM] ||
			tmbbox[BOXBOTTOM] >= ld->bbox[BOXTOP])
		return true;

	if (P_BoxOnLineSide (tmbbox, ld) != -1)
		return true;

	// This line crosses through the object.

	// Collect the sector(s) from the line and add to the
	// sector_list you're examining. If the Thing ends up being
	// allowed to move to this position, then the sector_list
	// will be attached to the Thing's AActor at touching_sectorlist.

	sector_list = P_AddSecnode (ld->frontsector,tmthing,sector_list);

	// Don't assume all lines are 2-sided, since some Things
	// like MT_TFOG are allowed regardless of whether their radius takes
	// them beyond an impassable linedef.

	// killough 3/27/98, 4/4/98:
	// Use sidedefs instead of 2s flag to determine two-sidedness.

	if (ld->backsector)
		sector_list = P_AddSecnode(ld->backsector, tmthing, sector_list);

	return true;
}


// phares 3/14/98
//
// P_CreateSecNodeList alters/creates the sector_list that shows what sectors
// the object resides in.

void P_CreateSecNodeList (AActor *thing, fixed_t x, fixed_t y)
{
	int xl;
	int xh;
	int yl;
	int yh;
	int bx;
	int by;
	msecnode_t *node;

	// First, clear out the existing m_thing fields. As each node is
	// added or verified as needed, m_thing will be set properly. When
	// finished, delete all nodes where m_thing is still NULL. These
	// represent the sectors the Thing has vacated.

	node = sector_list;
	while (node)
	{
		node->m_thing = NULL;
		node = node->m_tnext;
	}

	// denis - we may have been called from another P_BlockLinesIterator
	// e.g. when a telefrag results in an item drop.
	// so we need to back up tmthing and then restore it
	AActor *last_tmthing = tmthing;
	int last_tmx = tmx, last_tmy = y;
	int last_tmbbox[4] = {tmbbox[0], tmbbox[1], tmbbox[2], tmbbox[3]};

	tmthing = thing;
	tmx = x;
	tmy = y;

	tmbbox[BOXTOP]	  = y + tmthing->radius;
	tmbbox[BOXBOTTOM] = y - tmthing->radius;
	tmbbox[BOXRIGHT]  = x + tmthing->radius;
	tmbbox[BOXLEFT]   = x - tmthing->radius;

	validcount++; // used to make sure we only process a line once

	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for (bx=xl ; bx<=xh ; bx++)
		for (by=yl ; by<=yh ; by++)
			P_BlockLinesIterator (bx,by,PIT_GetSectors);

	// Add the sector of the (x,y) point to sector_list.

	sector_list = P_AddSecnode (thing->subsector->sector,thing,sector_list);

	// Now delete any nodes that won't be used. These are the ones where
	// m_thing is still NULL.

	node = sector_list;
	while (node)
	{
		if (node->m_thing == NULL)
		{
			if (node == sector_list)
				sector_list = node->m_tnext;
			node = P_DelSecnode(node);
		}
		else
			node = node->m_tnext;
	}

	// denis - restore tmthing
	tmthing = last_tmthing;
	tmx = last_tmx; tmy = last_tmy;
	tmbbox[0] = last_tmbbox[0];
	tmbbox[1] = last_tmbbox[1];
	tmbbox[2] = last_tmbbox[2];
	tmbbox[3] = last_tmbbox[3];
}

VERSION_CONTROL (p_map_cpp, "$Id$")

