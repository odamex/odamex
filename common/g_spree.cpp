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

#include "g_spree.h"
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
	spreeKillInterval = 5;
}

//
// SpreeManager::reset
//
// Erases the current spree levels and sets their defaults.
//
void SpreeManager::reset()
{
	spreeLevels.clear();
	spreeKillInterval = 5;
}

//
// SpreeManager::loadSpreeDefaults
//
// Sets defaults for loading sprees.
//
void SpreeManager::loadSpreeDefaults()
{
	spreeLevels.clear();
	spree_s emptylevel = {"", CR_GRAY};
	// First 2 levels (0 and 1), we insert empty levels as they're not multi kills.
	spreeLevels.push_back(emptylevel);     // 0
	spreeLevels.push_back(emptylevel); // 1

	// Next, we input the next 9 levels with the default text. We don't use LANGUAGE tokens
	// since some people won't have an updated WAD.
	spreeLevels.push_back({"Double Kill!", CR_WHITE});         // 2
	spreeLevels.push_back({"Triple Kill!", CR_TAN});           // 3
	spreeLevels.push_back({"Multi Kill!", CR_BLUE});           // 4
	spreeLevels.push_back({"Ultra Kill!", CR_BRICK});          // 5
	spreeLevels.push_back({"Overkill!", CR_CYAN});             // 6
	spreeLevels.push_back({"Mega Kill!", CR_CREAM});           // 7
	spreeLevels.push_back({"Monster Kill!", CR_ORANGE});       // 8
	spreeLevels.push_back({"Mythic Kill!", CR_PURPLE});        // 9
	spreeLevels.push_back({"Killionaire!", CR_DARKGREEN});     // 10
	spreeLevels.push_back({"Terminator!", CR_RED});            // 11

	spreeKillInterval = 5;
}

//
// SpreeManager::getSpreeInterval
//
// Gets the spree interval to trigger sprees.
//
int SpreeManager::getSpreeInterval()
{
	return spreeKillInterval;
}

//
// SpreeManager::getHighestSpreeLevel
//
// Gets the highest spree.
//
int SpreeManager::getHighestSpreeLevel()
{
	return spreeLevels.size();
}

//
// SpreeManager::getSpreeLevel
//
// Gets the highest multi kill level.
// If higher than the max level, get the highest one.
// If the array isnt populated, return it empty.
//
spree_s SpreeManager::getSpreeLevel(int level)
{
	if (getHighestSpreeLevel() <= 0)
		return {"", CR_GRAY};


	if (level > spreeLevels.size())
		level = spreeLevels.size();

	return spreeLevels.at(level);
}

//
// SpreeManager::setSpreeLevels
//
// Creates a new spree list and interval,
// as if reading a SPREEDEF to create a new multi kill level paradigm.
//
void SpreeManager::setSpreeLevels(const std::vector<spree_s> sprees,
                                          int newinterval)
{
	spreeLevels = sprees;
	spreeKillInterval = newinterval;
}

// ==========================================================
// Static functions start here.
// ==========================================================

/// <summary>
/// G_ProcessSprees occurs after a kill to determine
/// if this kill is an interval for a kill streak.
///
/// If so, show the kill streak text to the source player if he's the camera player.
/// Also, remove any kill streak from the killed player, and reset the timer for the
/// source player (if any)
/// </summary>
/// <param name="source">The killer (if a monster/player, null if environment/zombie
/// projectile)</param> <param name="target">The victim</param>
void G_ProcessSprees(AActor* source, player_t* target)
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
	source->player->multikilltics = SpreeManager::getInstance().getSpreeInterval();

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
void G_TicSprees(player_t* player)
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
