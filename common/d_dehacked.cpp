// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Internal DeHackEd patch parsing
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include <stdlib.h>

#include "cmdlib.h"
#include "d_dehacked.h"
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
#include "sprite.h"
#include "mobjinfo.h"
#include "state.h"

// Miscellaneous info that used to be constant
struct DehInfo deh = {
    100, // .StartHealth
    50,  // .StartBullets
    100, // .MaxHealth
    200, // .MaxArmor
    1,   // .GreenAC
    2,   // .BlueAC
    200, // .MaxSoulsphere
    100, // .SoulsphereHealth
    200, // .MegasphereHealth
    100, // .GodHealth
    200, // .FAArmor
    2,   // .FAAC
    200, // .KFAArmor
    2,   // .KFAAC
    40,  // .BFGCells (No longer used)
    0,   // .Infight
};

// These are the original heights of every Doom 2 thing. They are used if a patch
// specifies that a thing should be hanging from the ceiling but doesn't specify
// a height for the thing, since these are the heights it probably wants.

static byte OrgHeights[] = {
    56, 56,  56, 56, 16, 56, 8,  16, 64, 8,  56, 56, 56, 56, 56, 64, 8,  64, 56, 100,
    64, 110, 56, 56, 72, 16, 32, 32, 32, 16, 42, 8,  8,  8,  8,  8,  8,  16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 68,  84, 84, 68, 52, 84, 68, 52, 52, 68, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16,  16, 16, 16, 16, 16, 16, 88, 88, 64, 64, 64, 64, 16, 16, 16};

#define LINESIZE 2048

#define CHECKKEY(a, b)        \
	if (!stricmp(Line1, (a))) \
		(b) = atoi(Line2);

static char *PatchFile, *PatchPt;
static char *Line1, *Line2;
static int dversion, pversion;
static BOOL including, includenotext;

// English strings for DeHackEd replacement.
static StringTable ENGStrings;

// This is an offset to be used for computing the text stuff.
// Straight from the DeHackEd source which was
// Written by Greg Lewis, gregl@umich.edu.
static int toff[] = {129044, 129044, 129044, 129284, 129380};

// A conversion array to convert from the 448 code pointers to the 966
// Frames that exist.
// Again taken from the DeHackEd source.
static short codepconv[522] = {
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

static bool BackedUpData = false;
// This is the original data before it gets replaced by a patch.
static const char** OrgSprNames;
static actionf_p1 OrgActionPtrs[::NUMSTATES];

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
	actionf_p1 func;
	int argcount;
	long default_args[MAXSTATEARGS];
};

static const struct CodePtr CodePtrs[] = {
    {"NULL", NULL},
    {"MonsterRail", A_MonsterRail},
    {"FireRailgun", A_FireRailgun},
    {"FireRailgunLeft", A_FireRailgunLeft},
    {"FireRailgunRight", A_FireRailgunRight},
    {"RailWait", A_RailWait},
    {"Light0", A_Light0},
    {"WeaponReady", A_WeaponReady},
    {"Lower", A_Lower},
    {"Raise", A_Raise},
    {"Punch", A_Punch},
    {"ReFire", A_ReFire},
    {"FirePistol", A_FirePistol},
    {"Light1", A_Light1},
    {"FireShotgun", A_FireShotgun},
    {"Light2", A_Light2},
    {"FireShotgun2", A_FireShotgun2},
    {"CheckReload", A_CheckReload},
    {"OpenShotgun2", A_OpenShotgun2},
    {"LoadShotgun2", A_LoadShotgun2},
    {"CloseShotgun2", A_CloseShotgun2},
    {"FireCGun", A_FireCGun},
    {"GunFlash", A_GunFlash},
    {"FireMissile", A_FireMissile},
    {"Saw", A_Saw},
    {"FirePlasma", A_FirePlasma},
    {"BFGsound", A_BFGsound},
    {"FireBFG", A_FireBFG},
    {"BFGSpray", A_BFGSpray},
    {"Explode", A_Explode},
    {"Pain", A_Pain},
    {"PlayerScream", A_PlayerScream},
    {"Fall", A_Fall},
    {"XScream", A_XScream},
    {"Look", A_Look},
    {"Chase", A_Chase},
    {"FaceTarget", A_FaceTarget},
    {"PosAttack", A_PosAttack},
    {"Scream", A_Scream},
    {"SPosAttack", A_SPosAttack},
    {"VileChase", A_VileChase},
    {"VileStart", A_VileStart},
    {"VileTarget", A_VileTarget},
    {"VileAttack", A_VileAttack},
    {"StartFire", A_StartFire},
    {"Fire", A_Fire},
    {"FireCrackle", A_FireCrackle},
    {"Tracer", A_Tracer},
    {"SkelWhoosh", A_SkelWhoosh},
    {"SkelFist", A_SkelFist},
    {"SkelMissile", A_SkelMissile},
    {"FatRaise", A_FatRaise},
    {"FatAttack1", A_FatAttack1},
    {"FatAttack2", A_FatAttack2},
    {"FatAttack3", A_FatAttack3},
    {"BossDeath", A_BossDeath},
    {"CPosAttack", A_CPosAttack},
    {"CPosRefire", A_CPosRefire},
    {"TroopAttack", A_TroopAttack},
    {"SargAttack", A_SargAttack},
    {"HeadAttack", A_HeadAttack},
    {"BruisAttack", A_BruisAttack},
    {"SkullAttack", A_SkullAttack},
    {"Metal", A_Metal},
    {"SpidRefire", A_SpidRefire},
    {"BabyMetal", A_BabyMetal},
    {"BspiAttack", A_BspiAttack},
    {"Hoof", A_Hoof},
    {"CyberAttack", A_CyberAttack},
    {"PainAttack", A_PainAttack},
    {"PainDie", A_PainDie},
    {"KeenDie", A_KeenDie},
    {"BrainPain", A_BrainPain},
    {"BrainScream", A_BrainScream},
    {"BrainDie", A_BrainDie},
    {"BrainAwake", A_BrainAwake},
    {"BrainSpit", A_BrainSpit},
    {"SpawnSound", A_SpawnSound},
    {"SpawnFly", A_SpawnFly},
    {"BrainExplode", A_BrainExplode},
    {"Detonate", A_Detonate},     // killough 8/9/98
    {"Mushroom", A_Mushroom},     // killough 10/98
    {"Die", A_Die},               // killough 11/98
    {"Spawn", A_Spawn},           // killough 11/98
    {"Turn", A_Turn},             // killough 11/98
    {"Face", A_Face},             // killough 11/98
    {"Scratch", A_Scratch},       // killough 11/98
    {"PlaySound", A_PlaySound},   // killough 11/98
    {"RandomJump", A_RandomJump}, // killough 11/98
    {"LineEffect", A_LineEffect}, // killough 11/98
    {"BetaSkullAttack", A_BetaSkullAttack},

    // MBF21 Pointers
    {"SpawnObject", A_SpawnObject, 8},
    {"MonsterProjectile", A_MonsterProjectile, 5},
    {"MonsterBulletAttack", A_MonsterBulletAttack, 5, {0, 0, 1, 3, 5}},
    {"MonsterMeleeAttack", A_MonsterMeleeAttack, 4, {3, 8, 0, 0}},
    {"RadiusDamage", A_RadiusDamage, 2},
    {"NoiseAlert", A_NoiseAlert, 0},
    {"HealChase", A_HealChase, 2},
    {"SeekTracer", A_SeekTracer, 2},
    {"FindTracer", A_FindTracer, 2, {0, 10}},
    {"ClearTracer", A_ClearTracer, 0},
    {"JumpIfHealthBelow", A_JumpIfHealthBelow, 2},
    {"JumpIfTargetInSight", A_JumpIfTargetInSight, 2},
    {"JumpIfTargetCloser", A_JumpIfTargetCloser, 2},
    {"JumpIfTracerInSight", A_JumpIfTracerInSight, 2},
    {"JumpIfTracerCloser", A_JumpIfTracerCloser, 2},
    {"JumpIfFlagsSet", A_JumpIfFlagsSet, 3},
    {"AddFlags", A_AddFlags, 2},
    {"RemoveFlags", A_RemoveFlags, 2},
    // MBF21 Weapon Pointers
    {"WeaponProjectile", A_WeaponProjectile, 5},
    {"WeaponBulletAttack", A_WeaponBulletAttack, 5, {0, 0, 1, 5, 3}},
    {"WeaponMeleeAttack", A_WeaponMeleeAttack, 5, {2, 10, 1 * FRACUNIT, 0, 0}},
    {"WeaponSound", A_WeaponSound, 2},
    {"WeaponAlert", A_WeaponAlert, 0},
    {"WeaponJump", A_WeaponJump, 2},
    {"ConsumeAmmo", A_ConsumeAmmo, 1},
    {"CheckAmmo", A_CheckAmmo, 2},
    {"RefireTo", A_RefireTo, 2},
    {"GunFlashTo", A_GunFlashTo, 2},

    {NULL, NULL}};

struct Key
{
	const char* name;
	ptrdiff_t offset;
};

static int PatchThing(int);
static int PatchSound(int);
static int PatchFrame(int);
static int PatchSprite(int);
static int PatchSprites(int);
static int PatchAmmo(int);
static int PatchWeapon(int);
static int PatchPointer(int);
static int PatchCheats(int);
static int PatchMisc(int);
static int PatchText(int);
static int PatchStrings(int);
static int PatchPars(int);
static int PatchCodePtrs(int);
static int PatchMusic(int);
static int DoInclude(int);

static const struct
{
	const char* name;
	int (*func)(int);
} Modes[] = {
    // https://eternity.youfailit.net/wiki/DeHackEd_/_BEX_Reference

    // These appear in .deh and .bex files
    {"Thing", PatchThing},
    {"Sound", PatchSound},
    {"Frame", PatchFrame},
    {"Sprite", PatchSprite},
    {"Ammo", PatchAmmo},
    {"Weapon", PatchWeapon},
    {"Pointer", PatchPointer},
    {"Cheat", PatchCheats},
    {"Misc", PatchMisc},
    {"Text", PatchText},
    // These appear in .bex files
    {"include", DoInclude},
    {"[STRINGS]", PatchStrings},
    {"[PARS]", PatchPars},
    {"[CODEPTR]", PatchCodePtrs},
	{"[SPRITES]", PatchSprites},
    // Eternity engine added a few more features to BEX
    {"[MUSIC]", PatchMusic},
    {NULL, NULL},
};

static int HandleMode(const char* mode, int num);
static BOOL HandleKey(const struct Key* keys, void* structure, const char* key, int value,
                      const int structsize = 0);
static void BackupData(void);
static BOOL ReadChars(char** stuff, int size);
static char* igets(void);
static int GetLine(void);

static size_t filelen = 0; // Be quiet, gcc

#define IS_AT_PATCH_SIZE (((PatchPt - 1) - PatchFile) == (int)filelen)

static void PrintUnknown(const char* key, const char* loc, const size_t idx)
{
	DPrintf("Unknown key %s encountered in %s (%" PRIuSIZE ").\n", key, loc, idx);
}

static int HandleMode(const char* mode, int num)
{
	int i = 0;

	while (Modes[i].name && stricmp(Modes[i].name, mode))
	{
		i++;
	}

	if (Modes[i].name)
	{
		return Modes[i].func(num);
	}

	// Handle unknown or unimplemented data
	DPrintf("Unknown chunk %s encountered. Skipping.\n", mode);
	do
	{
		i = GetLine();
	} while (i == 1);

	return i;
}

static BOOL HandleKey(const struct Key* keys, void* structure, const char* key, int value,
                      const int structsize)
{
	while (keys->name && stricmp(keys->name, key))
		keys++;

	if (structsize && keys->offset + (int)sizeof(int) > structsize)
	{
		// Handle unknown or unimplemented data
		DPrintf("DeHackEd: Cannot apply key %s, offset would overrun.\n", keys->name);
		return false;
	}

	if (keys->name)
	{
		*((int*)(((byte*)structure) + keys->offset)) = value;
		return false;
	}

	return true;
}

typedef struct
{
	state_t backupStates[::NUMSTATES];
	mobjinfo_t backupMobjInfo[::NUMMOBJTYPES];
	weaponinfo_t backupWeaponInfo[NUMWEAPONS + 1];
	const char** backupSprnames;
	int backupMaxAmmo[NUMAMMO];
	int backupClipAmmo[NUMAMMO];
	DehInfo backupDeh;
} DoomBackup;

DoomBackup doomBackup = {};

static void BackupData(void)
{
	int i;

	if (BackedUpData)
	{
		return;
	}

	// backup sprites
	OrgSprNames = (const char**) M_Calloc(::NUMSPRITES + 1, sizeof(char*));
	for (i = 0; i < ::NUMSPRITES; i++)
	{
		OrgSprNames[i] = strdup(sprnames[i]);
	}
	OrgSprNames[NUMSPRITES] = NULL;

	doomBackup.backupSprnames = (const char**)M_Calloc(::NUMSPRITES + 1, sizeof(char*));
	for (i = 0; i < ::NUMSPRITES; i++)
	{
		doomBackup.backupSprnames[i] = strdup(sprnames[i]);
	}

	// backup states
	for (i = 0; i < ::NUMSTATES; i++)
	{
		OrgActionPtrs[i] = states[i].action;
	}

	std::copy(states, states + ::NUMSTATES, doomBackup.backupStates);
	std::copy(mobjinfo, mobjinfo + ::NUMMOBJTYPES, doomBackup.backupMobjInfo);
	std::copy(weaponinfo, weaponinfo + ::NUMWEAPONS + 1, doomBackup.backupWeaponInfo);
	// std::copy(sprnames, sprnames + ::NUMSPRITES + 1, backupSprnames);
	std::copy(clipammo, clipammo + ::NUMAMMO, doomBackup.backupClipAmmo);
	std::copy(maxammo, maxammo + ::NUMAMMO, doomBackup.backupMaxAmmo);
	doomBackup.backupDeh = deh;

	BackedUpData = true;
}

void D_UndoDehPatch()
{
	int i;

	if (!BackedUpData)
	{
		return;
	}

	// reset sprites
	// reset states
	// reset reset mobjinfo

	for (i = 0; i < ::NUMSPRITES; i++)
	{;
		free((char*)OrgSprNames[i]);
	}
	M_Free(OrgSprNames);

	
	/*
	for (i = 0; i < ::NUMSTATES; i++)
	{
		::states[i].action = ::OrgActionPtrs[i];
	}
	*/

	D_Initialize_sprnames(doomBackup.backupSprnames, ::NUMSPRITES);
	D_Initialize_States(doomBackup.backupStates, ::NUMSTATES);
	D_Initialize_Mobjinfo(doomBackup.backupMobjInfo, ::NUMMOBJTYPES);

	for (i = 0; i < ::NUMSPRITES; i++)
	{
		free((char*)doomBackup.backupSprnames[i]);
	}
	M_Free(doomBackup.backupSprnames);
	
	extern bool isFast;
	isFast = false;

	std::copy(doomBackup.backupWeaponInfo, doomBackup.backupWeaponInfo + ::NUMWEAPONS,
	          weaponinfo);
	std::copy(doomBackup.backupClipAmmo, doomBackup.backupClipAmmo + ::NUMAMMO, clipammo);
	std::copy(doomBackup.backupMaxAmmo, doomBackup.backupMaxAmmo + ::NUMAMMO, maxammo);

	deh = doomBackup.backupDeh;

	BackedUpData = false;
}

static BOOL ReadChars(char** stuff, int size)
{
	char* str = *stuff;

	if (!size)
	{
		*str = 0;
		return true;
	}

	do
	{
		// Ignore carriage returns
		if (*PatchPt != '\r')
		{
			*str++ = *PatchPt;
		}
		else
		{
			size++;
		}

		PatchPt++;
	} while (--size);

	*str = 0;
	return true;
}

static void ReplaceSpecialChars(char* str)
{
	char *p = str, c;
	int i;

	while ((c = *p++))
	{
		if (c != '\\')
		{
			*str++ = c;
		}
		else
		{
			switch (*p)
			{
			case 'n':
			case 'N':
				*str++ = '\n';
				break;
			case 't':
			case 'T':
				*str++ = '\t';
				break;
			case 'r':
			case 'R':
				*str++ = '\r';
				break;
			case 'x':
			case 'X':
				c = 0;
				p++;
				for (i = 0; i < 2; i++)
				{
					c <<= 4;
					if (*p >= '0' && *p <= '9')
					{
						c += *p - '0';
					}
					else if (*p >= 'a' && *p <= 'f')
					{
						c += 10 + *p - 'a';
					}
					else if (*p >= 'A' && *p <= 'F')
					{
						c += 10 + *p - 'A';
					}
					else
					{
						break;
					}
					p++;
				}
				*str++ = c;
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
				for (i = 0; i < 3; i++)
				{
					c <<= 3;
					if (*p >= '0' && *p <= '7')
					{
						c += *p - '0';
					}
					else
					{
						break;
					}
					p++;
				}
				*str++ = c;
				break;
			default:
				*str++ = *p;
				break;
			}
			p++;
		}
	}
	*str = 0;
}

static char* skipwhite(char* str)
{
	if (str)
	{
		while (*str && isspace(*str))
		{
			str++;
		}
	}
	return str;
}

static void stripwhite(char* str)
{
	char* end = str + strlen(str) - 1;

	while (end >= str && isspace(*end))
	{
		end--;
	}

	end[1] = '\0';
}

static char* igets(void)
{
	char* line;

	if (!PatchPt || IS_AT_PATCH_SIZE)
	{
		return NULL;
	}

	if (*PatchPt == '\0')
	{
		return NULL;
	}

	line = PatchPt;

	while (*PatchPt != '\n' && *PatchPt != '\0')
	{
		PatchPt++;
	}

	if (*PatchPt == '\n')
	{
		*PatchPt++ = 0;
	}

	return line;
}

static int GetLine(void)
{
	char *line, *line2;

	do
	{
		while ((line = igets()))
		{
			if (line[0] != '#') // Skip comment lines
			{
				break;
			}
		}

		Line1 = skipwhite(line);
	} while (Line1 && *Line1 == 0); // Loop until we get a line with
	                                // more than just whitespace.

	if (!Line1)
	{
		return 0;
	}

	line = strchr(Line1, '=');

	if (line)
	{ // We have an '=' in the input line
		line2 = line;
		while (--line2 >= Line1)
		{
			if (*line2 > ' ')
			{
				break;
			}
		}

		if (line2 < Line1)
		{
			return 0; // Nothing before '='
		}

		*(line2 + 1) = 0;

		line++;
		while (*line && *line <= ' ')
		{
			line++;
		}

		if (*line == 0)
		{
			return 0; // Nothing after '='
		}

		Line2 = line;

		return 1;
	}
	else
	{ // No '=' in input line
		line = Line1 + 1;
		while (*line > ' ')
		{
			line++; // Get beyond first word
		}

		*line++ = 0;
		while (*line && *line <= ' ')
		{
			line++; // Skip white space
		}

		//.bex files don't have this restriction
		// if (*line == 0)
		//	return 0;			// No second word

		Line2 = line;

		return 2;
	}
}

static int PatchThing(int thingy)
{

	enum
	{
		// Boom flags
		MF_TRANSLATION = 0x0c000000, // if 0x4 0x8 or 0xc, use a translation
		// MF_TRANSSHIFT = 26,       // table for player colormaps
		// A couple of Boom flags that don't exist in ZDoom
		MF_SLIDE = 0x00002000,       // Player: keep info about sliding along walls.
		MF_TRANSLUCENT = 0x80000000, // Translucent sprite?
		                             // MBF flags: TOUCHY is remapped to flags6, FRIEND is
		                             // turned into FRIENDLY, and finally BOUNCES is
		                             // replaced by bouncetypes with the BOUNCES_MBF bit.
	};

	size_t thingNum = thingy;

	// flags can be specified by name (a .bex extension):
	struct flagsystem_t
	{
		short Bit;
		short WhichFlags;
		const char* Name;
	};

	flagsystem_t bitnames[73] = {
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
	    {28, 0, "TOUCHY"},  // UNUSED FOR NOW
	    {29, 0, "BOUNCES"}, // UNUSED FOR NOW
	    {30, 0, "FRIEND"},
	    {31, 0, "TRANSLUCENT"}, // BOOM compatibility
	    {30, 0, "STEALTH"},

	    // TRANSLUCENT... HACKY BUT HEH.
	    {0, 2, "TRANSLUC25"},
	    {1, 2, "TRANSLUC50"},
	    {2, 2, "TRANSLUC75"},

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

	// MBF21 Bitname system
	flagsystem_t mbf_bitnames[19] = {
	    {0, 1, "LOGRAV"},         {1, 3, "SHORTMRANGE"},    {2, 3, "DMGIGNORED"},
	    {3, 3, "NORADIUSDMG"},    {4, 3, "FORCERADIUSDMG"}, {5, 3, "HIGHERMPROB"},
	    {6, 3, "RANGEHALF"},      {17, 1, "NOTHRESHOLD"},   {8, 3, "LONGMELEE"},
	    {15, 1, "BOSS"},          {10, 3, "MAP07BOSS1"},    {11, 3, "MAP07BOSS2"},
	    {12, 3, "E1M8BOSS"},      {13, 3, "E2M8BOSS"},      {14, 3, "E3M8BOSS"},
	    {15, 3, "E4M6BOSS"},      {16, 3, "E4M8BOSS"},      {8, 1, "RIP"},
	    {18, 3, "FULLVOLSOUNDS"},
	};

	int result;
	int oldflags;
	bool hadTranslucency = false;
	mobjinfo_t *info, dummy;
	int *ednum, dummyed;
	bool hadHeight = false;
	bool gibhealth = false;

	info = &dummy;
	ednum = &dummyed;

	// [CMB] TODO: find the index range for the Thing in the tables
    // [CMB] TODO: ensure capacity and create a new one if necessary
	thingNum--;
	if (thingNum < 0)
	{
		DPrintf("Thing %" PRIuSIZE " out of range.\n", thingNum);
	}
    if(thingNum >= ::num_mobjinfo_types)
    {
#if defined _DEBUG
        DPrintf("Thing %" PRIuSIZE " requires allocation.\n", thingNum);
#endif
        D_EnsureMobjInfoCapacity(thingNum);
		/*
        mobjinfo_t* newthing = (mobjinfo_t*) M_Malloc(sizeof(mobjinfo_t));
        mobjinfo[thingNum] = *newthing;
        info = newthing;
		*/
		info = &mobjinfo[thingNum];
		*ednum = *&info->doomednum;
    }
	else
	{
		info = &mobjinfo[thingNum];
		*ednum = *&info->doomednum;
#if defined _DEBUG
		DPrintf("Thing %" PRIuSIZE " found.\n", thingNum);
#endif
	}

	oldflags = info->flags;

	while ((result = GetLine()) == 1)
	{
		size_t sndmap = atoi(Line2);

		if (sndmap >= ARRAY_LENGTH(SoundMap))
		{
			sndmap = 0;
		}

		size_t val = atoi(Line2);
		int linelen = strlen(Line1);

		if (stricmp(Line1 + linelen - 6, " frame") == 0)
		{
			statenum_t state = (statenum_t)val;

			if (!strnicmp(Line1, "Initial frame", 13))
			{
				info->spawnstate = state;
			}
			else if (!strnicmp(Line1, "First moving", 12))
			{
				info->seestate = state;
			}
			else if (!strnicmp(Line1, "Injury", 6))
			{
				info->painstate = state;
			}
			else if (!strnicmp(Line1, "Close attack", 12))
			{
				info->meleestate = state;
			}
			else if (!strnicmp(Line1, "Far attack", 10))
			{
				info->missilestate = state;
			}
			else if (!strnicmp(Line1, "Death", 5))
			{
				info->deathstate = state;
			}
			else if (!strnicmp(Line1, "Exploding", 9))
			{
				info->xdeathstate = state;
			}
			else if (!strnicmp(Line1, "Respawn", 7))
			{
				info->raisestate = state;
			}
		}
		else if (stricmp(Line1 + linelen - 6, " sound") == 0)
		{
			char* snd;

			if (val == 0 || val >= ARRAY_LENGTH(SoundMap))
			{
				val = 0;
			}

			snd = (char*)SoundMap[val];

			if (!strnicmp(Line1, "Alert", 5))
			{
				info->seesound = snd;
			}
			else if (!strnicmp(Line1, "Attack", 6))
			{
				info->attacksound = snd;
			}
			else if (!strnicmp(Line1, "Pain", 4))
			{
				info->painsound = snd;
			}
			else if (!strnicmp(Line1, "Death", 5))
			{
				info->deathsound = snd;
			}
			else if (!strnicmp(Line1, "Action", 6))
			{
				info->activesound = snd;
			}
			else if (!strnicmp(Line1, "Rip", 3))
			{
				info->ripsound = snd;
			}
		}
		else if (stricmp(Line1, "Projectile group") == 0)
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
		else if (stricmp(Line1, "Infighting group") == 0)
		{
			info->infighting_group = val;

			if (info->infighting_group < 0)
			{
				I_Error("Infighting groups must be >= 0 (check your DEHACKED "
				        "entry, and correct it!)\n");
			}
			info->infighting_group = val + IG_END;
		}
		else if (stricmp(Line1, "Missile damage") == 0)
		{
			info->damage = val;
		}
		else if (stricmp(Line1, "Reaction time") == 0)
		{
			info->reactiontime = val;
		}
		else if (stricmp(Line1, "Translucency") == 0)
		{
			info->translucency = val;
			hadTranslucency = true;
		}
		else if (stricmp(Line1, "Dropped item") == 0)
		{
			if (val - 1 < 0 || val - 1 >= ::num_mobjinfo_types)
			{
				I_Error("Dropped item out of range. Check your DEHACKED.\n");
			}
			info->droppeditem = (mobjtype_t)(int)(val - 1); // deh is mobj + 1
		}
		else if (stricmp(Line1, "Splash group") == 0)
		{
			info->splash_group = val;
			if (info->splash_group < 0)
			{
				I_Error("Splash groups must be >= 0 (check your DEHACKED entry, "
				        "and correct it!)\n");
			}
			info->splash_group = val + SG_END;
		}
		else if (stricmp(Line1, "Pain chance") == 0)
		{
			info->painchance = (SWORD)val;
		}
		else if (stricmp(Line1, "Melee range") == 0)
		{
			info->meleerange = val;
		}
		else if (stricmp(Line1, "Hit points") == 0)
		{
			info->spawnhealth = val;
		}
		else if (stricmp(Line1, "Fast speed") == 0)
		{
			info->altspeed = val;
		}
		else if (stricmp(Line1, "Gib health") == 0)
		{
			gibhealth = true;
			info->gibhealth = val;

			// Special hack: DEH values are always positive, and since a gib is
			// always negative, a positive value will thus become negative.
			if (info->gibhealth > 0)
				info->gibhealth = -info->gibhealth;
		}
		else if (stricmp(Line1, "MBF21 Bits") == 0)
		{
			int value[4] = {0, 0, 0};
			bool vchanged[4] = {false, false, false};
			char* strval;

			for (strval = Line2; (strval = strtok(strval, ",+| \t\f\r")); strval = NULL)
			{
				if (IsNum(strval))
				{
					int tempval = atoi(strval);

					if (tempval & MF2_LOGRAV)
					{
						info->flags2 |= MF2_LOGRAV;
					}

					if (tempval & MF3_SHORTMRANGE)
					{
						info->flags3 |= MF3_SHORTMRANGE;
					}

					if (tempval & MF3_DMGIGNORED)
					{
						info->flags3 |= MF3_DMGIGNORED;
					}

					if (tempval & MF3_NORADIUSDMG)
					{
						info->flags3 |= MF3_NORADIUSDMG;
					}

					if (tempval & MF3_FORCERADIUSDMG)
					{
						info->flags3 |= MF3_FORCERADIUSDMG;
					}

					if (tempval & MF3_HIGHERMPROB)
					{
						info->flags3 |= MF3_HIGHERMPROB;
					}

					if (tempval & MF3_RANGEHALF)
					{
						info->flags3 |= MF3_RANGEHALF;
					}

					if (tempval & MF3_NOTHRESHOLD)
					{
						info->flags3 |= MF3_NOTHRESHOLD;
					}

					if (tempval & MF3_LONGMELEE)
					{
						info->flags3 |= MF3_LONGMELEE;
					}

					if (tempval & MF2_BOSS)
					{
						info->flags2 |= MF2_BOSS;
					}

					if (tempval & MF3_MAP07BOSS1)
					{
						info->flags3 |= MF3_MAP07BOSS1;
					}

					if (tempval & MF3_MAP07BOSS2)
					{
						info->flags3 |= MF3_MAP07BOSS2;
					}

					if (tempval & MF3_E1M8BOSS)
					{
						info->flags3 |= MF3_E1M8BOSS;
					}

					if (tempval & MF3_E2M8BOSS)
					{
						info->flags3 |= MF3_E2M8BOSS;
					}

					if (tempval & MF3_E3M8BOSS)
					{
						info->flags3 |= MF3_E3M8BOSS;
					}

					if (tempval & MF3_E4M6BOSS)
					{
						info->flags3 |= MF3_E4M6BOSS;
					}

					if (tempval & MF3_E4M8BOSS)
					{
						info->flags3 |= MF3_E4M8BOSS;
					}

					if (tempval & BIT(17)) // MBF21 RIP is 1 << 17
					{
						info->flags2 |= MF2_RIP;
					}

					if (tempval & MF3_FULLVOLSOUNDS)
					{
						info->flags3 |= MF3_FULLVOLSOUNDS;
					}

					value[3] |= atoi(strval);
					vchanged[3] = true;
				}
				else
				{
					size_t i;

					for (i = 0; i < ARRAY_LENGTH(mbf_bitnames); i++)
					{
						if (!stricmp(strval, mbf_bitnames[i].Name))
						{
							vchanged[mbf_bitnames[i].WhichFlags] = true;
							value[mbf_bitnames[i].WhichFlags] |= 1
							                                     << (mbf_bitnames[i].Bit);
							break;
						}
					}

					if (i == ARRAY_LENGTH(mbf_bitnames))
					{
						DPrintf("Unknown bit mnemonic %s\n", strval);
					}
				}
			}
			if (vchanged[3])
			{
				info->flags3 = value[3];
			}
		}
		else if (stricmp(Line1, "Height") == 0)
		{
			info->height = val;
			hadHeight = true;
		}
		else if (!stricmp(Line1, "Speed"))
		{
			info->speed = val;
		}
		else if (stricmp(Line1, "Width") == 0)
		{
			info->radius = val;
		}
		else if (!stricmp(Line1, "Bits"))
		{
			int value[4] = {0, 0, 0};
			bool vchanged[4] = {false, false, false};
			char* strval;

			for (strval = Line2; (strval = strtok(strval, ",+| \t\f\r")); strval = NULL)
			{
				if (IsNum(strval))
				{
					// Force the top 4 bits to 0 so that the user is forced
					// to use the mnemonics to change them.

					// I have no idea why everyone insists on using strtol here
					// even though it fails dismally if a value is parsed where
					// the highest bit it set. Do people really use negative
					// values here? Let's better be safe and check both.
					value[0] |= atoi(strval);
					vchanged[0] = true;
				}
				else
				{
					size_t i;

					for (i = 0; i < ARRAY_LENGTH(bitnames); i++)
					{
						if (!stricmp(strval, bitnames[i].Name))
						{
							vchanged[bitnames[i].WhichFlags] = true;
							value[bitnames[i].WhichFlags] |= 1 << (bitnames[i].Bit);
							break;
						}
					}

					if (i == ARRAY_LENGTH(bitnames))
						DPrintf("Unknown bit mnemonic %s\n", strval);
				}
			}
			if (vchanged[0])
			{
				if (value[0] & MF_TRANSLUCENT)
				{
					info->translucency =
					    TRANSLUC50; // Correct value should be 0.66 (BOOM)...
					hadTranslucency = true;
				}

				// Unsupported flags have to be announced for developers...
				if (value[0] & MF_TOUCHY)
				{
					DPrintf("[DEH Bits] Unsupported MBF flag TOUCHY.\n");
					value[0] &= ~MF_TOUCHY;
				}

				if (value[0] & MF_BOUNCES)
					DPrintf("[DEH Bits] MBF flag BOUNCES is partially supported. Use "
					        "it at your own risk!\n");

				if (value[0] & MF_FRIEND)
				{
					DPrintf("[DEH Bits] Unsupported MBF flag FRIEND.\n");
					value[0] &= ~MF_FRIEND;
				}

				info->flags = value[0];
			}
			if (vchanged[1])
			{
				info->flags2 = value[1];
			}
			if (vchanged[2])
			{
				if (value[2] & 7)
				{
					hadTranslucency = true;

					if (value[2] & 1)
						info->translucency = TRANSLUC25;
					else if (value[2] & 2)
						info->translucency = TRANSLUC50;
					else if (value[2] & 4)
						info->translucency = TRANSLUC75;
				}
			}
			if (vchanged[3])
			{
				info->flags3 = value[3];
			}
		}
		else if (stricmp(Line1, "ID #") == 0)
		{
			*&info->doomednum = (SDWORD)val;
		}
		else if (stricmp(Line1, "Mass") == 0)
		{
			info->mass = val;
		}
		else
		{
			PrintUnknown(Line1, "Thing", thingNum);
		}
	}

	// [ML] Set a thing's "real world height" to what's being offered here,
	// so it's consistent from the patch
	if (info != &dummy)
	{
		if (hadHeight && thingNum < sizeof(OrgHeights))
		{
			info->cdheight = info->height;
		}

		if (info->flags & MF_SPAWNCEILING && !hadHeight && thingNum < sizeof(OrgHeights))
		{
			info->height = OrgHeights[thingNum] * FRACUNIT;
		}

		// Set a default gibhealth if none was assigned.
		if (!gibhealth && info->spawnhealth && !info->gibhealth)
		{
			info->gibhealth = -info->spawnhealth;
		}
	}

	return result;
}

static int PatchSound(int soundNum)
{
	int result;

	DPrintf("Sound %d (no longer supported)\n", soundNum);
	/*
	    sfxinfo_t *info, dummy;
	    int offset = 0;
	    if (soundNum >= 1 && soundNum <= NUMSFX) {
	        info = &S_sfx[soundNum];
	    } else {
	        info = &dummy;
	        DPrintf ("Sound %d out of range.\n");
	    }
	*/
	while ((result = GetLine()) == 1)
	{
		/*
		if (!stricmp  ("Offset", Line1))
		    offset = atoi (Line2);
		else CHECKKEY ("Zero/One",			info->singularity)
		else CHECKKEY ("Value",				info->priority)
		else CHECKKEY ("Zero 1",			info->link)
		else CHECKKEY ("Neg. One 1",		info->pitch)
		else CHECKKEY ("Neg. One 2",		info->volume)
		else CHECKKEY ("Zero 2",			info->data)
		else CHECKKEY ("Zero 3",			info->usefulness)
		else CHECKKEY ("Zero 4",			info->lumpnum)
		else PrintUnknown(Line1, "Sound", soundNum);
		*/
	}
	/*
	    if (offset) {
	        // Calculate offset from start of sound names
	        offset -= toff[dversion] + 21076;

	        if (offset <= 64)			// pistol .. bfg
	            offset >>= 3;
	        else if (offset <= 260)		// sawup .. oof
	            offset = (offset + 4) >> 3;
	        else						// telept .. skeatk
	            offset = (offset + 8) >> 3;

	        if (offset >= 0 && offset < NUMSFX) {
	            S_sfx[soundNum].name = OrgSfxNames[offset + 1];
	        } else {
	            DPrintf ("Sound name %d out of range.\n", offset + 1);
	        }
	    }
	*/
	return result;
}

static int PatchFrame(int frameNum)
{
	static const struct Key keys[] = {{"Sprite number", offsetof(state_t, sprite)},
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
	                                  {"Args8", offsetof(state_t, args[7])},
	                                  {NULL, 0}};
	int result;
	state_t *info, dummy;

	static const struct
	{
		short Bit;
		const char* Name;
	} bitnames[] = {
	    {1, "SKILL5FAST"},
	};

	if(frameNum < 0)
	{
		info = &dummy;
		DPrintf("Frame %d out of range\n", frameNum);
	}

    // [CMB] TODO: ensure capacity if outside current limits
    if (frameNum >= 0 && frameNum < ::num_state_t_types)
    {
        info = &states[frameNum];
        DPrintf("Frame %d\n", frameNum);
    }
    else
    {
#if defined _DEBUG
        DPrintf("Frame %" PRIuSIZE " requires allocation.\n", frameNum);
#endif
        D_EnsureStateCapacity(frameNum);
		/*
        state_t* newstate = (state_t*) M_Malloc(sizeof(state_t));
        states[frameNum] = *newstate;
		*/
        info = &states[frameNum];
    }

	while ((result = GetLine()) == 1)
	{
		size_t val = atoi(Line2);
		int linelen = strlen(Line1);

		if (HandleKey(keys, info, Line1, val, sizeof(*info)))
		{
			if (linelen == 10)
			{
				if (stricmp(Line1, "MBF21 Bits") == 0)
				{
					int value = 0;
					bool vchanged = false;
					char* strval;

					for (strval = Line2; (strval = strtok(strval, ",+| \t\f\r"));
					     strval = NULL)
					{
						if (IsNum(strval))
						{
							// Force the top 4 bits to 0 so that the user is forced
							// to use the mnemonics to change them.

							// I have no idea why everyone insists on using strtol here
							// even though it fails dismally if a value is parsed where
							// the highest bit it set. Do people really use negative
							// values here? Let's better be safe and check both.
							value |= atoi(strval);
							vchanged = true;
						}
						else
						{
							size_t i;

							for (i = 0; i < ARRAY_LENGTH(bitnames); i++)
							{
								if (!stricmp(strval, bitnames[i].Name))
								{
									vchanged = true;
									value |= 1 << (bitnames[i].Bit);
									break;
								}
							}

							if (i == ARRAY_LENGTH(bitnames))
							{
								DPrintf("Unknown bit mnemonic %s\n", strval);
							}
						}
					}
					if (vchanged)
					{
						info->flags = value; // Weapon Flags
					}
				}
			}
			else
			{
				PrintUnknown(Line1, "Frame", frameNum);
			}
		}
	}
#if defined _DEBUG
	const char* sprsub = (info->sprite > 0 && info->sprite < num_spritenum_t_types) ? sprnames[info->sprite] : "";
	DPrintf("FRAME %d: Duration: %d, Next: %d, SprNum: %d(%s), SprSub: %d\n", frameNum,
	       info->tics, info->nextstate, info->sprite, sprsub,
			info->frame);
#endif

	return result;
}

static int PatchSprite(int sprNum)
{
	int result;
	int offset = 0;

	if (sprNum >= 0 && sprNum < ::NUMSPRITES)
	{
#if defined _DEBUG
		DPrintf("Sprite %d\n", sprNum);
#endif
	}
	else
	{
		DPrintf("Sprite %d out of range.\n", sprNum);
		sprNum = -1;
	}
	while ((result = GetLine()) == 1)
	{
		if (!stricmp("Offset", Line1))
		{
			offset = atoi(Line2);
		}
		else
		{
			PrintUnknown(Line1, "Sprite", sprNum);
		}
	}

	if (offset > 0 && sprNum != -1)
	{
		// Calculate offset from beginning of sprite names.
		offset = (offset - toff[dversion] - 22044) / 8;

		if (offset >= 0 && offset < ::num_spritenum_t_types)
		{
			sprnames[sprNum] = OrgSprNames[offset];
		}
		else
		{
			DPrintf("Sprite name %d out of range.\n", offset);
		}
	}

	return result;
}

/**
 * @brief patch sprites underneath SPRITES header
 * 
 * @param dummy - int value for function pointer
 * @return int - success or failure
 */
static int PatchSprites(int dummy)
{
	/* TODO
	 1. read each line beneath [SPRITES] table header
	 2. read individual line
	 3. check left hand value is a number
	 4. check right hand value is a four character string
	 5. check right hand value references an existing sprite
	 6. patch the sprite with the new name
	*/
	static size_t maxsprlen = 4;
	int result;
#if defined _DEBUG
	static int call_amt = 0;
	Printf_Bold("[SPRITES] call amt: %d\n", ++call_amt);
#endif

	// [CMB] static char* Line1 is the left hand side
	// [CMB] static char* Line2 is the right hand side
	while((result = GetLine()) == 1)
	{
		const char* zSprIdx = Line1;
        char* newSprName = skipwhite(Line2);
		stripwhite(newSprName);
        
        if(!newSprName && strlen(newSprName) > maxsprlen)
        {
            DPrintf("Invalid sprite replace at index %s\n", zSprIdx);
            return -1;
        }
        
        // If it's -1 there are two possibilities: it didn't find it or doesn't have enough space
        int sprIdx = D_FindOrgSpriteIndex(OrgSprNames, zSprIdx);
        if (sprIdx == -1 && IsNum(zSprIdx))
        {
            sprIdx = atoi(Line1);
            D_EnsureSprnamesCapacity(sprIdx);
        }
        if(sprIdx >= 0)
        {
#if defined _DEBUG
			const char* prevSprName =
			    sprnames[sprIdx] != NULL ? sprnames[sprIdx] : "No Sprite";
			DPrintf("Patching sprite at %d with name %s with new name %s\n",
			        sprIdx, prevSprName, newSprName);
#endif
			// this is based on the assumption that sprnames is 0'd on initial allocation
			if (sprnames[sprIdx])
			{
				char* s = const_cast<char*>(sprnames[sprIdx]);
				free(s);
			}
			sprnames[sprIdx] = strdup(newSprName);
        }
	}


#if defined _DEBUG
	for (int i = 0; i < ::num_spritenum_t_types; ++i)
	{
		Printf_Bold("Sprite[%d]=%s\n", i, sprnames[i]);
	}
#endif
	return result;
}

static int PatchAmmo(int ammoNum)
{
	extern int clipammo[NUMAMMO];

	int result;
	int* max;
	int* per;
	int dummy;

	if (ammoNum >= 0 && ammoNum < NUMAMMO)
	{
#if defined _DEBUG
		DPrintf("Ammo %d.\n", ammoNum);
#endif
		max = &maxammo[ammoNum];
		per = &clipammo[ammoNum];
	}
	else
	{
		DPrintf("Ammo %d out of range.\n", ammoNum);
		max = per = &dummy;
	}

	while ((result = GetLine()) == 1)
	{
		CHECKKEY("Max ammo", *max)
		else CHECKKEY("Per ammo", *per) else PrintUnknown(Line1, "Ammo", ammoNum);
	}

	return result;
}

static int PatchWeapon(int weapNum)
{
	static const struct Key keys[] = {
	    {"Ammo type", offsetof(weaponinfo_t, ammotype)},
	    {"Deselect frame", offsetof(weaponinfo_t, upstate)},
	    {"Select frame", offsetof(weaponinfo_t, downstate)},
	    {"Bobbing frame", offsetof(weaponinfo_t, readystate)},
	    {"Shooting frame", offsetof(weaponinfo_t, atkstate)},
	    {"Firing frame", offsetof(weaponinfo_t, flashstate)},
	    {"Ammo use", offsetof(weaponinfo_t, ammouse)},      // ZDoom 1.23b33
	    {"Ammo per shot", offsetof(weaponinfo_t, ammouse)}, // Eternity
	    {"Min ammo", offsetof(weaponinfo_t, minammo)},      // ZDoom 1.23b33
	    {NULL, 0}};

	static const struct
	{
		short Bit;
		const char* Name;
	} bitnames[] = {
	    {1, "NOTHRUST"},  {2, "SILENT"},          {4, "NOAUTOFIRE"},
	    {8, "FLEEMELEE"}, {16, "AUTOSWITCHFROM"}, {32, "NOAUTOSWITCHTO"},
	};

	int result;
	weaponinfo_t *info, dummy;

	if (weapNum >= 0 && weapNum < NUMWEAPONS)
	{
		info = &weaponinfo[weapNum];
#if defined _DEBUG
		DPrintf("Weapon %d\n", weapNum);
#endif
	}
	else
	{
		info = &dummy;
		DPrintf("Weapon %d out of range.\n", weapNum);
	}

	while ((result = GetLine()) == 1)
	{
		size_t val = atoi(Line2);
		size_t linelen = strlen(Line1);

		if (HandleKey(keys, info, Line1, val, sizeof(*info)))
		{
			if (linelen == 10)
			{
				if (stricmp(Line1, "MBF21 Bits") == 0)
				{
					int value = 0;
					bool vchanged = false;
					char* strval;

					for (strval = Line2; (strval = strtok(strval, ",+| \t\f\r"));
					     strval = NULL)
					{
						if (IsNum(strval))
						{
							// Force the top 4 bits to 0 so that the user is forced
							// to use the mnemonics to change them.

							// I have no idea why everyone insists on using strtol here
							// even though it fails dismally if a value is parsed where
							// the highest bit it set. Do people really use negative
							// values here? Let's better be safe and check both.
							value |= atoi(strval);
							vchanged = true;
						}
						else
						{
							size_t i;

							for (i = 0; i < ARRAY_LENGTH(bitnames); i++)
							{
								if (!stricmp(strval, bitnames[i].Name))
								{
									vchanged = true;
									value |= 1 << (bitnames[i].Bit);
									break;
								}
							}

							if (i == ARRAY_LENGTH(bitnames))
							{
								DPrintf("Unknown bit mnemonic %s\n", strval);
							}
						}
					}
					if (vchanged)
					{
						info->flags = value; // Weapon Flags
					}
				}
			}
			else
			{
				PrintUnknown(Line1, "Weapon", weapNum);
			}
		}
	}
	return result;
}

static int PatchPointer(int ptrNum)
{
	int result;

	if (ptrNum >= 0 && ptrNum < 448)
	{
#if defined _DEBUG
		DPrintf("Pointer %d\n", ptrNum);
#endif
	}
	else
	{
		DPrintf("Pointer %d out of range.\n", ptrNum);
		ptrNum = -1;
	}

	while ((result = GetLine()) == 1)
	{
		if ((ptrNum != -1) && (!stricmp(Line1, "Codep Frame")))
		{
			int i = atoi(Line2);

			// [CMB]: dsdhacked allows infinite code pointers
			if (i >= ::num_state_t_types)
			{
				DPrintf("Pointer %d overruns array (max: %d wanted: %d)."
				        "\n",
				        ptrNum, ::num_state_t_types, i);
			}
			else
			{
				states[codepconv[ptrNum]].action = OrgActionPtrs[i];
			}
		}
		else
		{
			PrintUnknown(Line1, "Pointer", ptrNum);
		}
	}
	return result;
}

static int PatchCheats(int dummy)
{
	int result;

	DPrintf("[DEHacked] Cheats support is depreciated. Ignoring these lines...\n");

	// Fake our work (don't do anything !)
	while ((result = GetLine()) == 1)
	{
	}

	return result;
}

static int PatchMisc(int dummy)
{
	static const struct Key keys[] = {
	    {"Initial Health", offsetof(struct DehInfo, StartHealth)},
	    {"Initial Bullets", offsetof(struct DehInfo, StartBullets)},
	    {"Max Health", offsetof(struct DehInfo, MaxHealth)},
	    {"Max Armor", offsetof(struct DehInfo, MaxArmor)},
	    {"Green Armor Class", offsetof(struct DehInfo, GreenAC)},
	    {"Blue Armor Class", offsetof(struct DehInfo, BlueAC)},
	    {"Max Soulsphere", offsetof(struct DehInfo, MaxSoulsphere)},
	    {"Soulsphere Health", offsetof(struct DehInfo, SoulsphereHealth)},
	    {"Megasphere Health", offsetof(struct DehInfo, MegasphereHealth)},
	    {"God Mode Health", offsetof(struct DehInfo, GodHealth)},
	    {"IDFA Armor", offsetof(struct DehInfo, FAArmor)},
	    {"IDFA Armor Class", offsetof(struct DehInfo, FAAC)},
	    {"IDKFA Armor", offsetof(struct DehInfo, KFAArmor)},
	    {"IDKFA Armor Class", offsetof(struct DehInfo, KFAAC)},
	    {"BFG Cells/Shot", offsetof(struct DehInfo, BFGCells)},
	    {"Monsters Infight", offsetof(struct DehInfo, Infight)},
	    {NULL, 0}};
	int result;
	gitem_t* item;
#if defined _DEBUG
	DPrintf("Misc\n");
#endif
	while ((result = GetLine()) == 1)
	{
		if (HandleKey(keys, &deh, Line1, atoi(Line2)))
		{
			DPrintf("Unknown miscellaneous info %s.\n", Line2);
		}

		// [SL] manually check if BFG Cells/Shot is being changed and
		// update weaponinfo accordingly. BFGCells should be considered depricated.
		if (Line1 != NULL && stricmp(Line1, "BFG Cells/Shot") == 0)
		{
			weaponinfo[wp_bfg].ammouse = deh.BFGCells;
			weaponinfo[wp_bfg].minammo = deh.BFGCells;
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

	return result;
}

static int PatchPars(int dummy)
{
	char *space, mapname[8], *moredata;
	int result, par;
#if defined _DEBUG
	DPrintf("[Pars]\n");
#endif
	while ((result = GetLine()))
	{
		// Argh! .bex doesn't follow the same rules as .deh
		if (result == 1)
		{
			DPrintf("Unknown key in [PARS] section: %s\n", Line1);
			continue;
		}
		if (stricmp("par", Line1))
		{
			return result;
		}

		space = strchr(Line2, ' ');

		if (!space)
		{
			DPrintf("Need data after par.\n");
			continue;
		}

		*space++ = '\0';

		while (*space && isspace(*space))
		{
			space++;
		}

		moredata = strchr(space, ' ');

		if (moredata)
		{
			// At least 3 items on this line, must be E?M? format
			sprintf(mapname, "E%cM%c", *Line2, *space);
			par = atoi(moredata + 1);
		}
		else
		{
			// Only 2 items, must be MAP?? format
			sprintf(mapname, "MAP%02d", atoi(Line2) % 100);
			par = atoi(space);
		}

		LevelInfos& levels = getLevelInfos();
		level_pwad_info_t& info = levels.findByName(mapname);

		if (!info.exists())
		{
			DPrintf("No map %s\n", mapname);
			continue;
		}

		info.partime = par;
#if defined _DEBUG
		DPrintf("Par for %s changed to %d\n", mapname, par);
#endif
	}
	return result;
}

static int PatchCodePtrs(int dummy)
{
	int result;
#if defined _DEBUG
	DPrintf("[CodePtr]\n");
#endif
	while ((result = GetLine()) == 1)
	{
		if (!strnicmp("Frame", Line1, 5) && isspace(Line1[5]))
		{
			int frame = atoi(Line1 + 5);

			if (frame < 0 || frame >= num_state_t_types)
			{
				// [CMB] TODO: at this point we should have created more space for the state
				DPrintf("Frame %d out of range\n", frame);
			}
			else
			{
				int i = 0;
				char* data;

				COM_Parse(Line2);

				if ((com_token[0] == 'A' || com_token[0] == 'a') && com_token[1] == '_')
				{
					data = com_token + 2;
				}
				else
				{
					data = com_token;
				}

				while (CodePtrs[i].name && stricmp(CodePtrs[i].name, data))
				{
					i++;
				}

				if (CodePtrs[i].name)
				{
					states[frame].action = CodePtrs[i].func;
					DPrintf("Frame %d set to %s\n", frame, CodePtrs[i].name);
				}
				else
				{
					states[frame].action = NULL;
					DPrintf("Unknown code pointer: %s\n", com_token);
				}
			}
		}
	}
	return result;
}

static int PatchMusic(int dummy)
{
	int result;
	char keystring[128];
#if defined _DEBUG
	DPrintf("[Music]\n");
#endif
	while ((result = GetLine()) == 1)
	{
		const char* newname = skipwhite(Line2);

		snprintf(keystring, ARRAY_LENGTH(keystring), "MUSIC_%s", Line1);
		if (GStrings.hasString(keystring))
		{
			GStrings.setString(keystring, newname);
			DPrintf("Music %s set to:\n%s\n", keystring, newname);
		}
	}

	return result;
}

static int PatchText(int oldSize)
{
	LevelInfos& levels = getLevelInfos();

	int newSize;
	char* oldStr;
	char* newStr;
	char* temp;
	BOOL good;
	int result;
	int i;
	const OString* name = NULL;

	// Skip old size, since we already know it
	temp = Line2;
	while (*temp > ' ')
	{
		temp++;
	}
	while (*temp && *temp <= ' ')
	{
		temp++;
	}

	if (*temp == 0)
	{
		Printf(PRINT_HIGH, "Text chunk is missing size of new string.\n");
		return 2;
	}
	newSize = atoi(temp);

	oldStr = new char[oldSize + 1];
	newStr = new char[newSize + 1];

	if (!oldStr || !newStr)
	{
		Printf(PRINT_HIGH, "Out of memory.\n");
		goto donewithtext;
	}

	good = ReadChars(&oldStr, oldSize);
	good += ReadChars(&newStr, newSize);

	if (!good)
	{
		delete[] newStr;
		delete[] oldStr;
		Printf(PRINT_HIGH, "Unexpected end-of-file.\n");
		return 0;
	}

	if (includenotext)
	{
		Printf(PRINT_HIGH, "Skipping text chunk in included patch.\n");
		goto donewithtext;
	}

	DPrintf("Searching for text:\n%s\n", oldStr);
	good = false;

	// Search through sprite names
	for (i = 0; i < ::num_spritenum_t_types; i++)
	{
		if (!strcmp(sprnames[i], oldStr))
		{
			sprnames[i] = copystring(newStr);
			good = true;
			// See above.
		}
	}

	if (good)
	{
		goto donewithtext;
	}

	// Search through music names.
	// [AM] Disabled because it relies on an extern wadlevelinfos
	/*if (oldSize < 7)
	{		// Music names are never >6 chars
	    char musname[9];
	    snprintf(musname, ARRAY_LENGTH(musname), "D_%s", oldStr);

	    for (size_t i = 0; i < levels.size(); i++)
	    {
	        level_pwad_info_t& level = levels.at(0);
	        if (stricmp(level.music, musname) == 0)
	        {
	            good = true;
	            uppercopy(level.music, musname);
	        }
	    }
	}*/

	if (good)
	{
		goto donewithtext;
	}

	// Search through most other texts
	name = &ENGStrings.matchString(oldStr);
	if (name != NULL && !name->empty())
	{
		GStrings.setString(*name, newStr);
		good = true;
	}

	if (!good)
	{
		DPrintf("   (Unmatched)\n");
	}

donewithtext:
	if (newStr)
	{
		delete[] newStr;
	}
	if (oldStr)
	{
		delete[] oldStr;
	}

	// Ensure that we've munched the entire line in the case of an incomplete
	// substitution.
	if (!(*PatchPt == '\0' || *PatchPt == '\n'))
	{
		igets();
	}

	// Fetch next identifier for main loop
	while ((result = GetLine()) == 1)
	{
	}

	return result;
}

static int PatchStrings(int dummy)
{
	static size_t maxstrlen = 128;
	static char* holdstring;
	int result;
#if defined _DEBUG
	DPrintf("[Strings]\n");
#endif
	if (!holdstring)
	{
		holdstring = (char*)Malloc(maxstrlen);
	}

	while ((result = GetLine()) == 1)
	{
		int i;

		*holdstring = '\0';
		do
		{
			while (maxstrlen < strlen(holdstring) + strlen(Line2) + 8)
			{
				maxstrlen += 128;
				holdstring = (char*)Realloc(holdstring, maxstrlen);
			}
			strcat(holdstring, skipwhite(Line2));
			stripwhite(holdstring);
			if (holdstring[strlen(holdstring) - 1] == '\\')
			{
				holdstring[strlen(holdstring) - 1] = '\0';
				Line2 = igets();
			}
			else
			{
				Line2 = NULL;
			}
		} while (Line2 && *Line2);

		i = GStrings.toIndex(Line1);
		if (i == -1)
		{
			Printf(PRINT_HIGH, "Unknown string: %s\n", Line1);
		}
		else
		{

			ReplaceSpecialChars(holdstring);
			if ((i >= GStrings.toIndex(OB_SUICIDE) && i <= GStrings.toIndex(OB_DEFAULT) &&
			     strstr(holdstring, "%o") == NULL) ||
			    (i >= GStrings.toIndex(OB_FRIENDLY1) &&
			     i <= GStrings.toIndex(OB_FRIENDLY4) && strstr(holdstring, "%k") == NULL))
			{
				int len = strlen(holdstring);
				memmove(holdstring + 3, holdstring, len);
				holdstring[0] = '%';
				holdstring[1] = i <= GStrings.toIndex(OB_DEFAULT) ? 'o' : 'k';
				holdstring[2] = ' ';
				holdstring[3 + len] = '.';
				holdstring[4 + len] = 0;
				if (i >= GStrings.toIndex(OB_MPFIST) && i <= GStrings.toIndex(OB_RAILGUN))
				{
					char* spot = strstr(holdstring, "%s");
					if (spot != NULL)
					{
						spot[1] = 'k';
					}
				}
			}
			// [CMB] TODO: Language string table change
			GStrings.setString(Line1, holdstring);
			DPrintf("%s set to:\n%s\n", Line1, holdstring);
		}
	}

	return result;
}

static int DoInclude(int dummy)
{
	char* data;
	int savedversion, savepversion;
	char *savepatchfile, *savepatchpt;
	OWantFile want;
	OResFile res;

	if (including)
	{
		DPrintf("Sorry, can't nest includes\n");
		goto endinclude;
	}

	data = COM_Parse(Line2);
	if (!stricmp(com_token, "notext"))
	{
		includenotext = true;
		data = COM_Parse(data);
	}

	if (!com_token[0])
	{
		includenotext = false;
		DPrintf("Include directive is missing filename\n");
		goto endinclude;
	}
#if defined _DEBUG
	DPrintf("Including %s\n", com_token);
#endif
	savepatchfile = PatchFile;
	savepatchpt = PatchPt;
	savedversion = dversion;
	savepversion = pversion;
	including = true;

	if (!OWantFile::make(want, com_token, OFILE_DEH))
	{
		Printf(PRINT_WARNING, "Could not find BEX include \"%s\"\n", com_token);
		goto endinclude;
	}

	if (!M_ResolveWantedFile(res, want))
	{
		Printf(PRINT_WARNING, "Could not resolve BEX include \"%s\"\n", com_token);
		goto endinclude;
	}

	D_DoDehPatch(&res, -1);

	DPrintf("Done with include\n");
	PatchFile = savepatchfile;
	PatchPt = savepatchpt;
	dversion = savedversion;
	pversion = savepversion;

endinclude:
	including = false;
	includenotext = false;
	return GetLine();
}

/**
 * @brief Attempt to load a DeHackEd file.
 *
 * @param patchfile File to attempt to load, NULL if not a file.
 * @param lump Lump index to load, -1 if not a lump.
 */
bool D_DoDehPatch(const OResFile* patchfile, const int lump)
{
	BackupData();
	::PatchFile = NULL;

	if (lump >= 0)
	{
		// Execute the DEHACKED lump as a patch.
		::filelen = W_LumpLength(lump);
		::PatchFile = new char[::filelen + 1];
		W_ReadLump(lump, ::PatchFile);
	}
	else if (patchfile)
	{
		// Try to use patchfile as a patch.
		FILE* fh = fopen(patchfile->getFullpath().c_str(), "rb+");
		if (fh == NULL)
		{
			Printf(PRINT_WARNING, "Could not open DeHackEd patch \"%s\"\n",
			       patchfile->getBasename().c_str());
			return false;
		}

		::filelen = M_FileLength(fh);
		::PatchFile = new char[::filelen + 1];

		size_t read = fread(::PatchFile, 1, filelen, fh);
		if (read < filelen)
		{
			DPrintf("Could not read file\n");
			return false;
		}
	}
	else
	{
		// Nothing to do.
		return false;
	}

	// Load english strings to match against.
	::ENGStrings.loadStrings(true);

	// End file with a NULL for our parser
	::PatchFile[::filelen] = 0;

	::dversion = pversion = -1;

	int cont = 0;
	if (!strncmp(::PatchFile, "Patch File for DeHackEd v", 25))
	{
		::PatchPt = strchr(::PatchFile, '\n');
		while ((cont = GetLine()) == 1)
		{
			CHECKKEY("Doom version", ::dversion)
			else CHECKKEY("Patch format", ::pversion)
		}
		if (!cont || ::dversion == -1 || ::pversion == -1)
		{
			delete[] ::PatchFile;
			if (patchfile)
			{
				Printf(PRINT_WARNING, "\"%s\" is not a DeHackEd patch file\n",
				       patchfile->getBasename().c_str());
			}
			else
			{
				Printf(PRINT_WARNING, "\"DEHACKED\" is not a DeHackEd patch lump\n");
			}
			return false;
		}
	}
	else
	{
		DPrintf("Patch does not have DeHackEd signature. Assuming .bex\n");
		::dversion = 19;
		::pversion = 6;
		::PatchPt = ::PatchFile;
		while ((cont = GetLine()) == 1)
		{
		}
	}

	if (::pversion != 6)
	{
		DPrintf("DeHackEd patch version is %d.\nUnexpected results may occur.\n",
		        ::pversion);
	}

	if (::dversion == 16)
	{
		::dversion = 0;
	}
	else if (::dversion == 17)
	{
		::dversion = 2;
	}
	else if (::dversion == 19)
	{
		::dversion = 3;
	}
	else if (::dversion == 20)
	{
		::dversion = 1;
	}
	else if (::dversion == 21)
	{
		::dversion = 4;
	}
	else if (::dversion == 2021)
	{
		// [CMB] TODO: handle 'Doom version = 2021'
		// [CMB] TODO: this version is used to calculate offsets for sprite limits
		// [CMB] TODO: dsdhacked has "unlimited" so for now we'll use 4, but we may need to use something else
		::dversion = 4;
	}
	else
	{
		DPrintf("Patch created with unknown DOOM version.\nAssuming version 1.9.\n");
		::dversion = 3;
	}

	do
	{
		if (cont == 1)
		{
			DPrintf("Key %s encountered out of context\n", ::Line1);
			cont = 0;
		}
		else if (cont == 2)
		{
			cont = HandleMode(::Line1, atoi(::Line2));
		}
	} while (cont);

	delete[] ::PatchFile;
	::PatchFile = NULL;

	if (patchfile)
	{
		Printf("adding %s\n", patchfile->getFullpath().c_str());
	}
	else
	{
		Printf("adding DEHACKED lump\n");
	}
	Printf(" (DeHackEd patch)\n");

	D_PostProcessDeh();

	return true;
}

static CodePtr null_bexptr = {"(NULL)", NULL};

/*
 * @brief Check loaded deh files for any problems prior
 * to launching the game.
 *
 * (Credit to DSDADoom for the inspiration for this)
 */

void D_PostProcessDeh()
{
	int i, j;
	const CodePtr* bexptr_match;

	for (i = 0; i < ::num_state_t_types; i++)
	{
		bexptr_match = &null_bexptr;

		for (j = 1; CodePtrs[j].func != NULL; ++j)
		{
			if (states[i].action == CodePtrs[j].func)
			{
				bexptr_match = &CodePtrs[j];
				break;
			}
		}

		// ensure states don't use more mbf21 args than their
		// action pointer expects, for future-proofing's sake
		for (j = MAXSTATEARGS - 1; j >= bexptr_match->argcount; j--)
		{
			if (states[i].args[j] != 0)
			{
				I_Error("Action %s on state %d expects no more than %d nonzero args (%d "
				        "found). Check your DEHACKED.",
				        bexptr_match->name, i, bexptr_match->argcount, j + 1);
			}
		}

		// replace unset fields with default values
		for (; j >= 0; j--)
		{
			if (states[i].args[j] == 0 && bexptr_match->default_args[j])
			{
				states[i].args[j] = bexptr_match->default_args[j];
			}
		}
	}
}

/*
* @brief Checks to see if TNT-range actor is defined, but useful for DEHEXTRA monsters.
* Because in HORDEDEF, sometimes a WAD author may accidentally use a DEHEXTRA monster
* that is undefined.
*/
bool CheckIfDehActorDefined(const mobjtype_t mobjtype)
{
	const mobjinfo_t mobj = ::mobjinfo[mobjtype];
	if (mobj.doomednum == -1 &&
		mobj.spawnstate == S_TNT1 &&
		mobj.spawnhealth == 0 &&
		mobj.gibhealth == 0 && 
		mobj.seestate == S_NULL &&
		mobj.seesound == NULL &&
	    mobj.reactiontime == 0 &&
		mobj.attacksound == NULL &&
		mobj.painstate == S_NULL &&
	    mobj.painchance == 0 &&
		mobj.painsound == NULL &&
		mobj.meleestate == S_NULL &&
	    mobj.missilestate == S_NULL &&
		mobj.deathstate == S_NULL &&
	    mobj.xdeathstate == S_NULL &&
		mobj.deathsound == NULL &&
		mobj.speed == 0 &&
	    mobj.radius == 0 &&
		mobj.height == 0 &&
		mobj.cdheight == 0 &&
	    mobj.mass == 0 &&
	    mobj.damage == 0 &&
		mobj.activesound == NULL &&
		mobj.flags == 0 &&
	    mobj.flags2 == 0 &&
		mobj.raisestate == S_NULL &&
		mobj.translucency == 0x10000 &&
	    mobj.altspeed == NO_ALTSPEED &&
		mobj.infighting_group == IG_DEFAULT &&
		mobj.projectile_group == PG_DEFAULT &&
		mobj.splash_group == SG_DEFAULT &&
		mobj.ripsound == "" &&
		mobj.meleerange == MELEERANGE &&
		mobj.droppeditem == MT_NULL)
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
	if (index < 0 || index >= ::num_state_t_types)
	{
		return;
	}

	// Print this state.
	state_t& state = ::states[index];
	Printf("%4d | s:%s f:%d t:%d a:%s m1:%d m2:%d\n", index, ::sprnames[state.sprite],
	       state.frame, state.tics, ActionPtrString(state.action), state.misc1,
	       state.misc2);
}

BEGIN_COMMAND(stateinfo)
{
	if (argc < 2)
	{
		Printf("Must pass one or two state indexes. (0 to %d)\n", ::num_state_t_types - 1);
		return;
	}

	int index1 = atoi(argv[1]);
	if (index1 < 0 || index1 >= ::num_state_t_types)
	{
		Printf("Not a valid index.\n");
		return;
	}
	int index2 = index1;

	if (argc == 3)
	{
		index2 = atoi(argv[2]);
		if (index2 < 0 || index2 >= ::num_state_t_types)
		{
			Printf("Not a valid index.\n");
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
		Printf("Must pass state index. (0 to %d)\n", ::num_state_t_types - 1);
		return;
	}

	int index = atoi(argv[1]);
	if (index < 0 || index >= ::num_state_t_types)
	{
		Printf("Not a valid index.\n");
		return;
	}

	OHashTable<int, bool> visited;
	for (;;)
	{
		// Check if we looped back, and exit if so.
		OHashTable<int, bool>::iterator it = visited.find(index);
		if (it != visited.end())
		{
			Printf("Looped back to %d\n", index);
			return;
		}

		PrintState(index);

		// Mark as visited.
		visited.insert(std::pair<int, bool>(index, true));

		// Next state.
		index = ::states[index].nextstate;
	}
}
END_COMMAND(playstate)

VERSION_CONTROL(d_dehacked_cpp, "$Id$")
