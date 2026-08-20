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

//
// Misc. mobj flags
//
enum class mobjflag_t : uint32_t
{
	// --- mobj.flags ---

	MF_SPECIAL		= BIT(0),	// call P_SpecialThing when touched
	MF_SOLID		= BIT(1),
	MF_SHOOTABLE	= BIT(2),
	MF_NOSECTOR		= BIT(3),	// don't use the sector links
								// (invisible but touchable)
	MF_NOBLOCKMAP	= BIT(4),	// don't use the blocklinks
								// (inert but displayable)
	MF_AMBUSH		= BIT(5),	// not activated by sound; deaf monster
	MF_JUSTHIT		= BIT(6),	// try to attack right back
	MF_JUSTATTACKED = BIT(7),	// take at least one step before attacking
	MF_SPAWNCEILING = BIT(8),	// hang from ceiling instead of floor
	MF_NOGRAVITY	= BIT(9),	// don't apply gravity every tic

	// movement flags
	MF_DROPOFF	= BIT(10),		// allow jumps from high places
	MF_PICKUP	= BIT(11),		// for players to pick up items
	MF_NOCLIP	= BIT(12),		// player cheat
	MF_SLIDE	= BIT(13),		// keep info about sliding along walls
	MF_FLOAT	= BIT(14),		// allow moves to any height, no gravity
	MF_TELEPORT = BIT(15),		// don't cross lines or look at heights
	MF_MISSILE	= BIT(16),		// don't hit same species, explode on block

	MF_DROPPED	= BIT(17),		// dropped by a demon, not level spawned
	MF_SHADOW	= BIT(18),		// actor is hard for monsters to see
	MF_NOBLOOD	= BIT(19),		// don't bleed when shot (use puff)
	MF_CORPSE	= BIT(20),		// don't stop moving halfway off a step
	MF_INFLOAT	= BIT(21),		// floating to a height for a move, don't
								// auto float to target's height

	MF_COUNTKILL = BIT(22),		// count towards intermission kill total
	MF_COUNTITEM = BIT(23),		// count towards intermission item total

	MF_SKULLFLY  = BIT(24),		// skull in flight
	MF_NOTDMATCH = BIT(25),		// don't spawn in death match (key cards)

	MF_TRANSLATION1 = BIT(26),
	MF_TRANSLATION2 = BIT(27),

	// Player sprites in multiplayer modes are modified
	//  using an internal color lookup table for re-indexing.
	// If 0x4 0x8 or 0xc, use a translation table for player colormaps
	MF_TRANSLATION = MF_TRANSLATION1 | MF_TRANSLATION2,

	MF_TOUCHY  = BIT(28), // MBF
	MF_BOUNCES = BIT(29), // MBF
	MF_FRIEND  = BIT(30), // MBF

	MF_TRANSLUCENT = BIT(31),
};

using enum mobjflag_t;

consteval mobjflag_t enable_bitflag_operators(mobjflag_t) { return MF_TRANSLUCENT; }

using ActorFlags1 = OFlags<mobjflag_t>;