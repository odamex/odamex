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
constexpr std::array odastates = std::to_array<state_t>({
	// ZDoom/Odamex stuff starts here
	{.statenum = S_GIB0,         .sprite = SPR_GIB0},
	{.statenum = S_GIB1,         .sprite = SPR_GIB1},
	{.statenum = S_GIB2,         .sprite = SPR_GIB2},
	{.statenum = S_GIB3,         .sprite = SPR_GIB3},
	{.statenum = S_GIB4,         .sprite = SPR_GIB4},
	{.statenum = S_GIB5,         .sprite = SPR_GIB5},
	{.statenum = S_GIB6,         .sprite = SPR_GIB6},
	{.statenum = S_GIB7,         .sprite = SPR_GIB7},
	{.statenum = S_AMBIENTSOUND, .sprite = SPR_TROO, .tics =  1, .action = A_Ambient, .nextstate = S_AMBIENTSOUND},
	{.statenum = S_UNKNOWNTHING, .sprite = SPR_UNKN},

	//	[Toke - CTF]
	{.statenum = S_BSOK,  .sprite = SPR_BSOK}, // Blue Socket
	{.statenum = S_RSOK,  .sprite = SPR_RSOK}, // Red Socket
	{.statenum = S_BFLG,  .sprite = SPR_BFLG, .frame = 32768, .tics = 4, .nextstate = S_BFLG2}, // BLUE Flag Animation; S_BFLG
	{.statenum = S_BFLG2, .sprite = SPR_BFLG, .frame = 32769, .tics = 4, .nextstate = S_BFLG3}, // S_BFLG2
	{.statenum = S_BFLG3, .sprite = SPR_BFLG, .frame = 32770, .tics = 4, .nextstate = S_BFLG4}, // S_BFLG3
	{.statenum = S_BFLG4, .sprite = SPR_BFLG, .frame = 32771, .tics = 4, .nextstate = S_BFLG5}, // S_BFLG4
	{.statenum = S_BFLG5, .sprite = SPR_BFLG, .frame = 32772, .tics = 4, .nextstate = S_BFLG6}, // S_BFLG5
	{.statenum = S_BFLG6, .sprite = SPR_BFLG, .frame = 32773, .tics = 4, .nextstate = S_BFLG7}, // S_BFLG6
	{.statenum = S_BFLG7, .sprite = SPR_BFLG, .frame = 32774, .tics = 4, .nextstate = S_BFLG8}, // S_BFLG7
	{.statenum = S_BFLG8, .sprite = SPR_BFLG, .frame = 32775, .tics = 4, .nextstate = S_BFLG }, // S_BFLG8
	{.statenum = S_RFLG,  .sprite = SPR_RFLG, .frame = 32768, .tics = 4, .nextstate = S_RFLG2}, // RED Flag Animation; S_RFLG
	{.statenum = S_RFLG2, .sprite = SPR_RFLG, .frame = 32769, .tics = 4, .nextstate = S_RFLG3}, // S_RFLG2
	{.statenum = S_RFLG3, .sprite = SPR_RFLG, .frame = 32770, .tics = 4, .nextstate = S_RFLG4}, // S_RFLG3
	{.statenum = S_RFLG4, .sprite = SPR_RFLG, .frame = 32771, .tics = 4, .nextstate = S_RFLG5}, // S_RFLG4
	{.statenum = S_RFLG5, .sprite = SPR_RFLG, .frame = 32772, .tics = 4, .nextstate = S_RFLG6}, // S_RFLG5
	{.statenum = S_RFLG6, .sprite = SPR_RFLG, .frame = 32773, .tics = 4, .nextstate = S_RFLG7}, // S_RFLG6
	{.statenum = S_RFLG7, .sprite = SPR_RFLG, .frame = 32774, .tics = 4, .nextstate = S_RFLG8}, // S_RFLG7
	{.statenum = S_RFLG8, .sprite = SPR_RFLG, .frame = 32775, .tics = 4, .nextstate = S_RFLG }, // S_RFLG8
	{.statenum = S_BDWN,  .sprite = SPR_BDWN}, // Blue Dropped Flag; S_BDWN
	{.statenum = S_RDWN,  .sprite = SPR_RDWN}, // Red Dropped Flag; S_RDWN
	{.statenum = S_BCAR,  .sprite = SPR_BCAR}, // Blue Carried Flag; S_BCAR
	{.statenum = S_RCAR,  .sprite = SPR_RCAR}, // Red Carried Flag; S_RCAR
	{.statenum = S_GSOK,  .sprite = SPR_GSOK}, // Green Socket,
	{.statenum = S_GFLG,  .sprite = SPR_GFLG, .frame = 32768, .tics = 4, .nextstate = S_GFLG2}, // Green Flag Animation; S_GFLG
	{.statenum = S_GFLG2, .sprite = SPR_GFLG, .frame = 32769, .tics = 4, .nextstate = S_GFLG3}, // S_GFLG2
	{.statenum = S_GFLG3, .sprite = SPR_GFLG, .frame = 32770, .tics = 4, .nextstate = S_GFLG4}, // S_GFLG3
	{.statenum = S_GFLG4, .sprite = SPR_GFLG, .frame = 32771, .tics = 4, .nextstate = S_GFLG5}, // S_GFLG4
	{.statenum = S_GFLG5, .sprite = SPR_GFLG, .frame = 32772, .tics = 4, .nextstate = S_GFLG6}, // S_GFLG5
	{.statenum = S_GFLG6, .sprite = SPR_GFLG, .frame = 32773, .tics = 4, .nextstate = S_GFLG7}, // S_GFLG6
	{.statenum = S_GFLG7, .sprite = SPR_GFLG, .frame = 32774, .tics = 4, .nextstate = S_GFLG8}, // S_GFLG7
	{.statenum = S_GFLG8, .sprite = SPR_GFLG, .frame = 32775, .tics = 4, .nextstate = S_GFLG }, // S_GFLG8
	{.statenum = S_GDWN,  .sprite = SPR_GDWN}, // Green Dropped Flag; S_GDWN
	{.statenum = S_GCAR,  .sprite = SPR_GCAR}, // Green Carried Flag; S_GCAR

	{.statenum = S_BRIDGE1, .sprite = SPR_TLGL, .frame = 32768, .tics = 4, .nextstate = S_BRIDGE2}, // S_BRIDGE1
	{.statenum = S_BRIDGE2, .sprite = SPR_TLGL, .frame = 32769, .tics = 4, .nextstate = S_BRIDGE3}, // S_BRIDGE2
	{.statenum = S_BRIDGE3, .sprite = SPR_TLGL, .frame = 32770, .tics = 4, .nextstate = S_BRIDGE4}, // S_BRIDGE3
	{.statenum = S_BRIDGE4, .sprite = SPR_TLGL, .frame = 32771, .tics = 4, .nextstate = S_BRIDGE5}, // S_BRIDGE4
	{.statenum = S_BRIDGE5, .sprite = SPR_TLGL, .frame = 32772, .tics = 4, .nextstate = S_BRIDGE1}, // S_BRIDGE5

	{.statenum = S_WPBF1, .sprite = SPR_WPBF, .frame = 0, .tics = 1, .nextstate = S_WPBF2}, // S_WPBF1 - Waypoint Blue Flag
	{.statenum = S_WPBF2, .sprite = SPR_WPBF, .frame = 1, .tics = 1, .nextstate = S_WPBF1}, // S_WPBF2
	{.statenum = S_WPRF1, .sprite = SPR_WPRF, .frame = 0, .tics = 1, .nextstate = S_WPRF2}, // S_WPRF1 - Waypoint Red Flag
	{.statenum = S_WPRF2, .sprite = SPR_WPRF, .frame = 1, .tics = 1, .nextstate = S_WPRF1}, // S_WPRF2
	{.statenum = S_WPGF1, .sprite = SPR_WPGF, .frame = 0, .tics = 1, .nextstate = S_WPGF2}, // S_WPGF1 - Waypoint Green Flag
	{.statenum = S_WPGF2, .sprite = SPR_WPGF, .frame = 1, .tics = 1, .nextstate = S_WPGF1}, // S_WPGF2

	{.statenum = S_CARE,  .sprite = SPR_CARE}, // S_CARE - Horde Care Package
	{.statenum = S_O1UP,  .sprite = SPR_O1UP, .frame = 32768, .tics = 4, .nextstate = S_O1UP2}, // S_O1UP - Horde Extra Life Powerup
	{.statenum = S_O1UP2, .sprite = SPR_O1UP, .frame = 32769, .tics = 4, .nextstate = S_O1UP3},
	{.statenum = S_O1UP3, .sprite = SPR_O1UP, .frame = 32770, .tics = 4, .nextstate = S_O1UP4},
	{.statenum = S_O1UP4, .sprite = SPR_O1UP, .frame = 32771, .tics = 4, .nextstate = S_O1UP5},
	{.statenum = S_O1UP5, .sprite = SPR_O1UP, .frame = 32770, .tics = 4, .nextstate = S_O1UP6},
	{.statenum = S_O1UP6, .sprite = SPR_O1UP, .frame = 32769, .tics = 4, .nextstate = S_O1UP },
	{.statenum = S_RES,   .sprite = SPR_RSTM, .frame = 32768, .tics = 5, .nextstate = S_RES2 }, // S_RES - Horde Resurrect Powerup
	{.statenum = S_RES2,  .sprite = SPR_RSTM, .frame = 32769, .tics = 5, .nextstate = S_RES3 },
	{.statenum = S_RES3,  .sprite = SPR_RSTM, .frame = 32770, .tics = 5, .nextstate = S_RES4 },
	{.statenum = S_RES4,  .sprite = SPR_RSTM, .frame = 32771, .tics = 5, .nextstate = S_RES  },

	{.statenum = S_NOWEAPONUP,   .tics = 1, .action = A_Raise,       .nextstate = S_NOWEAPON}, // S_NOWEAPONUP
	{.statenum = S_NOWEAPONDOWN, .tics = 1, .action = A_Lower,       .nextstate = S_NOWEAPON}, // S_NOWEAPONDOWN
	{.statenum = S_NOWEAPON,     .tics = 1, .action = A_WeaponReady, .nextstate = S_NOWEAPON}, // S_NOWEAPON
});

// reserved odamex sprites
// ::SPR_CARE - ::SPR_GIB0 + 2
constexpr std::array odasprnames = std::to_array<const char*>({
	"GIB0", "GIB1", "GIB2", "GIB3", "GIB4", "GIB5", "GIB6", "GIB7", "UNKN",
	// [Toke - CTF]
	"BSOK", "RSOK", "BFLG", "RFLG", "BDWN", "RDWN", "BCAR", "RCAR", "GSOK", "GFLG",
	"GDWN", "GCAR", "TLGL", "WPBF", "WPRF", "WPGF",
	// Horde
	"CARE", "O1UP", "RSTM",
});

// reserved odamex mobjinfo
// ::MT_CAREPACK - ::MT_GIB0 + 1
// this table *is* the constants for those numbers
// NOLINTBEGIN(readability-magic-numbers)
constexpr std::array odamobjinfo = std::to_array<mobjinfo_t>({
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

{
		// MT_SPRINGPAD
		.type             = MT_SPRINGPAD,
		.doomednum        = 5068,
		.spawnstate       = S_TNT1,
		.spawnhealth      = 1000,
		.reactiontime     = 8,
		.radius           = 20_fx,
		.height           = 16_fx,
		.cdheight         = 16_fx,
		.mass             = 100,
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY,
		.flags2           = MF2_DONTDRAW,
		.name             = "MT_SPRINGPAD",
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

	// ----------- odamex mobjinfo end -----------
});
// NOLINTEND(readability-magic-numbers)

std::span<const mobjinfo_t> getOdaMobjinfo() {
	return odamobjinfo;
}

std::span<const state_t> getOdaStates() {
	return odastates;
}

std::span<const char* const> getOdaSprNames() {
	return odasprnames;
}

VERSION_CONTROL (odainfo_cpp, "$Id$")
