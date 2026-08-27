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

	MF_SPECIAL       = BIT(0), // call P_SpecialThing when touched
	MF_SOLID         = BIT(1),
	MF_SHOOTABLE     = BIT(2),
	MF_NOSECTOR      = BIT(3), // don't use the sector links
	                           // (invisible but touchable)
	MF_NOBLOCKMAP    = BIT(4), // don't use the blocklinks
	                           // (inert but displayable)
	MF_AMBUSH        = BIT(5), // not activated by sound; deaf monster
	MF_JUSTHIT       = BIT(6), // try to attack right back
	MF_JUSTATTACKED  = BIT(7), // take at least one step before attacking
	MF_SPAWNCEILING  = BIT(8), // hang from ceiling instead of floor
	MF_NOGRAVITY     = BIT(9), // don't apply gravity every tic

	// movement flags
	MF_DROPOFF       = BIT(10), // allow jumps from high places
	MF_PICKUP        = BIT(11), // for players to pick up items
	MF_NOCLIP        = BIT(12), // player cheat
	MF_SLIDE         = BIT(13), // keep info about sliding along walls
	MF_FLOAT         = BIT(14), // allow moves to any height, no gravity
	MF_TELEPORT      = BIT(15), // don't cross lines or look at heights
	MF_MISSILE       = BIT(16), // don't hit same species, explode on block

	MF_DROPPED       = BIT(17), // dropped by a demon, not level spawned
	MF_SHADOW        = BIT(18), // actor is hard for monsters to see
	MF_NOBLOOD       = BIT(19), // don't bleed when shot (use puff)
	MF_CORPSE        = BIT(20), // don't stop moving halfway off a step
	MF_INFLOAT       = BIT(21), // floating to a height for a move, don't
	                            // auto float to target's height

	MF_COUNTKILL     = BIT(22), // count towards intermission kill total
	MF_COUNTITEM     = BIT(23), // count towards intermission item total

	MF_SKULLFLY      = BIT(24), // skull in flight
	MF_NOTDMATCH     = BIT(25), // don't spawn in death match (key cards)

	MF_TRANSLATION1  = BIT(26),
	MF_TRANSLATION2  = BIT(27),

	// Player sprites in multiplayer modes are modified
	//  using an internal color lookup table for re-indexing.
	// If 0x4 0x8 or 0xc, use a translation table for player colormaps
	MF_TRANSLATION  = MF_TRANSLATION1 | MF_TRANSLATION2,

	MF_TOUCHY       = BIT(28), // MBF
	MF_BOUNCES      = BIT(29), // MBF
	MF_FRIEND       = BIT(30), // MBF

	MF_TRANSLUCENT  = BIT(31),
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
	MF2_LOGRAV            = BIT(0), // alternate gravity setting
	MF2_WINDTHRUST        = BIT(1), // gets pushed around by the wind
	                                // specials
	MF2_FLOORBOUNCE       = BIT(2), // bounces off the floor
	MF2_BLASTED           = BIT(3), // mobj can be on an edge because it was hit by a blast
	MF2_FLY               = BIT(4), // fly mode is active
	MF2_FLOORCLIP         = BIT(5), // if feet are allowed to be clipped
	MF2_SPAWNFLOAT        = BIT(6), // spawn random float z
	MF2_NOTELEPORT        = BIT(7), // does not teleport
	MF2_RIP               = BIT(8), // missile rips through solid
	                                 // targets
	MF2_PUSHABLE          = BIT(9),  // can be pushed by other moving
	                                 // mobjs
	MF2_SLIDE             = BIT(10), // slides against walls
	MF2_ONMOBJ            = BIT(11), // mobj is resting on top of another
	                                 // mobj
	MF2_PASSMOBJ          = BIT(12), // Enable z block checking.  If on,
	                                 // this flag will allow the mobj to
	                                 // pass over/under other mobjs.
	MF2_CANNOTPUSH        = BIT(13), // cannot push other pushable mobjs
	MF2_THRUGHOST         = BIT(14), // missile will pass through ghosts [RH] was 8
	MF2_BOSS              = BIT(15), // mobj is a major boss
	MF2_FIREDAMAGE        = BIT(16), // does fire damage
	MF2_NODMGTHRUST       = BIT(17), // does not thrust target when damaging
	MF2_TELESTOMP         = BIT(18), // mobj can stomp another
	MF2_FLOATBOB          = BIT(19), // use float bobbing z movement
	MF2_DONTDRAW          = BIT(20), // don't generate a vissprite
	MF2_IMPACT            = BIT(21), // an MF_MISSILE mobj can activate SPAC_IMPACT
	MF2_PUSHWALL          = BIT(22), // mobj can push walls
	MF2_MCROSS            = BIT(23), // can activate monster cross lines
	MF2_PCROSS            = BIT(24), // can activate projectile cross lines
	MF2_CANTLEAVEFLOORPIC = BIT(25), // stay within a certain floor type
	MF2_NONSHOOTABLE      = BIT(26), // mobj is totally non-shootable,
	                                 // but still considered solid
	MF2_INVULNERABLE      = BIT(27), // mobj is invulnerable
	MF2_DORMANT           = BIT(28), // thing is dormant
	MF2_ICEDAMAGE         = BIT(29), // does ice damage
	MF2_SEEKERMISSILE     = BIT(30), // is a seeker (for reflection)
	MF2_REFLECTIVE        = BIT(31), // reflects missiles
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
	                              // BIT0 will be MF2_LOGRAV
	MF3_SHORTMRANGE    = BIT(1),  // has short missile range (archvile)
	MF3_DMGIGNORED     = BIT(2),  // other things ignore its attacks (archvile)
	MF3_NORADIUSDMG    = BIT(3),  // doesn't take splash damage
	MF3_FORCERADIUSDMG = BIT(4),  // does radius damage to everything, no exceptions
	MF3_HIGHERMPROB    = BIT(5),  // min prob. of miss. att. = 37.5% vs 22%
	MF3_RANGEHALF      = BIT(6),  // use half actual distance for missile attack probability
	MF3_NOTHRESHOLD    = BIT(7),  // has no targeting threshold (archvile)
	MF3_LONGMELEE      = BIT(8),  // long melee range
	                              // BIT 9 is MF2_BOSS -- RESERVED
	MF3_MAP07BOSS1     = BIT(10), // is a MAP07 boss type 1 (666)
	MF3_MAP07BOSS2     = BIT(11), // is a MAP07 boss type 2 (667)
	MF3_E1M8BOSS       = BIT(12), // is an E1M8 boss
	MF3_E2M8BOSS       = BIT(13), // is an E1M8 boss
	MF3_E3M8BOSS       = BIT(14), // is an E3M8 boss
	MF3_E4M6BOSS       = BIT(15), // is an E4M6 boss
	MF3_E4M8BOSS       = BIT(16), // is an E4M8 boss
	                              // BIT 17 is MF2_RIP -- RESERVED
	MF3_FULLVOLSOUNDS  = BIT(18), // full volume see / death sound
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
	SF_INVULN   = BIT(0),
	SF_BERSERK  = BIT(1),
	SF_IRONFEET = BIT(2),
	SF_INVIS    = BIT(3),
	SF_ALLMAP   = BIT(4),
	SF_INFRARED = BIT(5),
};
