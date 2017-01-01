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
//		Movement, collision handling.
//		Shooting and aiming.
//
//-----------------------------------------------------------------------------

#include "m_alloc.h"
#include "m_bbox.h"
#include "m_random.h"
#include "i_system.h"

#include "doomdef.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "c_effect.h"
#include "p_mobj.h"

#include "s_sound.h"

// State.
#include "doomstat.h"
#include "r_state.h"

#include "z_zone.h"
#include "p_unlag.h"
#include "m_vectors.h"
#include <math.h>
#include <set>

EXTERN_CVAR(sv_unblockplayers)

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
extern bool 	HasBehavior;	// ZDoom in Hexen Format

fixed_t 		tmfloorz;
fixed_t 		tmceilingz;
fixed_t 		tmdropoffz;
sector_t*		tmfloorsector;

extern sector_t *openbottomsec;

// keep track of the line that lowers the ceiling,
// so missiles don't explode against sky hack walls
line_t* 		ceilingline;
line_t			*BlockingLine;

// keep track of special lines as they are hit,
// but don't process them until the move is proven valid
std::vector<line_t*> spechit;

AActor *onmobj; // generic global onmobj...used for landing on pods/players
AActor *BlockingMobj;

// Temporary holder for thing_sectorlist threads
msecnode_t* sector_list = NULL;		// phares 3/16/98

// [SL] 2012-03-07 - Sectors that can change floor/ceiling height
std::set<short>	movable_sectors;

EXTERN_CVAR (co_allowdropoff)
EXTERN_CVAR (co_realactorheight)
EXTERN_CVAR (co_fixweaponimpacts)
EXTERN_CVAR (co_boomphys)				// [ML] Roll-up of various compat options
EXTERN_CVAR (co_zdoomphys)
EXTERN_CVAR (co_blockmapfix)
EXTERN_CVAR (co_boomsectortouch)
EXTERN_CVAR (sv_friendlyfire)
EXTERN_CVAR (sv_unblockplayers)

CVAR_FUNC_IMPL (sv_gravity)
{
	level.gravity = var;
}

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

	// Spectators shouldn't stomp anybody, and cannot be stomped.
	if (thing->player && thing->player->spectator)
		return true;

	if (tmthing->player && tmthing->player->spectator)
		return true;

	// Unblocked players shouldn't telefrag friendlies.  Thanks Amateur Spammer!
	if (tmthing->player && thing->player && sv_unblockplayers)
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

	if (co_realactorheight)
	{
		// [RH] Z-Check
		if (tmz > thing->z + thing->height)
			return true;        // overhead
		if (tmz + tmthing->height < thing->z)
			return true;        // underneath
	}

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

	newsubsec = P_PointInSubsector (x,y);
	ceilingline = NULL;

	// The base floor/ceiling is from the subsector
	// that contains the point.
	// Any contacted lines the step closer together
	// will adjust them.
	tmfloorz = tmdropoffz = P_FloorHeight(x, y, newsubsec->sector);
	tmceilingz = P_CeilingHeight(x, y, newsubsec->sector);
	tmfloorsector = newsubsec->sector;

	validcount++;
	spechit.clear();

	StompAlwaysFrags = tmthing->player || (level.flags & LEVEL_MONSTERSTELEFRAG) || telefrag;

	// stomp on any things contacted
	xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

	for (bx=xl ; bx<=xh ; bx++)
		for (by=yl ; by<=yh ; by++)
			if (!P_BlockThingsIterator(bx,by,PIT_StompThing))
				return false;

	// the move is ok,
	// so link the thing into its new position
	thing->SetOrigin (x, y, z);
	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->dropoffz = tmfloorz;
	thing->floorsector = tmfloorsector;

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
	else if ((!(mo->flags & MF_NOGRAVITY) && mo->waterlevel > 1) ||
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
				(mo->z <= P_FloorHeight(mo) ||
				(sec->heightsec && !(sec->heightsec->MoreFlags & SECF_IGNOREHEIGHTSEC) &&
				mo->z <= P_FloorHeight(mo))))
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

//
// CheckForPushSpecial
//
static void CheckForPushSpecial (line_t *line, int side, AActor *mobj)
{
	if (line->special)
	{
		if (mobj->flags2 & MF2_PUSHWALL)
		{
			P_PushSpecialLine(mobj, line, side);
		}
	}
}


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
	{ // One sided line
		BlockingLine = ld;
		CheckForPushSpecial (ld, 0, tmthing);
		return false;
	}

    if (!(tmthing->flags & MF_MISSILE) || (ld->flags & ML_BLOCKEVERYTHING))
    {
		if ((ld->flags & (ML_BLOCKING|ML_BLOCKEVERYTHING)) || 	// explicitly blocking everything
			(!tmthing->player && ld->flags & ML_BLOCKMONSTERS)) {	// block monsters only
				CheckForPushSpecial (ld, 0, tmthing);
				return false;
		}
    }

	// [RH] Steep sectors count as dropoffs (unless already in one)
	if (!(tmthing->flags & MF_DROPOFF) &&
		!(tmthing->flags & (MF_NOGRAVITY|MF_NOCLIP)))
	{
		if (ld->frontsector->floorplane.c < STEEPSLOPE ||
			ld->backsector->floorplane.c < STEEPSLOPE)
		{
			const msecnode_t *node = tmthing->touching_sectorlist;
			bool allow = false;
			int count = 0;
			while (node != NULL)
			{
				count++;
				if (node->m_sector->floorplane.c < STEEPSLOPE)
				{
					allow = true;
					break;
				}
				node = node->m_tnext;
			}

			if (!allow)
				return false;
		}
	}

	// set openrange, opentop, openbottom

	// Are the sectors on both sides of the line non-sloped?
	if (P_IsPlaneLevel(&ld->frontsector->floorplane) &&
		P_IsPlaneLevel(&ld->backsector->floorplane) &&
		P_IsPlaneLevel(&ld->frontsector->ceilingplane) &&
		P_IsPlaneLevel(&ld->backsector->ceilingplane))
	{
		P_LineOpening(ld, tmx, tmy, tmx, tmy);
	}
	else
	{
		// Find the point on the line closest to the actor's center, and use
		// that to calculate openings
		double dx = FIXED2DOUBLE(ld->dx);
		double dy = FIXED2DOUBLE(ld->dy);
		double r =	(FIXED2DOUBLE(tmx - ld->v1->x) * dx +
					 FIXED2DOUBLE(tmy - ld->v1->y) * dy) /
					(dx * dx + dy * dy);

		if (r <= 0.0)
		{
			P_LineOpening (ld, ld->v1->x, ld->v1->y, tmx, tmy);
		}
		else if (r >= 1.0)
		{
			P_LineOpening (ld, ld->v2->x, ld->v2->y, tmthing->x, tmthing->y);
		}
		else
		{
			fixed_t sx = ld->v1->x + r * ld->dx;
			fixed_t sy = ld->v1->y + r * ld->dy;
			P_LineOpening (ld, sx, sy, tmx, tmy);
		}
	}

	// adjust floor / ceiling heights
	if (opentop < tmceilingz)
	{
		tmceilingz = opentop;
		ceilingline = ld;
		BlockingLine = ld;
	}

	if (openbottom > tmfloorz)
	{
		tmfloorz = openbottom;
		tmfloorsector = openbottomsec;
		BlockingLine = ld;
	}

	if (lowfloor < tmdropoffz)
		tmdropoffz = lowfloor;

	// if contacted a special line, add it to the list
	if (ld->special)
		spechit.push_back(ld);

	return true;
}

//
// PIT_CheckThing
//
static BOOL PIT_CheckThing (AActor *thing)
{
	bool solid = thing->flags & MF_SOLID;

	// don't clip against self
	if (thing == tmthing)
		return true;

	if (!(thing->flags & (MF_SOLID|MF_SPECIAL|MF_SHOOTABLE)) )
		return true;	// can't hit thing

	// GhostlyDeath -- Spectators go through everything!
	if ((thing->player && thing->player->spectator) ||
		(tmthing->player && tmthing->player->spectator))
		return true;

    if (tmthing->player && thing->player && sv_unblockplayers)
        return true;

	fixed_t blockdist = thing->radius + tmthing->radius;
	if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
	{
		// didn't hit thing
		return true;
	}

	if (co_realactorheight)
		BlockingMobj = thing;

	if (co_realactorheight && (tmthing->flags2 & MF2_PASSMOBJ))
	{
		// check if a mobj passed over/under another object
		if (tmthing->z >= thing->z + thing->height || tmthing->z + tmthing->height <= thing->z)
			return true;
	}

    // check for skulls slamming into things
	if (tmthing->flags & MF_SKULLFLY)
	{
		int damage = ((P_Random(tmthing)%8)+1) * tmthing->info->damage;
		P_DamageMobj (thing, tmthing, tmthing, damage, MOD_UNKNOWN);
		tmthing->flags &= ~MF_SKULLFLY;
		tmthing->momx = tmthing->momy = tmthing->momz = 0;
		P_SetMobjState (tmthing, tmthing->info->spawnstate);
		if (co_realactorheight)
			BlockingMobj = NULL;
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
				return false;		// Hit same species as originator, explode, no damage
		}

		if (!(thing->flags & MF_SHOOTABLE))
			return !solid;		// didn't do any damage

		// damage / explode
		if (tmthing->info->damage)
		{
			int damage = ((P_Random(tmthing)%8)+1) * tmthing->info->damage;
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

		return false;		// don't traverse any more
	}

	// check for special pickup
	if (thing->flags & MF_SPECIAL && tmthing->flags & MF_PICKUP)
	{
		// [SL] Work-around the additional height added to players
		// in P_CheckPosition. Don't let players grab items above
		// their real height!
		if (!co_realactorheight || !tmthing->player ||
			thing->z < tmthing->z + tmthing->height - 24*FRACUNIT)
			P_TouchSpecialThing (thing, tmthing);	// can remove thing
	}

	return !solid;
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
	if (!(thing->flags & MF_SOLID))
		return true;

	// [RH] Corpses and specials don't block moves
	if (thing->flags & (MF_CORPSE|MF_SPECIAL))
		return true;

	// Don't clip against self
	if (thing == tmthing)
		return true;

	// over / under thing
	if (tmthing->z > thing->z + thing->height)
		return true;
	else if (tmthing->z + tmthing->height <= thing->z)
		return true;

	fixed_t blockdist = thing->radius+tmthing->radius;
	if (abs(thing->x - tmx) >= blockdist || abs(thing->y - tmy) >= blockdist)
		return true;		// Didn't hit thing

	onmobj = thing;
	return false;
}

//
// MOVEMENT CLIPPING
//

//
// P_TestMobjLocation
//
// Returns true if the mobj is not blocked by anything at its current
// location, otherwise returns false.
//
BOOL P_TestMobjLocation (AActor *mobj)
{
	int flags = mobj->flags;
	mobj->flags &= ~MF_PICKUP;

	if (P_CheckPosition(mobj, mobj->x, mobj->y))
	{
		// XY is ok, now check Z
		mobj->flags = flags;
		if ((mobj->z < mobj->floorz) || (mobj->z + mobj->height > mobj->ceilingz))
			return false;		// Bad Z

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
	AActor *thingblocker = NULL;
	fixed_t realheight = thing->height;
	bool spectator = thing->player && thing->player->spectator;
	subsector_t *subsec = P_PointInSubsector(x,y);
	// NOTE(jsd): NULL check here fixes crash while awaiting download in an active game.
	if (subsec == NULL)
		return false;
	sector_t *newsec = subsec->sector;

	tmbbox[BOXTOP] = y + thing->radius;
	tmbbox[BOXBOTTOM] = y - thing->radius;
	tmbbox[BOXRIGHT] = x + thing->radius;
	tmbbox[BOXLEFT] = x - thing->radius;

	// The base floor / ceiling is from the subsector that contains the point.
	// Any contacted lines the step closer together will adjust them.
	tmfloorz = tmdropoffz = P_FloorHeight(x, y, newsec);
	tmceilingz = P_CeilingHeight(x, y, newsec);
	tmfloorsector = newsec;
	tmthing = thing;
	tmflags = thing->flags;
	tmx = x;
	tmy = y;
	ceilingline = BlockingLine = NULL;

	validcount++;
	spechit.clear();

	if (tmflags & MF_NOCLIP && !(tmflags & MF_SKULLFLY))
		return true;

	// Check things first, possibly picking things up.
	// The bounding box is extended by MAXRADIUS
	// because DActors are grouped into mapblocks
	// based on their origin point, and can overlap
	// into adjacent blocks by up to MAXRADIUS units.
	int xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	int xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	int yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	int yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

	BlockingMobj = NULL;

	if (co_realactorheight && !spectator)
	{
		if (thing->player)	// [RH] Fake taller height to catch stepping up into things.
			thing->height += 24*FRACUNIT;

		for (int bx = xl; bx <= xh; bx++)
		{
			for (int by = yl; by <= yh; by++)
			{
				AActor *robin = NULL;
				do
				{
					if (!P_BlockThingsIterator (bx, by, PIT_CheckThing, robin))
					{ // [RH] If a thing can be stepped up on, we need to continue checking
					  // other things in the blocks and see if we hit something that is
					  // definitely blocking. Otherwise, we need to check the lines, or we
					  // could end up stuck inside a wall.
						if (BlockingMobj == NULL)
						{ // Thing slammed into something; don't let it move now.
							thing->height = realheight;
							return false;
						}
						else if (!BlockingMobj->player && thing->player &&
							BlockingMobj->z + BlockingMobj->height - thing->z <= 24*FRACUNIT)
						{
							if (thingblocker == NULL ||	BlockingMobj->z > thingblocker->z)
								thingblocker = BlockingMobj;

							robin = BlockingMobj->bmapnode.Next(bx, by);
							BlockingMobj = NULL;
						}
						else if (thing->player &&
							thing->z + thing->height - BlockingMobj->z <= 24*FRACUNIT)
						{
							if (thingblocker)
							{ // There is something to step up on. Return this thing as
							  // the blocker so that we don't step up.
								thing->height = realheight;
								return false;
							}
							// Nothing is blocking us, but this actor potentially could
							// if there is something else to step on.
							robin = BlockingMobj->bmapnode.Next(bx, by);
							BlockingMobj = NULL;
						}
						else
						{ // Definitely blocking
							thing->height = realheight;
							return false;
						}
					}
					else
					{
						robin = NULL;
					}
				} while (robin);
			}
		}

		BlockingMobj = NULL;
		thing->height = realheight;
		if (tmflags & MF_NOCLIP)
			return (BlockingMobj = thingblocker) == NULL;
		if (tmceilingz - tmfloorz < thing->height)
			return false;
	}
	else
	{
		// vanilla Doom's check for blocking things
		for (int bx=xl ; bx<=xh ; bx++)
			for (int by=yl ; by<=yh ; by++)
				if (!P_BlockThingsIterator(bx,by,PIT_CheckThing))
					return false;

		if (tmflags & MF_NOCLIP)
			return true;
	}

	// check lines
	xl = (tmbbox[BOXLEFT] - bmaporgx)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy)>>MAPBLOCKSHIFT;

	for (int bx=xl ; bx<=xh ; bx++)
		for (int by=yl ; by<=yh ; by++)
			if (!P_BlockLinesIterator (bx,by,PIT_CheckLine))
				return false;

	if (co_realactorheight)
		return (BlockingMobj = thingblocker) == NULL;

	return true;
}


//
// P_CheckOnmobj(AActor *thing)
// Checks if the new Z position is legal
//

AActor *P_CheckOnmobj (AActor *thing)
{
	fixed_t oldz;
	bool good;

	oldz = thing->z;
	P_FakeZMovement (thing);
	good = P_TestMobjZ (thing);
	thing->z = oldz;

	return good ? NULL : onmobj;
}

bool P_TestMobjZ (AActor *actor)
{
	int	xl,xh,yl,yh,bx,by;
	fixed_t x, y;

	if (actor->flags & MF_NOCLIP)
		return true;

	if (!(actor->flags & MF_SOLID))
		return true;

	tmx = x = actor->x;
	tmy = y = actor->y;
	tmthing = actor;

	tmbbox[BOXTOP] = y + actor->radius;
	tmbbox[BOXBOTTOM] = y - actor->radius;
	tmbbox[BOXRIGHT] = x + actor->radius;
	tmbbox[BOXLEFT] = x - actor->radius;
//
// the bounding box is extended by MAXRADIUS because actors are grouped
// into mapblocks based on their origin point, and can overlap into adjacent
// blocks by up to MAXRADIUS units
//
	xl = (tmbbox[BOXLEFT] - bmaporgx - MAXRADIUS)>>MAPBLOCKSHIFT;
	xh = (tmbbox[BOXRIGHT] - bmaporgx + MAXRADIUS)>>MAPBLOCKSHIFT;
	yl = (tmbbox[BOXBOTTOM] - bmaporgy - MAXRADIUS)>>MAPBLOCKSHIFT;
	yh = (tmbbox[BOXTOP] - bmaporgy + MAXRADIUS)>>MAPBLOCKSHIFT;

	for (bx = xl; bx <= xh; bx++)
		for (by = yl; by <= yh; by++)
			if (!P_BlockThingsIterator (bx, by, PIT_CheckOnmobjZ))
				return false;

	return true;
}


//
// P_FakeZMovement
// Fake the zmovement so that we can check if a move is legal
//
void P_FakeZMovement(AActor *mo)
{
//
// adjust height
//
	mo->z += mo->momz;
	if ((mo->flags&MF_FLOAT) && mo->target)
	{ // float down towards target if too close
		if (!(mo->flags & MF_SKULLFLY) && !(mo->flags & MF_INFLOAT))
		{
			fixed_t dist = P_AproxDistance (mo->x - mo->target->x, mo->y - mo->target->y);
			fixed_t delta = (mo->target->z + (mo->height>>1)) - mo->z;
			if (delta < 0 && dist < -(delta*3))
				mo->z -= FLOATSPEED;
			else if (delta > 0 && dist < (delta*3))
				mo->z += FLOATSPEED;
		}
	}
	if (mo->player && mo->flags2&MF2_FLY && (mo->z > mo->floorz))
	{
		mo->z += finesine[(FINEANGLES/80*level.time)&FINEMASK]/8;
	}

//
// clip movement
//
	if (mo->z <= mo->floorz)
	{ // hit the floor
		mo->z = mo->floorz;
	}

	if (mo->z + mo->height > mo->ceilingz)
	{ // hit the ceiling
		mo->z = mo->ceilingz - mo->height;
	}
}


void P_CheckPushLines(AActor *thing)
{
	if (!thing || !HasBehavior)
		return;

	if (!(thing->flags&(MF_TELEPORT|MF_NOCLIP)))
	{
		for (size_t i = 0; i < spechit.size(); i++)
		{
			// see which lines were pushed
			line_t *ld = spechit[i];
			int side = P_PointOnLineSide(thing->x, thing->y, ld);
			CheckForPushSpecial(ld, side, thing);
		}
	}
}

//
// P_TryMove
// Attempt to move to a new position,
// crossing special lines unless MF_TELEPORT is set.
//
BOOL P_TryMove (AActor *thing, fixed_t x, fixed_t y,
				bool dropoff, // killough 3/15/98: allow dropoff as option
				bool onfloor) // [RH] Let P_TryMove keep the thing on the floor
{
	fixed_t		testz = thing->z;
	sector_t*	oldsec = thing->subsector->sector;	// [RH] for sector actions

	floatok = false;

	if (onfloor)
		testz = P_FloorHeight(x, y, thing->floorsector);

	if (!P_CheckPosition(thing, x, y))
	{
		// Solid wall or thing
		if ((!BlockingMobj || BlockingMobj->player || !thing->player) ||
			(BlockingMobj->z + BlockingMobj->height - testz > (24 * FRACUNIT) ||
			(P_CeilingHeight(x, y, BlockingMobj->subsector->sector) -
			(BlockingMobj->z + BlockingMobj->height) < thing->height) ||
			(tmceilingz - (BlockingMobj->z + BlockingMobj->height) < thing->height)))
		{
			P_CheckPushLines(thing);
			return false;
		}

		if (!co_realactorheight || !(tmthing->flags2 & MF2_PASSMOBJ))
			return false;
	}

	if (onfloor && tmfloorsector == thing->floorsector)
		testz = tmfloorz;

	if (!(thing->flags & MF_NOCLIP) && !(thing->player && thing->player->spectator))
	{
		if (tmceilingz - tmfloorz < thing->height)
		{
			// doesn't fit
			P_CheckPushLines(thing);
			return false;
		}

		floatok = true;

		if (!(thing->flags & MF_TELEPORT)
			&& tmceilingz - testz < thing->height
			&& !(thing->flags2 & MF2_FLY))
		{
			// mobj must lower itself to fit
			P_CheckPushLines(thing);
			return false;
		}

		if (thing->flags2 & MF2_FLY && testz + thing->height > tmceilingz)
		{
			P_CheckPushLines(thing);
			return false;
		}

		if (!(thing->flags & MF_TELEPORT))
		{
			if (tmfloorz - testz > 24*FRACUNIT)
			{
				// too big a step up
				P_CheckPushLines(thing);
				return false;
			}
			else if (co_fixweaponimpacts && thing->flags & MF_MISSILE &&
					tmfloorz > testz)
			{
				// [SL] 2011-09-16 - Fix the vanilla Doom bug that allows
				// missiles to climb stairs.
				P_CheckPushLines(thing);
				return false;
			}
			else if (co_realactorheight && testz < tmfloorz)
			{
				// [RH] Check to make sure there's nothing in the way for the step up
				fixed_t savedz = thing->z;
				thing->z = tmfloorz;
				bool good = P_TestMobjZ(thing);
				thing->z = savedz;
				if (!good)
				{
					P_CheckPushLines(thing);
					return false;
				}
			}
		}

		// killough 3/15/98: Allow certain objects to drop off
		if (!(co_allowdropoff && dropoff) &&
			!(thing->flags & (MF_DROPOFF|MF_FLOAT|MF_MISSILE)) &&
			  tmfloorz - tmdropoffz > 24*FRACUNIT &&
			!(thing->flags2 & MF2_BLASTED))
		{ // Can't move over a dropoff unless it's been blasted
			return false;
		}

		// killough 11/98: prevent falling objects from going up too many steps
		if (co_zdoomphys && thing->flags & MF_FALLING && tmfloorz - testz >
			FixedMul(thing->momx, thing->momx) + FixedMul(thing->momy, thing->momy))
		{
			return false;
		}
	}

	// the move is ok, so link the thing into its new position
	thing->UnlinkFromWorld ();

	fixed_t oldx = thing->x;
	fixed_t oldy = thing->y;
	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->dropoffz = tmdropoffz;		// killough 11/98: keep track of dropoffs
	thing->floorsector = tmfloorsector;
	thing->x = x;
	thing->y = y;
	thing->z = testz;

	thing->LinkToWorld ();

	// if any special lines were hit, do the effect
	if (! (thing->flags&(MF_TELEPORT|MF_NOCLIP)) )
	{
		while (!spechit.empty())
		{
			// see if the line was crossed
			line_t *ld = spechit.back();
			spechit.pop_back();

			int side = P_PointOnLineSide (thing->x, thing->y, ld);
			int oldside = P_PointOnLineSide (oldx, oldy, ld);
			if (side != oldside && ld->special)
				P_CrossSpecialLine (ld-lines, oldside, thing);
		}
	}

	// [RH] If changing sectors, trigger transitions
	sector_t *newsec = thing->subsector->sector;
	if (oldsec != newsec)
	{
		if (oldsec->SecActTarget)
		{
			A_TriggerAction(oldsec->SecActTarget, thing, SECSPAC_Exit);
		}
		if (newsec->SecActTarget)
		{
			int act = SECSPAC_Enter;
			if (testz <= P_FloorHeight(thing->x, thing->y, newsec))
			{
				act |= SECSPAC_HitFloor;
			}
			if (testz + thing->height >= P_CeilingHeight(thing->x, thing->y, newsec))
			{
				act |= SECSPAC_HitCeiling;
			}
			A_TriggerAction(newsec->SecActTarget, thing, act);
		}
	}

	return true;
}

//
// killough 9/12/98:
//
// Apply "torque" to objects hanging off of ledges, so that they
// fall off. It's not really torque, since Doom has no concept of
// rotation, but it's a convincing effect which avoids anomalies
// such as lifeless objects hanging more than halfway off of ledges,
// and allows objects to roll off of the edges of moving lifts, or
// to slide up and then back down stairs, or to fall into a ditch.
// If more than one linedef is contacted, the effects are cumulative,
// so balancing is possible.
//

static BOOL PIT_ApplyTorque (line_t *ld)
{
	if (ld->backsector &&		// If thing touches two-sided pivot linedef
		tmbbox[BOXRIGHT]  > ld->bbox[BOXLEFT]  &&
		tmbbox[BOXLEFT]   < ld->bbox[BOXRIGHT] &&
		tmbbox[BOXTOP]    > ld->bbox[BOXBOTTOM] &&
		tmbbox[BOXBOTTOM] < ld->bbox[BOXTOP] &&
		P_BoxOnLineSide(tmbbox, ld) == -1)
	{
		AActor *mo = tmthing;

		fixed_t dist =								// lever arm
	  + (ld->dx >> FRACBITS) * (mo->y >> FRACBITS)
	  - (ld->dy >> FRACBITS) * (mo->x >> FRACBITS)
	  - (ld->dx >> FRACBITS) * (ld->v1->y >> FRACBITS)
	  + (ld->dy >> FRACBITS) * (ld->v1->x >> FRACBITS);

		if (dist < 0 ?								// dropoff direction
			P_FloorHeight(mo->x, mo->y, ld->frontsector) < mo->z &&
			P_FloorHeight(mo->x, mo->y, ld->backsector) >= mo->z :
				P_FloorHeight(mo->x, mo->y, ld->backsector) < mo->z &&
				P_FloorHeight(mo->x, mo->y, ld->frontsector) >= mo->z)
		{
		// At this point, we know that the object straddles a two-sided
		// linedef, and that the object's center of mass is above-ground.

			fixed_t x = abs(ld->dx), y = abs(ld->dy);

			if (y > x)
			{
				fixed_t t = x;
				x = y;
				y = t;
			}

			y = finesine[(tantoangle[FixedDiv(y,x)>>DBITS] +
				ANG90) >> ANGLETOFINESHIFT];

			// Momentum is proportional to distance between the
			// object's center of mass and the pivot linedef.
			//
			// It is scaled by 2^(OVERDRIVE - gear). When gear is
			// increased, the momentum gradually decreases to 0 for
			// the same amount of pseudotorque, so that oscillations
			// are prevented, yet it has a chance to reach equilibrium.

			dist = FixedDiv(FixedMul(dist, (mo->gear < OVERDRIVE) ?
							y << -(mo->gear - OVERDRIVE) :
							y >> +(mo->gear - OVERDRIVE)), x);

			// Apply momentum away from the pivot linedef.

			x = FixedMul(ld->dy, dist);
			y = FixedMul(ld->dx, dist);

			// Avoid moving too fast all of a sudden (step into "overdrive")

			dist = FixedMul(x,x) + FixedMul(y,y);

			while (dist > FRACUNIT*4 && mo->gear < MAXGEAR)
				++mo->gear, x >>= 1, y >>= 1, dist >>= 1;

			mo->momx -= x;
			mo->momy += y;
		}
	}
	return true;
}

//
// killough 9/12/98
//
// Applies "torque" to objects, based on all contacted linedefs
//

void P_ApplyTorque (AActor *mo)
{
	int xl = ((tmbbox[BOXLEFT] =
			mo->x - mo->radius) - bmaporgx) >> MAPBLOCKSHIFT;
	int xh = ((tmbbox[BOXRIGHT] =
			mo->x + mo->radius) - bmaporgx) >> MAPBLOCKSHIFT;
	int yl = ((tmbbox[BOXBOTTOM] =
			mo->y - mo->radius) - bmaporgy) >> MAPBLOCKSHIFT;
	int yh = ((tmbbox[BOXTOP] =
			mo->y + mo->radius) - bmaporgy) >> MAPBLOCKSHIFT;
	int bx,by;
	int flags = mo->flags;	//Remember the current state, for gear-change

	tmthing = mo;
	++validcount; // prevents checking same line twice

	for (bx = xl ; bx <= xh ; bx++)
		for (by = yl ; by <= yh ; by++)
			P_BlockLinesIterator(bx, by, PIT_ApplyTorque);

	// If any momentum, mark object as 'falling' using engine-internal flags
	if (mo->momx | mo->momy)
		mo->flags |= MF_FALLING;
	else  // Clear the engine-internal flag indicating falling object.
		mo->flags &= ~MF_FALLING;

	// If the object has been moving, step up the gear.
	// This helps reach equilibrium and avoid oscillations.
	//
	// Doom has no concept of potential energy, much less
	// of rotation, so we have to creatively simulate these
	// systems somehow :)

	if (!((mo->flags | flags) & MF_FALLING))	// If not falling for a while,
		mo->gear = 0;							// Reset it to full strength
	else if (mo->gear < MAXGEAR)				// Else if not at max gear,
		mo->gear++;								// move up a gear
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
	if (!thing)
		return true;

	bool onfloor = (thing->z <= thing->floorz);

	AActor *underthing = P_CheckOnmobj(thing);
	bool onthing = co_realactorheight && underthing && underthing->z < thing->z;

	// calculate new floorz/ceilingz, etc
	P_CheckPosition (thing, thing->x, thing->y);

	thing->floorz = tmfloorz;
	thing->ceilingz = tmceilingz;
	thing->dropoffz = tmdropoffz;
	thing->floorsector = tmfloorsector;

	// standing on another actor - adjust the actor underneath first
	if (onthing && !P_ThingHeightClip(underthing))
		return false;

	fixed_t newz = (onthing) ?
					underthing->z + underthing->height :
					thing->floorz;

	if (onfloor || onthing)
	{
		thing->z = newz;
	}
	else
	{
		// don't adjust a floating monster unless forced to
		if (thing->z + thing->height > thing->ceilingz)
			thing->z = thing->ceilingz - thing->height;
	}

	// thing won't fit
	if (thing->ceilingz - newz < thing->height)
		return false;

	return true;
}


//============================================================================
//
// P_CheckSlopeWalk
//
//============================================================================

bool P_CheckSlopeWalk (AActor *actor, fixed_t &xmove, fixed_t &ymove)
{
	if (!actor || actor->flags & MF_NOGRAVITY || !actor->floorsector)
		return false;

	const plane_t *plane = &actor->floorsector->floorplane;

	if (actor->floorsector != actor->subsector->sector)
		return false;

	// Don't bother with non-sloping floors
	if (P_IsPlaneLevel(plane))
		return false;

	fixed_t floorheight = P_FloorHeight(actor->x, actor->y, actor->floorsector);

	// not on floor ?
	if (actor->z - floorheight > FRACUNIT)
		return false;

	fixed_t destx = actor->x + xmove;
	fixed_t desty = actor->y + ymove;
	fixed_t t = FixedMul(plane->a, destx) + FixedMul(plane->b, desty) +
				FixedMul(plane->c, actor->z) + plane->d;

	if (t < 0)
	{ // Desired location is behind (below) the plane
	  // (i.e. Walking up the plane)
		if (plane->c < STEEPSLOPE)
		{
			// Can't climb up slopes of ~45 degrees or more
			if (actor->flags & MF_NOCLIP || (actor->player && actor->player->spectator))
				return true;
			else
			{
				const msecnode_t *node;
				bool dopush = true;

				if (plane->c > STEEPSLOPE*2/3)
				{
					for (node = actor->touching_sectorlist; node; node = node->m_tnext)
					{
						const sector_t *sec = node->m_sector;
						if (sec->floorplane.c >= STEEPSLOPE)
						{
							if (P_FloorHeight(destx, desty, sec) >= actor->z - 24*FRACUNIT)
							{
								dopush = false;
								break;
							}
						}
					}
				}
				if (dopush)
				{
					xmove = actor->momx = plane->a * 2;
					ymove = actor->momy = plane->b * 2;
				}
				return false;
			}
		}

		// Slide the desired location along the plane's normal
		// so that it lies on the plane's surface
		xmove -= FixedMul(plane->a, t);
		ymove -= FixedMul(plane->b, t);
		return true;
	}
	else if (t > 0)
	{ // Desired location is in front of (above) the plane
		if (floorheight == actor->z)
		{
			// Actor's current spot is on/in the plane, so walk down it
			// Same principle as walking up, except reversed
			xmove += FixedMul(plane->a, t);
			ymove += FixedMul(plane->b, t);
			return true;
		}
	}

	return false;
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
		!slidemo->player->spectator &&	// Ch0wW: disables bouncing as a flying spectator. (prevents grunt1 sound from playing)
		(P_AproxDistance(tmxmove, tmymove) > 4*FRACUNIT) &&
		slidemo->z <= slidemo->floorz &&		  // killough 8/28/98: calc friction on demand
		P_GetFriction (slidemo, NULL) > ORIG_FRICTION;

	if (ld->slopetype == ST_HORIZONTAL)
	{
		if (icyfloor && (abs(tmymove) > abs(tmxmove)))
		{
			tmxmove /= 2; // absorb half the momentum
			tmymove = -tmymove/2;
			UV_SoundAvoidPlayer(slidemo, CHAN_VOICE, "player/male/grunt1", ATTN_IDLE);
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
			UV_SoundAvoidPlayer(slidemo, CHAN_VOICE, "player/male/grunt1", ATTN_IDLE);
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
		UV_SoundAvoidPlayer(slidemo, CHAN_VOICE, "player/male/grunt1", ATTN_IDLE);

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
		I_Error ("PTR_SlideTraverse: non-line intercept\n");

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
	P_LineOpening(li, trace.x + FixedMul(trace.dx, in->frac),
				trace.y + FixedMul(trace.dy, in->frac));

	if (openrange < slidemo->height)
		goto isblocking;				// doesn't fit

	if (opentop - slidemo->z < slidemo->height)
		goto isblocking;				// mobj is too high

	if (openbottom - slidemo->z > 24*FRACUNIT)
	{
		goto isblocking;				// too big a step up
	}
	else if (co_zdoomphys && slidemo->z < openbottom)
	{
		// [RH] Check to make sure there's nothing in the way for the step up
		fixed_t savedz = slidemo->z;
		slidemo->z = openbottom;
		bool good = P_TestMobjZ (slidemo);
		slidemo->z = savedz;
		if (!good)
			goto isblocking;
	}

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

    bool		walkplane;

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
		fixed_t xmove, ymove;
		// the move must have hit the middle, so stairstep
	  stairstep:
		xmove = 0, ymove = mo->momy;
		walkplane = P_CheckSlopeWalk (mo, xmove, ymove);
		if (!P_TryMove (mo, mo->x + xmove, mo->y + ymove, true, walkplane))
		{
			xmove = mo->momx, ymove = 0;
			walkplane = P_CheckSlopeWalk (mo, xmove, ymove);
			P_TryMove (mo, mo->x + xmove, mo->y + ymove, true, walkplane);
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

	walkplane = P_CheckSlopeWalk (mo, tmxmove, tmymove);

	if (!P_TryMove (mo, mo->x+tmxmove, mo->y+tmymove, true, walkplane))
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
		P_LineOpening(li, trace.x + FixedMul(trace.dx, in->frac),
				trace.y + FixedMul(trace.dy, in->frac));

		if (openbottom >= opentop)
			return false;				// stop

		dist = FixedMul (attackrange, in->frac);

		// [SL] 2012-02-08 - Calculate the point where the intercept crosses
		// the line
		fixed_t crossx = trace.x + FixedMul(trace.dx, in->frac);
		fixed_t crossy = trace.y + FixedMul(trace.dy, in->frac);

		if (P_FloorHeight(crossx, crossy, li->frontsector) !=
			P_FloorHeight(crossx, crossy, li->backsector))
		{
			slope = FixedDiv (openbottom - shootz , dist);
			if (slope > bottomslope)
				bottomslope = slope;
		}

		if (P_CeilingHeight(crossx, crossy, li->frontsector) !=
			P_CeilingHeight(crossx, crossy, li->backsector))
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

	// [SL] 2011-10-31 - Don't aim at teammates
	if ((sv_gametype == GM_CTF || sv_gametype == GM_TEAMDM) &&
		shootthing->player && th->player &&
		shootthing->player->userinfo.team == th->player->userinfo.team &&
		!sv_friendlyfire)
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
// P_ShootLine
//
// Helper function for PTR_ShootTraverse. Spawns a bullet puff if the intercept
// hits a line or the floor/ceiling. Returns true if the intercept should continue
// because it did not hit a solid line.
//
bool P_ShootLine(intercept_t* in)
{
	bool precise = (co_fixweaponimpacts != 0);
	line_t* li = in->d.line;

	if (!in->isaline)
		return true;

	if (li->special)
		P_ShootSpecialLine(shootthing, li);

	// don't shoot horizon lines
	if (li->special == Line_Horizon)
		return false;

	// [SL] 2012-02-08 - Calculates where the intercept crosses the line
	fixed_t crossx = trace.x + FixedMul(trace.dx, in->frac);
	fixed_t crossy = trace.y + FixedMul(trace.dy, in->frac);

	// [SL] determine which sector is on the side of the line that faces the shooter
	sector_t *sec1, *sec2;
	if (!precise || !li->backsector	|| !P_PointOnLineSide(trace.x, trace.y, li))
	{
		sec1 = li->frontsector;
		sec2 = li->backsector;
	}
	else
	{
		sec1 = li->backsector;
		sec2 = li->frontsector;
	}

	fixed_t ceilingheight1 = P_CeilingHeight(crossx, crossy, sec1);
	fixed_t ceilingheight2 = sec2 ? P_CeilingHeight(crossx, crossy, sec2) : MAXINT;
	fixed_t floorheight1 = P_FloorHeight(crossx, crossy, sec1);
	fixed_t floorheight2 = sec2 ? P_FloorHeight(crossx, crossy, sec2) : MAXINT;

	// position the destination for the bullet puff a bit closer
	fixed_t frac = in->frac - FixedDiv(4 * FRACUNIT, attackrange);
	fixed_t z = shootz + FixedMul(aimslope, FixedMul(frac, attackrange));

	if (li->flags & ML_TWOSIDED && !(li->flags & ML_BLOCKEVERYTHING))
	{
		// crosses a two sided line
		P_LineOpening(li, trace.x + FixedMul(trace.dx, in->frac), trace.y + FixedMul(trace.dy, in->frac));

		if (precise)
		{
			if (z < opentop && z > openbottom)
			{
				// shot didn't hit the solid part of this line
				opentop = ceilingheight2;
				openbottom = floorheight2;
				return true;
			}
		}
		else
		{
			// e6y: emulation of missed back side on two-sided lines.
			// backsector can be NULL when emulating missing back side.

			fixed_t dist = FixedMul(attackrange, in->frac);
			bool hittop = (li->backsector == NULL || ceilingheight1 != ceilingheight2) &&
						FixedDiv(opentop - shootz, dist) < aimslope;
			bool hitbottom = (li->backsector == NULL || floorheight1 != floorheight2) &&
						FixedDiv(openbottom - shootz, dist) > aimslope;

			if (!hittop && !hitbottom)
				return true;	// shot didn't hit the solid part of this line
		}
	}

	// definitely hit the solid part of the line

	bool skyceiling1 = sec1->ceilingpic == skyflatnum;
	bool skyceiling2 = sec2 && sec2->ceilingpic == skyflatnum;

	// sky wall hack
	if (skyceiling1 && skyceiling2)
	{
		if (!precise || z > ceilingheight2)
			return false;
	}

	// check for shooting skies
	if (skyceiling1 && z > ceilingheight1)
		return false;

	// check for shooting sky floors
	if (precise && sec1->floorpic == skyflatnum && z < floorheight1)
		return false;

	v3fixed_t lineorg, linedir, puffpos;

	// set origin vector for the bullet
	M_SetVec3Fixed(&lineorg, trace.x, trace.y, shootz);
	// set direction vector for the bullet
	M_SetVec3Fixed(&linedir, trace.dx, trace.dy, FixedMul(aimslope, attackrange));

	if (precise && z > ceilingheight1)
	{
		puffpos = P_LinePlaneIntersection(&sec1->ceilingplane, lineorg, linedir);
	}
	else if (precise && z < floorheight1)
	{
		puffpos =  P_LinePlaneIntersection(&sec1->floorplane, lineorg, linedir);
	}
	else
	{
		puffpos.x = trace.x + FixedMul(trace.dx, frac);
		puffpos.y = trace.y + FixedMul(trace.dy, frac);
		puffpos.z = z;
	}

	// Spawn bullet puffs.
	P_SpawnPuff(puffpos.x, puffpos.y, puffpos.z);
	return false;
}


//
// PTR_ShootTraverse
//
BOOL PTR_ShootTraverse (intercept_t* in)
{
	fixed_t x, y, z;
	fixed_t frac;
	AActor *th;
	fixed_t thingtopslope, thingbottomslope;

	if (in->isaline)
		return P_ShootLine(in);

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
	fixed_t dist = FixedMul(attackrange, in->frac);
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
	bool spawnblood = !(in->d.thing->flags & MF_NOBLOOD);

	// [SL] 2011-05-11 - In unlagged games, spawn blood at the target's current
	// position, not at their reconciled position
	if (shootthing->player && th->player)
	{
		fixed_t xoffs = 0, yoffs = 0, zoffs = 0;
		Unlag::getInstance().getReconciliationOffset(th->player->id, xoffs, yoffs, zoffs);
		x += xoffs; y += yoffs; z += zoffs;

		if (P_AreTeammates(*shootthing->player, *th->player) && !sv_friendlyfire)
			spawnblood = false;
	}

	if (spawnblood)
		P_SpawnBlood(x, y, z, la_damage);
	else
		P_SpawnPuff(x, y, z);

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

	// [RH] Technically, this is now correct for an engine with true 6 DOF
	// instead of one which implements y-shearing, like we currently do.
	angle_t topangle = t1->pitch - ANG(32);
	angle_t bottomangle = t1->pitch + ANG(32);

	if (topangle <= ANG360 - ANG180)
		topslope = finetangent[FINEANGLES/2-1];
	else
		topslope = finetangent[FINEANGLES/4-((signed)topangle>>ANGLETOFINESHIFT)];

	if (bottomangle >= ANG180)
		bottomslope = finetangent[0];
	else
		bottomslope = finetangent[FINEANGLES/4-((signed)bottomangle>>ANGLETOFINESHIFT)];

	attackrange = distance;
	linetarget = NULL;

	P_PathTraverse (t1->x, t1->y, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_AimTraverse);

	if (linetarget)
		return aimslope;

	return 0;
}

/**
 * A function created especially for implementing player autoaim.  First
 * attempts to shoot a tracer straight ahead, then tries progressively wider
 * and wider tracers in a short cone.
 *
 * @param  actor    Source actor.
 * @param  angle    Angle of shot.  Set to the 'correct' angle on return.
 * @param  spread   Maximum spread angle of shot, plus or minus 0 degrees.
 *                  Vanilla is 1 << 26.
 * @param  tracers  Number of tracers to shoot per side.  Fewer tracers means
 *                  you're less likely to hit a distant object that is between
 *                  straigt ahead and the maximum spread.  Vanilla is 1.
 * @param  distance Distance to shoot the tracer.  Vanilla is 16 * 64 * FRACUNIT.
 * @return          Slope of the resulting shot.
 */
fixed_t P_AutoAimLineAttack(AActor* actor, angle_t& angle, const angle_t spread, const int tracers, fixed_t distance)
{
	fixed_t slope = P_AimLineAttack(actor, angle, distance);

	if (linetarget)
		return slope;

	angle_t originangle = angle;
	angle_t testspread = spread / tracers;

	for (int i = 1;i <= tracers;i++)
	{
		angle = originangle;
		if (i == tracers)
			angle += spread;
		else
			angle += testspread * i;
		slope = P_AimLineAttack(actor, angle, distance);
		if (linetarget)
			return slope;

		if (i == tracers)
			angle -= spread * 2;
		else
			angle -= testspread * i * 2;
		slope = P_AimLineAttack(actor, angle, distance);
		if (linetarget)
			return slope;
	}

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
	opentop = P_CeilingHeight(t1);
	openbottom = P_FloorHeight(t1);

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
			if ( (subsector = P_PointInSubsector (x2, y2)) ) {
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
// [RH] PTR_RailTraverse
//
static int MaxRailHits, NumRailHits;
static struct SRailHit {
	AActor *hitthing;
	fixed_t x,y,z;
} *RailHits;
static v3double_t RailEnd;

BOOL PTR_RailTraverse (intercept_t *in)
{
	fixed_t 			x;
	fixed_t 			y;
	fixed_t 			z;
	fixed_t 			frac;

	line_t* 			li;

	AActor* 			th;

	fixed_t 			dist;
	fixed_t 			thingtopslope;
	fixed_t 			thingbottomslope;
	fixed_t				floorheight;
	fixed_t				ceilingheight;

	if (in->isaline)
	{
		li = in->d.line;

		fixed_t crossx = trace.x + FixedMul (trace.dx, in->frac);
		fixed_t crossy = trace.y + FixedMul (trace.dy, in->frac);

		// [SL] 2012-04-18 - origin and direction vectors for the shot
		v3fixed_t lineorg, linedir;
		M_SetVec3Fixed(&lineorg, trace.x, trace.y, shootz);
		M_SetVec3Fixed(&linedir, trace.dx, trace.dy, FixedMul(aimslope, attackrange));

		frac = in->frac;
		z = shootz + FixedMul (aimslope, FixedMul (frac, attackrange));

		if (!(li->flags & ML_TWOSIDED) || (li->flags & ML_BLOCKEVERYTHING))
			goto hitline;

		// crosses a two sided line
		P_LineOpening(li, trace.x + FixedMul(trace.dx, in->frac),
				trace.y + FixedMul(trace.dy, in->frac));

		if (z >= opentop || z <= openbottom)
			goto hitline;

		// shot continues

		// TODO: handle SPAC_PCROSS specials

		return true;


		// hit line
	  hitline:
		plane_t *floorplane, *ceilingplane;

		if (!li->backsector || !P_PointOnLineSide (trace.x, trace.y, li))
		{
			ceilingplane = &li->frontsector->ceilingplane;
			floorplane = &li->frontsector->floorplane;

			ceilingheight = P_CeilingHeight(crossx, crossy, li->frontsector);
			floorheight = P_FloorHeight(crossx, crossy, li->frontsector);
		}
		else
		{
			ceilingplane = &li->backsector->ceilingplane;
			floorplane = &li->backsector->floorplane;

			ceilingheight = P_CeilingHeight(crossx, crossy, li->backsector);
			floorheight = P_FloorHeight(crossx, crossy, li->backsector);
		}

		if (z < floorheight) {
			// [SL] 2012-04-18 - Calculate where the the tracer intersects
			// with the floor plane
			v3fixed_t pt = P_LinePlaneIntersection(floorplane, lineorg, linedir);
			x = pt.x;  y = pt.y;  z = pt.z;
		} else if (z > ceilingheight) {
			// [SL] 2012-04-18 - Calculate where the the tracer intersects
			// with the ceiling plane
			v3fixed_t pt = P_LinePlaneIntersection(ceilingplane, lineorg, linedir);
			x = pt.x;  y = pt.y;  z = pt.z;
		} else {
			x = trace.x + FixedMul(trace.dx, frac);
			y = trace.y + FixedMul(trace.dy, frac);

			// Shot actually hit a wall. It might be set up for shoot activation
			if (li->special)
				P_ShootSpecialLine(shootthing, li);
		}

		// Save final position of rail shot.
		M_SetVec3(&RailEnd, x, y, z);

		// don't go any farther
		return false;
	}

	// shoot a thing
	th = in->d.thing;
	if (th == shootthing)
		return true;			// can't shoot self

	if (!(th->flags & MF_SHOOTABLE))
		return true;			// corpse or something

	// check angles to see if the thing can be aimed at
	dist = FixedMul (attackrange, in->frac);
	thingtopslope = FixedDiv (th->z+th->height - shootz , dist);

	if (thingtopslope < aimslope)
		return true;			// shot over the thing

	thingbottomslope = FixedDiv (th->z - shootz, dist);

	if (thingbottomslope > aimslope)
		return true;			// shot under the thing

	// hit thing
	// if it's invulnerable, it completely blocks the shot
	if (th->flags2 & MF2_INVULNERABLE)
		return false;

	// position a bit closer
	frac = in->frac - FixedDiv (10*FRACUNIT,attackrange);

	x = trace.x + FixedMul (trace.dx, frac);
	y = trace.y + FixedMul (trace.dy, frac);
	z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

	// Save this thing for damaging later
	if (NumRailHits >= MaxRailHits)
	{
		MaxRailHits = MaxRailHits ? MaxRailHits * 2 : 16;
		RailHits = (SRailHit *)Realloc (RailHits, sizeof(*RailHits) * MaxRailHits);
	}
	RailHits[NumRailHits].hitthing = th;
	RailHits[NumRailHits].x = x;
	RailHits[NumRailHits].y = y;
	RailHits[NumRailHits].z = z;
	NumRailHits++;

	// continue the trace
	return true;
}

void P_RailAttack (AActor *source, int damage, int offset)
{
	v3double_t start, end;

	int angleidx = (source->angle - ANG90) >> ANGLETOFINESHIFT;
	fixed_t x1 = source->x + offset*finecosine[angleidx];
	fixed_t y1 = source->y + offset*finesine[angleidx];

	angleidx = (source->angle) >> ANGLETOFINESHIFT;
	fixed_t x2 = source->x + 8192*finecosine[angleidx];
	fixed_t y2 = source->y + 8192*finesine[angleidx];

	shootz = source->z + (source->height >> 1) + 8*FRACUNIT;
	attackrange = 8192*FRACUNIT;
	aimslope = finetangent[FINEANGLES/4-(source->pitch>>ANGLETOFINESHIFT)];
	shootthing = source;
	NumRailHits = 0;

	M_SetVec3(&start, x1, y1, shootz);

	if (P_PathTraverse (x1, y1, x2, y2, PT_ADDLINES|PT_ADDTHINGS, PTR_RailTraverse))
	{
		// Nothing hit, so just shoot the air
		M_AngleToVec3(&end, source->angle, source->pitch);

		M_ScaleVec3(&end, &end, 8192.0);
		M_AddVec3(&end, &start, &end);
	}
	else
	{
		// Hit a wall, maybe some things as well
		end = RailEnd;

		for (int i = 0; i < NumRailHits; i++)
		{
			if (RailHits[i].hitthing->flags & MF_NOBLOOD)
				P_SpawnPuff(RailHits[i].x, RailHits[i].y, RailHits[i].z);
			else
				P_SpawnBlood(RailHits[i].x, RailHits[i].y, RailHits[i].z, damage);
			P_DamageMobj (RailHits[i].hitthing, source, source, damage, MOD_RAILGUN);
		}
	}

	if (clientside)
		P_DrawRailTrail (start, end);
	else
	{
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			AActor *mo = it->mo;
			if (!mo || mo == source)
				continue;

			buf_t* buf = &(it->client.netbuf);
			MSG_WriteMarker(buf, svc_railtrail);
			MSG_WriteShort(buf, short(start.x));
			MSG_WriteShort(buf, short(start.y));
			MSG_WriteShort(buf, short(start.z));
			MSG_WriteShort(buf, short(end.x));
			MSG_WriteShort(buf, short(end.y));
			MSG_WriteShort(buf, short(end.z));
		}
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

	fixed_t crossx = trace.x + FixedMul(trace.dx, in->frac);
	fixed_t crossy = trace.y + FixedMul(trace.dy, in->frac);

	frac = in->frac - CAMERA_DIST;
	z = shootz + FixedMul (aimslope, FixedMul(frac, attackrange));

	li = in->d.line;

	if (!(li->flags & ML_TWOSIDED))
		goto hitline;

	// crosses a two sided line
	P_LineOpening(li, crossx, crossy);

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
			ceilingheight = P_CeilingHeight(crossx, crossy, li->frontsector);
			floorheight = P_FloorHeight(crossx, crossy, li->frontsector);
		} else {
			ceilingheight = P_CeilingHeight(crossx, crossy, li->backsector);
			floorheight = P_FloorHeight(crossx, crossy, li->backsector);
		}
		ceilingheight -= CAMERA_DIST;
		floorheight += CAMERA_DIST;

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
	subsector = P_PointInSubsector (x2, y2);
	if (subsector) {
		fixed_t ceilingheight = P_CeilingHeight(x2, y2, subsector->sector) - CAMERA_DIST;
		fixed_t floorheight = P_FloorHeight(x2, y2, subsector->sector) + CAMERA_DIST;
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
bool foundline;

BOOL PTR_UseTraverse (intercept_t *in)
{
	if (!in->isaline)
		I_Error ("PTR_UseTraverse: non-line intercept\n");

	if (!in->d.line->special)
	{
		P_LineOpening(in->d.line, trace.x + FixedMul(trace.dx, in->frac),
				trace.y + FixedMul(trace.dy, in->frac));

		if (openrange <= 0)
		{
			// [RH] Give sector a chance to intercept the use
			sector_t *sec = in->d.line->frontsector;
			if ((!sec->SecActTarget ||
			    !A_TriggerAction(sec->SecActTarget, usething, SECSPAC_Use|SECSPAC_UseWall)) &&
			    usething->player)
			{
				UV_SoundAvoidPlayer(usething, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
			}
			// can't use through a wall
			return false;
		}

		foundline = true;
		return true; // not a special line, but keep checking
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
	if (!in->isaline)
		I_Error ("PTR_NoWayTraverse: non-line intercept\n");

	line_t *ld = in->d.line;					// This linedef

	return ld->special || !(					// Ignore specials
		ld->flags & (ML_BLOCKING|ML_BLOCKEVERYTHING) || (		// Always blocking
		P_LineOpening(ld, trace.x + FixedMul(trace.dx, in->frac),
				trace.y + FixedMul(trace.dy, in->frac)),		// Find openings
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
	foundline = false;

	//Added by MC: Check if bot and use special activating (spin round) if it is.
	angle = player->mo->angle >> ANGLETOFINESHIFT;

	x1 = player->mo->x;
	y1 = player->mo->y;
	x2 = x1 + (USERANGE>>FRACBITS)*finecosine[angle];
	y2 = y1 + (USERANGE>>FRACBITS)*finesine[angle];

	if (P_PathTraverse (x1, y1, x2, y2, PT_ADDLINES, PTR_UseTraverse)) {
		// [RH] Give sector a chance to eat the use
		sector_t *sec = usething->subsector->sector;
		int spac = SECSPAC_Use;
		if (foundline)
			spac |= SECSPAC_UseWall;
		if ((!sec->SecActTarget || !A_TriggerAction(sec->SecActTarget, usething, spac)) &&
		    (co_boomphys && !P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_NoWayTraverse)))
		{
			// This added test makes the "oof" sound work on 2s lines -- killough:
			// [ML] It also apparently allows additional silent bfg tricks not present in vanilla...
			// [ML] co_boomlinecheck now part of co_boomphys
			UV_SoundAvoidPlayer(usething, CHAN_VOICE, "player/male/grunt1", ATTN_NORM);
		}
	}
}

//
// RADIUS ATTACK
//
static AActor* 		bombsource;
static AActor* 		bombspot;
static int			bombdamage;
static float		bombdamagefloat;
static int			bombdistance;
static float		bombdistancefloat;
static bool			DamageSource;
static int			bombmod;

// [RH] Damage scale to apply to thing that shot the missile. (co_zdoomphys)
static float selfthrustscale;

CVAR_FUNC_IMPL(sv_splashfactor)
{
	selfthrustscale = 1.0f / var;
}


//
// PIT_DoomRadiusAttack
//
// "bombsource" is the creature that caused the explosion at "bombspot".
//
static BOOL PIT_DoomRadiusAttack(AActor* thing)
{
	if (!serverside || !(thing->flags & MF_SHOOTABLE))
		return true;

	// Boss spider and cyborg
	// take no damage from concussion.
	if (thing->type == MT_CYBORG || thing->type == MT_SPIDER)
		return true;

	fixed_t dx = abs(thing->x - bombspot->x);
	fixed_t dy = abs(thing->y - bombspot->y);
	fixed_t dist = (MAX(dx, dy) - thing->radius) >> FRACBITS;

	if (dist < 0)
		dist = 0;

	if (dist >= bombdamage)
		return true;	// out of range

	if (P_CheckSight(thing, bombspot))
	{
		// must be in direct path
		P_DamageMobj(thing, bombspot, bombsource, (bombdamage - dist) * sv_splashfactor, bombmod);
	}

	return true;
}


//
// PIT_ZDoomRadiusAttack
//
// "bombsource" is the creature that caused the explosion at "bombspot".
// [RH] Now it knows about vertical distances and can thrust things vertically, too.
//
static BOOL PIT_ZDoomRadiusAttack(AActor* thing)
{
	if (!serverside || !(thing->flags & MF_SHOOTABLE))
		return true;

	// Boss spider and cyborg
	// take no damage from concussion.
	if (thing->flags2 & MF2_BOSS)
		return true;

	// Barrels always use the original code, since this makes
	// them far too "active." BossBrains also use the old code
	// because some user levels require they have a height of 16,
	// which can make them near impossible to hit with the new code.
	if (bombspot->type == MT_BARREL || thing->type == MT_BARREL ||
		thing->type == MT_BOSSBRAIN)
	{
		return PIT_DoomRadiusAttack(thing);
	}

	// [RH] New code. The bounding box only covers the
	// height of the thing and not the height of the map.
	fixed_t dx = abs(thing->x - bombspot->x);
	fixed_t dy = abs(thing->y - bombspot->y);
	float len = float(MAX(dx, dy));
	float boxradius = float(thing->radius);

	if (bombspot->z < thing->z || bombspot->z >= thing->z + thing->height)
	{
		float dz;

		if (bombspot->z > thing->z)
			dz = float(thing->z + thing->height - bombspot->z);
		else
			dz = float(thing->z - bombspot->z);

		if (len <= boxradius)
			len = dz;
		else
		{
			len -= boxradius;
			len = sqrtf(len*len + dz*dz);
		}
	}
	else
	{
		len -= boxradius;
		if (len < 0.0f)
			len = 0.0f;
	}

	float points = bombdamage - (len / FRACUNIT) + 1.0f;
	if (thing == bombsource)
		points *= sv_splashfactor;

	if (points > 0.0f && P_CheckSight(thing, bombspot))
	{
		// OK to damage; target is in direct path

		fixed_t momx = thing->momx;
		fixed_t momy = thing->momy;
		int damage = (int)points;

		P_DamageMobj(thing, bombspot, bombsource, damage, bombmod);

		float thrust = points * 0.5f / thing->info->mass;
		if (bombsource == thing)
			thrust *= selfthrustscale;

		float momz = (float)(thing->z + (thing->height>>1) - bombspot->z) * thrust;
		if (bombsource != thing)
			momz *= 0.5f;
		else
			momz *= 0.8f;

		thing->momx = momx + (fixed_t)((thing->x - bombspot->x) * thrust);
		thing->momy = momy + (fixed_t)((thing->y - bombspot->y) * thrust);
		thing->momz += (fixed_t)momz;
	}

	return true;
}

//
// P_RadiusAttack
// Source is the creature that caused the explosion at spot.
//
void P_RadiusAttack(AActor *spot, AActor *source, int damage, int distance,
	bool hurtSource, int mod)
{
	fixed_t dist = (distance+MAXRADIUS)<<FRACBITS;
	int yh = MIN<int>((spot->y + dist - bmaporgy)>>MAPBLOCKSHIFT, bmapheight - 1);
	int yl = MAX<int>((spot->y - dist - bmaporgy)>>MAPBLOCKSHIFT, 0);
	int xh = MIN<int>((spot->x + dist - bmaporgx)>>MAPBLOCKSHIFT, bmapwidth - 1);
	int xl = MAX<int>((spot->x - dist - bmaporgx)>>MAPBLOCKSHIFT, 0);
	bombspot = spot;
	bombsource = source;
	bombdamage = damage;
	bombdistance = distance;
	bombdistancefloat = 1.f / (float)distance;
	DamageSource = hurtSource;
	bombdamagefloat = (float)damage;
	bombmod = mod;

	// decide which radius attack function to use
	BOOL (*pAttackFunc)(AActor*) = co_zdoomphys ?
		PIT_ZDoomRadiusAttack : PIT_DoomRadiusAttack;

	if (co_blockmapfix)
	{
		// [SL] 2012-12-03 - With co_blockmapfix, an actor can get radius
		// damage more than once since an actor can be in more than one block.
		// So we make a list of unique actors in the surrounding blocks and
		// then call the radius attack function once for each actor.

		std::set<AActor*> actorset;
		for (int y=yl ; y<=yh ; y++)
		{
			for (int x=xl ; x<=xh ; x++)
			{
				AActor *mobj = blocklinks[y*bmapwidth+x];
				while (mobj)
				{
					actorset.insert(mobj);
					mobj = mobj->bmapnode.Next(x, y);
				}
			}
		}

		std::set<AActor*>::iterator itr = actorset.begin();
		while (itr != actorset.end())
		{
			pAttackFunc(*itr);
			++itr;
		}
	}
	else
	{
		for (int y=yl ; y<=yh ; y++)
			for (int x=xl ; x<=xh ; x++)
				P_BlockThingsIterator (x, y, pAttackFunc);
	}
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

			mo->momx = P_RandomDiff (mo) << 12;
			mo->momy = P_RandomDiff (mo) << 12;
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
	if (!sector)
		return true;

	nofit = false;
	crushchange = crunch;

	// [ML] co_boomsectortouch now part of co_boomphys
	if (co_boomphys)
	{
		msecnode_t *n;

		// killough 4/4/98: scan list front-to-back until empty or exhausted,
		// restarting from beginning after each thing is processed. Avoids
		// crashes, and is sure to examine all things in the sector, and only
		// the things which are in the sector, until a steady-state is reached.
		// Things can arbitrarily be inserted and removed and it won't mess up.
		//
		// killough 4/7/98: simplified to avoid using complicated counter

		// Mark all things invalid

		for (n=sector->touching_thinglist; n; n=n->m_snext)
			n->visited = false;

		do
			for (n=sector->touching_thinglist; n; n=n->m_snext)	// go through list
				if (!n->visited)								// unprocessed thing found
				{
					n->visited	= true; 						// mark thing as processed
					if (!(n->m_thing->flags & MF_NOBLOCKMAP))	//jff 4/7/98 don't do these
						PIT_ChangeSector(n->m_thing); 			// process it
					break;										// exit and start over
				}
		while (n);	// repeat from scratch until all things left are marked valid
	}
	else
	{
		int x, y;

		// re-check heights for all things near the moving sector
		for (x=sector->blockbox[BOXLEFT] ; x<= sector->blockbox[BOXRIGHT] ; x++)
			for (y=sector->blockbox[BOXBOTTOM];y<= sector->blockbox[BOXTOP] ; y++)
				P_BlockThingsIterator (x, y, PIT_ChangeSector);

	}

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
	if (!node)
		return;

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
	if (!s)
		return NULL;

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
	int last_tmx = tmx, last_tmy = tmy;
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

//
// P_InvertPlane
//
// Changes the direction of a plane's normal vector to the opposite direction
//
void P_InvertPlane(plane_t *plane)
{
	if (plane)
	{
		plane->a = -plane->a;
		plane->b = -plane->b;
		plane->c = -plane->c;
		plane->d = -plane->d;
		plane->invc = -plane->invc;
	}
}

//
// P_IsPlaneLevel
//
// Returns true if the z-value is the same for all points on the plane.
// This occurs when the a and b coefficents of the planar equation are 0
// since 0*x + 0*y + c*z + d = 0
//
// In other words, is this a plain vanilla Doom floor/ceiling or a sloping one?
//
bool P_IsPlaneLevel(const plane_t *plane)
{
	if (!plane)
		return false;

	return (plane->a | plane->b) == 0;
}

fixed_t P_PlaneZ(fixed_t x, fixed_t y, const plane_t *plane)
{
	if (!plane)
		return MAXINT;

	// Is the plane level?  (Z value is constant for entire plane)
	if (P_IsPlaneLevel(plane))
		return -FixedMul(plane->c, plane->d);

	return -FixedMul(plane->invc, FixedMul(plane->a, x) + FixedMul(plane->b, y) + plane->d);
}

double P_PlaneZ(double x, double y, const plane_t *plane)
{
	if (!plane)
		return MAXINT / 65536.0;

	static const double m = 1.0 / (65536.0 * 65536.0);

	// Is the plane level?  (Z value is constant for entire plane)
	if (P_IsPlaneLevel(plane))
		return -double(plane->c) * plane->d * m;

	return -(plane->a * x + plane->b * y + plane->d) / plane->c;
}


//
// P_FloorHeight()
//
// Returns the height of a floor plane at the point (x, y).  The subsector
// parameter is optional but provides a speedup if used because P_PlaneZ
// can avoid a call to P_PointInSubsector().
//
// Note that there is no check made to ensure the point (x, y) is actually
// within the subsector.
//
// MAXINT is returned if the point is not within any sector.
//
fixed_t P_FloorHeight(fixed_t x, fixed_t y, const sector_t *sector)
{
	if (!sector && !(sector = P_PointInSubsector(x, y)->sector))
		return MAXINT;

	return P_PlaneZ(x, y, &sector->floorplane);
}

fixed_t P_FloorHeight(const AActor *mo)
{
	if (mo && mo->subsector && mo->subsector->sector)
		return P_PlaneZ(mo->x, mo->y, &mo->subsector->sector->floorplane);
	else
		return MAXINT;
}

fixed_t P_FloorHeight(const sector_t *sector)
{
	if (!sector)
		return MAXINT;

	const plane_t *plane = &sector->floorplane;
	return P_PlaneZ(plane->texx, plane->texy, plane);
}

//
// P_CeilingHeight()
//
// Returns the height of a ceiling plane at the point (x, y).  The subsector
// parameter is optional but provides a speedup if used because P_PlaneZ
// can avoid a call to P_PointInSubsector().
//
// Note that there is no check made to ensure the point (x, y) is actually
// within the subsector.
//
// MAXINT is returned if the point is not within any sector.
//
fixed_t P_CeilingHeight(fixed_t x, fixed_t y, const sector_t *sector)
{
	if (!sector && !(sector = P_PointInSubsector(x, y)->sector))
		return MAXINT;

	return P_PlaneZ(x, y, &sector->ceilingplane);
}

fixed_t P_CeilingHeight(const AActor *mo)
{
	if (mo && mo->subsector && mo->subsector->sector)
		return P_PlaneZ(mo->x, mo->y, &mo->subsector->sector->ceilingplane);
	else
		return MAXINT;
}

fixed_t P_CeilingHeight(const sector_t *sector)
{
	if (!sector)
		return MAXINT;

	const plane_t *plane = &sector->ceilingplane;
	return P_PlaneZ(plane->texx, plane->texy, plane);
}

//
// P_LinePlaneIntersection
//
// Calculates the point of intersection of a plane and a line that begins
// at lineorg and is along the direction of linedir.
//
// If the line does not intersect the plane, it must be parallel to the surface
// of the plane.  In that case, a vector with the value MAXINT for all
// components is returned.
//
v3fixed_t P_LinePlaneIntersection(const plane_t *plane,
									const v3fixed_t &lineorg,
									const v3fixed_t &linedir)
{
	v3fixed_t pt;
	M_SetVec3Fixed(&pt, MAXINT, MAXINT, MAXINT);	// marks as invalid

	if (!plane)		// sanity check
		return pt;

	fixed_t numer = -plane->d -
					FixedMul(plane->a, lineorg.x) -
					FixedMul(plane->b, lineorg.y) -
					FixedMul(plane->c, lineorg.z);

	fixed_t denom = FixedMul(plane->a, linedir.x) +
					FixedMul(plane->b, linedir.y) +
					FixedMul(plane->c, linedir.z);

	if (!denom)	// line is parallel to the surface of plane
		return pt;

	fixed_t t = FixedDiv(numer, denom);
	M_SetVec3Fixed(&pt, lineorg.x + FixedMul(t, linedir.x),
						lineorg.y + FixedMul(t, linedir.y),
						lineorg.z + FixedMul(t, linedir.z));

	return pt;
}

bool P_PointOnPlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z)
{
	static const fixed_t threshold = FRACUNIT >> 6;

	if (!plane)
		return false;

	fixed_t result =
		(FixedMul(plane->a, x) + FixedMul(plane->b, y) + FixedMul(plane->c, z) + plane->d);

	return abs(result) < threshold;
}

bool P_PointAbovePlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z)
{
	if (!plane)
		return false;

	return (FixedMul(plane->a, x) + FixedMul(plane->b, y) + FixedMul(plane->c, z)) > 0;
}

bool P_PointBelowPlane(const plane_t *plane, fixed_t x, fixed_t y, fixed_t z)
{
	if (!plane)
		return false;

	return (FixedMul(plane->a, x) + FixedMul(plane->b, y) + FixedMul(plane->c, z)) < 0;
}

bool P_IdenticalPlanes(const plane_t *pl1, const plane_t *pl2)
{
	if (!pl1 || !pl2)
		return false;

	return (pl1 == pl2) ||
		(pl1->a == pl2->a && pl1->b == pl2->b && pl1->c == pl2->c && pl1->d == pl2->d);
}

void P_ChangeCeilingHeight(sector_t *sector, fixed_t amount)
{
	if (!sector)
		return;

	plane_t *plane = &sector->ceilingplane;
	plane->d -= FixedMul(amount, plane->c);

	// The sector's ceilingheight variable is still used for (among other things)
	// calculating wall texture offsets
	sector->ceilingheight += amount;
}

void P_ChangeFloorHeight(sector_t *sector, fixed_t amount)
{
	if (!sector)
		return;

	plane_t *plane = &sector->floorplane;
	plane->d -= FixedMul(amount, plane->c);

	// The sector's floorheight variable is still used for (among other things)
	// calculating wall texture offsets
	sector->floorheight += amount;
}

void P_SetCeilingHeight(sector_t *sector, fixed_t value)
{
	if (!sector)
		return;

	plane_t *plane = &sector->ceilingplane;
	fixed_t oldvalue = P_PlaneZ(plane->texx, plane->texy, plane);

	P_ChangeCeilingHeight(sector, value - oldvalue);
}

void P_SetFloorHeight(sector_t *sector, fixed_t value)
{
	if (!sector)
		return;

	plane_t *plane = &sector->floorplane;
	fixed_t oldvalue = P_PlaneZ(plane->texx, plane->texy, plane);

	P_ChangeFloorHeight(sector, value - oldvalue);
}

//
// P_LowestHeightOfCeiling
//
// Returns the lowest height of any point of the ceiling
//
fixed_t P_LowestHeightOfCeiling(sector_t *sector)
{
	fixed_t height = MAXINT;
	if (!sector)
		return height;

	for (int i = 0; i < sector->linecount; i++)
	{
		vertex_t *v1 = sector->lines[i]->v1;
		vertex_t *v2 = sector->lines[i]->v2;

		fixed_t thisheight;
		if ((thisheight = P_CeilingHeight(v1->x, v1->y, sector)) < height)
			height = thisheight;
		if ((thisheight = P_CeilingHeight(v2->x, v2->y, sector)) < height)
			height = thisheight;
	}

	return height;
}

//
// P_LowestHeightOfFloor
//
// Returns the lowest height of any point of the floor
//
fixed_t P_LowestHeightOfFloor(sector_t *sector)
{
	fixed_t height = MAXINT;
	if (!sector)
		return height;

	for (int i = 0; i < sector->linecount; i++)
	{
		vertex_t *v1 = sector->lines[i]->v1;
		vertex_t *v2 = sector->lines[i]->v2;

		fixed_t thisheight;
		if ((thisheight = P_FloorHeight(v1->x, v1->y, sector)) < height)
			height = thisheight;
		if ((thisheight = P_FloorHeight(v2->x, v2->y, sector)) < height)
			height = thisheight;
	}

	return height;
}

//
// P_HighestHeightOfCeiling
//
// Returns the highest height of any point of the ceiling
//
fixed_t P_HighestHeightOfCeiling(sector_t *sector)
{
	fixed_t height = MININT;
	if (!sector)
		return height;

	for (int i = 0; i < sector->linecount; i++)
	{
		vertex_t *v1 = sector->lines[i]->v1;
		vertex_t *v2 = sector->lines[i]->v2;

		fixed_t thisheight;
		if ((thisheight = P_CeilingHeight(v1->x, v1->y, sector)) > height)
			height = thisheight;
		if ((thisheight = P_CeilingHeight(v2->x, v2->y, sector)) > height)
			height = thisheight;
	}

	return height;
}

//
// P_HighestHeightOfFloor
//
// Returns the highest height of any point of the floor
//
fixed_t P_HighestHeightOfFloor(sector_t *sector)
{
	fixed_t height = MININT;
	if (!sector)
		return height;

	for (int i = 0; i < sector->linecount; i++)
	{
		vertex_t *v1 = sector->lines[i]->v1;
		vertex_t *v2 = sector->lines[i]->v2;

		fixed_t thisheight;
		if ((thisheight = P_FloorHeight(v1->x, v1->y, sector)) > height)
			height = thisheight;
		if ((thisheight = P_FloorHeight(v2->x, v2->y, sector)) > height)
			height = thisheight;
	}

	return height;
}

//
// P_CopySector
//
// This function allocates ceilingdata, floordata and lightingdata.
// Be sure to delete them once you're done with the copy.
//
void P_CopySector(sector_t *dest, sector_t *src)
{
	if (!dest || !src)
		return;

	dest->floorheight			= src->floorheight;
	dest->ceilingheight			= src->ceilingheight;
	dest->floorpic				= src->floorpic;
	dest->ceilingpic			= src->ceilingpic;
	dest->lightlevel			= src->lightlevel;
	dest->special				= src->special;
	dest->tag					= src->tag;
	dest->nexttag				= src->nexttag;
	dest->firsttag				= src->firsttag;
	dest->soundtraversed		= src->soundtraversed;
	dest->validcount			= src->validcount;
	dest->seqType				= src->seqType;
	dest->sky					= src->sky;
	dest->friction				= src->friction;
	dest->movefactor			= src->movefactor;
	dest->moveable				= src->moveable;
	dest->stairlock				= src->stairlock;
	dest->prevsec				= src->prevsec;
	dest->floor_xoffs			= src->floor_xoffs;
	dest->floor_yoffs			= src->floor_yoffs;
	dest->ceiling_xoffs			= src->ceiling_xoffs;
	dest->ceiling_yoffs			= src->ceiling_yoffs;
	dest->floor_xscale			= src->floor_xscale;
	dest->floor_yscale			= src->floor_yscale;
	dest->ceiling_xscale		= src->ceiling_xscale;
	dest->ceiling_yscale		= src->ceiling_yscale;
	dest->floor_angle			= src->floor_angle;
	dest->ceiling_angle			= src->ceiling_angle;
	dest->base_floor_angle		= src->base_floor_angle;
	dest->base_ceiling_angle	= src->base_ceiling_angle;
	dest->base_floor_yoffs		= src->base_floor_yoffs;
	dest->base_ceiling_yoffs	= src->base_ceiling_yoffs;
	dest->bottommap				= src->bottommap;
	dest->midmap				= src->midmap;
	dest->topmap				= src->topmap;
	dest->gravity				= src->gravity;
	dest->damage				= src->damage;
	dest->mod					= src->mod;
	dest->colormap				= src->colormap;
	dest->alwaysfake			= src->alwaysfake;
	dest->waterzone				= src->waterzone;
	dest->MoreFlags				= src->MoreFlags;

	dest->heightsec				= src->heightsec;
	dest->floorlightsec			= src->floorlightsec;
	dest->ceilinglightsec		= src->ceilinglightsec;

	dest->linecount				= src->linecount;
	dest->lines					= src->lines;

	memcpy(dest->blockbox, src->blockbox, sizeof(src->blockbox));
	memcpy(dest->soundorg, src->soundorg, sizeof(src->soundorg));

	dest->floorplane			= src->floorplane;
	dest->ceilingplane			= src->ceilingplane;

	dest->touching_thinglist	= src->touching_thinglist;
	dest->thinglist				= src->thinglist;
	dest->soundtarget			= src->soundtarget;

	if (src->ceilingdata != NULL)
		dest->ceilingdata = src->ceilingdata->Clone(dest);
	else
		dest->ceilingdata = NULL;
	
	if (src->floordata != NULL)
		dest->floordata = src->floordata->Clone(dest);
	else
		dest->floordata = NULL;
	
	if (src->lightingdata != NULL)
		dest->lightingdata = src->lightingdata->Clone(dest);
	else
		dest->lightingdata = NULL;
}


VERSION_CONTROL (p_map_cpp, "$Id$")

