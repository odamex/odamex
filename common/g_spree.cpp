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
#include "g_gametype.h"
#include "infomap.h"

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
	spreeDamageInterval = 10000;
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
	spreeDamageInterval = 10000;
}

//
// SpreeManager::loadSpreeDefaults
//
// Sets defaults for loading sprees.
//
void SpreeManager::loadSpreeDefaults()
{
	spreeLevels.clear();
	spreeLevels.push_back({"Killing spree!", "%s is on a killing spree!", CR_WHITE});    // 5  kills / 10000 dmg
	spreeLevels.push_back({"Rampage!", "%s is on a rampage!", CR_BLUE});              // 10 kills / 20000 dmg
	spreeLevels.push_back({"Dominating!", "%s is dominating!", CR_GREEN});          // 15 kills / 30000 dmg
	spreeLevels.push_back({"Unstoppable!", "%s is unstoppable!", CR_YELLOW});         // 20 kills / 40000 dmg
	spreeLevels.push_back({"Untouchable!", "%s is untouchable!", CR_CYAN});          // 25 kills / 50000 dmg
	spreeLevels.push_back({"Legendary!", "%s is legendary!", CR_GOLD});           // 30 kills / 60000 dmg

	spreeKillInterval = 5;
	spreeDamageInterval = 10000;
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
		return {"", "", CR_GRAY};


	if (level >= spreeLevels.size())
		level = spreeLevels.size() - 1;

	return spreeLevels.at(level);
}

//
// SpreeManager::setSpreeLevels
//
// Creates a new spree list and interval,
// as if reading a SPREEDEF to create a new multi kill level paradigm.
//
void SpreeManager::setSpreeLevels(const std::vector<spree_s> sprees,
                                  int newKillInterval, 
                                  int newDamageInterval)
{
	spreeLevels = sprees;
	spreeKillInterval = newKillInterval;
	spreeDamageInterval = newDamageInterval;
}

//
// SpreeManager::getSpreeBreaker
//
// Gets the current spree_breaker object.
//
spree_breaker SpreeManager::getSpreeBreaker()
{
	return spreeBreaker;
}

//
// SpreeManager::setSpreeBreaker
//
// Sets the current spree_breaker object.
//
void SpreeManager::setSpreeBreaker(AActor* source, player_t* target)
{
	// No player no spree
	if (!target)
		return;

	std::string endedPlayerName = target->userinfo.netname;
	int endedPlayerId = target->id;
	team_t endedTeam = target->userinfo.team;

	std::string enderName = "";
	int enderPlayerId = 0;
	team_t enderTeam = TEAM_NONE;
	bool enderIsMonster = false;

	// no source? treat it as a self kill
	if (!source)
	{
		enderName = endedPlayerName;
		enderPlayerId = endedPlayerId;
	}
	else if (source->player)
	{
		enderName = source->player->userinfo.netname;
		enderPlayerId = source->player->id;
		team_t enderTeam = source->player->userinfo.team;
	}
	else // potential monster
	{
		enderName = P_MobjToName(static_cast<mobjtype_t>(source->type));
		enderIsMonster = true;
	}

	spreeBreaker = {
			endedPlayerName,
			endedPlayerId,
			endedTeam,

			enderName,
			enderPlayerId,
			enderTeam,

			enderIsMonster,  
			
			::gametic
	};

	return;
}

//
// SpreeManager::hasSpree
//
// Checks if the user has a current spree active
//
bool SpreeManager::hasSpree(const player_t* player)
{
	if (!player)
		return false;

	if (spreeRecord.find(player->id) == spreeRecord.end())
		return false;
	else if (spreeRecord.find(player->id) != spreeRecord.end())
		return true;

	return false;
}

//
// SpreeManager::removeSpree
//
// Removes a spree for a specific player.
//
void SpreeManager::removeSpree(int playerid)
{
	if (spreeRecord.find(playerid) == spreeRecord.end())
		return;
	
	spreeRecord.erase(playerid);
	return;
}

// ==========================================================
// Static functions start here.
// ==========================================================

/// <summary>
/// G_ProcessSprees occurs after a kill to determine
/// if this kill is an interval for a killing spree.
///
/// If so, show the spree text to the source player if he's the camera player.
/// Also, remove any spree from a player who was killed, and
/// </summary>
/// <param name="source">The killer (if a monster/player, null if environment/zombie projectile)</param>
/// <param name="target">The victim</param>
void G_ProcessSpreeKill(AActor* source, player_t* target)
{
	static SpreeManager& manager = SpreeManager::getInstance();

	if (target)
	{
		target->killssincelastdeath = 0;
		target->damagesincelastdeath = 0;

		// If this player was on a spree, update it as the latest spree breaker.
		if (manager.hasSpree(target))
		{
			manager.removeSpree(target->id);
			manager.setSpreeBreaker(source, target);
		}
	}

	if (!source->player)
		return;

	source->player->killssincelastdeath += 1;

	// Check for spree interval, update the spree map with updates
	// If the gamemode isn't coop
	if (G_IsCoopGame())
		return;



	if (displayplayer_id == source->player->id &&
			source->player->multikills > 1)
	{
		// Play the sound for the new multi kill
		//S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

/// <summary>
/// Handles internal ticking for spree bookkeeping.
/// </summary>
void G_TicSprees()
{

}
