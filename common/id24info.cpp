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

constexpr std::array id24mobjinfo = std::to_array<mobjinfo_t>({
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

// std::span<const state_t> getID24States() {
// 	return id24states;
// }

std::span<const char* const> getI24SprNames() {
	return id24sprnames;
}


VERSION_CONTROL (info_cpp, "$Id$")
