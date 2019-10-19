// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2015 by The Odamex Team.
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
//	 Clientside function stubs
//
//-----------------------------------------------------------------------------

#include "actor.h"
#include "c_cvars.h"
#include "p_ctf.h"
#include "doomdef.h"
#include "d_player.h"

// Unnatural Level Progression.  True if we've used 'map' or another command
// to switch to a specific map out of order, otherwise false.
bool unnatural_level_progression;

void STACK_ARGS SV_BroadcastPrintf(int level, const char *fmt, ...) {}
void STACK_ARGS SV_BroadcastButPlayerPrintf(int level, int player_id, const char *fmt, ...);

void D_SendServerInfoChange(const cvar_t *cvar, const char *value) {}
void D_DoServerInfoChange(byte **stream) {}
void D_WriteUserInfoStrings(int i, byte **stream, bool compact) {}
void D_ReadUserInfoStrings(int i, byte **stream, bool update) {}

void ClientObituary (AActor *self, AActor *inflictor, AActor *attacker) {}

void SV_SpawnMobj(AActor *mobj) {}
void SV_TouchSpecial(AActor *special, player_t *player) {}
ItemEquipVal SV_FlagTouch (player_t &player, flag_t f, bool firstgrab) { return IEV_NotEquipped; }
void SV_SocketTouch (player_t &player, flag_t f) {}
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill) {}
void SV_SendDamagePlayer(player_t *player, int pain) {}
void SV_SendDamageMobj(AActor *target, int pain) {}
void SV_CTFEvent(flag_t f, flag_score_t event, player_t &who) {}
void SV_UpdateFrags(player_t &player) {}
void SV_ActorTarget(AActor *actor) {}
void SV_SendDestroyActor(AActor *mo) {}
void SV_ExplodeMissile(AActor *mo) {}
void SV_SendPlayerInfo(player_t &player) {}
void SV_PreservePlayer(player_t &player) {}
void SV_UpdateMobjState(AActor *mo) {}
void SV_BroadcastSector(int sectornum) {}

void CTF_RememberFlagPos(mapthing2_t *mthing) {}
void CTF_SpawnFlag(flag_t f) {}
bool SV_AwarenessUpdate(player_t &pl, AActor* mo) { return true; }
void SV_SendPackets(void) {}

VERSION_CONTROL (cl_stubs_cpp, "$Id$")

