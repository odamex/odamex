// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2024 by Lexi Mayfield.
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
//   Client message functions.
//   - Functions should send exactly one message.
//   - Functions should be named after the message they send.
//   - Functions should be self-contained and not rely on global state.
//   - Functions should take a buf_t& as a first parameter.
//
//-----------------------------------------------------------------------------

#pragma once

#include "client.pb.h"

#include "c_maplist.h"
#include "c_vote.h"
#include "d_netcmd.h"

odaproto::clc::Say CLC_Say(byte who, const std::string& message);
odaproto::clc::Move CLC_Move(int tic, NetCommand* cmds, size_t cmds_len);
odaproto::clc::ClientUserInfo CLC_UserInfo(const UserInfo& info);
odaproto::clc::PingReply CLC_PingReply(uint64_t ms_time);
odaproto::clc::Ack CLC_Ack(uint32_t recent, uint32_t ackBits);
odaproto::clc::RCon CLC_RCon(const std::string& command);
odaproto::clc::RConPassword CLC_RConPasswordLogin(const std::string& password,
                                                  const std::string& hash);
odaproto::clc::RConPassword CLC_RConPasswordLogout();
odaproto::clc::Spectate CLC_Spectate(bool enabled);
odaproto::clc::Spectate CLC_SpectateUpdate(const AActor* mobj);
odaproto::clc::Cheat CLC_CheatNumber(int number);
odaproto::clc::Cheat CLC_CheatGiveTo(const std::string& item);
odaproto::clc::MapList CLC_MapList(maplist_status_t status);
odaproto::clc::CallVote CLC_CallVote(vote_type_t vote_type, const char* const* args,
                                     size_t len);
odaproto::clc::NetCmd CLC_NetCmd(const char* const* args, size_t len);
odaproto::clc::Spy CLC_Spy(byte pid);
odaproto::clc::PrivMsg CLC_PrivMsg(byte pid, const std::string& str);
