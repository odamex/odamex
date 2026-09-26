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
//	info.cpp for id24 stuff
//
//-----------------------------------------------------------------------------

#include "odamex.h"
#include "m_fixed.h"
#include "actor.h"
#include "info.h"

#include "id24info.h"

// code pointers
// TODO: we should really have a single header that's just these
// instead of redeclaring them in each info file and d_dehacked
void A_BossDeath(AActor*);
void A_Chase(AActor*);
void A_CyberAttack(AActor*);
void A_FaceTarget(AActor*);
void A_Fall(AActor*);
void A_Hoof(AActor*);
void A_Look(AActor*);
void A_MonsterProjectile(AActor*);
void A_Pain(AActor*);
void A_PlaySound(AActor*);
void A_RadiusDamage(AActor*);
void A_RandomJump(AActor*);
void A_RemoveFlags(AActor*);
void A_Scream(AActor*);
void A_SpidRefire(AActor*);
void A_SpawnObject(AActor*);
void A_SPosAttack(AActor*);
void A_XScream(AActor*);

namespace
{

constexpr std::array id24sprnames = std::to_array({
	"BSH1",
	"BSH2",
	"BSHE",
	"CBR2",
	"CHR1",
	"CSPI",
	"CYB2",
	"FLMF",
	"FLMG",
	"GBAL",
	"GHUL",
	"GOR6",
	"GOR7",
	"GOR8",
	"GORA",
	"HBB2",
	"HBBQ",
	"HDB7",
	"HDB8",
	"HETB",
	"HETC",
	"HETF",
	"HETG",
	"IFLM",
	"LAMP",
	"PHED",
	"POB6",
	"POL7",
	"POLA",
	"PPOS",
	"STC1",
	"STC2",
	"STC3",
	"STG1",
	"STG2",
	"STG3",
	"STMI",
	"TLP6",
	"VASS",
	"VFLM",
	"INCN",
	"CBLD",
	"FCPU",
	"FTNK",
});

constexpr std::array id24states = std::to_array<state_t>({
	// --------------------- decorations ---------------------
	// pile of bodies
	{ .statenum = S_ID24_1,   .sprite = SPR_POL7 },
	// human barbecue 1
	{ .statenum = S_ID24_2,   .sprite = SPR_HBBQ, .frame = 0_bright, .tics = 5, .nextstate = S_ID24_3 },
	{ .statenum = S_ID24_3,   .sprite = SPR_HBBQ, .frame = 1_bright, .tics = 5, .nextstate = S_ID24_4 },
	{ .statenum = S_ID24_4,   .sprite = SPR_HBBQ, .frame = 2_bright, .tics = 5, .nextstate = S_ID24_2 },
	// human barbecue 2
	{ .statenum = S_ID24_5,   .sprite = SPR_HBB2, .frame = 0_bright, .tics = 5, .nextstate = S_ID24_6 },
	{ .statenum = S_ID24_6,   .sprite = SPR_HBB2, .frame = 1_bright, .tics = 5, .nextstate = S_ID24_7 },
	{ .statenum = S_ID24_7,   .sprite = SPR_HBB2, .frame = 2_bright, .tics = 5, .nextstate = S_ID24_5 },
	// hanging bodies
	{ .statenum = S_ID24_8,   .sprite = SPR_GOR6 },
	{ .statenum = S_ID24_9,   .sprite = SPR_GOR7 },
	{ .statenum = S_ID24_10,  .sprite = SPR_GOR8 },
	{ .statenum = S_ID24_11,  .sprite = SPR_GORA },
	{ .statenum = S_ID24_12,  .sprite = SPR_HDB7 },
	{ .statenum = S_ID24_13,  .sprite = SPR_HDB8 },
	// skewered heads
	{ .statenum = S_ID24_14,  .sprite = SPR_POLA },
	// viscera puddle
	{ .statenum = S_ID24_15,  .sprite = SPR_POB6 },
	// column
	{ .statenum = S_ID24_16,  .sprite = SPR_COLU },
	// bush 1
	{ .statenum = S_ID24_17,  .sprite = SPR_BSH1, .frame = 0 },
	{ .statenum = S_ID24_18,  .sprite = SPR_BSH1, .frame = 1 },
	{ .statenum = S_ID24_19,  .sprite = SPR_BSH1, .frame = 2 },
	// bush 2
	{ .statenum = S_ID24_20,  .sprite = SPR_BSH2, .frame = 0 },
	{ .statenum = S_ID24_21,  .sprite = SPR_BSH2, .frame = 1 },
	{ .statenum = S_ID24_22,  .sprite = SPR_BSH2, .frame = 2 },
	// rock column
	{ .statenum = S_ID24_23,  .sprite = SPR_STMI },
	// stalagmite
	{ .statenum = S_ID24_24,  .sprite = SPR_STG1 },
	{ .statenum = S_ID24_25,  .sprite = SPR_STG2 },
	{ .statenum = S_ID24_26,  .sprite = SPR_STG3 },
	// stalactite
	{ .statenum = S_ID24_27,  .sprite = SPR_STC1 },
	{ .statenum = S_ID24_28,  .sprite = SPR_STC2 },
	{ .statenum = S_ID24_29,  .sprite = SPR_STC3 },
	// chair
	{ .statenum = S_ID24_30,  .sprite = SPR_CHR1 },
	// lamp
	{ .statenum = S_ID24_31,  .sprite = SPR_LAMP, .frame = 0_bright },
	{ .statenum = S_ID24_32,  .sprite = SPR_LAMP, .frame = 1_bright, .tics = 3, .action = A_Scream, .nextstate = S_ID24_33 },
	{ .statenum = S_ID24_33,  .sprite = SPR_LAMP, .frame = 1_bright, .tics = 3, .action = A_Fall,   .nextstate = S_ID24_34 },
	{ .statenum = S_ID24_34,  .sprite = SPR_LAMP, .frame = 2_bright, .tics = 5,                     .nextstate = S_ID24_35 },
	{ .statenum = S_ID24_35,  .sprite = SPR_LAMP, .frame = 3_bright, .tics = 5,                     .nextstate = S_ID24_34 },
	// ceiling lamp
	{ .statenum = S_ID24_36,  .sprite = SPR_TLP6, .frame = 0_bright },
	// candelabra
	{ .statenum = S_ID24_37,  .sprite = SPR_CBR2, .frame = 0_bright },
	// ambient sounds
	{ .statenum = S_ID24_38,  .tics =   1, .action = A_Look,      .nextstate = S_ID24_38 },
	{ .statenum = S_ID24_39,  .tics =  35, .action = A_PlaySound, .nextstate = S_ID24_39, .misc1 = -1879048168 /* ambient/klaxon      */, .misc2 = 0 },
	{ .statenum = S_ID24_40,  .tics = 250, .action = A_PlaySound,                         .misc1 = -1879048182 /* ambient/portalopen  */, .misc2 = 1 },
	{ .statenum = S_ID24_41,  .tics = 139, .action = A_PlaySound, .nextstate = S_ID24_41, .misc1 = -1879048183 /* ambient/portalloop  */, .misc2 = 0 },
	{ .statenum = S_ID24_42,  .tics = 105, .action = A_PlaySound,                         .misc1 = -1879048184 /* ambient/portalclose */, .misc2 = 1 },
	// ---------------------- monsters -----------------------
	// ghoul
	{ .statenum = S_ID24_43,  .sprite = SPR_GHUL, .frame =  0,        .tics = 10, .action = A_Look,              .nextstate = S_ID24_44 },
	{ .statenum = S_ID24_44,  .sprite = SPR_GHUL, .frame =  1,        .tics = 10, .action = A_Look,              .nextstate = S_ID24_43 },
	{ .statenum = S_ID24_45,  .sprite = SPR_GHUL, .frame =  0,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_46 },
	{ .statenum = S_ID24_46,  .sprite = SPR_GHUL, .frame =  0,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_47 },
	{ .statenum = S_ID24_47,  .sprite = SPR_GHUL, .frame =  1,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_48 },
	{ .statenum = S_ID24_48,  .sprite = SPR_GHUL, .frame =  1,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_49 },
	{ .statenum = S_ID24_49,  .sprite = SPR_GHUL, .frame =  2,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_50 },
	{ .statenum = S_ID24_50,  .sprite = SPR_GHUL, .frame =  2,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_51 },
	{ .statenum = S_ID24_51,  .sprite = SPR_GHUL, .frame =  1,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_52 },
	{ .statenum = S_ID24_52,  .sprite = SPR_GHUL, .frame =  1,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_45 },
	{ .statenum = S_ID24_53,  .sprite = SPR_GHUL, .frame =  3_bright, .tics =  4, .action = A_FaceTarget,        .nextstate = S_ID24_54 },
	{ .statenum = S_ID24_54,  .sprite = SPR_GHUL, .frame =  4_bright, .tics =  4, .action = A_FaceTarget,        .nextstate = S_ID24_55 },
	{ .statenum = S_ID24_55,  .sprite = SPR_GHUL, .frame =  5_bright, .tics =  4, .action = A_MonsterProjectile, .nextstate = S_ID24_56, .args = { MT_GHOUL_BALL + 1, 0, 0, 0, -8_fx, 0, 0, 0 } },
	{ .statenum = S_ID24_56,  .sprite = SPR_GHUL, .frame =  6_bright, .tics =  4,                                .nextstate = S_ID24_45 },
	{ .statenum = S_ID24_57,  .sprite = SPR_GHUL, .frame =  8_bright, .tics =  3,                                .nextstate = S_ID24_58 },
	{ .statenum = S_ID24_58,  .sprite = SPR_GHUL, .frame = 10_bright, .tics =  3, .action = A_Pain,              .nextstate = S_ID24_45 },
	{ .statenum = S_ID24_59,  .sprite = SPR_GHUL, .frame = 11_bright, .tics =  5,                                .nextstate = S_ID24_60 },
	{ .statenum = S_ID24_60,  .sprite = SPR_GHUL, .frame = 12_bright, .tics =  5, .action = A_Scream,            .nextstate = S_ID24_61 },
	{ .statenum = S_ID24_61,  .sprite = SPR_GHUL, .frame = 13_bright, .tics =  5,                                .nextstate = S_ID24_62 },
	{ .statenum = S_ID24_62,  .sprite = SPR_GHUL, .frame = 14_bright, .tics =  5,                                .nextstate = S_ID24_63 },
	{ .statenum = S_ID24_63,  .sprite = SPR_GHUL, .frame = 15_bright, .tics =  5, .action = A_Fall,              .nextstate = S_ID24_64 },
	{ .statenum = S_ID24_64,  .sprite = SPR_GHUL, .frame = 16_bright, .tics =  5,                                .nextstate = S_ID24_65 },
	{ .statenum = S_ID24_65,  .sprite = SPR_GHUL, .frame = 17_bright, .tics =  5,                                .nextstate = S_ID24_66 },
	{ .statenum = S_ID24_66,  .sprite = SPR_GHUL, .frame = 18,        .tics = -1 },
	// ghoul projectile
	{ .statenum = S_ID24_67,  .sprite = SPR_GBAL, .frame = 0_bright, .tics = 4, .nextstate = S_ID24_68 },
	{ .statenum = S_ID24_68,  .sprite = SPR_GBAL, .frame = 1_bright, .tics = 4, .nextstate = S_ID24_67 },
	{ .statenum = S_ID24_69,  .sprite = SPR_GBAL, .frame = 2_bright, .tics = 5, .nextstate = S_ID24_70 },
	{ .statenum = S_ID24_70,  .sprite = SPR_APBX, .frame = 1_bright, .tics = 5, .nextstate = S_ID24_71 },
	{ .statenum = S_ID24_71,  .sprite = SPR_APBX, .frame = 2_bright, .tics = 5, .nextstate = S_ID24_72 },
	{ .statenum = S_ID24_72,  .sprite = SPR_APBX, .frame = 3_bright, .tics = 5, .nextstate = S_ID24_73 },
	{ .statenum = S_ID24_73,  .sprite = SPR_APBX, .frame = 4_bright, .tics = 5 },
	// banshee
	{ .statenum = S_ID24_74,  .sprite = SPR_BSHE, .frame = 0_bright, .tics = 10, .action = A_Look,         .nextstate = S_ID24_75 },
	{ .statenum = S_ID24_75,  .sprite = SPR_BSHE, .frame = 1_bright, .tics = 10, .action = A_Look,         .nextstate = S_ID24_74 },
	{ .statenum = S_ID24_76,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_77 },
	{ .statenum = S_ID24_77,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_78 },
	{ .statenum = S_ID24_78,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_79 },
	{ .statenum = S_ID24_79,  .sprite = SPR_BSHE, .frame = 1_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_80 },
	{ .statenum = S_ID24_80,  .sprite = SPR_BSHE, .frame = 1_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_81 },
	{ .statenum = S_ID24_81,  .sprite = SPR_BSHE, .frame = 1_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_82 },
	{ .statenum = S_ID24_82,  .sprite = SPR_BSHE, .frame = 2_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_83 },
	{ .statenum = S_ID24_83,  .sprite = SPR_BSHE, .frame = 2_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_84 },
	{ .statenum = S_ID24_84,  .sprite = SPR_BSHE, .frame = 2_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_85 },
	{ .statenum = S_ID24_85,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_86 },
	{ .statenum = S_ID24_86,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_87 },
	{ .statenum = S_ID24_87,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_88 },
	{ .statenum = S_ID24_88,  .sprite = SPR_BSHE, .frame = 1_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_89 },
	{ .statenum = S_ID24_89,  .sprite = SPR_BSHE, .frame = 1_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_90 },
	{ .statenum = S_ID24_90,  .sprite = SPR_BSHE, .frame = 1_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_91 },
	{ .statenum = S_ID24_91,  .sprite = SPR_BSHE, .frame = 2_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_92 },
	{ .statenum = S_ID24_92,  .sprite = SPR_BSHE, .frame = 2_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_93 },
	{ .statenum = S_ID24_93,  .sprite = SPR_BSHE, .frame = 2_bright, .tics =  2, .action = A_Chase,        .nextstate = S_ID24_94 },
	{ .statenum = S_ID24_94,  .sprite = SPR_BSHE, .frame = 0_bright, .tics =  0, .action = A_PlaySound,    .nextstate = S_ID24_76, .misc1 = -1879048192 /* monsters/banshee/active */, .misc2 = 0 },
	{ .statenum = S_ID24_95,  .sprite = SPR_BSHE, .frame = 3_bright, .tics =  1, .action = A_RadiusDamage, .nextstate = S_ID24_95, .args = { 100, 8, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_96,  .sprite = SPR_BSHE, .frame = 3_bright, .tics =  3,                           .nextstate = S_ID24_97 },
	{ .statenum = S_ID24_97,  .sprite = SPR_BSHE, .frame = 3_bright, .tics =  3, .action = A_Pain,         .nextstate = S_ID24_76 },
	{ .statenum = S_ID24_98,  .sprite = SPR_BSHE, .frame = 3_bright, .tics =  4, .action = A_Scream,       .nextstate = S_ID24_99 },
	{ .statenum = S_ID24_99,  .sprite = SPR_BSHE, .frame = 4_bright, .tics =  6, .action = A_RadiusDamage, .nextstate = S_ID24_100, .args = { 128, 128, 0, 0, 0, 0, 0 , 0 } },
	{ .statenum = S_ID24_100, .sprite = SPR_BSHE, .frame = 5_bright, .tics =  8, .action = A_Fall,         .nextstate = S_ID24_101 },
	{ .statenum = S_ID24_101, .sprite = SPR_BSHE, .frame = 6_bright, .tics =  6,                           .nextstate = S_ID24_102 },
	{ .statenum = S_ID24_102, .sprite = SPR_BSHE, .frame = 7_bright, .tics =  4,                           .nextstate = S_ID24_103 },
	{ .statenum = S_ID24_103,                                        .tics = 20 },
	// mindweaver
	{ .statenum = S_ID24_104, .sprite = SPR_CSPI, .frame =  0,        .tics = 10, .action = A_Look,       .nextstate = S_ID24_105 },
	{ .statenum = S_ID24_105, .sprite = SPR_CSPI, .frame =  1,        .tics = 10, .action = A_Look,       .nextstate = S_ID24_104 },
	{ .statenum = S_ID24_106, .sprite = SPR_CSPI, .frame =  0,        .tics = 20,                         .nextstate = S_ID24_107 },
	{ .statenum = S_ID24_107, .sprite = SPR_CSPI, .frame =  0,        .tics =  0, .action = A_PlaySound,  .nextstate = S_ID24_108, .misc1 = -1879048185 /* monsters/mindweaver/walk */, .misc2 = 0 },
	{ .statenum = S_ID24_108, .sprite = SPR_CSPI, .frame =  0,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_109 },
	{ .statenum = S_ID24_109, .sprite = SPR_CSPI, .frame =  0,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_110 },
	{ .statenum = S_ID24_110, .sprite = SPR_CSPI, .frame =  1,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_111 },
	{ .statenum = S_ID24_111, .sprite = SPR_CSPI, .frame =  1,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_112 },
	{ .statenum = S_ID24_112, .sprite = SPR_CSPI, .frame =  2,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_113 },
	{ .statenum = S_ID24_113, .sprite = SPR_CSPI, .frame =  2,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_114 },
	{ .statenum = S_ID24_114, .sprite = SPR_CSPI, .frame =  3,        .tics =  0, .action = A_PlaySound,  .nextstate = S_ID24_115, .misc1 = -1879048185 /* monsters/mindweaver/walk */, .misc2 = 0 },
	{ .statenum = S_ID24_115, .sprite = SPR_CSPI, .frame =  3,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_116 },
	{ .statenum = S_ID24_116, .sprite = SPR_CSPI, .frame =  3,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_117 },
	{ .statenum = S_ID24_117, .sprite = SPR_CSPI, .frame =  4,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_118 },
	{ .statenum = S_ID24_118, .sprite = SPR_CSPI, .frame =  4,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_119 },
	{ .statenum = S_ID24_119, .sprite = SPR_CSPI, .frame =  5,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_120 },
	{ .statenum = S_ID24_120, .sprite = SPR_CSPI, .frame =  5,        .tics =  3, .action = A_Chase,      .nextstate = S_ID24_107 },
	{ .statenum = S_ID24_121, .sprite = SPR_CSPI, .frame =  0_bright, .tics = 20, .action = A_FaceTarget, .nextstate = S_ID24_122 },
	{ .statenum = S_ID24_122, .sprite = SPR_CSPI, .frame =  6_bright, .tics =  4, .action = A_SPosAttack, .nextstate = S_ID24_123 },
	{ .statenum = S_ID24_123, .sprite = SPR_CSPI, .frame =  7_bright, .tics =  4, .action = A_SPosAttack, .nextstate = S_ID24_124 },
	{ .statenum = S_ID24_124, .sprite = SPR_CSPI, .frame =  7_bright, .tics =  1, .action = A_SpidRefire, .nextstate = S_ID24_122 },
	{ .statenum = S_ID24_125, .sprite = SPR_CSPI, .frame =  8,        .tics =  3,                         .nextstate = S_ID24_126 },
	{ .statenum = S_ID24_126, .sprite = SPR_CSPI, .frame =  8,        .tics =  3, .action = A_Pain,       .nextstate = S_ID24_107 },
	{ .statenum = S_ID24_127, .sprite = SPR_CSPI, .frame =  9,        .tics = 20, .action = A_Scream,     .nextstate = S_ID24_128 },
	{ .statenum = S_ID24_128, .sprite = SPR_CSPI, .frame = 10,        .tics =  7, .action = A_Fall,       .nextstate = S_ID24_129 },
	{ .statenum = S_ID24_129, .sprite = SPR_CSPI, .frame = 11,        .tics =  7,                         .nextstate = S_ID24_130 },
	{ .statenum = S_ID24_130, .sprite = SPR_CSPI, .frame = 12,        .tics =  7,                         .nextstate = S_ID24_131 },
	{ .statenum = S_ID24_131, .sprite = SPR_CSPI, .frame = 13,        .tics =  7,                         .nextstate = S_ID24_132 },
	{ .statenum = S_ID24_132, .sprite = SPR_CSPI, .frame = 14,        .tics =  7,                         .nextstate = S_ID24_133 },
	{ .statenum = S_ID24_133, .sprite = SPR_CSPI, .frame = 15,        .tics = -1, .action = A_BossDeath },
	{ .statenum = S_ID24_134, .sprite = SPR_CSPI, .frame = 15,        .tics =  5,                         .nextstate = S_ID24_135 },
	{ .statenum = S_ID24_135, .sprite = SPR_CSPI, .frame = 14,        .tics =  5,                         .nextstate = S_ID24_136 },
	{ .statenum = S_ID24_136, .sprite = SPR_CSPI, .frame = 13,        .tics =  5,                         .nextstate = S_ID24_137 },
	{ .statenum = S_ID24_137, .sprite = SPR_CSPI, .frame = 12,        .tics =  5,                         .nextstate = S_ID24_138 },
	{ .statenum = S_ID24_138, .sprite = SPR_CSPI, .frame = 11,        .tics =  5,                         .nextstate = S_ID24_139 },
	{ .statenum = S_ID24_139, .sprite = SPR_CSPI, .frame = 10,        .tics =  5,                         .nextstate = S_ID24_140 },
	{ .statenum = S_ID24_140, .sprite = SPR_CSPI, .frame =  9,        .tics =  5,                         .nextstate = S_ID24_107 },
	// shocktrooper
	{ .statenum = S_ID24_141, .sprite = SPR_PPOS, .frame =  0, .tics = 10, .action = A_Look,              .nextstate = S_ID24_142 },
	{ .statenum = S_ID24_142, .sprite = SPR_PPOS, .frame =  1, .tics = 10, .action = A_Look,              .nextstate = S_ID24_141 },
	{ .statenum = S_ID24_143, .sprite = SPR_PPOS, .frame =  0, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_144, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_144, .sprite = SPR_PPOS, .frame =  0, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_145, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_145, .sprite = SPR_PPOS, .frame =  1, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_146, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_146, .sprite = SPR_PPOS, .frame =  1, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_147, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_147, .sprite = SPR_PPOS, .frame =  2, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_148, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_148, .sprite = SPR_PPOS, .frame =  2, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_149, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_149, .sprite = SPR_PPOS, .frame =  3, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_150, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_150, .sprite = SPR_PPOS, .frame =  3, .tics =  2, .action = A_Chase,             .nextstate = S_ID24_143, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_151, .sprite = SPR_PPOS, .frame =  4, .tics = 10, .action = A_FaceTarget,        .nextstate = S_ID24_152 },
	{ .statenum = S_ID24_152, .sprite = SPR_PPOS, .frame =  5, .tics =  2, .action = A_MonsterProjectile, .nextstate = S_ID24_153, .args = { MT_PLASMA + 1, 0, 0, 0, 0, 0, 0, 0 }, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_153, .sprite = SPR_PPOS, .frame =  4, .tics =  4,                                .nextstate = S_ID24_154, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_154, .sprite = SPR_PPOS, .frame =  5, .tics =  2, .action = A_MonsterProjectile, .nextstate = S_ID24_155, .args = { MT_PLASMA + 1, 0, 0, 0, 0, 0, 0, 0 }, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_155, .sprite = SPR_PPOS, .frame =  4, .tics =  4,                                .nextstate = S_ID24_156, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_156, .sprite = SPR_PPOS, .frame =  5, .tics =  2, .action = A_MonsterProjectile, .nextstate = S_ID24_157, .args = { MT_PLASMA + 1, 0, 0, 0, 0, 0, 0, 0 }, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_157, .sprite = SPR_PPOS, .frame =  4, .tics =  4,                                .nextstate = S_ID24_143, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_158, .sprite = SPR_PPOS, .frame =  6, .tics =  5,                                .nextstate = S_ID24_159, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_159, .sprite = SPR_PPOS, .frame =  6, .tics =  5, .action = A_Pain,              .nextstate = S_ID24_143, .flags = STATEF_SKILL5FAST },
	{ .statenum = S_ID24_160, .sprite = SPR_PPOS, .frame =  7, .tics =  0, .action = A_FaceTarget,        .nextstate = S_ID24_161 },
	{ .statenum = S_ID24_161, .sprite = SPR_PPOS, .frame =  7, .tics =  5, .action = A_SpawnObject,       .nextstate = S_ID24_162, .args = { MT_SHOCKTROOPER_HEAD + 1, 175_fx, 0, 0, 40_fx, 2_fx, 0, 1.5_fx } },
	{ .statenum = S_ID24_162, .sprite = SPR_PPOS, .frame =  8, .tics =  5, .action = A_Scream,            .nextstate = S_ID24_163 },
	{ .statenum = S_ID24_163, .sprite = SPR_PPOS, .frame =  9, .tics =  5, .action = A_Fall,              .nextstate = S_ID24_164 },
	{ .statenum = S_ID24_164, .sprite = SPR_PPOS, .frame = 10, .tics =  5,                                .nextstate = S_ID24_165 },
	{ .statenum = S_ID24_165, .sprite = SPR_PPOS, .frame = 11, .tics =  5,                                .nextstate = S_ID24_166 },
	{ .statenum = S_ID24_166, .sprite = SPR_PPOS, .frame = 12, .tics = -1 },
	{ .statenum = S_ID24_167, .sprite = SPR_PPOS, .frame = 13, .tics =  5,                                .nextstate = S_ID24_168 },
	{ .statenum = S_ID24_168, .sprite = SPR_PPOS, .frame = 14, .tics =  5, .action = A_XScream,           .nextstate = S_ID24_169 },
	{ .statenum = S_ID24_169, .sprite = SPR_PPOS, .frame = 15, .tics =  5, .action = A_Fall,              .nextstate = S_ID24_170 },
	{ .statenum = S_ID24_170, .sprite = SPR_PPOS, .frame = 16, .tics =  0, .action = A_FaceTarget,        .nextstate = S_ID24_171 },
	{ .statenum = S_ID24_171, .sprite = SPR_PPOS, .frame = 16, .tics =  5, .action = A_SpawnObject,       .nextstate = S_ID24_172, .args = { MT_SHOCKTROOPER_TORSO + 1, 170_fx, 0, -8_fx, 32_fx, 4_fx, 0, 2_fx } },
	{ .statenum = S_ID24_172, .sprite = SPR_PPOS, .frame = 17, .tics =  5,                                .nextstate = S_ID24_173 },
	{ .statenum = S_ID24_173, .sprite = SPR_PPOS, .frame = 18, .tics =  5,                                .nextstate = S_ID24_174 },
	{ .statenum = S_ID24_174, .sprite = SPR_PPOS, .frame = 19, .tics =  5,                                .nextstate = S_ID24_175 },
	{ .statenum = S_ID24_175, .sprite = SPR_PPOS, .frame = 20, .tics = -1 },
	{ .statenum = S_ID24_176, .sprite = SPR_PPOS, .frame = 12, .tics =  5,                                .nextstate = S_ID24_177 },
	{ .statenum = S_ID24_177, .sprite = SPR_PPOS, .frame = 11, .tics =  5,                                .nextstate = S_ID24_178 },
	{ .statenum = S_ID24_178, .sprite = SPR_PPOS, .frame = 10, .tics =  5,                                .nextstate = S_ID24_179 },
	{ .statenum = S_ID24_179, .sprite = SPR_PPOS, .frame =  9, .tics =  5,                                .nextstate = S_ID24_180 },
	{ .statenum = S_ID24_180, .sprite = SPR_PPOS, .frame =  8, .tics =  5,                                .nextstate = S_ID24_181 },
	{ .statenum = S_ID24_181, .sprite = SPR_PPOS, .frame =  7, .tics =  5,                                .nextstate = S_ID24_143 },
	// shocktrooper head
	{ .statenum = S_ID24_182, .sprite = SPR_PHED, .frame = 0, .tics =  3, .nextstate = S_ID24_183 },
	{ .statenum = S_ID24_183, .sprite = SPR_PHED, .frame = 1, .tics =  3, .nextstate = S_ID24_184 },
	{ .statenum = S_ID24_184, .sprite = SPR_PHED, .frame = 2, .tics =  3, .nextstate = S_ID24_185 },
	{ .statenum = S_ID24_185, .sprite = SPR_PHED, .frame = 3, .tics =  3, .nextstate = S_ID24_186 },
	{ .statenum = S_ID24_186, .sprite = SPR_PHED, .frame = 4, .tics =  3, .nextstate = S_ID24_187 },
	{ .statenum = S_ID24_187, .sprite = SPR_PHED, .frame = 5, .tics =  3, .nextstate = S_ID24_188 },
	{ .statenum = S_ID24_188, .sprite = SPR_PHED, .frame = 6, .tics =  3, .nextstate = S_ID24_189 },
	{ .statenum = S_ID24_189, .sprite = SPR_PHED, .frame = 7, .tics =  3, .nextstate = S_ID24_190 },
	{ .statenum = S_ID24_190, .sprite = SPR_PHED, .frame = 8, .tics =  3, .nextstate = S_ID24_182 },
	{ .statenum = S_ID24_191, .sprite = SPR_PHED, .frame = 9, .tics = -1, .nextstate = S_ID24_191 },
	// shocktrooper torso
	{ .statenum = S_ID24_192, .sprite = SPR_PPOS, .frame = 21, .tics = -1, .nextstate = S_ID24_192 },
	{ .statenum = S_ID24_193, .sprite = SPR_PPOS, .frame = 22, .tics =  5, .nextstate = S_ID24_194 },
	{ .statenum = S_ID24_194, .sprite = SPR_PPOS, .frame = 23, .tics = -1 },
	// vassago
	{ .statenum = S_ID24_195, .sprite = SPR_VASS, .frame =  0,        .tics = 10, .action = A_Look,              .nextstate = S_ID24_196 },
	{ .statenum = S_ID24_196, .sprite = SPR_VASS, .frame =  1,        .tics = 10, .action = A_Look,              .nextstate = S_ID24_195 },
	{ .statenum = S_ID24_197, .sprite = SPR_VASS, .frame =  0,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_198 },
	{ .statenum = S_ID24_198, .sprite = SPR_VASS, .frame =  0,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_199 },
	{ .statenum = S_ID24_199, .sprite = SPR_VASS, .frame =  1,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_200 },
	{ .statenum = S_ID24_200, .sprite = SPR_VASS, .frame =  1,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_201 },
	{ .statenum = S_ID24_201, .sprite = SPR_VASS, .frame =  2,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_202 },
	{ .statenum = S_ID24_202, .sprite = SPR_VASS, .frame =  2,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_203 },
	{ .statenum = S_ID24_203, .sprite = SPR_VASS, .frame =  3,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_204 },
	{ .statenum = S_ID24_204, .sprite = SPR_VASS, .frame =  3,        .tics =  3, .action = A_Chase,             .nextstate = S_ID24_197 },
	{ .statenum = S_ID24_205, .sprite = SPR_VASS, .frame =  4_bright, .tics =  0, .action = A_PlaySound,         .nextstate = S_ID24_206, .misc1 = -1879048159 /* monsters/vassago/attack */, .misc2 = 0 },
	{ .statenum = S_ID24_206, .sprite = SPR_VASS, .frame =  4_bright, .tics =  8, .action = A_FaceTarget,        .nextstate = S_ID24_207 },
	{ .statenum = S_ID24_207, .sprite = SPR_VASS, .frame =  5_bright, .tics =  4, .action = A_FaceTarget,        .nextstate = S_ID24_208 },
	{ .statenum = S_ID24_208, .sprite = SPR_VASS, .frame =  6_bright, .tics =  4, .action = A_FaceTarget,        .nextstate = S_ID24_209 },
	{ .statenum = S_ID24_209, .sprite = SPR_VASS, .frame =  7_bright, .tics =  8, .action = A_MonsterProjectile, .nextstate = S_ID24_197, .args = { MT_VASSAGO_FLAME + 1, 0, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_210, .sprite = SPR_VASS, .frame =  8,        .tics =  2,                                .nextstate = S_ID24_211 },
	{ .statenum = S_ID24_211, .sprite = SPR_VASS, .frame =  8,        .tics =  2, .action = A_Pain,              .nextstate = S_ID24_197 },
	{ .statenum = S_ID24_212, .sprite = SPR_VASS, .frame =  9_bright, .tics =  8,                                .nextstate = S_ID24_213 },
	{ .statenum = S_ID24_213, .sprite = SPR_VASS, .frame = 10_bright, .tics =  8, .action = A_Scream,            .nextstate = S_ID24_214 },
	{ .statenum = S_ID24_214, .sprite = SPR_VASS, .frame = 11_bright, .tics =  7,                                .nextstate = S_ID24_215 },
	{ .statenum = S_ID24_215, .sprite = SPR_VASS, .frame = 12_bright, .tics =  6, .action = A_Fall,              .nextstate = S_ID24_216 },
	{ .statenum = S_ID24_216, .sprite = SPR_VASS, .frame = 13_bright, .tics =  6,                                .nextstate = S_ID24_217 },
	{ .statenum = S_ID24_217, .sprite = SPR_VASS, .frame = 14_bright, .tics =  6,                                .nextstate = S_ID24_218 },
	{ .statenum = S_ID24_218, .sprite = SPR_VASS, .frame = 15_bright, .tics =  7,                                .nextstate = S_ID24_219 },
	{ .statenum = S_ID24_219, .sprite = SPR_VASS, .frame = 16,        .tics = -1, .action = A_BossDeath },
	{ .statenum = S_ID24_220, .sprite = SPR_VASS, .frame = 15,        .tics =  8,                                .nextstate = S_ID24_221 },
	{ .statenum = S_ID24_221, .sprite = SPR_VASS, .frame = 14,        .tics =  8,                                .nextstate = S_ID24_222 },
	{ .statenum = S_ID24_222, .sprite = SPR_VASS, .frame = 13,        .tics =  8,                                .nextstate = S_ID24_223 },
	{ .statenum = S_ID24_223, .sprite = SPR_VASS, .frame = 12,        .tics =  8,                                .nextstate = S_ID24_224 },
	{ .statenum = S_ID24_224, .sprite = SPR_VASS, .frame = 11,        .tics =  8,                                .nextstate = S_ID24_225 },
	{ .statenum = S_ID24_225, .sprite = SPR_VASS, .frame = 10,        .tics =  8,                                .nextstate = S_ID24_226 },
	{ .statenum = S_ID24_226, .sprite = SPR_VASS, .frame =  9,        .tics =  8,                                .nextstate = S_ID24_197 },
	// vassago flame
	{ .statenum = S_ID24_227, .sprite = SPR_VFLM, .frame =  0_bright, .tics = 4,                           .nextstate = S_ID24_228 },
	{ .statenum = S_ID24_228, .sprite = SPR_VFLM, .frame =  1_bright, .tics = 4,                           .nextstate = S_ID24_229 },
	{ .statenum = S_ID24_229, .sprite = SPR_VFLM, .frame =  2_bright, .tics = 0, .action = A_RemoveFlags,  .nextstate = S_ID24_230, .args = { static_cast<statearg_t>(MF_NOBLOCKMAP), 0, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_230, .sprite = SPR_VFLM, .frame =  2_bright, .tics = 0, .action = A_RemoveFlags,  .nextstate = S_ID24_231, .args = { static_cast<statearg_t>(MF_NOGRAVITY), 0, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_231, .sprite = SPR_VFLM, .frame =  2_bright, .tics = 0, .action = A_RandomJump,   .nextstate = S_ID24_232, .misc1 = S_ID24_233, .misc2 = 128 },
	{ .statenum = S_ID24_232, .sprite = SPR_VFLM, .frame =  2_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_234, .misc1 = -1879048171 /* monsters/vassago/hot1 */, .misc2 = 0 },
	{ .statenum = S_ID24_233, .sprite = SPR_VFLM, .frame =  2_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_234, .misc1 = -1879048170 /* monsters/vassago/hot2 */, .misc2 = 0 },
	{ .statenum = S_ID24_234, .sprite = SPR_VFLM, .frame =  2_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_235, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_235, .sprite = SPR_VFLM, .frame =  3_bright, .tics = 4,                           .nextstate = S_ID24_236 },
	{ .statenum = S_ID24_236, .sprite = SPR_VFLM, .frame =  4_bright, .tics = 4,                           .nextstate = S_ID24_237 },
	{ .statenum = S_ID24_237, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_238, .misc1 = -1879048169 /* monsters/vassago/hot3 */, .misc2 = 0 },
	{ .statenum = S_ID24_238, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_239, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_239, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_240 },
	{ .statenum = S_ID24_240, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_241 },
	{ .statenum = S_ID24_241, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_242, .misc1 = -1879048170 /* monsters/vassago/hot2 */, .misc2 = 0 },
	{ .statenum = S_ID24_242, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_243, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_243, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_244 },
	{ .statenum = S_ID24_244, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_245 },
	{ .statenum = S_ID24_245, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_246, .misc1 = -1879048169 /* monsters/vassago/hot3 */, .misc2 = 0 },
	{ .statenum = S_ID24_246, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_247, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_247, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_248 },
	{ .statenum = S_ID24_248, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_249 },
	{ .statenum = S_ID24_249, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_250, .misc1 = -1879048171 /* monsters/vassago/hot1 */, .misc2 = 0 },
	{ .statenum = S_ID24_250, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_251, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_251, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_252 },
	{ .statenum = S_ID24_252, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_253 },
	{ .statenum = S_ID24_253, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_254, .misc1 = -1879048170 /* monsters/vassago/hot2 */, .misc2 = 0 },
	{ .statenum = S_ID24_254, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_255, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_255, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_256 },
	{ .statenum = S_ID24_256, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_257 },
	{ .statenum = S_ID24_257, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_258, .misc1 = -1879048171 /* monsters/vassago/hot1 */, .misc2 = 0 },
	{ .statenum = S_ID24_258, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_259 },
	{ .statenum = S_ID24_259, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_260 },
	{ .statenum = S_ID24_260, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_261 },
	{ .statenum = S_ID24_261, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_262, .misc1 = -1879048170 /* monsters/vassago/hot2 */, .misc2 = 0 },
	{ .statenum = S_ID24_262, .sprite = SPR_VFLM, .frame =  5_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_263, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_263, .sprite = SPR_VFLM, .frame =  6_bright, .tics = 4,                           .nextstate = S_ID24_264 },
	{ .statenum = S_ID24_264, .sprite = SPR_VFLM, .frame =  7_bright, .tics = 4,                           .nextstate = S_ID24_265 },
	{ .statenum = S_ID24_265, .sprite = SPR_VFLM, .frame =  8_bright, .tics = 0, .action = A_PlaySound,    .nextstate = S_ID24_266, .misc1 = -1879048169 /* monsters/vassago/hot3 */, .misc2 = 0 },
	{ .statenum = S_ID24_266, .sprite = SPR_VFLM, .frame =  8_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_267, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_267, .sprite = SPR_VFLM, .frame =  9_bright, .tics = 4,                           .nextstate = S_ID24_268 },
	{ .statenum = S_ID24_268, .sprite = SPR_VFLM, .frame = 10_bright, .tics = 4,                           .nextstate = S_ID24_269 },
	{ .statenum = S_ID24_269, .sprite = SPR_VFLM, .frame = 11_bright, .tics = 4, .action = A_RadiusDamage, .nextstate = S_ID24_270, .args = { 10, 128, 0, 0, 0, 0, 0, 0 } },
	{ .statenum = S_ID24_270, .sprite = SPR_VFLM, .frame = 12_bright, .tics = 4,                           .nextstate = S_ID24_271 },
	{ .statenum = S_ID24_271, .sprite = SPR_VFLM, .frame = 13_bright, .tics = 4,                           .nextstate = S_ID24_272 },
	{ .statenum = S_ID24_272, .sprite = SPR_VFLM, .frame = 14_bright, .tics = 4,                           .nextstate = S_ID24_273 },
	{ .statenum = S_ID24_273, .sprite = SPR_VFLM, .frame = 15_bright, .tics = 4,                           .nextstate = S_ID24_274 },
	{ .statenum = S_ID24_274, .sprite = SPR_VFLM, .frame = 16_bright, .tics = 4 },
	// tyrant
	{ .statenum = S_ID24_275, .sprite = SPR_CYB2, .frame =  0,        .tics = 10, .action = A_Look,        .nextstate = S_ID24_276 },
	{ .statenum = S_ID24_276, .sprite = SPR_CYB2, .frame =  1,        .tics = 10, .action = A_Look,        .nextstate = S_ID24_275 },
	{ .statenum = S_ID24_277, .sprite = SPR_CYB2, .frame =  0,        .tics =  3, .action = A_Hoof,        .nextstate = S_ID24_278 },
	{ .statenum = S_ID24_278, .sprite = SPR_CYB2, .frame =  0,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_279 },
	{ .statenum = S_ID24_279, .sprite = SPR_CYB2, .frame =  1,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_280 },
	{ .statenum = S_ID24_280, .sprite = SPR_CYB2, .frame =  1,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_281 },
	{ .statenum = S_ID24_281, .sprite = SPR_CYB2, .frame =  2,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_282 },
	{ .statenum = S_ID24_282, .sprite = SPR_CYB2, .frame =  2,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_283 },
	{ .statenum = S_ID24_283, .sprite = SPR_CYB2, .frame =  3,        .tics =  0, .action = A_PlaySound,   .nextstate = S_ID24_284, .misc1 =  -1879048161 /* monsters/tyrant/walk */, .misc2 = 0 },
	{ .statenum = S_ID24_284, .sprite = SPR_CYB2, .frame =  3,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_285 },
	{ .statenum = S_ID24_285, .sprite = SPR_CYB2, .frame =  3,        .tics =  3, .action = A_Chase,       .nextstate = S_ID24_277 },
	{ .statenum = S_ID24_286, .sprite = SPR_CYB2, .frame =  4,        .tics =  6, .action = A_FaceTarget,  .nextstate = S_ID24_287 },
	{ .statenum = S_ID24_287, .sprite = SPR_CYB2, .frame =  5_bright, .tics = 12, .action = A_CyberAttack, .nextstate = S_ID24_288 },
	{ .statenum = S_ID24_288, .sprite = SPR_CYB2, .frame =  4,        .tics = 12, .action = A_FaceTarget,  .nextstate = S_ID24_289 },
	{ .statenum = S_ID24_289, .sprite = SPR_CYB2, .frame =  5_bright, .tics = 12, .action = A_CyberAttack, .nextstate = S_ID24_290 },
	{ .statenum = S_ID24_290, .sprite = SPR_CYB2, .frame =  4,        .tics = 12, .action = A_FaceTarget,  .nextstate = S_ID24_291 },
	{ .statenum = S_ID24_291, .sprite = SPR_CYB2, .frame =  5_bright, .tics = 12, .action = A_CyberAttack, .nextstate = S_ID24_277 },
	{ .statenum = S_ID24_292, .sprite = SPR_CYB2, .frame =  6,        .tics = 10, .action = A_Pain,        .nextstate = S_ID24_277 },
	{ .statenum = S_ID24_293, .sprite = SPR_CYB2, .frame =  7,        .tics = 10,                          .nextstate = S_ID24_294 },
	{ .statenum = S_ID24_294, .sprite = SPR_CYB2, .frame =  8,        .tics = 10, .action = A_Scream,      .nextstate = S_ID24_295 },
	{ .statenum = S_ID24_295, .sprite = SPR_CYB2, .frame =  9,        .tics = 10,                          .nextstate = S_ID24_296 },
	{ .statenum = S_ID24_296, .sprite = SPR_CYB2, .frame = 10,        .tics = 10,                          .nextstate = S_ID24_297 },
	{ .statenum = S_ID24_297, .sprite = SPR_CYB2, .frame = 11,        .tics = 10,                          .nextstate = S_ID24_298 },
	{ .statenum = S_ID24_298, .sprite = SPR_CYB2, .frame = 12,        .tics = 10, .action = A_Fall,        .nextstate = S_ID24_299 },
	{ .statenum = S_ID24_299, .sprite = SPR_CYB2, .frame = 13,        .tics = 10,                          .nextstate = S_ID24_300 },
	{ .statenum = S_ID24_300, .sprite = SPR_CYB2, .frame = 14,        .tics = 10,                          .nextstate = S_ID24_301 },
	{ .statenum = S_ID24_301, .sprite = SPR_CYB2, .frame = 15,        .tics = 30,                          .nextstate = S_ID24_302 },
	{ .statenum = S_ID24_302, .sprite = SPR_CYB2, .frame = 15,        .tics = -1, .action = A_BossDeath },
	// ------------------------ weapons ------------------------
	{ .statenum = S_ID24_303, .sprite = -1 },
	{ .statenum = S_ID24_304, .sprite = -1 },
	{ .statenum = S_ID24_305, .sprite = -1 },
	{ .statenum = S_ID24_306, .sprite = -1 },
	{ .statenum = S_ID24_307, .sprite = -1 },
	{ .statenum = S_ID24_308, .sprite = -1 },
	{ .statenum = S_ID24_309, .sprite = -1 },
	{ .statenum = S_ID24_310, .sprite = -1 },
	{ .statenum = S_ID24_311, .sprite = -1 },
	{ .statenum = S_ID24_312, .sprite = -1 },
	{ .statenum = S_ID24_313, .sprite = -1 },
	{ .statenum = S_ID24_314, .sprite = -1 },
	{ .statenum = S_ID24_315, .sprite = -1 },
	{ .statenum = S_ID24_316, .sprite = -1 },
	{ .statenum = S_ID24_317, .sprite = -1 },
	{ .statenum = S_ID24_318, .sprite = -1 },
	{ .statenum = S_ID24_319, .sprite = -1 },
	{ .statenum = S_ID24_320, .sprite = -1 },
	{ .statenum = S_ID24_321, .sprite = -1 },
	{ .statenum = S_ID24_322, .sprite = -1 },
	{ .statenum = S_ID24_323, .sprite = -1 },
	{ .statenum = S_ID24_324, .sprite = -1 },
	{ .statenum = S_ID24_325, .sprite = -1 },
	{ .statenum = S_ID24_326, .sprite = -1 },
	{ .statenum = S_ID24_327, .sprite = -1 },
	{ .statenum = S_ID24_328, .sprite = -1 },
	{ .statenum = S_ID24_329, .sprite = -1 },
	{ .statenum = S_ID24_330, .sprite = -1 },
	{ .statenum = S_ID24_331, .sprite = -1 },
	{ .statenum = S_ID24_332, .sprite = -1 },
	{ .statenum = S_ID24_333, .sprite = -1 },
	{ .statenum = S_ID24_334, .sprite = -1 },
	{ .statenum = S_ID24_335, .sprite = -1 },
	{ .statenum = S_ID24_336, .sprite = -1 },
	{ .statenum = S_ID24_337, .sprite = -1 },
	{ .statenum = S_ID24_338, .sprite = -1 },
	{ .statenum = S_ID24_339, .sprite = -1 },
	{ .statenum = S_ID24_340, .sprite = -1 },
	{ .statenum = S_ID24_341, .sprite = -1 },
	{ .statenum = S_ID24_342, .sprite = -1 },
	{ .statenum = S_ID24_343, .sprite = -1 },
	{ .statenum = S_ID24_344, .sprite = -1 },
	{ .statenum = S_ID24_345, .sprite = -1 },
	{ .statenum = S_ID24_346, .sprite = -1 },
	{ .statenum = S_ID24_347, .sprite = -1 },
	{ .statenum = S_ID24_348, .sprite = -1 },
	{ .statenum = S_ID24_349, .sprite = -1 },
	{ .statenum = S_ID24_350, .sprite = -1 },
	{ .statenum = S_ID24_351, .sprite = -1 },
	{ .statenum = S_ID24_352, .sprite = -1 },
	{ .statenum = S_ID24_353, .sprite = -1 },
	{ .statenum = S_ID24_354, .sprite = -1 },
	{ .statenum = S_ID24_355, .sprite = -1 },
	{ .statenum = S_ID24_356, .sprite = -1 },
	{ .statenum = S_ID24_357, .sprite = -1 },
	{ .statenum = S_ID24_358, .sprite = -1 },
	{ .statenum = S_ID24_359, .sprite = -1 },
	{ .statenum = S_ID24_360, .sprite = -1 },
	{ .statenum = S_ID24_361, .sprite = -1 },
	{ .statenum = S_ID24_362, .sprite = -1 },
	{ .statenum = S_ID24_363, .sprite = -1 },
	{ .statenum = S_ID24_364, .sprite = -1 },
	{ .statenum = S_ID24_365, .sprite = -1 },
	{ .statenum = S_ID24_366, .sprite = -1 },
	{ .statenum = S_ID24_367, .sprite = -1 },
	{ .statenum = S_ID24_368, .sprite = -1 },
	{ .statenum = S_ID24_369, .sprite = -1 },
	{ .statenum = S_ID24_370, .sprite = -1 },
	{ .statenum = S_ID24_371, .sprite = -1 },
	{ .statenum = S_ID24_372, .sprite = -1 },
	{ .statenum = S_ID24_373, .sprite = -1 },
	{ .statenum = S_ID24_374, .sprite = -1 },
	{ .statenum = S_ID24_375, .sprite = -1 },
	{ .statenum = S_ID24_376, .sprite = -1 },
	{ .statenum = S_ID24_377, .sprite = -1 },
	{ .statenum = S_ID24_378, .sprite = -1 },
	{ .statenum = S_ID24_379, .sprite = -1 },
	{ .statenum = S_ID24_380, .sprite = -1 },
	{ .statenum = S_ID24_381, .sprite = -1 },
	{ .statenum = S_ID24_382, .sprite = -1 },
	{ .statenum = S_ID24_383, .sprite = -1 },
	{ .statenum = S_ID24_384, .sprite = -1 },
	{ .statenum = S_ID24_385, .sprite = -1 },
	{ .statenum = S_ID24_386, .sprite = -1 },
	{ .statenum = S_ID24_387, .sprite = -1 },
	{ .statenum = S_ID24_388, .sprite = -1 },
	{ .statenum = S_ID24_389, .sprite = -1 },
	{ .statenum = S_ID24_390, .sprite = -1 },
	{ .statenum = S_ID24_391, .sprite = -1 },
	{ .statenum = S_ID24_392, .sprite = -1 },
	{ .statenum = S_ID24_393, .sprite = -1 },
	{ .statenum = S_ID24_394, .sprite = -1 },
	{ .statenum = S_ID24_395, .sprite = -1 },
	{ .statenum = S_ID24_396, .sprite = -1 },
	{ .statenum = S_ID24_397, .sprite = -1 },
	{ .statenum = S_ID24_398, .sprite = -1 },
	{ .statenum = S_ID24_399, .sprite = -1 },
	{ .statenum = S_ID24_400, .sprite = -1 },
	{ .statenum = S_ID24_401, .sprite = -1 },
	{ .statenum = S_ID24_402, .sprite = -1 },
	{ .statenum = S_ID24_403, .sprite = -1 },
	{ .statenum = S_ID24_404, .sprite = -1 },
	{ .statenum = S_ID24_405, .sprite = -1 },
	{ .statenum = S_ID24_406, .sprite = -1 },
	{ .statenum = S_ID24_407, .sprite = -1 },
	{ .statenum = S_ID24_408, .sprite = -1 },
	{ .statenum = S_ID24_409, .sprite = -1 },
	{ .statenum = S_ID24_410, .sprite = -1 },
	{ .statenum = S_ID24_411, .sprite = -1 },
	{ .statenum = S_ID24_412, .sprite = -1 },
	{ .statenum = S_ID24_413, .sprite = -1 },
	{ .statenum = S_ID24_414, .sprite = -1 },
	{ .statenum = S_ID24_415, .sprite = -1 },
	{ .statenum = S_ID24_416, .sprite = -1 },
	{ .statenum = S_ID24_417, .sprite = -1 },
	{ .statenum = S_ID24_418, .sprite = -1 },
	{ .statenum = S_ID24_419, .sprite = -1 },
	{ .statenum = S_ID24_420, .sprite = -1 },
	{ .statenum = S_ID24_421, .sprite = -1 },
	{ .statenum = S_ID24_422, .sprite = -1 },
	{ .statenum = S_ID24_423, .sprite = -1 },
	{ .statenum = S_ID24_424, .sprite = -1 },
	{ .statenum = S_ID24_425, .sprite = -1 },
	{ .statenum = S_ID24_426, .sprite = -1 },
	{ .statenum = S_ID24_427, .sprite = -1 },
	{ .statenum = S_ID24_428, .sprite = -1 },
	{ .statenum = S_ID24_429, .sprite = -1 },
	{ .statenum = S_ID24_430, .sprite = -1 },
	{ .statenum = S_ID24_431, .sprite = -1 },
	{ .statenum = S_ID24_432, .sprite = -1 },
	{ .statenum = S_ID24_433, .sprite = -1 },
	{ .statenum = S_ID24_434, .sprite = -1 },
	{ .statenum = S_ID24_435, .sprite = -1 },
	{ .statenum = S_ID24_436, .sprite = -1 },
	{ .statenum = S_ID24_437, .sprite = -1 },
	{ .statenum = S_ID24_438, .sprite = -1 },
	{ .statenum = S_ID24_439, .sprite = -1 },
	{ .statenum = S_ID24_440, .sprite = -1 },
	{ .statenum = S_ID24_441, .sprite = -1 },
	{ .statenum = S_ID24_442, .sprite = -1 },
	{ .statenum = S_ID24_443, .sprite = -1 },
	{ .statenum = S_ID24_444, .sprite = -1 },
	{ .statenum = S_ID24_445, .sprite = -1 },
	{ .statenum = S_ID24_446, .sprite = -1 },
	{ .statenum = S_ID24_447, .sprite = -1 },
	{ .statenum = S_ID24_448, .sprite = -1 },
	{ .statenum = S_ID24_449, .sprite = -1 },
	{ .statenum = S_ID24_450, .sprite = -1 },
	{ .statenum = S_ID24_451, .sprite = -1 },
	{ .statenum = S_ID24_452, .sprite = -1 },
	{ .statenum = S_ID24_453, .sprite = -1 },
	{ .statenum = S_ID24_454, .sprite = -1 },
	{ .statenum = S_ID24_455, .sprite = -1 },
	{ .statenum = S_ID24_456, .sprite = -1 },
	{ .statenum = S_ID24_457, .sprite = -1 },
	{ .statenum = S_ID24_458, .sprite = -1 },
	{ .statenum = S_ID24_459, .sprite = -1 },
	{ .statenum = S_ID24_460, .sprite = -1 },
	{ .statenum = S_ID24_461, .sprite = -1 },
	{ .statenum = S_ID24_462, .sprite = -1 },
	{ .statenum = S_ID24_463, .sprite = -1 },
	{ .statenum = S_ID24_464, .sprite = -1 },
	{ .statenum = S_ID24_465, .sprite = -1 },
});

const std::array id24mobjinfo = std::to_array<const mobjinfo_t>({
	{
		.type         = MT_GHOUL,
		.doomednum    = -28672,
		// .spawnstate   =
		.spawnhealth  = 50,
		// .seestate     =
		.seesound     = "monsters/ghoul/sight",
		.reactiontime = 8,
		.flags = MF_FLOAT|MF_NOGRAVITY,

	},
	{
		.type = MT_BANSHEE,
	},
	{
		.type = MT_MINDWEAVER,
	},
	{
		.type = MT_SHOCKTROOPER,
	},
	{
		.type = MT_VASSAGO,
	},
	{
		.type = MT_TYRANT,
	},
	{
		.type = MT_TYRANT_BOSS_1,
	},
	{
		.type = MT_TYRANT_BOSS_2,
	},
	{
		.type = MT_INCINERATOR_FLAME,
	},
	{
		.type = MT_HEATWAVE_SPAWNER,
	},
	{
		.type = MT_HEATWAVE_RIPPER,
	},
	{
		.type = MT_GHOUL_BALL,
	},
	{
		.type = MT_SHOCKTROOPER_HEAD,
	},
	{
		.type = MT_SHOCKTROOPER_TORSO,
	},
	{
		.type = MT_VASSAGO_FLAME,
	},
	{
		.type = MT_STALAGMITE_GRAY,
	},
	{
		.type = MT_LARGE_CORPSE_PILE,
	},
	{
		.type = MT_HUMAN_BBQ_1,
	},
	{
		.type = MT_HUMAN_BBQ_2,
	},
	{
		.type = MT_HANGING_VICTIM_BOTH_LEGS,
	},
	{
		.type = MT_HANGING_VICTIM_BOTH_LEGS_BLOCKING,
	},
	{
		.type = MT_HANGING_VICTIM_CRUCIFIED,
	},
	{
		.type = MT_HANGING_VICTIM_CRUCIFIED_BLOCKING,
	},
	{
		.type = MT_HANGING_VICTIM_ARMS_BOUND,
	},
	{
		.type = MT_HANGING_VICTIM_ARMS_BOUND_BLOCKING,
	},
	{
		.type = MT_HANGING_BARON_OF_HELL,
	},
	{
		.type = MT_HANGING_BARON_OF_HELL_BLOCKING,
	},
	{
		.type = MT_HANGING_VICTIM_CHAINED,
	},
	{
		.type = MT_HANGING_VICTIM_CHAINED_BLOCKING,
	},
	{
		.type = MT_HANGING_TORSO_CHAINED,
	},
	{
		.type = MT_HANGING_TORSO_CHAINED_BLOCKING,
	},
	{
		.type = MT_SKULL_POLE_TRIO,
	},
	{
		.type = MT_SKULL_GIBS,
	},
	{
		.type = MT_BUSH_SHORT,
	},
	{
		.type = MT_BUSH_SHORT_BURNED_1,
	},
	{
		.type = MT_BUSH_SHORT_BURNED_2,
	},
	{
		.type = MT_BUSH_TALL,
	},
	{
		.type = MT_BUSH_TALL_BURNED_1,
	},
	{
		.type = MT_BUSH_TALL_BURNED_2,
	},
	{
		.type = MT_CAVE_ROCK_COLUMN,
	},
	{
		.type = MT_CAVE_STALAGMITE_LARGE,
	},
	{
		.type = MT_CAVE_STALAGMITE_MEDIUM,
	},
	{
		.type = MT_CAVE_STALAGMITE_SMALL,
	},
	{
		.type = MT_CAVE_STALACTITE_LARGE,
	},
	{
		.type = MT_CAVE_STALACTITE_LARGE_BLOCKING,
	},
	{
		.type = MT_CAVE_STALACTITE_MEDIUM,
	},
	{
		.type = MT_CAVE_STALACTITE_MEDIUM_BLOCKING,
	},
	{
		.type = MT_CAVE_STALACTITE_SMALL,
	},
	{
		.type = MT_CAVE_STALACTITE_SMALL_BLOCKING,
	},
	{
		.type = MT_OFFICE_CHAIR,
	},
	{
		.type = MT_OFFICE_LAMP_BREAKABLE,
	},
	{
		.type = MT_CEILING_LAMP,
	},
	{
		.type = MT_CANDELABRA_SHORT,
	},
	{
		.type = MT_AMBIENT_KLAXON,
	},
	{
		.type = MT_AMBIENT_PORTAL_OPEN,
	},
	{
		.type = MT_AMBIENT_PORTAL_LOOP,
	},
	{
		.type = MT_AMBIENT_PORTAL_CLOSE,
	},
	{
		.type = MT_FUEL_CAN,
	},
	{
		.type = MT_FUEL_TANK,
	},
	{
		.type = MT_CALAMITY_BLADE,
	},
	{
		.type = MT_INCINERATOR,
	},
});

} // namespace

std::span<const mobjinfo_t> getID24Mobjinfo() {
	return id24mobjinfo;
}

std::span<const state_t> getID24States() {
	return id24states;
}

std::span<const char* const> getI24SprNames() {
	return id24sprnames;
}


VERSION_CONTROL (info_cpp, "$Id$")
