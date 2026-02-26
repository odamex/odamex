#pragma once

#include "common.pb.h"

class player_t;

void CLC_ClientCommandFromPlayer(odaproto::ClientCommand& msg, const player_t& player);
void CLC_ClientCommandToPlayer(player_t& player, const odaproto::ClientCommand& msg);
