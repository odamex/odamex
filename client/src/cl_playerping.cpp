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
//     Shared client presentation for player pings
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "cl_playerping.h"

#include <cmath>

#include "cl_main.h"
#include "g_gametype.h"
#include "p_local.h"
#include "p_mobj.h"
#include "p_playerping.h"
#include "r_main.h"

namespace
{
static constexpr fixed_t StrictHeadOffset = 12 * FRACUNIT;
static constexpr fixed_t BossHeadOffset = 24 * FRACUNIT;
static constexpr fixed_t FollowPingHeadOffset = 4 * FRACUNIT;
static constexpr fixed_t FollowPingNearDistance = 512 * FRACUNIT;
static constexpr fixed_t FollowPingFarDistance = 4096 * FRACUNIT;
static constexpr fixed_t FollowPingSmoothing = FRACUNIT / 3;
static constexpr float PingReferenceHeight = 1080.0f;
static constexpr float PingMinimumResolutionScale = 0.40f;
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

std::array<v3fixed_t, MAXPLAYERS> WorldSmoothPositions{};
std::array<bool, MAXPLAYERS> WorldSmoothPositionValid{};
std::array<v3fixed_t, MAXPLAYERS> LabelSmoothPositions{};
std::array<bool, MAXPLAYERS> LabelSmoothPositionValid{};

fixed_t CL_SmoothPingCoordinate(fixed_t from, fixed_t to)
{
	return from + FixedMul(FollowPingSmoothing, to - from);
}

void CL_PingFollowTargetPosition(const AActor* target, const AActor* view,
                                 v3fixed_t& position)
{
	const fixed_t targetX =
	    target->prevx + FixedMul(render_lerp_amount, target->x - target->prevx);
	const fixed_t targetY =
	    target->prevy + FixedMul(render_lerp_amount, target->y - target->prevy);
	const fixed_t targetZ =
	    target->prevz + FixedMul(render_lerp_amount, target->z - target->prevz);
	const fixed_t distance = P_AproxDistance(view->x - targetX, view->y - targetY);
	const fixed_t clampedDistance =
	    std::clamp(distance, FollowPingNearDistance, FollowPingFarDistance);
	const fixed_t fraction =
	    FixedDiv(clampedDistance - FollowPingNearDistance,
	             FollowPingFarDistance - FollowPingNearDistance);
	const fixed_t nearZ = targetZ + target->height + FollowPingHeadOffset;
	const fixed_t farZ = targetZ + (target->height >> 1);

	position.x = targetX;
	position.y = targetY;
	position.z = nearZ - FixedMul(nearZ - farZ, fraction);
}

void CL_SmoothPingPosition(size_t playerId, ping_render_position_t renderType,
                           v3fixed_t& position)
{
	if (playerId >= MAXPLAYERS)
		return;

	std::array<v3fixed_t, MAXPLAYERS>& positions =
	    renderType == PING_RENDER_WORLD ? WorldSmoothPositions : LabelSmoothPositions;
	std::array<bool, MAXPLAYERS>& valid =
	    renderType == PING_RENDER_WORLD ? WorldSmoothPositionValid
	                                    : LabelSmoothPositionValid;
	if (!valid[playerId])
	{
		positions[playerId] = position;
		valid[playerId] = true;
	}
	else
	{
		v3fixed_t& smoothed = positions[playerId];
		smoothed.x = CL_SmoothPingCoordinate(smoothed.x, position.x);
		smoothed.y = CL_SmoothPingCoordinate(smoothed.y, position.y);
		smoothed.z = CL_SmoothPingCoordinate(smoothed.z, position.z);
	}

	position = positions[playerId];
}
} // namespace

// Determine whether a replicated ping is eligible for client presentation.
bool CL_ShouldDrawPlayerPing(const player_t& player, const playerPing_s& ping,
                             bool allowTeammateMarkers, bool allowHordeBossMarkers)
{
	if (G_IsTeamGame() && player.id != consoleplayer().id &&
	    player.userinfo.team != consoleplayer().userinfo.team)
	{
		return false;
	}
	if (P_IsPingExpired(ping))
		return false;
	if (consoleplayer().mo && ping.target_netid == consoleplayer().mo->netid)
		return false;
	if (ping.type == PING_TEAMMATE && !allowTeammateMarkers)
		return false;
	if (ping.type == PING_BOSS && !allowHordeBossMarkers)
		return false;
	return true;
}

// Resolve an interpolated and smoothed position for a world marker or HUD label.
bool CL_ResolvePingRenderPosition(const player_t& player, const playerPing_s& ping,
                                  ping_render_position_t renderType, v3fixed_t& position)
{
	if (!P_ResolvePingPosition(ping, position))
		return false;
	if (!ping.follow_target || ping.target_netid == 0)
		return true;

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
		else if (renderType == PING_RENDER_WORLD)
		{
			CL_PingFollowTargetPosition(target, view, position);
		}
	}

	CL_SmoothPingPosition(player.id, renderType, position);
	return true;
}

// Scale a marker's reference pixel size for the current viewport resolution.
int CL_ResolutionScaledPingPixels(int basePixels)
{
	const int currentViewHeight = std::max(1, viewheight);
	const float scale =
	    std::clamp(static_cast<float>(currentViewHeight) / PingReferenceHeight,
	               PingMinimumResolutionScale, 1.0f);
	return std::max(1, static_cast<int>(std::lround(basePixels * scale)));
}

// Calculate a marker's screen size from its type and distance from the viewer.
int CL_PingMarkerPixels(ping_type_t type, const v3fixed_t& position)
{
	if (type != PING_ITEM && type != PING_MONSTER && type != PING_FLAG &&
	    type != PING_GENERAL && type != PING_WARNING && type != PING_DROP)
	{
		return CL_ResolutionScaledPingPixels(PingWorldMarkerPixels);
	}

	AActor* view = consoleplayer().camera ? consoleplayer().camera : consoleplayer().mo;
	if (!view)
		return CL_ResolutionScaledPingPixels(PingWorldMarkerPixels);

	const fixed_t distance =
	    P_AproxDistance(view->x - position.x, view->y - position.y);
	const int nearPixels = CL_ResolutionScaledPingPixels(PingWorldMarkerPixels);
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

// Combine ping age and viewer distance to determine presentation opacity.
float CL_PingMarkerAlpha(const playerPing_s& ping, const v3fixed_t& position,
                         fixed_t viewerX, fixed_t viewerY)
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

	const fixed_t distance =
	    P_AproxDistance(viewerX - position.x, viewerY - position.y);
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

// Select the translation used for the icon beside an off-screen indicator.
translationref_t CL_PingIconTranslation(const player_t& player,
                                        const playerPing_s& ping)
{
	if (ping.type == PING_FLAG && ping.flag_team != TEAM_NONE)
		return P_PingTeamTranslation(ping.flag_team);
	if (ping.type == PING_GENERAL || ping.type == PING_WARNING || ping.type == PING_DROP ||
	    ping.type == PING_TEAMMATE)
	{
		return P_PingReadablePlayerTranslation(player);
	}
	return {};
}

// Select the translation used by the ping's in-world marker.
translationref_t CL_PingWorldTranslation(const player_t& player,
                                         const playerPing_s& ping)
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
