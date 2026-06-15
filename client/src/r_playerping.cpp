// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// DESCRIPTION:
//     Player ping world marker rendering
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "r_playerping.h"

#include <cmath>
#include <unordered_map>

#include "cl_main.h"
#include "cl_playerping.h"
#include "g_gametype.h"
#include "p_local.h"
#include "p_playerping.h"
#include "r_main.h"
#include "r_things.h"
#include "w_wad.h"

EXTERN_CVAR(cl_showpings)
EXTERN_CVAR(hud_marker_teammates)
EXTERN_CVAR(hud_marker_hordeboss)
EXTERN_CVAR(sv_pingsystem)
EXTERN_CVAR(sv_marker_teammates)
EXTERN_CVAR(sv_marker_hordeboss)

namespace
{
static constexpr fixed_t StrictHeadOffset = 12 * FRACUNIT;
static constexpr fixed_t BossHeadOffset = 24 * FRACUNIT;
static constexpr fixed_t FollowPingSmoothing = FRACUNIT / 3;

std::unordered_map<uint32_t, v3fixed_t> ActorSmoothPositions{};

// Move one fixed-point coordinate toward its current render position.
fixed_t R_SmoothPingCoordinate(fixed_t from, fixed_t to)
{
	return from + FixedMul(FollowPingSmoothing, to - from);
}

// Smooth markers that are tied to actors, using the actor netid as the stable key.
v3fixed_t R_SmoothPingActorPosition(uint32_t netid, const v3fixed_t& targetPosition)
{
	if (netid == 0)
		return targetPosition;

	auto [iterator, inserted] = ActorSmoothPositions.try_emplace(netid, targetPosition);
	if (inserted)
		return targetPosition;

	v3fixed_t& current = iterator->second;
	current.x = R_SmoothPingCoordinate(current.x, targetPosition.x);
	current.y = R_SmoothPingCoordinate(current.y, targetPosition.y);
	current.z = R_SmoothPingCoordinate(current.z, targetPosition.z);
	return current;
}

// Add each active player-created ping to the world sprite list.
void R_AddActivePlayerPingSprites(bool allowTeammateMarkers, bool allowHordeBossMarkers)
{
	for (const player_t& player : players)
	{
		if (!player.player_ping)
			continue;
		const playerPing_s& ping = *player.player_ping;
		if (!CL_ShouldDrawPlayerPing(player, ping, allowTeammateMarkers,
		                             allowHordeBossMarkers))
			continue;

		v3fixed_t position{};
		if (!CL_ResolvePingRenderPosition(player, ping, PING_RENDER_WORLD, position))
			continue;

		const int markerPixels =
		    ping.type == PING_BOSS
		        ? CL_ResolutionScaledPingPixels(PingBossMarkerPixels)
		        : CL_PingMarkerPixels(ping.type, position);
		AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
		const fixed_t viewerX = view ? view->x : 0;
		const fixed_t viewerY = view ? view->y : 0;
		R_Add3DHUDSprite(ping.lump, position, CL_PingWorldTranslation(player, ping),
		                 CL_PingMarkerAlpha(ping, position, viewerX, viewerY),
		                 markerPixels, markerPixels, false);
	}
}

// Add persistent markers above every live horde boss.
void R_AddHordeBossPingSprites()
{
	AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
	const int bossLump = W_GetNumForName("OPNG_BOS");
	if (!view || bossLump == -1)
		return;

	TThinkerIterator<AActor> iterator;
	while (AActor* actor = iterator.Next())
	{
		if (!P_IsHordeBossForPing(actor))
			continue;

		const fixed_t x =
		    actor->prevx + FixedMul(render_lerp_amount, actor->x - actor->prevx);
		const fixed_t y =
		    actor->prevy + FixedMul(render_lerp_amount, actor->y - actor->prevy);
		const fixed_t z =
		    actor->prevz + FixedMul(render_lerp_amount, actor->z - actor->prevz);
		v3fixed_t position{x, y, z + actor->height + BossHeadOffset};
		position = R_SmoothPingActorPosition(actor->netid, position);

		const int markerPixels = CL_ResolutionScaledPingPixels(PingBossMarkerPixels);
		R_Add3DHUDSprite(bossLump, position, {}, 1.0f, markerPixels, markerPixels, false);
	}
}

// Add persistent markers above eligible cooperative or team-game players.
void R_AddTeammatePingSprites()
{
	static constexpr int TeammateNearPixels = 56;
	static constexpr int TeammateFarPixels = 22;
	static constexpr fixed_t TeammateNearDistance = 512 * FRACUNIT;
	static constexpr fixed_t TeammateFarDistance = 4096 * FRACUNIT;

	const int teammateLump = W_GetNumForName("OPNG_TM");
	if (teammateLump == -1)
		return;

	AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
	for (const player_t& player : players)
	{
		if (player.id == consoleplayer().id || !player.ingame() || player.spectator ||
		    !player.mo || player.playerstate != PST_LIVE)
		{
			continue;
		}
		if (G_IsTeamGame() && player.userinfo.team != consoleplayer().userinfo.team)
			continue;

		const fixed_t x =
		    player.mo->prevx + FixedMul(render_lerp_amount, player.mo->x - player.mo->prevx);
		const fixed_t y =
		    player.mo->prevy + FixedMul(render_lerp_amount, player.mo->y - player.mo->prevy);
		const fixed_t z =
		    player.mo->prevz + FixedMul(render_lerp_amount, player.mo->z - player.mo->prevz);
		v3fixed_t position{x, y, z + player.mo->height + StrictHeadOffset};
		position = R_SmoothPingActorPosition(player.mo->netid, position);

		const int nearPixels = CL_ResolutionScaledPingPixels(TeammateNearPixels);
		const int farPixels = CL_ResolutionScaledPingPixels(TeammateFarPixels);
		int markerPixels = nearPixels;
		if (view)
		{
			const fixed_t distance = P_AproxDistance(view->x - x, view->y - y);
			const fixed_t clampedDistance =
			    std::clamp(distance, TeammateNearDistance, TeammateFarDistance);
			const fixed_t fraction = FixedDiv(clampedDistance - TeammateNearDistance,
			                                  TeammateFarDistance -
			                                      TeammateNearDistance);
			markerPixels =
			    nearPixels - (((nearPixels - farPixels) * fraction) >> FRACBITS);

			// Do not let the marker grow wider than the projected player sprite.
			// This keeps the limit valid for modded player radii and sprite scales.
			fixed_t projectedX = 0;
			fixed_t projectedY = 0;
			R_RotatePoint(x - viewx, y - viewy, ANG90 - viewangle, projectedX,
			              projectedY);
			if (projectedY > FRACUNIT)
			{
				const int left =
				    R_ProjectPointX(projectedX - player.mo->radius, projectedY);
				const int right =
				    R_ProjectPointX(projectedX + player.mo->radius, projectedY);
				const int projectedWidth = std::abs(right - left);
				if (projectedWidth > 0)
					markerPixels = std::min(markerPixels, projectedWidth);
			}
		}
		markerPixels = std::max(CL_ResolutionScaledPingPixels(24), markerPixels);

		R_Add3DHUDSprite(teammateLump, position, P_PingReadablePlayerTranslation(player),
		                 1.0f, markerPixels, markerPixels, false);
	}
}
} // namespace

// Add all enabled player pings, horde bosses, and teammate markers for this frame.
void R_AddPingSprites()
{
	const bool allowPingMarkers = sv_pingsystem && cl_showpings;
	const bool allowTeammateMarkers = sv_marker_teammates && hud_marker_teammates;
	const bool allowHordeBossMarkers = sv_marker_hordeboss && hud_marker_hordeboss;
	if (!allowPingMarkers && !allowTeammateMarkers && !allowHordeBossMarkers)
		return;

	if (allowPingMarkers)
		R_AddActivePlayerPingSprites(allowTeammateMarkers, allowHordeBossMarkers);
	if (allowHordeBossMarkers)
		R_AddHordeBossPingSprites();
	if ((G_IsCoopGame() || G_IsTeamGame()) && allowTeammateMarkers)
		R_AddTeammatePingSprites();
}
