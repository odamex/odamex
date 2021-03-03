// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2021 by Alex Mayfield.
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
//   Server message functions.
//   - Functions should send exactly one message.
//   - Functions should be named after the message they send.
//   - Functions should be self-contained and not rely on global state.
//   - Functions should take a buf_t& as a first parameter.
//
//-----------------------------------------------------------------------------

#ifndef __SV_MESSAGE_H__
#define __SV_MESSAGE_H__

#include <string>
#include <vector>

#include "r_state.h"

#include "actor.h"
#include "d_player.h"
#include "g_level.h"
#include "g_levelstate.h"
#include "i_net.h"
#include "p_spec.h"

void SVC_Disconnect(buf_t& b, const char* message = NULL);
void SVC_PlayerInfo(buf_t& b, player_t& player);
void SVC_MovePlayer(buf_t& b, player_t& player, const int tic);
void SVC_UpdateLocalPlayer(buf_t& b, AActor& mo, const int tic);
void SVC_LevelLocals(buf_t& b, const level_locals_t& locals, uint32_t flags);
void SVC_PingRequest(buf_t& b);
void SVC_UpdatePing(buf_t& b, player_t& player);
void SVC_SpawnMobj(buf_t& b, AActor* mo);
void SVC_DisconnectClient(buf_t& b, player_t& player);
void SVC_LoadMap(buf_t& b, const OResFiles& wadnames, const OResFiles& patchnames,
                 const std::string& mapname, int time);
void SVC_ConsolePlayer(buf_t& b, player_t& player, const std::string& digest);
void SVC_UpdateMobj(buf_t& b, AActor& mobj, uint32_t flags);
void SVC_KillMobj(buf_t& b, AActor* source, AActor* target, AActor* inflictor, int mod,
                  bool joinkill);
void SVC_PlayerMembers(buf_t& b, player_t& player, byte flags);
void SVC_TeamMembers(buf_t& b, team_t team);
void SVC_ActivateLine(buf_t& b, line_t* line, AActor* mo, int side,
                      LineActivationType type);
void SVC_MovingSector(buf_t& b, const sector_t& sector);
void SVC_PlayerState(buf_t& b, player_t& player);
void SVC_LevelState(buf_t& b, const SerializedLevelState& sls);
void SVC_SecretFound(buf_t& b, int playerid, int sectornum);
void SVC_SectorProperties(buf_t& b, sector_t& sector);
void SVC_ExecuteLineSpecial(buf_t& b, byte special, line_t* line, AActor* mo,
                            const int (&args)[5]);
void SVC_ThinkerUpdate(buf_t& b, DThinker* thinker);
void SVC_NetdemoCap(buf_t& b, player_t* player);

#endif // __SV_MESSAGE_H__
