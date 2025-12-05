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
	repeatingSpreeText = "";
}

//
// SpreeManager::reset
//
// Erases the current spree levels and sets their defaults.
//
void SpreeManager::reset()
{
	spreeLevels.clear();
	spreeRecord.clear();
	spreeBreaker = {"", -1, TEAM_NONE, "", -1, TEAM_NONE, "", "", CR_GOLD, false, 0, 0};
	spreeKillInterval = 5;
	spreeDamageInterval = 10000;
	repeatingSpreeText = "";
}

//
// SpreeManager::clearSprees
//
// Erases the current spree records.
//
void SpreeManager::clearSprees()
{
	spreeRecord.clear();
	spreeBreaker = {"", -1, TEAM_NONE, "", -1, TEAM_NONE, "", "", CR_GOLD, false, 0, 0};
}

//
// SpreeManager::loadSpreeDefaults
//
// Sets defaults for loading sprees.
//
void SpreeManager::loadSpreeDefaults()
{
	spreeLevels.clear();
	spreeLevels.push_back({"Killing spree!", "%s is on a %s", CR_WHITE});// 5  kills / 10000 dmg
	spreeLevels.push_back({"Rampage!", "%s is on a %s", CR_BLUE});       // 10 kills / 20000 dmg
	spreeLevels.push_back({"Dominating!", "%s is %s", CR_GREEN});        // 15 kills / 30000 dmg
	spreeLevels.push_back({"Unstoppable!", "%s is %s", CR_YELLOW});      // 20 kills / 40000 dmg
	spreeLevels.push_back({"Untouchable!", "%s is %s", CR_CYAN});        // 25 kills / 50000 dmg
	spreeLevels.push_back({"Legendary!", "%s is %s", CR_GOLD});          // 30 kills / 60000 dmg

	repeatingSpreeText = "%s is STILL %s!";

	spreeEndPlayer = "%s's %s was ended by %s";
	spreeEndSelf = "%s was looking good until %g killed %hself!";
	spreeEndMonster = "%s's %s was ended by a %s!";

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
	return spreeLevels.size() - 1;
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
	spree_s spree = {"", "", CR_GRAY};
	bool stillDominating = false;

	if (getHighestSpreeLevel() <= -1)
	{
		return spree;
	}

	if (level >= spreeLevels.size())
	{
		level = spreeLevels.size() - 1;
		stillDominating = true;
	}

	spree = spreeLevels.at(level);

	if (stillDominating)
	{
		spree.spreeBroadcastText = repeatingSpreeText;
	}

	return spree;
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

	spree_s spreeLevel = getSpreeLevel(getSpreeLevelByKills(target->killssincelastdeath));

	std::string enderName = "";
	int enderPlayerId = 0;
	team_t enderTeam = TEAM_NONE;
	bool enderIsMonster = false;

	std::string broadcastText = "";
	std::string spreeEnded = "";
	EColorRange spreeEndedColor = CR_GOLD;

	int kills = 0;

	// no source? treat it as a self kill
	if (!source)
	{
		enderName = endedPlayerName;
		enderPlayerId = endedPlayerId;
		broadcastText = spreeEndSelf;
	}
	else if (source->player)
	{
		enderName = source->player->userinfo.netname;
		enderPlayerId = source->player->id;
		team_t enderTeam = source->player->userinfo.team;
		broadcastText = spreeEndPlayer;
	}
	else // potential monster
	{
		enderName = P_MobjToName(static_cast<mobjtype_t>(source->type));
		enderIsMonster = true;
		broadcastText = spreeEndMonster;
	}

	spreeEnded = spreeLevel.spreeText;
	spreeEndedColor = spreeLevel.color;

	spreeBreaker = {
			endedPlayerName,
			endedPlayerId,
			endedTeam,

			enderName,
			enderPlayerId,
			enderTeam,

			broadcastText,
			spreeEnded,
			spreeEndedColor,

			enderIsMonster,

			kills,
			
			::gametic
	};

	return;
}

//
// SpreeManager::hasSpree
//
// Checks if the user has a current spree active
//
bool SpreeManager::hasSpree(const int playerid)
{
	if (spreeRecord.find(playerid) == spreeRecord.end())
		return false;
	else if (spreeRecord.find(playerid) != spreeRecord.end())
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

//
// SpreeManager::getSpreeLevelByKills
//
// Gets a specific spree level by the amount of kills the player has.
//
int SpreeManager::getSpreeLevelByKills(int kills)
{
	if (spreeLevels.size() == 0 || spreeKillInterval <= 0)
		return -1;

	// Still on the first 5 kills?
	if (kills < spreeKillInterval)
		return -1;

	int level = kills / spreeKillInterval;

	if (level <= 0)
		return -1;

	return level - 1;
}

//
// SpreeManager::recordPlayerKill
//
// Records the players kills since last death,
// and updates the spree record accordingly.
// 
// If the player doesn't have a spree record yet, create one.
// 
// If the player has reached a higher spree level, update it.
// 
// Returns true if the spree level was updated, false otherwise.
//
bool SpreeManager::recordPlayerKill(const player_t* player)
{

	if (!player)
		return false;

	int newSpreeLevel = getSpreeLevelByKills(player->killssincelastdeath);
	int maxSpreeLevel = getHighestSpreeLevel();

	if (newSpreeLevel <= -1)
		return false;

	if (spreeRecord.find(player->id) == spreeRecord.end())
	{
		// Spree record not found, create it (if necessary)
		spree_record newRecord;
		newRecord.playerId = player->id;
		newRecord.playerName = player->userinfo.netname;
		newRecord.spreeLevel = newSpreeLevel > maxSpreeLevel ? maxSpreeLevel : newSpreeLevel;
		newRecord.spreeStartTic = ::gametic;
		newRecord.stillDominating = false;
		spreeRecord[player->id] = newRecord;
		return true;
	}
	else
	{
		// Spree record found, check if we can upgrade
		spree_record& record = spreeRecord[player->id];

		if (newSpreeLevel > record.spreeLevel)
		{
			// Upgrade spree level
			record.spreeLevel = newSpreeLevel;
			record.spreeStartTic = ::gametic;
			record.stillDominating = newSpreeLevel > maxSpreeLevel ? true : false;
			return true;
		}
	}

	return false;
}

//
// SpreeManager::getSpreeRecord
//
// Gets a spree record for a player.
//
spree_record SpreeManager::getSpreeRecord(int playerId)
{
	if (spreeRecord.find(playerId) != spreeRecord.end())
	{
		return spreeRecord[playerId];
	}

	spree_record record = {"null", -1, 0, 0, false};

	return record;
}

//
// SpreeManager::expireOldSprees
//
// Cleans up any old sprees or spree breakers.
//
void SpreeManager::expireOldSprees()
{
	if (::gametic - spreeBreaker.spreeEndedTic > 4 * TICRATE ||
	    spreeBreaker.spreeEndedTic > ::gametic)
	{
		spreeBreaker = {"", -1, TEAM_NONE, "",    -1, TEAM_NONE,
		                "", "", CR_GOLD,   false, 0,  0};
	}

	for (auto& it : spreeRecord)
	{
		spree_record& record = it.second;

		if (::gametic - record.spreeStartTic > 4 * TICRATE ||
		    record.spreeStartTic > ::gametic)
		{
			spreeRecord.erase(it.first);
		}
	}
}

//
// SpreeManager::getLatestSpreeRecord
//
// Gets the latest spree record excluding the current player.
//
spree_record SpreeManager::getLatestSpreeRecord(int notPlayerId)
{
	spree_record record = {"null", -1, 0, 0, false};

	for (auto& it : spreeRecord)
	{
		if (it.first == notPlayerId)
			continue;

		if (it.second.spreeStartTic > record.spreeStartTic)
		{
			record = it.second;
		}
	}

	return record;
}

// ==========================================================
// Static functions start here.
// ==========================================================

/// <summary>
/// G_ProcessSprees occurs after a kill to determine
/// if this kill is an interval for a killing spree.
///
/// If so, show the spree text to the source player if he's the camera player.
/// Also, remove any spree from a player who was killed, and process any spree breakers.
/// </summary>
/// <param name="source">The killer (if a monster/player, null if environment/zombie projectile)</param>
/// <param name="target">The victim</param>
void P_ProcessSpreeKill(AActor* source, player_t* target)
{
	static SpreeManager& manager = SpreeManager::getInstance();

	if (target)
	{
		// If this player was on a spree, update it as the latest spree breaker.
		if (manager.hasSpree(target->id))
		{
			manager.setSpreeBreaker(source, target);
			manager.removeSpree(target->id);
		}

		target->killssincelastdeath = 0;
		target->damagesincelastdeath = 0;
	}

	if (!source || !source->player)
		return;

	source->player->killssincelastdeath += 1;

	// Check for spree interval, update the spree map with updates
	// If the gamemode isn't coop
	//if (G_IsCoopGame())
	//	return;

	bool update = manager.recordPlayerKill(source->player);

	if (displayplayer_id == source->player->id && update)
	{
		// Play the sound for the new multi kill
		//S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

/// <summary>
/// Handles internal ticking for spree bookkeeping.
/// </summary>
void P_TicSprees()
{
	//SpreeManager::getInstance().expireOldSprees();
}
