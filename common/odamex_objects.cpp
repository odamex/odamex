#include "odamex.h"
#include "m_fixed.h"
#include "actor.h"
#include "info.h"

#include "odamex_objects.h"

// ------------------------------------------------------------------
// Odamex specific States, Sprites, and Things
// ------------------------------------------------------------------

// code pointers
void A_Lower(AActor *);
void A_Raise(AActor *);
void A_WeaponReady(AActor *);
void A_Ambient(AActor*);

// reserved odamex states
state_t odastates[] = {
    // ZDoom/Odamex stuff starts here
    {S_GIB0, SPR_GIB0, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB0
    {S_GIB1, SPR_GIB1, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB1
    {S_GIB2, SPR_GIB2, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB2
    {S_GIB3, SPR_GIB3, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB3
    {S_GIB4, SPR_GIB4, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB4
    {S_GIB5, SPR_GIB5, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB5
    {S_GIB6, SPR_GIB6, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB6
    {S_GIB7, SPR_GIB7, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB7
    {S_AMBIENTSOUND, SPR_TROO, 0, 1, A_Ambient, S_AMBIENTSOUND, 0, 0}, // S_AMBIENTSOUND
    {S_UNKNOWNTHING, SPR_UNKN, 0, -1, NULL, S_NULL, 0, 0},             // S_UNKNOWNTHING

    //	[Toke - CTF]
    {S_BSOK, SPR_BSOK, 0, -1, NULL, S_NULL, 0, 0},     // Blue Socket
    {S_RSOK, SPR_RSOK, 0, -1, NULL, S_NULL, 0, 0},     // Red Socket
    {S_BFLG, SPR_BFLG, 32768, 4, NULL, S_BFLG2, 0, 0}, // BLUE Flag Animation; S_BFLG
    {S_BFLG2, SPR_BFLG, 32769, 4, NULL, S_BFLG3, 0, 0}, // S_BFLG2
    {S_BFLG3, SPR_BFLG, 32770, 4, NULL, S_BFLG4, 0, 0}, // S_BFLG3
    {S_BFLG4, SPR_BFLG, 32771, 4, NULL, S_BFLG5, 0, 0}, // S_BFLG4
    {S_BFLG5, SPR_BFLG, 32772, 4, NULL, S_BFLG6, 0, 0}, // S_BFLG5
    {S_BFLG6, SPR_BFLG, 32773, 4, NULL, S_BFLG7, 0, 0}, // S_BFLG6
    {S_BFLG7, SPR_BFLG, 32774, 4, NULL, S_BFLG8, 0, 0}, // S_BFLG7
    {S_BFLG8, SPR_BFLG, 32775, 4, NULL, S_BFLG, 0, 0}, // S_BFLG8
    {S_RFLG, SPR_RFLG, 32768, 4, NULL, S_RFLG2, 0, 0}, // RED Flag Animation; S_RFLG
    {S_RFLG2, SPR_RFLG, 32769, 4, NULL, S_RFLG3, 0, 0}, // S_RFLG2
    {S_RFLG3, SPR_RFLG, 32770, 4, NULL, S_RFLG4, 0, 0}, // S_RFLG3
    {S_RFLG4, SPR_RFLG, 32771, 4, NULL, S_RFLG5, 0, 0}, // S_RFLG4
    {S_RFLG5, SPR_RFLG, 32772, 4, NULL, S_RFLG6, 0, 0}, // S_RFLG5
    {S_RFLG6, SPR_RFLG, 32773, 4, NULL, S_RFLG7, 0, 0}, // S_RFLG6
    {S_RFLG7, SPR_RFLG, 32774, 4, NULL, S_RFLG8, 0, 0}, // S_RFLG7
    {S_RFLG8, SPR_RFLG, 32775, 4, NULL, S_RFLG, 0, 0}, // S_RFLG8
    {S_BDWN, SPR_BDWN, 0, -1, NULL, S_NULL, 0, 0}, // Blue Dropped Flag; S_BDWN
    {S_RDWN, SPR_RDWN, 0, -1, NULL, S_NULL, 0, 0}, // Red Dropped Flag; S_RDWN
    {S_BCAR, SPR_BCAR, 0, -1, NULL, S_NULL, 0, 0}, // Blue Dropped Flag; S_BCAR
    {S_RCAR, SPR_RCAR, 0, -1, NULL, S_NULL, 0, 0}, // Red Dropped Flag; S_RCAR

    {S_GSOK, SPR_GSOK, 0, -1, NULL, S_NULL, 0, 0},     // S_GSOK,
    {S_GFLG, SPR_GFLG, 32768, 4, NULL, S_GFLG2, 0, 0}, // Green Flag Animation; S_GFLG
    {S_GFLG2, SPR_GFLG, 32769, 4, NULL, S_GFLG3, 0, 0}, // S_GFLG2
    {S_GFLG3, SPR_GFLG, 32770, 4, NULL, S_GFLG4, 0, 0}, // S_GFLG3
    {S_GFLG4, SPR_GFLG, 32771, 4, NULL, S_GFLG5, 0, 0}, // S_GFLG4
    {S_GFLG5, SPR_GFLG, 32772, 4, NULL, S_GFLG6, 0, 0}, // S_GFLG5
    {S_GFLG6, SPR_GFLG, 32773, 4, NULL, S_GFLG7, 0, 0}, // S_GFLG6
    {S_GFLG7, SPR_GFLG, 32774, 4, NULL, S_GFLG8, 0, 0}, // S_GFLG7
    {S_GFLG8, SPR_GFLG, 32775, 4, NULL, S_GFLG, 0, 0}, // S_GFLG8
    {S_GDWN, SPR_GDWN, 0, -1, NULL, S_NULL, 0, 0}, // S_GDWN,
    {S_GCAR, SPR_GCAR, 0, -1, NULL, S_NULL, 0, 0}, // S_GCAR,

    {S_BRIDGE1, SPR_TLGL, 32768, 4, NULL, S_BRIDGE2, 0, 0}, // S_BRIDGE1
    {S_BRIDGE2, SPR_TLGL, 32769, 4, NULL, S_BRIDGE3, 0, 0}, // S_BRIDGE2
    {S_BRIDGE3, SPR_TLGL, 32770, 4, NULL, S_BRIDGE4, 0, 0}, // S_BRIDGE3
    {S_BRIDGE4, SPR_TLGL, 32771, 4, NULL, S_BRIDGE5, 0, 0}, // S_BRIDGE4
    {S_BRIDGE5, SPR_TLGL, 32772, 4, NULL, S_BRIDGE1, 0, 0}, // S_BRIDGE5

    {S_WPBF1, SPR_WPBF, 0, 1, NULL, S_WPBF2, 0, 0},       // S_WPBF1 - Waypoint Blue Flag
    {S_WPBF2, SPR_WPBF, 1, 1, NULL, S_WPBF1, 0, 0},       // S_WPBF2
    {S_WPRF1, SPR_WPRF, 0, 1, NULL, S_WPRF2, 0, 0},       // S_WPRF1 - Waypoint Red Flag
    {S_WPRF2, SPR_WPRF, 1, 1, NULL, S_WPRF1, 0, 0},       // S_WPRF2
    {S_WPGF1, SPR_WPGF, 0, 1, NULL, S_WPGF2, 0, 0},       // S_WPGF1 - Waypoint Green Flag
    {S_WPGF2, SPR_WPGF, 1, 1, NULL, S_WPGF1, 0, 0},       // S_WPGF2

    {S_CARE, SPR_CARE, 0, -1, NULL, S_NULL, 0, 0}, // S_CARE - Horde Care Package

    {S_NOWEAPONUP, SPR_TNT1, 0, 1, A_Raise, S_NOWEAPON, 0, 0},       // S_NOWEAPONUP
    {S_NOWEAPONDOWN, SPR_TNT1, 0, 1, A_Lower, S_NOWEAPON, 0, 0},       // S_NOWEAPONDOWN
    {S_NOWEAPON, SPR_TNT1, 0, 1, A_WeaponReady, S_NOWEAPON, 0, 0}, // S_NOWEAPON
};

// reserved odamex sprites
// ::SPR_CARE - ::SPR_GIB0 + 2
const char* odasprnames[] = {
    "GIB0","GIB1","GIB2",
    "GIB3","GIB4","GIB5",
    "GIB6","GIB7","UNKN",
    //	[Toke - CTF]
	"BSOK","RSOK","BFLG",
    "RFLG","BDWN","RDWN",
    "BCAR","RCAR","GSOK", 
    "GFLG","GDWN","GCAR",
    "TLGL","WPBF","WPRF",
    "WPGF","CARE"
};

// reserved odamex mobjinfo
// ::MT_CAREPACK - ::MT_GIB0 + 1
mobjinfo_t odamobjinfo[] = {
    // ------------ odamex things start ------------ //
    {        //  MT_GIB0
	MT_GIB0,
    -1,        // doomednum
    S_GIB0,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB0"
    },

    {        //  MT_GIB1
	MT_GIB1,
    -1,        // doomednum
    S_GIB1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB1"
    },

    {        //  MT_GIB2
	MT_GIB2,
    -1,        // doomednum
    S_GIB2,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB2"
    },

    {        //  MT_GIB3
	MT_GIB3,
    -1,        // doomednum
    S_GIB3,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB3"
    },

    {        //  MT_GIB4
	MT_GIB4,
    -1,        // doomednum
    S_GIB4,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB4"
    },

    {        //  MT_GIB5
	MT_GIB5,
    -1,        // doomednum
    S_GIB5,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB5"
    },

    {        //  MT_GIB6
	MT_GIB6,
    -1,        // doomednum
    S_GIB6,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB6"
    },

    {        //  MT_GIB7
	MT_GIB7,
    -1,        // doomednum
    S_GIB7,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_DROPOFF|MF_CORPSE,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GIB7"
    },

    {        //  MT_UNKNOWNTHING
	MT_UNKNOWNTHING,
    -1,        // doomednum
    S_UNKNOWNTHING,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    32*FRACUNIT,        // radius
    56*FRACUNIT,        // height
    56*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0,
    "MT_UNKNOWNTHING"
    },

    {        // [RH] MT_PATHNODE -- used for monster patrols
    MT_PATHNODE,
    9024,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    8,        // radius
    8,        // height
    8*FRACUNIT,    // cdheight
    10,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0,
    "MT_PATHNODE -- used for monster patrols"
    },

    {        // [RH] MT_AMBIENT (subsumes MT_AMBIENT0-MT_AMBIENT63)
    MT_AMBIENT,
    14065,        // doomednum
    S_AMBIENTSOUND,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOSECTOR,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_AMBIENT (subsumes MT_AMBIENT0-MT_AMBIENT63)"
    },

    {        // [RH] MT_TELEPORTMAN2 - Height-sensitive teleport destination
    	MT_TELEPORTMAN2,
    9044,        // doomednum
    S_NULL,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_TELEPORTMAN2 - Height-sensitive teleport destination"
    },

    {        // [RH] MT_CAMERA
    	MT_CAMERA,
    9025,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOGRAVITY,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_CAMERA"
    },

    {        // [RH] MT_SPARK
    	MT_SPARK,
    9026,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOSECTOR|MF_NOBLOCKMAP|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0,
    "MT_SPARK"
    },

    {        // [RH] MT_FOUNTAIN
    	MT_FOUNTAIN,
    -1,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    0*FRACUNIT,        // height
    0*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0,
    "MT_FOUNTAIN"
    },

    {        //  MT_NODE   //Added by MC.
	MT_NODE,   //Added by MC.,
    786,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOSECTOR | MF_NOGRAVITY,        // flags  MF_NOSECTOR  Makes it invisible
    0,        // flags2
    S_NULL,   // raisestate
    0,
    "MT_NODE   //Added by MC."
    },

    {        //  MT_WATERZONE
	MT_WATERZONE,
    9045,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_WATERZONE"
    },

    {        //  MT_SECRETTRIGGER
	MT_SECRETTRIGGER,
    9046,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0,
    "MT_SECRETTRIGGER"
    },

            // [Toke - CTF] Blue Socket
    {        //  MT_BSOK
	MT_BSOK,
    5130,        // doomednum
    S_BSOK,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    14*FRACUNIT,        // height
    14*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BSOK"
    },
    
            // [Toke - CTF] Red Socket
    {        //  MT_RSOK
	MT_RSOK,
    5131,        // doomednum
    S_RSOK,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    14*FRACUNIT,        // height
    14*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_RSOK"
    },
    
    // Nes - Reserve 5132 for Neutral Socket


            // [Toke - CTF] Blue Flag
    {        //  MT_BFLG
	MT_BFLG,
    -1,        // doomednum
    S_BFLG,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BFLG"
    },

            // [Toke - CTF] Red Flag
    {        //  MT_RFLG
	MT_RFLG,
    -1,        // doomednum
    S_RFLG,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_RFLG"
    },

            // [Toke - CTF] Blue Dropped Flag
    {        //  MT_BDWN
	MT_BDWN,
    -1,        // doomednum
    S_BDWN,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BDWN"
    },

            // [Toke - CTF] Red Dropped Flag
    {        //  MT_RDWN
	MT_RDWN,
    -1,        // doomednum
    S_RDWN,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_RDWN"
    },

            // [Toke - CTF] Blue Carrying Flag
    {        //  MT_BCAR
	MT_BCAR,
    -1,        // doomednum
    S_BCAR,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BCAR"
    },

            // [Toke - CTF] Red Carrying Flag
    {        //  MT_RCAR
	MT_RCAR,
    -1,        // doomednum
    S_RCAR,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_RCAR"
    },

    {        //  MT_BRIDGE
	MT_BRIDGE,
    118,        // doomednum
    S_BRIDGE1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    36*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SOLID|MF_NOGRAVITY,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BRIDGE"
    },

    {        //  MT_MAPSPOT
	MT_MAPSPOT,
    9001,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_MAPSPOT"
    },

    {        //  MT_MAPSPOTGRAVITY
	MT_MAPSPOTGRAVITY,
    9013,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_MAPSPOTGRAV"
    },

    {        //  MT_BRIDGE32
	MT_BRIDGE32,
    5061,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    32*FRACUNIT,        // radius
    8*FRACUNIT,        // height
    8*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SOLID|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BRIDGE32"
    },
    
    {        //  MT_BRIDGE16
	MT_BRIDGE16,
    5064,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    8*FRACUNIT,        // height
    8*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SOLID|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BRIDGE16"
    },

    {        //  MT_BRIDGE8
	MT_BRIDGE8,
    5065,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    8*FRACUNIT,        // radius
    8*FRACUNIT,        // height
    8*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SOLID|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_BRIDGE8"
    },

    {        //  MT_ZDOOMBRIDGE
	MT_ZDOOMBRIDGE,
    9990,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    32*FRACUNIT,        // radius
    4*FRACUNIT,        // height
    4*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SOLID|MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_ZDOOMBRIDGE"
    },

    {        //  MT_SECACTENTER
	MT_SECACTENTER,
    9998,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTENTER"
    },

    {        //  MT_SECACTEXIT
	MT_SECACTEXIT,
    9997,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTEXIT"
    },

    {        //  MT_SECACTHITFLOOR
	MT_SECACTHITFLOOR,
    9999,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTHITFLOOR"
    },

    {        //  MT_SECACTHITCEIL
	MT_SECACTHITCEIL,
    9996,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTHITCEIL"
    },

    {        //  MT_SECACTUSE
	MT_SECACTUSE,
    9995,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTUSE"
    },

    {        //  MT_SECACTUSEWALL
	MT_SECACTUSEWALL,
    9994,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTUSEWALL"
    },

    {        //  MT_SECACTEYESDIVE
	MT_SECACTEYESDIVE,
    9993,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTEYESDIVE"
    },

    {        //  MT_SECACTEYESSURFACE
	MT_SECACTEYESSURFACE,
    9992,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTEYESSURFACE"
    },

    {        //  MT_SECACTEYESBELOWC
	MT_SECACTEYESBELOWC,
    9983,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTEYESBELOWC"
    },

    {        //  MT_SECACTEYESABOVEC
	MT_SECACTEYESABOVEC,
    9982,        // doomednum
    S_TNT1,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    16*FRACUNIT,        // radius
    16*FRACUNIT,        // height
    16*FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    0,        // flags
    MF2_DONTDRAW,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_SECACTEYESABOVEC"
    },

    {        //  MT_GSOK
	MT_GSOK,
    5133,        // doomednum
    S_GSOK,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20 * FRACUNIT,        // radius
    14 * FRACUNIT,        // height
    14 * FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GSOK"
    },

    {        //  MT_GFLG
	MT_GFLG,
    -1,        // doomednum
    S_GFLG,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20 * FRACUNIT,        // radius
    16 * FRACUNIT,        // height
    16 * FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GFLG"
    },

    {        //  MT_GDWN
	MT_GDWN,
    -1,        // doomednum
    S_GDWN,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20 * FRACUNIT,        // radius
    16 * FRACUNIT,        // height
    16 * FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_SPECIAL,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GDWN"
    },

    {        //  MT_GCAR
	MT_GCAR,
    -1,        // doomednum
    S_GCAR,        // spawnstate
    1000,        // spawnhealth
    0,        // gibhealth
    S_NULL,        // seestate
    NULL,        // seesound
    8,        // reactiontime
    NULL,        // attacksound
    S_NULL,        // painstate
    0,        // painchance
    NULL,        // painsound
    S_NULL,        // meleestate
    S_NULL,        // missilestate
    S_NULL,        // deathstate
    S_NULL,        // xdeathstate
    NULL,        // deathsound
    0,        // speed
    20 * FRACUNIT,        // radius
    16 * FRACUNIT,        // height
    16 * FRACUNIT,    // cdheight
    100,        // mass
    0,        // damage
    NULL,        // activesound
    MF_NOGRAVITY,        // flags
    0,        // flags2
    S_NULL,        // raisestate
    0x10000,
    "MT_GCAR"
    },
    {                  // MT_WPBFLAG
    	MT_WPBFLAG,
        -1,            // doomednum
        S_WPBF1,       // spawnstate
        1000,          // spawnhealth
        0,        // gibhealth
        S_NULL,        // seestate
        NULL,          // seesound
        8,             // reactiontime
        NULL,          // attacksound
        S_NULL,        // painstate
        0,             // painchance
        NULL,          // painsound
        S_NULL,        // meleestate
        S_NULL,        // missilestate
        S_NULL,        // deathstate
        S_NULL,        // xdeathstate
        NULL,          // deathsound
        0,             // speed
        20 * FRACUNIT, // radius
        16 * FRACUNIT, // height
        16 * FRACUNIT, // cdheight
        100,           // mass
        0,             // damage
        NULL,          // activesound
        0,             // flags
        0,             // flags2
        S_NULL,        // raisestate
        0xC000,
        "MT_WPBFLAG"
    },
    {                  // MT_WPRFLAG
    	MT_WPRFLAG,
        -1,            // doomednum
        S_WPRF1,       // spawnstate
        1000,          // spawnhealth
        0,        // gibhealth
        S_NULL,        // seestate
        NULL,          // seesound
        8,             // reactiontime
        NULL,          // attacksound
        S_NULL,        // painstate
        0,             // painchance
        NULL,          // painsound
        S_NULL,        // meleestate
        S_NULL,        // missilestate
        S_NULL,        // deathstate
        S_NULL,        // xdeathstate
        NULL,          // deathsound
        0,             // speed
        20 * FRACUNIT, // radius
        16 * FRACUNIT, // height
        16 * FRACUNIT, // cdheight
        100,           // mass
        0,             // damage
        NULL,          // activesound
        0,             // flags
        0,             // flags2
        S_NULL,        // raisestate
        0xC000,
        "MT_WPRFLAG"
    },
    {                  // MT_WPGFLAG
    	MT_WPGFLAG,
        -1,            // doomednum
        S_WPGF1,       // spawnstate
        1000,          // spawnhealth
        0,        // gibhealth
        S_NULL,        // seestate
        NULL,          // seesound
        8,             // reactiontime
        NULL,          // attacksound
        S_NULL,        // painstate
        0,             // painchance
        NULL,          // painsound
        S_NULL,        // meleestate
        S_NULL,        // missilestate
        S_NULL,        // deathstate
        S_NULL,        // xdeathstate
        NULL,          // deathsound
        0,             // speed
        20 * FRACUNIT, // radius
        16 * FRACUNIT, // height
        16 * FRACUNIT, // cdheight
        100,           // mass
        0,             // damage
        NULL,          // activesound
        0,             // flags
        0,             // flags2
        S_NULL,        // raisestate
        0xC000,
        "MT_WPGFLAG"
    },
    {                   // MT_AVATAR
    	MT_AVATAR,
        -1,             // doomednum
        S_PLAY,         // spawnstate
        100,            // spawnhealth
        0,        // gibhealth
        S_PLAY_RUN1,    // seestate
        NULL,           // seesound
        0,              // reactiontime
        NULL,           // a ttacksound
        S_PLAY_PAIN,    // painstate
        255,            // painchance
        "*pain100_1",   // painsound
        S_NULL,         // meleestate
        S_PLAY_ATK1,    // missilestate
        S_PLAY_DIE1,    // deathstate
        S_PLAY_XDIE1,   // xdeathstate
        "*death1",      // deathsound
        0,              // speed
        16*FRACUNIT,    // radius
        56*FRACUNIT,    // height
        56*FRACUNIT,    // cdheight
        100,            // mass
        0,              // damage
        NULL,           // activesound
        MF_SOLID|MF_SHOOTABLE|MF_DROPOFF|MF_PICKUP|MF_NOTDMATCH, // flags
        MF2_SLIDE|MF2_PASSMOBJ|MF2_PUSHWALL,                     // flags2
        S_NULL,         // raisestate
        0x10000,
        "MT_AVATAR"
    },
    {                   // MT_HORDESPAWN
    	MT_HORDESPAWN,
        -1,             // doomednum
        S_TNT1,         // spawnstate
        100,            // spawnhealth
        0,                // gibhealth
        S_NULL,         // seestate
        NULL,           // seesound
        0,              // reactiontime
        NULL,           // a ttacksound
        S_NULL,         // painstate
        0,              // painchance
        NULL,           // painsound
        S_NULL,         // meleestate
        S_NULL,         // missilestate
        S_NULL,         // deathstate
        S_NULL,         // xdeathstate
        NULL,           // deathsound
        0,              // speed
        16*FRACUNIT,    // radius
        16*FRACUNIT,    // height
        16*FRACUNIT,    // cdheight
        100,            // mass
        0,              // damage
        NULL,           // activesound
        MF_NOGRAVITY,   // flags
        0,              // flags2
        S_NULL,         // raisestate
        0x10000,
        "MT_HORDESPAWN"
    },
    {               // MT_CAREPACK
    	MT_CAREPACK,
        -1,             // doomednum
        S_CARE,         // spawnstate
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
        MF_SPECIAL,     // flags
        0,              // flags2
        S_NULL,         // raisestate
        0x10000,
        "MT_CAREPACK"
    },

	// ----------- odamex mobjinfo end -----------
};

// size functions

size_t odastates_size() {
	return ARRAY_LENGTH(odastates);
}

size_t odasprnames_size() {
	return ARRAY_LENGTH(odasprnames);
}

size_t odamobjinfo_size() {
	return ARRAY_LENGTH(odamobjinfo);
}

void D_Initialize_Odamex_Objects()
{
    statenum_t sidx = S_GIB0;
	size_t size = odastates_size();
    // states
	for (int i = 0; i < size; i++)
    {
		state_t* state = (state_t*) M_Calloc(1,sizeof(state_t));
		*state = odastates[i];
        states.insert(state, sidx);
        sidx = statenum_t(sidx + 1);
    }
#if defined _DEBUG
	Printf(PRINT_HIGH, "Loaded %d Odamex States\n", size);
#endif

    // mobjinfo
	size = odamobjinfo_size();
    mobjtype_t midx = MT_GIB0;
    for(int i = 0; i < odamobjinfo_size(); i++)
    {
    	mobjinfo_t* mobj = (mobjinfo_t*) M_Calloc(1,sizeof(mobjinfo_t));
    	*mobj = odamobjinfo[i];
        mobjinfo.insert(mobj, midx);
        midx = mobjtype_t(midx + 1);
    }
#if defined _DEBUG
	Printf(PRINT_HIGH, "Loaded %d Odamex Things\n", size);
#endif

    // sprites
	size = odasprnames_size();
    spritenum_t spidx = SPR_GIB0;
	for (int i = 0; i < size; i++)
    {
        sprnames.insert(odasprnames[i], spidx);
        spidx = spritenum_t(spidx + 1);
    }
#if defined _DEBUG
	Printf(PRINT_HIGH, "Loaded %d Odamex Sprites\n", size);
#endif
}
