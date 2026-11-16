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
//		AActor flags
//
//-----------------------------------------------------------------------------

#pragma once

#include "flags.h"
#include "doomtype.h" // for BIT

//
// Misc. mobj flags
//
enum class mobjflag_t : uint32_t
{
	// --- mobj.flags ---

	MF_SPECIAL       =        0x1, // call P_SpecialThing when touched
	MF_SOLID         =        0x2,
	MF_SHOOTABLE     =        0x4,
	MF_NOSECTOR      =        0x8, // don't use the sector links (invisible but touchable)

	MF_NOBLOCKMAP    =       0x10, // don't use the blocklinks (inert but displayable)
	MF_AMBUSH        =       0x20, // not activated by sound; deaf monster
	MF_JUSTHIT       =       0x40, // try to attack right back
	MF_JUSTATTACKED  =       0x80, // take at least one step before attacking

	MF_SPAWNCEILING  =      0x100, // hang from ceiling instead of floor
	MF_NOGRAVITY     =      0x200, // don't apply gravity every tic

	// movement flags
	MF_DROPOFF       =      0x400, // allow jumps from high places
	MF_PICKUP        =      0x800, // for players to pick up items

	MF_NOCLIP        =     0x1000, // player cheat
	MF_SLIDE         =     0x2000, // keep info about sliding along walls
	MF_FLOAT         =     0x4000, // allow moves to any height, no gravity
	MF_TELEPORT      =     0x8000, // don't cross lines or look at heights

	MF_MISSILE       =    0x10000, // don't hit same species, explode on block
	MF_DROPPED       =    0x20000, // dropped by a demon, not level spawned
	MF_SHADOW        =    0x40000, // actor is hard for monsters to see
	MF_NOBLOOD       =    0x80000, // don't bleed when shot (use puff)

	MF_CORPSE        =   0x100000, // don't stop moving halfway off a step
	MF_INFLOAT       =   0x200000, // floating to a height for a move, don't auto float to target's height

	MF_COUNTKILL     =   0x400000, // count towards intermission kill total
	MF_COUNTITEM     =   0x800000, // count towards intermission item total

	MF_SKULLFLY      =  0x1000000, // skull in flight
	MF_NOTDMATCH     =  0x2000000, // don't spawn in death match (key cards)

	MF_TRANSLATION1  =  0x4000000,
	MF_TRANSLATION2  =  0x8000000,

	// Player sprites in multiplayer modes are modified
	//  using an internal color lookup table for re-indexing.
	// If 0x4 0x8 or 0xc, use a translation table for player colormaps
	MF_TRANSLATION  = MF_TRANSLATION1 | MF_TRANSLATION2,

	MF_TOUCHY       = 0x10000000, // MBF
	MF_BOUNCES      = 0x20000000, // MBF
	MF_FRIEND       = 0x40000000, // MBF

	MF_TRANSLUCENT  = 0x80000000,
	// if modifying this enum (which almost certainly shouldn't happen), make sure to update the
	// enable_bitflag_operators below to return the highest bit variant
};

using enum mobjflag_t;
constexpr mobjflag_t enable_bitflag_operators(mobjflag_t) { return MF_TRANSLUCENT; }
using ActorFlags1 = OFlags<mobjflag_t>;

enum class mobjflag2_t : uint32_t
{

	// --- mobj.flags2 ---
	// Heretic flags
	MF2_LOGRAV            =        0x1, // alternate gravity setting
	MF2_WINDTHRUST        =        0x2, // gets pushed around by the wind specials
	MF2_FLOORBOUNCE       =        0x4, // bounces off the floor
	MF2_BLASTED           =        0x8, // mobj can be on an edge because it was hit by a blast

	MF2_FLY               =       0x10, // fly mode is active
	MF2_FLOORCLIP         =       0x20, // if feet are allowed to be clipped
	MF2_SPAWNFLOAT        =       0x40, // spawn random float z
	MF2_NOTELEPORT        =       0x80, // does not teleport

	MF2_RIP               =      0x100, // missile rips through solid targets
	MF2_PUSHABLE          =      0x200, // can be pushed by other moving mobjs
	MF2_SLIDE             =      0x400, // slides against walls
	MF2_ONMOBJ            =      0x800, // mobj is resting on top of another mobj

	MF2_PASSMOBJ          =     0x1000, // Enable z block checking.  If on, this flag will allow the mobj to pass over/under other mobjs.
	MF2_CANNOTPUSH        =     0x2000, // cannot push other pushable mobjs
	MF2_THRUGHOST         =     0x4000, // missile will pass through ghosts [RH] was 8
	MF2_BOSS              =     0x8000, // mobj is a major boss

	MF2_FIREDAMAGE        =    0x10000, // does fire damage
	MF2_NODMGTHRUST       =    0x20000, // does not thrust target when damaging
	MF2_TELESTOMP         =    0x40000, // mobj can stomp another
	MF2_FLOATBOB          =    0x80000, // use float bobbing z movement

	MF2_DONTDRAW          =   0x100000, // don't generate a vissprite
	MF2_IMPACT            =   0x200000, // an MF_MISSILE mobj can activate SPAC_IMPACT
	MF2_PUSHWALL          =   0x400000, // mobj can push walls
	MF2_MCROSS            =   0x800000, // can activate monster cross lines

	MF2_PCROSS            =  0x1000000, // can activate projectile cross lines
	MF2_CANTLEAVEFLOORPIC =  0x2000000, // stay within a certain floor type
	MF2_NONSHOOTABLE      =  0x4000000, // mobj is totally non-shootable, but still considered solid
	MF2_INVULNERABLE      =  0x8000000, // mobj is invulnerable

	MF2_DORMANT           = 0x10000000, // thing is dormant
	MF2_ICEDAMAGE         = 0x20000000, // does ice damage
	MF2_SEEKERMISSILE     = 0x40000000, // is a seeker (for reflection)
	MF2_REFLECTIVE        = 0x80000000, // reflects missiles
	// if modifying this enum (which almost certainly shouldn't happen), make sure to update the
	// enable_bitflag_operators below to return the highest bit variant
};

using enum mobjflag2_t;
constexpr mobjflag2_t enable_bitflag_operators(mobjflag2_t) { return MF2_REFLECTIVE; }
using ActorFlags2 = OFlags<mobjflag2_t>;

enum class mobjflag3_t : uint32_t
{
	// --- mobj.flags3 ---
	// MBF21-specific flags
	MF3_RESERVED0      =        0x1,  // BIT0 will be MF2_LOGRAV
	MF3_SHORTMRANGE    =        0x2,  // has short missile range (archvile)
	MF3_DMGIGNORED     =        0x4,  // other things ignore its attacks (archvile)
	MF3_NORADIUSDMG    =        0x8,  // doesn't take splash damage

	MF3_FORCERADIUSDMG =       0x10,  // does radius damage to everything, no exceptions
	MF3_HIGHERMPROB    =       0x20,  // min prob. of miss. att. = 37.5% vs 22%
	MF3_RANGEHALF      =       0x40,  // use half actual distance for missile attack probability
	MF3_NOTHRESHOLD    =       0x80,  // has no targeting threshold (archvile)

	MF3_LONGMELEE      =      0x100,  // long melee range
	MF3_RESERVED9      =      0x200, // BIT 9 is MF2_BOSS -- RESERVED
	MF3_MAP07BOSS1     =      0x400, // is a MAP07 boss type 1 (666)
	MF3_MAP07BOSS2     =      0x800, // is a MAP07 boss type 2 (667)

	MF3_E1M8BOSS       =     0x1000, // is an E1M8 boss
	MF3_E2M8BOSS       =     0x2000, // is an E1M8 boss
	MF3_E3M8BOSS       =     0x4000, // is an E3M8 boss
	MF3_E4M6BOSS       =     0x8000, // is an E4M6 boss

	MF3_E4M8BOSS       =    0x10000, // is an E4M8 boss
	MF3_RESERVED17     =    0x20000, // BIT 17 is MF2_RIP -- RESERVED
	MF3_FULLVOLSOUNDS  =    0x40000, // full volume see / death sound
	// if modifying this enum, make sure to update the
	// enable_bitflag_operators below to return the highest bit variant
};

using enum mobjflag3_t;
constexpr mobjflag3_t enable_bitflag_operators(mobjflag3_t) { return MF3_FULLVOLSOUNDS; }
using ActorFlags3 = OFlags<mobjflag3_t>;

enum class mobjoflag_t : uint32_t
{
	// --- mobj.oflags ---
	// Odamex-specific flags
	MFO_NOSNAPZ             =        0x1,   // [clientside only] ignore snapshot z this tic
	MFO_HEALTHPOOL          =        0x2,   // global health pool that tracks killed HP
	MFO_INFIGHTINVUL        =        0x4,   // invulnerable to infighting
	MFO_UNFLINCHING         =        0x8,   // monster flinching reduced to 1 in 256

	MFO_ARMOR               =       0x10,   // damage taken by monster is reduced
	MFO_QUICK               =       0x20,   // speed of monster is increased
	MFO_NORAISE             =       0x40,   // vile can't raise corpse
	MFO_ISHORDEBOSS         =       0x80,   // Is a horde boss?  Implies that damage subtracts from boss health pool

	MFO_FULLBRIGHT          =      0x100,   // monster is fullbright
	MFO_SPECTATOR           =      0x200,   // GhostlyDeath -- thing is/was a spectator and can't be seen!
	MFO_FALLING             =      0x400,   // [INTERNAL] for falling
	MFO_ARMED               =      0x800,   // [INTERNAL] for TOUCHY (object is armed)

	MFO_LINEDONE            =     0x1000,   // [INTERNAL] for A_LineEffect, line special already done
	// MFO_STEALTH          =     0x2000,   // Andy Baker's stealth monsters
	MFO_ISONCONVEYOR        =     0x4000,   // [INTERNAL] Mobj is in motion due to being carried by a sector
	MFO_MOVESLIKEAMONSTER   =     0x8000,   // [INTERNAL] Mobj has been updated through monster movement routines

	MFO_ISAWAITINGSPAWN     =    0x10000,   // [INTERNAL] Mobj is a temporary mobj for a client-side player object awaiting its SpawnPlayer message
	// if modifying this enum, make sure to update the
	// enable_bitflag_operators below to return the highest bit variant
};

using enum mobjoflag_t;
constexpr mobjoflag_t enable_bitflag_operators(mobjoflag_t) { return MFO_ISAWAITINGSPAWN; }
using ActorOFlags = OFlags<mobjoflag_t>;

//
// Status flags
// Flags to set when you want to indicate the status of a player to other players
// Like powerups, lagging, etc
//
enum statusflag_t
{
	SF_INVULN   =        0x1,
	SF_BERSERK  =        0x2,
	SF_IRONFEET =        0x4,
	SF_INVIS    =        0x8,

	SF_ALLMAP   =       0x10,
	SF_INFRARED =       0x20,
};
