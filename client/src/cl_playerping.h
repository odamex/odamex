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

#pragma once

#include "d_player.h"

enum ping_render_position_t
{
	PING_RENDER_WORLD,
	PING_RENDER_LABEL
};

static constexpr int PingWorldMarkerPixels = 56;
static constexpr int PingBossMarkerPixels = 84;

bool CL_ShouldDrawPlayerPing(const player_t& player, const playerPing_s& ping,
                             bool allowTeammateMarkers, bool allowHordeBossMarkers);
bool CL_ResolvePingRenderPosition(const player_t& player, const playerPing_s& ping,
                                  ping_render_position_t renderType, v3fixed_t& position);
int CL_ResolutionScaledPingPixels(int basePixels);
int CL_PingMarkerPixels(ping_type_t type, const v3fixed_t& position);
float CL_PingMarkerAlpha(const playerPing_s& ping, const v3fixed_t& position,
                         fixed_t viewerX, fixed_t viewerY);
translationref_t CL_PingIconTranslation(const player_t& player, const playerPing_s& ping);
translationref_t CL_PingWorldTranslation(const player_t& player, const playerPing_s& ping);
