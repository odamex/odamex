// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2008 by The Odamex Team.
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
//	Translate old linedefs to new linedefs
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "g_level.h"
#include "p_lnspec.h"
#include "doomdata.h"
#include "r_data.h"
#include "m_swap.h"
#include "p_spec.h"
#include "p_local.h"

// Speeds for ceilings/crushers (x/8 units per tic)
//	(Hexen crushers go up at half the speed that they go down)
#define C_SLOW			8
#define C_NORMAL		16
#define C_FAST			32
#define C_TURBO			64

#define CEILWAIT		150

// Speeds for floors (x/8 units per tic)
#define F_SLOW			8
#define F_NORMAL		16
#define F_FAST			32
#define F_TURBO			64

// Speeds for doors (x/8 units per tic)
#define D_SLOW			16
#define D_NORMAL		32
#define D_FAST			64
#define D_TURBO			128

#define VDOORWAIT		150

// Speeds for stairs (x/8 units per tic)
#define S_SLOW			2
#define S_NORMAL		4
#define S_FAST			16
#define S_TURBO			32

// Speeds for plats (Hexen plats stop 8 units above the floor)
#define P_SLOW			8
#define P_NORMAL		16
#define P_FAST			32
#define P_TURBO			64

#define PLATWAIT		105

#define ELEVATORSPEED	32

// Speeds for donut slime and pillar (x/8 units per tic)
#define DORATE			4

// Texture scrollers operate at a rate of x/64 units per tic.
#define SCROLL_UNIT		64


//Define masks, shifts, for fields in generalized linedef types
// (from BOOM's p_psec.h file)

#define GenFloorBase          (0x6000)
#define GenCeilingBase        (0x4000)
#define GenDoorBase           (0x3c00)
#define GenLockedBase         (0x3800)
#define GenLiftBase           (0x3400)
#define GenStairsBase         (0x3000)
#define GenCrusherBase        (0x2F80)

#define OdamexStaticInits      (333)

// define names for the TriggerType field of the general linedefs

typedef enum
{
  WalkOnce,
  WalkMany,
  SwitchOnce,
  SwitchMany,
  GunOnce,
  GunMany,
  PushOnce,
  PushMany
} triggertype_e;


// Line special translation structure
typedef struct {
	bool	cross, use, shoot;
	bool	allow_monster, allow_monster_only, allow_repeat;
	byte	newspecial;
	byte	args[5];
} xlat_t;

#define TAG	123		// Special value that gets replaced with the line tag

static const xlat_t SpecialTranslation[] = {
/*   0 */ { 0,0,0, 0,0,0, 0,							{ 0, 0, 0, 0, 0 } },
/*   1 */ { 0,1,0, 1,0,1, Door_Raise,					{ 0, D_SLOW, VDOORWAIT, 0, 0 } },
/*   2 */ { 1,0,0, 0,0,0, Door_Open,					{ TAG, D_SLOW, 0, 0, 0 } },
/*   3 */ { 1,0,0, 0,0,0, Door_Close,					{ TAG, D_SLOW, 0, 0, 0 } },
/*   4 */ { 1,0,0, 1,0,0, Door_Raise,					{ TAG, D_SLOW, VDOORWAIT, 0, 0 } },
/*   5 */ { 1,0,0, 0,0,0, Floor_RaiseToLowestCeiling,	{ TAG, F_SLOW, 0, 0, 0 } },
/*	 6 */ { 1,0,0, 0,0,0, Ceiling_CrushAndRaiseA,		{ TAG, C_NORMAL, C_NORMAL, 10, 0 } },
/*   7 */ { 0,1,0, 0,0,0, Stairs_BuildUpDoom,			{ TAG, S_SLOW, 8, 0, 0 } },
/*   8 */ { 1,0,0, 0,0,0, Stairs_BuildUpDoom,			{ TAG, S_SLOW, 8, 0, 0 } },
/*	 9 */ { 0,1,0, 0,0,0, Floor_Donut,					{ TAG, DORATE, DORATE, 0, 0 } },
/*  10 */ { 1,0,0, 1,0,0, Plat_DownWaitUpStayLip,		{ TAG, P_FAST, PLATWAIT, 0, 0 } },
/*  11 */ { 0,1,0, 0,0,0, Exit_Normal,					{ 0, 0, 0, 0, 0 } },
/*  12 */ { 1,0,0, 0,0,0, Light_MaxNeighbor,			{ TAG, 0, 0, 0, 0 } },
/*  13 */ { 1,0,0, 0,0,0, Light_ChangeToValue,			{ TAG, 255, 0, 0, 0 } },
/*  14 */ { 0,1,0, 0,0,0, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 4, 0, 0 } },
/*  15 */ { 0,1,0, 0,0,0, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 3, 0, 0 } },
/*  16 */ { 1,0,0, 0,0,0, Door_CloseWaitOpen,			{ TAG, D_SLOW, 240, 0, 0 } },
/*  17 */ { 1,0,0, 0,0,0, Light_StrobeDoom,				{ TAG, 5, 35, 0, 0 } },
/*  18 */ { 0,1,0, 0,0,0, Floor_RaiseToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  19 */ { 1,0,0, 0,0,0, Floor_LowerToHighest,			{ TAG, F_SLOW, 128, 0, 0 } },
/*  20 */ { 0,1,0, 0,0,0, Plat_RaiseAndStayTx0,			{ TAG, P_SLOW/2, 0, 0, 0 } },
/*  21 */ { 0,1,0, 0,0,0, Plat_DownWaitUpStayLip,		{ TAG, P_FAST, PLATWAIT, 0, 0 } },
/*  22 */ { 1,0,0, 0,0,0, Plat_RaiseAndStayTx0,			{ TAG, P_SLOW/2, 0, 0, 0 } },
/*  23 */ { 0,1,0, 0,0,0, Floor_LowerToLowest,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  24 */ { 0,0,1, 0,0,0, Floor_RaiseToLowestCeiling,	{ TAG, F_SLOW, 0, 0, 0 } },
/*  25 */ { 1,0,0, 0,0,0, Ceiling_CrushAndRaiseA,		{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/*  26 */ { 0,1,0, 0,0,1, Door_LockedRaise,				{ TAG, D_SLOW, VDOORWAIT, BCard | CardIsSkull, 0 } },
/*  27 */ { 0,1,0, 0,0,1, Door_LockedRaise,				{ TAG, D_SLOW, VDOORWAIT, YCard | CardIsSkull, 0 } },
/*  28 */ { 0,1,0, 0,0,1, Door_LockedRaise,				{ TAG, D_SLOW, VDOORWAIT, RCard | CardIsSkull, 0 } },
/*  29 */ { 0,1,0, 0,0,0, Door_Raise,					{ TAG, D_SLOW, VDOORWAIT, 0, 0 } },
/*  30 */ { 1,0,0, 0,0,0, Floor_RaiseByTexture,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  31 */ { 0,1,0, 0,0,0, Door_Open,					{ 0, D_SLOW, 0, 0, 0 } },
/*  32 */ { 0,1,0, 1,0,0, Door_LockedRaise,				{ 0, D_SLOW, 0, BCard | CardIsSkull, 0 } },
/*  33 */ { 0,1,0, 1,0,0, Door_LockedRaise,				{ 0, D_SLOW, 0, RCard | CardIsSkull, 0 } },
/*  34 */ { 0,1,0, 1,0,0, Door_LockedRaise,				{ 0, D_SLOW, 0, YCard | CardIsSkull, 0 } },
/*  35 */ { 1,0,0, 0,0,0, Light_ChangeToValue,			{ TAG, 35, 0, 0, 0 } },
/*  36 */ { 1,0,0, 0,0,0, Floor_LowerToHighest,			{ TAG, F_FAST, 136, 0, 0 } },
/*  37 */ { 1,0,0, 0,0,0, Floor_LowerToLowestTxTy,		{ TAG, F_SLOW, 0, 0, 0 } },
/*  38 */ { 1,0,0, 0,0,0, Floor_LowerToLowest,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  39 */ { 1,0,0, 1,0,0, Teleport,						{ TAG, 0, 0, 0, 0 } },
/*  40 */ { 1,0,0, 0,0,0, Generic_Ceiling,				{ TAG, C_SLOW, 0, 1, 8 } },
/*  41 */ { 0,1,0, 0,0,0, Ceiling_LowerToFloor,			{ TAG, C_SLOW, 0, 0, 0 } },
/*  42 */ { 0,1,0, 0,0,1, Door_Close,					{ TAG, D_SLOW, 0, 0, 0 } },
/*  43 */ { 0,1,0, 0,0,1, Ceiling_LowerToFloor,			{ TAG, C_SLOW, 0, 0, 0 } },
/*  44 */ { 1,0,0, 0,0,0, Ceiling_LowerAndCrush,		{ TAG, C_SLOW, 0, 0, 0 } },
/*  45 */ { 0,1,0, 0,0,1, Floor_LowerToHighest,			{ TAG, F_SLOW, 128, 0, 0 } },
/*  46 */ { 0,0,1, 1,0,1, Door_Open,					{ TAG, D_SLOW, 0, 0, 0 } },
/*  47 */ { 0,0,1, 0,0,0, Plat_RaiseAndStayTx0,			{ TAG, P_SLOW/2, 0, 0, 0 } },
/*  48 */ { 0,0,0, 0,0,0, Scroll_Texture_Left,			{ SCROLL_UNIT, 0, 0, 0, 0 } },
/*  49 */ { 0,1,0, 0,0,0, Ceiling_CrushAndRaiseA,		{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/*  50 */ { 0,1,0, 0,0,0, Door_Close,					{ TAG, D_SLOW, 0, 0, 0 } },
/*  51 */ { 0,1,0, 0,0,0, Exit_Secret,					{ 0, 0, 0, 0, 0 } },
/*  52 */ { 1,0,0, 0,0,0, Exit_Normal,					{ 0, 0, 0, 0, 0 } },
/*  53 */ { 1,0,0, 0,0,0, Plat_PerpetualRaiseLip,		{ TAG, P_SLOW, PLATWAIT, 0, 0 } },
/*  54 */ { 1,0,0, 0,0,0, Plat_Stop,					{ TAG, 0, 0, 0, 0 } },
/*  55 */ { 0,1,0, 0,0,0, Floor_RaiseAndCrush,			{ TAG, F_SLOW, 10, 0, 0 } },
/*  56 */ { 1,0,0, 0,0,0, Floor_RaiseAndCrush,			{ TAG, F_SLOW, 10, 0, 0 } },
/*  57 */ { 1,0,0, 0,0,0, Ceiling_CrushStop,			{ TAG, 0, 0, 0, 0 } },
/*  58 */ { 1,0,0, 0,0,0, Floor_RaiseByValue,			{ TAG, F_SLOW, 24, 0, 0 } },
/*  59 */ { 1,0,0, 0,0,0, Floor_RaiseByValueTxTy,		{ TAG, F_SLOW, 24, 0, 0 } },
/*  60 */ { 0,1,0, 0,0,1, Floor_LowerToLowest,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  61 */ { 0,1,0, 0,0,1, Door_Open,					{ TAG, D_SLOW, 0, 0, 0 } },
/*  62 */ { 0,1,0, 0,0,1, Plat_DownWaitUpStayLip,		{ TAG, P_FAST, PLATWAIT, 0, 0 } },
/*  63 */ { 0,1,0, 0,0,1, Door_Raise,					{ TAG, D_SLOW, VDOORWAIT, 0, 0 } },
/*  64 */ { 0,1,0, 0,0,1, Floor_RaiseToLowestCeiling,	{ TAG, F_SLOW, 0, 0, 0 } },
/*  65 */ { 0,1,0, 0,0,1, Floor_RaiseAndCrush,			{ TAG, F_SLOW, 10, 0, 0 } },
/*  66 */ { 0,1,0, 0,0,1, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 3, 0, 0 } },
/*  67 */ { 0,1,0, 0,0,1, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 4, 0, 0 } },
/*  68 */ { 0,1,0, 0,0,1, Plat_RaiseAndStayTx0,			{ TAG, P_SLOW/2, 0, 0, 0 } },
/*  69 */ { 0,1,0, 0,0,1, Floor_RaiseToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  70 */ { 0,1,0, 0,0,1, Floor_LowerToHighest,			{ TAG, F_FAST, 136, 0, 0 } },
/*  71 */ { 0,1,0, 0,0,0, Floor_LowerToHighest,			{ TAG, F_FAST, 136, 0, 0 } },
/*  72 */ { 1,0,0, 0,0,1, Ceiling_LowerAndCrush,		{ TAG, C_SLOW, 0, 0, 0 } },
/*  73 */ { 1,0,0, 0,0,1, Ceiling_CrushAndRaiseA,		{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/*  74 */ { 1,0,0, 0,0,1, Ceiling_CrushStop,			{ TAG, 0, 0, 0, 0 } },
/*  75 */ { 1,0,0, 0,0,1, Door_Close,					{ TAG, D_SLOW, 0, 0, 0 } },
/*  76 */ { 1,0,0, 0,0,1, Door_CloseWaitOpen,			{ TAG, D_SLOW, 240, 0, 0 } },
/*  77 */ { 1,0,0, 0,0,1, Ceiling_CrushAndRaiseA,		{ TAG, C_NORMAL, C_NORMAL, 10, 0 } },
/*  78 */ { 0,1,0, 0,0,1, Floor_TransferNumeric,		{ TAG, 0, 0, 0, 0 } },			// <- BOOM special
/*  79 */ { 1,0,0, 0,0,1, Light_ChangeToValue,			{ TAG, 35, 0, 0, 0 } },
/*  80 */ { 1,0,0, 0,0,1, Light_MaxNeighbor,			{ TAG, 0, 0, 0, 0 } },
/*  81 */ { 1,0,0, 0,0,1, Light_ChangeToValue,			{ TAG, 255, 0, 0, 0 } },
/*  82 */ { 1,0,0, 0,0,1, Floor_LowerToLowest,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  83 */ { 1,0,0, 0,0,1, Floor_LowerToHighest,			{ TAG, F_SLOW, 128, 0, 0 } },
/*  84 */ { 1,0,0, 0,0,1, Floor_LowerToLowestTxTy,		{ TAG, F_SLOW, 0, 0, 0 } },
/*  85 */ { 0,0,0, 0,0,0, Scroll_Texture_Right,			{ SCROLL_UNIT, 0, 0, 0, 0 } }, // <- BOOM special
/*  86 */ { 1,0,0, 0,0,1, Door_Open,					{ TAG, D_SLOW, 0, 0, 0 } },
/*  87 */ { 1,0,0, 0,0,1, Plat_PerpetualRaiseLip,		{ TAG, P_SLOW, PLATWAIT, 0, 0 } },
/*  88 */ { 1,0,0, 1,0,1, Plat_DownWaitUpStayLip,		{ TAG, P_FAST, PLATWAIT, 0, 0 } },
/*  89 */ { 1,0,0, 0,0,1, Plat_Stop,					{ TAG, 0, 0, 0, 0 } },
/*  90 */ { 1,0,0, 0,0,1, Door_Raise,					{ TAG, D_SLOW, VDOORWAIT, 0, 0 } },
/*  91 */ { 1,0,0, 0,0,1, Floor_RaiseToLowestCeiling,	{ TAG, F_SLOW, 0, 0, 0 } },
/*  92 */ { 1,0,0, 0,0,1, Floor_RaiseByValue,			{ TAG, F_SLOW, 24, 0, 0 } },
/*  93 */ { 1,0,0, 0,0,1, Floor_RaiseByValueTxTy,		{ TAG, F_SLOW, 24, 0, 0 } },
/*  94 */ { 1,0,0, 0,0,1, Floor_RaiseAndCrush,			{ TAG, F_SLOW, 10, 0, 0 } },
/*  95 */ { 1,0,0, 0,0,1, Plat_RaiseAndStayTx0,			{ TAG, P_SLOW/2, 0, 0, 0 } },
/*  96 */ { 1,0,0, 0,0,1, Floor_RaiseByTexture,			{ TAG, F_SLOW, 0, 0, 0 } },
/*  97 */ { 1,0,0, 1,0,1, Teleport,						{ TAG, 0, 0, 0, 0 } },
/*  98 */ { 1,0,0, 0,0,1, Floor_LowerToHighest,			{ TAG, F_FAST, 136, 0, 0 } },
/*  99 */ { 0,1,0, 0,0,1, Door_LockedRaise,				{ TAG, D_FAST, 0, BCard | CardIsSkull, 0 } },
/* 100 */ { 1,0,0, 0,0,0, Stairs_BuildUpDoom,			{ TAG, S_TURBO, 16, 0, 0 } },
/* 101 */ { 0,1,0, 0,0,0, Floor_RaiseToLowestCeiling,	{ TAG, F_SLOW, 0, 0, 0 } },
/* 102 */ { 0,1,0, 0,0,0, Floor_LowerToHighest,			{ TAG, F_SLOW, 128, 0, 0 } },
/* 103 */ { 0,1,0, 0,0,0, Door_Open,					{ TAG, D_SLOW, 0, 0, 0 } },
/* 104 */ { 1,0,0, 0,0,0, Light_MinNeighbor,			{ TAG, 0, 0, 0, 0 } },
/* 105 */ { 1,0,0, 0,0,1, Door_Raise,					{ TAG, D_FAST, VDOORWAIT, 0, 0 } },
/* 106 */ { 1,0,0, 0,0,1, Door_Open,					{ TAG, D_FAST, 0, 0, 0 } },
/* 107 */ { 1,0,0, 0,0,1, Door_Close,					{ TAG, D_FAST, 0, 0, 0 } },
/* 108 */ { 1,0,0, 0,0,0, Door_Raise,					{ TAG, D_FAST, VDOORWAIT, 0, 0 } },
/* 109 */ { 1,0,0, 0,0,0, Door_Open,					{ TAG, D_FAST, 0, 0, 0 } },
/* 110 */ { 1,0,0, 0,0,0, Door_Close,					{ TAG, D_FAST, 0, 0, 0 } },
/* 111 */ { 0,1,0, 0,0,0, Door_Raise,					{ TAG, D_FAST, VDOORWAIT, 0, 0 } },
/* 112 */ { 0,1,0, 0,0,0, Door_Open,					{ TAG, D_FAST, 0, 0, 0 } },
/* 113 */ { 0,1,0, 0,0,0, Door_Close,					{ TAG, D_FAST, 0, 0, 0 } },
/* 114 */ { 0,1,0, 0,0,1, Door_Raise,					{ TAG, D_FAST, VDOORWAIT, 0, 0 } },
/* 115 */ { 0,1,0, 0,0,1, Door_Open,					{ TAG, D_FAST, 0, 0, 0 } },
/* 116 */ { 0,1,0, 0,0,1, Door_Close,					{ TAG, D_FAST, 0, 0, 0 } },
/* 117 */ { 0,1,0, 0,0,1, Door_Raise,					{ 0, D_FAST, VDOORWAIT, 0, 0 } },
/* 118 */ { 0,1,0, 0,0,0, Door_Open,					{ 0, D_FAST, 0, 0, 0 } },
/* 119 */ { 1,0,0, 0,0,0, Floor_RaiseToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 120 */ { 1,0,0, 0,0,1, Plat_DownWaitUpStayLip,		{ TAG, P_TURBO, PLATWAIT, 0, 0 } },
/* 121 */ { 1,0,0, 0,0,0, Plat_DownWaitUpStayLip,		{ TAG, P_TURBO, PLATWAIT, 0, 0 } },
/* 122 */ { 0,1,0, 0,0,0, Plat_DownWaitUpStayLip,		{ TAG, P_TURBO, PLATWAIT, 0, 0 } },
/* 123 */ { 0,1,0, 0,0,1, Plat_DownWaitUpStayLip,		{ TAG, P_TURBO, PLATWAIT, 0, 0 } },
/* 124 */ { 1,0,0, 0,0,0, Exit_Secret,					{ 0, 0, 0, 0, 0 } },
/* 125 */ { 1,0,0, 1,1,0, Teleport,						{ TAG, 0, 0, 0, 0 } },
/* 126 */ { 1,0,0, 1,1,1, Teleport,						{ TAG, 0, 0, 0, 0 } },
/* 127 */ { 0,1,0, 0,0,0, Stairs_BuildUpDoom,			{ TAG, S_TURBO, 16, 0, 0 } },
/* 128 */ { 1,0,0, 0,0,1, Floor_RaiseToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 129 */ { 1,0,0, 0,0,1, Floor_RaiseToNearest,			{ TAG, F_FAST, 0, 0, 0 } },
/* 130 */ { 1,0,0, 0,0,0, Floor_RaiseToNearest,			{ TAG, F_FAST, 0, 0, 0 } },
/* 131 */ { 0,1,0, 0,0,0, Floor_RaiseToNearest,			{ TAG, F_FAST, 0, 0, 0 } },
/* 132 */ { 0,1,0, 0,0,1, Floor_RaiseToNearest,			{ TAG, F_FAST, 0, 0, 0 } },
/* 133 */ { 0,1,0, 0,0,0, Door_LockedRaise,				{ TAG, D_FAST, 0, BCard | CardIsSkull, 0 } },
/* 134 */ { 0,1,0, 0,0,1, Door_LockedRaise,				{ TAG, D_FAST, 0, RCard | CardIsSkull, 0 } },
/* 135 */ { 0,1,0, 0,0,0, Door_LockedRaise,				{ TAG, D_FAST, 0, RCard | CardIsSkull, 0 } },
/* 136 */ { 0,1,0, 0,0,1, Door_LockedRaise,				{ TAG, D_FAST, 0, YCard | CardIsSkull, 0 } },
/* 137 */ { 0,1,0, 0,0,0, Door_LockedRaise,				{ TAG, D_FAST, 0, YCard | CardIsSkull, 0 } },
/* 138 */ { 0,1,0, 0,0,1, Light_ChangeToValue,			{ TAG, 255, 0, 0, 0 } },
/* 139 */ { 0,1,0, 0,0,1, Light_ChangeToValue,			{ TAG, 35, 0, 0, 0 } },
/* 140 */ { 0,1,0, 0,0,0, Floor_RaiseByValueTimes8,		{ TAG, F_SLOW, 64, 0, 0 } },
/* 141 */ { 1,0,0, 0,0,0, Ceiling_CrushAndRaiseSilentA,	{ TAG, C_SLOW, C_SLOW, 10, 0 } },

/****** The following are all new to BOOM ******/

/* 142 */ { 1,0,0, 0,0,0, Floor_RaiseByValueTimes8,		{ TAG, F_SLOW, 64, 0, 0 } },
/* 143 */ { 1,0,0, 0,0,0, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 3, 0, 0 } },
/* 144 */ { 1,0,0, 0,0,0, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 4, 0, 0 } },
/* 145 */ { 1,0,0, 0,0,0, Ceiling_LowerToFloor,			{ TAG, C_SLOW, 0, 0 } },
/* 146 */ { 1,0,0, 0,0,0, Floor_Donut,					{ TAG, DORATE, DORATE, 0, 0 } },
/* 147 */ { 1,0,0, 0,0,1, Floor_RaiseByValueTimes8,		{ TAG, F_SLOW, 64, 0, 0 } },
/* 148 */ { 1,0,0, 0,0,1, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 3, 0, 0 } },
/* 149 */ { 1,0,0, 0,0,1, Plat_UpByValueStayTx,			{ TAG, P_SLOW/2, 4, 0, 0 } },
/* 150 */ { 1,0,0, 0,0,1, Ceiling_CrushAndRaiseSilentA,	{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/* 151 */ { 1,0,0, 0,0,1, FloorAndCeiling_LowerRaise,	{ TAG, F_SLOW, C_SLOW, 0, 0 } },
/* 152 */ { 1,0,0, 0,0,1, Ceiling_LowerToFloor,			{ TAG, C_SLOW, 0, 0, 0 } },
/* 153 */ { 1,0,0, 0,0,0, Floor_TransferTrigger,		{ TAG, 0, 0, 0, 0 } },
/* 154 */ { 1,0,0, 0,0,1, Floor_TransferTrigger,		{ TAG, 0, 0, 0, 0 } },
/* 155 */ { 1,0,0, 0,0,1, Floor_Donut,					{ TAG, DORATE, DORATE, 0, 0 } },
/* 156 */ { 1,0,0, 0,0,1, Light_StrobeDoom,				{ TAG, 5, 35, 0, 0 } },
/* 157 */ { 1,0,0, 0,0,1, Light_MinNeighbor,			{ TAG, 0, 0, 0, 0 } },
/* 158 */ { 0,1,0, 0,0,0, Floor_RaiseByTexture,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 159 */ { 0,1,0, 0,0,0, Floor_LowerToLowestTxTy,		{ TAG, F_SLOW, 0, 0, 0 } },
/* 160 */ { 0,1,0, 0,0,0, Floor_RaiseByValueTxTy,		{ TAG, F_SLOW, 24, 0, 0 } },
/* 161 */ { 0,1,0, 0,0,0, Floor_RaiseByValue,			{ TAG, F_SLOW, 24, 0, 0 } },
/* 162 */ { 0,1,0, 0,0,0, Plat_PerpetualRaiseLip,		{ TAG, P_SLOW, PLATWAIT, 0, 0 } },
/* 163 */ { 0,1,0, 0,0,0, Plat_Stop,					{ TAG, 0, 0, 0, 0 } },
/* 164 */ { 0,1,0, 0,0,0, Ceiling_CrushAndRaiseA,		{ TAG, C_NORMAL, C_NORMAL, 10, 0 } },
/* 165 */ { 0,1,0, 0,0,0, Ceiling_CrushAndRaiseSilentA,	{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/* 166 */ { 0,1,0, 0,0,0, FloorAndCeiling_LowerRaise,	{ TAG, F_SLOW, C_SLOW, 0, 0 } },
/* 167 */ { 0,1,0, 0,0,0, Ceiling_LowerAndCrush,		{ TAG, C_SLOW, 0, 0, 0 } },
/* 168 */ { 0,1,0, 0,0,0, Ceiling_CrushStop,			{ TAG, 0, 0, 0, 0 } },
/* 169 */ { 0,1,0, 0,0,0, Light_MaxNeighbor,			{ TAG, 0, 0, 0, 0 } },
/* 170 */ { 0,1,0, 0,0,0, Light_ChangeToValue,			{ TAG, 35, 0, 0, 0 } },
/* 171 */ { 0,1,0, 0,0,0, Light_ChangeToValue,			{ TAG, 255, 0, 0, 0 } },
/* 172 */ { 0,1,0, 0,0,0, Light_StrobeDoom,				{ TAG, 5, 35, 0, 0 } },
/* 173 */ { 0,1,0, 0,0,0, Light_MinNeighbor,			{ TAG, 0, 0, 0, 0 } },
/* 174 */ { 0,1,0, 1,0,0, Teleport,						{ TAG, 0, 0, 0, 0 } },
/* 175 */ { 0,1,0, 0,0,0, Door_CloseWaitOpen,			{ TAG, F_SLOW, 240, 0, 0 } },
/* 176 */ { 0,1,0, 0,0,1, Floor_RaiseByTexture,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 177 */ { 0,1,0, 0,0,1, Floor_LowerToLowestTxTy,		{ TAG, F_SLOW, 0, 0, 0 } },
/* 178 */ { 0,1,0, 0,0,1, Floor_RaiseByValueTimes8,		{ TAG, F_SLOW, 64, 0, 0 } },
/* 179 */ { 0,1,0, 0,0,1, Floor_RaiseByValueTxTy,		{ TAG, F_SLOW, 24, 0, 0 } },
/* 180 */ { 0,1,0, 0,0,1, Floor_RaiseByValue,			{ TAG, F_SLOW, 24, 0, 0 } },
/* 181 */ { 0,1,0, 0,0,1, Plat_PerpetualRaiseLip,		{ TAG, P_SLOW, PLATWAIT, 0, 0 } },
/* 182 */ { 0,1,0, 0,0,1, Plat_Stop,					{ TAG, 0, 0, 0, 0 } },
/* 183 */ { 0,1,0, 0,0,1, Ceiling_CrushAndRaiseA,		{ TAG, C_NORMAL, C_NORMAL, 10, 0 } },
/* 184 */ { 0,1,0, 0,0,1, Ceiling_CrushAndRaiseA,		{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/* 185 */ { 0,1,0, 0,0,1, Ceiling_CrushAndRaiseSilentA,	{ TAG, C_SLOW, C_SLOW, 10, 0 } },
/* 186 */ { 0,1,0, 0,0,1, FloorAndCeiling_LowerRaise,	{ TAG, F_SLOW, C_SLOW, 0, 0 } },
/* 187 */ { 0,1,0, 0,0,1, Ceiling_LowerAndCrush,		{ TAG, C_SLOW, 0, 0, 0 } },
/* 188 */ { 0,1,0, 0,0,1, Ceiling_CrushStop,			{ TAG, 0, 0, 0, 0 } },
/* 189 */ { 0,1,0, 0,0,0, Floor_TransferTrigger,		{ TAG, 0, 0, 0, 0 } },
/* 190 */ { 0,1,0, 0,0,1, Floor_TransferTrigger,		{ TAG, 0, 0, 0, 0 } },
/* 191 */ { 0,1,0, 0,0,1, Floor_Donut,					{ TAG, DORATE, DORATE, 0, 0 } },
/* 192 */ { 0,1,0, 0,0,1, Light_MaxNeighbor,			{ TAG, 0, 0, 0, 0 } },
/* 193 */ { 0,1,0, 0,0,1, Light_StrobeDoom,				{ TAG, 5, 35, 0, 0 } },
/* 194 */ { 0,1,0, 0,0,1, Light_MinNeighbor,			{ TAG, 0, 0, 0, 0 } },
/* 195 */ { 0,1,0, 1,0,1, Teleport,						{ TAG, 0, 0, 0, 0 } },
/* 196 */ { 0,1,0, 0,0,1, Door_CloseWaitOpen,			{ TAG, D_SLOW, 240, 0, 0 } },
/* 197 */ { 0,0,1, 0,0,0, Exit_Normal,					{ 0, 0, 0, 0, 0 } },
/* 198 */ { 0,0,1, 0,0,0, Exit_Secret,					{ 0, 0, 0, 0, 0 } },
/* 199 */ { 1,0,0, 0,0,0, Ceiling_LowerToLowest,		{ TAG, C_SLOW, 0, 0, 0 } },
/* 200 */ { 1,0,0, 0,0,0, Ceiling_LowerToHighestFloor,	{ TAG, C_SLOW, 0, 0, 0 } },
/* 201 */ { 1,0,0, 0,0,1, Ceiling_LowerToLowest,		{ TAG, C_SLOW, 0, 0, 0 } },
/* 202 */ { 1,0,0, 0,0,1, Ceiling_LowerToHighestFloor,	{ TAG, C_SLOW, 0, 0, 0 } },
/* 203 */ { 0,1,0, 0,0,0, Ceiling_LowerToLowest,		{ TAG, C_SLOW, 0, 0, 0 } },
/* 204 */ { 0,1,0, 0,0,0, Ceiling_LowerToHighestFloor,	{ TAG, C_SLOW, 0, 0, 0 } },
/* 205 */ { 0,1,0, 0,0,1, Ceiling_LowerToLowest,		{ TAG, C_SLOW, 0, 0, 0 } },
/* 206 */ { 0,1,0, 0,0,1, Ceiling_LowerToHighestFloor,	{ TAG, C_SLOW, 0, 0, 0 } },
/* 207 */ { 1,0,0, 1,0,0, Teleport_NoFog,				{ TAG, 0, 0, 0, 0 } },
/* 208 */ { 1,0,0, 1,0,1, Teleport_NoFog,				{ TAG, 0, 0, 0, 0 } },
/* 209 */ { 0,1,0, 1,0,0, Teleport_NoFog,				{ TAG, 0, 0, 0, 0 } },
/* 210 */ { 0,1,0, 1,0,1, Teleport_NoFog,				{ TAG, 0, 0, 0, 0 } },
/* 211 */ { 0,1,0, 0,0,1, Plat_ToggleCeiling,			{ TAG, 0, 0, 0, 0 } },
/* 212 */ { 1,0,0, 0,0,1, Plat_ToggleCeiling,			{ TAG, 0, 0, 0, 0 } },
/* 213 */ { 0,0,0, 0,0,0, Transfer_FloorLight,			{ TAG, 0, 0, 0, 0 } },
/* 214 */ { 0,0,0, 0,0,0, Scroll_Ceiling,				{ TAG, 6, 0, 0, 0 } },
/* 215 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 6, 0, 0, 0 } },
/* 216 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 6, 1, 0, 0 } },
/* 217 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 6, 2, 0, 0 } },
/* 218 */ { 0,0,0, 0,0,0, Scroll_Texture_Model,			{ TAG, 2, 0, 0, 0 } },
/* 219 */ { 1,0,0, 0,0,0, Floor_LowerToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 220 */ { 1,0,0, 0,0,1, Floor_LowerToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 221 */ { 0,1,0, 0,0,0, Floor_LowerToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 222 */ { 0,1,0, 0,0,1, Floor_LowerToNearest,			{ TAG, F_SLOW, 0, 0, 0 } },
/* 223 */ { 0,0,0, 0,0,0, Sector_SetFriction,			{ TAG, 0, 0, 0, 0 } },
/* 224 */ { 0,0,0, 0,0,0, Sector_SetWind,				{ TAG, 0, 0, 1, 0 } },
/* 225 */ { 0,0,0, 0,0,0, Sector_SetCurrent,			{ TAG, 0, 0, 1, 0 } },
/* 226 */ { 0,0,0, 0,0,0, PointPush_SetForce,			{ TAG, 0, 0, 1, 0 } },
/* 227 */ { 1,0,0, 0,0,0, Elevator_RaiseToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 228 */ { 1,0,0, 0,0,1, Elevator_RaiseToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 229 */ { 0,1,0, 0,0,0, Elevator_RaiseToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 230 */ { 0,1,0, 0,0,1, Elevator_RaiseToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 231 */ { 1,0,0, 0,0,0, Elevator_LowerToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 232 */ { 1,0,0, 0,0,1, Elevator_LowerToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 233 */ { 0,1,0, 0,0,0, Elevator_LowerToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 234 */ { 0,1,0, 0,0,1, Elevator_LowerToNearest,		{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 235 */ { 1,0,0, 0,0,0, Elevator_MoveToFloor,			{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 236 */ { 1,0,0, 0,0,1, Elevator_MoveToFloor,			{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 237 */ { 0,1,0, 0,0,0, Elevator_MoveToFloor,			{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 238 */ { 0,1,0, 0,0,1, Elevator_MoveToFloor,			{ TAG, ELEVATORSPEED, 0, 0, 0 } },
/* 239 */ { 1,0,0, 0,0,0, Floor_TransferNumeric,		{ TAG, 0, 0, 0, 0 } },
/* 240 */ { 1,0,0, 0,0,1, Floor_TransferNumeric,		{ TAG, 0, 0, 0, 0 } },
/* 241 */ { 0,1,0, 0,0,0, Floor_TransferNumeric,		{ TAG, 0, 0, 0, 0 } },
/* 242 */ { 0,0,0, 0,0,0, Transfer_Heights,				{ TAG, 0, 0, 0, 0 } },
/* 243 */ { 1,0,0, 1,0,0, Teleport_Line,				{ TAG, TAG, 0, 0, 0 } },
/* 244 */ { 1,0,0, 1,0,1, Teleport_Line,				{ TAG, TAG, 0, 0, 0 } },
/* 245 */ { 0,0,0, 0,0,0, Scroll_Ceiling,				{ TAG, 5, 0, 0, 0 } },
/* 246 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 5, 0, 0, 0 } },
/* 247 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 5, 1, 0, 0 } },
/* 248 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 5, 2, 0, 0 } },
/* 249 */ { 0,0,0, 0,0,0, Scroll_Texture_Model,			{ TAG, 1, 0, 0, 0 } },
/* 250 */ { 0,0,0, 0,0,0, Scroll_Ceiling,				{ TAG, 4, 0, 0, 0 } },
/* 251 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 4, 0, 0, 0 } },
/* 252 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 4, 1, 0, 0 } },
/* 253 */ { 0,0,0, 0,0,0, Scroll_Floor,					{ TAG, 4, 2, 0, 0 } },
/* 254 */ { 0,0,0, 0,0,0, Scroll_Texture_Model,			{ TAG, 0, 0, 0, 0 } },
/* 255 */ { 0,0,0, 0,0,0, Scroll_Texture_Offsets,		{ 0, 0, 0, 0, 0 } },
/* 256 */ { 1,0,0, 0,0,1, Stairs_BuildUpDoom,			{ TAG, S_SLOW, 8, 0, 0 } },
/* 257 */ { 1,0,0, 0,0,1, Stairs_BuildUpDoom,			{ TAG, S_TURBO, 16, 0, 0 } },
/* 258 */ { 0,1,0, 0,0,1, Stairs_BuildUpDoom,			{ TAG, S_SLOW, 8, 0, 0 } },
/* 259 */ { 0,1,0, 0,0,1, Stairs_BuildUpDoom,			{ TAG, S_TURBO, 16, 0, 0 } },
/* 260 */ { 0,0,0, 0,0,0, TranslucentLine,				{ TAG, 128, 0, 0, 0 } },
/* 261 */ { 0,0,0, 0,0,0, Transfer_CeilingLight,		{ TAG, 0, 0, 0, 0 } },
/* 262 */ { 1,0,0, 1,0,0, Teleport_Line,				{ TAG, TAG, 1, 0, 0 } },
/* 263 */ { 1,0,0, 1,0,1, Teleport_Line,				{ TAG, TAG, 1, 0, 0 } },
/* 264 */ { 1,0,0, 1,1,0, Teleport_Line,				{ TAG, TAG, 1, 0, 0 } },
/* 265 */ { 1,0,0, 1,1,1, Teleport_Line,				{ TAG, TAG, 1, 0, 0 } },
/* 266 */ { 1,0,0, 1,1,0, Teleport_Line,				{ TAG, TAG, 0, 0, 0 } },
/* 267 */ { 1,0,0, 1,1,1, Teleport_Line,				{ TAG, TAG, 0, 0, 0 } },
/* 268 */ { 1,0,0, 1,1,0, Teleport_NoFog,				{ TAG, 0, 0, 0, 0 } },
/* 269 */ { 1,0,0, 1,1,1, Teleport_NoFog,				{ TAG, 0, 0, 0, 0 } },
/* 270 */ { 0,0,0, 0,0,0, 0,							{ 0, 0, 0, 0, 0 } },
/* 271 */ { 0,0,0, 0,0,0, Static_Init,					{ TAG, Init_TransferSky, 0, 0, 0 } },
/* 272 */ { 0,0,0, 0,0,0, Static_Init,					{ TAG, Init_TransferSky, 1, 0, 0 } }
};
#define NUM_SPECIALS 272

void P_TranslateLineDef (line_t *ld, maplinedef_t *mld)
{
	short special = SHORT(mld->special);
	short tag = SHORT(mld->tag);
	short flags = SHORT(mld->flags);
	BOOL passthrough;
	int i;

	passthrough = (flags & ML_PASSUSE_BOOM); // denis - fixme

	flags = flags & 0x01ff;	// Ignore flags unknown to DOOM

	if (special <= NUM_SPECIALS)
	{
		// This is a regular special; translate thru LUT
		xlat_t trans = SpecialTranslation[special];

		if(trans.cross)
			flags |= ML_SPECIAL_CROSS;
		if(trans.allow_monster)
			flags |= ML_SPECIAL_MONSTER;
		if(trans.allow_monster_only)
			flags |= ML_SPECIAL_MONSTER_ONLY;
		if(trans.use)
			flags |= ML_SPECIAL_USE;
		if(trans.shoot)
			flags |= ML_SPECIAL_SHOOT;
		if(trans.allow_repeat)
			flags |= ML_SPECIAL_REPEAT;

		ld->special = SpecialTranslation[special].newspecial;
		for (i = 0; i < 5; i++)
			ld->args[i] = SpecialTranslation[special].args[i] == TAG ? tag :
						  SpecialTranslation[special].args[i];
	}
	else if (special <= GenCrusherBase)
	{
		if (special >= OdamexStaticInits && special < OdamexStaticInits + NUM_STATIC_INITS)
		{
			// A ZDoom Static_Init special
			ld->special = Static_Init;
			ld->args[0] = tag;
			ld->args[1] = special - OdamexStaticInits;
		}
		else
		{
			// This is an unknown special. Just zero it.
			ld->special = 0;
			memset (ld->args, 0, sizeof(ld->args));
		}
	}
	else
	{
		// Anything else is a BOOM generalized linedef type
		// denis - todo - fixme - wrecked by license
		switch (special & 0x0007)
		{
		case WalkMany:
			flags |= ML_SPECIAL_REPEAT;
		case WalkOnce:
			flags |= ML_SPECIAL_USE;
			break;

		case SwitchMany:
		case PushMany:
			flags |= ML_SPECIAL_REPEAT;
		case SwitchOnce:
		case PushOnce:
			if (passthrough)
				flags |= ML_SPECIAL_USE;
			else
				flags |= ML_SPECIAL_USE;
			break;

		case GunMany:
			flags |= ML_SPECIAL_REPEAT;
		case GunOnce:
			flags |= ML_SPECIAL_SHOOT;
			break;
		}

		// We treat push triggers like switch triggers with zero tags.
		if ((special & 0x0007) == PushMany ||
			(special & 0x0007) == PushOnce)
			ld->args[0] = 0;
		else
			ld->args[0] = tag;

		if (special <= GenStairsBase)
		{
			// Generalized crusher (tag, dnspeed, upspeed, silent, damage)
			ld->special = Generic_Crusher;
			if (special & 0x0020)
				flags |= ML_SPECIAL_MONSTER;
			switch (special & 0x0018) {
				case 0x0000:	ld->args[1] = C_SLOW;	break;
				case 0x0008:	ld->args[1] = C_NORMAL;	break;
				case 0x0010:	ld->args[1] = C_FAST;	break;
				case 0x0018:	ld->args[1] = C_TURBO;	break;
			}
			ld->args[2] = ld->args[1];
			ld->args[3] = (special & 0x0040) >> 6;
			ld->args[4] = 10;
		}
		else if (special <= GenLiftBase)
		{
			// Generalized stairs (tag, speed, step, dir/igntxt, reset)
			ld->special = Generic_Stairs;
			if (special & 0x0020)
				flags |= ML_SPECIAL_MONSTER;
			switch (special & 0x0018)
			{
				case 0x0000:	ld->args[1] = S_SLOW;	break;
				case 0x0008:	ld->args[1] = S_NORMAL;	break;
				case 0x0010:	ld->args[1] = S_FAST;	break;
				case 0x0018:	ld->args[1] = S_TURBO;	break;
			}
			switch (special & 0x00c0)
			{
				case 0x0000:	ld->args[2] = 4;		break;
				case 0x0040:	ld->args[2] = 8;		break;
				case 0x0080:	ld->args[2] = 16;		break;
				case 0x00c0:	ld->args[2] = 24;		break;
			}
			ld->args[3] = (special & 0x0300) >> 8;
			ld->args[4] = 0;
		}
		else if (special <= GenLockedBase)
		{
			// Generalized lift (tag, speed, delay, target, height)
			ld->special = Generic_Lift;
			if (special & 0x0020)
				flags |= ML_SPECIAL_MONSTER;
			switch (special & 0x0018)
			{
				case 0x0000:	ld->args[1] = P_SLOW*2;		break;
				case 0x0008:	ld->args[1] = P_NORMAL*2;	break;
				case 0x0010:	ld->args[1] = P_FAST*2;		break;
				case 0x0018:	ld->args[1] = P_TURBO*2;	break;
			}
			switch (special & 0x00c0)
			{
				case 0x0000:	ld->args[2] = 8;		break;
				case 0x0040:	ld->args[2] = 24;		break;
				case 0x0080:	ld->args[2] = 40;		break;
				case 0x00c0:	ld->args[2] = 80;		break;
			}
			ld->args[3] = ((special & 0x0300) >> 8) + 1;
			ld->args[4] = 0;
		}
		else if (special <= GenDoorBase)
		{
			// Generalized locked door (tag, speed, kind, delay, lock)
			ld->special = Generic_Door;
			if (special & 0x0080)
				flags |= ML_SPECIAL_MONSTER;
			switch (special & 0x0018)
			{
				case 0x0000:	ld->args[1] = D_SLOW;	break;
				case 0x0008:	ld->args[1] = D_NORMAL;	break;
				case 0x0010:	ld->args[1] = D_FAST;	break;
				case 0x0018:	ld->args[1] = D_TURBO;	break;
			}
			ld->args[2] = (special & 0x0020) >> 5;
			ld->args[3] = (special & 0x0020) ? 0 : 34;
			ld->args[4] = (special & 0x01c0) >> 6;
			if (ld->args[4] == 0)
				ld->args[4] = AnyKey;
			else if (ld->args[4] == 7)
				ld->args[4] = AllKeys;
			ld->args[4] |= (special & 0x0200) >> 2;
		}
		else if (special <= GenCeilingBase)
		{
			// Generalized door (tag, speed, kind, delay, lock)
			ld->special = Generic_Door;
			switch (special & 0x0018)
			{
				case 0x0000:	ld->args[1] = D_SLOW;	break;
				case 0x0008:	ld->args[1] = D_NORMAL;	break;
				case 0x0010:	ld->args[1] = D_FAST;	break;
				case 0x0018:	ld->args[1] = D_TURBO;	break;
			}
			ld->args[2] = (special & 0x0060) >> 5;
			switch (special & 0x0300)
			{
				case 0x0000:	ld->args[3] = 8;		break;
				case 0x0100:	ld->args[3] = 32;		break;
				case 0x0200:	ld->args[3] = 72;		break;
				case 0x0300:	ld->args[3] = 240;		break;
			}
			ld->args[4] = 0;
		}
		else
		{
			// Generalized ceiling (tag, speed, height, target, change/model/direct/crush)
			// Generalized floor (tag, speed, height, target, change/model/direct/crush)
			if (special <= GenFloorBase)
				ld->special = Generic_Ceiling;
			else
				ld->special = Generic_Floor;
			
			switch (special & 0x0018)
			{
				case 0x0000:	ld->args[1] = F_SLOW;	break;
				case 0x0008:	ld->args[1] = F_NORMAL;	break;
				case 0x0010:	ld->args[1] = F_FAST;	break;
				case 0x0018:	ld->args[1] = F_TURBO;	break;
			}
			ld->args[3] = ((special & 0x0380) >> 7) + 1;
			if (ld->args[3] >= 7)
			{
				ld->args[2] = 24 + (ld->args[3] - 7) * 8;
				ld->args[3] = 0;
			}
			else
			{
				ld->args[2] = 0;
			}
			ld->args[4] = ((special & 0x0c00) >> 10) |
						  ((special & 0x0060) >> 3) |
						  ((special & 0x1000) >> 8);
		}
	}

	ld->flags = flags;

	// For purposes of maintaining BOOM compatibility, each
	// line also needs to have its ID set to the same as its tag.
	// An external conversion program would need to do this more
	// intelligently.

	ld->id = tag;
}

// The teleport specials that use things as destinations also require
// that their TIDs be set to the tags of their containing sectors. We
// do that after the rest of the level has been loaded.

void P_TranslateTeleportThings (void)
{
	int i, j;

	for (i = 0; i < numlines; i++)
	{
		if (lines[i].special == Teleport ||
			lines[i].special == Teleport_NoFog)
		{
			// The sector tag hash table hasn't been set up yet,
			// so we need to use this linear search.
			for (j = 0; j < numsectors; j++)
			{
				if (sectors[j].tag == lines[i].args[0])
				{
					AActor *other;
					TThinkerIterator<AActor> iterator;

					while ( (other = iterator.Next ()) )
					{
						// not a teleportman
						if (other->type != MT_TELEPORTMAN && other->type != MT_TELEPORTMAN2)
							continue;

						// wrong sector
						if (other->subsector->sector - sectors != j)
							continue;

						// It's a teleportman, so set it's tid to match
						// the sector's tag.
						other->tid = lines[i].args[0];
						other->AddToHash ();

						// We only bother with the first teleportman
						break;
					}
					if (other)
						break;
				}
			}
		}
	}
}

int P_TranslateSectorSpecial (int special)
{
	// This supports phased lighting with specials 21-24
	return (special == 9) ? SECRET_MASK :
			((special & 0xfe0) << 3) |
			((special & 0x01f) + (((special & 0x1f) < 21) ? 64 : -20));
}

VERSION_CONTROL (p_xlat_cpp, "$Id$")

