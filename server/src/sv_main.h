// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	SV_MAIN
//
//-----------------------------------------------------------------------------


#ifndef __I_SVMAIN_H__
#define __I_SVMAIN_H__

#include <string>

#include "actor.h"
#include "d_player.h"
#include "i_net.h"


#define REMOVECOPRSESTIC TICRATE*80

#define GAME_NORMAL		1
#define GAME_TEAMPLAY	2
#define GAME_CTF		3

extern int shotclock;
extern player_t *WinPlayer;

extern bool	unnatural_level_progression;

class client_c
{
public:

	size_t size() { return players.size(); }
	player_s::client_t &operator [](size_t n) { return players[n].client; }
	client_c() {}
};

extern client_c clients;

void SV_InitNetwork (void);
void SV_SendDisconnectSignal();
void SV_SendReconnectSignal();
void SV_ExitLevel();
void SV_DrawScores();

bool SV_IsPlayerAllowedToSee(player_t &pl, AActor *mobj);
byte SV_PlayerHearingLoss(player_t &cl, fixed_t &x, fixed_t &y);

void STACK_ARGS SV_BroadcastPrintf (int level, const char *fmt, ...);
void STACK_ARGS SV_SpectatorPrintf (int level, const char *fmt, ...);
void SV_CheckTimeouts (void);
void SV_ConnectClient(void);
void SV_WriteCommands(void);
void SV_ClearClientsBPS(void);
bool SV_SendPacket(player_t &pl);
void SV_AcknowledgePacket(player_t &player);
void SV_RunTics (void);
void SV_ParseCommands(player_t &player);
short SV_FindClientByAddr(void);
void SV_UpdateFrags (player_t &player);
void SV_RemoveCorpses (void);
void SV_DropClient(player_t &who);
void SV_PlayerTriedToCheat(player_t &player);
void SV_ActorTarget(AActor *actor);
void SV_ActorTracer(AActor *actor);
void SV_Suicide(player_t &player);
void SV_SpawnMobj(AActor *mo);

void SV_Sound (AActor *mo, byte channel, const char *name, byte attenuation);
void SV_Sound (client_t *cl, AActor *mo, byte channel, const char *name, byte attenuation);
void SV_Sound (fixed_t x, fixed_t y, byte channel, const char *name, byte attenuation);
void SV_SoundTeam (byte channel, const char* name, byte attenuation, int t);
void SV_SoundAvoidCl (player_t &player, AActor *mo, byte channel, char *name, byte attenuation);

void SV_MidPrint (const char *msg, player_t *p);

int SV_CountTeamPlayers(int team);

extern std::vector<std::string> wadnames;

EXTERN_CVAR(playeringamelimit)

#endif

