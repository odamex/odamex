// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//     Player Ping system
//
// ORIGINAL AUTHOR:
//     Lexi Mayfield
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_playerping.h"

#include "c_dispatch.h"
#include "p_local.h"
#include "w_wad.h"
#include "../client/sdl/afxres.h"

//------------------------------------------------------------------------------

void P_PlayerPing(player_t &player)
{
	playerPing_s ping{};
	ping.translation = player.mo->translation;
	ping.pingtic = ::gametic;

	P_AutoAimLineAttack(player.mo, player.mo->angle, 1 << 26, 10, 16 * 64 * FRACUNIT);
	if (linetarget)
	{
		ping.pos.x = linetarget->x;
		ping.pos.y = linetarget->y;
		ping.pos.z = linetarget->z;
		PrintFmt("Ping: {}\n", ping.pos);
	}
	else
	{
		PrintFmt("No ping...\n");
	}

	int lump = W_GetNumForName("NET");
	if (lump == -1)
		return;
	ping.lump = lump;

	player.player_ping = std::make_unique<playerPing_s>(std::move(ping));
}

//------------------------------------------------------------------------------

void R_AddPingSprites()
{
#ifdef CLIENT_APP
	for (auto &pl : players)
	{
		if (!pl.player_ping)
			continue;
		
		auto& ping = *pl.player_ping;
		R_Add3DHUDSprite(ping.lump, ping.pos, {}, 1.0);
	}
#endif
}

//------------------------------------------------------------------------------

BEGIN_COMMAND(player_ping)
{
	if (::gamestate != GS_LEVEL)
	{
		return;
	}
	
	if (consoleplayer().playerstate != PST_LIVE)
	{
		// Dead players tell no tales.
		return;
	}
	
	P_PlayerPing(consoleplayer());
}
END_COMMAND(player_ping)
