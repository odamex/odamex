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
};

// reserved odamex sprites
// ::SPR_CARE - ::SPR_GIB0 + 2
const char* odasprnames[] = {
	"GIB0", "GIB1", "GIB2", "GIB3", "GIB4", "GIB5", "GIB6", "GIB7", "UNKN",
	//	[Toke - CTF]
	"BSOK", "RSOK", "BFLG", "RFLG", "BDWN", "RDWN", "BCAR", "RCAR", "GSOK", "GFLG",
	"GDWN", "GCAR", "TLGL", "WPBF", "WPRF", "WPGF", "CARE", "O1UP", "RSTM",};

// reserved odamex mobjinfo
// ::MT_CAREPACK - ::MT_GIB0 + 1
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
		.doomednum        = -1,                           // doomednum
		.spawnstate       = S_TNT1,                       // spawnstate
		.spawnhealth      = 1000,                         // spawnhealth
		.gibhealth        = 0,                            // gibhealth
		.seestate         = S_NULL,                       // seestate
		.seesound         = NULL,                         // seesound
		.reactiontime     = 8,                            // reactiontime
		.attacksound      = NULL,                         // attacksound
		.painstate        = S_NULL,                       // painstate
		.painchance       = 0,                            // painchance
		.painsound        = NULL,                         // painsound
		.meleestate       = S_NULL,                       // meleestate
		.missilestate     = S_NULL,                       // missilestate
		.deathstate       = S_NULL,                       // deathstate
		.xdeathstate      = S_NULL,                       // xdeathstate
		.deathsound       = NULL,                         // deathsound
		.speed            = 0,                            // speed
		.radius           = 20 * FRACUNIT,                // radius
		.height           = 0 * FRACUNIT,                 // height
		.cdheight         = 0 * FRACUNIT,                 // cdheight
		.mass             = 100,                          // mass
		.damage           = 0,                            // damage
		.activesound      = NULL,                         // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOGRAVITY, // flags
		.flags2           = 0,                            // flags2
		.raisestate       = S_NULL,                       // raisestate
		.translucency     = 0,
		.name             = "MT_FOUNTAIN",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_NODE   //Added by MC.
		.type             = MT_NODE,                    // Added by MC.,
		.doomednum        = 786,                        // doomednum
		.spawnstate       = S_TNT1,                     // spawnstate
		.spawnhealth      = 1000,                       // spawnhealth
		.gibhealth        = 0,                          // gibhealth
		.seestate         = S_NULL,                     // seestate
		.seesound         = NULL,                       // seesound
		.reactiontime     = 8,                          // reactiontime
		.attacksound      = NULL,                       // attacksound
		.painstate        = S_NULL,                     // painstate
		.painchance       = 0,                          // painchance
		.painsound        = NULL,                       // painsound
		.meleestate       = S_NULL,                     // meleestate
		.missilestate     = S_NULL,                     // missilestate
		.deathstate       = S_NULL,                     // deathstate
		.xdeathstate      = S_NULL,                     // xdeathstate
		.deathsound       = NULL,                       // deathsound
		.speed            = 0,                          // speed
		.radius           = 20 * FRACUNIT,              // radius
		.height           = 16 * FRACUNIT,              // height
		.cdheight         = 16 * FRACUNIT,              // cdheight
		.mass             = 100,                        // mass
		.damage           = 0,                          // damage
		.activesound      = NULL,                       // activesound
		.flags            = MF_NOSECTOR | MF_NOGRAVITY, // flags  MF_NOSECTOR  Makes it invisible
		.flags2           = 0,                          // flags2
		.raisestate       = S_NULL,                     // raisestate
		.translucency     = 0,
		.name             = "MT_NODE   //Added by MC.",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_WATERZONE
		.type             = MT_WATERZONE,
		.doomednum        = 9045,                                       // doomednum
		.spawnstate       = S_TNT1,                                     // spawnstate
		.spawnhealth      = 1000,                                       // spawnhealth
		.gibhealth        = 0,                                          // gibhealth
		.seestate         = S_NULL,                                     // seestate
		.seesound         = NULL,                                       // seesound
		.reactiontime     = 8,                                          // reactiontime
		.attacksound      = NULL,                                       // attacksound
		.painstate        = S_NULL,                                     // painstate
		.painchance       = 0,                                          // painchance
		.painsound        = NULL,                                       // painsound
		.meleestate       = S_NULL,                                     // meleestate
		.missilestate     = S_NULL,                                     // missilestate
		.deathstate       = S_NULL,                                     // deathstate
		.xdeathstate      = S_NULL,                                     // xdeathstate
		.deathsound       = NULL,                                       // deathsound
		.speed            = 0,                                          // speed
		.radius           = 16 * FRACUNIT,                              // radius
		.height           = 16 * FRACUNIT,                              // height
		.cdheight         = 16 * FRACUNIT,                              // cdheight
		.mass             = 100,                                        // mass
		.damage           = 0,                                          // damage
		.activesound      = NULL,                                       // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY, // flags
		.flags2           = 0,                                          // flags2
		.raisestate       = S_NULL,                                     // raisestate
		.translucency     = 0x10000,
		.name             = "MT_WATERZONE",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECRETTRIGGER
		.type             = MT_SECRETTRIGGER,
		.doomednum        = 9046,                                       // doomednum
		.spawnstate       = S_TNT1,                                     // spawnstate
		.spawnhealth      = 1000,                                       // spawnhealth
		.gibhealth        = 0,                                          // gibhealth
		.seestate         = S_NULL,                                     // seestate
		.seesound         = NULL,                                       // seesound
		.reactiontime     = 8,                                          // reactiontime
		.attacksound      = NULL,                                       // attacksound
		.painstate        = S_NULL,                                     // painstate
		.painchance       = 0,                                          // painchance
		.painsound        = NULL,                                       // painsound
		.meleestate       = S_NULL,                                     // meleestate
		.missilestate     = S_NULL,                                     // missilestate
		.deathstate       = S_NULL,                                     // deathstate
		.xdeathstate      = S_NULL,                                     // xdeathstate
		.deathsound       = NULL,                                       // deathsound
		.speed            = 0,                                          // speed
		.radius           = 16 * FRACUNIT,                              // radius
		.height           = 16 * FRACUNIT,                              // height
		.cdheight         = 16 * FRACUNIT,                              // cdheight
		.mass             = 100,                                        // mass
		.damage           = 0,                                          // damage
		.activesound      = NULL,                                       // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY, // flags
		.flags2           = 0,                                          // flags2
		.raisestate       = S_NULL,                                     // raisestate
		.translucency     = 0,
		.name             = "MT_SECRETTRIGGER",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{
		// MT_SKYVIEWPOINT
		.type             = MT_SKYVIEWPOINT,
		.doomednum        = 9080,                                       // doomednum
		.spawnstate       = S_TNT1,                                     // spawnstate
		.spawnhealth      = 1000,                                       // spawnhealth
		.gibhealth        = 0,                                          // gibhealth
		.seestate         = S_NULL,                                     // seestate
		.seesound         = NULL,                                       // seesound
		.reactiontime     = 8,                                          // reactiontime
		.attacksound      = NULL,                                       // attacksound
		.painstate        = S_NULL,                                     // painstate
		.painchance       = 0,                                          // painchance
		.painsound        = NULL,                                       // painsound
		.meleestate       = S_NULL,                                     // meleestate
		.missilestate     = S_NULL,                                     // missilestate
		.deathstate       = S_NULL,                                     // deathstate
		.xdeathstate      = S_NULL,                                     // xdeathstate
		.deathsound       = NULL,                                       // deathsound
		.speed            = 0,                                          // speed
		.radius           = 20 * FRACUNIT,                              // radius
		.height           = 16 * FRACUNIT,                              // height
		.cdheight         = 16 * FRACUNIT,                              // cdheight
		.mass             = 100,                                        // mass
		.damage           = 0,                                          // damage
		.activesound      = NULL,                                       // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY, // flags
		.flags2           = MF2_DONTDRAW,                               // flags2
		.raisestate       = S_NULL,                                     // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SKYVIEWPOINT",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{
		// MT_SKYPICKER
		.type             = MT_SKYPICKER,
		.doomednum        = 9081,                                       // doomednum
		.spawnstate       = S_TNT1,                                     // spawnstate
		.spawnhealth      = 1000,                                       // spawnhealth
		.gibhealth        = 0,                                          // gibhealth
		.seestate         = S_NULL,                                     // seestate
		.seesound         = NULL,                                       // seesound
		.reactiontime     = 8,                                          // reactiontime
		.attacksound      = NULL,                                       // attacksound
		.painstate        = S_NULL,                                     // painstate
		.painchance       = 0,                                          // painchance
		.painsound        = NULL,                                       // painsound
		.meleestate       = S_NULL,                                     // meleestate
		.missilestate     = S_NULL,                                     // missilestate
		.deathstate       = S_NULL,                                     // deathstate
		.xdeathstate      = S_NULL,                                     // xdeathstate
		.deathsound       = NULL,                                       // deathsound
		.speed            = 0,                                          // speed
		.radius           = 20 * FRACUNIT,                              // radius
		.height           = 16 * FRACUNIT,                              // height
		.cdheight         = 16 * FRACUNIT,                              // cdheight
		.mass             = 100,                                        // mass
		.damage           = 0,                                          // damage
		.activesound      = NULL,                                       // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY, // flags
		.flags2           = MF2_DONTDRAW,                               // flags2
		.raisestate       = S_NULL,                                     // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SKYPICKER",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{
		// MT_SECTORSILENCER
		.type             = MT_SECTORSILENCER,
		.doomednum        = 9082,                         // doomednum
		.spawnstate       = S_TNT1,                       // spawnstate
		.spawnhealth      = 1000,                         // spawnhealth
		.gibhealth        = 0,                            // gibhealth
		.seestate         = S_NULL,                       // seestate
		.seesound         = NULL,                         // seesound
		.reactiontime     = 8,                            // reactiontime
		.attacksound      = NULL,                         // attacksound
		.painstate        = S_NULL,                       // painstate
		.painchance       = 0,                            // painchance
		.painsound        = NULL,                         // painsound
		.meleestate       = S_NULL,                       // meleestate
		.missilestate     = S_NULL,                       // missilestate
		.deathstate       = S_NULL,                       // deathstate
		.xdeathstate      = S_NULL,                       // xdeathstate
		.deathsound       = NULL,                         // deathsound
		.speed            = 0,                            // speed
		.radius           = 20 * FRACUNIT,                // radius
		.height           = 16 * FRACUNIT,                // height
		.cdheight         = 16 * FRACUNIT,                // cdheight
		.mass             = 100,                          // mass
		.damage           = 0,                            // damage
		.activesound      = NULL,                         // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOGRAVITY, // flags
		.flags2           = MF2_DONTDRAW,                 // flags2
		.raisestate       = S_NULL,                       // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECTORSILENCER",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Blue Socket
	{
		//  MT_BSOK
		.type             = MT_BSOK,
		.doomednum        = 5130,          // doomednum
		.spawnstate       = S_BSOK,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 14 * FRACUNIT, // height
		.cdheight         = 14 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BSOK",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Red Socket
	{
		//  MT_RSOK
		.type             = MT_RSOK,
		.doomednum        = 5131,          // doomednum
		.spawnstate       = S_RSOK,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 14 * FRACUNIT, // height
		.cdheight         = 14 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_RSOK",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// Nes - Reserve 5132 for Neutral Socket

	// [Toke - CTF] Blue Flag
	{
		//  MT_BFLG
		.type             = MT_BFLG,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_BFLG,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 48 * FRACUNIT, // height
		.cdheight         = 48 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BFLG",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Red Flag
	{
		//  MT_RFLG
		.type             = MT_RFLG,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_RFLG,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 48 * FRACUNIT, // height
		.cdheight         = 48 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_RFLG",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Blue Dropped Flag
	{
		//  MT_BDWN
		.type             = MT_BDWN,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_BDWN,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 48 * FRACUNIT, // height
		.cdheight         = 48 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BDWN",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Red Dropped Flag
	{
		//  MT_RDWN
		.type             = MT_RDWN,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_RDWN,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 48 * FRACUNIT, // height
		.cdheight         = 48 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_RDWN",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Blue Carrying Flag
	{
		//  MT_BCAR
		.type             = MT_BCAR,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_BCAR,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 0,             // radius
		.height           = 0,             // height
		.cdheight         = 0,             // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_NOGRAVITY,  // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BCAR",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	// [Toke - CTF] Red Carrying Flag
	{
		//  MT_RCAR
		.type             = MT_RCAR,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_RCAR,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 0,             // radius
		.height           = 0,             // height
		.cdheight         = 0,             // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_NOGRAVITY,  // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_RCAR",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_BRIDGE
		.type             = MT_BRIDGE,
		.doomednum        = 118,                     // doomednum
		.spawnstate       = S_BRIDGE1,               // spawnstate
		.spawnhealth      = 1000,                    // spawnhealth
		.gibhealth        = 0,                       // gibhealth
		.seestate         = S_NULL,                  // seestate
		.seesound         = NULL,                    // seesound
		.reactiontime     = 8,                       // reactiontime
		.attacksound      = NULL,                    // attacksound
		.painstate        = S_NULL,                  // painstate
		.painchance       = 0,                       // painchance
		.painsound        = NULL,                    // painsound
		.meleestate       = S_NULL,                  // meleestate
		.missilestate     = S_NULL,                  // missilestate
		.deathstate       = S_NULL,                  // deathstate
		.xdeathstate      = S_NULL,                  // xdeathstate
		.deathsound       = NULL,                    // deathsound
		.speed            = 0,                       // speed
		.radius           = 36 * FRACUNIT,           // radius
		.height           = 4 * FRACUNIT,            // height
		.cdheight         = 4 * FRACUNIT,            // cdheight
		.mass             = 100,                     // mass
		.damage           = 0,                       // damage
		.activesound      = NULL,                    // activesound
		.flags            = MF_SOLID | MF_NOGRAVITY, // flags
		.flags2           = MF2_DONTDRAW,            // flags2
		.raisestate       = S_NULL,                  // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BRIDGE",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_MAPSPOT
		.type             = MT_MAPSPOT,
		.doomednum        = 9001,                                       // doomednum
		.spawnstate       = S_TNT1,                                     // spawnstate
		.spawnhealth      = 1000,                                       // spawnhealth
		.gibhealth        = 0,                                          // gibhealth
		.seestate         = S_NULL,                                     // seestate
		.seesound         = NULL,                                       // seesound
		.reactiontime     = 8,                                          // reactiontime
		.attacksound      = NULL,                                       // attacksound
		.painstate        = S_NULL,                                     // painstate
		.painchance       = 0,                                          // painchance
		.painsound        = NULL,                                       // painsound
		.meleestate       = S_NULL,                                     // meleestate
		.missilestate     = S_NULL,                                     // missilestate
		.deathstate       = S_NULL,                                     // deathstate
		.xdeathstate      = S_NULL,                                     // xdeathstate
		.deathsound       = NULL,                                       // deathsound
		.speed            = 0,                                          // speed
		.radius           = 16 * FRACUNIT,                              // radius
		.height           = 16 * FRACUNIT,                              // height
		.cdheight         = 16 * FRACUNIT,                              // cdheight
		.mass             = 100,                                        // mass
		.damage           = 0,                                          // damage
		.activesound      = NULL,                                       // activesound
		.flags            = MF_NOBLOCKMAP | MF_NOSECTOR | MF_NOGRAVITY, // flags
		.flags2           = 0,                                          // flags2
		.raisestate       = S_NULL,                                     // raisestate
		.translucency     = 0x10000,
		.name             = "MT_MAPSPOT",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_MAPSPOTGRAVITY
		.type             = MT_MAPSPOTGRAVITY,
		.doomednum        = 9013,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_MAPSPOTGRAV",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_BRIDGE32
		.type             = MT_BRIDGE32,
		.doomednum        = 5061,                    // doomednum
		.spawnstate       = S_TNT1,                  // spawnstate
		.spawnhealth      = 1000,                    // spawnhealth
		.gibhealth        = 0,                       // gibhealth
		.seestate         = S_NULL,                  // seestate
		.seesound         = NULL,                    // seesound
		.reactiontime     = 8,                       // reactiontime
		.attacksound      = NULL,                    // attacksound
		.painstate        = S_NULL,                  // painstate
		.painchance       = 0,                       // painchance
		.painsound        = NULL,                    // painsound
		.meleestate       = S_NULL,                  // meleestate
		.missilestate     = S_NULL,                  // missilestate
		.deathstate       = S_NULL,                  // deathstate
		.xdeathstate      = S_NULL,                  // xdeathstate
		.deathsound       = NULL,                    // deathsound
		.speed            = 0,                       // speed
		.radius           = 32 * FRACUNIT,           // radius
		.height           = 8 * FRACUNIT,            // height
		.cdheight         = 8 * FRACUNIT,            // cdheight
		.mass             = 100,                     // mass
		.damage           = 0,                       // damage
		.activesound      = NULL,                    // activesound
		.flags            = MF_SOLID | MF_NOGRAVITY, // flags
		.flags2           = 0,                       // flags2
		.raisestate       = S_NULL,                  // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BRIDGE32",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_BRIDGE16
		.type             = MT_BRIDGE16,
		.doomednum        = 5064,                    // doomednum
		.spawnstate       = S_TNT1,                  // spawnstate
		.spawnhealth      = 1000,                    // spawnhealth
		.gibhealth        = 0,                       // gibhealth
		.seestate         = S_NULL,                  // seestate
		.seesound         = NULL,                    // seesound
		.reactiontime     = 8,                       // reactiontime
		.attacksound      = NULL,                    // attacksound
		.painstate        = S_NULL,                  // painstate
		.painchance       = 0,                       // painchance
		.painsound        = NULL,                    // painsound
		.meleestate       = S_NULL,                  // meleestate
		.missilestate     = S_NULL,                  // missilestate
		.deathstate       = S_NULL,                  // deathstate
		.xdeathstate      = S_NULL,                  // xdeathstate
		.deathsound       = NULL,                    // deathsound
		.speed            = 0,                       // speed
		.radius           = 16 * FRACUNIT,           // radius
		.height           = 8 * FRACUNIT,            // height
		.cdheight         = 8 * FRACUNIT,            // cdheight
		.mass             = 100,                     // mass
		.damage           = 0,                       // damage
		.activesound      = NULL,                    // activesound
		.flags            = MF_SOLID | MF_NOGRAVITY, // flags
		.flags2           = 0,                       // flags2
		.raisestate       = S_NULL,                  // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BRIDGE16",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_BRIDGE8
		.type             = MT_BRIDGE8,
		.doomednum        = 5065,                    // doomednum
		.spawnstate       = S_TNT1,                  // spawnstate
		.spawnhealth      = 1000,                    // spawnhealth
		.gibhealth        = 0,                       // gibhealth
		.seestate         = S_NULL,                  // seestate
		.seesound         = NULL,                    // seesound
		.reactiontime     = 8,                       // reactiontime
		.attacksound      = NULL,                    // attacksound
		.painstate        = S_NULL,                  // painstate
		.painchance       = 0,                       // painchance
		.painsound        = NULL,                    // painsound
		.meleestate       = S_NULL,                  // meleestate
		.missilestate     = S_NULL,                  // missilestate
		.deathstate       = S_NULL,                  // deathstate
		.xdeathstate      = S_NULL,                  // xdeathstate
		.deathsound       = NULL,                    // deathsound
		.speed            = 0,                       // speed
		.radius           = 8 * FRACUNIT,            // radius
		.height           = 8 * FRACUNIT,            // height
		.cdheight         = 8 * FRACUNIT,            // cdheight
		.mass             = 100,                     // mass
		.damage           = 0,                       // damage
		.activesound      = NULL,                    // activesound
		.flags            = MF_SOLID | MF_NOGRAVITY, // flags
		.flags2           = 0,                       // flags2
		.raisestate       = S_NULL,                  // raisestate
		.translucency     = 0x10000,
		.name             = "MT_BRIDGE8",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_ZDOOMBRIDGE
		.type             = MT_ZDOOMBRIDGE,
		.doomednum        = 9990,                    // doomednum
		.spawnstate       = S_TNT1,                  // spawnstate
		.spawnhealth      = 1000,                    // spawnhealth
		.gibhealth        = 0,                       // gibhealth
		.seestate         = S_NULL,                  // seestate
		.seesound         = NULL,                    // seesound
		.reactiontime     = 8,                       // reactiontime
		.attacksound      = NULL,                    // attacksound
		.painstate        = S_NULL,                  // painstate
		.painchance       = 0,                       // painchance
		.painsound        = NULL,                    // painsound
		.meleestate       = S_NULL,                  // meleestate
		.missilestate     = S_NULL,                  // missilestate
		.deathstate       = S_NULL,                  // deathstate
		.xdeathstate      = S_NULL,                  // xdeathstate
		.deathsound       = NULL,                    // deathsound
		.speed            = 0,                       // speed
		.radius           = 32 * FRACUNIT,           // radius
		.height           = 4 * FRACUNIT,            // height
		.cdheight         = 4 * FRACUNIT,            // cdheight
		.mass             = 100,                     // mass
		.damage           = 0,                       // damage
		.activesound      = NULL,                    // activesound
		.flags            = MF_SOLID | MF_NOGRAVITY, // flags
		.flags2           = 0,                       // flags2
		.raisestate       = S_NULL,                  // raisestate
		.translucency     = 0x10000,
		.name             = "MT_ZDOOMBRIDGE",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTENTER
		.type             = MT_SECACTENTER,
		.doomednum        = 9998,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTENTER",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTEXIT
		.type             = MT_SECACTEXIT,
		.doomednum        = 9997,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTEXIT",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTHITFLOOR
		.type             = MT_SECACTHITFLOOR,
		.doomednum        = 9999,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTHITFLOOR",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTHITCEIL
		.type             = MT_SECACTHITCEIL,
		.doomednum        = 9996,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTHITCEIL",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTUSE
		.type             = MT_SECACTUSE,
		.doomednum        = 9995,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTUSE",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTUSEWALL
		.type             = MT_SECACTUSEWALL,
		.doomednum        = 9994,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTUSEWALL",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTEYESDIVE
		.type             = MT_SECACTEYESDIVE,
		.doomednum        = 9993,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTEYESDIVE",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTEYESSURFACE
		.type             = MT_SECACTEYESSURFACE,
		.doomednum        = 9992,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTEYESSURFACE",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTEYESBELOWC
		.type             = MT_SECACTEYESBELOWC,
		.doomednum        = 9983,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTEYESBELOWC",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_SECACTEYESABOVEC
		.type             = MT_SECACTEYESABOVEC,
		.doomednum        = 9982,          // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = MF2_DONTDRAW,  // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_SECACTEYESABOVEC",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_GSOK
		.type             = MT_GSOK,
		.doomednum        = 5133,          // doomednum
		.spawnstate       = S_GSOK,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 14 * FRACUNIT, // height
		.cdheight         = 14 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_GSOK",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_GFLG
		.type             = MT_GFLG,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_GFLG,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 48 * FRACUNIT, // height
		.cdheight         = 48 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_GFLG",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_GDWN
		.type             = MT_GDWN,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_GDWN,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 48 * FRACUNIT, // height
		.cdheight         = 48 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_GDWN",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},

	{
		//  MT_GCAR
		.type             = MT_GCAR,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_GCAR,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 0,             // radius
		.height           = 0,             // height
		.cdheight         = 0,             // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_NOGRAVITY,  // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_GCAR",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{
		// MT_WPBFLAG
		.type             = MT_WPBFLAG,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_WPBF1,       // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0xC000,
		.name             = "MT_WPBFLAG",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{
		// MT_WPRFLAG
		.type             = MT_WPRFLAG,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_WPRF1,       // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0xC000,
		.name             = "MT_WPRFLAG"
	},
	{
		// MT_WPGFLAG
		.type             = MT_WPGFLAG,
		.doomednum        = -1,            // doomednum
		.spawnstate       = S_WPGF1,       // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = 0,             // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0xC000,
		.name             = "MT_WPGFLAG"
	},
	{
		// MT_AVATAR
		.type             = MT_AVATAR,
		.doomednum        = -1,                                                              // doomednum
		.spawnstate       = S_PLAY,                                                          // spawnstate
		.spawnhealth      = 100,                                                             // spawnhealth
		.gibhealth        = 0,                                                               // gibhealth
		.seestate         = S_PLAY_RUN1,                                                     // seestate
		.seesound         = NULL,                                                            // seesound
		.reactiontime     = 0,                                                               // reactiontime
		.attacksound      = NULL,                                                            // a ttacksound
		.painstate        = S_PLAY_PAIN,                                                     // painstate
		.painchance       = 255,                                                             // painchance
		.painsound        = "*pain100_1",                                                    // painsound
		.meleestate       = S_NULL,                                                          // meleestate
		.missilestate     = S_PLAY_ATK1,                                                     // missilestate
		.deathstate       = S_PLAY_DIE1,                                                     // deathstate
		.xdeathstate      = S_PLAY_XDIE1,                                                    // xdeathstate
		.deathsound       = "*death1",                                                       // deathsound
		.speed            = 0,                                                               // speed
		.radius           = 16 * FRACUNIT,                                                   // radius
		.height           = 56 * FRACUNIT,                                                   // height
		.cdheight         = 56 * FRACUNIT,                                                   // cdheight
		.mass             = 100,                                                             // mass
		.damage           = 0,                                                               // damage
		.activesound      = NULL,                                                            // activesound
		.flags            = MF_SOLID | MF_SHOOTABLE | MF_DROPOFF | MF_PICKUP | MF_NOTDMATCH, // flags
		.flags2           = MF2_SLIDE | MF2_PASSMOBJ | MF2_PUSHWALL,                         // flags2
		.raisestate       = S_NULL,                                                          // raisestate
		.translucency     = 0x10000,
		.name             = "MT_AVATAR"
	},
	{
		// MT_HORDESPAWN
		.type             = MT_HORDESPAWN,
		.doomednum        = 5302,        // doomednum
		.spawnstate       = S_TNT1,        // spawnstate
		.spawnhealth      = 100,           // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 0,             // reactiontime
		.attacksound      = NULL,          // a ttacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 16 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_NOGRAVITY,  // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_HORDESPAWN",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{
		// MT_CAREPACK
		.type             = MT_CAREPACK,
		.doomednum        = 5301,        // doomednum
		.spawnstate       = S_CARE,        // spawnstate
		.spawnhealth      = 1000,          // spawnhealth
		.gibhealth        = 0,             // gibhealth
		.seestate         = S_NULL,        // seestate
		.seesound         = NULL,          // seesound
		.reactiontime     = 8,             // reactiontime
		.attacksound      = NULL,          // attacksound
		.painstate        = S_NULL,        // painstate
		.painchance       = 0,             // painchance
		.painsound        = NULL,          // painsound
		.meleestate       = S_NULL,        // meleestate
		.missilestate     = S_NULL,        // missilestate
		.deathstate       = S_NULL,        // deathstate
		.xdeathstate      = S_NULL,        // xdeathstate
		.deathsound       = NULL,          // deathsound
		.speed            = 0,             // speed
		.radius           = 20 * FRACUNIT, // radius
		.height           = 16 * FRACUNIT, // height
		.cdheight         = 16 * FRACUNIT, // cdheight
		.mass             = 100,           // mass
		.damage           = 0,             // damage
		.activesound      = NULL,          // activesound
		.flags            = MF_SPECIAL,    // flags
		.flags2           = 0,             // flags2
		.raisestate       = S_NULL,        // raisestate
		.translucency     = 0x10000,
		.name             = "MT_CAREPACK",
		.altspeed         = NO_ALTSPEED,   // altspeed
		.meleerange       = 64 * FRACUNIT, // meleerange
		.infighting_group = IG_DEFAULT,    // infighting group
		.projectile_group = PG_DEFAULT,    // projectile group
		.splash_group     = SG_DEFAULT,    // splash group
		.flags3           = 0,             // flags3
		.ripsound         = NULL,          // ripsound
		.droppeditem      = MT_NULL        // droppeditem
	},
	{                 // MT_EXTRALIFE
		.type             = MT_EXTRALIFE,
		.doomednum        = -1,             // doomednum
		.spawnstate       = S_O1UP,         // spawnstate
		.spawnhealth      = 1000,           // spawnhealth
		.gibhealth        = 0,              // gibhealth
		.seestate         = S_NULL,         // seestate
		.seesound         = NULL,           // seesound
		.reactiontime     = 8,              // reactiontime
		.attacksound      = NULL,           // attacksound
		.painstate        = S_NULL,         // painstate
		.painchance       = 0,              // painchance
		.painsound        = NULL,           // painsound
		.meleestate       = S_NULL,         // meleestate
		.missilestate     = S_NULL,         // missilestate
		.deathstate       = S_NULL,         // deathstate
		.xdeathstate      = S_NULL,         // xdeathstate
		.deathsound       = NULL,           // deathsound
		.speed            = 0,              // speed
		.radius           = 20*FRACUNIT,    // radius
		.height           = 16*FRACUNIT,    // height
		.cdheight         = 16*FRACUNIT,    // cdheight
		.mass             = 100,            // mass
		.damage           = 0,              // damage
		.activesound      = NULL,           // activesound
		.flags            = MF_SPECIAL,     // flags
		.flags2           = 0,              // flags2
		.raisestate       = S_NULL,         // raisestate
		.translucency     = 0x10000,
		.name             = "MT_EXTRALIFE",
		.altspeed         = NO_ALTSPEED,	// altspeed
		.meleerange       = 64 * FRACUNIT,	// meleerange
		.infighting_group = IG_DEFAULT,		// infighting group
		.projectile_group = PG_DEFAULT,		// projectile group
		.splash_group     = SG_DEFAULT,		// splash group
		.flags3           = 0,		// flags3
		.ripsound         = NULL, // ripsound
		.droppeditem      = MT_NULL		// droppeditem
	},
	{                 // MT_RESTEAMMATE
		.type             = MT_RESTEAMMATE,
		.doomednum        = -1,             // doomednum
		.spawnstate       = S_RES,          // spawnstate
		.spawnhealth      = 1000,           // spawnhealth
		.gibhealth        = 0,              // gibhealth
		.seestate         = S_NULL,         // seestate
		.seesound         = NULL,           // seesound
		.reactiontime     = 8,              // reactiontime
		.attacksound      = NULL,           // attacksound
		.painstate        = S_NULL,         // painstate
		.painchance       = 0,              // painchance
		.painsound        = NULL,           // painsound
		.meleestate       = S_NULL,         // meleestate
		.missilestate     = S_NULL,         // missilestate
		.deathstate       = S_NULL,         // deathstate
		.xdeathstate      = S_NULL,         // xdeathstate
		.deathsound       = NULL,           // deathsound
		.speed            = 0,              // speed
		.radius           = 20*FRACUNIT,    // radius
		.height           = 16*FRACUNIT,    // height
		.cdheight         = 16*FRACUNIT,    // cdheight
		.mass             = 100,            // mass
		.damage           = 0,              // damage
		.activesound      = NULL,           // activesound
		.flags            = MF_SPECIAL,     // flags
		.flags2           = 0,              // flags2
		.raisestate       = S_NULL,         // raisestate
		.translucency     = 0x10000,
		.name             = "MT_RESTEAMMATE",
		.altspeed         = NO_ALTSPEED,	// altspeed
		.meleerange       = 64 * FRACUNIT,	// meleerange
		.infighting_group = IG_DEFAULT,		// infighting group
		.projectile_group = PG_DEFAULT,		// projectile group
		.splash_group     = SG_DEFAULT,		// splash group
		.flags3           = 0,		// flags3
		.ripsound         = NULL, // ripsound
		.droppeditem      = MT_NULL		// droppeditem
	},

	// ----------- odamex mobjinfo end -----------
};

std::span<mobjinfo_t> getOdaMobjinfo() {
	return { odamobjinfo, ARRAY_LENGTH(odamobjinfo) };
}

std::span<state_t> getOdaStates() {
	return { odastates, ARRAY_LENGTH(odastates) };
}

std::span<const char*> getOdaSprNames() {
	return { odasprnames, ARRAY_LENGTH(odasprnames) };
}
