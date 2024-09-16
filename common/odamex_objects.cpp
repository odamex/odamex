#include "odamex.h"
#include "m_fixed.h"
#include "actor.h"

#include "hashtable.h"

#define ODAIDX_START 0x80000000
#define ODAIX_END 0x8FFFFFFF

// code pointers
void A_Lower(AActor *);
void A_Raise(AActor *);
void A_WeaponReady(AActor *);
void A_Ambient(AActor*);

typedef OHashTable<int,int> IndexTable;

template<typename IdxType>
inline void init_idx_map(IdxType idxStart, IdxType idxEnd, int limit, IndexTable& table)
{
	IdxType idx = idxStart;
	for(int i = 0; (IdxType) i < idxEnd; i++)
	{
		table[idx] = limit + i;
		idx = IdxType(idx + 1);
	}
}

template void init_idx_map< statenum_t >(statenum_t, statenum_t, int, IndexTable&);
template void init_idx_map< spritenum_t >(spritenum_t, spritenum_t, int, IndexTable&);
template void init_idx_map< mobjtype_t >(mobjtype_t, mobjtype_t, int, IndexTable&);

// [CMB] these maps are used to associate the index associated with the doom object to the index in the lookup table

IndexTable odamex_states_indices_map;
IndexTable odamex_sprnames_indices_map;
IndexTable odamex_mobjinfo_indices_map;

// reserved odamex states
state_t odastates_reserved[] = {
    // ZDoom/Odamex stuff starts here

    {SPR_GIB0, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB0
    {SPR_GIB1, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB1
    {SPR_GIB2, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB2
    {SPR_GIB3, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB3
    {SPR_GIB4, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB4
    {SPR_GIB5, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB5
    {SPR_GIB6, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB6
    {SPR_GIB7, 0, -1, NULL, S_NULL, 0, 0},             // S_GIB7
    {SPR_TROO, 0, 1, A_Ambient, S_AMBIENTSOUND, 0, 0}, // S_AMBIENTSOUND
    {SPR_UNKN, 0, -1, NULL, S_NULL, 0, 0},             // S_UNKNOWNTHING

    //	[Toke - CTF]
    {SPR_BSOK, 0, -1, NULL, S_NULL, 0, 0},     // Blue Socket
    {SPR_RSOK, 0, -1, NULL, S_NULL, 0, 0},     // Red Socket
    {SPR_BFLG, 32768, 4, NULL, S_BFLG2, 0, 0}, // BLUE Flag Animation
    {SPR_BFLG, 32769, 4, NULL, S_BFLG3, 0, 0},
    {SPR_BFLG, 32770, 4, NULL, S_BFLG4, 0, 0},
    {SPR_BFLG, 32771, 4, NULL, S_BFLG5, 0, 0},
    {SPR_BFLG, 32772, 4, NULL, S_BFLG6, 0, 0},
    {SPR_BFLG, 32773, 4, NULL, S_BFLG7, 0, 0},
    {SPR_BFLG, 32774, 4, NULL, S_BFLG8, 0, 0},
    {SPR_BFLG, 32775, 4, NULL, S_BFLG, 0, 0},
    {SPR_RFLG, 32768, 4, NULL, S_RFLG2, 0, 0}, // RED Flag Animation
    {SPR_RFLG, 32769, 4, NULL, S_RFLG3, 0, 0},
    {SPR_RFLG, 32770, 4, NULL, S_RFLG4, 0, 0},
    {SPR_RFLG, 32771, 4, NULL, S_RFLG5, 0, 0},
    {SPR_RFLG, 32772, 4, NULL, S_RFLG6, 0, 0},
    {SPR_RFLG, 32773, 4, NULL, S_RFLG7, 0, 0},
    {SPR_RFLG, 32774, 4, NULL, S_RFLG8, 0, 0},
    {SPR_RFLG, 32775, 4, NULL, S_RFLG, 0, 0},
    {SPR_BDWN, 0, -1, NULL, S_NULL, 0, 0}, // Blue Dropped Flag
    {SPR_RDWN, 0, -1, NULL, S_NULL, 0, 0}, // Red Dropped Flag
    {SPR_BCAR, 0, -1, NULL, S_NULL, 0, 0}, // Blue Dropped Flag
    {SPR_RCAR, 0, -1, NULL, S_NULL, 0, 0}, // Red Dropped Flag

    {SPR_GSOK, 0, -1, NULL, S_NULL, 0, 0},     // S_GSOK,
    {SPR_GFLG, 32768, 4, NULL, S_GFLG2, 0, 0}, // BLUE Flag Animation
    {SPR_GFLG, 32769, 4, NULL, S_GFLG3, 0, 0},
    {SPR_GFLG, 32770, 4, NULL, S_GFLG4, 0, 0},
    {SPR_GFLG, 32771, 4, NULL, S_GFLG5, 0, 0},
    {SPR_GFLG, 32772, 4, NULL, S_GFLG6, 0, 0},
    {SPR_GFLG, 32773, 4, NULL, S_GFLG7, 0, 0},
    {SPR_GFLG, 32774, 4, NULL, S_GFLG8, 0, 0},
    {SPR_GFLG, 32775, 4, NULL, S_GFLG, 0, 0},
    {SPR_GDWN, 0, -1, NULL, S_NULL, 0, 0}, // S_GDWN,
    {SPR_GCAR, 0, -1, NULL, S_NULL, 0, 0}, // S_GCAR,

    {SPR_TLGL, 32768, 4, NULL, S_BRIDGE2, 0, 0}, // S_BRIDGE1
    {SPR_TLGL, 32769, 4, NULL, S_BRIDGE3, 0, 0}, // S_BRIDGE2
    {SPR_TLGL, 32770, 4, NULL, S_BRIDGE4, 0, 0}, // S_BRIDGE3
    {SPR_TLGL, 32771, 4, NULL, S_BRIDGE5, 0, 0}, // S_BRIDGE4
    {SPR_TLGL, 32772, 4, NULL, S_BRIDGE1, 0, 0}, // S_BRIDGE5

    {SPR_WPBF, 0, 1, NULL, S_WPBF2, 0, 0},       // S_WPBF1 - Waypoint Blue Flag
    {SPR_WPBF, 1, 1, NULL, S_WPBF1, 0, 0},       // S_WPBF2
    {SPR_WPRF, 0, 1, NULL, S_WPRF2, 0, 0},       // S_WPRF1 - Waypoint Red Flag
    {SPR_WPRF, 1, 1, NULL, S_WPRF1, 0, 0},       // S_WPRF2
    {SPR_WPGF, 0, 1, NULL, S_WPGF2, 0, 0},       // S_WPGF1 - Waypoint Green Flag
    {SPR_WPGF, 1, 1, NULL, S_WPGF1, 0, 0},       // S_WPGF2

    {SPR_CARE, 0, -1, NULL, S_NULL, 0, 0}, // S_CARE - Horde Care Package

    {SPR_TNT1, 0, 1, A_Raise, S_NOWEAPON, 0, 0},       // S_NOWEAPONUP
    {SPR_TNT1, 0, 1, A_Lower, S_NOWEAPON, 0, 0},       // S_NOWEAPONDOWN
    {SPR_TNT1, 0, 1, A_WeaponReady, S_NOWEAPON, 0, 0}, // S_NOWEAPON
};
// reserved odamex sprites
const char* odasprnames[::SPR_CARE - ::SPR_GIB0 + 2] = {
    "GIB0","GIB1","GIB2",
    "GIB3","GIB4","GIB5",
    "GIB6","GIB7","UNKN",
    //	[Toke - CTF]
	"BSOK","RSOK","BFLG",
    "RFLG","BDWN","RDWN",
    "BCAR","RCAR","GSOK", 
    "GFLG","GDWN","GCAR",
    "TLGL","WPBF","WPRF",
    "WPGF","CARE", NULL
};
// reserved odamex mobjinfo
mobjinfo_t odamobjinfop[::MT_CAREPACK - ::MT_GIB0 + 1] = {
    // ----------- odamex mobjinfo start -----------
	{		// MT_GIB0
	-1,		// doomednum
	S_GIB0,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB0"
	},

	{		// MT_GIB1
	-1,		// doomednum
	S_GIB1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB1"
	},

	{		// MT_GIB2
	-1,		// doomednum
	S_GIB2,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB2"
	},

	{		// MT_GIB3
	-1,		// doomednum
	S_GIB3,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB3"
	},

	{		// MT_GIB4
	-1,		// doomednum
	S_GIB4,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB4"
	},

	{		// MT_GIB5
	-1,		// doomednum
	S_GIB5,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB5"
	},

	{		// MT_GIB6
	-1,		// doomednum
	S_GIB6,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB6"
	},

	{		// MT_GIB7
	-1,		// doomednum
	S_GIB7,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_DROPOFF|MF_CORPSE,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GIB7"
	},

	{		// MT_UNKNOWNTHING
	-1,		// doomednum
	S_UNKNOWNTHING,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	32*FRACUNIT,		// radius
	56*FRACUNIT,		// height
	56*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0,
	"MT_UNKNOWNTHING"
	},
			// [Toke - CTF] Blue Socket
	{		// MT_BSOK
	5130,		// doomednum
	S_BSOK,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	14*FRACUNIT,		// height
	14*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BSOK"
	},
	
			// [Toke - CTF] Red Socket
	{		// MT_RSOK
	5131,		// doomednum
	S_RSOK,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	14*FRACUNIT,		// height
	14*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_RSOK"
	},
	
	// Nes - Reserve 5132 for Neutral Socket


			// [Toke - CTF] Blue Flag
	{		// MT_BFLG
	-1,		// doomednum
	S_BFLG,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BFLG"
	},

			// [Toke - CTF] Red Flag
	{		// MT_RFLG
	-1,		// doomednum
	S_RFLG,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_RFLG"
	},

			// [Toke - CTF] Blue Dropped Flag
	{		// MT_BDWN
	-1,		// doomednum
	S_BDWN,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BDWN"
	},

			// [Toke - CTF] Red Dropped Flag
	{		// MT_RDWN
	-1,		// doomednum
	S_RDWN,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_RDWN"
	},

			// [Toke - CTF] Blue Carrying Flag
	{		// MT_BCAR
	-1,		// doomednum
	S_BCAR,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BCAR"
	},

			// [Toke - CTF] Red Carrying Flag
	{		// MT_RCAR
	-1,		// doomednum
	S_RCAR,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_RCAR"
	},

	{		// MT_BRIDGE
	118,		// doomednum
	S_BRIDGE1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	36*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SOLID|MF_NOGRAVITY,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BRIDGE"
	},

	{		// MT_MAPSPOT
	9001,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_NOBLOCKMAP|MF_NOSECTOR|MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_MAPSPOT"
	},

	{		// MT_MAPSPOTGRAV
	9013,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_MAPSPOTGRAV"
	},

	{		// MT_BRIDGE32
	5061,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	32*FRACUNIT,		// radius
	8*FRACUNIT,		// height
	8*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SOLID|MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BRIDGE32"
	},
	
	{		// MT_BRIDGE16
	5064,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	8*FRACUNIT,		// height
	8*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SOLID|MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BRIDGE16"
	},

	{		// MT_BRIDGE8
	5065,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	8*FRACUNIT,		// radius
	8*FRACUNIT,		// height
	8*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SOLID|MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_BRIDGE8"
	},

	{		// MT_ZDOOMBRIDGE
	9990,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	32*FRACUNIT,		// radius
	4*FRACUNIT,		// height
	4*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SOLID|MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_ZDOOMBRIDGE"
	},

	{		// MT_SECACTENTER
	9998,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTENTER"
	},

	{		// MT_SECACTEXIT
	9997,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTEXIT"
	},

	{		// MT_SECACTHITFLOOR
	9999,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTHITFLOOR"
	},

	{		// MT_SECACTHITCEIL
	9996,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTHITCEIL"
	},

	{		// MT_SECACTUSE
	9995,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTUSE"
	},

	{		// MT_SECACTUSEWALL
	9994,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTUSEWALL"
	},

	{		// MT_SECACTEYESDIVE
	9993,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTEYESDIVE"
	},

	{		// MT_SECACTEYESSURFACE
	9992,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTEYESSURFACE"
	},

	{		// MT_SECACTEYESBELOWC
	9983,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTEYESBELOWC"
	},

	{		// MT_SECACTEYESABOVEC
	9982,		// doomednum
	S_TNT1,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	16*FRACUNIT,		// radius
	16*FRACUNIT,		// height
	16*FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	0,		// flags
	MF2_DONTDRAW,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_SECACTEYESABOVEC"
	},

	{		// MT_GSOK
	5133,		// doomednum
	S_GSOK,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20 * FRACUNIT,		// radius
	14 * FRACUNIT,		// height
	14 * FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GSOK"
	},

	{		// MT_GFLG
	-1,		// doomednum
	S_GFLG,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20 * FRACUNIT,		// radius
	16 * FRACUNIT,		// height
	16 * FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GFLG"
	},

	{		// MT_GDWN
	-1,		// doomednum
	S_GDWN,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20 * FRACUNIT,		// radius
	16 * FRACUNIT,		// height
	16 * FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_SPECIAL,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GDWN"
	},

	{		// MT_GCAR
	-1,		// doomednum
	S_GCAR,		// spawnstate
	1000,		// spawnhealth
	0,		// gibhealth
	S_NULL,		// seestate
	NULL,		// seesound
	8,		// reactiontime
	NULL,		// attacksound
	S_NULL,		// painstate
	0,		// painchance
	NULL,		// painsound
	S_NULL,		// meleestate
	S_NULL,		// missilestate
	S_NULL,		// deathstate
	S_NULL,		// xdeathstate
	NULL,		// deathsound
	0,		// speed
	20 * FRACUNIT,		// radius
	16 * FRACUNIT,		// height
	16 * FRACUNIT,	// cdheight
	100,		// mass
	0,		// damage
	NULL,		// activesound
	MF_NOGRAVITY,		// flags
	0,		// flags2
	S_NULL,		// raisestate
	0x10000,
	"MT_GCAR"
	},
	{                  // MT_WPBFLAG
		-1,            // doomednum
		S_WPBF1,       // spawnstate
		1000,          // spawnhealth
		0,		// gibhealth
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
		-1,            // doomednum
		S_WPRF1,       // spawnstate
		1000,          // spawnhealth
		0,		// gibhealth
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
		-1,            // doomednum
		S_WPGF1,       // spawnstate
		1000,          // spawnhealth
		0,		// gibhealth
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
		-1,             // doomednum
		S_PLAY,         // spawnstate
		100,            // spawnhealth
		0,		// gibhealth
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
		-1,             // doomednum
		S_TNT1,         // spawnstate
		100,            // spawnhealth
		0,				// gibhealth
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
	}
	// ----------- odamex mobjinfo end -----------
};

void init_Odamex_Indices(int odastates_limit, int odasprites_limit, int odamobjinfo_limit)
{
	init_idx_map(S_GIB0, S_NOWEAPON, odastates_limit, odamex_states_indices_map);
	init_idx_map(SPR_GIB0, SPR_CARE, odasprites_limit, odamex_sprnames_indices_map);
	init_idx_map(MT_GIB0, MT_CAREPACK, odamobjinfo_limit, odamex_mobjinfo_indices_map);
}

state_t* D_GetOdaState(statenum_t statenum)
{
    IndexTable::iterator found = odamex_states_indices_map.find(statenum);
    if (found == odamex_states_indices_map.end()) return nullptr;
	
	int realIdx = found->second;
	return &odastates_reserved[realIdx];
}

mobjinfo_t* D_GetOdaMobjinfo(mobjtype_t mobjtype)
{
    IndexTable::iterator found = odamex_mobjinfo_indices_map.find(mobjtype);
	if (found == odamex_mobjinfo_indices_map.end()) return nullptr;

	int realIdx = found->second;
    return &odamobjinfop[realIdx];
}

const char* D_GetOdaSprName(spritenum_t spritenum)
{
	IndexTable::iterator found = odamex_sprnames_indices_map.find(spritenum);
	if (found == odamex_sprnames_indices_map.end()) return nullptr;
	
    int realIdx = found->second;
    return odasprnames[realIdx];
}
