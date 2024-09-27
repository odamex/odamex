// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	 Clientside function stubs
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include <stdarg.h>

#include "actor.h"
#include "cmdlib.h"
#include "p_ctf.h"
#include "d_player.h"

// Unnatural Level Progression.  True if we've used 'map' or another command
// to switch to a specific map out of order, otherwise false.
bool unnatural_level_progression;

FORMAT_PRINTF(2, 3)
void STACK_ARGS SV_BroadcastPrintf(int printlevel, const char* format, ...)
{
	if (!serverside)
		return;

	// Local game, print the message normally.
	std::string str;
	va_list va;
	va_start(va, format);
	VStrFormat(str, format, va);
	va_end(va);

	Printf(printlevel, "%s", str.c_str());
}

FORMAT_PRINTF(1, 2)
void STACK_ARGS SV_BroadcastPrintf(const char* format, ...)
{
	if (!serverside)
		return;

	// Local game, print the message normally.
	std::string str;
	va_list va;
	va_start(va, format);
	VStrFormat(str, format, va);
	va_end(va);

	Printf(PRINT_HIGH, "%s", str.c_str());
}

void D_SendServerInfoChange(const cvar_t *cvar, const char *value) {}
void D_DoServerInfoChange(byte **stream) {}
void D_WriteUserInfoStrings(int i, byte **stream, bool compact) {}
void D_ReadUserInfoStrings(int i, byte **stream, bool update) {}

void SV_SpawnMobj(AActor *mobj) {}
void SV_TouchSpecial(AActor *special, player_t *player) {}
ItemEquipVal SV_FlagTouch (player_t &player, team_t f, bool firstgrab) { return IEV_NotEquipped; }
void SV_SocketTouch (player_t &player, team_t f) {}
void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill) {}
void SV_SendDamagePlayer(player_t *player, AActor* inflictor, int healthDamage, int armorDamage) {}
void SV_SendDamageMobj(AActor *target, int pain) {}
void SV_CTFEvent(team_t f, flag_score_t event, player_t &who) {}
void SV_UpdateFrags(player_t &player) {}
void SV_ActorTarget(AActor *actor) {}
void SV_SendDestroyActor(AActor *mo) {}
void SV_ExplodeMissile(AActor *mo) {}
void SV_SendPlayerInfo(player_t &player) {}
void SV_PreservePlayer(player_t &player) {}
void SV_BroadcastSector(int sectornum) {}
void SV_UpdateMobj(AActor* mo) {}
void SV_UpdateMobjState(AActor* mo) {}

void CTF_RememberFlagPos(mapthing2_t *mthing) {}
void CTF_SpawnFlag(team_t f) {}
bool SV_AwarenessUpdate(player_t &pl, AActor* mo) { return true; }
void SV_SendPackets(void) {}
void SV_SendExecuteLineSpecial(byte special, line_t* line, AActor* activator, int arg0,
                               int arg1, int arg2, int arg3, int arg4)
{
}

void SV_UpdateMonsterRespawnCount() {}
void SV_Sound(AActor* mo, byte channel, const char* name, byte attenuation) {}


CVAR_FUNC_IMPL(sv_sharekeys) {}

VERSION_CONTROL (cl_stubs_cpp, "$Id$")
