// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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

#ifndef __SV_MSG_H__
#define __SV_MSG_H__

#include <string>
#include <vector>

#include "actor.h"
#include "d_player.h"
#include "g_level.h"
#include "g_levelstate.h"
#include "i_net.h"

void SVC_PlayerInfo(buf_t& b, player_t& player);
void SVC_LevelLocals(buf_t& b, const level_locals_t& locals, byte flags);
void SVC_LoadMap(buf_t& b, const std::vector<std::string>& wadnames,
                 const std::vector<std::string>& wadhashes,
                 const std::vector<std::string>& patchnames,
                 const std::vector<std::string>& patchhashes, const std::string& mapname,
                 int time);
void SVC_KillMobj(buf_t& b, AActor* source, AActor* target, AActor* inflictor, int mod,
                  bool joinkill);
void SVC_PlayerState(buf_t& b, player_t& player);
void SVC_LevelState(buf_t& b, const SerializedLevelState& sls);

#endif
