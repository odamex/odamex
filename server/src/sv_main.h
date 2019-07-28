// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	SV_MAIN
//
//-----------------------------------------------------------------------------


#ifndef __I_SVMAIN_H__
#define __I_SVMAIN_H__

#include <string>

#include "actor.h"
#include "d_player.h"
#include "i_net.h"

#define GAME_NORMAL		1
#define GAME_TEAMPLAY	2
#define GAME_CTF		3

#include <json/json.h>

extern int shotclock;

class client_c
{
public:

	size_t size() { return players.size(); }
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
void STACK_ARGS SV_BroadcastButPlayerPrintf(int level, int player_id, const char *fmt, ...);
void STACK_ARGS SV_ClientPrintf (client_t *cl, int level, const char *fmt, ...);
void STACK_ARGS SV_SpectatorPrintf (int level, const char *fmt, ...);
void STACK_ARGS SV_PlayerPrintf (int level, int who, const char *fmt, ...);
void SV_CheckTimeouts (void);
void SV_ConnectClient(void);
void SV_WriteCommands(void);
void SV_ClearClientsBPS(void);
bool SV_SendPacket(player_t &pl);
void SV_AcknowledgePacket(player_t &player);
void SV_DisplayTics();
void SV_RunTics();
void SV_ParseCommands(player_t &player);
short SV_FindClientByAddr(void);
void SV_UpdateFrags (player_t &player);
void SV_RemoveCorpses (void);
void SV_DropClient(player_t &who);
void SV_PlayerTriedToCheat(player_t &player);
void SV_ActorTarget(AActor *actor);
void SV_ActorTracer(AActor *actor);
void SV_ForceSetTeam(player_t &who, team_t team);
void SV_CheckTeam(player_t &player);
void SV_SendUserInfo(player_t &player, client_t* cl);
void SV_Suicide(player_t &player);
void SV_SpawnMobj(AActor *mo);
void SV_TouchSpecial(AActor *special, player_t *player);

void SV_Sound (AActor *mo, byte channel, const char *name, byte attenuation);
void SV_Sound (client_t *cl, AActor *mo, byte channel, const char *name, byte attenuation);
void SV_Sound (fixed_t x, fixed_t y, byte channel, const char *name, byte attenuation);
void SV_SoundTeam (byte channel, const char* name, byte attenuation, int t);

void SV_MidPrint (const char *msg, player_t *p, int msgtime=0);

int SV_CountTeamPlayers(int team);

extern std::vector<std::string> wadnames;
void MSG_WriteMarker (buf_t *b, svc_t c);

void SV_SendKillMobj(AActor *source, AActor *target, AActor *inflictor, bool joinkill);
void SV_SendDamagePlayer(player_t *player, int pain);
void SV_SendDamageMobj(AActor *target, int pain);
// Tells clients to remove an actor from the world as it doesn't exist anymore
void SV_SendDestroyActor(AActor *mo);

bool M_ReadJSON(Json::Value &json, const char *filename);
bool M_WriteJSON(const char *filename, Json::Value &value, bool styled);

// [AM] Coinflip
void CMD_CoinFlip(std::string &result);

// [AM] Spectating and Kicking
bool CMD_KickCheck(std::vector<std::string> arguments, std::string &error,
				   size_t &pid, std::string &reason);
void SV_KickPlayer(player_t &player, const std::string &reason = "");
bool CMD_ForcespecCheck(const std::vector<std::string> &arguments,
						std::string &error, size_t &pid);
void SV_SetPlayerSpec(player_t &player, bool setting, bool silent = false);
void SV_SetReady(player_t &player, bool setting, bool silent = false);

void SV_SendLoadMap(const std::vector<std::string> &wadnames,
                    const std::vector<std::string> &patchnames,
                    const std::string &mapname, player_t *player);




extern bool unnatural_level_progression;

#endif
