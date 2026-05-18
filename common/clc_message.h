#pragma once

#include "client.pb.h"

class player_t;
struct UserInfo;

void CLC_PackPlayerInputMessageFromPlayer(      odaproto::clc::PlayerInput& msg, const player_t& player, int clientTic, int clientWorldIndex);
void CLC_UnpackPlayerInputMessageToPlayer(const odaproto::clc::PlayerInput& msg,       player_t& player);

odaproto::clc::NetdemoCap CLC_NetdemoCap(const player_t& player, const odaproto::clc::PlayerInput& inputMessage);

odaproto::clc::DisconnectMe CLC_DisconnectMe();
odaproto::clc::Say          CLC_Say         (const std::string_view& text, uint32_t visibility);
odaproto::clc::UserInfo     CLC_UserInfo    (const UserInfo& userInfo);
odaproto::clc::PingReply    CLC_PingReply   (uint64_t msec);

odaproto::clc::Rcon         CLC_Rcon        (const std::string_view& text);
odaproto::clc::RconPassword CLC_RconPassword(const std::string_view& text);
odaproto::clc::RconLogout   CLC_RconLogout  ();

odaproto::clc::SpectateBegin  CLC_SpectateBegin();
odaproto::clc::SpectateEnd    CLC_SpectateEnd();
odaproto::clc::SpectateUpdate CLC_SpectateUpdate(const player_t& player);
