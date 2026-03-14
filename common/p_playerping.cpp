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
#include "p_mobj.h"
#include "w_wad.h"
#ifdef CLIENT_APP
#include "../client/src/cl_main.h"
#endif
#include "../client/sdl/afxres.h"

//------------------------------------------------------------------------------

namespace
{
struct pingTrace_t
{
	fixed_t x1 = 0;
	fixed_t y1 = 0;
	fixed_t dx = 0;
	fixed_t dy = 0;
	fixed_t shootz = 0;
	fixed_t slope = 0;
	fixed_t range = 0;
	v3fixed_t hit{};
	bool blocked = false;
} g_pingTrace;

bool PTR_PingTraverse(intercept_t* in)
{
	if (!in || !in->isaline)
		return true;

	line_t* li = in->d.line;
	const fixed_t crossx = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, in->frac);
	const fixed_t crossy = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, in->frac);
	const fixed_t z =
	    g_pingTrace.shootz + FixedMul(g_pingTrace.slope, FixedMul(in->frac, g_pingTrace.range));

	if (!(li->flags & ML_TWOSIDED))
	{
		g_pingTrace.blocked = true;
	}
	else
	{
		P_LineOpening(li, crossx, crossy);
		g_pingTrace.blocked = (z >= opentop || z <= openbottom);
	}

	if (!g_pingTrace.blocked)
		return true;

	fixed_t frac = in->frac;
	if (g_pingTrace.range > 0)
	{
		frac -= FixedDiv(4 * FRACUNIT, g_pingTrace.range);
	}
	frac = std::clamp(frac, 0, FRACUNIT);

	g_pingTrace.hit.x = g_pingTrace.x1 + FixedMul(g_pingTrace.dx, frac);
	g_pingTrace.hit.y = g_pingTrace.y1 + FixedMul(g_pingTrace.dy, frac);
	g_pingTrace.hit.z =
	    g_pingTrace.shootz + FixedMul(g_pingTrace.slope, FixedMul(frac, g_pingTrace.range));
	return false;
}

v3fixed_t P_TracePingEndpoint(const AActor* source, angle_t angle, fixed_t slope, fixed_t range)
{
	const int angleidx = angle >> ANGLETOFINESHIFT;
	const fixed_t x1 = source->x;
	const fixed_t y1 = source->y;
	const fixed_t x2 = x1 + FixedMul(range, finecosine[angleidx]);
	const fixed_t y2 = y1 + FixedMul(range, finesine[angleidx]);

	g_pingTrace = pingTrace_t{};
	g_pingTrace.x1 = x1;
	g_pingTrace.y1 = y1;
	g_pingTrace.dx = x2 - x1;
	g_pingTrace.dy = y2 - y1;
	g_pingTrace.shootz = source->z + (source->height >> 1) + 8 * FRACUNIT;
	g_pingTrace.slope = slope;
	g_pingTrace.range = range;
	g_pingTrace.hit = {x2, y2, g_pingTrace.shootz + FixedMul(range, slope)};

	P_PathTraverse(x1, y1, x2, y2, PT_ADDLINES, PTR_PingTraverse);
	return g_pingTrace.hit;
}
} // namespace

//------------------------------------------------------------------------------

void P_PlayerPing(player_t &player)
{
	static constexpr fixed_t PingRange = 16 * 64 * FRACUNIT;

	playerPing_s ping{};
	ping.translation = player.mo->translation;
	ping.pingtic = ::gametic;

	angle_t aimAngle = player.mo->angle;
	P_AutoAimLineAttack(player.mo, aimAngle, 1 << 26, 10, PingRange);
	if (linetarget)
	{
		ping.pos.x = linetarget->x;
		ping.pos.y = linetarget->y;
		ping.pos.z = linetarget->z;
		ping.target_netid = linetarget->netid;
		ping.follow_target = ping.target_netid != 0;
		PrintFmt("Ping: {}\n", ping.pos);
	}
	else
	{
		const fixed_t pitchSlope =
		    finetangent[FINEANGLES / 4 - (player.mo->pitch >> ANGLETOFINESHIFT)];
		ping.pos = P_TracePingEndpoint(player.mo, player.mo->angle, pitchSlope, PingRange);
		ping.target_netid = 0;
		ping.follow_target = false;
		PrintFmt("World ping: {}\n", ping.pos);
	}

	int lump = W_GetNumForName("NET");
	if (lump == -1)
		return;
	ping.lump = lump;

	player.player_ping = std::make_unique<playerPing_s>(std::move(ping));
}

//------------------------------------------------------------------------------

bool P_IsPingExpired(const playerPing_s& ping)
{
	static constexpr int PingLifetimeTics = 10 * TICRATE;
	if (ping.pingtic < 0)
		return true;

	return (::gametic - ping.pingtic) > PingLifetimeTics;
}

//------------------------------------------------------------------------------

bool P_ResolvePingPosition(const playerPing_s& ping, v3fixed_t& outPos)
{
	if (ping.follow_target && ping.target_netid != 0)
	{
		AActor* target = P_FindThingById(ping.target_netid);
		if (target)
		{
			outPos.x = target->x;
			outPos.y = target->y;
			outPos.z = target->z + (target->height >> 1);
			return true;
		}
	}

	outPos = ping.pos;
	return true;
}

//------------------------------------------------------------------------------

void R_AddPingSprites()
{
#ifdef CLIENT_APP
	static constexpr int FollowPingMinScreenPx = 48;
	static constexpr int FollowPingMaxScreenPx = 128;
	static constexpr fixed_t FollowPingHeadOffset = 4 * FRACUNIT;
	static constexpr fixed_t FollowPingNearDist = 512 * FRACUNIT;
	static constexpr fixed_t FollowPingFarDist = 4096 * FRACUNIT;

	for (auto &pl : players)
	{
		if (!pl.player_ping)
			continue;

		auto& ping = *pl.player_ping;
		if (P_IsPingExpired(ping))
			continue;

		v3fixed_t pos{};
		if (!P_ResolvePingPosition(ping, pos))
			continue;

		if (ping.follow_target && ping.target_netid != 0)
		{
			AActor* target = P_FindThingById(ping.target_netid);
			AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
			if (target && view)
			{
				const fixed_t dist = P_AproxDistance(view->x - target->x, view->y - target->y);
				const fixed_t clamped_dist = std::clamp(dist, FollowPingNearDist, FollowPingFarDist);
				const fixed_t t = FixedDiv(clamped_dist - FollowPingNearDist,
				                           FollowPingFarDist - FollowPingNearDist);

				const fixed_t near_z = target->z + target->height + FollowPingHeadOffset;
				const fixed_t far_z = target->z + (target->height >> 1);

				pos.x = target->x;
				pos.y = target->y;
				pos.z = near_z - FixedMul(near_z - far_z, t);
			}
		}

		const int min_px = ping.follow_target ? FollowPingMinScreenPx : 0;
		const int max_px = ping.follow_target ? FollowPingMaxScreenPx : 0;
		R_Add3DHUDSprite(ping.lump, pos, {}, 1.0f, min_px, max_px);
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

#ifdef CLIENT_APP
	// Connected clients must request ping creation from the server so that
	// other clients receive the replicated ping message.
	if (connected)
	{
		buf_t& netBuf = messenger.NetBuf().Obtain();
		MSG_WriteMarker(&netBuf, clc_netcmd);
		MSG_WriteString(&netBuf, "player_ping");
		MSG_WriteByte(&netBuf, 0);
		return;
	}
#endif

	P_PlayerPing(consoleplayer());
}
END_COMMAND(player_ping)
