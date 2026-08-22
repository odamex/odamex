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
//	Cheat code checking.
//
//-----------------------------------------------------------------------------


#pragma once

#include "d_player.h"

//
// CHEAT TYPES
//
enum CheatEnum
{
	CHT_GOD = 0,
	CHT_NOCLIP,
	CHT_NOTARGET,
	CHT_CHAINSAW,
	CHT_IDKFA,
	CHT_IDFA,
	CHT_BEHOLDV,
	CHT_BEHOLDS,
	CHT_BEHOLDI,
	CHT_BEHOLDR,
	CHT_BEHOLDA,
	CHT_BEHOLDL,
	CHT_IDDQD, // Same as CHT_GOD but sets health
	CHT_MASSACRE,
	CHT_CHASECAM,
	CHT_FLY,
	CHT_BUDDHA,
	CHT_MDK,	// He has revolver eyes...
};

// [RH] Functions that actually perform the cheating
namespace cheat
{
void DoCheat(player_t& player, int cheat, bool silentmsg = false);
void GiveTo(player_t& player, const char *item);
AActor* Summon(player_t& player, const std::string& sum, bool friendly);
bool ValidSummonActor(const std::string& summon);

// Heretic code (unused)
#if 0
void Suicide (player_t& player);
#endif

bool AreCheatsEnabled();
}
