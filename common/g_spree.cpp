// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2025 by The Odamex Team.
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
//   Handle the loading of multi kill data from SPREEDEF,
//   as well as static functions to handle players'
//   multikills events.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "p_mobj.h"
#include "p_local.h"
#include "s_sound.h"

#include "g_multikill.h"
#include "m_ostring.h"

//
// SpreeManager::getInstance
//
// Singleton pattern
// Returns a pointer to the only allowable SpreeManager object
//
SpreeManager& SpreeManager::getInstance()
{
	static SpreeManager instance;
	return instance;
}

SpreeManager::~SpreeManager()
{
	reset();
}

SpreeManager::SpreeManager()
{
	multiTimeInterval = 4 * TICRATE;
}

//
// SpreeManager::reset
//
// Erases the current multikills levels and sets their defaults.
//
void SpreeManager::reset()
{
	multiKillLevels.clear();
	multiTimeInterval = 4 * TICRATE;
}

//
// SpreeManager::loadMultiKillDefaults
//
// Sets defaults for loading multi kills.
//
void SpreeManager::loadMultiKillDefaults()
{
	multiKillLevels.clear();
	MultiKillLevel_s emptylevel = {"", CR_GRAY};
	// First 2 levels (0 and 1), we insert empty levels as they're not multi kills.
	multiKillLevels.push_back(emptylevel); // 0
	multiKillLevels.push_back(emptylevel); // 1

	// Next, we input the next 9 levels with the default text. We don't use LANGUAGE tokens
	// since some people won't have an updated WAD.
	multiKillLevels.push_back({"Double Kill!",  CR_WHITE});    // 2
	multiKillLevels.push_back({"Triple Kill!",  CR_TAN});      // 3
	multiKillLevels.push_back({"Multi Kill!",   CR_BLUE});     // 4
	multiKillLevels.push_back({"Ultra Kill!",   CR_BRICK});    // 5
	multiKillLevels.push_back({"Overkill!",     CR_CYAN});     // 6
	multiKillLevels.push_back({"Mega Kill!",    CR_CREAM});    // 7
	multiKillLevels.push_back({"Monster Kill!", CR_ORANGE});   // 8
	multiKillLevels.push_back({"Mythic Kill!",  CR_PURPLE});   // 9
	multiKillLevels.push_back({"Killionaire!",  CR_DARKGREEN});// 10
	multiKillLevels.push_back({"Terminator!",   CR_RED});      // 11

	multiTimeInterval = 4 * TICRATE;
}

//
// SpreeManager::getMultiKillInterval
//
// Gets the multi kill interval to trigger multi kills.
//
int SpreeManager::getMultiKillInterval()
{
	return multiTimeInterval;
}

//
// SpreeManager::getHighestMultiKillLevel
//
// Gets the highest multi kill level.
//
int SpreeManager::getHighestMultiKillLevel()
{
	return multiKillLevels.size();
}

//
// SpreeManager::getMultiKillLevel
//
// Gets the highest multi kill level.
// If higher than the max level, get the highest one.
// If the array isnt populated, return it empty.
//
MultiKillLevel_s SpreeManager::getMultiKillLevel(int level)
{
	if (getHighestMultiKillLevel() <= 0)
		return {"", CR_GRAY};


	if (level > multiKillLevels.size())
		level = multiKillLevels.size();

	return multiKillLevels.at(level);
}

//
// SpreeManager::setMultiKillLevels
//
// Creates a new MultiKillLevel list and interval,
// as if reading a SPREEDEF to create a new multi kill level paradigm.
//
void SpreeManager::setMultiKillLevels(const std::vector<MultiKillLevel_s> multikills,
                                          int newinterval)
{
	multiKillLevels = multikills;
	multiTimeInterval = newinterval;
}

// ==========================================================
// Static functions start here.
// ==========================================================

/// <summary>
/// G_ProcessMultiKills occurs after a kill to determine
/// if this kill is an interval for a kill streak.
///
/// If so, show the kill streak text to the source player if he's the camera player.
/// Also, remove any kill streak from the killed player, and reset the timer for the
/// source player (if any)
/// </summary>
/// <param name="source">The killer (if a monster/player, null if environment/zombie
/// projectile)</param> <param name="target">The victim</param>
void G_ProcessMultiKills(AActor* source, player_t* target)
{
	if (target)
	{
		target->multikilltics = 0;
		target->multikills = 0;
		target->lastkilltime = 0;
	}

	if (!source->player)
		return;

	source->player->multikills++;
	source->player->lastkilltime = ::level.time;

	// Reset this player's multikilltics
	source->player->multikilltics = SpreeManager::getInstance().getMultiKillInterval();

	if (displayplayer_id == source->player->id &&
			source->player->multikills > 1)
	{
		// Play the sound for the new multi kill
		//S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

/// <summary>
/// Handles ticking players for multi kills.
/// </summary>
/// <param name="player">Player to tick.</param>
void G_TicMultiKill(player_t* player)
{
	if (!player)
		return;

	if (player->multikilltics)
	{
		player->multikilltics--;

		// We out of tics?
		if (player->multikilltics == 0)
		{
			// Remove the current multi kill as well.
			player->multikills = 0;
		}
	}
}
