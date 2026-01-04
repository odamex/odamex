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
//   Handle the loading of spree data from SPREEDEF,
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
#include "svc_message.h"

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
	spreeBreaker = SpreeBreaker_t();
	repeatingSpreeText = "";
	spreeEndPlayer = "";
	spreeEndSelf = "";
	spreeEndMonster = "";
	emptyRecord = {"null", -1, 0, {"", "", CR_GRAY}, 0, false};
	emptySpree = Spree_s();
}

void SpreeManager::reset()
{
	spreeLevels.clear();
	spreeRecord.clear();
	pointsSinceLastDeath.clear();
	spreeBreaker = SpreeBreaker_t();
	spreeKillInterval = 5;
	spreeDamageInterval = 10000;
	repeatingSpreeText = "";
	spreeEndPlayer = "";
	spreeEndSelf = "";
	spreeEndMonster = "";
}

void SpreeManager::clearSprees()
{
	spreeRecord.clear();
	pointsSinceLastDeath.clear();
	spreeBreaker = SpreeBreaker_t();
}

void SpreeManager::loadSpreeDefaults()
{
	spreeLevels.clear();
	spreeLevels.push_back({"Killing spree", "%k is on a %s!", CR_WHITE});// 5  kills / 5000 dmg
	spreeLevels.push_back({"Rampage", "%k is on a %s!", CR_BLUE});       // 10 kills / 10000 dmg
	spreeLevels.push_back({"Dominating", "%k is %s!", CR_GREEN});        // 15 kills / 15000 dmg
	spreeLevels.push_back({"Unstoppable", "%k is %s!", CR_YELLOW});      // 20 kills / 20000 dmg
	spreeLevels.push_back({"Untouchable", "%k is %s!", CR_CYAN});        // 25 kills / 25000 dmg
	spreeLevels.push_back({"Legendary", "%k is %s!", CR_GOLD});          // 30 kills / 30000 dmg

	repeatingSpreeText = "%k is STILL %s!";

	spreeEndPlayer = "%o's %s was ended by %k";
	spreeEndSelf = "%k was looking good until %g killed %hself!";
	spreeEndMonster = "%o's %s was ended by a %k!";

	spreeKillInterval = 5;
	spreeDamageInterval = 5000;
}

int SpreeManager::getSpreeKillInterval()
{
	return spreeKillInterval;
}

int SpreeManager::getSpreeDamageInterval()
{
	return spreeDamageInterval;
}

int SpreeManager::getHighestSpreeLevel()
{
	return spreeLevels.size() - 1;
}

Spree_s& SpreeManager::getSpreeLevel(const int level)
{
	int newlevel = level;

	if (getHighestSpreeLevel() <= -1)
		return emptySpree;

	if (level >= spreeLevels.size())
		newlevel = spreeLevels.size() - 1;

	return spreeLevels.at(newlevel);
}

void SpreeManager::setSpreeLevels(const NewSprees_s& newSprees)
{
	spreeLevels = newSprees.newSprees;
	spreeKillInterval = newSprees.newKillInterval;
	spreeDamageInterval = newSprees.newDamageInterval;
	spreeEndPlayer = newSprees.newSpreeEndPlayer;
	spreeEndSelf = newSprees.newSpreeEndSelf;
	spreeEndMonster = newSprees.newSpreeEndMonster;
	repeatingSpreeText = newSprees.newRepeatingSpreeText;
}

SpreeBreaker_t& SpreeManager::getSpreeBreaker()
{
	return spreeBreaker;
}

void SpreeManager::setRawSpreeBreaker(const SpreeBreaker_t& breaker, const int level, const SpreeBreakerType breakerType)
{
	player_t& victim = idplayer(breaker.spreeEndedPlayerId);
	SpreeBreaker_t newbreaker = breaker;

	if (!validplayer(victim))
		return;

	newbreaker.spreeEndedTeam = victim.userinfo.team;
	newbreaker.spreeEnderTeam = TEAM_NONE;
	newbreaker.spreeEndedTic = ::gametic;

	Spree_s spreeLevel = getSpreeLevel(level);

	newbreaker.spreeEnded = spreeLevel.spreeText;
	newbreaker.spreeEndedColor = spreeLevel.color;

	// Determine the type to figure out which localized broadcast string to serve
	switch (breakerType)
	{
		case BR_SELF:
				newbreaker.spreeEndedBroadcastText = spreeEndSelf;
				newbreaker.spreeEnderMonster = false;
		break;
	  case BR_PLAYER: {
				newbreaker.spreeEndedBroadcastText = spreeEndPlayer;
				newbreaker.spreeEnderMonster = false;

				player_t& source = idplayer(newbreaker.spreeEnderPlayerId);

				if (!validplayer(source))
						break;

				newbreaker.spreeEndedTeam = source.userinfo.team;
				}
		break;
		case BR_MONSTER:
		    newbreaker.spreeEndedBroadcastText = spreeEndMonster;
		    newbreaker.spreeEnderMonster = true;
		break;
	}

	spreeBreaker = newbreaker;
}

void SpreeManager::setSpreeBreaker(const AActor* source, const player_t* target)
{
	if (clientside && network_game)
		return;

	// No player no spree
	if (!target)
		return;

	std::string endedPlayerName = target->userinfo.netname;
	int endedPlayerId = target->id;
	team_t endedTeam = target->userinfo.team;

	SpreeBreakerType type = BR_SELF;

	int points = getPoints(endedPlayerId);
	int level = 0;

	if (G_IsCoopGame())
	{
		level = getSpreeLevelByDamage(points);
	}
	else
	{
		level = getSpreeLevelByKills(points);
	}

	Spree_s spreeLevel = getSpreeLevel(level);

	std::string enderName = "";
	int enderPlayerId = 0;
	team_t enderTeam = TEAM_NONE;
	bool enderIsMonster = false;

	std::string broadcastText = "";
	std::string spreeEnded = "";
	EColorRange spreeEndedColor = CR_GOLD;

	// no source? treat it as a self kill
	// and if its a self kill, treat it as such
	if (!source || (source->player && source->player->id == target->id))
	{
		enderName = endedPlayerName;
		enderPlayerId = endedPlayerId;
		broadcastText = spreeEndSelf;
		type = BR_SELF;
	}
	else if (source->player)
	{
		enderName = source->player->userinfo.netname;
		enderPlayerId = source->player->id;
		team_t enderTeam = source->player->userinfo.team;
		broadcastText = spreeEndPlayer;
		type = BR_PLAYER;
	}
	else // potential monster
	{
		enderName = P_MobjToName(static_cast<mobjtype_t>(source->type));
		enderIsMonster = true;
		broadcastText = spreeEndMonster;
		type = BR_MONSTER;
	}

	spreeEnded = spreeLevel.spreeText;
	spreeEndedColor = spreeLevel.color;

	SpreeBreaker_t breaker = {endedPlayerName, endedPlayerId, endedTeam,

	                         enderName,       enderPlayerId, enderTeam,

	                         broadcastText,   spreeEnded,    spreeEndedColor,

	                         enderIsMonster,

	                         points,

	                         ::gametic};

	#ifdef SERVER_APP
	// Broadcast to all clients
	MSG_BroadcastSVC(CLBUF_NET, SVC_SpreeBreaker(breaker, level, type), -1);
	#endif

	spreeBreaker = breaker;

	return;
}

bool SpreeManager::hasSpree(const int playerid)
{
	if (spreeRecord.find(playerid) == spreeRecord.end())
		return false;
	else if (spreeRecord.find(playerid) != spreeRecord.end())
		return true;

	return false;
}

void SpreeManager::removeSpree(const int playerid)
{
	if (spreeRecord.find(playerid) == spreeRecord.end())
		return;

	spreeRecord.erase(playerid);
	return;
}

int SpreeManager::getSpreeLevelByKills(const int kills)
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

int SpreeManager::getSpreeLevelByDamage(const int damage)
{
	if (spreeLevels.size() == 0 || spreeDamageInterval <= 0)
		return -1;

	// Still on the first 10000 damage?
	if (damage < spreeDamageInterval)
		return -1;

	int level = damage / spreeDamageInterval;

	if (level <= 0)
		return -1;

	return level - 1;
}

bool SpreeManager::recordPlayerKill(const player_t* player)
{
	if (!player)
		return false;

	addPoints(player->id, 1);

	int newSpreeLevel = getSpreeLevelByKills(getPoints(player->id));

	return checkForSpreeUpdates(player->id, player->userinfo.netname, newSpreeLevel, ::gametic);
}

bool SpreeManager::recordPlayerDamage(const player_t* player, const int totalDamage)
{
	if (!player)
		return false;

	addPoints(player->id, totalDamage);

	int newSpreeLevel = getSpreeLevelByDamage(getPoints(player->id));

	return checkForSpreeUpdates(player->id, player->userinfo.netname, newSpreeLevel, ::gametic);
}

bool SpreeManager::checkForSpreeUpdates(const int playerId, const std::string playerName, const int newSpreeLevel, const int tic) {
	int maxSpreeLevel = getHighestSpreeLevel();

	if (newSpreeLevel <= -1)
		return false;

	if (spreeRecord.find(playerId) == spreeRecord.end())
	{
		// Spree record not found, create it (if necessary)
		SpreeRecord_t newRecord;
		newRecord.playerId = playerId;
		newRecord.playerName = playerName;
		newRecord.spreeLevel =
		    newSpreeLevel > maxSpreeLevel ? maxSpreeLevel : newSpreeLevel;
		newRecord.spree = getSpreeLevel(newRecord.spreeLevel);
		newRecord.spreeStartTic = tic;
		newRecord.stillDominating = false;
		spreeRecord[playerId] = newRecord;
#ifdef SERVER_APP
		// Broadcast to all clients
		MSG_BroadcastSVC(CLBUF_NET, SVC_Spree(newRecord), -1);
#endif
		return true;
	}
	else
	{
		// Spree record found, check if we can upgrade
		SpreeRecord_t& record = spreeRecord[playerId];

		if (newSpreeLevel > record.spreeLevel)
		{
			// Upgrade spree level
			record.spreeLevel = newSpreeLevel;
			record.spreeStartTic = tic;
			record.spree = getSpreeLevel(record.spreeLevel);
			record.stillDominating = newSpreeLevel > maxSpreeLevel ? true : false;

			if (record.stillDominating)
			{
				record.spree.spreeBroadcastText = repeatingSpreeText;
			}
#ifdef SERVER_APP
			// Broadcast to all clients
			MSG_BroadcastSVC(CLBUF_NET, SVC_Spree(record), -1);
#endif
			return true;
		}
	}

	return false;
}

bool SpreeManager::setRawSpree(const int playerId, const int newSpreeLevel)
{
	if (newSpreeLevel <= -1)
		return false;

	player_t& player = idplayer(playerId);

	if (!validplayer(player))
		return false;

	return checkForSpreeUpdates(playerId, player.userinfo.netname, newSpreeLevel, ::gametic);
}

SpreeRecord_t& SpreeManager::getSpreeRecord(const int playerId)
{
	if (spreeRecord.find(playerId) != spreeRecord.end())
	{
		return spreeRecord[playerId];
	}

	return emptyRecord;
}

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
		SpreeRecord_t& record = it.second;

		if (::gametic - record.spreeStartTic > 4 * TICRATE ||
		    record.spreeStartTic > ::gametic)
		{
			spreeRecord.erase(it.first);
		}
	}
}

SpreeRecord_t& SpreeManager::getLatestSpreeRecord(const int notPlayerId)
{
	SpreeRecord_t record = emptyRecord;

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
// Spree kill bookkeeping functions start here.
// ==========================================================

void SpreeManager::addPoints(const int playerid, const int points)
{
	if (pointsSinceLastDeath.find(playerid) == pointsSinceLastDeath.end())
	{
		pointsSinceLastDeath[playerid] = points;
	}
	else
	{
		pointsSinceLastDeath[playerid] += points;
	}
}

void SpreeManager::erasePoints(const int playerid)
{
	if (pointsSinceLastDeath.find(playerid) == pointsSinceLastDeath.end())
		return;

	pointsSinceLastDeath.erase(playerid);
	return;
}

int SpreeManager::getPoints(const int playerid)
{
	if (pointsSinceLastDeath.find(playerid) == pointsSinceLastDeath.end())
	{
		return 0;
	}
	else
	{
		return pointsSinceLastDeath[playerid];
	}
}

void SpreeManager::clearPoints()
{
	pointsSinceLastDeath.clear();
	return;
}

// ==========================================================
// Static functions start here.
// ==========================================================

void P_ProcessSpreeKill(const AActor* source, const player_t* target)
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

		manager.erasePoints(target->id);
	}

	if (!source || !source->player)
		return;

	if (clientside && network_game)
		return;

	// Check for spree interval, update the spree map with updates
	// If the gamemode isn't coop
	if (G_IsCoopGame())
		return;

	bool update = manager.recordPlayerKill(source->player);

	if (displayplayer_id == source->player->id && update)
	{
		// Play the sound for the new multi kill
		// S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

void P_ProcessSpreeDamage(const player_t* source, const int totalDamage)
{
	if (clientside && network_game)
		return;

	static SpreeManager& manager = SpreeManager::getInstance();

	if (!source)
		return;

	// Check for spree interval, update the spree map with updates
	// If the gamemode is coop
	if (!G_IsCoopGame())
		return;

	bool update = manager.recordPlayerDamage(source, totalDamage);

	if (displayplayer_id == source->id && update)
	{
		// Play the sound for the new multi kill
		// S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

void P_TicSprees()
{
	// SpreeManager::getInstance().expireOldSprees();
}
