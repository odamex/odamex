// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  CLC Message packers.
//
//-----------------------------------------------------------------------------

#pragma once

#include "client.pb.h"

class player_t;
struct UserInfo;
class OdaMessenger;

void CLC_PackPlayerInputMessageFromPlayer(      odaproto::clc::PlayerInput& msg, player_t& player, int clientTic, int clientWorldIndex);
void CLC_UnpackPlayerInputMessageToPlayer(const odaproto::clc::PlayerInput& msg, player_t& player);

odaproto::clc::NetdemoCap CLC_NetdemoCap(const player_t&                    player,
                                         const odaproto::clc::PlayerInput&  inputMessage,
                                         const OdaMessenger&                playerMessenger);

odaproto::clc::DisconnectMe   CLC_DisconnectMe   ();
odaproto::clc::Say            CLC_Say            (const std::string_view& text, uint32_t visibility);
odaproto::clc::UserInfo       CLC_UserInfo       (const UserInfo& userInfo);
odaproto::clc::PingReply      CLC_PingReply      (uint64_t msec);
odaproto::clc::Kill           CLC_Kill           ();
odaproto::clc::GetPlayerInfo  CLC_GetPlayerInfo  ();
odaproto::clc::Spy            CLC_Spy            (uint32_t playerId);
odaproto::clc::PrivMsg        CLC_PrivMsg        (uint32_t playerId, const std::string_view& text);
odaproto::clc::SendMobjUpdate CLC_SendMobjUpdate (uint32_t netId);

template <typename IteratorType>
odaproto::clc::Netcmd CLC_Netcmd(IteratorType begin, IteratorType end)
{
    odaproto::clc::Netcmd msg;
    while (begin != end)
    {
        PrintFmt("{}\n", *begin);
        msg.add_argv(*begin++);
    }
    return msg;
}
odaproto::clc::Netcmd CLC_Netcmd(const std::string& arg);

odaproto::clc::Rcon         CLC_Rcon        (const std::string_view& text);
odaproto::clc::RconPassword CLC_RconPassword(const std::string_view& text);
odaproto::clc::RconLogout   CLC_RconLogout  ();

odaproto::clc::SpectateBegin  CLC_SpectateBegin();
odaproto::clc::SpectateEnd    CLC_SpectateEnd();
odaproto::clc::SpectateUpdate CLC_SpectateUpdate(const player_t& player);

odaproto::clc::Cheat             CLC_Cheat(uint32_t cheatValue);
odaproto::clc::CheatGive         CLC_CheatGive(const std::string& item);
odaproto::clc::CheatSummon       CLC_CheatSummon(const std::string& monster);
odaproto::clc::CheatSummonFriend CLC_CheatSummonFriend(const std::string& monster);

odaproto::clc::CallVote CLC_CallVote(uint32_t voteType);
odaproto::clc::CallVote CLC_CallVote(uint32_t voteType, const std::string& arg);
odaproto::clc::CallVote CLC_CallVote(uint32_t voteType, const std::vector<std::string>& args);

odaproto::clc::Maplist       CLC_Maplist      (uint32_t status);
odaproto::clc::MaplistUpdate CLC_MaplistUpdate();
