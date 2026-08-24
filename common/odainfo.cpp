// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
// Copyright (C) 2024-2025 by Christian Bernard.
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
//  info.cpp for odamex/zdoom/ctf stuff
//
//-----------------------------------------------------------------------------

#include "odamex.h"
#include "m_fixed.h"
#include "actor.h"
#include "info.h"

#include "odainfo.h"

// ------------------------------------------------------------------
// Odamex specific States, Sprites, and Things
// ------------------------------------------------------------------

// code pointers
void A_Lower(AActor*);
void A_Raise(AActor*);
void A_WeaponReady(AActor*);
void A_Ambient(AActor*);

// reserved odamex states
state_t odastates[] = {
	// ZDoom/Odamex stuff starts here
	{S_GIB0, SPR_GIB0, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB0
	{S_GIB1, SPR_GIB1, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB1
	{S_GIB2, SPR_GIB2, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB2
	{S_GIB3, SPR_GIB3, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB3
	{S_GIB4, SPR_GIB4, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB4
	{S_GIB5, SPR_GIB5, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB5
	{S_GIB6, SPR_GIB6, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB6
	{S_GIB7, SPR_GIB7, 0, -1, NULL, S_NULL, 0, 0},                     // S_GIB7
	{S_AMBIENTSOUND, SPR_TROO, 0, 1, A_Ambient, S_AMBIENTSOUND, 0, 0}, // S_AMBIENTSOUND
	{S_UNKNOWNTHING, SPR_UNKN, 0, -1, NULL, S_NULL, 0, 0},             // S_UNKNOWNTHING

	//	[Toke - CTF]
	{S_BSOK, SPR_BSOK, 0, -1, NULL, S_NULL, 0, 0},      // Blue Socket
	{S_RSOK, SPR_RSOK, 0, -1, NULL, S_NULL, 0, 0},      // Red Socket
	{S_BFLG, SPR_BFLG, 32768, 4, NULL, S_BFLG2, 0, 0},  // BLUE Flag Animation; S_BFLG
	{S_BFLG2, SPR_BFLG, 32769, 4, NULL, S_BFLG3, 0, 0}, // S_BFLG2
	{S_BFLG3, SPR_BFLG, 32770, 4, NULL, S_BFLG4, 0, 0}, // S_BFLG3
	{S_BFLG4, SPR_BFLG, 32771, 4, NULL, S_BFLG5, 0, 0}, // S_BFLG4
	{S_BFLG5, SPR_BFLG, 32772, 4, NULL, S_BFLG6, 0, 0}, // S_BFLG5
	{S_BFLG6, SPR_BFLG, 32773, 4, NULL, S_BFLG7, 0, 0}, // S_BFLG6
	{S_BFLG7, SPR_BFLG, 32774, 4, NULL, S_BFLG8, 0, 0}, // S_BFLG7
	{S_BFLG8, SPR_BFLG, 32775, 4, NULL, S_BFLG, 0, 0},  // S_BFLG8
	{S_RFLG, SPR_RFLG, 32768, 4, NULL, S_RFLG2, 0, 0},  // RED Flag Animation; S_RFLG
	{S_RFLG2, SPR_RFLG, 32769, 4, NULL, S_RFLG3, 0, 0}, // S_RFLG2
	{S_RFLG3, SPR_RFLG, 32770, 4, NULL, S_RFLG4, 0, 0}, // S_RFLG3
	{S_RFLG4, SPR_RFLG, 32771, 4, NULL, S_RFLG5, 0, 0}, // S_RFLG4
	{S_RFLG5, SPR_RFLG, 32772, 4, NULL, S_RFLG6, 0, 0}, // S_RFLG5
	{S_RFLG6, SPR_RFLG, 32773, 4, NULL, S_RFLG7, 0, 0}, // S_RFLG6
	{S_RFLG7, SPR_RFLG, 32774, 4, NULL, S_RFLG8, 0, 0}, // S_RFLG7
	{S_RFLG8, SPR_RFLG, 32775, 4, NULL, S_RFLG, 0, 0},  // S_RFLG8
	{S_BDWN, SPR_BDWN, 0, -1, NULL, S_NULL, 0, 0},      // Blue Dropped Flag; S_BDWN
	{S_RDWN, SPR_RDWN, 0, -1, NULL, S_NULL, 0, 0},      // Red Dropped Flag; S_RDWN
	{S_BCAR, SPR_BCAR, 0, -1, NULL, S_NULL, 0, 0},      // Blue Dropped Flag; S_BCAR
	{S_RCAR, SPR_RCAR, 0, -1, NULL, S_NULL, 0, 0},      // Red Dropped Flag; S_RCAR

	{S_GSOK, SPR_GSOK, 0, -1, NULL, S_NULL, 0, 0},      // S_GSOK,
	{S_GFLG, SPR_GFLG, 32768, 4, NULL, S_GFLG2, 0, 0},  // Green Flag Animation; S_GFLG
	{S_GFLG2, SPR_GFLG, 32769, 4, NULL, S_GFLG3, 0, 0}, // S_GFLG2
	{S_GFLG3, SPR_GFLG, 32770, 4, NULL, S_GFLG4, 0, 0}, // S_GFLG3
	{S_GFLG4, SPR_GFLG, 32771, 4, NULL, S_GFLG5, 0, 0}, // S_GFLG4
	{S_GFLG5, SPR_GFLG, 32772, 4, NULL, S_GFLG6, 0, 0}, // S_GFLG5
	{S_GFLG6, SPR_GFLG, 32773, 4, NULL, S_GFLG7, 0, 0}, // S_GFLG6
	{S_GFLG7, SPR_GFLG, 32774, 4, NULL, S_GFLG8, 0, 0}, // S_GFLG7
	{S_GFLG8, SPR_GFLG, 32775, 4, NULL, S_GFLG, 0, 0},  // S_GFLG8
	{S_GDWN, SPR_GDWN, 0, -1, NULL, S_NULL, 0, 0},      // S_GDWN,
	{S_GCAR, SPR_GCAR, 0, -1, NULL, S_NULL, 0, 0},      // S_GCAR,

	{S_BRIDGE1, SPR_TLGL, 32768, 4, NULL, S_BRIDGE2, 0, 0}, // S_BRIDGE1
	{S_BRIDGE2, SPR_TLGL, 32769, 4, NULL, S_BRIDGE3, 0, 0}, // S_BRIDGE2
	{S_BRIDGE3, SPR_TLGL, 32770, 4, NULL, S_BRIDGE4, 0, 0}, // S_BRIDGE3
	{S_BRIDGE4, SPR_TLGL, 32771, 4, NULL, S_BRIDGE5, 0, 0}, // S_BRIDGE4
	{S_BRIDGE5, SPR_TLGL, 32772, 4, NULL, S_BRIDGE1, 0, 0}, // S_BRIDGE5

	{S_WPBF1, SPR_WPBF, 0, 1, NULL, S_WPBF2, 0, 0}, // S_WPBF1 - Waypoint Blue Flag
	{S_WPBF2, SPR_WPBF, 1, 1, NULL, S_WPBF1, 0, 0}, // S_WPBF2
	{S_WPRF1, SPR_WPRF, 0, 1, NULL, S_WPRF2, 0, 0}, // S_WPRF1 - Waypoint Red Flag
	{S_WPRF2, SPR_WPRF, 1, 1, NULL, S_WPRF1, 0, 0}, // S_WPRF2
	{S_WPGF1, SPR_WPGF, 0, 1, NULL, S_WPGF2, 0, 0}, // S_WPGF1 - Waypoint Green Flag
	{S_WPGF2, SPR_WPGF, 1, 1, NULL, S_WPGF1, 0, 0}, // S_WPGF2

	{S_CARE, SPR_CARE, 0, -1, NULL, S_NULL, 0, 0},  // S_CARE - Horde Care Package
	{S_O1UP,  SPR_O1UP, 32768, 4, NULL, S_O1UP2, 0, 0},  // S_O1UP - Horde Extra Life Powerup
	{S_O1UP2, SPR_O1UP, 32769, 4, NULL, S_O1UP3, 0, 0},
	{S_O1UP3, SPR_O1UP, 32770, 4, NULL, S_O1UP4, 0, 0},
	{S_O1UP4, SPR_O1UP, 32771, 4, NULL, S_O1UP5, 0, 0},
	{S_O1UP5, SPR_O1UP, 32770, 4, NULL, S_O1UP6, 0, 0},
	{S_O1UP6, SPR_O1UP, 32769, 4, NULL, S_O1UP, 0, 0},
	{S_RES,  SPR_RSTM, 32768, 5, NULL, S_RES2, 0, 0},   // S_RES - Horde Resurrect Powerup
	{S_RES2, SPR_RSTM, 32769, 5, NULL, S_RES3, 0, 0},
	{S_RES3, SPR_RSTM, 32770, 5, NULL, S_RES4, 0, 0},
	{S_RES4, SPR_RSTM, 32771, 5, NULL, S_RES, 0, 0},

	{S_NOWEAPONUP, SPR_TNT1, 0, 1, A_Raise, S_NOWEAPON, 0, 0},     // S_NOWEAPONUP
	{S_NOWEAPONDOWN, SPR_TNT1, 0, 1, A_Lower, S_NOWEAPON, 0, 0},   // S_NOWEAPONDOWN
	{S_NOWEAPON, SPR_TNT1, 0, 1, A_WeaponReady, S_NOWEAPON, 0, 0}, // S_NOWEAPON

	{S_STGRENADE, SPR_GREN, 32768, 1, NULL, S_STGRENADE, 0, 0}, // S_STGRENADE - Skulltag grenade

	{S_TURB, SPR_TURB, 32768, 6, NULL, S_TURB2, 0, 0},          // S_TURB - Turbosphere
	{S_TURB2, SPR_TURB, 32769, 6, NULL, S_TURB3, 0, 0},         // S_TURB2
	{S_TURB3, SPR_TURB, 32770, 6, NULL, S_TURB4, 0, 0},         // S_TURB3
	{S_TURB4, SPR_TURB, 32771, 6, NULL, S_TURB, 0, 0},          // S_TURB4
	{S_FREEZE, SPR_TIME, 32768, 6, NULL, S_FREEZE2, 0, 0},      // S_FREEZE - Time freeze sphere
	{S_FREEZE2, SPR_TIME, 32769, 6, NULL, S_FREEZE3, 0, 0},     // S_FREEZE2
	{S_FREEZE3, SPR_TIME, 32770, 6, NULL, S_FREEZE4, 0, 0},     // S_FREEZE3
	{S_FREEZE4, SPR_TIME, 32771, 6, NULL, S_FREEZE, 0, 0},      // S_FREEZE4
	{S_INVS, SPR_INVS, 32768, 6, NULL, S_INVS2, 0, 0},          // S_INVS - Invisibility sphere
	{S_INVS2, SPR_INVS, 32769, 6, NULL, S_INVS3, 0, 0},         // S_INVS2
	{S_INVS3, SPR_INVS, 32770, 6, NULL, S_INVS4, 0, 0},         // S_INVS3
	{S_INVS4, SPR_INVS, 32771, 6, NULL, S_INVS, 0, 0},          // S_INVS4
	{S_DOOMSPH, SPR_DOOM, 32768, 10, NULL, S_DOOMSPH2, 0, 0},   // S_DOOMSPH - Doomsphere
	{S_DOOMSPH2, SPR_DOOM, 32769, 15, NULL, S_DOOMSPH3, 0, 0},  // S_DOOMSPH2
	{S_DOOMSPH3, SPR_DOOM, 32770, 8, NULL, S_DOOMSPH4, 0, 0},   // S_DOOMSPH3
	{S_DOOMSPH4, SPR_DOOM, 32771, 6, NULL, S_DOOMSPH, 0, 0},    // S_DOOMSPH4

	{S_RUN1, SPR_RUN1, 0, 6, NULL, S_RUN1A, 0, 0},       // S_RUN1 - Strength
	{S_RUN1A, SPR_RUN1, 1, 6, NULL, S_RUN1B, 0, 0},      // S_RUN1A
	{S_RUN1B, SPR_RUN1, 2, 6, NULL, S_RUN1C, 0, 0},      // S_RUN1B
	{S_RUN1C, SPR_RUN1, 3, 6, NULL, S_RUN1D, 0, 0},      // S_RUN1C
	{S_RUN1D, SPR_RUN1, 4, 6, NULL, S_RUN1, 0, 0},       // S_RUN1D
	{S_RUN2, SPR_RUN2, 0, 6, NULL, S_RUN2A, 0, 0},       // S_RUN2 - Rage
	{S_RUN2A, SPR_RUN2, 1, 6, NULL, S_RUN2, 0, 0},       // S_RUN2A
	{S_RUN3, SPR_RUN3, 32768, 6, NULL, S_RUN3A, 0, 0},   // S_RUN3 - Drain
	{S_RUN3A, SPR_RUN3, 1, 6, NULL, S_RUN3, 0, 0},       // S_RUN3A
	{S_RUN4, SPR_RUN4, 0, 6, NULL, S_RUN4A, 0, 0},       // S_RUN4 - Resistance
	{S_RUN4A, SPR_RUN4, 1, 6, NULL, S_RUN4B, 0, 0},      // S_RUN4A
	{S_RUN4B, SPR_RUN4, 2, 6, NULL, S_RUN4C, 0, 0},      // S_RUN4B
	{S_RUN4C, SPR_RUN4, 3, 6, NULL, S_RUN4D, 0, 0},      // S_RUN4C
	{S_RUN4D, SPR_RUN4, 4, 6, NULL, S_RUN4E, 0, 0},      // S_RUN4D
	{S_RUN4E, SPR_RUN4, 5, 6, NULL, S_RUN4F, 0, 0},      // S_RUN4E
	{S_RUN4F, SPR_RUN4, 6, 6, NULL, S_RUN4G, 0, 0},      // S_RUN4F
	{S_RUN4G, SPR_RUN4, 7, 6, NULL, S_RUN4, 0, 0},       // S_RUN4G
	{S_RUN5, SPR_RUN5, 32768, 6, NULL, S_RUN5A, 0, 0},   // S_RUN5 - Regeneration
	{S_RUN5A, SPR_RUN5, 1, 6, NULL, S_RUN5, 0, 0},       // S_RUN5A
	{S_RUN6, SPR_RUN6, 0, 6, NULL, S_RUN6A, 0, 0},       // S_RUN6 - Prosperity
	{S_RUN6A, SPR_RUN6, 1, 6, NULL, S_RUN6B, 0, 0},      // S_RUN6A
	{S_RUN6B, SPR_RUN6, 2, 6, NULL, S_RUN6C, 0, 0},      // S_RUN6B
	{S_RUN6C, SPR_RUN6, 3, 6, NULL, S_RUN6D, 0, 0},      // S_RUN6C
	{S_RUN6D, SPR_RUN6, 4, 6, NULL, S_RUN6, 0, 0},       // S_RUN6D
	{S_RUN7, SPR_RUN7, 0, 6, NULL, S_RUN7A, 0, 0},       // S_RUN7 - High jump
	{S_RUN7A, SPR_RUN7, 1, 6, NULL, S_RUN7B, 0, 0},      // S_RUN7A
	{S_RUN7B, SPR_RUN7, 2, 6, NULL, S_RUN7C, 0, 0},      // S_RUN7B
	{S_RUN7C, SPR_RUN7, 3, 6, NULL, S_RUN7, 0, 0},       // S_RUN7C
	{S_RUN8, SPR_RUN8, 32768, 6, NULL, S_RUN8A, 0, 0},   // S_RUN8 - Haste
	{S_RUN8A, SPR_RUN8, 1, 6, NULL, S_RUN8B, 0, 0},      // S_RUN8A
	{S_RUN8B, SPR_RUN8, 32770, 6, NULL, S_RUN8C, 0, 0},  // S_RUN8B
	{S_RUN8C, SPR_RUN8, 3, 6, NULL, S_RUN8, 0, 0},       // S_RUN8C
	{S_RUN9, SPR_RUN9, 32768, 6, NULL, S_RUN9A, 0, 0},   // S_RUN9 - Spread
	{S_RUN9A, SPR_RUN9, 1, 6, NULL, S_RUN9B, 0, 0},      // S_RUN9A
	{S_RUN9B, SPR_RUN9, 32770, 6, NULL, S_RUN9C, 0, 0},  // S_RUN9B
	{S_RUN9C, SPR_RUN9, 3, 6, NULL, S_RUN9, 0, 0},       // S_RUN9C
	{S_RUNB, SPR_RUNB, 0, 6, NULL, S_RUNB2, 0, 0},       // S_RUNB - Reflection
	{S_RUNB2, SPR_RUNB, 1, 6, NULL, S_RUNB3, 0, 0},      // S_RUNB2
	{S_RUNB3, SPR_RUNB, 2, 6, NULL, S_RUNB4, 0, 0},      // S_RUNB3
	{S_RUNB4, SPR_RUNB, 3, 6, NULL, S_RUNB, 0, 0},       // S_RUNB4
};

// reserved odamex sprites
// ::SPR_CARE - ::SPR_GIB0 + 2
const char* odasprnames[] = {
	"GIB0", "GIB1", "GIB2", "GIB3", "GIB4", "GIB5", "GIB6", "GIB7", "UNKN",
	//	[Toke - CTF]
	"BSOK", "RSOK", "BFLG", "RFLG", "BDWN", "RDWN", "BCAR", "RCAR", "GSOK", "GFLG",
	"GDWN", "GCAR", "TLGL", "WPBF", "WPRF", "WPGF", "CARE", "O1UP", "RSTM",
	// Skulltag grenades, powerup spheres, and runes
	"GREN", "TURB", "TIME", "INVS", "DOOM",
	"RUN1", "RUN2", "RUN3", "RUN4", "RUN5",
	"RUN6", "RUN7", "RUN8", "RUN9", "RUNB",};

// reserved odamex mobjinfo
// ::MT_CAREPACK - ::MT_GIB0 + 1
// this table *is* the constants for those numbers
// NOLINTBEGIN(readability-magic-numbers)
mobjinfo_t odamobjinfo[] = {
	// ------------ odamex things start ------------ //
	{
		//  MT_GIB0
		.type             = MT_GIB0,
		.spawnstate       = S_GIB0,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB0",
	},

	{
		//  MT_GIB1
		.type             = MT_GIB1,
		.spawnstate       = S_GIB1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB1",
	},

	{
		//  MT_GIB2
		.type             = MT_GIB2,
		.spawnstate       = S_GIB2,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB2",
	},

	{
		//  MT_GIB3
		.type             = MT_GIB3,
		.spawnstate       = S_GIB3,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB3",
	},

	{
		//  MT_GIB4
		.type             = MT_GIB4,
		.spawnstate       = S_GIB4,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB4",
	},

	{
		//  MT_GIB5
		.type             = MT_GIB5,
		.spawnstate       = S_GIB5,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB5",
	},

	{
		//  MT_GIB6
		.type             = MT_GIB6,
		.spawnstate       = S_GIB6,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB6",
	},

	{
		//  MT_GIB7
		.type             = MT_GIB7,
		.spawnstate       = S_GIB7,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_DROPOFF | MF_CORPSE,
		.name             = "MT_GIB7",
	},

	{
		//  MT_UNKNOWNTHING
		.type             = MT_UNKNOWNTHING,
		.spawnstate       = S_UNKNOWNTHING,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 32_fx,
		.height           = 56_fx,
		.cdheight         = 56_fx,
		.mass             = 100,
		.flags            = MF_NOGRAVITY,
		.name             = "MT_UNKNOWNTHING",
	},

	{
		// [RH] MT_PATHNODE -- used for monster patrols
		.type             = MT_PATHNODE,
		.doomednum        = 9024,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 8,
		.height           = 8,
		.cdheight         = 8_fx,
		.mass             = 10,
		.flags            = MF_NOBLOCKMAP,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_PATHNODE -- used for monster patrols",
	},

	{
		// [RH] MT_AMBIENT (subsumes MT_AMBIENT0-MT_AMBIENT63)
		.type             = MT_AMBIENT,
		.doomednum        = 14065,
		.spawnstate       = S_AMBIENTSOUND,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR,
		.name             = "MT_AMBIENT (subsumes MT_AMBIENT0-MT_AMBIENT63)",
	},

	{
		// [RH] MT_TELEPORTMAN2 - Height-sensitive teleport destination
		.type             = MT_TELEPORTMAN2,
		.doomednum        = 9044,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.name             = "MT_TELEPORTMAN2 - Height-sensitive teleport destination",
	},

	{
		// [RH] MT_CAMERA
		.type             = MT_CAMERA,
		.doomednum        = 9025,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_CAMERA",
	},

	{
		// [RH] MT_SPARK
		.type             = MT_SPARK,
		.doomednum        = 9026,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOSECTOR | MF_NOBLOCKMAP | MF_NOGRAVITY,
		.name             = "MT_SPARK",
	},

	{
		// [RH] MT_FOUNTAIN
		.type             = MT_FOUNTAIN,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 0_fx,
		.cdheight         = 0_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOGRAVITY,
		.name             = "MT_FOUNTAIN",
	},

	{
		//  MT_NODE   // Added by MC.
		.type             = MT_NODE,
		.doomednum        = 786,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOSECTOR | MF_NOGRAVITY, // MF_NOSECTOR makes it invisible
		.name             = "MT_NODE   //Added by MC.",
	},

	{
		//  MT_WATERZONE
		.type             = MT_WATERZONE,
		.doomednum        = 9045,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.name             = "MT_WATERZONE",
	},

	{
		//  MT_SECRETTRIGGER
		.type             = MT_SECRETTRIGGER,
		.doomednum        = 9046,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.name             = "MT_SECRETTRIGGER",
	},

	{
		// MT_UPPERSTACK
		.type             = MT_UPPERSTACK,
		.doomednum        = 9077,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_UPPERSTACK",
	},

	{
		// MT_LOWERSTACK
		.type             = MT_LOWERSTACK,
		.doomednum        = 9078,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_LOWERSTACK",
	},

	{
		// MT_SKYVIEWPOINT
		.type             = MT_SKYVIEWPOINT,
		.doomednum        = 9080,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SKYVIEWPOINT",
	},

	{
		// MT_SKYPICKER
		.type             = MT_SKYPICKER,
		.doomednum        = 9081,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SKYPICKER",
	},

	{
		// MT_SECTORSILENCER
		.type             = MT_SECTORSILENCER,
		.doomednum        = 9082,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECTORSILENCER",
	},

	// [Toke - CTF] Blue Socket
	{
		//  MT_BSOK
		.type             = MT_BSOK,
		.doomednum        = 5130,
		.spawnstate       = S_BSOK,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 14_fx,
		.cdheight         = 14_fx,
		.flags            = MF_SPECIAL,
		.name             = "MT_BSOK",
	},

	// [Toke - CTF] Red Socket
	{
		//  MT_RSOK
		.type             = MT_RSOK,
		.doomednum        = 5131,
		.spawnstate       = S_RSOK,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 14_fx,
		.cdheight         = 14_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_RSOK",
	},

	// Nes - Reserve 5132 for Neutral Socket

	// [Toke - CTF] Blue Flag
	{
		//  MT_BFLG
		.type             = MT_BFLG,
		.doomednum        = -1,
		.spawnstate       = S_BFLG,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 48_fx,
		.cdheight         = 48_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_BFLG",
	},

	// [Toke - CTF] Red Flag
	{
		//  MT_RFLG
		.type             = MT_RFLG,
		.spawnstate       = S_RFLG,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 48_fx,
		.cdheight         = 48_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_RFLG",
	},

	// [Toke - CTF] Blue Dropped Flag
	{
		//  MT_BDWN
		.type             = MT_BDWN,
		.spawnstate       = S_BDWN,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 48_fx,
		.cdheight         = 48_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_BDWN",
	},

	// [Toke - CTF] Red Dropped Flag
	{
		//  MT_RDWN
		.type             = MT_RDWN,
		.spawnstate       = S_RDWN,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 48_fx,
		.cdheight         = 48_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_RDWN",
	},

	// [Toke - CTF] Blue Carrying Flag
	{
		//  MT_BCAR
		.type             = MT_BCAR,
		.spawnstate       = S_BCAR,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 0_fx,
		.height           = 0_fx,
		.cdheight         = 0_fx,
		.mass             = 100,
		.flags            = MF_NOGRAVITY,
		.name             = "MT_BCAR",
	},

	// [Toke - CTF] Red Carrying Flag
	{
		//  MT_RCAR
		.type             = MT_RCAR,
		.spawnstate       = S_RCAR,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 0_fx,
		.height           = 0_fx,
		.cdheight         = 0_fx,
		.mass             = 100,
		.flags            = MF_NOGRAVITY,
		.name             = "MT_RCAR",
	},

	{
		//  MT_BRIDGE
		.type             = MT_BRIDGE,
		.doomednum        = 118,
		.spawnstate       = S_BRIDGE1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 36_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_SOLID | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_BRIDGE",
	},

	{
		//  MT_MAPSPOT
		.type             = MT_MAPSPOT,
		.doomednum        = 9001,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.name             = "MT_MAPSPOT",
	},

	{
		//  MT_MAPSPOTGRAVITY
		.type             = MT_MAPSPOTGRAVITY,
		.doomednum        = 9013,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_MAPSPOTGRAV",
	},

	{
		//  MT_BRIDGE32
		.type             = MT_BRIDGE32,
		.doomednum        = 5061,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 32_fx,
		.height           = 8_fx,
		.cdheight         = 8_fx,
		.mass             = 100,
		.flags            = MF_SOLID | MF_NOGRAVITY,
		.name             = "MT_BRIDGE32",
	},

	{
		//  MT_BRIDGE16
		.type             = MT_BRIDGE16,
		.doomednum        = 5064,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 8_fx,
		.cdheight         = 8_fx,
		.mass             = 100,
		.flags            = MF_SOLID | MF_NOGRAVITY,
		.name             = "MT_BRIDGE16",
	},

	{
		//  MT_BRIDGE8
		.type             = MT_BRIDGE8,
		.doomednum        = 5065,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 8_fx,
		.height           = 8_fx,
		.cdheight         = 8_fx,
		.mass             = 100,
		.flags            = MF_SOLID | MF_NOGRAVITY,
		.name             = "MT_BRIDGE8",
	},

	{
		//  MT_ZDOOMBRIDGE
		.type             = MT_ZDOOMBRIDGE,
		.doomednum        = 9990,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 32_fx,
		.height           = 4_fx,
		.cdheight         = 4_fx,
		.mass             = 100,
		.flags            = MF_SOLID | MF_NOGRAVITY,
		.name             = "MT_ZDOOMBRIDGE",
	},

	{
		//  MT_SECACTENTER
		.type             = MT_SECACTENTER,
		.doomednum        = 9998,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTENTER",
	},

	{
		//  MT_SECACTEXIT
		.type             = MT_SECACTEXIT,
		.doomednum        = 9997,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTEXIT",
	},

	{
		//  MT_SECACTHITFLOOR
		.type             = MT_SECACTHITFLOOR,
		.doomednum        = 9999,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTHITFLOOR",
	},

	{
		//  MT_SECACTHITCEIL
		.type             = MT_SECACTHITCEIL,
		.doomednum        = 9996,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTHITCEIL",
	},

	{
		//  MT_SECACTUSE
		.type             = MT_SECACTUSE,
		.doomednum        = 9995,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTUSE",
	},

	{
		//  MT_SECACTUSEWALL
		.type             = MT_SECACTUSEWALL,
		.doomednum        = 9994,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTUSEWALL",
	},

	{
		//  MT_SECACTEYESDIVE
		.type             = MT_SECACTEYESDIVE,
		.doomednum        = 9993,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTEYESDIVE",
	},

	{
		//  MT_SECACTEYESSURFACE
		.type             = MT_SECACTEYESSURFACE,
		.doomednum        = 9992,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTEYESSURFACE",
	},

	{
		//  MT_SECACTEYESBELOWC
		.type             = MT_SECACTEYESBELOWC,
		.doomednum        = 9983,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTEYESBELOWC",
	},

	{
		//  MT_SECACTEYESABOVEC
		.type             = MT_SECACTEYESABOVEC,
		.doomednum        = 9982,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SECACTEYESABOVEC",
	},

	{
		//  MT_GSOK
		.type             = MT_GSOK,
		.doomednum        = 5133,
		.spawnstate       = S_GSOK,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 14_fx,
		.cdheight         = 14_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_GSOK",
	},

	{
		//  MT_GFLG
		.type             = MT_GFLG,
		.spawnstate       = S_GFLG,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 48_fx,
		.cdheight         = 48_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_GFLG",
	},

	{
		//  MT_GDWN
		.type             = MT_GDWN,
		.spawnstate       = S_GDWN,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 48_fx,
		.cdheight         = 48_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_GDWN",
	},

	{
		//  MT_GCAR
		.type             = MT_GCAR,
		.spawnstate       = S_GCAR,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 0_fx,
		.height           = 0_fx,
		.cdheight         = 0_fx,
		.mass             = 100,
		.flags            = MF_NOGRAVITY,
		.name             = "MT_GCAR",
	},
	{
		// MT_WPBFLAG
		.type             = MT_WPBFLAG,
		.spawnstate       = S_WPBF1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.translucency     = 0xC000,
		.name             = "MT_WPBFLAG",
	},
	{
		// MT_WPRFLAG
		.type             = MT_WPRFLAG,
		.spawnstate       = S_WPRF1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.translucency     = 0xC000,
		.name             = "MT_WPRFLAG"
	},
	{
		// MT_WPGFLAG
		.type             = MT_WPGFLAG,
		.spawnstate       = S_WPGF1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.translucency     = 0xC000,
		.name             = "MT_WPGFLAG"
	},
	{
		// MT_AVATAR
		.type             = MT_AVATAR,
		.spawnstate       = S_PLAY,
		.spawnhealth      = 100,
		.seestate         = S_PLAY_RUN1,
		.painstate        = S_PLAY_PAIN,
		.painchance       = 255,
		.painsound        = "*pain100_1",
		.missilestate     = S_PLAY_ATK1,
		.deathstate       = S_PLAY_DIE1,
		.xdeathstate      = S_PLAY_XDIE1,
		.deathsound       = "*death1",
		.radius           = 16_fx,
		.height           = 56_fx,
		.cdheight         = 56_fx,
		.mass             = 100,
		.flags            = MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP | MF_NOTDMATCH,
		.flags2           = MF2_SLIDE | MF2_PASSMOBJ | MF2_PUSHWALL,
		.name             = "MT_AVATAR"
	},
	{
		// MT_HORDESPAWN
		.type             = MT_HORDESPAWN,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 100,
		.radius           = 16_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOGRAVITY,
		.name             = "MT_HORDESPAWN",
	},
	{
		// MT_CAREPACK
		.type             = MT_CAREPACK,
		.spawnstate       = S_CARE,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_CAREPACK",
	},
	{                 // MT_EXTRALIFE
		.type             = MT_EXTRALIFE,
		.spawnstate       = S_O1UP,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_EXTRALIFE",
	},
	{                 // MT_RESTEAMMATE
		.type             = MT_RESTEAMMATE,
		.spawnstate       = S_RES,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_SPECIAL,
		.name             = "MT_RESTEAMMATE",
	},
	{                 // MT_GRENADE
        MT_GRENADE,
		-1,             // doomednum
		S_STGRENADE,    // spawnstate
		1000,           // spawnhealth
		0,              // gibhealth
		S_NULL,         // seestate
		"weapons/rocklf", // seesound
		8,              // reactiontime
		NULL,           // attacksound
		S_NULL,         // painstate
		0,              // painchance
		NULL,           // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_EXPLODE1,     // deathstate
		S_NULL,         // xdeathstate
		"weapons/rocklx", // deathsound
		25*FRACUNIT,    // speed
		8*FRACUNIT,     // radius
		8*FRACUNIT,     // height
		8*FRACUNIT,     // cdheight
		100,            // mass
		20,             // damage
		NULL,           // activesound
		MF_NOBLOCKMAP|MF_MISSILE|MF_DROPOFF, // flags
		MF2_PCROSS|MF2_IMPACT|MF2_FIREDAMAGE|MF2_LOGRAV, // flags2
		S_NULL,         // raisestate
		0x10000,
		"MT_GRENADE",
		NO_ALTSPEED,	// altspeed
		64 * FRACUNIT,	// meleerange
		IG_DEFAULT,		// infighting group
		PG_DEFAULT,		// projectile group
		SG_DEFAULT,		// splash group
		MF3_GRENADE,	// flags3
		NULL, // ripsound
		MT_NULL		// droppeditem
	},
	{                 // MT_BFG10KSHOT
        MT_BFG10KSHOT,
		-1,             // doomednum
		S_DETONATE,     // spawnstate
		1000,           // spawnhealth
		0,              // gibhealth
		S_NULL,         // seestate
		"weapons/rocklx", // seesound
		8,              // reactiontime
		NULL,           // attacksound
		S_NULL,         // painstate
		0,              // painchance
		NULL,           // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		NULL,           // deathsound
		20*FRACUNIT,    // speed
		11*FRACUNIT,    // radius
		8*FRACUNIT,     // height
		8*FRACUNIT,     // cdheight
		100,            // mass
		192,            // damage
		NULL,           // activesound
		MF_NOBLOCKMAP|MF_DROPOFF|MF_NOGRAVITY, // flags
		MF2_PCROSS|MF2_IMPACT, // flags2
		S_NULL,         // raisestate
		0x10000,
		"MT_BFG10KSHOT",
		NO_ALTSPEED,	// altspeed
		64 * FRACUNIT,	// meleerange
		IG_DEFAULT,		// infighting group
		PG_DEFAULT,		// projectile group
		SG_DEFAULT,		// splash group
		0,		// flags3
		NULL, // ripsound
		MT_NULL		// droppeditem
	},
	{                 // MT_TURBOSPHERE
        MT_TURBOSPHERE,
		5030,           // doomednum
		S_TURB,         // spawnstate
		1000,           // spawnhealth
		0,              // gibhealth
		S_NULL,         // seestate
		NULL,           // seesound
		8,              // reactiontime
		NULL,           // attacksound
		S_NULL,         // painstate
		0,              // painchance
		NULL,           // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		NULL,           // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16*FRACUNIT,    // cdheight
		100,            // mass
		0,              // damage
		NULL,           // activesound
		MF_SPECIAL|MF_COUNTITEM, // flags
		0,              // flags2
		S_NULL,         // raisestate
		0x10000,
		"MT_TURBOSPHERE",
		NO_ALTSPEED,	// altspeed
		64 * FRACUNIT,	// meleerange
		IG_DEFAULT,		// infighting group
		PG_DEFAULT,		// projectile group
		SG_DEFAULT,		// splash group
		0,		// flags3
		NULL, // ripsound
		MT_NULL		// droppeditem
	},
	{                 // MT_TIMEFREEZER
        MT_TIMEFREEZER,
		5032,           // doomednum
		S_FREEZE,       // spawnstate
		1000,           // spawnhealth
		0,              // gibhealth
		S_NULL,         // seestate
		NULL,           // seesound
		8,              // reactiontime
		NULL,           // attacksound
		S_NULL,         // painstate
		0,              // painchance
		NULL,           // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		NULL,           // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16*FRACUNIT,    // cdheight
		100,            // mass
		0,              // damage
		NULL,           // activesound
		MF_SPECIAL|MF_COUNTITEM, // flags
		0,              // flags2
		S_NULL,         // raisestate
		0x10000,
		"MT_TIMEFREEZER",
		NO_ALTSPEED,	// altspeed
		64 * FRACUNIT,	// meleerange
		IG_DEFAULT,		// infighting group
		PG_DEFAULT,		// projectile group
		SG_DEFAULT,		// splash group
		0,		// flags3
		NULL, // ripsound
		MT_NULL		// droppeditem
	},
	{                 // MT_INVISIBILITY
        MT_INVISIBILITY,
		5036,           // doomednum
		S_INVS,         // spawnstate
		1000,           // spawnhealth
		0,              // gibhealth
		S_NULL,         // seestate
		NULL,           // seesound
		8,              // reactiontime
		NULL,           // attacksound
		S_NULL,         // painstate
		0,              // painchance
		NULL,           // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		NULL,           // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16*FRACUNIT,    // cdheight
		100,            // mass
		0,              // damage
		NULL,           // activesound
		MF_SPECIAL|MF_COUNTITEM, // flags
		0,              // flags2
		S_NULL,         // raisestate
		0x10000,
		"MT_INVISIBILITY",
		NO_ALTSPEED,	// altspeed
		64 * FRACUNIT,	// meleerange
		IG_DEFAULT,		// infighting group
		PG_DEFAULT,		// projectile group
		SG_DEFAULT,		// splash group
		0,		// flags3
		NULL, // ripsound
		MT_NULL		// droppeditem
	},
	{                 // MT_DOOMSPHERE
        MT_DOOMSPHERE,
		5037,           // doomednum
		S_DOOMSPH,      // spawnstate
		1000,           // spawnhealth
		0,              // gibhealth
		S_NULL,         // seestate
		NULL,           // seesound
		8,              // reactiontime
		NULL,           // attacksound
		S_NULL,         // painstate
		0,              // painchance
		NULL,           // painsound
		S_NULL,         // meleestate
		S_NULL,         // missilestate
		S_NULL,         // deathstate
		S_NULL,         // xdeathstate
		NULL,           // deathsound
		0,              // speed
		20*FRACUNIT,    // radius
		16*FRACUNIT,    // height
		16*FRACUNIT,    // cdheight
		100,            // mass
		0,              // damage
		NULL,           // activesound
		MF_SPECIAL|MF_COUNTITEM, // flags
		0,              // flags2
		S_NULL,         // raisestate
		0x10000,
		"MT_DOOMSPHERE",
		NO_ALTSPEED,	// altspeed
		64 * FRACUNIT,	// meleerange
		IG_DEFAULT,		// infighting group
		PG_DEFAULT,		// projectile group
		SG_DEFAULT,		// splash group
		0,		// flags3
		NULL, // ripsound
		MT_NULL		// droppeditem
	},

	{MT_RUNESTRENGTH, 5100, S_RUN1, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNESTRENGTH", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNERAGE, 5101, S_RUN2, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNERAGE", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNEDRAIN, 5102, S_RUN3, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNEDRAIN", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNERESISTANCE, 5103, S_RUN4, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNERESISTANCE", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNEREGENERATION, 5104, S_RUN5, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNEREGENERATION", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNEPROSPERITY, 5105, S_RUN6, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNEPROSPERITY", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNEHIGHJUMP, 5106, S_RUN7, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNEHIGHJUMP", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNEHASTE, 5107, S_RUN8, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNEHASTE", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNESPREAD, 5108, S_RUN9, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNESPREAD", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},
	{MT_RUNEREFLECTION, 5110, S_RUNB, 1000, 0, S_NULL, NULL, 8, NULL, S_NULL, 0, NULL, S_NULL, S_NULL, S_NULL, S_NULL, NULL, 0, 20*FRACUNIT, 10*FRACUNIT, 10*FRACUNIT, 100, 0, NULL, MF_SPECIAL|MF_COUNTITEM|MF_NOGRAVITY, 0, S_NULL, 0x10000, "MT_RUNEREFLECTION", NO_ALTSPEED, 64*FRACUNIT, IG_DEFAULT, PG_DEFAULT, SG_DEFAULT, 0, NULL, MT_NULL},

	// ----------- odamex mobjinfo end -----------
};
// NOLINTEND(readability-magic-numbers)

std::span<mobjinfo_t> getOdaMobjinfo() {
	return { odamobjinfo, ARRAY_LENGTH(odamobjinfo) };
}

std::span<state_t> getOdaStates() {
	return { odastates, ARRAY_LENGTH(odastates) };
}

std::span<const char*> getOdaSprNames() {
	return { odasprnames, ARRAY_LENGTH(odasprnames) };
}
