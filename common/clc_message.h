#pragma once

#include "client.pb.h"

class player_t;

void CLC_PackPlayerInputMessageFromPlayer(      odaproto::clc::PlayerInput& msg, const player_t& player, int clientTic, int clientWorldIndex);
void CLC_UnpackPlayerInputMessageToPlayer(const odaproto::clc::PlayerInput& msg,       player_t& player);

odaproto::clc::NetdemoCap CLC_NetdemoCap(const player_t& player, const odaproto::clc::PlayerInput& inputMessage);

