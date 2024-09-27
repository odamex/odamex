// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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
//	 CTF Implementation
//
//-----------------------------------------------------------------------------

#pragma once

#include "d_netinf.h"
#include "p_local.h"

// events
enum flag_score_t
{
	SCORE_NONE,
	SCORE_REFRESH,
	SCORE_KILL,
	SCORE_BETRAYAL,
	SCORE_GRAB,
	SCORE_FIRSTGRAB,
	SCORE_CARRIERKILL,
	SCORE_RETURN,
	SCORE_CAPTURE,
	SCORE_DROP,
	SCORE_MANUALRETURN,
	NUM_CTF_SCORE
};

//	Network Events
// [CG] I'm aware having CL_* and SV_* functions in common/ is not great, I'll
//      do more work on CTF and team-related things later.
void SV_CTFEvent(team_t f, flag_score_t event, player_t &who);
ItemEquipVal SV_FlagTouch(player_t &player, team_t f, bool firstgrab);
void SV_SocketTouch(player_t &player, team_t f);
void CTF_Connect(player_t &player);

//	Internal Events
void CTF_DrawHud(void);
void CTF_CarryFlag(player_t &who, team_t flag);
void CTF_MoveFlags(void);
void CTF_RunTics(void);
void CTF_SpawnFlag(team_t f);
void CTF_SpawnDroppedFlag(team_t f, int x, int y, int z);
void CTF_RememberFlagPos(mapthing2_t *mthing);
void CTF_CheckFlags(player_t &player);
void CTF_Sound(team_t f, team_t t, flag_score_t event);
void CTF_Message(team_t f, team_t t, flag_score_t event);
bool CTF_ShouldSpawnHomeFlag(mobjtype_t type);
void CTF_ReplaceFlagWithWaypoint(AActor* mo);

FArchive &operator<< (FArchive &arc, flagdata &flag);
FArchive &operator>> (FArchive &arc, flagdata &flag);
