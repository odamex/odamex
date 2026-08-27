// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Internal DeHackEd patch parsing
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <array>
#include <stdlib.h>
#include <sstream>
#include <variant>
#include <string.h>
#include <nonstd/scope.hpp>
#include <span>

#include "cmdlib.h"
#include "c_dispatch.h"
#include "d_dehacked.h"
#include "m_doomobjcontainer.h"
#include "d_items.h"
#include "gstrings.h"
#include "i_system.h"
#include "info.h"
#include "m_alloc.h"
#include "m_cheat.h"
#include "m_fileio.h"
#include "p_local.h"
#include "s_sound.h"
#include "w_wad.h"
#include "infomap.h"

// These are the original heights of every Doom 2 thing. They are used if a patch
// specifies that a thing should be hanging from the ceiling but doesn't specify
// a height for the thing, since these are the heights it probably wants.

namespace
{

constexpr byte OrgHeights[] = {
    56, 56,  56, 56, 16, 56, 8,  16, 64, 8,  56, 56, 56, 56, 56, 64, 8,  64, 56, 100,
    64, 110, 56, 56, 72, 16, 32, 32, 32, 16, 42, 8,  8,  8,  8,  8,  8,  16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 68,  84, 84, 68, 52, 84, 68, 52, 52, 68, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 88, 88, 64, 64, 64, 64, 16, 16, 16
};

// English strings for DeHackEd replacement.
StringTable ENGStrings;

// This is an offset to be used for computing the text stuff.
// Straight from the DeHackEd source which was
// Written by Greg Lewis, gregl@umich.edu.
constexpr std::array<int, 3> toff = {129044, 129284, 129380};

// A conversion array to convert from the 448 code pointers to the 966
// Frames that exist.
// Again taken from the DeHackEd source.
constexpr std::array<int16_t, 522> codepconv = {
    1, 2, 3, 4, 6, 9, 10, 11, 12, 14, 16, 17, 18, 19, 20, 22, 29, 30, 31, 32, 33, 34, 36,
    38, 39, 41, 43, 44, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
    63, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
    85, 86, 87, 88, 89, 119, 127, 157, 159, 160, 166, 167, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 188, 190, 191, 195, 196, 207, 208, 209, 210, 211, 212,
    213, 214, 215, 216, 217, 218, 221, 223, 224, 228, 229, 241, 242, 243, 244, 245, 246,
    247, 248, 249, 250, 251, 252, 253, 254, 255, 256, 257, 258, 259, 260, 261, 262, 263,
    264, 270, 272, 273, 281, 282, 283, 284, 285, 286, 287, 288, 289, 290, 291, 292, 293,
    294, 295, 296, 297, 298, 299, 300, 301, 302, 303, 304, 305, 306, 307, 308, 309, 310,
    316, 317, 321, 322, 323, 324, 325, 326, 327, 328, 329, 330, 331, 332, 333, 334, 335,
    336, 337, 338, 339, 340, 341, 342, 344, 347, 348, 362, 363, 364, 365, 366, 367, 368,
    369, 370, 371, 372, 373, 374, 375, 376, 377, 378, 379, 380, 381, 382, 383, 384, 385,
    387, 389, 390, 397, 406, 407, 408, 409, 410, 411, 412, 413, 414, 415, 416, 417, 418,
    419, 421, 423, 424, 430, 431, 442, 443, 444, 445, 446, 447, 448, 449, 450, 451, 452,
    453, 454, 456, 458, 460, 463, 465, 475, 476, 477, 478, 479, 480, 481, 482, 483, 484,
    485, 486, 487, 489, 491, 493, 502, 503, 504, 505, 506, 508, 511, 514, 527, 528, 529,
    530, 531, 532, 533, 534, 535, 536, 537, 538, 539, 541, 543, 545, 548, 556, 557, 558,
    559, 560, 561, 562, 563, 564, 565, 566, 567, 568, 570, 572, 574, 585, 586, 587, 588,
    589, 590, 594, 596, 598, 601, 602, 603, 604, 605, 606, 607, 608, 609, 610, 611, 612,
    613, 614, 615, 616, 617, 618, 620, 621, 622, 631, 632, 633, 635, 636, 637, 638, 639,
    640, 641, 642, 643, 644, 645, 646, 647, 648, 650, 652, 653, 654, 659, 674, 675, 676,
    677, 678, 679, 680, 681, 682, 683, 684, 685, 686, 687, 688, 689, 690, 692, 696, 700,
    701, 702, 703, 704, 705, 706, 707, 708, 709, 710, 711, 713, 715, 718, 726, 727, 728,
    729, 730, 731, 732, 733, 734, 735, 736, 737, 738, 739, 740, 741, 743, 745, 746, 750,
    751, 766, 774, 777, 779, 780, 783, 784, 785, 786, 787, 788, 789, 790, 791, 792, 793,
    794, 795, 796, 797, 798, 801, 809, 811,

    // Now for the 74 MBF states with code pointers
    968, 969, 970, 972, 973, 974, 975, 976, 977, 978, 979, 980, 981, 982, 983, 984, 986,
    988, 990, 999, 1000, 1001, 1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010, 1011,
    1012, 1013, 1014, 1015, 1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025,
    1026, 1027, 1028, 1029, 1030, 1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038, 1039,
    1040, 1041, 1056, 1057, 1058, 1059, 1060, 1061, 1062, 1065, 1071, 1073, 1074,
    1075 // Total: 522
};

bool BackedUpData = false;
// This is the original data before it gets replaced by a patch.
std::array<std::string, ::NUMSPRITES> OrgSprNames;
std::array<actionf_p1, ::NUMSTATES> OrgActionPtrs;

} // namespace

// Functions used in a .bex [CODEPTR] chunk
void A_FireRailgun(AActor*);
void A_FireRailgunLeft(AActor*);
void A_FireRailgunRight(AActor*);
void A_RailWait(AActor*);
void A_Light0(AActor*);
void A_WeaponReady(AActor*);
void A_Lower(AActor*);
void A_Raise(AActor*);
void A_Punch(AActor*);
void A_ReFire(AActor*);
void A_FirePistol(AActor*);
void A_Light1(AActor*);
void A_FireShotgun(AActor*);
void A_Light2(AActor*);
void A_FireShotgun2(AActor*);
void A_CheckReload(AActor*);
void A_OpenShotgun2(AActor*);
void A_LoadShotgun2(AActor*);
void A_CloseShotgun2(AActor*);
void A_FireCGun(AActor*);
void A_GunFlash(AActor*);
void A_FireMissile(AActor*);
void A_Saw(AActor*);
void A_FirePlasma(AActor*);
void A_BFGsound(AActor*);
void A_FireBFG(AActor*);
void A_BFGSpray(AActor*);
void A_Explode(AActor*);
void A_Pain(AActor*);
void A_PlayerScream(AActor*);
void A_Fall(AActor*);
void A_XScream(AActor*);
void A_Look(AActor*);
void A_Chase(AActor*);
void A_FaceTarget(AActor*);
void A_PosAttack(AActor*);
void A_Scream(AActor*);
void A_SPosAttack(AActor*);
void A_VileChase(AActor*);
void A_VileStart(AActor*);
void A_VileTarget(AActor*);
void A_VileAttack(AActor*);
void A_StartFire(AActor*);
void A_Fire(AActor*);
void A_FireCrackle(AActor*);
void A_Tracer(AActor*);
void A_SkelWhoosh(AActor*);
void A_SkelFist(AActor*);
void A_SkelMissile(AActor*);
void A_FatRaise(AActor*);
void A_FatAttack1(AActor*);
void A_FatAttack2(AActor*);
void A_FatAttack3(AActor*);
void A_BossDeath(AActor*);
void A_CPosAttack(AActor*);
void A_CPosRefire(AActor*);
void A_TroopAttack(AActor*);
void A_SargAttack(AActor*);
void A_HeadAttack(AActor*);
void A_BruisAttack(AActor*);
void A_SkullAttack(AActor*);
void A_Metal(AActor*);
void A_SpidRefire(AActor*);
void A_BabyMetal(AActor*);
void A_BspiAttack(AActor*);
void A_Hoof(AActor*);
void A_CyberAttack(AActor*);
void A_PainAttack(AActor*);
void A_PainDie(AActor*);
void A_KeenDie(AActor*);
void A_BrainPain(AActor*);
void A_BrainScream(AActor*);
void A_BrainDie(AActor*);
void A_BrainAwake(AActor*);
void A_BrainSpit(AActor*);
void A_SpawnSound(AActor*);
void A_SpawnFly(AActor*);
void A_BrainExplode(AActor*);
void A_MonsterRail(AActor*);
void A_Detonate(AActor*);
void A_Mushroom(AActor*);
void A_Die(AActor*);
void A_Spawn(AActor*);
void A_Turn(AActor*);
void A_Face(AActor*);
void A_Scratch(AActor*);
void A_PlaySound(AActor*);
void A_RandomJump(AActor*);
void A_LineEffect(AActor*);
void A_BetaSkullAttack(AActor* actor);
void A_Stop(AActor*);

// MBF21
void A_SpawnObject(AActor*);
void A_MonsterProjectile(AActor* actor);
void A_MonsterBulletAttack(AActor* actor);
void A_MonsterMeleeAttack(AActor* actor);
void A_RadiusDamage(AActor* actor);
void A_NoiseAlert(AActor* actor);
void A_HealChase(AActor* actor);
void A_SeekTracer(AActor* actor);
void A_FindTracer(AActor* actor);
void A_ClearTracer(AActor* actor);
void A_JumpIfHealthBelow(AActor* actor);
void A_JumpIfTargetInSight(AActor* actor);
void A_JumpIfTargetCloser(AActor* actor);
void A_JumpIfTracerInSight(AActor* actor);
void A_JumpIfTracerCloser(AActor* actor);
void A_JumpIfFlagsSet(AActor* actor);
void A_AddFlags(AActor* actor);
void A_RemoveFlags(AActor* actor);
// MBF21 Weapons
void A_ConsumeAmmo(AActor* mo);
void A_CheckAmmo(AActor* mo);
void A_WeaponJump(AActor* mo);
void A_WeaponProjectile(AActor* mo);
void A_WeaponBulletAttack(AActor* actor);
void A_WeaponMeleeAttack(AActor* actor);
void A_WeaponAlert(AActor* actor);
void A_WeaponSound(AActor* actor);
void A_RefireTo(AActor* mo);
void A_GunFlashTo(AActor* mo);

struct CodePtr
{
	const char* name;
	actionf_p1 func = nullptr;
	int argcount = 0;
	std::array<statearg_t, MAXSTATEARGS> default_args{};
};

namespace
{

constexpr std::array<CodePtr, 120> CodePtrs = {{
    {.name = "NULL"},
    {.name = "MonsterRail",      .func = A_MonsterRail     },
    {.name = "FireRailgun",      .func = A_FireRailgun     },
    {.name = "FireRailgunLeft",  .func = A_FireRailgunLeft },
    {.name = "FireRailgunRight", .func = A_FireRailgunRight},
    {.name = "RailWait",         .func = A_RailWait        },
    {.name = "Light0",           .func = A_Light0          },
    {.name = "WeaponReady",      .func = A_WeaponReady     },
    {.name = "Lower",            .func = A_Lower           },
    {.name = "Raise",            .func = A_Raise           },
    {.name = "Punch",            .func = A_Punch           },
    {.name = "ReFire",           .func = A_ReFire          },
    {.name = "FirePistol",       .func = A_FirePistol      },
    {.name = "Light1",           .func = A_Light1          },
    {.name = "FireShotgun",      .func = A_FireShotgun     },
    {.name = "Light2",           .func = A_Light2          },
    {.name = "FireShotgun2",     .func = A_FireShotgun2    },
    {.name = "CheckReload",      .func = A_CheckReload     },
    {.name = "OpenShotgun2",     .func = A_OpenShotgun2    },
    {.name = "LoadShotgun2",     .func = A_LoadShotgun2    },
    {.name = "CloseShotgun2",    .func = A_CloseShotgun2   },
    {.name = "FireCGun",         .func = A_FireCGun        },
    {.name = "GunFlash",         .func = A_GunFlash        },
    {.name = "FireMissile",      .func = A_FireMissile     },
    {.name = "Saw",              .func = A_Saw             },
    {.name = "FirePlasma",       .func = A_FirePlasma      },
    {.name = "BFGsound",         .func = A_BFGsound        },
    {.name = "FireBFG",          .func = A_FireBFG         },
    {.name = "BFGSpray",         .func = A_BFGSpray        },
    {.name = "Explode",          .func = A_Explode         },
    {.name = "Pain",             .func = A_Pain            },
    {.name = "PlayerScream",     .func = A_PlayerScream    },
    {.name = "Fall",             .func = A_Fall            },
    {.name = "XScream",          .func = A_XScream         },
    {.name = "Look",             .func = A_Look            },
    {.name = "Chase",            .func = A_Chase           },
    {.name = "FaceTarget",       .func = A_FaceTarget      },
    {.name = "PosAttack",        .func = A_PosAttack       },
    {.name = "Scream",           .func = A_Scream          },
    {.name = "SPosAttack",       .func = A_SPosAttack      },
    {.name = "VileChase",        .func = A_VileChase       },
    {.name = "VileStart",        .func = A_VileStart       },
    {.name = "VileTarget",       .func = A_VileTarget      },
    {.name = "VileAttack",       .func = A_VileAttack      },
    {.name = "StartFire",        .func = A_StartFire       },
    {.name = "Fire",             .func = A_Fire            },
    {.name = "FireCrackle",      .func = A_FireCrackle     },
    {.name = "Tracer",           .func = A_Tracer          },
    {.name = "SkelWhoosh",       .func = A_SkelWhoosh      },
    {.name = "SkelFist",         .func = A_SkelFist        },
    {.name = "SkelMissile",      .func = A_SkelMissile     },
    {.name = "FatRaise",         .func = A_FatRaise        },
    {.name = "FatAttack1",       .func = A_FatAttack1      },
    {.name = "FatAttack2",       .func = A_FatAttack2      },
    {.name = "FatAttack3",       .func = A_FatAttack3      },
    {.name = "BossDeath",        .func = A_BossDeath       },
    {.name = "CPosAttack",       .func = A_CPosAttack      },
    {.name = "CPosRefire",       .func = A_CPosRefire      },
    {.name = "TroopAttack",      .func = A_TroopAttack     },
    {.name = "SargAttack",       .func = A_SargAttack      },
    {.name = "HeadAttack",       .func = A_HeadAttack      },
    {.name = "BruisAttack",      .func = A_BruisAttack     },
    {.name = "SkullAttack",      .func = A_SkullAttack     },
    {.name = "Metal",            .func = A_Metal           },
    {.name = "SpidRefire",       .func = A_SpidRefire      },
    {.name = "BabyMetal",        .func = A_BabyMetal       },
    {.name = "BspiAttack",       .func = A_BspiAttack      },
    {.name = "Hoof",             .func = A_Hoof            },
    {.name = "CyberAttack",      .func = A_CyberAttack     },
    {.name = "PainAttack",       .func = A_PainAttack      },
    {.name = "PainDie",          .func = A_PainDie         },
    {.name = "KeenDie",          .func = A_KeenDie         },
    {.name = "BrainPain",        .func = A_BrainPain       },
    {.name = "BrainScream",      .func = A_BrainScream     },
    {.name = "BrainDie",         .func = A_BrainDie        },
    {.name = "BrainAwake",       .func = A_BrainAwake      },
    {.name = "BrainSpit",        .func = A_BrainSpit       },
    {.name = "SpawnSound",       .func = A_SpawnSound      },
    {.name = "SpawnFly",         .func = A_SpawnFly        },
    {.name = "BrainExplode",     .func = A_BrainExplode    },
    {.name = "Detonate",         .func = A_Detonate        }, // killough 8/9/98
    {.name = "Mushroom",         .func = A_Mushroom        }, // killough 10/98
    {.name = "Die",              .func = A_Die             }, // killough 11/98
    {.name = "Spawn",            .func = A_Spawn           }, // killough 11/98
    {.name = "Turn",             .func = A_Turn            }, // killough 11/98
    {.name = "Face",             .func = A_Face            }, // killough 11/98
    {.name = "Scratch",          .func = A_Scratch         }, // killough 11/98
    {.name = "PlaySound",        .func = A_PlaySound       }, // killough 11/98
    {.name = "RandomJump",       .func = A_RandomJump      }, // killough 11/98
    {.name = "LineEffect",       .func = A_LineEffect      }, // killough 11/98
    {.name = "BetaSkullAttack",  .func = A_BetaSkullAttack },
	{.name = "Stop",             .func = A_Stop            },

    // MBF21 Pointers
    {.name = "SpawnObject",         .func = A_SpawnObject,         .argcount = 8                                              },
    {.name = "MonsterProjectile",   .func = A_MonsterProjectile,   .argcount = 5                                              },
    {.name = "MonsterBulletAttack", .func = A_MonsterBulletAttack, .argcount = 5, .default_args = {0, 0, 1, 3, 5, 0, 0, 0}    },
    {.name = "MonsterMeleeAttack",  .func = A_MonsterMeleeAttack,  .argcount = 4, .default_args = {3, 8, 0, 0, 0, 0, 0, 0}    },
    {.name = "RadiusDamage",        .func = A_RadiusDamage,        .argcount = 2                                              },
    {.name = "NoiseAlert",          .func = A_NoiseAlert                                                                      },
    {.name = "HealChase",           .func = A_HealChase,           .argcount = 2                                              },
    {.name = "SeekTracer",          .func = A_SeekTracer,          .argcount = 2                                              },
    {.name = "FindTracer",          .func = A_FindTracer,          .argcount = 2, .default_args = {0, 10, 0, 0, 0, 0, 0, 0}   },
    {.name = "ClearTracer",         .func = A_ClearTracer                                                                     },
    {.name = "JumpIfHealthBelow",   .func = A_JumpIfHealthBelow,   .argcount = 2                                              },
    {.name = "JumpIfTargetInSight", .func = A_JumpIfTargetInSight, .argcount = 2                                              },
    {.name = "JumpIfTargetCloser",  .func = A_JumpIfTargetCloser,  .argcount = 2                                              },
    {.name = "JumpIfTracerInSight", .func = A_JumpIfTracerInSight, .argcount = 2                                              },
    {.name = "JumpIfTracerCloser",  .func = A_JumpIfTracerCloser,  .argcount = 2                                              },
    {.name = "JumpIfFlagsSet",      .func = A_JumpIfFlagsSet,      .argcount = 3                                              },
    {.name = "AddFlags",            .func = A_AddFlags,            .argcount = 2                                              },
    {.name = "RemoveFlags",         .func = A_RemoveFlags,         .argcount = 2                                              },
    // MBF21 Weapon Pointers
    {.name = "WeaponProjectile",    .func = A_WeaponProjectile,    .argcount = 5                                              },
    {.name = "WeaponBulletAttack",  .func = A_WeaponBulletAttack,  .argcount = 5, .default_args = {0, 0, 1, 5, 3, 0, 0, 0}    },
    {.name = "WeaponMeleeAttack",   .func = A_WeaponMeleeAttack,   .argcount = 5, .default_args = {2, 10, 1_fx, 0, 0, 0, 0, 0}},
    {.name = "WeaponSound",         .func = A_WeaponSound,         .argcount = 2                                              },
    {.name = "WeaponAlert",         .func = A_WeaponAlert,         .argcount = 0                                              },
    {.name = "WeaponJump",          .func = A_WeaponJump,          .argcount = 2                                              },
    {.name = "ConsumeAmmo",         .func = A_ConsumeAmmo,         .argcount = 1                                              },
    {.name = "CheckAmmo",           .func = A_CheckAmmo,           .argcount = 2                                              },
    {.name = "RefireTo",            .func = A_RefireTo,            .argcount = 2                                              },
    {.name = "GunFlashTo",          .func = A_GunFlashTo,          .argcount = 2                                              },
}};

struct flag_mapper_t
{
	std::string_view name;
	int32_t dehBit;
	void (*setter)(mobjinfo_t&);
};

// for mapping dehacked MBF21 Bits property to the actual internal flags
// dehBit is the bit used in a numeric MBF21 Bits, setter sets corresponding internal flag
constexpr std::array<flag_mapper_t, 19> mbf21flagtranslation = {{
	// flags2
	{ .name = "LOGRAV",         .dehBit = BIT(0),  .setter = [](mobjinfo_t& info) { info.flags2 |= MF2_LOGRAV; } },
	{ .name = "BOSS",           .dehBit = BIT(9),  .setter = [](mobjinfo_t& info) { info.flags2 |= MF2_BOSS; } },
	{ .name = "RIP",            .dehBit = BIT(17), .setter = [](mobjinfo_t& info) { info.flags2 |= MF2_RIP; } },
	// flags3
	{ .name = "SHORTMRANGE",    .dehBit = BIT(1),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_SHORTMRANGE; } },
	{ .name = "DMGIGNORED",     .dehBit = BIT(2),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_DMGIGNORED; } },
	{ .name = "NORADIUSDMG",    .dehBit = BIT(3),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_NORADIUSDMG; } },
	{ .name = "FORCERADIUSDMG", .dehBit = BIT(4),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_FORCERADIUSDMG; } },
	{ .name = "HIGHERMPROB",    .dehBit = BIT(5),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_HIGHERMPROB; } },
	{ .name = "RANGEHALF",      .dehBit = BIT(6),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_RANGEHALF; } },
	{ .name = "NOTHRESHOLD",    .dehBit = BIT(7),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_NOTHRESHOLD; } },
	{ .name = "LONGMELEE",      .dehBit = BIT(8),  .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_LONGMELEE; } },
	{ .name = "MAP07BOSS1",     .dehBit = BIT(10), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_MAP07BOSS1; } },
	{ .name = "MAP07BOSS2",     .dehBit = BIT(11), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_MAP07BOSS2; } },
	{ .name = "E1M8BOSS",       .dehBit = BIT(12), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_E1M8BOSS; } },
	{ .name = "E2M8BOSS",       .dehBit = BIT(13), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_E2M8BOSS; } },
	{ .name = "E3M8BOSS",       .dehBit = BIT(14), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_E3M8BOSS; } },
	{ .name = "E4M6BOSS",       .dehBit = BIT(15), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_E4M6BOSS; } },
	{ .name = "E4M8BOSS",       .dehBit = BIT(16), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_E4M8BOSS; } },
	{ .name = "FULLVOLSOUNDS",  .dehBit = BIT(18), .setter = [](mobjinfo_t& info) { info.flags3 |= MF3_FULLVOLSOUNDS; } },
}};

constexpr void ClearMBF21Bits(mobjinfo_t& info)
{
	info.flags2 &= ~(MF2_LOGRAV|MF2_BOSS|MF2_RIP);
	info.flags3 &= ~(MF3_SHORTMRANGE|MF3_DMGIGNORED|MF3_NORADIUSDMG|MF3_FORCERADIUSDMG|
	                 MF3_HIGHERMPROB|MF3_RANGEHALF|MF3_NOTHRESHOLD|MF3_LONGMELEE|
	                 MF3_MAP07BOSS1|MF3_MAP07BOSS2|MF3_E1M8BOSS|MF3_E2M8BOSS|MF3_E3M8BOSS|
	                 MF3_E4M6BOSS|MF3_E4M8BOSS|MF3_FULLVOLSOUNDS);
};

}

struct Key
{
	std::string_view name;
	ptrdiff_t offset;
};

class DehScanner;

static void PatchThing(int, std::string_view, DehScanner&);
static void PatchSound(int, DehScanner&);
static void PatchSounds(DehScanner&);
static void PatchFrame(int, DehScanner&);
static void PatchSprite(int, DehScanner&);
static void PatchSprites(DehScanner&);
static void PatchAmmo(int, DehScanner&);
static void PatchWeapon(int, DehScanner&);
static void PatchPointer(int, DehScanner&);
static void PatchCheats(DehScanner&);
static void PatchMisc(DehScanner&);
static void PatchText(int, int, DehScanner&);
static void PatchStrings(DehScanner&);
static void PatchPars(DehScanner&);
static void PatchCodePtrs(DehScanner&);
static void PatchMusic(DehScanner&);
static void PatchHelper(DehScanner&);
static int DoInclude(std::string_view, size_t);

static std::tuple<int, std::string_view> ParseThingHeader(std::string_view, size_t);
static int ParsePointerHeader(std::string_view, size_t);
static std::tuple<int, int> ParseTextHeader(std::string_view, size_t);
static int ParseClassicHeader(std::string_view header, size_t length)
{
	std::string_view textNum = header.substr(length);
	if (auto num = ParseNum<int32_t>(textNum))
	{
		return *num;
	}
	else
	{
		DPrintFmt("Invalid section header: '{}'\n", header);
		return -1;
	}
};

template <typename... Args>
struct ModeFuncs
{
	void (*parsebody)(Args..., DehScanner&) = nullptr;

	using HeaderResult =
		std::conditional_t<
			sizeof...(Args) == 1,
			// Args...[0]
			std::tuple_element_t<0, std::tuple<Args...>>,
			std::tuple<Args...>>;

	HeaderResult (*parseheader)(std::string_view, size_t) = nullptr;

	void operator()(std::string_view header, size_t length, DehScanner& scanner) const
	{
		if constexpr (sizeof...(Args) == 1)
		{
			parsebody(parseheader(header, length), scanner);
		}
		else
		{
			std::apply(
				[&](auto&&... args)
				{
					parsebody(std::forward<decltype(args)>(args)..., scanner);
				},
				parseheader(header, length)
			);
		}

	}
};

template <>
struct ModeFuncs<>
{
	void (*parsebody)(DehScanner&);

	void operator()(std::string_view, size_t, DehScanner& scanner) const
	{
		parsebody(scanner);
	}
};

static constexpr struct
{
	std::string_view name;
	std::variant<ModeFuncs<>, ModeFuncs<int>, ModeFuncs<int, int>, ModeFuncs<int, std::string_view>> funcs;
} Modes[] = {
    // https://eternity.youfailit.net/wiki/DeHackEd_/_BEX_Reference

    // These appear in .deh and .bex files
    {"Thing", ModeFuncs<int, std::string_view>{ PatchThing, ParseThingHeader }},
    {"Sound", ModeFuncs<int>{ PatchSound, ParseClassicHeader }},
    {"Frame", ModeFuncs<int>{ PatchFrame, ParseClassicHeader }},
    {"Sprite", ModeFuncs<int>{ PatchSprite, ParseClassicHeader }},
    {"Ammo", ModeFuncs<int>{ PatchAmmo, ParseClassicHeader }},
    {"Weapon", ModeFuncs<int>{ PatchWeapon, ParseClassicHeader }},
    {"Pointer", ModeFuncs<int>{ PatchPointer, ParsePointerHeader }},
    {"Cheat", ModeFuncs<>{ PatchCheats }},
    {"Misc", ModeFuncs<>{ PatchMisc }},
    {"Text", ModeFuncs<int, int>{ PatchText, ParseTextHeader }},
    // These appear in .bex files
    {"include", ModeFuncs<int>{ [](int, DehScanner&){}, DoInclude }},
    {"[STRINGS]", ModeFuncs<>{ PatchStrings }},
    {"[PARS]", ModeFuncs<>{ PatchPars }},
    {"[CODEPTR]", ModeFuncs<>{ PatchCodePtrs }},
    // Eternity engine added a few more features to BEX
    {"[MUSIC]", ModeFuncs<>{ PatchMusic }},
	// DSDHacked BEX additions
	{"[SPRITES]", ModeFuncs<>{ PatchSprites }},
	{"[SOUNDS]", ModeFuncs<>{ PatchSounds }},
	{"[HELPER]", ModeFuncs<>{ PatchHelper }},
};

static bool HandleKey(std::span<const Key> keys, void* structure, std::string_view key, int value,
                      const int structsize);
static void BackupData();

class DehScanner
{
public:
	using HeaderLine = std::string_view;
	using KVLine = std::pair<std::string_view, std::string_view>;
	using Line = std::variant<KVLine, HeaderLine>;

private:
	std::string_view m_remainingData;
	std::string_view m_currentLine;
	std::optional<Line> m_buffer;

	bool readLine()
    {
        if (m_remainingData.empty())
        {
            m_currentLine = std::string_view{};
            return false;
        }

        size_t pos = m_remainingData.find('\n');
        if (pos == std::string_view::npos)
		{
			m_currentLine = m_remainingData;
			m_remainingData = std::string_view{};
		}
        else
		{
			m_currentLine = m_remainingData.substr(0, pos);
			m_remainingData.remove_prefix(pos + 1);
		}
        return true;
    }

	[[nodiscard]]
	bool isEmptyLine() const
	{
		return m_currentLine.empty() ||
			std::all_of(
				m_currentLine.begin(),
				m_currentLine.end(),
				[](char c){ return std::isspace(static_cast<unsigned char>(c)); }
			);
	}

	[[nodiscard]]
	bool isCommentLine() const
	{
		return !m_currentLine.empty() && m_currentLine[0] == '#';
	}

	[[nodiscard]]
	bool isKVLine() const
	{
		return !m_currentLine.empty() && m_currentLine.find('=') != std::string_view::npos;
	}

	static std::string_view trimSpace(std::string_view str)
	{
		while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front())))
			str.remove_prefix(1);
		while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back())))
			str.remove_suffix(1);
		return str;
	}

	[[nodiscard]]
	KVLine parseKV() const
    {
        size_t equalPos = m_currentLine.find('=');
        auto key = trimSpace(m_currentLine.substr(0, equalPos));
        auto value = trimSpace(m_currentLine.substr(equalPos + 1));
        return { key, value };
    }

	[[nodiscard]]
    HeaderLine parseHeader() const
    {
        return trimSpace(m_currentLine);
    }

	[[nodiscard]]
	std::optional<Line> parseCurrentLine() const
    {
        if (isKVLine())
            return Line{ parseKV() };
        if (!isEmptyLine() && !isCommentLine())
            return Line{ parseHeader() };

        return std::nullopt;
    }

public:
	explicit DehScanner(std::string_view data) : m_remainingData(data) {}

	[[nodiscard]] bool hasMoreLines() const {
		return !m_remainingData.empty() || m_buffer.has_value();
	}

	void unscan()
	{
		if (!m_buffer)
            m_buffer = parseCurrentLine();
	}

	void skipLine() { std::ignore = getNextLine(); }

	[[nodiscard]] std::optional<Line> getNextLine()
	{
		if (m_buffer)
        {
            auto out = m_buffer;
            m_buffer.reset();
            return out;
        }

		while (readLine())
		{
			if (isCommentLine() || isEmptyLine())
				continue;

			return parseCurrentLine();
		}

		return std::nullopt;
	}

	[[nodiscard]] std::optional<KVLine> getNextKeyValue()
    {
        if (auto line = getNextLine())
        {
            if (auto kv = std::get_if<KVLine>(&line.value()))
                return *kv;

            m_buffer = *line; // put back
        }
        return std::nullopt;
    }

	[[nodiscard]] std::optional<HeaderLine> getNextHeader()
    {
        if (auto line = getNextLine())
        {
            if (auto header = std::get_if<HeaderLine>(&*line))
                return *header;

            m_buffer = *line; // put back
        }
        return std::nullopt;
    }

	[[nodiscard]] std::optional<std::string> readTextString(int size)
	{
		std::string str;
		str.reserve(size);

		while (size--)
		{
			if (m_remainingData.empty())
			{
				DPrintFmt("Unexpected end of Text data.\n");
				break;
			}

			if (m_remainingData[0] != '\r')
				str += m_remainingData[0];
			else
				size++;

			m_remainingData.remove_prefix(1);
		}

		return str;
	}

	struct ParsedState
	{
		size_t textOffsetIdx;
		bool includenotext;
		std::vector<int32_t> droppedItems;
		std::vector<std::pair<const char**, int>> soundMapIndices;
		std::vector<std::pair<int32_t, const CodePtr*>> codePtrs;
	} m_state;
};

static void PrintUnknown(std::string_view key, const char* loc, const size_t idx)
{
	DPrintFmt("Unknown key {} encountered in {} ({}).\n", key, loc, idx);
}

static void HandleMode(std::string_view header, DehScanner& scanner)
{
	// TODO C++20: restore the structured bindings here
	// apparently apple clang doesn't support this yet even in C++20 mode
	// for (const auto& [name, funcs] : Modes)
	for (const auto& mode : Modes)
	{
		if (header.length() >= mode.name.length() && !strnicmp(mode.name.data(), header.data(), mode.name.size()))
		{
			std::visit([&](const auto& func) {
				func(header, mode.name.length(), scanner);
			}, mode.funcs);
			return;
		}
	}

	// Handle unknown or unimplemented data
	DPrintFmt("Unknown chunk {} encountered. Skipping.\n", header);
	while (scanner.getNextKeyValue());
}

static bool HandleKey(std::span<const Key> keys, void* structure, std::string_view key, int value,
                      const int structsize)
{
	for (const auto& keyit : keys)
	{
		if (iequals(keyit.name, key))
		{
			if (structsize && keyit.offset + static_cast<int>(sizeof(int)) > structsize)
			{
				// Handle unknown or unimplemented data
				DPrintFmt("DeHackEd: Cannot apply key {}, offset would overrun.\n", keyit.name);
				return true;
			}

			*(reinterpret_cast<int*>((static_cast<byte*>(structure)) + keyit.offset)) = value;
			return false;
		}
	}

	return true;
}

struct DoomBackup_t
{
	DoomObjectContainer<state_t, int32_t> backupStates; // boomstates
	DoomObjectContainer<mobjinfo_t, int32_t> backupMobjInfo; // doom_mobjinfo
	DoomObjectContainer<std::string, int32_t> backupSprnames; // doom_sprnames
	DoomObjectContainer<std::string, int32_t> backupSoundMap; // doom_SoundMap

	std::array<weaponinfo_t, NUMWEAPONS> backupWeaponInfo;
	std::array<int,          NUMAMMO>    backupMaxAmmo;
	std::array<int,          NUMAMMO>    backupClipAmmo;

	DehInfo backupDeh;

} doomBackup;

// [CMB] useful typedefs for iteration over global doom object containers
using StatesIterator = DoomObjectContainer<state_t, int32_t>::iterator;
using MobjIterator = DoomObjectContainer<mobjinfo_t, int32_t>::iterator;
using SpriteNamesIterator = DoomObjectContainer<std::string, int32_t>::iterator;
using SoundMapIterator = DoomObjectContainer<std::string, int32_t>::iterator;

static void BackupData(void)
{
	if (BackedUpData)
	{
		return;
	}

	// backup sprites
	for (int i = 0; i < ::NUMSPRITES; i++)
	{
		OrgSprNames[i] = sprnames[i];
	}

	// backup action pointers
	for (int i = 0; i < ::NUMSTATES; i++)
	{
		OrgActionPtrs[i] = states[i].action;
	}

	doomBackup.backupStates = states;
	doomBackup.backupMobjInfo = mobjinfo;
	doomBackup.backupSprnames = sprnames;
	doomBackup.backupSoundMap = SoundMap;

	doomBackup.backupWeaponInfo = weaponinfo;
	doomBackup.backupClipAmmo   = clipammo;
	doomBackup.backupMaxAmmo    = maxammo;

	doomBackup.backupDeh = deh;

	BackedUpData = true;
}

void D_UndoDehPatch()
{
	if (!BackedUpData)
	{
		return;
	}

	sprnames = std::move(doomBackup.backupSprnames);
	mobjinfo = std::move(doomBackup.backupMobjInfo);
	states = std::move(doomBackup.backupStates);
	SoundMap = std::move(doomBackup.backupSoundMap);

	D_BuildSpawnMap();

	extern bool isFast;
	isFast = false;

	weaponinfo = doomBackup.backupWeaponInfo;
	clipammo   = doomBackup.backupClipAmmo;
	maxammo    = doomBackup.backupMaxAmmo;

	deh = doomBackup.backupDeh;

	BackedUpData = false;
}

static void ReplaceSpecialChars(std::string& str)
{
	char *read = str.data();
	char *write = str.data();

	while (char c = *read++)
	{
		if (c != '\\')
		{
			*write++ = c;
		}
		// don't read past a trailing backslash
		else if (*read)
		{
			switch (*read)
			{
			case 'n':
			case 'N':
				*write++ = '\n';
				break;
			case 't':
			case 'T':
				*write++ = '\t';
				break;
			case 'r':
			case 'R':
				*write++ = '\r';
				break;
			case 'x':
			case 'X':
				c = 0;
				read++;
				for (int i = 0; i < 2; i++)
				{
					c <<= 4;
					if (*read >= '0' && *read <= '9')
					{
						c += *read - '0';
					}
					else if (*read >= 'a' && *read <= 'f')
					{
						c += 10 + *read - 'a';
					}
					else if (*read >= 'A' && *read <= 'F')
					{
						c += 10 + *read - 'A';
					}
					else
					{
						break;
					}
					read++;
				}
				*write++ = c;
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				c = 0;
				for (int i = 0; i < 3; i++)
				{
					c <<= 3;
					if (*read >= '0' && *read <= '7')
					{
						c += *read - '0';
					}
					else
					{
						break;
					}
					read++;
				}
				*write++ = c;
				break;
			default:
				*write++ = *read;
				break;
			}
			read++;
		}
	}
	*write = 0;
	str.resize(write - str.data());
}

static auto SplitBexBits(std::string_view str, std::string_view delims)
{
    std::vector<std::string_view> out;
    size_t start = str.find_first_not_of(delims);
    while (start != std::string_view::npos) {
        size_t end = str.find_first_of(delims, start);
        out.push_back(str.substr(start, end - start));
        if (end == std::string_view::npos) break;
        start = str.find_first_not_of(delims, end);
    }
    return out;
}

static std::tuple<int, std::string_view> ParseThingHeader(std::string_view header, size_t length) {
	int thingNum;
	std::string_view textNum = TrimStringViewStart(header.substr(length));
	if (auto num = ParseNum<int32_t>(textNum))
	{
		thingNum = *num;
	}
	else
	{
		DPrintFmt("Invalid thing header: '{}'\n", header);
		return { -1, "" };
	}

	const auto idxPastNum = textNum.find_first_not_of("0123456789");
	if (idxPastNum == std::string_view::npos)
		return { thingNum, "" };

	std::string_view thingName = textNum.substr(idxPastNum);

	thingName = TrimStringView(thingName);

	if (thingName.size() < 2)
		return { thingNum, "" };

	// remove parentheses
	thingName.remove_prefix(1);
	thingName.remove_suffix(1);

	return { thingNum, thingName };
}

static void PatchThing(int thingNum, std::string_view thingName, DehScanner& scanner)
{
	// flags can be specified by name (a .bex extension):
	struct
	{
		short Bit;
		short WhichFlags;
		const char* Name;
	} bitnames[73] = {
	    {0, 0, "SPECIAL"},
	    {1, 0, "SOLID"},
	    {2, 0, "SHOOTABLE"},
	    {3, 0, "NOSECTOR"},
	    {4, 0, "NOBLOCKMAP"},
	    {5, 0, "AMBUSH"},
	    {6, 0, "JUSTHIT"},
	    {7, 0, "JUSTATTACKED"},
	    {8, 0, "SPAWNCEILING"},
	    {9, 0, "NOGRAVITY"},
	    {10, 0, "DROPOFF"},
	    {11, 0, "PICKUP"},
	    {12, 0, "NOCLIP"},
	    {13, 0, "SLIDE"}, // UNUSED FOR NOW
	    {14, 0, "FLOAT"},
	    {15, 0, "TELEPORT"},
	    {16, 0, "MISSILE"},
	    {17, 0, "DROPPED"},
	    {18, 0, "SHADOW"},
	    {19, 0, "NOBLOOD"},
	    {20, 0, "CORPSE"},
	    {21, 0, "INFLOAT"},
	    {22, 0, "COUNTKILL"},
	    {23, 0, "COUNTITEM"},
	    {24, 0, "SKULLFLY"},
	    {25, 0, "NOTDMATCH"},
	    {26, 0, "TRANSLATION1"},
	    {26, 0, "TRANSLATION"}, // BOOM compatibility
	    {27, 0, "TRANSLATION2"},
	    {27, 0, "UNUSED1"}, // BOOM compatibility
	    {28, 0, "UNUSED2"}, // BOOM compatibility
	    {29, 0, "UNUSED3"}, // BOOM compatibility
	    {30, 0, "UNUSED4"}, // BOOM compatibility
	    {28, 0, "TOUCHY"},
	    {29, 0, "BOUNCES"},
	    {30, 0, "FRIEND"},
	    {31, 0, "TRANSLUCENT"}, // BOOM compatibility

	    // TRANSLUCENT... HACKY BUT HEH.
	    {0, 2, "TRANSLUC25"},
	    {1, 2, "TRANSLUC50"},
	    {2, 2, "TRANSLUC75"},
		// old zdoom only allowed setting this by mnemonic anyway
		{3, 2, "STEALTH"}, // UNUSED FOR NOW

	    // Names for flags2
	    {0, 1, "LOGRAV"},
	    {1, 1, "WINDTHRUST"},
	    {2, 1, "FLOORBOUNCE"},
	    {3, 1, "BLASTED"},
	    {4, 1, "FLY"},
	    {5, 1, "FLOORCLIP"},
	    {6, 1, "SPAWNFLOAT"},
	    {7, 1, "NOTELEPORT"},
	    {8, 1, "RIP"},
	    {9, 1, "PUSHABLE"},
	    {10, 1, "CANSLIDE"}, // Avoid conflict with SLIDE from BOOM
	    {11, 1, "ONMOBJ"},
	    {12, 1, "PASSMOBJ"},
	    {13, 1, "CANNOTPUSH"},
	    {14, 1, "DROPPED"},
	    {15, 1, "BOSS"},
	    {16, 1, "FIREDAMAGE"},
	    {17, 1, "NODMGTHRUST"},
	    {18, 1, "TELESTOMP"},
	    {19, 1, "FLOATBOB"},
	    {20, 1, "DONTDRAW"},
	    {21, 1, "IMPACT"},
	    {22, 1, "PUSHWALL"},
	    {23, 1, "MCROSS"},
	    {24, 1, "PCROSS"},
	    {25, 1, "CANTLEAVEFLOORPIC"},
	    {26, 1, "NONSHOOTABLE"},
	    {27, 1, "INVULNERABLE"},
	    {28, 1, "DORMANT"},
	    {29, 1, "ICEDAMAGE"},
	    {30, 1, "SEEKERMISSILE"},
	    {31, 1, "REFLECTIVE"},
	};

	mobjinfo_t *info;
	bool hadHeight = false;
	bool gibhealth = false;

	thingNum--;
	MobjIterator mobjinfo_it = mobjinfo.find(thingNum);
	if (mobjinfo_it == mobjinfo.end())
	{
		info = &mobjinfo.insert(mobjinfo_t{}, static_cast<mobjtype_t>(thingNum));
		// set the type
		info->type = thingNum;
	} else
	{
		info = &mobjinfo_it->second;
	}

	info->deh_name = thingName;

#if defined ODAMEX_DEBUG
	DPrintFmt("Thing {} found.\n", thingNum);
#endif

	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(0);

		const auto ends_with = [](std::string_view str, std::string_view ending)
		{
			return str.size() >= ending.size() &&
			       iequals(str.substr(str.size() - ending.size()), ending);
		};

		const auto starts_with = [](std::string_view str, std::string_view beginning)
		{
			return str.size() >= beginning.size() &&
			       iequals(str.substr(0, beginning.size()), beginning);
		};

		if (ends_with(key, " frame"))
		{
			const auto state = static_cast<statenum_t>(val);

			if (starts_with(key, "Initial frame"))
			{
				info->spawnstate = state;
			}
			else if (starts_with(key, "First moving"))
			{
				info->seestate = state;
			}
			else if (starts_with(key, "Injury"))
			{
				info->painstate = state;
			}
			else if (starts_with(key, "Close attack"))
			{
				info->meleestate = state;
			}
			else if (starts_with(key, "Far attack"))
			{
				info->missilestate = state;
			}
			else if (starts_with(key, "Death"))
			{
				info->deathstate = state;
			}
			else if (starts_with(key, "Exploding"))
			{
				info->xdeathstate = state;
			}
			else if (starts_with(key, "Respawn"))
			{
				info->raisestate = state;
			}
		}
		else if (ends_with(key, " sound"))
		{
			if (starts_with(key, "Alert"))
			{
				scanner.m_state.soundMapIndices.emplace_back(&info->seesound, val);
			}
			else if (starts_with(key, "Attack"))
			{
				scanner.m_state.soundMapIndices.emplace_back(&info->attacksound, val);
			}
			else if (starts_with(key, "Pain"))
			{
				scanner.m_state.soundMapIndices.emplace_back(&info->painsound, val);
			}
			else if (starts_with(key, "Death"))
			{
				scanner.m_state.soundMapIndices.emplace_back(&info->deathsound, val);
			}
			else if (starts_with(key, "Action"))
			{
				scanner.m_state.soundMapIndices.emplace_back(&info->activesound, val);
			}
			else if (starts_with(key, "Rip"))
			{
				scanner.m_state.soundMapIndices.emplace_back(&info->ripsound, val);
			}
		}
		else if (iequals(key, "Projectile group"))
		{
			info->projectile_group = val;

			if (info->projectile_group < 0)
			{
				info->projectile_group = PG_GROUPLESS;
			}
			else
			{
				info->projectile_group = val + PG_END;
			}
		}
		else if (iequals(key, "Infighting group"))
		{
			info->infighting_group = val;

			if (info->infighting_group < 0)
			{
				I_Error("Infighting groups must be >= 0 (check your DEHACKED "
				        "entry, and correct it!)\n");
			}
			info->infighting_group = val + IG_END;
		}
		else if (iequals(key, "Missile damage"))
		{
			info->damage = val;
		}
		else if (iequals(key, "Reaction time"))
		{
			info->reactiontime = val;
		}
		else if (iequals(key, "Translucency"))
		{
			info->translucency = val;
		}
		else if (iequals(key, "Dropped item"))
		{
			if (val == 0)
			{
				info->droppeditem = MT_NULL;
			}
			else
			{
				int validx = val - 1;
				scanner.m_state.droppedItems.push_back(validx);
				info->droppeditem = static_cast<mobjtype_t>(validx); // deh is mobj + 1
			}
		}
		else if (iequals(key, "Splash group"))
		{
			info->splash_group = val;
			if (info->splash_group < 0)
			{
				I_Error("Splash groups must be >= 0 (check your DEHACKED entry, "
				        "and correct it!)\n");
			}
			info->splash_group = val + SG_END;
		}
		else if (iequals(key, "Pain chance"))
		{
			info->painchance = val;
		}
		else if (iequals(key, "Melee range"))
		{
			info->meleerange = val;
		}
		else if (iequals(key, "Hit points"))
		{
			info->spawnhealth = val;
		}
		else if (iequals(key, "Fast speed"))
		{
			info->altspeed = val;
		}
		else if (iequals(key, "Gib health"))
		{
			gibhealth = true;
			info->gibhealth = val;

			// Special hack: DEH values are always positive, and since a gib is
			// always negative, a positive value will thus become negative.
			if (info->gibhealth > 0)
				info->gibhealth = -info->gibhealth;
		}
		else if (iequals(key, "MBF21 Bits"))
		{
			ClearMBF21Bits(*info);

			for (const auto strval : SplitBexBits(value, ",+| \t\f\r"))
			{
				if (IsNum(strval))
				{
					// TODO: maybe give a warning for out of range bits
					const int32_t tempval = ParseNum<int32_t>(strval).value_or(0);

					for (const auto& [_, dehflag, setter] : mbf21flagtranslation)
					{
						if (tempval & dehflag)
							setter(*info);
					}
				}
				else
				{
					bool found = false;
					for (const auto& [name, _, setter] : mbf21flagtranslation)
					{
						if (iequals(strval, name))
						{
							setter(*info);
							found = true;
						}
					}

					if (!found)
					{
						DPrintFmt("Unknown bit mnemonic {}\n", strval);
					}
				}
			}
		}
		else if (iequals(key, "Height"))
		{
			info->height = val;
			hadHeight = true;
		}
		else if (iequals(key, "Speed"))
		{
			info->speed = val;
		}
		else if (iequals(key, "Width"))
		{
			info->radius = val;
		}
		else if (iequals(key, "Bits"))
		{
			auto lineval = value;
			int value[3] = {0, 0, 0};
			bool vchanged[3] = {false, false, false};

			for (const auto strval : SplitBexBits(lineval, ",+| \t\f\r"))
			{
				if (IsNum(strval))
				{
					// TODO: maybe give a warning for out of range bits
					value[0] |= ParseNum<int32_t>(strval).value_or(0);
					vchanged[0] = true;
				}
				else
				{
					size_t i;

					for (i = 0; i < ARRAY_LENGTH(bitnames); i++)
					{
						if (iequals(strval, bitnames[i].Name))
						{
							vchanged[bitnames[i].WhichFlags] = true;
							value[bitnames[i].WhichFlags] |= 1 << (bitnames[i].Bit);
							break;
						}
					}

					if (i == ARRAY_LENGTH(bitnames))
						DPrintFmt("Unknown bit mnemonic {}\n", strval);
				}
			}
			if (vchanged[0])
			{
				info->flags = ActorFlags1::unsafe_from_int(static_cast<uint32_t>(value[0]));
				if (info->flags & MF_TRANSLUCENT)
				{
					info->translucency = TRANSLUC66;
				}
			}
			if (vchanged[1])
			{
				info->flags2 = ActorFlags2::unsafe_from_int(static_cast<uint32_t>(value[1]));
			}
			if (vchanged[2])
			{
				if (value[2] & 7)
				{
					if (value[2] & 1)
						info->translucency = TRANSLUC25;
					else if (value[2] & 2)
						info->translucency = TRANSLUC50;
					else if (value[2] & 4)
						info->translucency = TRANSLUC75;
				}
				if (value[2] & 8)
				{
					// info->oflags |= MFO_STEALTH; // not yet implemented
				}
			}
		}
		else if (iequals(key, "ID #"))
		{
			info->doomednum = static_cast<int16_t>(val);
			// update spawn map
			spawn_map.insert(info, info->doomednum);
		}
		else if (iequals(key, "Mass"))
		{
			info->mass = val;
		}
		else if (iequals(key, "Tag"))
		{
			info->display_name = value;
			info->display_name_set = true;
		}
		else
		{
			PrintUnknown(key, "Thing", thingNum);
		}
	}

	// [ML] Set a thing's "real world height" to what's being offered here,
	// so it's consistent from the patch
	if (hadHeight && thingNum < ARRAY_LENGTH(OrgHeights) && thingNum >= 0)
	{
		info->cdheight = info->height;
	}

	if (info->flags & MF_SPAWNCEILING && !hadHeight && thingNum < ARRAY_LENGTH(OrgHeights) && thingNum >= 0)
	{
		info->height = OrgHeights[thingNum] * FRACUNIT;
	}

	// Set a default gibhealth if none was assigned.
	if (!gibhealth && info->spawnhealth && !info->gibhealth)
	{
		info->gibhealth = -info->spawnhealth;
	}
}

static void PatchSound(int soundNum, DehScanner& scanner)
{
	DPrintFmt("Sound {} (no longer supported)\n", soundNum);

	while (auto result = scanner.getNextKeyValue());
}

static void PatchFrame(int frameNum, DehScanner& scanner)
{
	static constexpr Key keys[] = {{"Sprite number", offsetof(state_t, sprite)},
	                               {"Sprite subnumber", offsetof(state_t, frame)},
	                               {"Duration", offsetof(state_t, tics)},
	                               {"Next frame", offsetof(state_t, nextstate)},
	                               {"Unknown 1", offsetof(state_t, misc1)},
	                               {"Unknown 2", offsetof(state_t, misc2)},
	                               {"Args1", offsetof(state_t, args[0])},
	                               {"Args2", offsetof(state_t, args[1])},
	                               {"Args3", offsetof(state_t, args[2])},
	                               {"Args4", offsetof(state_t, args[3])},
	                               {"Args5", offsetof(state_t, args[4])},
	                               {"Args6", offsetof(state_t, args[5])},
	                               {"Args7", offsetof(state_t, args[6])},
	                               {"Args8", offsetof(state_t, args[7])}};
	state_t *info;

    static const struct
    {
        short Bit;
        const char* Name;
    } bitnames[] = {
        {0x01, "SKILL5FAST"},
    };

	StatesIterator states_it = states.find(frameNum);
	if(states_it == states.end())
    {
		info = &states.insert(state_t{}, frameNum);
		info->statenum  = static_cast<statenum_t>(frameNum);
		info->nextstate = static_cast<statenum_t>(frameNum);
	}
	else
	{
		info = &states_it->second;
	}


	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(0);

		if (HandleKey(keys, info, key, val, sizeof(*info)))
		{
			if (iequals(key, "MBF21 Bits"))
			{
				auto lineval = value;
				int value = 0;
				bool vchanged = false;

				for (const auto strval : SplitBexBits(lineval, ",+| \t\f\r"))
				{
					if (IsNum(strval))
					{
						// TODO: maybe give a warning for out of range bits
						value |= ParseNum<int32_t>(strval).value_or(0);
						vchanged = true;
					}
					else
					{
						size_t i;

						for (i = 0; i < ARRAY_LENGTH(bitnames); i++)
						{
							if (iequals(strval, bitnames[i].Name))
							{
								vchanged = true;
								value |= bitnames[i].Bit;
								break;
							}
						}

						if (i == ARRAY_LENGTH(bitnames))
						{
							DPrintFmt("Unknown bit mnemonic {}\n", strval);
						}
					}
				}
				if (vchanged)
				{
					info->flags = value; // Weapon Flags
				}
			}
			else
			{
				PrintUnknown(key, "Frame", frameNum);
			}
		}
	}
#if defined ODAMEX_DEBUG
	SpriteNamesIterator sprnames_it = sprnames.find(info->sprite);
	// TODO: sprname might appear as <No Sprite> when it's just not be defined yet
	std::string_view sprname = (sprnames_it == sprnames.end()) ? "<No Sprite>"sv : sprnames_it->second;
	DPrintFmt("FRAME {}: Duration: {}, Next: {}, SprNum: {}({}), SprSub: {}\n", frameNum,
	          info->tics, info->nextstate, info->sprite, sprname,
	          info->frame);
#endif
}

static void PatchSprite(int sprNum, DehScanner& scanner)
{
	int offset = 0;

	if (sprNum >= 0 && sprNum < ::NUMSPRITES)
	{
#if defined ODAMEX_DEBUG
		DPrintFmt("Sprite {}\n", sprNum);
#endif
	}
	else
	{
		DPrintFmt("Sprite {} out of range.\n", sprNum);
		sprNum = -1;
	}
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(0);

		if (iequals(key, "Offset"))
		{
			offset = val;
		}
		else
		{
			PrintUnknown(key, "Sprite", sprNum);
		}
	}

	if (offset > 0 && sprNum != -1)
	{
		// Calculate offset from beginning of sprite names.
		offset = (offset - toff[scanner.m_state.textOffsetIdx] - 22044) / 8;

		if (offset >= 0 && offset < sprnames.size())
		{
			sprnames[sprNum] = OrgSprNames[offset];
		}
		else
		{
			DPrintFmt("Sprite name {} out of range.\n", offset);
		}
	}
}

/**
 * @brief patch sprites underneath SPRITES header
 *
 * @param dummy - int value for function pointer
 */
static void PatchSprites(DehScanner& scanner)
{
	static constexpr size_t maxsprlen = 4;
#if defined ODAMEX_DEBUG
	DPrintFmt("[SPRITES]\n");
#endif

	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;

        if(value.length() > maxsprlen)
        {
            DPrintFmt("Invalid sprite {}\n", value);
            continue; // TODO: should this be an error instead?
        }

		const std::string newSprName(value);

		int32_t sprIdx = -1;
		if (IsNum(key))
		{
			sprIdx = ParseNum<int32_t>(key).value_or(-1);
		}
		else
		{
			// find the value that matches
			for (int i = 0; i < OrgSprNames.size(); i++)
			{
				if (strnicmp(key.data(), OrgSprNames[i].c_str(), maxsprlen) == 0)
				{
					sprIdx = i;
				}
			}
		}
		if (sprIdx == -1)
		{
			DPrintFmt("Invalid sprite index {}.\n", key);
			continue; // TODO: should this be an error instead?
		}
#if defined ODAMEX_DEBUG
		SpriteNamesIterator sprnames_it = sprnames.find(sprIdx);
		const char* prevSprName =
		    sprnames_it != sprnames.end() ? sprnames_it->second.c_str() : "No Sprite";
		DPrintFmt("Patching sprite at {} with name {} with new name {}\n",
		          sprIdx, prevSprName, newSprName);
#endif
		sprnames.insert(newSprName, sprIdx);
	}
}

static void PatchSounds(DehScanner& scanner)
{
#if defined ODAMEX_DEBUG
	DPrintFmt("[Sounds]\n");
#endif
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const std::string newname(value);
		const OLumpName newnameds = fmt::format("DS{}", value);

		if (IsNum(key))
		{
			const std::string sndname = fmt::format("dsdhacked/{}", StdStringToLower(newname));
			if (const auto soundIdx = ParseNum<int32_t>(key)) {
				SoundMap.insert(sndname, soundIdx.value());
				S_AddSound(sndname.c_str(), newnameds.c_str());
			}
			else
			{
				DPrintFmt("Invalid sound index {}.\n", key);
			}
		}
		else
		{
			const int lumpnum = W_CheckNumForName(fmt::format("DS{}", key).c_str());
			const int sndIdx = S_FindSoundByLump(lumpnum);
			if (sndIdx == -1)
				I_Error("Sound {} not found.", key);
			S_AddSound(S_sfx[sndIdx].name, newnameds.c_str());
		}
	}
	S_HashSounds();
}

static void PatchAmmo(int ammoNum, DehScanner& scanner)
{
	int* max;
	int* per;
	int dummy;

	if (ammoNum >= 0 && ammoNum < NUMAMMO)
	{
#if defined ODAMEX_DEBUG
		DPrintFmt("Ammo {}.\n", ammoNum);
#endif
		max = &maxammo[ammoNum];
		per = &clipammo[ammoNum];
	}
	else
	{
		DPrintFmt("Ammo {} out of range.\n", ammoNum);
		max = per = &dummy;
	}

	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(0);

		if (iequals(key, "Max ammo"))
			*max = val;
		else if (iequals(key, "Per ammo"))
			*per = val;
		else
			PrintUnknown(key, "Ammo", ammoNum);
	}
}

static void PatchWeapon(int weapNum, DehScanner& scanner)
{
	static constexpr Key keys[] = {
	    {"Ammo type", offsetof(weaponinfo_t, ammotype)},
	    {"Deselect frame", offsetof(weaponinfo_t, upstate)},
	    {"Select frame", offsetof(weaponinfo_t, downstate)},
	    {"Bobbing frame", offsetof(weaponinfo_t, readystate)},
	    {"Shooting frame", offsetof(weaponinfo_t, atkstate)},
	    {"Firing frame", offsetof(weaponinfo_t, flashstate)}};

	static constexpr struct
	{
		short Bit;
		const char* Name;
	} bitnames[] = {
	    {0x01, "NOTHRUST"},  {0x02, "SILENT"},         {0x04, "NOAUTOFIRE"},
	    {0x08, "FLEEMELEE"}, {0x10, "AUTOSWITCHFROM"}, {0x20, "NOAUTOSWITCHTO"},
	};

	weaponinfo_t *info, dummy;

	if (weapNum >= 0 && weapNum < NUMWEAPONS)
	{
		info = &weaponinfo[weapNum];
#if defined ODAMEX_DEBUG
		DPrintFmt("Weapon {}\n", weapNum);
#endif
	}
	else
	{
		info = &dummy;
		DPrintFmt("Weapon {} out of range.\n", weapNum);
	}

	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(-1);

		if (HandleKey(keys, info, key, val, sizeof(*info)))
		{
			if (iequals(key, "MBF21 Bits"))
			{
				auto lineval = value;
				int value = 0;
				bool vchanged = false;

				for (const auto strval : SplitBexBits(lineval, ",+| \t\f\r"))
				{
					if (IsNum(strval))
					{
						// TODO: maybe give a warning for out of range bits
						value |= ParseNum<int32_t>(strval).value_or(0);
						vchanged = true;
					}
					else
					{
						size_t i;

						for (i = 0; i < ARRAY_LENGTH(bitnames); i++)
						{
							if (iequals(strval, bitnames[i].Name))
							{
								vchanged = true;
								value |= bitnames[i].Bit;
								break;
							}
						}

						if (i == ARRAY_LENGTH(bitnames))
						{
							DPrintFmt("Unknown bit mnemonic {}\n", strval);
						}
					}
				}
				if (vchanged)
				{
					info->flags = value; // Weapon Flags
				}
			}
			else if (iequals(key, "Ammo per shot"))  // Eternity/MBF21
			{
				info->ammopershot = val;
				info->internalflags |= WIF_ENABLEAPS;
				deh.ZDAmmo = false;
			}
			else if (iequals(key, "Ammo use"))  // ZDoom 1.23b33
			{
				info->ammouse = val;
				deh.ZDAmmo = true;
			}
			else if (iequals(key, "Min ammo"))  // ZDoom 1.23b33
			{
				info->minammo = val;
				deh.ZDAmmo = true;
			}
			else
			{
				PrintUnknown(key, "Weapon", weapNum);
			}
		}
	}
}

static int ParsePointerHeader(std::string_view header, size_t) {
	auto headerParser = ParseString(header, false);

	// skip first token, we already know it's "Pointer"
	headerParser();

	auto expect_token = [&](std::string_view match = "") -> std::optional<std::string> {
		auto t = headerParser().token;
		if (t && (match.empty() || iequals(*t, match))) return t;
		DPrintFmt("Pointer block header is invalid: \"{}\"\n", header);
		return std::nullopt;
    };

	auto expect_number = [&]() -> std::optional<int> {
		auto tok = expect_token();
		if (!tok) return std::nullopt;

		if (auto num = ParseNum<int32_t>(*tok)) return num;

		DPrintFmt("Pointer block header is invalid: \"{}\"\n", header);
		return std::nullopt;
	};

	const auto ptrNum = expect_number();
	if (!ptrNum)
		return -1;

	if (!expect_token("(Frame"))
		return -1;

	const auto frameNum = expect_number();
	if (!frameNum)
		return -1;

	const int ptr = *ptrNum;
	const int frame = *frameNum;

#if defined ODAMEX_DEBUG
	DPrintFmt("Pointer {}\n", ptr);
#endif

	if (ptr < 0 || ptr >= codepconv.size()) {
		DPrintFmt("Pointer {} out of range.\n", ptr);
		return -1;
	}

	if (ptr && frame && codepconv[ptr] != frame) {
		DPrintFmt("Pointer {} expects frame {}, but frame was {}\n", ptr, codepconv[ptr], frame);
		return -1;
	} else if (frame) {
		return frame;
	} else {
		return codepconv[ptr];
	}
}

static void PatchPointer(int ptrNum, DehScanner& scanner)
{
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(0);
		if ((ptrNum != -1) && (iequals(key, "Codep Frame")))
		{
			// This check is okay because [CODEPTR] must be used for states added by DSDHacked
            if (states.find(val) == states.end())
			{
				DPrintFmt("Source frame {} not found while patching pointer for destination frame {}.\n",
				          val, ptrNum);
			}
			else
			{
				states[ptrNum].action = OrgActionPtrs[val];
			}
		}
		else
		{
			PrintUnknown(key, "Pointer", ptrNum);
		}
	}
}

static void PatchCheats(DehScanner& scanner)
{
	DPrintFmt("[DeHackEd] Cheats support is deprecated. Ignoring these lines...\n");

	// Fake our work (don't do anything !)
	while (const auto line = scanner.getNextKeyValue());
}

static void PatchMisc(DehScanner& scanner)
{
	static constexpr Key keys[] = {
	    {"Initial Health", offsetof(DehInfo, StartHealth)},
	    {"Initial Bullets", offsetof(DehInfo, StartBullets)},
	    {"Max Health", offsetof(DehInfo, MaxHealth)},
	    {"Max Armor", offsetof(DehInfo, MaxArmor)},
	    {"Green Armor Class", offsetof(DehInfo, GreenAC)},
	    {"Blue Armor Class", offsetof(DehInfo, BlueAC)},
	    {"Max Soulsphere", offsetof(DehInfo, MaxSoulsphere)},
	    {"Soulsphere Health", offsetof(DehInfo, SoulsphereHealth)},
	    {"Megasphere Health", offsetof(DehInfo, MegasphereHealth)},
	    {"God Mode Health", offsetof(DehInfo, GodHealth)},
	    {"IDFA Armor", offsetof(DehInfo, FAArmor)},
	    {"IDFA Armor Class", offsetof(DehInfo, FAAC)},
	    {"IDKFA Armor", offsetof(DehInfo, KFAArmor)},
	    {"IDKFA Armor Class", offsetof(DehInfo, KFAAC)},
	    {"BFG Cells/Shot", offsetof(DehInfo, BFGCells)},
	    {"Monsters Infight", offsetof(DehInfo, Infight)}};
	gitem_t* item;
#if defined ODAMEX_DEBUG
	DPrintFmt("Misc\n");
#endif
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const auto val = ParseNum<int32_t>(value).value_or(0);
		if (HandleKey(keys, &deh, key, val, sizeof(deh)))
		{
			DPrintFmt("Unknown miscellaneous info {}.\n", key);
		}

		// [SL] manually check if BFG Cells/Shot is being changed and
		// update weaponinfo accordingly. BFGCells should be considered depricated.
		if (iequals(key, "BFG Cells/Shot") == 0)
		{
			weaponinfo[wp_bfg].ammouse = deh.BFGCells;
			weaponinfo[wp_bfg].minammo = deh.BFGCells;
			weaponinfo[wp_bfg].ammopershot = deh.BFGCells;
		}
	}

	if ((item = FindItem("Basic Armor")))
	{
		item->offset = deh.GreenAC;
	}

	if ((item = FindItem("Mega Armor")))
	{
		item->offset = deh.BlueAC;
	}

	// 0xDD == enable infighting
	deh.Infight = deh.Infight == 0xDD ? 1 : 0;
}

static void PatchPars(DehScanner& scanner)
{
#if defined ODAMEX_DEBUG
	DPrintFmt("[Pars]\n");
#endif
	while (const auto line = scanner.getNextLine())
	{
		// Argh! .bex doesn't follow the same rules as .deh
		if (auto kv = std::get_if<DehScanner::KVLine>(&line.value()))
		{
			DPrintFmt("Unknown key in [PARS] section: {}\n", kv->first);
			continue;
		}

		// safe because if we reached this point, we know its not the other variant
		std::string_view parline = std::get<DehScanner::HeaderLine>(line.value());

		// didnt start with par
		if (!iequals(parline.substr(0, 4), "par "))
		{
			// we've consumed a header by accident, need to go back one line
			scanner.unscan();
			return;
		}

		parline.remove_prefix(4);

		if (parline.empty())
		{
			DPrintFmt("Need data after par.\n");
			continue;
		}

		int time;
		OLumpName mapname;

		auto parser = ParseString(parline, false);

		int nums[] = { -1, -1, -1 };

		for (int& num : nums)
		{
			if (auto token = parser().token)
				num = ParseNum<int32_t>(*token).value_or(-1);
		}

		if (std::all_of(std::begin(nums), std::end(nums), [](int n){ return n > -1; }))
		{
			time = nums[2];
			mapname = fmt::format("E{}M{}", nums[0], nums[1]);
		}
		else if (nums[0] > -1 && nums[1] > -1)
		{
			time = nums[1];
			mapname = fmt::format("MAP{:02d}", nums[0]);
		}
		else
		{
			DPrintFmt("Invalid [PARS] format: {}", parline);
			continue;
		}

		LevelInfos& levels = getLevelInfos();
		level_pwad_info_t& info = levels.findByName(mapname);

		// TODO: THIS CHECK FAILS EXCEPT WHEN DOUBLE DEHACKED HAPPENS
		// SO PAR TIMES ARE SET INCORRECTLY ON SERVERS
		if (!info.exists())
		{
			DPrintFmt("No map {}\n", mapname);
			continue;
		}

		info.partime = time;
#if defined ODAMEX_DEBUG
		DPrintFmt("Par for {} changed to {}\n", mapname, time);
#endif
	}
}

static void PatchCodePtrs(DehScanner& scanner)
{
#if defined ODAMEX_DEBUG
	DPrintFmt("[CodePtr]\n");
#endif
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto [key, value] = *line;
		if (iequals(key.substr(0, 6), "Frame "))
		{
			const auto frameOpt = ParseNum<int32_t>(key.substr(6));
			if (!frameOpt)
			{
				DPrintFmt("Unknown key in [CODEPTR] section: {}\n", key);
				continue;
			}

			const int32_t frame = frameOpt.value();
			std::string_view data = value;

			if ((value[0] == 'A' || value[0] == 'a') && value[1] == '_')
				data.remove_prefix(2);


			const auto it = std::find_if(std::begin(CodePtrs), std::end(CodePtrs), [data](const CodePtr& ptr){
				return iequals(ptr.name, data);
			});

			if (it != std::end(CodePtrs))
			{
				scanner.m_state.codePtrs.emplace_back(frame, &*it);
			}
			else
			{
				scanner.m_state.codePtrs.emplace_back(frame, nullptr);
				DPrintFmt("Unknown code pointer: {}\n", value);
			}
		}
		else
		{
			DPrintFmt("Unknown key in [CODEPTR] section: {}\n", key);
		}
	}
}

static void PatchMusic(DehScanner& scanner)
{
#if defined ODAMEX_DEBUG
	DPrintFmt("[Music]\n");
#endif
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		const std::string_view newname = value;

		OString keystring(fmt::format("MUSIC_{}", key));
		if (GStrings.hasString(keystring))
		{
			GStrings.setString(keystring, OString(newname));
			DPrintFmt("Music {} set to:\n{}\n", keystring, newname);
		}
	}
}

static void PatchHelper(DehScanner& scanner)
{
#if defined ODAMEX_DEBUG
	DPrintFmt("[Helper]\n");
#endif
	while (const auto line = scanner.getNextKeyValue())
	{
		const auto& [key, value] = *line;
		if (iequals(key, "type"))
		{
			deh.helper = ParseNum<int32_t>(value).value_or(-1);
		}
		else
		{
			DPrintFmt("Unknown key {} in [HELPER] section", key);
		}
	}
}

static std::tuple<int, int> ParseTextHeader(std::string_view header, size_t)
{
	std::string_view idk = header.substr(4);
	auto parser = ParseString(idk, false);
	int oldsize = -1;
	int newsize = -1;
	if (auto token = parser().token)
	{
		if (auto num = ParseNum<int32_t>(*token))
		{
			oldsize = *num;
		}
	}

	if (auto token = parser().token)
	{
		if (auto num = ParseNum<int32_t>(*token))
		{
			newsize = *num;
		}
	}

	if (oldsize == -1 || newsize == -1)
	{
		DPrintFmt("Invalid Text header: '{}'\n", header);
		return { -1, -1 };
	}

	// awful hack, but we know that the max size here is only a few hundred
	return { oldsize, newsize };
}

static void PatchText(const int oldSize, const int newSize, DehScanner& scanner)
{
	if (oldSize == -1)
		return;

	const auto oldStr = scanner.readTextString(oldSize);
	const auto newStr = scanner.readTextString(newSize);

	if (!oldStr || !newStr)
	{
		DPrintFmt("Unexpected-end-of-file");
		return;
	}

	if (scanner.m_state.includenotext)
	{
		PrintFmt(PRINT_HIGH, "Skipping text chunk in included patch.\n");
		return;
	}

	DPrintFmt("Searching for text:\n{}\n", *oldStr);

	// Search through sprite names
	for (auto&& [_, sprname] : sprnames)
	{
		if (sprname == *oldStr)
		{
			sprname = *newStr;
			return;
		}
	}

	const OLumpName newnameds = fmt::format("DS{}", *newStr);
	const OLumpName oldnameds = fmt::format("DS{}", *oldStr);

	const int oldlumpnum = W_CheckNumForName(oldnameds);
	const int sndIdx = S_FindSoundByLump(oldlumpnum);
	if (sndIdx != -1 && W_CheckNumForName(newnameds) != -1)
	{
		S_AddSound(S_sfx[sndIdx].name, newnameds.c_str());
		S_HashSounds();
		return;
	}

	// Search through most other texts
	const OString& name = ENGStrings.matchString(OString(*oldStr));
	if (!name.empty())
	{
		GStrings.setString(name, OString(*newStr));
		return;
	}

	DPrintFmt("   (Unmatched)\n");
}

static void PatchStrings(DehScanner& scanner)
{
#if defined ODAMEX_DEBUG
	DPrintFmt("[Strings]\n");
#endif
	while (const auto line = scanner.getNextKeyValue())
	{
		std::string string;
		const auto [key, value] = *line;
		std::string_view nextpart = value;
		while (nextpart.back() == '\\')
		{
			nextpart.remove_suffix(1);
			string += nextpart;
			if (const auto nextOpt = scanner.getNextHeader())
				nextpart = *nextOpt;
			else
				DPrintFmt("[STRINGS] invalid line continuation: {}\\", nextpart);
		}
		string += nextpart;

		const OString okey(key);
		const OString ostring(string);

		int i = GStrings.toIndex(okey);
		if (iequals("DEHTHING_", key.substr(0, 9)))
		{
			if (auto type = ParseNum<int32_t>(string))
			{
				(*type)--;
				P_MapDehThing(static_cast<mobjtype_t>(*type), std::string(key)); // TODO: rework so no casting needed
				GStrings.setString(okey, ostring);
				DPrintFmt("{} set to:\n{}\n", key, string);
			}
			else
			{
				PrintFmt(PRINT_HIGH, "Invalid thing type {} for {}\n", string, key);
			}
		}
		else if (i == -1)
		{
			if (iequals("USER_", key.substr(0, 5)))
			{
				ReplaceSpecialChars(string);
				GStrings.setString(okey, ostring);
				DPrintFmt("{} set to:\n{}\n", key, string);
			}
			else
			{
				PrintFmt(PRINT_HIGH, "Unknown string: {}\n", key);
			}
		}
		else
		{

			ReplaceSpecialChars(string);
			if ((i >= GStrings.toIndex(OB_SUICIDE) && i <= GStrings.toIndex(OB_DEFAULT) &&
			     string.find("%o") == std::string::npos) ||
			    (i >= GStrings.toIndex(OB_FRIENDLY1) &&
			     i <= GStrings.toIndex(OB_FRIENDLY4) && string.find("%k") == std::string::npos))
			{
				string = fmt::format("%{} {}.", i <= GStrings.toIndex(OB_DEFAULT) ? 'o' : 'k', string);
				if (i >= GStrings.toIndex(OB_MPFIST) && i <= GStrings.toIndex(OB_RAILGUN))
				{
					size_t spot = string.find("%s");
					if (spot != std::string::npos)
					{
						string[spot + 1] = 'k';
					}
				}
			}
			// [CMB] TODO: Language string table change // [EB] what is this comment about??
			GStrings.setString(okey, ostring);
			DPrintFmt("{} set to:\n{}\n", key, string);
		}
	}
}

static int DoInclude(std::string_view include, size_t)
{
	bool notext = false;
	OWantFile want;
	OResFile res;

	auto lineParser = ParseString(include, false);

	// skip first token, we already know it has to be "include"
	lineParser();

	auto token = lineParser().token;
	if (!token)
	{
		DPrintFmt("Include directive is missing filename\n");
		return 0;
	}

	if (iequals(*token, "notext"))
	{
		notext = true;
		token = lineParser().token;
	}

	if (!token)
	{
		DPrintFmt("Include directive is missing filename\n");
		return 0;
	}

	std::string filename = *token;

#if defined ODAMEX_DEBUG
	DPrintFmt("Including {}\n", filename);
#endif

	if (!OWantFile::make(want, filename, OFILE_DEH))
	{
		PrintFmt(PRINT_WARNING, "Could not find BEX include \"{}\"\n", filename);
		return 0;
	}

	if (!M_ResolveWantedFile(res, want))
	{
		PrintFmt(PRINT_WARNING, "Could not resolve BEX include \"{}\"\n", filename);
		return 0;
	}

	D_DoDehPatch(&res, -1, false, notext);

	DPrintFmt("Done with include\n");

	return 0;
}

static void D_PostProcessDeh(const DehScanner::ParsedState& dp);

/**
 * @brief Attempt to load a DeHackEd file.
 *
 * @param patchfile File to attempt to load, NULL if not a file.
 * @param lump Lump index to load, -1 if not a lump.
 */
bool D_DoDehPatch(const OResFile* patchfile, const int lump, bool textonly, bool notext)
{
	BackupData();

	std::string buffer;

	if (lump >= 0)
	{
		// Execute the DEHACKED lump as a patch.
		const auto lumplen = W_LumpLength(lump);
		buffer.resize(lumplen);
		W_ReadLump(lump, buffer.data());
	}
	else if (patchfile)
	{
		// Try to use patchfile as a patch.
		std::ifstream fh(patchfile->getFullpath(),
                         std::ios::in |
                         std::ios::binary);
		if (not fh.good())
		{
			PrintFmt(PRINT_WARNING, "Could not open DeHackEd patch \"{}\"\n",
			         patchfile->getBasename());
			return false;
		}

		const auto filelen = M_FileLength(fh);
		buffer.resize(filelen);

		fh.read(buffer.data(), filelen);
		const size_t read = fh.gcount();

		if (read < filelen)
		{
			DPrintFmt("Could not read file\n");
			return false;
		}

		// Check to see if this file is really a WAD, and if so, don't parse it as a
		// DEH file.
		if (buffer.size() >= 4 &&
		    (!memcmp(buffer.data(), "IWAD", 4) || !memcmp(buffer.data(), "PWAD", 4)))
		{
			PrintFmt(PRINT_WARNING, "\"{}\" is a WAD file, not a DeHackEd patch.\n",
			         patchfile->getBasename());
			return false;
		}
	}
	else
	{
		// Nothing to do.
		return false;
	}

	DehScanner scanner(buffer);
	DehScanner::ParsedState& dp = scanner.m_state;

	dp.includenotext = notext;

	// Load english strings to match against.
	::ENGStrings.loadStrings(true);

	int pversion = -1, dversion = -1;

	if (!strncmp(buffer.c_str(), "Patch File for DeHackEd v", 25))
	{
		scanner.skipLine();
		while (std::optional<DehScanner::KVLine> kvLine = scanner.getNextKeyValue())
		{
			const auto& [key, value] = *kvLine;
			if (iequals(key, "Doom version"))
			{
				dversion = ParseNum<int32_t>(value).value_or(-1);
			}
			else if (iequals(key, "Patch format"))
			{
				pversion = ParseNum<int32_t>(value).value_or(-1);
			}
			else
			{
				DPrintFmt("Unknown header key: {}\n", key);
			}
		}
		if (!scanner.hasMoreLines() || dversion == -1 || pversion == -1)
		{
			if (patchfile)
			{
				PrintFmt(PRINT_WARNING, "\"{}\" is not a DeHackEd patch file\n",
				         patchfile->getBasename());
			}
			else
			{
				PrintFmt(PRINT_WARNING, "\"DEHACKED\" is not a DeHackEd patch lump\n");
			}
			return false;
		}
	}
	else
	{
		DPrintFmt("Patch does not have DeHackEd signature. Assuming .bex\n");
		dversion = 19;
		pversion = 6;
	}

	if (pversion != 6)
	{
		DPrintFmt("DeHackEd patch version is {}.\nUnexpected results may occur.\n",
		          pversion);
	}

	switch (dversion)
	{
	case 16:
	case 17:
	case 20:
		dp.textOffsetIdx = 0;
		break;
	case 19:
		dp.textOffsetIdx = 1;
		break;
	case 21:
	case 2021:
	case 2024:
		dp.textOffsetIdx = 2;
		break;
	default:
		DPrintFmt("Patch created with unknown DOOM version.\nAssuming version 1.9.\n");
		dp.textOffsetIdx = 1;
	}

	if (textonly)
	{
		while (std::optional<DehScanner::Line> line = scanner.getNextLine())
		{
			static constexpr std::string_view STRINGS_HEADER = "[STRINGS]";
			static constexpr std::string_view TEXT_HEADER = "Text";
			auto header = std::get_if<DehScanner::HeaderLine>(&line.value());
			if (header &&
			    (!strnicmp(header->data(), STRINGS_HEADER.data(), STRINGS_HEADER.size()) ||
			     !strnicmp(header->data(), TEXT_HEADER.data(), TEXT_HEADER.size())))
			{
				HandleMode(*header, scanner);
			}
			else
			{
				scanner.skipLine();
			}
		}
		return true;
	}

	while (std::optional<DehScanner::Line> line = scanner.getNextLine())
	{
		std::visit(OUtil::visitor {
			[](const DehScanner::KVLine& kvLine) {
				DPrintFmt("Key {} encountered out of context\n", kvLine.first);
			},
			[&scanner](const DehScanner::HeaderLine& headerLine) {
				HandleMode(headerLine, scanner);
			}
		}, *line);
	}

	if (patchfile)
	{
		PrintFmt("adding {}\n", patchfile->getFullpath());
	}
	else
	{
		PrintFmt("adding DEHACKED lump\n");
	}
	PrintFmt(" (DeHackEd patch)\n");

	D_PostProcessDeh(dp);

	return true;
}

static constexpr CodePtr null_bexptr = {"(NULL)", NULL, 0, {0, 0, 0, 0, 0, 0, 0, 0}};

/*
 * @brief Check loaded deh files for any problems prior
 * to launching the game.
 *
 * (Credit to DSDADoom for the inspiration for this)
 */

static void D_PostProcessDeh(const DehScanner::ParsedState& dp)
{
	// resolve states with assigned codeptrs
	for (auto& [frame, codeptr] : dp.codePtrs)
	{
		auto states_it = states.find(frame);
		if (states_it == states.end())
		{
			DPrintFmt("Frame {} out of range\n", frame);
		}
		else
		{
			state_t& state = states_it->second;
			if (codeptr == nullptr)
			{
				state.action = nullptr;
			}
			else
			{
				state.action = codeptr->func;
				DPrintFmt("Frame {} set to {}\n", frame, codeptr->name);
			}
		}
	}

	for (auto&& [_, state] : states)
	{
		const CodePtr* bexptr_match = &null_bexptr;

		for (const auto& codeptr : CodePtrs)
		{
			if (state.action == codeptr.func)
			{
				bexptr_match = &codeptr;
				break;
			}
		}

		// ensure states don't use more mbf21 args than their
		// action pointer expects, for future-proofing's sake
		int i;
		for (i = MAXSTATEARGS - 1; i >= bexptr_match->argcount; i--)
		{
			if (state.args[i] != 0)
			{
				I_Error("Action {} on state {} expects no more than {} nonzero args ({} "
				        "found). Check your DEHACKED.",
				        bexptr_match->name, state.statenum, bexptr_match->argcount, i + 1);
			}
		}

		// replace unset fields with default values
		for (; i >= 0; i--)
		{
			if (state.args[i] == 0 && bexptr_match->default_args[i])
			{
				state.args[i] = bexptr_match->default_args[i];
			}
		}

		// remap mbf21 flags to flags2/flags3
		if (bexptr_match->func == A_AddFlags ||
		    bexptr_match->func == A_RemoveFlags ||
		    bexptr_match->func == A_JumpIfFlagsSet)
		{
			const statearg_t mbf21flags = bexptr_match->func == A_JumpIfFlagsSet ? state.args[2] : state.args[1];
			mobjinfo_t dummy;
			dummy.flags2.clear();
			dummy.flags3.clear();

			for (const auto& [_, dehflag, setter]  : mbf21flagtranslation)
			{
				if (mbf21flags & dehflag)
				{
					setter(dummy);
				}
			}

			if (bexptr_match->func == A_JumpIfFlagsSet)
			{
				state.args[2] = dummy.flags2.to_int();
				state.args[3] = dummy.flags3.to_int();
			}
			else
			{
				state.args[1] = dummy.flags2.to_int();
				state.args[2] = dummy.flags3.to_int();
			}
		}
	}

	// Resolve sounds
	for (const auto& [soundPtr, index] : dp.soundMapIndices)
	{
		auto soundIt = SoundMap.find(index);
		if (soundIt == SoundMap.end())
		{
			I_Error("Sound {} is not defined. Check your DEHACKED.\n", index);
		}
		*soundPtr = soundIt->second.c_str();
	}

	// Check dropped items for validity
	for (auto droppedItem : dp.droppedItems)
	{
		if (mobjinfo.find(droppedItem) == mobjinfo.end())
		{
			I_Error("Dropped item type {} is not defined. Check your DEHACKED.\n", droppedItem);
		}
	}

	if (deh.helper != -1 && mobjinfo.find(deh.helper) == mobjinfo.end())
	{
		DPrintFmt("Helper type {} is not defined.", deh.helper);
		deh.helper = -1;
	}
}

/*
* @brief Checks to see if TNT-range actor is defined, but useful for DEHEXTRA monsters.
* Because in HORDEDEF, sometimes a WAD author may accidentally use a DEHEXTRA monster
* that is undefined.
* Assumes the value exists - no range checking
*/
bool CheckIfDehActorDefined(const mobjtype_t mobjtype)
{
	auto it = ::mobjinfo.find(mobjtype);
	if (it == ::mobjinfo.end())
		return false;

	const mobjinfo_t defaults{};
	const auto& mobj = it->second;
	if (mobj.doomednum == defaults.doomednum &&
		mobj.spawnstate == S_TNT1 &&
		mobj.spawnhealth == defaults.spawnhealth &&
		mobj.gibhealth == defaults.gibhealth &&
		mobj.seestate == defaults.seestate &&
		mobj.seesound == defaults.seesound &&
	    mobj.reactiontime == defaults.reactiontime &&
		mobj.attacksound == defaults.attacksound &&
		mobj.painstate == defaults.painstate &&
	    mobj.painchance == defaults.painchance &&
		mobj.painsound == defaults.painsound &&
		mobj.meleestate == defaults.meleestate &&
	    mobj.missilestate == defaults.missilestate &&
		mobj.deathstate == defaults.deathstate &&
	    mobj.xdeathstate == defaults.xdeathstate &&
		mobj.deathsound == defaults.deathsound &&
		mobj.speed == defaults.speed &&
	    mobj.radius == defaults.radius &&
		mobj.height == defaults.height &&
		mobj.cdheight == defaults.cdheight &&
	    mobj.mass == defaults.mass &&
	    mobj.damage == defaults.damage &&
		mobj.activesound == defaults.activesound &&
		mobj.flags == defaults.flags &&
	    mobj.flags2 == defaults.flags2 &&
		mobj.flags3 == defaults.flags3 &&
		mobj.raisestate == defaults.raisestate &&
		mobj.translucency == defaults.translucency &&
	    mobj.altspeed == defaults.altspeed &&
		mobj.infighting_group == defaults.infighting_group &&
		mobj.projectile_group == defaults.projectile_group &&
		mobj.splash_group == defaults.splash_group &&
		mobj.ripsound == defaults.ripsound &&
		mobj.meleerange == defaults.meleerange &&
		mobj.droppeditem == defaults.droppeditem)
	{
		return false;
	}
	return true;
}

#include "c_dispatch.h"

static const char* ActionPtrString(actionf_p1 func)
{
	int i = 0;
	while (::CodePtrs[i].name != NULL && ::CodePtrs[i].func != func)
	{
		i++;
	}

	if (::CodePtrs[i].name == NULL)
	{
		return "NULL";
	}

	return ::CodePtrs[i].name;
}

static void PrintState(int index)
{
	StatesIterator it = states.find(index);
    if (it == states.end())
	{
		return;
	}

	// Print this state.
	state_t& state = it->second;
	PrintFmt("{:>4d} | sprite:{} frame:{} tics:{} action:{} m1:{} m2:{}\n", index, ::sprnames[state.sprite],
	       state.frame, state.tics, ActionPtrString(state.action), state.misc1,
	       state.misc2);
}

static void PrintMobjinfo(int index)
{
	MobjIterator it = mobjinfo.find(index);
    if (it == mobjinfo.end())
    {
        return;
    }

	PrintFmt("{}", it->second);
}

BEGIN_COMMAND(mobinfo)
{
    if (argc < 2)
    {
        PrintFmt("Must pass one or two mobjinfo indexes.\n");
        return;
    }

    int index1 = atoi(argv[1]);
    if (mobjinfo.find(index1) == mobjinfo.end())
    {
        PrintFmt("Index 1: Not a valid index.\n");
        return;
    }
    int index2 = index1;

    if (argc == 3)
    {
        index2 = atoi(argv[2]);
        if (mobjinfo.find(index2) == mobjinfo.end())
        {
            PrintFmt("Index 2: Not a valid index.\n");
            return;
        }
    }

    if (index2 < index1)
    {
        std::swap(index1, index2);
    }

    for(int i = index1; i <= index2; i++)
    {
        PrintMobjinfo(i);
    }
}
END_COMMAND(mobinfo)

BEGIN_COMMAND(stateinfo)
{
	if (argc < 2)
	{
		PrintFmt("Must pass one or two state indexes.\n");
		return;
	}

	int index1 = atoi(argv[1]);
    if (states.find(index1) == states.end())
	{
		PrintFmt("Index 1: Not a valid index.\n");
		return;
	}
	int index2 = index1;

	if (argc == 3)
	{
		index2 = atoi(argv[2]);
        if (states.find(index2) == states.end())
		{
			PrintFmt("Index 2: Not a valid index.\n");
			return;
		}
	}

	// Swap arguments if need be.
	if (index2 < index1)
	{
		int tmp = index1;
		index1 = index2;
		index2 = tmp;
	}

    // [CMB] TODO: index range here may not correspond correctly -- iterator needed
	for (int i = index1; i <= index2; i++)
	{
		PrintState(i);
	}
}
END_COMMAND(stateinfo)

BEGIN_COMMAND(playstate)
{
	if (argc < 2)
	{
		PrintFmt("Must pass state index.\n");
		return;
	}

	int index = atoi(argv[1]);
	if (!states.contains(index))
	{
		PrintFmt("Not a valid index.\n");
		return;
	}

	OHashTable<int, bool> visited;
	for (;;)
	{
		// Check if we looped back, and exit if so.
		OHashTable<int, bool>::iterator it = visited.find(index);
		if (it != visited.end())
		{
			PrintFmt("Looped back to {}\n", index);
			return;
		}

		PrintState(index);

		// Mark as visited.
		visited.emplace(index, true);

		// Next state.
		index = ::states[index].nextstate;
	}
}
END_COMMAND(playstate)

VERSION_CONTROL(d_dehacked_cpp, "$Id$")
