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

#pragma once

#include "d_player.h"

enum ping_submit_result_t
{
	PING_SUBMIT_NONE = 0,
	PING_SUBMIT_PLACED,
	PING_SUBMIT_PLACED_RETAP_WARNING,
	PING_SUBMIT_RATE_LIMITED
};

struct ping_filter_t
{
	bool pickups = true;
	bool monsters = true;
	bool flags = true;
	bool mouselook = false;
};

/**
 * @brief A player wants to ping something, try to do so.
 *
 * @param player Player who wants to ping.
 */
ping_submit_result_t P_PlayerPing(player_t &player, const ping_filter_t& filter = ping_filter_t{},
                                  bool dropAtSelf = false);

/**
 * @brief Add ping sprites to be rendered.
 */
void R_AddPingSprites();

/**
 * @brief Clear all active player pings.
 *
 * Used for graceful cleanup during disconnect and level transitions.
 */
void P_ClearAllPlayerPings();

/**
 * @brief Clear ping state for a specific player (including spam limiter state).
 */
void P_ClearPlayerPingState(player_t& player);

/**
 * @brief Resolve a ping's current world position.
 *
 * Follows target actors when configured and falls back to the stored static
 * position if the target no longer exists.
 */
bool P_ResolvePingPosition(const playerPing_s& ping, v3fixed_t& outPos);

/**
 * @brief Returns true if the ping has expired.
 */
bool P_IsPingExpired(const playerPing_s& ping);

#ifdef CLIENT_APP
/**
 * @brief Return a readable ping translation for a player's color.
 *
 * Uses the player's exact translation for normal colors, and a hue-aware
 * readable fallback for colors that are too close to black/white.
 */
translationref_t P_PingReadablePlayerTranslation(const player_t& pl);

/**
 * @brief Return a fixed team-color translation for ping markers.
 *
 * This is deterministic and does not depend on player colors.
 */
translationref_t P_PingTeamTranslation(team_t team);
#endif
