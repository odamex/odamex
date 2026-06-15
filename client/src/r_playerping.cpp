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
#include "g_gametype.h"
#include "p_local.h"
#include "p_mobj.h"
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
static constexpr float PingReferenceHeight = 1080.0f;
static constexpr float PingMinimumResolutionScale = 0.40f;
static constexpr int FixedPingScreenPixels = 56;
static constexpr int FixedBossScreenPixels = 84;
static constexpr fixed_t PingScaleNearDistance = 512 * FRACUNIT;
static constexpr fixed_t DropScaleFarDistance = 1024 * FRACUNIT;
static constexpr fixed_t ItemScaleNearDistance = 128 * FRACUNIT;
static constexpr fixed_t ItemScaleFarDistance = 768 * FRACUNIT;
static constexpr fixed_t FlagScaleNearDistance = 256 * FRACUNIT;
static constexpr fixed_t FlagScaleFarDistance = 768 * FRACUNIT;
static constexpr fixed_t MonsterScaleNearDistance = 1024 * FRACUNIT;
static constexpr fixed_t PingScaleFarDistance = 2048 * FRACUNIT;
static constexpr float PingMinimumAlpha = 0.45f;
static constexpr int PingSolidTics = TICRATE;
static constexpr int PingFadeTics = TICRATE / 2;
static constexpr fixed_t PingAlphaNearDistance = 192 * FRACUNIT;
static constexpr fixed_t PingAlphaFarDistance = 512 * FRACUNIT;

std::array<v3fixed_t, MAXPLAYERS> FollowSmoothPositions{};
std::array<bool, MAXPLAYERS> FollowSmoothPositionValid{};
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

// Scale a marker's reference pixel size for the current viewport resolution.
int R_ResolutionScaledPingPixels(int basePixels)
{
	const int currentViewHeight = std::max(1, viewheight);
	const float scale =
	    std::clamp(static_cast<float>(currentViewHeight) / PingReferenceHeight,
	               PingMinimumResolutionScale, 1.0f);
	return std::max(1, static_cast<int>(std::lround(basePixels * scale)));
}

// Calculate a ping marker's screen size from its type and distance from the viewer.
int R_ScaledPingPixels(ping_type_t type, const v3fixed_t& position)
{
	if (type != PING_ITEM && type != PING_MONSTER && type != PING_FLAG &&
	    type != PING_GENERAL && type != PING_WARNING && type != PING_DROP)
	{
		return R_ResolutionScaledPingPixels(FixedPingScreenPixels);
	}

	AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
	if (!view)
		return R_ResolutionScaledPingPixels(FixedPingScreenPixels);

	const fixed_t distance =
	    P_AproxDistance(view->x - position.x, view->y - position.y);
	const int nearPixels = R_ResolutionScaledPingPixels(FixedPingScreenPixels);
	int farPixels = nearPixels / 2;
	fixed_t nearDistance = PingScaleNearDistance;
	fixed_t farDistance = PingScaleFarDistance;

	switch (type)
	{
	case PING_ITEM:
		nearDistance = ItemScaleNearDistance;
		farDistance = ItemScaleFarDistance;
		farPixels = (nearPixels * 2) / 5;
		break;
	case PING_DROP:
		farDistance = DropScaleFarDistance;
		break;
	case PING_FLAG:
		nearDistance = FlagScaleNearDistance;
		farDistance = FlagScaleFarDistance;
		break;
	case PING_MONSTER:
		nearDistance = MonsterScaleNearDistance;
		break;
	default:
		break;
	}

	if (distance <= nearDistance)
		return nearPixels;
	if (distance >= farDistance)
		return farPixels;

	const fixed_t fraction =
	    FixedDiv(distance - nearDistance, farDistance - nearDistance);
	return nearPixels - (((nearPixels - farPixels) * fraction) >> FRACBITS);
}

// Combine ping age and viewer distance to determine world-marker opacity.
float R_PingWorldAlpha(const playerPing_s& ping, const v3fixed_t& position)
{
	if (ping.type == PING_BOSS || ping.type == PING_TEAMMATE)
		return 1.0f;

	const int age = std::max(0, ::gametic - ping.pingtic);
	float ageAlpha = 1.0f;
	if (age > PingSolidTics)
	{
		const float fade =
		    std::min(1.0f, static_cast<float>(age - PingSolidTics) /
		                       static_cast<float>(std::max(1, PingFadeTics)));
		ageAlpha = 1.0f + (PingMinimumAlpha - 1.0f) * fade;
	}

	AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
	const fixed_t distance =
	    view ? P_AproxDistance(view->x - position.x, view->y - position.y) : 0;
	float distanceAlpha = PingMinimumAlpha;
	if (distance <= PingAlphaNearDistance)
	{
		distanceAlpha = 1.0f;
	}
	else if (distance < PingAlphaFarDistance)
	{
		const float fade = static_cast<float>(distance - PingAlphaNearDistance) /
		                   static_cast<float>(PingAlphaFarDistance -
		                                      PingAlphaNearDistance);
		distanceAlpha = 1.0f + (PingMinimumAlpha - 1.0f) * fade;
	}

	return std::max(ageAlpha, distanceAlpha);
}

// Position a following marker between the target's head and center as distance increases.
void R_PingFollowTargetPosition(const AActor* target, const AActor* view,
                                v3fixed_t& position)
{
	static constexpr fixed_t HeadOffset = 4 * FRACUNIT;
	static constexpr fixed_t NearDistance = 512 * FRACUNIT;
	static constexpr fixed_t FarDistance = 4096 * FRACUNIT;

	const fixed_t targetX =
	    target->prevx + FixedMul(render_lerp_amount, target->x - target->prevx);
	const fixed_t targetY =
	    target->prevy + FixedMul(render_lerp_amount, target->y - target->prevy);
	const fixed_t targetZ =
	    target->prevz + FixedMul(render_lerp_amount, target->z - target->prevz);
	const fixed_t distance = P_AproxDistance(view->x - targetX, view->y - targetY);
	const fixed_t clampedDistance = std::clamp(distance, NearDistance, FarDistance);
	const fixed_t fraction =
	    FixedDiv(clampedDistance - NearDistance, FarDistance - NearDistance);
	const fixed_t nearZ = targetZ + target->height + HeadOffset;
	const fixed_t farZ = targetZ + (target->height >> 1);

	position.x = targetX;
	position.y = targetY;
	position.z = nearZ - FixedMul(nearZ - farZ, fraction);
}

// Resolve and smooth the interpolated render position of a ping that follows an actor.
void R_ResolveFollowPingPosition(const player_t& player, const playerPing_s& ping,
                                 v3fixed_t& position)
{
	if (!ping.follow_target || ping.target_netid == 0)
		return;

	AActor* target = P_FindThingById(ping.target_netid);
	AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
	if (target && view)
	{
		if (ping.type == PING_MONSTER || ping.type == PING_BOSS)
		{
			position.x =
			    target->prevx + FixedMul(render_lerp_amount, target->x - target->prevx);
			position.y =
			    target->prevy + FixedMul(render_lerp_amount, target->y - target->prevy);
			const fixed_t targetZ =
			    target->prevz + FixedMul(render_lerp_amount, target->z - target->prevz);
			const fixed_t headOffset =
			    ping.type == PING_BOSS ? BossHeadOffset : StrictHeadOffset;
			position.z = targetZ + target->height + headOffset;
		}
		else
		{
			R_PingFollowTargetPosition(target, view, position);
		}
	}

	if (player.id >= MAXPLAYERS)
		return;

	if (!FollowSmoothPositionValid[player.id])
	{
		FollowSmoothPositions[player.id] = position;
		FollowSmoothPositionValid[player.id] = true;
	}
	else
	{
		v3fixed_t& smoothed = FollowSmoothPositions[player.id];
		smoothed.x = R_SmoothPingCoordinate(smoothed.x, position.x);
		smoothed.y = R_SmoothPingCoordinate(smoothed.y, position.y);
		smoothed.z = R_SmoothPingCoordinate(smoothed.z, position.z);
	}

	position = FollowSmoothPositions[player.id];
}

// Select the color translation appropriate for the ping type and source player.
translationref_t R_PingTranslation(const player_t& player, const playerPing_s& ping)
{
	if (ping.type == PING_ITEM)
		return {};
	if (ping.type == PING_GENERAL || ping.type == PING_WARNING || ping.type == PING_DROP)
		return P_PingReadablePlayerTranslation(player);
	if (ping.type == PING_FLAG && ping.flag_team != TEAM_NONE)
		return P_PingTeamTranslation(ping.flag_team);
	if (ping.translation)
		return ping.translation;
	return player.mo ? player.mo->translation : translationref_t{};
}

// Add each active player-created ping to the world sprite list.
void R_AddActivePlayerPingSprites(bool allowTeammateMarkers, bool allowHordeBossMarkers)
{
	for (const player_t& player : players)
	{
		if (!player.player_ping)
			continue;
		if (G_IsTeamGame() && player.id != consoleplayer().id &&
		    player.userinfo.team != consoleplayer().userinfo.team)
		{
			continue;
		}

		const playerPing_s& ping = *player.player_ping;
		if (P_IsPingExpired(ping))
			continue;
		if (consoleplayer().mo && ping.target_netid == consoleplayer().mo->netid)
			continue;
		if (ping.type == PING_TEAMMATE && !allowTeammateMarkers)
			continue;
		if (ping.type == PING_BOSS && !allowHordeBossMarkers)
			continue;

		v3fixed_t position{};
		if (!P_ResolvePingPosition(ping, position))
			continue;
		R_ResolveFollowPingPosition(player, ping, position);

		const int markerPixels =
		    ping.type == PING_BOSS
		        ? R_ResolutionScaledPingPixels(FixedBossScreenPixels)
		        : R_ScaledPingPixels(ping.type, position);
		R_Add3DHUDSprite(ping.lump, position, R_PingTranslation(player, ping),
		                 R_PingWorldAlpha(ping, position), markerPixels, markerPixels, false);
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

		const int markerPixels = R_ResolutionScaledPingPixels(FixedBossScreenPixels);
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

		const int nearPixels = R_ResolutionScaledPingPixels(TeammateNearPixels);
		const int farPixels = R_ResolutionScaledPingPixels(TeammateFarPixels);
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
		markerPixels = std::max(R_ResolutionScaledPingPixels(24), markerPixels);

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
