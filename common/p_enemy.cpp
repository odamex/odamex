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
//	Enemy thinking, AI.
//	Action Pointer Functions
//	that are associated with states/frames.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <math.h>
#include "m_random.h"
#include "m_alloc.h"
#include "i_system.h"
#include "p_local.h"
#include "p_lnspec.h"
#include "s_sound.h"
#include "r_state.h"
#include "gi.h"
#include "p_mobj.h"

#include "d_player.h"
#include "p_setup.h"
#include "d_dehacked.h"
#include "g_skill.h"
#include "p_mapformat.h"


EXTERN_CVAR(sv_allowexit)
EXTERN_CVAR (sv_fastmonsters)
EXTERN_CVAR (co_zdoomphys)
EXTERN_CVAR (co_novileghosts)
EXTERN_CVAR(co_zdoomsound)
EXTERN_CVAR(co_removesoullimit)

enum dirtype_t
{
	DI_EAST,
	DI_NORTHEAST,
	DI_NORTH,
	DI_NORTHWEST,
	DI_WEST,
	DI_SOUTHWEST,
	DI_SOUTH,
	DI_SOUTHEAST,
	DI_NODIR,
	NUMDIRS
};

//
// P_NewChaseDir related LUT.
//

dirtype_t opposite[9] =
{
	DI_WEST, DI_SOUTHWEST, DI_SOUTH, DI_SOUTHEAST,
	DI_EAST, DI_NORTHEAST, DI_NORTH, DI_NORTHWEST, DI_NODIR
};

dirtype_t diags[4] =
{
	DI_NORTHWEST, DI_NORTHEAST, DI_SOUTHWEST, DI_SOUTHEAST
};

fixed_t xspeed[8] = {FRACUNIT,47000,0,-47000,-FRACUNIT,-47000,0,47000};
fixed_t yspeed[8] = {0,47000,FRACUNIT,47000,0,-47000,-FRACUNIT,-47000};


void A_Die (AActor *actor);
void A_Detonate (AActor *mo);
void A_Explode (AActor *thing);
void A_Mushroom (AActor *actor);
void A_Fall (AActor *actor);


void SV_UpdateMonsterRespawnCount();
void SV_UpdateMobj(AActor* mo);
void SV_Sound(AActor* mo, byte channel, const char* name, byte attenuation);

// killough 8/8/98: distance friends tend to move towards players
const int distfriend = 128;

// killough 9/8/98: whether monsters are allowed to strafe or retreat
const int monster_backing = 0;

extern bool isFast;

//
// ENEMY THINKING
// Enemies are always spawned
// with targetplayer = -1, threshold = 0
// Most monsters are spawned unaware of all players,
// but some can be made preaware
//


//
// Called by P_NoiseAlert.
// Recursively traverse adjacent sectors,
// sound blocking lines cut off traversal.
//

void P_RecursiveSound (sector_t *sec, int soundblocks, AActor *soundtarget)
{
	int 		i;
	line_t* 	check;
	sector_t*	other;

	// wake up all monsters in this sector
	if (sec->validcount == validcount
		&& sec->soundtraversed <= soundblocks+1)
	{
		return; 		// already flooded
	}

	sec->validcount = validcount;
	sec->soundtraversed = soundblocks+1;
	sec->soundtarget = soundtarget->ptr();

	for (i=0 ;i<sec->linecount ; i++)
	{
		check = sec->lines[i];
		if (! (check->flags & ML_TWOSIDED) )
			continue;

		if ( sides[ check->sidenum[0] ].sector == sec)
			other = sides[ check->sidenum[1] ] .sector;
		else
			other = sides[ check->sidenum[0] ].sector;

		// [SL] 2012-02-08 - FIXME: Currently only checks for a line opening at
		// midpoint of a sloped linedef.  P_RecursiveSound() in ZDoom 1.23 causes
		// demo desyncs.
		P_LineOpening(check, (check->v1->x >> 1) + (check->v2->x >> 1),
							 (check->v1->y >> 1) + (check->v2->y >> 1));

		if (openrange <= 0)
			continue;	// closed door

		if (check->flags & ML_SOUNDBLOCK)
		{
			if (!soundblocks)
				P_RecursiveSound (other, 1, soundtarget);
		}
		else
			P_RecursiveSound (other, soundblocks, soundtarget);
	}
}



//
// P_NoiseAlert
// If a monster yells at a player,
// it will alert other monsters to the player.
//
void P_NoiseAlert (AActor *target, AActor *emmiter)
{
	if (target->player && (!multiplayer && (target->player->cheats & CF_NOTARGET)))
		return;

	validcount++;
	P_RecursiveSound (emmiter->subsector->sector, 0, target);
}


//
// P_CheckRange
//

static bool P_CheckRange(AActor* actor, fixed_t range)
{
	AActor* pl = actor->target;

	return // killough 7/18/98: friendly monsters don't attack other friends
	    pl && !(actor->flags & pl->flags & MF_FRIEND) &&
	    P_AproxDistance(pl->x - actor->x, pl->y - actor->y) < range &&
	    P_CheckSight(actor, actor->target) &&
	        pl->z <= actor->z + actor->height && actor->z <= pl->z + pl->height;
}


//
// P_CheckMeleeRange
//
BOOL P_CheckMeleeRange (AActor *actor)
{
	AActor *pl;
	fixed_t dist;

	if (!actor->target)
		return false;

	fixed_t range = actor->info->meleerange;

	pl = actor->target;
	dist = P_AproxDistance (pl->x-actor->x, pl->y-actor->y);

	if (dist >= range - 20 * FRACUNIT + pl->info->radius)
		return false;

	// [RH] If moving toward goal, then we've reached it.
	if (actor->target == actor->goal)
		return true;

	// [RH] Don't melee things too far above or below actor.
	if (P_AllowPassover())
	{
		if (pl->z > actor->z + actor->height)
			return false;
		if (pl->z + pl->height < actor->z)
			return false;
	}

	if (!P_CheckSight(actor, pl))
		return false;

	return true;
}

//
// P_CheckMissileRange
//
BOOL P_CheckMissileRange (AActor *actor)
{
	fixed_t dist;

	if (!P_CheckSight (actor, actor->target))
		return false;

	if (actor->flags & MF_JUSTHIT)
	{
		// the target just hit the enemy,
		// so fight back!
		actor->flags &= ~MF_JUSTHIT;
		return true;
	}

	if (actor->reactiontime)
		return false;	// do not attack yet

	// OPTIMIZE: get this from a global checksight
	dist = P_AproxDistance ( actor->x-actor->target->x,
							 actor->y-actor->target->y) - 64*FRACUNIT;

	if (!actor->info->meleestate)
		dist -= 128*FRACUNIT;	// no melee attack, so fire more

	dist >>= 16;

	if (actor->flags3 & MF3_SHORTMRANGE)
	{
		if (dist > 14*64)
			return false;		// too far away
	}


	if (actor->flags3 & MF3_LONGMELEE)
	{
		if (dist < 196)
			return false;		// close for fist attack
		dist >>= 1;
	}


	if (actor->flags3 & MF3_RANGEHALF)
		dist >>= 1;

	if (dist > 200)
		dist = 200;

	if (actor->flags3 & MF3_HIGHERMPROB && dist > 160)
		dist = 160;

	if (P_Random (actor) < dist)
		return false;

	return true;
}


//
// P_Move
// Move in the current direction,
// returns false if the move is blocked.
//
extern	std::vector<line_t*> spechit;

BOOL P_Move (AActor *actor)
{
	fixed_t tryx, tryy, deltax, deltay, origx, origy;
	BOOL try_ok;
	int good;
	int speed;
	int movefactor = ORIG_FRICTION_FACTOR;
	int friction = ORIG_FRICTION;

	if (!actor->subsector)
		return false;

	if (actor->flags2 & MF2_BLASTED)
		return true;

	if (actor->movedir == DI_NODIR)
		return false;

	// [RH] Instead of yanking non-floating monsters to the ground,
	// let gravity drop them down, unless they're moving down a step.
	if (co_zdoomphys && !(actor->flags & MF_NOGRAVITY) && actor->z > actor->floorz
		&& !(actor->flags2 & MF2_ONMOBJ))
	{
		if (actor->z > actor->floorz + 24*FRACUNIT)
		{
			return false;
		}
		else
		{
			actor->z = actor->floorz;
		}
	}

	if ((unsigned)actor->movedir >= 8)
		I_Error ("Weird actor->movedir!");

	speed = actor->info->speed;

	// [AM] Quick monsters are faster.
	if (actor->oflags & MFO_QUICK)
	{
		speed = speed + (speed / 2);
	}

#if 0	// [RH] I'm not so sure this is such a good idea
	// killough 10/98: make monsters get affected by ice and sludge too:
	movefactor = P_GetMoveFactor (actor, &friction);

	if (friction < ORIG_FRICTION &&		// sludge
		!(speed = ((ORIG_FRICTION_FACTOR - (ORIG_FRICTION_FACTOR-movefactor)/2)
		   * speed) / ORIG_FRICTION_FACTOR))
		speed = 1;	// always give the monster a little bit of speed
#endif

	tryx = (origx = actor->x) + (deltax = speed * xspeed[actor->movedir]);
	tryy = (origy = actor->y) + (deltay = speed * yspeed[actor->movedir]);

	// killough 3/15/98: don't jump over dropoffs:
	try_ok = P_TryMove (actor, tryx, tryy, false);

	if (try_ok && friction > ORIG_FRICTION)
	{
		actor->x = origx;
		actor->y = origy;
		movefactor *= FRACUNIT / ORIG_FRICTION_FACTOR / 4;
		actor->momx += FixedMul (deltax, movefactor);
		actor->momy += FixedMul (deltay, movefactor);
	}

	if (!try_ok)
	{
		// open any specials
		if (actor->flags & MF_FLOAT && floatok)
		{
			// must adjust height
			if (actor->z < tmfloorz)
				actor->z += FLOATSPEED;
			else
				actor->z -= FLOATSPEED;

			actor->flags |= MF_INFLOAT;
			return true;
		}

		if (spechit.empty())
			return false;

		actor->movedir = DI_NODIR;
		good = false;
		while (!spechit.empty())
		{
			line_t *ld = spechit.back();
			spechit.pop_back();

			// if the special is not a door
			// that can be opened,
			// return false
			if (P_UseSpecialLine (actor, ld, 0, false) ||
				P_PushSpecialLine (actor, ld, 0))
				good = true;
		}
		return good;
	}
	else
	{
		actor->flags &= ~MF_INFLOAT;
	}

	if (!co_zdoomphys && !(actor->flags & MF_FLOAT))
		actor->z = actor->floorz;

	return true;
}


//
// TryWalk
// Attempts to move actor on
// in its current (ob->moveangle) direction.
// If blocked by either a wall or an actor
// returns FALSE
// If move is either clear or blocked only by a door,
// returns TRUE and sets...
// If a door is in the way,
// an OpenDoor call is made to start it opening.
//
BOOL P_TryWalk (AActor *actor)
{
	if (!P_Move (actor))
	{
		return false;
	}

	actor->movecount = P_Random (actor) & 15;
	return true;
}

/*
 * P_IsOnLift
 *
 * killough 9/9/98:
 *
 * Returns true if the object is on a lift. Used for AI,
 * since it may indicate the need for crowded conditions,
 * or that a monster should stay on the lift for a while
 * while it goes up or down.
 */

bool P_IsOnLift(const AActor* actor)
{
	const sector_t* sec = actor->subsector->sector;
	line_t line;
	int l;

	// Short-circuit: it's on a lift which is active.
	if (sec->floordata && ((DFloor*)sec->floordata)->m_Status == DFloor::up)
		return true;

	// Check to see if it's in a sector which can be activated as a lift.
	if ((line.id = sec->tag))
		for (l = -1; (l = P_FindLineFromLineTag(&line, l)) >= 0;)
			switch (lines[l].special)
			{
			case 10:
			case 14:
			case 15:
			case 20:
			case 21:
			case 22:
			case 47:
			case 53:
			case 62:
			case 66:
			case 67:
			case 68:
			case 87:
			case 88:
			case 95:
			case 120:
			case 121:
			case 122:
			case 123:
			case 143:
			case 162:
			case 163:
			case 181:
			case 182:
			case 144:
			case 148:
			case 149:
			case 211:
			case 227:
			case 228:
			case 231:
			case 232:
			case 235:
			case 236:
				return true;
			}

	return false;
}

void P_NewChaseDir (AActor *actor)
{
	fixed_t 	deltax;
	fixed_t 	deltay;

	dirtype_t	d[3];

	int			tdir;
	dirtype_t	olddir;

	dirtype_t	turnaround;

	if (!actor->target)
		I_Error ("P_NewChaseDir: called with no target");

	olddir = (dirtype_t)actor->movedir;
	turnaround = opposite[olddir];

	deltax = actor->target->x - actor->x;
	deltay = actor->target->y - actor->y;

	if (monster_backing)
	{
		fixed_t dist = P_AproxDistance(deltax, deltay);
		if (actor->flags & actor->target->flags & MF_FRIEND &&
		    distfriend << FRACBITS > dist && !P_IsOnLift(actor) &&
		    !P_IsUnderDamage(actor))
		{
			deltax = -deltax, deltay = -deltay;
		}
		else if (actor->target->health > 0 &&
		         (actor->flags ^ actor->target->flags) & MF_FRIEND)
		{ // Live enemy target
			if (actor->info->missilestate && actor->type != MT_SKULL &&
			    ((!actor->target->info->missilestate &&
			      dist < actor->target->info->meleerange * 2) ||
			     (actor->target->player &&
			      dist < actor->target->player->mo->info->meleerange * 3 &&
			      weaponinfo[actor->target->player->readyweapon].flags & WPF_FLEEMELEE)))
			{ // Back away from melee attacker
				deltax = -deltax, deltay = -deltay;
			}
		}
	}

	if (deltax>10*FRACUNIT)
		d[1]= DI_EAST;
	else if (deltax<-10*FRACUNIT)
		d[1]= DI_WEST;
	else
		d[1]=DI_NODIR;

	if (deltay<-10*FRACUNIT)
		d[2]= DI_SOUTH;
	else if (deltay>10*FRACUNIT)
		d[2] = DI_NORTH;
	else
		d[2] = DI_NODIR;

	// try direct route
	if (d[1] != DI_NODIR && d[2] != DI_NODIR)
	{
		actor->movedir = diags[((deltay<0)<<1) + (deltax>0)];
		if (actor->movedir != turnaround && P_TryWalk(actor))
			return;
	}

	// try other directions
	if (P_Random (actor) > 200 || abs(deltay) > abs(deltax))
	{
		tdir = d[1];
		d[1] = d[2];
		d[2] = (dirtype_t)tdir;
	}

	if (d[1] == turnaround)
		d[1] = DI_NODIR;
	if (d[2] == turnaround)
		d[2] = DI_NODIR;

	if (d[1] != DI_NODIR)
	{
		actor->movedir = d[1];
		if (P_TryWalk (actor))
		{
			// either moved forward or attacked
			return;
		}
	}

	if (d[2] != DI_NODIR)
	{
		actor->movedir = d[2];

		if (P_TryWalk (actor))
			return;
	}

	// there is no direct path to the player,
	// so pick another direction.
	if (olddir != DI_NODIR)
	{
		actor->movedir = olddir;

		if (P_TryWalk (actor))
			return;
	}

	// randomly determine direction of search
	if (P_Random (actor) & 1)
	{
		for (tdir = DI_EAST; tdir <= DI_SOUTHEAST; tdir++)
		{
			if (tdir != turnaround)
			{
				actor->movedir = tdir;

				if ( P_TryWalk(actor) )
					return;
			}
		}
	}
	else
	{
		for (tdir = DI_SOUTHEAST; tdir != (DI_EAST-1); tdir--)
		{
			if (tdir != turnaround)
			{
				actor->movedir = tdir;

				if ( P_TryWalk(actor) )
					return;
			}
		}
	}

	if (turnaround != DI_NODIR)
	{
		actor->movedir =turnaround;
		if ( P_TryWalk(actor) )
			return;
	}

	actor->movedir = DI_NODIR;	// can not move
}



//
// P_LookForPlayers
// If allaround is false, only look 180 degrees in front.
// Returns true if a player is targeted.
//
// This function can trip you up if you're not careful.  The piece of
// functionality that is critical to vanilla compatibility is actor->lastlook.
// In vanilla, it was randomly set to a number between 0 and 3 on mobj
// creation, and this function is SUPPOSED to loop through all player numbers,
// set the mobj target and leave lastlook at the last-looked at player, in
// the hopes that not all monsters will immediately target the same player.
//
// However, the looping logic is _very_ tricky to get your head around, as
// the function was written with a fixed array of players in mind and makes
// some bad assumptions.  The ending sentinal (stop) can refer to a player
// index that doesn't exist.  The hard-limit counter (c) is post-incremented,
// so the loop will do at most two sight-checks, but lastlook is actually
// incremented one more time than that.
//
bool P_LookForPlayers(AActor *actor, bool allaround)
{
	// [AM] Check subsectors first.
	if (actor->subsector == NULL)
		return false;

	sector_t* sector = actor->subsector->sector;
	if (!sector)
		return false;

	// Construct our table of ingame players
	// [AM] TODO: Have the Players container handle this instead of having to
	//            check every single tic.
	static player_t* playeringame[MAXPLAYERS];
	memset(playeringame, 0, sizeof(player_t*) * MAXPLAYERS);

	short maxid = 0;
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (it->ingame() && !(it->spectator))
		{
			playeringame[(it->id) - 1] = &*it;
			maxid = it->id;
		}
	}

	// If there are no ingame players, we need to bug out now because
	// otherwise we're going to cause an infinite loop.
	if (maxid == 0)
		return false;

	// denis - vanilla sync, original code always looped over size-4 array.
	if (maxid < MAXPLAYERS_VANILLA)
		maxid = MAXPLAYERS_VANILLA;

	// denis - prevents calling P_CheckSight twice on the same player
	static bool sightcheckfailed[MAXPLAYERS];
	memset(sightcheckfailed, 0, sizeof(bool) * maxid);

	int counter = 0;

	// [AM] Vanilla braindamage, "fixing" will lead to vanilla desyncs
	unsigned int stop;
	if (actor->lastlook > 0)
		stop = actor->lastlook - 1;
	else
		stop = maxid - 1;

	for ( ; ; actor->lastlook = (actor->lastlook + 1) % maxid)
	{
		if (playeringame[actor->lastlook] == NULL)
			continue;

		if (++counter == 3 || actor->lastlook == stop)
		{
			// done looking
			// [RH] Use goal as a last resort
			if (!actor->target && actor->goal)
			{
				actor->target = actor->goal;
				return true;
			}
			return false;
		}

		if (sightcheckfailed[actor->lastlook])
			continue;

		player_t* player = playeringame[actor->lastlook];

		if (player->cheats & CF_NOTARGET)
			continue; // no target

		if (player->health <= 0)
			continue; // dead

		if (!player->mo)
			continue; // out of game

		if (!P_CheckSight(actor, player->mo))
		{
			sightcheckfailed[actor->lastlook] = true;
			continue; // out of sight
		}

		if (!allaround)
		{
			angle_t an = P_PointToAngle(actor->x, actor->y,
			                            player->mo->x, player->mo->y) - actor->angle;
			if (an > ANG90 && an < ANG270)
			{
				fixed_t dist = P_AproxDistance(player->mo->x - actor->x,
				                               player->mo->y - actor->y);
				// if real close, react anyway
				if (dist > MELEERANGE)
					continue; // behind back
			}
		}

		// [RH] Need to be sure the reactiontime is 0 if the monster is
		//		leaving its goal to go after a player.
		if (actor->goal && actor->target == actor->goal)
			actor->reactiontime = 0;

		actor->target = player->mo->ptr();
		return true;
	}

	return false;
}


//
// A_KeenDie
// DOOM II special, map 32.
// Uses special tag 666.
//
void A_KeenDie (AActor *actor)
{
	A_Fall (actor);

	// scan the remaining thinkers
	// to see if all Keens are dead
	AActor *other;
	TThinkerIterator<AActor> iterator;

	while ( (other = iterator.Next ()) )
	{
		if (other != actor && other->type == actor->type && other->health > 0)
		{
			// other Keen not dead
			return;
		}
	}

	EV_DoDoor (DDoor::doorOpen, NULL, NULL, 666, 2*FRACUNIT, 0, NoKey);
}


//
// ACTION ROUTINES
//

//
// A_Look
// Stay in state until a player is sighted.
// [RH] Will also leave state to move to goal.
//
void A_Look (AActor *actor)
{
	AActor *targ;
	AActor *newgoal;

	if(!actor->subsector)
		return;

	// [RH] Set goal now if appropriate
	if (actor->special == Thing_SetGoal && actor->args[0] == 0)
	{
		actor->special = 0;
		newgoal = AActor::FindGoal (NULL, actor->args[1], MT_PATHNODE);
		actor->goal = newgoal->ptr();
		actor->reactiontime = actor->args[2] * TICRATE + level.time;
	}

	actor->threshold = 0;		// any shot will wake up
	targ = actor->subsector->sector->soundtarget;

	if (targ && targ->player && (targ->player->cheats & CF_NOTARGET))
		return;

	// GhostlyDeath -- can't hear spectators
	if (targ && targ->player && targ->player->spectator)
	{
		actor->target = AActor::AActorPtr();
		//return;
	}

	if (targ && (targ->flags & MF_SHOOTABLE))
	{
		actor->target = targ->ptr();

		if (actor->flags & MF_AMBUSH)
		{
			if (P_CheckSight(actor, actor->target))
				goto seeyou;
		}
		else
			goto seeyou;
	}


	if (!P_LookForPlayers (actor, false))
		return;

	// go into chase state
  seeyou:

  	// GhostlyDeath -- Can't see spectators
  	if (actor->target->player && actor->target->player->spectator)
	{
		actor->target = AActor::AActorPtr();
  		//return;
	}
	// [RH] Don't start chasing after a goal if it isn't time yet.
	if (actor->target == actor->goal)
	{
		if (actor->reactiontime > level.time)
			actor->target = AActor::AActorPtr();
	}
	else if (actor->info->seesound)
	{
		char sound[MAX_SNDNAME];

		strcpy (sound, actor->info->seesound);

		if (sound[strlen(sound)-1] == '1')
		{
			sound[strlen(sound)-1] = P_Random(actor)%3 + '1';
			if (S_FindSound (sound) == -1)
				sound[strlen(sound)-1] = '1';
		}

		if (!co_zdoomsound && (actor->flags2 & MF2_BOSS || actor->flags3 & MF3_FULLVOLSOUNDS))
			S_Sound(CHAN_VOICE, sound, 1, ATTN_NORM);
		else
			S_Sound (actor, CHAN_VOICE, sound, 1, ATTN_NORM);
	}

	if (actor->target)
		P_SetMobjState (actor, actor->info->seestate, true);
}
#include "m_vectors.h"

//
// A_Chase
// Actor has a melee attack,
// so it tries to close as fast as possible
//
void A_Chase (AActor *actor)
{
	int delta;
	AActor *ngoal;

	// GhostlyDeath -- Don't chase spectators at all
	if (actor->target && actor->target->player && actor->target->player->spectator)
	{
		actor->target = AActor::AActorPtr();
		//return;
	}

	if (actor->reactiontime)
		actor->reactiontime--;

	// modify target threshold
	if (actor->threshold)
	{
		if (!actor->target || actor->target->health <= 0)
		{
			actor->threshold = 0;
		}
		else
			actor->threshold--;
	}

	// turn towards movement direction if not there yet
	if (actor->movedir < 8)
	{
		actor->angle &= (angle_t)(7<<29);
		delta = actor->angle - (actor->movedir << 29);

		if (delta > 0)
			actor->angle -= ANG90/2;
		else if (delta < 0)
			actor->angle += ANG90/2;
	}

	// [RH] If the target is dead (and not a goal), stop chasing it.
	if (actor->target && actor->target != actor->goal && actor->target->health <= 0)
		actor->target = AActor::AActorPtr();

	if (!actor->target || !(actor->target->flags & MF_SHOOTABLE))
	{
		// look for a new target
		if (P_LookForPlayers (actor, true) && actor->target != actor->goal)
			return; 	// got a new target

		if (!actor->target)
		{
			P_SetMobjState (actor, actor->info->spawnstate, true); // denis - todo - this sometimes leads to a stack overflow due to infinite recursion: A_Chase->SetMobjState->A_Look->SetMobjState
			return;
		}
	}

	// do not attack twice in a row
	if (actor->flags & MF_JUSTATTACKED)
	{
		actor->flags &= ~MF_JUSTATTACKED;
		if (G_GetCurrentSkill().respawn_counter == 0 && !sv_fastmonsters)
			P_NewChaseDir (actor);
		return;
	}

	// [RH] Don't attack if just moving toward goal
	if (actor->target == actor->goal)
	{
		if (P_CheckMeleeRange (actor))
		{
			// reached the goal
			actor->reactiontime = actor->goal->args[1] * TICRATE + level.time;
			ngoal = AActor::FindGoal (NULL, actor->goal->args[0], MT_PATHNODE);
			if (ngoal)
				actor->goal = ngoal->ptr();
			else
				actor->goal = AActor::AActorPtr();

			actor->target = AActor::AActorPtr();
			P_SetMobjState (actor, actor->info->spawnstate, true);
			return;
		}
		goto nomissile;
	}

	// check for melee attack
	if (actor->info->meleestate && P_CheckMeleeRange (actor))
	{
		if (actor->info->attacksound)
			S_Sound (actor, CHAN_WEAPON, actor->info->attacksound, 1, ATTN_NORM);

		if (serverside)
			P_SetMobjState (actor, actor->info->meleestate, true);
		return;
	}

	// check for missile attack
	if (actor->info->missilestate)
	{
		if (!G_GetCurrentSkill().fast_monsters && actor->movecount && !sv_fastmonsters)
		{
			goto nomissile;
		}

		if (!P_CheckMissileRange (actor))
			goto nomissile;

		if (serverside)
			P_SetMobjState (actor, actor->info->missilestate, true);
		actor->flags |= MF_JUSTATTACKED;
		return;
	}

	// ?
  nomissile:
	// possibly choose another target
	if (multiplayer
		&& !actor->threshold
		&& !P_CheckSight(actor, actor->target))
	{
		if (P_LookForPlayers(actor,true))
			return; 	// got a new target
	}

	// chase towards player
	if (--actor->movecount < 0 || !P_Move (actor))
	{
		P_NewChaseDir (actor);
	}

	// make active sound
	if (actor->info->activesound && P_Random (actor) < 3)
	{
		S_Sound (actor, CHAN_VOICE, actor->info->activesound, 1, ATTN_IDLE);
	}
}


//
// A_FaceTarget
//
void A_FaceTarget (AActor *actor)
{
	if (!actor->target)
		return;

	actor->flags &= ~MF_AMBUSH;

	actor->angle = P_PointToAngle (actor->x,
									actor->y,
									actor->target->x,
									actor->target->y);

	if (actor->target->flags & MF_SHADOW)
		actor->angle += P_RandomDiff(actor)<<21;
}

//
// [RH] A_MonsterRail
//
// New function to let monsters shoot a railgun
//
void A_MonsterRail (AActor *actor)
{
	if (!actor->target)
		return;

	actor->flags &= ~MF_AMBUSH;

	actor->angle = R_PointToAngle2 (actor->x,
									actor->y,
									actor->target->x,
									actor->target->y);

	//actor->pitch = tantoangle[P_AimLineAttack (actor, actor->angle, MISSILERANGE) >> DBITS];
	actor->pitch = -(int)(tan ((float)P_AimLineAttack (actor, actor->angle, MISSILERANGE)/65536.0f)*ANG180/PI);

	// Let the aim trail behind the player
	actor->angle = R_PointToAngle2 (actor->x,
									actor->y,
									actor->target->x - actor->target->momx * 3,
									actor->target->y - actor->target->momy * 3);

	if (actor->target->flags & MF_SHADOW)
    {
		int t = P_Random(actor);
		actor->angle += (t-P_Random(actor))<<21;
    }

	P_RailAttack (actor, actor->info->damage, 0);
}

//
//
// A_PosAttack
//
void A_PosAttack (AActor *actor)
{
	int angle;
	int damage;
	int slope;

	if (!actor->target)
		return;

	A_FaceTarget (actor);
	angle = actor->angle;
	slope = P_AimLineAttack (actor, angle, MISSILERANGE);

	S_Sound (actor, CHAN_WEAPON, "grunt/attack", 1, ATTN_NORM);
	angle += P_RandomDiff (actor)<<20;
	damage = ((P_Random (actor)%5)+1)*3;
	P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_SPosAttack (AActor *actor)
{
	int i;
	int bangle;
	int slope;

	if (!actor->target)
		return;

	S_Sound (actor, CHAN_WEAPON, "shotguy/attack", 1, ATTN_NORM);
	A_FaceTarget (actor);
	bangle = actor->angle;
	slope = P_AimLineAttack (actor, bangle, MISSILERANGE);

	for (i=0 ; i<3 ; i++)
    {
		int angle = bangle + (P_RandomDiff (actor)<<20);
		int damage = ((P_Random (actor)%5)+1)*3;
		P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
    }
}

void A_CPosAttack (AActor *actor)
{
	int angle;
	int bangle;
	int damage;
	int slope;

	if (!actor->target)
		return;

	S_Sound (actor, CHAN_WEAPON, "chainguy/attack", 1, ATTN_NORM);
	A_FaceTarget (actor);
	bangle = actor->angle;
	slope = P_AimLineAttack (actor, bangle, MISSILERANGE);

	angle = bangle + (P_RandomDiff (actor)<<20);
	damage = ((P_Random (actor)%5)+1)*3;
	P_LineAttack (actor, angle, MISSILERANGE, slope, damage);
}

void A_CPosRefire (AActor *actor)
{
	// keep firing unless target got out of sight
	A_FaceTarget (actor);

	if (P_Random (actor) < 40)
		return;

	if (!actor->target
		|| actor->target->health <= 0
		|| !P_CheckSight(actor, actor->target)
        )
	{
		P_SetMobjState (actor, actor->info->seestate, true);
	}
}


void A_SpidRefire (AActor *actor)
{
	// keep firing unless target got out of sight
	A_FaceTarget (actor);

	if (P_Random (actor) < 10)
		return;

	if (!actor->target
		|| actor->target->health <= 0
		|| !P_CheckSight(actor, actor->target)
        )
	{
		P_SetMobjState (actor, actor->info->seestate, true);
	}
}

void A_BspiAttack (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);

	// launch a missile
	if(serverside)
		P_SpawnMissile (actor, actor->target, MT_ARACHPLAZ);
}


//
// A_TroopAttack
//
void A_TroopAttack (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);
	if (P_CheckMeleeRange (actor))
	{
		S_Sound (actor, CHAN_WEAPON, "imp/melee", 1, ATTN_NORM);
		int damage = (P_Random (actor)%8+1)*3;
		P_DamageMobj (actor->target, actor, actor, damage, MOD_HIT);
		return;
	}

	// launch a missile
	if(serverside)
		P_SpawnMissile (actor, actor->target, MT_TROOPSHOT);
}


void A_SargAttack (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);
	if (P_CheckMeleeRange (actor))
	{
		int damage = ((P_Random (actor)%10)+1)*4;
		P_DamageMobj (actor->target, actor, actor, damage, MOD_HIT);
	}
}

void A_HeadAttack (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);
	if (P_CheckMeleeRange (actor))
	{
		int damage = (P_Random (actor)%6+1)*10;
		P_DamageMobj (actor->target, actor, actor, damage, MOD_HIT);
		return;
	}

	// launch a missile
	if(serverside)
		P_SpawnMissile (actor, actor->target, MT_HEADSHOT);
}

void A_CyberAttack (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);

	if(serverside)
	{
		P_SpawnMissile (actor, actor->target, MT_ROCKET);
	}
}


void A_BruisAttack (AActor *actor)
{
	if (!actor->target)
		return;

	if (P_CheckMeleeRange (actor))
	{
		int damage = (P_Random (actor)%8+1)*10;
		S_Sound (actor, CHAN_WEAPON, "baron/melee", 1, ATTN_NORM);
		P_DamageMobj (actor->target, actor, actor, damage, MOD_HIT);
		return;
	}

	// launch a missile
	if(serverside)
		P_SpawnMissile (actor, actor->target, MT_BRUISERSHOT);
}


//
// A_SkelMissile
//
void A_SkelMissile (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);

	if(serverside)
	{
		actor->z += 16*FRACUNIT;	// so missile spawns higher
		AActor *mo = P_SpawnMissile (actor, actor->target, MT_TRACER);
		actor->z -= 16*FRACUNIT;	// back to normal

		mo->x += mo->momx;
		mo->y += mo->momy;
		mo->tracer = actor->target;
	}
}

#define TRACEANGLE (0xc000000)

void A_Tracer (AActor *actor)
{
	// killough 1/18/98: this is why some missiles do not have smoke
	// and some do. Also, internal demos start at random gametics, thus
	// the bug in which revenants cause internal demos to go out of sync.
	//
	// killough 3/6/98: fix revenant internal demo bug by subtracting
	// levelstarttic from gametic:
	//
	// [RH] level.time is always 0-based, so nothing special to do here.

	// denis - demogametic must be 0-based, but from start of entire demo,
	// not just this level!
	extern int demostartgametic;
	int demogametic = gametic - demostartgametic;
	if (demogametic & 3)
		return;

	// spawn a puff of smoke behind the rocket
	if(serverside)
	{
		P_SpawnTracerPuff(actor->x, actor->y, actor->z);

		AActor* th = new AActor (actor->x - actor->momx,
						 actor->y - actor->momy,
						 actor->z, MT_SMOKE);

		th->momz = FRACUNIT;
		th->tics -= P_Random (th)&3;
		if (th->tics < 1)
			th->tics = 1;
	}

	// adjust direction
	AActor *dest = actor->tracer;

	if (!dest || dest->health <= 0)
		return;

	// change angle
	angle_t exact = P_PointToAngle (actor->x,
							 actor->y,
							 dest->x,
							 dest->y);

	if (exact != actor->angle)
	{
		if (exact - actor->angle > ANG180)
		{
			actor->angle -= TRACEANGLE;
			if (exact - actor->angle < ANG180)
				actor->angle = exact;
		}
		else
		{
			actor->angle += TRACEANGLE;
			if (exact - actor->angle > ANG180)
				actor->angle = exact;
		}
	}

	exact = actor->angle>>ANGLETOFINESHIFT;

	actor->momx = FixedMul(actor->info->speed, finecosine[exact]);
	actor->momy = FixedMul(actor->info->speed, finesine[exact]);

	// change slope
	fixed_t dist = P_AproxDistance (dest->x - actor->x,
							dest->y - actor->y);

	dist = dist / actor->info->speed;

	if (dist < 1)
		dist = 1;
	fixed_t slope = (dest->z+40*FRACUNIT - actor->z) / dist;

	if (slope < actor->momz)
		actor->momz -= FRACUNIT/8;
	else
		actor->momz += FRACUNIT/8;
}


void A_SkelWhoosh (AActor *actor)
{
	if (!actor->target)
		return;
	A_FaceTarget (actor);
	S_Sound (actor, CHAN_WEAPON, "skeleton/swing", 1, ATTN_NORM);
}

void A_SkelFist (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);

	if (P_CheckMeleeRange (actor))
	{
		int damage = ((P_Random (actor)%10)+1)*6;
		S_Sound (actor, CHAN_WEAPON, "skeleton/melee", 1, ATTN_NORM);
		P_DamageMobj (actor->target, actor, actor, damage, MOD_HIT);
	}
}



//
// PIT_VileCheck
// Detect a corpse that could be raised.
//
AActor* 		corpsehit;
AActor* 		vileobj;
fixed_t 		viletryx;
fixed_t 		viletryy;
int				viletryradius;

BOOL PIT_VileCheck (AActor *thing)
{
	int 	maxdist;
	BOOL 	check;

	if (thing->oflags & MFO_NORAISE)
		return true;	// [AM] Can't raise

	if (!(thing->flags & MF_CORPSE) )
		return true;	// not a monster

	if (thing->tics != -1)
		return true;	// not lying still yet

	if (thing->info->raisestate == S_NULL)
		return true;	// monster doesn't have a raise state

	maxdist = thing->info->radius + viletryradius;

	if ( abs(thing->x - viletryx) > maxdist
		 || abs(thing->y - viletryy) > maxdist )
		return true;			// not actually touching

	corpsehit = thing;
	corpsehit->momx = corpsehit->momy = 0;
	corpsehit->height <<= 2;
	check = P_CheckPosition (corpsehit, corpsehit->x, corpsehit->y);
	corpsehit->height >>= 2;

	return !check;
}



//
// A_VileChase
// Check for ressurecting a body
//
void A_VileChase (AActor *actor)
{
	if(!serverside)
	{
		// Return to normal attack.
		A_Chase (actor);
		return;
	}

	int 				xl;
	int 				xh;
	int 				yl;
	int 				yh;

	int 				bx;
	int 				by;

	mobjinfo_t* 		info;
	AActor::AActorPtr 	temp;

	if (actor->movedir != DI_NODIR)
	{
		// check for corpses to raise
		viletryx = actor->x + actor->info->speed * xspeed[actor->movedir];
		viletryy = actor->y + actor->info->speed * yspeed[actor->movedir];

		xl = (viletryx - bmaporgx - MAXRADIUS*2)>>MAPBLOCKSHIFT;
		xh = (viletryx - bmaporgx + MAXRADIUS*2)>>MAPBLOCKSHIFT;
		yl = (viletryy - bmaporgy - MAXRADIUS*2)>>MAPBLOCKSHIFT;
		yh = (viletryy - bmaporgy + MAXRADIUS*2)>>MAPBLOCKSHIFT;

		vileobj = actor;

		viletryradius = mobjinfo[MT_VILE].radius;

		for (bx=xl ; bx<=xh ; bx++)
		{
			for (by=yl ; by<=yh ; by++)
			{
				// Call PIT_VileCheck to check
				// whether object is a corpse
				// that canbe raised.
				if (!P_BlockThingsIterator(bx,by,PIT_VileCheck))
				{
					// got one!
					temp = actor->target;
					actor->target = corpsehit->ptr();
					A_FaceTarget (actor);
					actor->target = temp;

					P_SetMobjState (actor, S_VILE_HEAL1, true);

					if (!clientside)
						SV_Sound(corpsehit, CHAN_BODY, "vile/raise", ATTN_IDLE);
					else
						S_Sound(corpsehit, CHAN_BODY, "vile/raise", 1, ATTN_IDLE);

					info = corpsehit->info;

					if (serverside)
					{
						level.respawned_monsters++;
						SV_UpdateMonsterRespawnCount();
					}

					P_SetMobjState (corpsehit,info->raisestate, true);

					// [Nes] - Classic demo compatability: Ghost monster bug.
					if ((co_novileghosts)) {
						corpsehit->height = P_ThingInfoHeight(info);	// [RH] Use real mobj height
						corpsehit->radius = info->radius;	// [RH] Use real radius
					} else {
						corpsehit->height <<= 2;
					}

					corpsehit->flags = info->flags;
					corpsehit->health = info->spawnhealth;
					corpsehit->target = AActor::AActorPtr();

					return;
				}
			}
		}
	}

	// Return to normal attack.
	A_Chase (actor);
}


//
// A_VileStart
//
void A_VileStart (AActor *actor)
{
	S_Sound (actor, CHAN_VOICE, "vile/start", 1, ATTN_NORM);
}


//
// A_Fire
// Keep fire in front of player unless out of sight
//
void A_Fire (AActor *actor);

void A_StartFire (AActor *actor)
{
	S_Sound(actor, CHAN_BODY, "vile/firestrt", 1, ATTN_NORM);
	A_Fire(actor);
}

void A_FireCrackle (AActor *actor)
{
	S_Sound(actor, CHAN_BODY, "vile/firecrkl", 1, ATTN_NORM);
	A_Fire(actor);
}

void A_Fire (AActor *actor)
{
	AActor* 	dest;
	unsigned	an;

	dest = actor->tracer;
	if (!dest)
		return;

	// don't move it if the vile lost sight
	if (!P_CheckSight(actor->target, dest))
		return;

	an = dest->angle >> ANGLETOFINESHIFT;

	actor->SetOrigin (dest->x + FixedMul (24*FRACUNIT, finecosine[an]),
					  dest->y + FixedMul (24*FRACUNIT, finesine[an]),
					  dest->z);
}



//
// A_VileTarget
// Spawn the hellfire
//
void A_VileTarget (AActor *actor)
{
	AActor *fog;

	if (!actor->target)
		return;

	A_FaceTarget (actor);

	fog = new AActor (actor->target->x,
					  actor->target->x,
					  actor->target->z, MT_FIRE);

	actor->tracer = fog->ptr();
	fog->target = actor->ptr();
	fog->tracer = actor->target;
	A_Fire (fog);
}




//
// A_VileAttack
//
void A_VileAttack (AActor *actor)
{
	AActor *fire;
	int an;

	if (!actor->target)
		return;

	A_FaceTarget (actor);

	if (!P_CheckSight(actor, actor->target))
		return;

	S_Sound (actor, CHAN_WEAPON, "vile/stop", 1, ATTN_NORM);
	P_DamageMobj(actor->target, actor, actor, 20, MOD_VILEFIRE);
	actor->target->momz = 1000*FRACUNIT/actor->target->info->mass;

	an = actor->angle >> ANGLETOFINESHIFT;

	fire = actor->tracer;

	if (!fire)
		return;

	// move the fire between the vile and the player
	fire->x = actor->target->x - FixedMul (24*FRACUNIT, finecosine[an]);
	fire->y = actor->target->y - FixedMul (24*FRACUNIT, finesine[an]);
	P_RadiusAttack(fire, actor, 70, 70, true, MOD_VILEFIRE);
}




//
// Mancubus attack,
// firing three missiles (bruisers) in three different directions?
// Doesn't look like it.
//
#define FATSPREAD		(ANG90/8)

void A_FatRaise (AActor *actor)
{
	A_FaceTarget (actor);
	S_Sound (actor, CHAN_WEAPON, "fatso/raiseguns", 1, ATTN_NORM);
}


void A_FatAttack1 (AActor *actor)
{
	if(!actor->target)
		return;

	A_FaceTarget (actor);

	if(serverside)
	{
		// Change direction  to ...
		actor->angle += FATSPREAD;

		P_SpawnMissile (actor, actor->target, MT_FATSHOT);
		AActor *mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);

		mo->angle += FATSPREAD;
		int an = mo->angle >> ANGLETOFINESHIFT;
		mo->momx = FixedMul(mo->info->speed, finecosine[an]);
		mo->momy = FixedMul(mo->info->speed, finesine[an]);
	}
}

void A_FatAttack2 (AActor *actor)
{
	if(!actor->target)
		return;

	A_FaceTarget (actor);

	if(serverside)
	{
		// Now here choose opposite deviation.
		actor->angle -= FATSPREAD;

		P_SpawnMissile (actor, actor->target, MT_FATSHOT);

		AActor *mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);

		mo->angle -= FATSPREAD*2;
		int an = mo->angle >> ANGLETOFINESHIFT;
		mo->momx = FixedMul(mo->info->speed, finecosine[an]);
		mo->momy = FixedMul(mo->info->speed, finesine[an]);
	}
}

void A_FatAttack3 (AActor *actor)
{
	if(!actor->target)
		return;

	A_FaceTarget (actor);

	if(serverside)
	{
		AActor *mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);

		mo->angle -= FATSPREAD/2;
		int an = mo->angle >> ANGLETOFINESHIFT;

		mo->momx = FixedMul(mo->info->speed, finecosine[an]);
		mo->momy = FixedMul(mo->info->speed, finesine[an]);

		mo = P_SpawnMissile (actor, actor->target, MT_FATSHOT);
		mo->angle += FATSPREAD/2;
		an = mo->angle >> ANGLETOFINESHIFT;
		mo->momx = FixedMul(mo->info->speed, finecosine[an]);
		mo->momy = FixedMul(mo->info->speed, finesine[an]);
	}
}


//
// killough 9/98: a mushroom explosion effect, sorta :)
// Original idea: Linguica
//

void A_Mushroom(AActor* actor)
{
	if (!serverside)
		return;

	int n = actor->info->damage;

	// Mushroom parameters are part of code pointer's state
	fixed_t misc1 = actor->state->misc1 ? actor->state->misc1 : FRACUNIT * 4;
	fixed_t misc2 = actor->state->misc2 ? actor->state->misc2 : FRACUNIT / 2;

	A_Explode(actor); // make normal explosion

	for (int i = -n; i <= n; i += 8) // launch mushroom cloud
	{
		for (int j = -n; j <= n; j += 8)
		{
			AActor* target =
				new AActor(actor->x, actor->y, actor->z, MT_UNKNOWNTHING);
			target->x += i << FRACBITS; // Aim in many directions from source
			target->y += j << FRACBITS;
			target->z += P_AproxDistance(i, j) * misc1;             // Aim fairly high
			AActor* mo = P_SpawnMissile(actor, target, MT_FATSHOT); // Launch fireball
			if (mo != NULL)
			{
				mo->momx = FixedMul(mo->momx, misc2);
				mo->momy = FixedMul(mo->momy, misc2); // Slow down a bit
				mo->momz = FixedMul(mo->momz, misc2);
				mo->flags &= ~MF_NOGRAVITY; // Make debris fall under gravity
			}
			target->Destroy();
		}
	}
}



//
// SkullAttack
// Fly at the player like a missile.
//
#define SKULLSPEED (20*FRACUNIT)

void A_SkullAttack (AActor *actor)
{
	AActor* 			dest;
	angle_t 			an;
	int 				dist;

	if (!actor->target)
		return;

	dest = actor->target;
	actor->flags |= MF_SKULLFLY;

	S_Sound (actor, CHAN_VOICE, actor->info->attacksound, 1, ATTN_NORM);
	A_FaceTarget (actor);
	an = actor->angle >> ANGLETOFINESHIFT;
	actor->momx = FixedMul (SKULLSPEED, finecosine[an]);
	actor->momy = FixedMul (SKULLSPEED, finesine[an]);
	dist = P_AproxDistance (dest->x - actor->x, dest->y - actor->y);
	dist = dist / SKULLSPEED;

	if (dist < 1)
		dist = 1;
	actor->momz = (dest->z+(dest->height>>1) - actor->z) / dist;
}

//
// A_BetaSkullAttack()
// killough 10/98: this emulates the beta version's lost soul attacks
//

void A_BetaSkullAttack(AActor* actor)
{
	if (!actor->target || actor->target->type == MT_SKULL)
		return;

	// [FG] fix crash when attack sound is missing
	if (actor->info->attacksound)
	{
		S_Sound(actor, CHAN_VOICE, actor->info->attacksound, 1, ATTN_NORM);
	}
	A_FaceTarget(actor);
	int damage = (P_Random(actor) % 8 + 1) * actor->info->damage;
	P_DamageMobj(actor->target, actor, actor, damage);
}

//
// A_SpawnObject
// Basically just A_Spawn with better behavior and more args.
//   args[0]: Type of actor to spawn
//   args[1]: Angle (degrees, in fixed point), relative to calling actor's angle
//   args[2]: X spawn offset (fixed point), relative to calling actor
//   args[3]: Y spawn offset (fixed point), relative to calling actor
//   args[4]: Z spawn offset (fixed point), relative to calling actor
//   args[5]: X velocity (fixed point)
//   args[6]: Y velocity (fixed point)
//   args[7]: Z velocity (fixed point)
//
void A_SpawnObject(AActor* actor)
{
	int type, angle, ofs_x, ofs_y, ofs_z, vel_x, vel_y, vel_z;
	angle_t an;
	fixed_t fan, dx, dy;
	AActor* mo;

	if (!actor->state->args[0] || !serverside)
		return;

	type = actor->state->args[0] - 1;
	angle = actor->state->args[1];
	ofs_x = actor->state->args[2];
	ofs_y = actor->state->args[3];
	ofs_z = actor->state->args[4];
	vel_x = actor->state->args[5];
	vel_y = actor->state->args[6];
	vel_z = actor->state->args[7];

	if (!CheckIfDehActorDefined((mobjtype_t)type))
	{
		I_Error("A_SpawnObject: Attempted to spawn undefined object type.");
	}

	// calculate position offsets
	an = actor->angle + (unsigned int)(((int64_t)angle << 16) / 360);
	fan = an >> ANGLETOFINESHIFT;
	dx = FixedMul(ofs_x, finecosine[fan]) - FixedMul(ofs_y, finesine[fan]);
	dy = FixedMul(ofs_x, finesine[fan]) + FixedMul(ofs_y, finecosine[fan]);
	// spawn it, yo
	mo = new AActor(actor->x + dx, actor->y + dy, actor->z + ofs_z, (mobjtype_t)type);
	if (!mo)
		return;

	// angle dangle
	mo->angle = an;

	// set velocity
	mo->momx = FixedMul(vel_x, finecosine[fan]) - FixedMul(vel_y, finesine[fan]);
	mo->momy = FixedMul(vel_x, finesine[fan]) + FixedMul(vel_y, finecosine[fan]);
	mo->momz = vel_z;

	// if spawned object is a missile, set target+tracer
	if (mo->info->flags & (MF_MISSILE | MF_BOUNCES))
	{
		// if spawner is also a missile, copy 'em
		if (actor->info->flags & (MF_MISSILE | MF_BOUNCES))
		{
			mo->target = actor->target;
			mo->tracer = actor->tracer;
		}
		// otherwise, set 'em as if a monster fired 'em
		else
		{
			mo->target = actor->ptr();
			mo->tracer = actor->target;
		}
	}

	// [XA] don't bother with the dont-inherit-friendliness hack
	// that exists in A_Spawn, 'cause WTF is that about anyway?
}

//
// A_MonsterProjectile
// A parameterized monster projectile attack.
//   args[0]: Type of actor to spawn
//   args[1]: Angle (degrees, in fixed point), relative to calling actor's angle
//   args[2]: Pitch (degrees, in fixed point), relative to calling actor's pitch; approximated
//   args[3]: X/Y spawn offset, relative to calling actor's angle
//   args[4]: Z spawn offset, relative to actor's default projectile fire height
//
void A_MonsterProjectile(AActor* actor)
{
	int type, angle, pitch, spawnofs_xy, spawnofs_z;
	AActor* mo;
	int an;

	if (!actor->target || !actor->state->args[0] || !serverside)
		return;

	type = actor->state->args[0] - 1;
	angle = actor->state->args[1];
	pitch = actor->state->args[2];
	spawnofs_xy = actor->state->args[3];
	spawnofs_z = actor->state->args[4];

	if (!CheckIfDehActorDefined((mobjtype_t)type))
	{
		I_Error("A_MonsterProjectile: Attempted to spawn undefined projectile type.");
	}

	A_FaceTarget(actor);
	mo = P_SpawnMissile(actor, actor->target, (mobjtype_t)type);
	if (!mo)
		return;

	// adjust angle
	mo->angle += (angle_t)(((int64_t)angle << 16) / 360);
	an = mo->angle >> ANGLETOFINESHIFT;
	mo->momx = FixedMul(mo->info->speed, finecosine[an]);
	mo->momy = FixedMul(mo->info->speed, finesine[an]);

	// adjust pitch (approximated, using Doom's ye olde
	// finetangent table; same method as monster aim)
	mo->momz += FixedMul(mo->info->speed, DegToSlope(pitch));

	// adjust position
	an = (actor->angle - ANG90) >> ANGLETOFINESHIFT;
	mo->x += FixedMul(spawnofs_xy, finecosine[an]);
	mo->y += FixedMul(spawnofs_xy, finesine[an]);
	mo->z += spawnofs_z;

	// always set the 'tracer' field, so this pointer
	// can be used to fire seeker missiles at will.
	mo->tracer = actor->target;
}

//
// A_MonsterBulletAttack
// A parameterized monster bullet attack.
//   args[0]: Horizontal spread (degrees, in fixed point)
//   args[1]: Vertical spread (degrees, in fixed point)
//   args[2]: Number of bullets to fire; if not set, defaults to 1
//   args[3]: Base damage of attack (e.g. for 3d5, customize the 3); if not set, defaults to 3
//   args[4]: Attack damage modulus (e.g. for 3d5, customize the 5); if not set, defaults to 5
//

void A_MonsterBulletAttack(AActor* actor)
{
	int hspread, vspread, numbullets, damagebase, damagemod;
	int aimslope, i, damage, angle, slope;

	if (!actor->target)
		return;

	hspread = actor->state->args[0];
	vspread = actor->state->args[1];
	numbullets = actor->state->args[2];
	damagebase = actor->state->args[3];
	damagemod = actor->state->args[4];

	A_FaceTarget(actor);
	S_Sound(actor, CHAN_WEAPON, actor->info->attacksound, 1, ATTN_NORM);

	aimslope = P_AimLineAttack(actor, actor->angle, MISSILERANGE);

	for (i = 0; i < numbullets; i++)
	{
		damage = (P_Random() % damagemod + 1) * damagebase;
		angle = (int)actor->angle + P_RandomHitscanAngle(hspread);
		slope = aimslope + P_RandomHitscanSlope(vspread);

		P_LineAttack(actor, angle, MISSILERANGE, slope, damage);
	}
}

//
// A_MonsterMeleeAttack
// A parameterized monster melee attack.
//   args[0]: Base damage of attack (e.g. for 3d8, customize the 3); if not set, defaults to 3
//   args[1]: Attack damage modulus (e.g. for 3d8, customize the 8); if not set, defaults to 8
//   args[2]: Sound to play if attack hits args[3]: Range (fixed point); if not set, defaults to monster's melee range
//
void A_MonsterMeleeAttack(AActor* actor)
{
	int damagebase, damagemod, hitsound, range;
	int damage;

	if (!actor->target)
		return;

	damagebase = actor->state->args[0];
	damagemod = actor->state->args[1];
	hitsound = actor->state->args[2];
	range = actor->state->args[3];

	if (range <= 0)
		range = actor->info->meleerange;

	range += actor->target->info->radius - 20 * FRACUNIT;

	A_FaceTarget(actor);
	if (!P_CheckRange(actor, range))
		return;

	S_Sound(actor, CHAN_WEAPON, SoundMap[hitsound], 1, ATTN_NORM);

	damage = (P_Random() % damagemod + 1) * damagebase;
	P_DamageMobj(actor->target, actor, actor, damage, MOD_HIT);
}

//
// A_RadiusDamage
// A parameterized version of A_Explode. Friggin' finally. :P
//   args[0]: Damage (int)
//   args[1]: Radius (also int; no real need for fractional precision here)
//
void A_RadiusDamage(AActor* actor)
{
	if (!actor->state)
		return;

	P_RadiusAttack(actor, actor->target, actor->state->args[0], actor->state->args[1], true, MOD_R_SPLASH);
}

//
// A_NoiseAlert
// Alerts nearby monsters (via sound) to the calling actor's target's presence.
//
void A_NoiseAlert(AActor* actor)
{
	if (!actor->target)
		return;

	P_NoiseAlert(actor->target, actor);
}

//
// A_HealChase
// A parameterized version of A_VileChase.
//   args[0]: State to jump to on the calling actor when resurrecting a corpse
//   args[1]: Sound to play when resurrecting a corpse
//
void A_HealChase(AActor* actor)
{
	int state, sound;

	if (!actor)
		return;

	state = actor->state->args[0];
	sound = actor->state->args[1];

	if (!P_HealCorpse(actor, actor->info->radius, state, sound))
		A_Chase(actor);
}

//
// P_HealCorpse
// A generic corpse resurrection codepointer.
//
bool P_HealCorpse(AActor* actor, int radius, int healstate, int healsound)
{
	// don't attempt to resurrect clientside
	if (!serverside)
	{
		return false;
	}

	int xl, xh;
	int yl, yh;
	int bx, by;

	if (actor->movedir != DI_NODIR)
	{
		// check for corpses to raise
		viletryx = actor->x + actor->info->speed * xspeed[actor->movedir];
		viletryy = actor->y + actor->info->speed * yspeed[actor->movedir];

		xl = (viletryx - bmaporgx - MAXRADIUS * 2) >> MAPBLOCKSHIFT;
		xh = (viletryx - bmaporgx + MAXRADIUS * 2) >> MAPBLOCKSHIFT;
		yl = (viletryy - bmaporgy - MAXRADIUS * 2) >> MAPBLOCKSHIFT;
		yh = (viletryy - bmaporgy + MAXRADIUS * 2) >> MAPBLOCKSHIFT;

		vileobj = actor;
		viletryradius = radius;
		for (bx = xl; bx <= xh; bx++)
		{
			for (by = yl; by <= yh; by++)
			{
				// Call PIT_VileCheck to check
				// whether object is a corpse
				// that can be raised.
				if (!P_BlockThingsIterator(bx, by, PIT_VileCheck))
				{
					mobjinfo_t* info;

					// got one!
					AActor::AActorPtr temp = actor->target;
					actor->target = corpsehit->ptr();
					A_FaceTarget(actor);
					actor->target = temp;

					P_SetMobjState(actor, (statenum_t)healstate, true);

					if (!clientside)
						SV_Sound(corpsehit, CHAN_BODY, SoundMap[healsound], ATTN_IDLE);
					else
						S_Sound(corpsehit, CHAN_BODY, SoundMap[healsound], 1, ATTN_IDLE);

					info = corpsehit->info;

					if (serverside)
					{
						level.respawned_monsters++;
						SV_UpdateMonsterRespawnCount();
					}

					P_SetMobjState(corpsehit, info->raisestate, true);

					// [Nes] - Classic demo compatability: Ghost monster bug.
					if ((co_novileghosts))
					{
						corpsehit->height =
						    P_ThingInfoHeight(info);      // [RH] Use real mobj height
						corpsehit->radius = info->radius; // [RH] Use real radius
					}
					else
					{
						corpsehit->height <<= 2;
					}

					corpsehit->flags = info->flags;
					corpsehit->health = info->spawnhealth;
					corpsehit->target = AActor::AActorPtr();

					return true;
				}
			}
		}
	}
	return false;
}

//
// A_SeekTracer
// A parameterized seeker missile function.
//   args[0]: direct-homing threshold angle (degrees, in fixed point)
//   args[1]: maximum turn angle (degrees, in fixed point)
//
void A_SeekTracer(AActor* actor)
{
	angle_t threshold, maxturnangle;

	if (!actor || !serverside)
		return;

	threshold = FixedToAngle(actor->state->args[0]);
	maxturnangle = FixedToAngle(actor->state->args[1]);

	if (P_SeekerMissile(actor, actor->tracer, threshold, maxturnangle, true))
	{
		actor->flags2 |= MF2_SEEKERMISSILE;
		SV_UpdateMobj(actor);
	}
	else
	{
		actor->flags2 &= ~MF2_SEEKERMISSILE;
	}
}

//
// A_FindTracer
// Search for a valid tracer (seek target), if the calling actor doesn't already have one.
//   args[0]: field-of-view to search in (degrees, in fixed point); if zero, will search
//   in all directions
//   args[1]: distance to search (map blocks, i.e. 128 units)
//
void A_FindTracer(AActor* actor)
{
	angle_t fov;
	int dist;

	if (!actor || (actor->tracer != AActor::AActorPtr() && actor->tracer->health > 0) ||
	    !serverside)
		return;

	if (actor->tracer != AActor::AActorPtr() && actor->tracer->health <= 0)
		actor->tracer = AActor::AActorPtr(); // [Blair] Clear tracer if it died, to keep
		                                     // with MBF21 spec

	fov = FixedToAngle(actor->state->args[0]);
	dist = (actor->state->args[1]);

	AActor* tracer = P_RoughTargetSearch(actor, fov, dist);

	if (!tracer || tracer->health <= 0)
		return;

	actor->tracer = tracer->ptr();

	actor->flags2 |= MF2_SEEKERMISSILE;
	SV_UpdateMobj(actor);
}

//
// A_ClearTracer
// Clear current tracer (seek target).
//
void A_ClearTracer(AActor* actor)
{
	if (!actor || !serverside)
		return;

	actor->tracer = AActor::AActorPtr();

	actor->flags2 &= ~MF2_SEEKERMISSILE;
	SV_UpdateMobj(actor);
}

//
// A_JumpIfHealthBelow
// Jumps to a state if caller's health is below the specified threshold.
//   args[0]: State to jump to
//   args[1]: Health threshold
//
void A_JumpIfHealthBelow(AActor* actor)
{
	int state, health;

	if (!actor)
		return;

	state = actor->state->args[0];
	health = actor->state->args[1];

	if (actor->health < health)
		P_SetMobjState(actor, (statenum_t)state, true);
}

//
// A_JumpIfTargetInSight
// Jumps to a state if caller's target is in line-of-sight.
//   args[0]: State to jump to
//   args[1]: Field-of-view to check (degrees, in fixed point); if zero, will check in all
//   directions
//
void A_JumpIfTargetInSight(AActor* actor)
{
	int state;
	angle_t fov;

	if (!actor || !actor->target)
		return;

	state = (actor->state->args[0]);
	fov = FixedToAngle(actor->state->args[1]);

	// Check FOV first since it's faster
	if (fov > 0 && !P_CheckFov(actor, actor->target, fov))
		return;

	if (P_CheckSight(actor, actor->target))
		P_SetMobjState(actor, (statenum_t)state, true);
}


//
// A_JumpIfTargetCloser
// Jumps to a state if caller's target is closer than the specified distance.
//   args[0]: State to jump to
//   args[1]: Distance threshold
//
void A_JumpIfTargetCloser(AActor* actor)
{
	int state, distance;

	if (!actor || !actor->target)
		return;

	state = actor->state->args[0];
	distance = actor->state->args[1];

	if (distance >
	    P_AproxDistance(actor->x - actor->target->x, actor->y - actor->target->y))
		P_SetMobjState(actor, (statenum_t)state, true);
}

//
// A_JumpIfTracerInSight
// Jumps to a state if caller's tracer (seek target) is in line-of-sight.
//   args[0]: State to jump to
//   args[1]: Field-of-view to check (degrees, in fixed point); if zero, will check in all
//   directions
//
void A_JumpIfTracerInSight(AActor* actor)
{
	angle_t fov;
	int state;

	if (!actor || !actor->tracer || !serverside)
		return;

	state = (actor->state->args[0]);
	fov = FixedToAngle(actor->state->args[1]);

	// Check FOV first since it's faster
	if (fov > 0 && !P_CheckFov(actor, actor->tracer, fov))
		return;

	if (actor->tracer->health <= 0)
		return;

	if (P_CheckSight(actor, actor->tracer))
		P_SetMobjState(actor, (statenum_t)state, true);
}

//
// A_JumpIfTracerCloser
// Jumps to a state if caller's tracer (seek target) is closer than the specified
// distance.
//   args[0]: State to jump to
//   args[1]: Distance threshold (fixed point)
//
void A_JumpIfTracerCloser(AActor* actor)
{
	int state, distance;

	if (!actor || !actor->tracer || !serverside)
		return;

	state = actor->state->args[0];
	distance = actor->state->args[1];

	if (actor->tracer->health <= 0)
		return;

	if (distance >
		P_AproxDistance(actor->x - actor->tracer->x, actor->y - actor->tracer->y))
		P_SetMobjState(actor, (statenum_t)state, true);
}

//
// A_JumpIfFlagsSet
// Jumps to a state if caller has the specified thing flags set.
//   args[0]: State to jump to
//   args[1]: Standard Flag(s) to check
//   args[2]: MBF21 Flag(s) to check
//
void A_JumpIfFlagsSet(AActor* actor)
{
	int state;
	int flags, flags2;

	if (!actor)
		return;

	state = actor->state->args[0];
	flags = actor->state->args[1];
	flags2 = actor->state->args[2];

	if ((actor->flags & flags) == flags && (actor->flags2 & flags2) == flags2)
		P_SetMobjState(actor, (statenum_t)state, true);
}


//
// A_AddFlags
// Adds the specified thing flags to the caller.
//   args[0]: Standard Flag(s) to add
//   args[1]: MBF21 Flag(s) to add
//
void A_AddFlags(AActor* actor)
{
	int flags, flags2;

	if (!actor)
		return;

	flags = actor->state->args[0];
	flags2 = actor->state->args[1];

	actor->flags |= flags;
	actor->flags2 |= flags2;
}

//
// A_RemoveFlags
// Removes the specified thing flags from the caller.
//   args[0]: Flag(s) to remove
//   args[1]: MBF21 Flag(s) to remove
//
void A_RemoveFlags(AActor* actor)
{
	int flags, flags2;

	if (!actor)
		return;

	flags = actor->state->args[0];
	flags2 = actor->state->args[1];

	actor->flags &= ~flags;
	actor->flags2 &= ~flags2;
}

void A_Stop(AActor* actor)
{
	actor->momx = actor->momy = actor->momz = 0;
}

// P_RemoveSoulLimit
bool P_RemoveSoulLimit()
{
	if (level.flags & LEVEL_COMPAT_LIMITPAIN)
		return false;

	return co_removesoullimit;
}

//
// A_PainShootSkull
// Spawn a lost soul and launch it at the target
//
void A_PainShootSkull (AActor *actor, angle_t angle)
{
	fixed_t 	x;
	fixed_t 	y;
	fixed_t 	z;

	AActor* 	other;
	angle_t 	an;
	int 		prestep;
	int 		count;

	if(!serverside)
		return;

	// count total number of skull currently on the level
	count = 0;

	TThinkerIterator<AActor> iterator;

	while ( (other = iterator.Next ()) )
	{
		if (other->type == MT_SKULL)
			count++;
	}

	// if there are already 20 skulls on the level,
	// don't spit another one
	// co_removesoullimit removes the standard limit
	if (count > 20 && !P_RemoveSoulLimit())
		return;
	// multiplayer retains a hard limit of 128
	if (multiplayer && count > 128)
		return;
	// okay, there's room for another one
	an = angle >> ANGLETOFINESHIFT;

	prestep = 4*FRACUNIT + 3*(actor->info->radius + mobjinfo[MT_SKULL].radius)/2;

	x = actor->x + FixedMul (prestep, finecosine[an]);
	y = actor->y + FixedMul (prestep, finesine[an]);
	z = actor->z + 8*FRACUNIT;

	// Check whether the Lost Soul is being fired through a 1-sided	// phares
	// wall or an impassible line, or a "monsters can't cross" line.//   |
	// If it is, then we don't allow the spawn.						//   V
	// denis - vanilla desync
	/*
	if (Check_Sides(actor,x,y))
		return;*/

	other = new AActor (x, y, z, MT_SKULL);

	// Check to see if the new Lost Soul's z value is above the
	// ceiling of its new sector, or below the floor. If so, kill it.
	// denis - vanilla desync
	/*
	if ((other->z >
         (other->subsector->sector->ceilingheight - other->height)) ||
        (other->z < other->subsector->sector->floorheight))
	{
		// kill it immediately
		P_DamageMobj (other, actor, actor, 10000, MOD_UNKNOWN);		//   ^
		return;														//   |
	}																// phares
	 */
	// Check for movements.
	if (!P_TryMove(other, x, y, false))
	{
		// kill it immediately
		P_DamageMobj (other, actor, actor, 10000, MOD_UNKNOWN);
		return;
	}

	other->target = actor->target;
	A_SkullAttack (other);
}


//
// A_PainAttack
// Spawn a lost soul and launch it at the target
//
void A_PainAttack (AActor *actor)
{
	if (!actor->target)
		return;

	A_FaceTarget (actor);

	if(!serverside)
		return;

	A_PainShootSkull (actor, actor->angle);
}

void A_PainDie (AActor *actor)
{
	A_Fall (actor);

	if(!serverside)
		return;

	A_PainShootSkull (actor, actor->angle+ANG90);
	A_PainShootSkull (actor, actor->angle+ANG180);
	A_PainShootSkull (actor, actor->angle+ANG270);
}

void A_Scream (AActor *actor)
{
    char sound[MAX_SNDNAME];

	if (actor->info->deathsound == NULL)
        return;


	strcpy (sound, actor->info->deathsound);

    if (stricmp(sound, "grunt/death1") == 0 ||
        stricmp(sound, "shotguy/death1") == 0 ||
        stricmp(sound, "chainguy/death1") == 0)
    {
        sound[strlen(sound)-1] = P_Random(actor) % 3 + '1';
    }

    if (stricmp(sound, "imp/death1") == 0 ||
        stricmp(sound, "imp/death2") == 0)
    {
        sound[strlen(sound)-1] = P_Random(actor) % 2 + '1';
    }

	if (!co_zdoomsound && (actor->flags2 & MF2_BOSS || actor->flags3 & MF3_FULLVOLSOUNDS))
		S_Sound(CHAN_VOICE, sound, 1, ATTN_NORM);
	else
	    S_Sound (actor, CHAN_VOICE, sound, 1, ATTN_NORM);
}


void A_XScream (AActor *actor)
{
	if (actor->player)
		S_Sound (actor, CHAN_VOICE, "*gibbed", 1, ATTN_NORM);
	else
		S_Sound (actor, CHAN_VOICE, "misc/gibbed", 1, ATTN_NORM);
}

void A_Pain (AActor *actor)
{
	if (actor->info->painsound)
		S_Sound (actor, CHAN_VOICE, actor->info->painsound, 1, ATTN_NORM);
}



void A_Fall (AActor *actor)
{
	// actor is on ground, it can be walked over
	actor->flags &= ~MF_SOLID;

	// So change this if corpse objects
	// are meant to be obstacles.

	// Remove any sort of boss effect on kill
	// OFlags hack because of client issues
	// Only remove the sparkling fountain, keep the transition
	if (actor->type != MT_PLAYER && (actor->oflags & hordeBossModMask))
	{
		actor->effects = 0;
	}
}


// killough 11/98: kill an object
void A_Die (AActor *actor)
{
	P_DamageMobj (actor, NULL, NULL, actor->health, MOD_UNKNOWN);
}

//
// A_Detonate
// killough 8/9/98: same as A_Explode, except that the damage is variable
//

void A_Detonate (AActor *mo)
{
	P_RadiusAttack (mo, mo->target, mo->damage, mo->damage, true, MOD_R_SPLASH);
	if (mo->z <= mo->floorz + (mo->damage<<FRACBITS))
	{
		P_HitFloor (mo);
	}
}


//
// A_Explode
//
void A_Explode (AActor *thing)
{
	// [RH] figure out means of death;
	int mod;
	int damage = 128;
	int distance = 128;
	bool hurtSource = true;

	switch (thing->type) {
		case MT_BARREL:
			mod = MOD_BARREL;
			break;
		case MT_ROCKET:
			mod = MOD_R_SPLASH;
			break;
		default:
			mod = MOD_R_SPLASH;
			break;
	}

	P_RadiusAttack (thing, thing->target, damage, distance, hurtSource, mod);
}

//
// A_BossDeath
// Possibly trigger special effects if on a boss level
//
void A_BossDeath(AActor *actor)
{
	// make sure there is a player alive for victory
	Players::const_iterator it = players.begin();
	for (; it != players.end(); ++it)
	{
		if (it->ingame() && it->health > 0)
			break;
	}

	if (it == players.end())
		return; // no one left alive, so do not end game

	if (!level.bossactions.empty())
	{
		std::vector<bossaction_t>::iterator ba = level.bossactions.begin();

		// see if the BossAction applies to this type
		for (; ba != level.bossactions.end(); ++ba)
		{
			if (ba->type == actor->type)
				break;
		}
		if (ba == level.bossactions.end())
			return;

		// scan the remaining thinkers to see if all bosses are dead
		TThinkerIterator<AActor> iterator;
		AActor* other;

		while ((other = iterator.Next()))
		{
			if (other != actor && other->type == actor->type && other->health > 0)
			{
				// other boss not dead
				return;
			}
		}

		ba = level.bossactions.begin();

		for (; ba != level.bossactions.end(); ++ba)
		{
			if (ba->type == actor->type)
			{
				line_t ld;

				if (map_format.getZDoom())
				{
					maplinedef_t mld;
					mld.special = (ba->special);
					mld.tag = (ba->tag);

					P_TranslateLineDef(&ld, &mld);
				}
				else
				{
					ld.special = ba->special;
					ld.id = ba->tag;
				}

				if (!P_UseSpecialLine(actor, &ld, 0, true))
					P_CrossSpecialLine(&ld, 0, actor, true);
			}
		}
	}
}


void A_Hoof (AActor *mo)
{
	S_Sound (mo, CHAN_BODY, "cyber/hoof", 1, ATTN_IDLE);
	A_Chase (mo);
}

void A_Metal (AActor *mo)
{
	S_Sound (mo, CHAN_BODY, "spider/walk", 1, ATTN_IDLE);
	A_Chase (mo);
}

void A_BabyMetal (AActor *mo)
{
	S_Sound (mo, CHAN_BODY, "baby/walk", 1, ATTN_IDLE);
	A_Chase (mo);
}

// killough 2/7/98: Remove limit on icon landings:
AActor **braintargets;
int    numbraintargets_alloc;
int    numbraintargets;

struct brain_s brain;   // killough 3/26/98: global state of boss brain

// killough 3/26/98: initialize icon landings at level startup,
// rather than at boss wakeup, to prevent savegame-related crashes

void P_SpawnBrainTargets (void)	// killough 3/26/98: renamed old function
{
	AActor *other;
	TThinkerIterator<AActor> iterator;

	// find all the target spots
	numbraintargets = 0;
	brain.targeton = 0;
	brain.easy = 0;				// killough 3/26/98: always init easy to 0

	while ( (other = iterator.Next ()) )
	{
		if (other->type == MT_BOSSTARGET)
		{	// killough 2/7/98: remove limit on icon landings:
			if (numbraintargets >= numbraintargets_alloc)
			{
				braintargets = (AActor **)Realloc (braintargets,
					(numbraintargets_alloc = numbraintargets_alloc ?
					 numbraintargets_alloc*2 : 32) *sizeof *braintargets);
			}
			braintargets[numbraintargets++] = other;
		}
	}
}

void A_BrainAwake (AActor *mo)
{
	// killough 3/26/98: only generates sound now
	S_Sound (mo, CHAN_VOICE, "brain/sight", 1, ATTN_NONE);
}


void A_BrainPain (AActor *mo)
{
	S_Sound (mo, CHAN_VOICE, "brain/pain", 1, ATTN_NONE);
}


void A_BrainScream (AActor *mo)
{
	if(!clientside)
		return;

	int 		x;
	int 		y;
	int 		z;
	AActor* 	th;

	for (x=mo->x - 196*FRACUNIT ; x< mo->x + 320*FRACUNIT ; x+= FRACUNIT*8)
	{
		y = mo->y - 320*FRACUNIT;
		z = 128 + (P_Random (mo) << (FRACBITS + 1));
		th = new AActor (x,y,z, MT_ROCKET);
		th->momz = P_Random (mo) << 9;

		P_SetMobjState (th, S_BRAINEXPLODE1, true);

		th->tics -= P_Random (mo) & 7;
		if (th->tics < 1)
			th->tics = 1;
	}

	S_Sound (mo, CHAN_VOICE, "brain/death", 1, ATTN_NONE);
}



void A_BrainExplode (AActor *mo)
{
	if(!clientside)
		return;

	int x = mo->x + P_RandomDiff (mo)*2048;
	int y = mo->y;
	int z = 128 + P_Random (mo)*2*FRACUNIT;
	AActor *th = new AActor (x, y, z, MT_ROCKET);
	th->momz = P_Random (mo) << 9;

	P_SetMobjState (th, S_BRAINEXPLODE1, true);

	th->tics -= P_Random (mo) & 7;
	if (th->tics < 1)
		th->tics = 1;
}


void A_BrainDie (AActor *mo)
{
	// [RH] If noexit, then don't end the level.
	if (sv_gametype != GM_COOP && !sv_allowexit)
		return;

	G_ExitLevel (0, 1);
}

void A_BrainSpit (AActor *mo)
{
	if(!serverside)
		return;

	// [RH] Do nothing if there are no brain targets.
	if (numbraintargets == 0)
		return;

	brain.easy ^= 1;		// killough 3/26/98: use brain struct
	if (G_GetCurrentSkill().easy_boss_brain && (!brain.easy))
		return;

	// shoot a cube at current target
	AActor* targ = braintargets[brain.targeton++];	// killough 3/26/98:
	brain.targeton %= numbraintargets;		// Use brain struct for targets

	// spawn brain missile
	AActor* newmobj = P_SpawnMissile(mo, targ, MT_SPAWNSHOT);

	if(newmobj)
	{
		newmobj->target = targ->ptr();
		newmobj->reactiontime =
			((targ->y - mo->y)/newmobj->momy) / newmobj->state->tics;
	}

	S_Sound (mo, CHAN_WEAPON, "brain/spit", 1, ATTN_NONE);
}



void A_SpawnFly (AActor *mo);

// travelling cube sound
void A_SpawnSound (AActor *mo)
{
	S_Sound (mo, CHAN_BODY, "brain/cube", 1, ATTN_IDLE);
	A_SpawnFly(mo);
}

void A_SpawnFly (AActor *mo)
{
	AActor* 	newmobj;
	AActor* 	fog;
	AActor* 	targ;
	int 		r;
	mobjtype_t	type;

	if (--mo->reactiontime)
		return; // still flying

	if(!serverside)
		return;

	targ = mo->target;
	// When loading a save game, any in-flight cube will have lost its pointer to its target.
	if (!targ) {
		mo->Destroy ();
		return;
	}

	// First spawn teleport fog.
	fog = new AActor (targ->x, targ->y, targ->z, MT_SPAWNFIRE);
	S_Sound (fog, CHAN_BODY, "misc/teleport", 1, ATTN_NORM);

	// Randomly select monster to spawn.
	r = P_Random (mo);

	// Probability distribution (kind of :),
	// decreasing likelihood.
	if ( r<50 )
		type = MT_TROOP;
	else if (r<90)
		type = MT_SERGEANT;
	else if (r<120)
		type = MT_SHADOWS;
	else if (r<130)
		type = MT_PAIN;
	else if (r<160)
		type = MT_HEAD;
	else if (r<162)
		type = MT_VILE;
	else if (r<172)
		type = MT_UNDEAD;
	else if (r<192)
		type = MT_BABY;
	else if (r<222)
		type = MT_FATSO;
	else if (r<246)
		type = MT_KNIGHT;
	else
		type = MT_BRUISER;

	newmobj = new AActor (targ->x, targ->y, targ->z, type);
	if (P_LookForPlayers (newmobj, true))
		P_SetMobjState (newmobj, newmobj->info->seestate, true);

	// telefrag anything in this spot
	P_TeleportMove (newmobj, newmobj->x, newmobj->y, newmobj->z, true);

	// remove self (i.e., cube).
	mo->Destroy ();

	// [SL] 2011-06-19 - Emulate vanilla doom bug where monsters spawned after
	// the start of the level (eg, spawned from a cube) are respawned at map
	// location (0, 0).
	memset(&newmobj->spawnpoint, 0, sizeof(newmobj->spawnpoint));
}



void A_PlayerScream (AActor *mo)
{
	if (!mo)
		return;

	char nametemp[128];
	const char *sound;

	// [SL] 2011-06-15 - Spectators shouldn't make any noises
	if (mo->player && mo->player->spectator)
		return;

	// [Fly] added for csDoom
	if (mo->health < -mo->info->spawnhealth)
	{
		A_XScream (mo);
		return;
	}

	if (!(gameinfo.flags & GI_NOCRAZYDEATH) && (mo->health < -50))
	{
		// IF THE PLAYER DIES LESS THAN -50% WITHOUT GIBBING
		sound = "*xdeath1";
	} else {
		// [RH] More variety in death sounds
		//sprintf (nametemp, "*death%d", (M_Random ()&3) + 1); // denis - do not use randomness source!
		snprintf (nametemp, 128, "*death1");
		sound = nametemp;
	}

	S_Sound (mo, CHAN_VOICE, sound, 1, ATTN_NORM);
}

void A_Gibify(AActor *mo) // denis - squash thing
{
	mo->flags &= ~MF_SOLID;
	mo->height = 0;
	mo->radius = 0;
}

//
// killough 11/98
//
// The following were inspired by Len Pitre
//
// A small set of highly-sought-after code pointers
//
void A_Spawn(AActor* mo)
{
	if (!serverside)
		return;

	// Partial integration of A_Spawn.
	// ToDo: Currently missing MBF's MF_FRIEND flag support!
	if (mo->state->misc1)
	{
		AActor* newmobj;

		newmobj = new AActor(mo->x, mo->y, (mo->state->misc2 << FRACBITS) + mo->z,
			                    (mobjtype_t)(mo->state->misc1 - 1));

		// newmobj->flags = (newmobj->flags & ~MF_FRIEND) | (mo->flags & MF_FRIEND);
		// // TODO !!!
	}
}

void A_Turn(AActor* mo)
{
	mo->angle += (angle_t)(((uint64_t)mo->state->misc1 << 32) / 360);
}

void A_Face(AActor* mo)
{
	mo->angle = (angle_t)(((uint64_t)mo->state->misc1 << 32) / 360);
}

void A_Scratch(AActor* mo)
{
	if (mo->target)
	{
		A_FaceTarget(mo);

		if (P_CheckMeleeRange(mo))
		{
			if (mo->state->misc2)
				S_Sound(mo, CHAN_BODY, SoundMap[mo->state->misc2], 1, ATTN_NORM);

			P_DamageMobj(mo->target, mo, mo, mo->state->misc1);
		}
	}
}

void A_PlaySound(AActor* mo)
{
	// Play the sound from the SoundMap

	int sndmap = mo->state->misc1;

	if (sndmap >= static_cast<int>(ARRAY_LENGTH(SoundMap)))
	{
		DPrintf("Warning: Sound ID is beyond the array of the Sound Map!\n");
		sndmap = 0;
	}

	S_Sound(mo, CHAN_BODY, SoundMap[sndmap], 1,
		        (mo->state->misc2 ? ATTN_NONE : ATTN_NORM));
}

void A_RandomJump(AActor* mo)
{
	if (P_Random(mo) < mo->state->misc2)
		P_SetMobjState(mo, (statenum_t)mo->state->misc1);
}

//
// This allows linedef effects to be activated inside deh frames.
//

void A_LineEffect(AActor* mo)
{
	/* [AM] Not implemented...yet.
	if (!(mo->intflags & MIF_LINEDONE))                // Unless already used up
	{
		line_t junk = *lines;                          // Fake linedef set to 1st
		if ((junk.special = (short)mo->state->misc1))  // Linedef type
		{
			// [FG] made static
			static player_t player;                    // Remember player status
			player_t* oldplayer = mo->player;          // Remember player status
			mo->player = &player;                      // Fake player
			player.health = 100;                       // Alive player
			junk.tag = (short)mo->state->misc2;        // Sector tag for linedef
			if (!P_UseSpecialLine(mo, &junk, 0))       // Try using it
			    P_CrossSpecialLine(&junk, 0, mo);        // Try crossing it
			if (!junk.special)                         // If type cleared,
			    mo->intflags |= MIF_LINEDONE;            // no more for this thing
			mo->player = oldplayer;                    // Restore player status
		}
	}
	*/
}

VERSION_CONTROL (p_enemy_cpp, "$Id$")
