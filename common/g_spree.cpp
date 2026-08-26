// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
#include "p_inter.h"
#include "farchive.h"
#include "infomap.h"
#include "svc_message.h"

EXTERN_CVAR(sv_showsprees)

#ifdef CLIENT_APP
EXTERN_CVAR(cl_showsprees)
EXTERN_CVAR(cl_showofflinesprees)

namespace
{

//
// Whether spree messages should be shown to this player at all.
//
bool CanShowSprees()
{
	if (!cl_showsprees)
		return false;

	if (network_game)
		return sv_showsprees;

	return cl_showofflinesprees;
}

//
// Logs a spree message to the console.
//
void PrintSpreeMessage(const std::string& message, const int tic)
{
	if (message.empty() || !CanShowSprees())
		return;

	const int age = ::gametic - tic;

	if (age < 0 || age >= SPREE_DISPLAY_TICS)
		return;

	PrintFmt(PRINT_FILTERHIGH, TEXTCOLOR_GRAY "{}\n", message);
}

} // namespace
#endif

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
	emptyRecord = {"null", -1, 0, {"", "", "", CR_GRAY}, 0, false};
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
	spreeLevels.push_back({"Killing spree", "%k is on a %s!", "", CR_WHITE});// 5  kills / 5000 dmg
	spreeLevels.push_back({"Rampage", "%k is on a %s!", "", CR_BLUE});       // 10 kills / 10000 dmg
	spreeLevels.push_back({"Dominating", "%k is %s!", "", CR_GREEN});        // 15 kills / 15000 dmg
	spreeLevels.push_back({"Unstoppable", "%k is %s!", "", CR_YELLOW});      // 20 kills / 20000 dmg
	spreeLevels.push_back({"Untouchable", "%k is %s!", "", CR_CYAN});        // 25 kills / 25000 dmg
	spreeLevels.push_back({"Legendary", "%k is %s!", "", CR_GOLD});          // 30 kills / 30000 dmg

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

const Spree_s& SpreeManager::getSpreeLevel(const int level)
{
	const int highest = getHighestSpreeLevel();

	if (highest <= -1 || level < 0)
		return emptySpree;

	return spreeLevels.at(level > highest ? highest : level);
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

const SpreeBreaker_t& SpreeManager::getSpreeBreaker()
{
	return spreeBreaker;
}

void SpreeManager::setRawSpreeBreaker(const SpreeBreaker_t& breaker, const int level,
                                      const SpreeBreakerType breakerType,
                                      const int ticsAgo)
{
	player_t& victim = idplayer(breaker.spreeEndedPlayerId);
	SpreeBreaker_t newbreaker = breaker;

	// The victim may have already disconnected, in which case we still show the
	// breaker using the name the server sent us, just without a team color.
	newbreaker.spreeEndedTeam = validplayer(victim) ? victim.userinfo.team : TEAM_NONE;
	newbreaker.spreeEnderTeam = TEAM_NONE;
	newbreaker.spreeEndedTic = ::gametic - ticsAgo;
	newbreaker.spreeEndedLevel = level;
	newbreaker.spreeEndedType = breakerType;

	Spree_s spreeLevel = getSpreeLevel(level);

	newbreaker.spreeEnded = spreeLevel.spreeText;
	newbreaker.spreeEndedColor = spreeLevel.color;

	// Determine the type to figure out which localized broadcast string to serve
	switch (breakerType)
	{
		case BR_SELF:
				newbreaker.spreeEndedBroadcastText = spreeEndSelf;
				newbreaker.spreeEnderMonster = false;

				// The ender is the victim, so they share the same team.
				newbreaker.spreeEnderTeam = newbreaker.spreeEndedTeam;
		break;
	  case BR_PLAYER: {
				newbreaker.spreeEndedBroadcastText = spreeEndPlayer;
				newbreaker.spreeEnderMonster = false;

				player_t& source = idplayer(newbreaker.spreeEnderPlayerId);

				if (!validplayer(source))
						break;

				newbreaker.spreeEnderTeam = source.userinfo.team;
				}
		break;
		case BR_MONSTER:
		    newbreaker.spreeEndedBroadcastText = spreeEndMonster;
		    newbreaker.spreeEnderMonster = true;
		break;
	}

	setBreakerLanguage(newbreaker, breakerType);

#ifdef CLIENT_APP
	PrintSpreeMessage(newbreaker.spreeEndedBroadcastText, newbreaker.spreeEndedTic);
#endif

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
		enderTeam = endedTeam;
		broadcastText = spreeEndSelf;
		type = BR_SELF;
	}
	else if (source->player)
	{
		enderName = source->player->userinfo.netname;
		enderPlayerId = source->player->id;
		enderTeam = source->player->userinfo.team;
		broadcastText = spreeEndPlayer;
		type = BR_PLAYER;
	}
	else // potential monster
	{
		enderName = source->info->getDisplayName();
		enderPlayerId = -1;
		enderIsMonster = true;
		broadcastText = spreeEndMonster;
		type = BR_MONSTER;
	}

	spreeEnded = spreeLevel.spreeText;
	spreeEndedColor = spreeLevel.color;

	SpreeBreaker_t breaker = {endedPlayerName, endedPlayerId, endedTeam,

	                         enderName,       enderPlayerId, enderTeam,

	                         broadcastText,   spreeEnded,    spreeEndedColor,

	                         enderIsMonster,   points,       ::gametic,

	                         level,            type};


	#ifdef SERVER_APP
	if (sv_showsprees)
	{
		// Broadcast to all clients
		MSG_BroadcastSVC(CLBUF_NET, SVC_SpreeBreaker(breaker, level, type, 0), -1);
	}
	#endif

	setBreakerLanguage(breaker, type);

#ifdef CLIENT_APP
	PrintSpreeMessage(breaker.spreeEndedBroadcastText, breaker.spreeEndedTic);
#endif

	spreeBreaker = breaker;
}

bool SpreeManager::hasSpree(const int playerid)
{
	if (spreeRecord.find(playerid) == spreeRecord.end())
		return false;

	return true;
}

void SpreeManager::removeSpree(const int playerid)
{
	if (spreeRecord.find(playerid) == spreeRecord.end())
		return;

	spreeRecord.erase(playerid);
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
		newRecord.spreeLevel = newSpreeLevel;
		newRecord.spree = getSpreeLevel(newSpreeLevel);
		newRecord.spreeStartTic = tic;
		newRecord.stillDominating = newSpreeLevel > maxSpreeLevel;

		if (newRecord.stillDominating)
		{
			newRecord.spree.spreeBroadcastText = repeatingSpreeText;
		}

		// Apply sexmessage to the broadcast text
		setSpreeRecordLanguage(newRecord, playerId);

		spreeRecord[playerId] = newRecord;
#ifdef CLIENT_APP
		PrintSpreeMessage(newRecord.spree.spreeBroadcastText, tic);
#endif
#ifdef SERVER_APP
		if (sv_showsprees)
		{
			// Broadcast to all clients
			MSG_BroadcastSVC(CLBUF_NET, SVC_Spree(newRecord, 0), -1);
		}
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

			// Apply sexmessage to the broadcast text
			setSpreeRecordLanguage(record, playerId);
#ifdef CLIENT_APP
			PrintSpreeMessage(record.spree.spreeBroadcastText, tic);
#endif
#ifdef SERVER_APP
			if (sv_showsprees)
			{
				// Broadcast to all clients
				MSG_BroadcastSVC(CLBUF_NET, SVC_Spree(record, 0), -1);
			}
#endif
			return true;
		}
	}

	return false;
}

bool SpreeManager::setRawSpree(const int playerId, const int newSpreeLevel,
                               const int ticsAgo)
{
	if (newSpreeLevel <= -1)
		return false;

	player_t& player = idplayer(playerId);

	if (!validplayer(player))
		return false;

	return checkForSpreeUpdates(playerId, player.userinfo.netname, newSpreeLevel,
	                            ::gametic - ticsAgo);
}

const std::unordered_map<int, SpreeRecord_t>& SpreeManager::getSpreeRecords() const
{
	return spreeRecord;
}

const SpreeRecord_t& SpreeManager::getSpreeRecord(const int playerId)
{
	if (spreeRecord.find(playerId) != spreeRecord.end())
	{
		return spreeRecord[playerId];
	}

	return emptyRecord;
}

void SpreeManager::expireOldSprees()
{
	// Drop the last spree breaker once it can no longer be shown, or if it happened
	// in the future, which means we rewound a demo.
	if (spreeBreaker.spreeEndedPlayerId != -1 &&
	    (::gametic - spreeBreaker.spreeEndedTic > SPREE_DISPLAY_TICS ||
	     spreeBreaker.spreeEndedTic > ::gametic))
	{
		spreeBreaker = SpreeBreaker_t();
	}

	std::erase_if(spreeRecord, [](const auto& pair){
		// Spree happened in the future, indicating we're in a rewinded demo
		// Remove it
		return pair.second.spreeStartTic > ::gametic;
	});
}

const SpreeRecord_t& SpreeManager::getLatestSpreeRecord(const int notPlayerId)
{
	if (spreeRecord.empty())
		return emptyRecord;

	auto it = std::max_element(spreeRecord.begin(), spreeRecord.end(),
	                           [notPlayerId](const auto& a, const auto& b) {
		                           if (a.first == notPlayerId)
			                           return true;
		                           if (b.first == notPlayerId)
			                           return false;
		                           return a.second.spreeStartTic < b.second.spreeStartTic;
	                           });

	if (it == spreeRecord.end() || it->first == notPlayerId)
		return emptyRecord;

	return it->second;
}

void SpreeManager::setSpreeRecordLanguage(SpreeRecord_t& record, const int playerId)
{
	// Apply sexmessage to the broadcast text
	std::string playerColor = TEXTCOLOR_GOLD;
	char gendermessage[1024];
	gender_t gender = GENDER_OTHER;

	if (validplayer(idplayer(playerId)))
	{
		player_t& player = idplayer(playerId);
		gender = player.userinfo.gender;

		if (G_IsTeamGame())
		{
			TeamInfo* info = GetTeamInfo(player.userinfo.team);
			playerColor = info->ToastColor;
		}
	}

	SexMessage(record.spree.spreeBroadcastText.c_str(), gendermessage, gender, "",
	           playerColor + record.playerName + TEXTCOLOR_NORMAL,
	           TextColorFromRange(record.spree.color) + record.spree.spreeText +
	               TEXTCOLOR_NORMAL);

	record.spree.spreeBroadcastText = gendermessage;
}

void SpreeManager::setBreakerLanguage(SpreeBreaker_t& breaker,
                                      const SpreeBreakerType type)
{
	std::string endedPlayerColor = TEXTCOLOR_GOLD;
	std::string enderPlayerColor = TEXTCOLOR_GOLD;

	int enderPlayerId = breaker.spreeEnderPlayerId;

	if (G_IsTeamGame())
	{
		TeamInfo* endedinfo = GetTeamInfo(breaker.spreeEndedTeam);
		endedPlayerColor = endedinfo->ToastColor;

		TeamInfo* enderinfo = GetTeamInfo(breaker.spreeEnderTeam);
		enderPlayerColor = enderinfo->ToastColor;
	}

	char gendermessage[1024];
	gender_t gender = GENDER_OTHER;

	const player_t& endedPlayer = idplayer(breaker.spreeEndedPlayerId);

	if (validplayer(endedPlayer))
	{
		gender = endedPlayer.userinfo.gender;
	}

	// Replace any possible gender or victim/killer/spree text with gendered text
	SexMessage(breaker.spreeEndedBroadcastText.c_str(), gendermessage, gender,
	           endedPlayerColor + breaker.spreeEndedName + TEXTCOLOR_NORMAL,
	           enderPlayerColor + breaker.spreeEnderName + TEXTCOLOR_NORMAL,
	           TextColorFromRange(breaker.spreeEndedColor) + breaker.spreeEnded +
	               TEXTCOLOR_NORMAL);

	breaker.spreeEndedBroadcastText = gendermessage;

	//// Get final points and add to the message
	// int pts = breaker.endedPoints;

	//// Insert commas for every 3 digits
	// std::string formattedPts = std::to_string(pts);

	// for (int i = formattedPts.size() - 3; i > 0; i -= 3)
	//{
	//	formattedPts.insert(i, ",");
	// }

	// std::string pointsType = "";

	// if (G_IsCoopGame())
	//{
	//	pointsType = "dmg";
	// }
	// else
	//{
	//	pointsType = "frags";
	// }

	// std::string ptsStr = fmt::sprintf(" (%s %s)", formattedPts, pointsType.c_str());

	// msg += ptsStr;
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
}

void SpreeManager::serialize(FArchive& arc)
{
	if (arc.IsStoring())
	{
		arc << static_cast<uint32_t>(spreeRecord.size());

		for (const auto& [playerId, record] : spreeRecord)
		{
			arc << playerId << record.playerName << record.spreeLevel
			    << record.spreeStartTic << record.stillDominating << record.spree.spreeText
			    << record.spree.spreeBroadcastText << record.spree.gameSfxToken
			    << record.spree.color;
		}

		arc << spreeBreaker.spreeEndedName << spreeBreaker.spreeEndedPlayerId
		    << spreeBreaker.spreeEndedTeam << spreeBreaker.spreeEnderName
		    << spreeBreaker.spreeEnderPlayerId << spreeBreaker.spreeEnderTeam
		    << spreeBreaker.spreeEndedBroadcastText << spreeBreaker.spreeEnded
		    << spreeBreaker.spreeEndedColor << spreeBreaker.spreeEnderMonster
		    << spreeBreaker.endedPoints << spreeBreaker.spreeEndedTic
		    << spreeBreaker.spreeEndedLevel << spreeBreaker.spreeEndedType;
	}
	else
	{
		spreeRecord.clear();

		uint32_t count = 0;
		arc >> count;

		for (uint32_t i = 0; i < count; i++)
		{
			SpreeRecord_t record;
			arc >> record.playerId >> record.playerName >> record.spreeLevel >>
			    record.spreeStartTic >> record.stillDominating >> record.spree.spreeText >>
			    record.spree.spreeBroadcastText >> record.spree.gameSfxToken >>
			    record.spree.color;

			spreeRecord[record.playerId] = record;
		}

		arc >> spreeBreaker.spreeEndedName >> spreeBreaker.spreeEndedPlayerId >>
		    spreeBreaker.spreeEndedTeam >> spreeBreaker.spreeEnderName >>
		    spreeBreaker.spreeEnderPlayerId >> spreeBreaker.spreeEnderTeam >>
		    spreeBreaker.spreeEndedBroadcastText >> spreeBreaker.spreeEnded >>
		    spreeBreaker.spreeEndedColor >> spreeBreaker.spreeEnderMonster >>
		    spreeBreaker.endedPoints >> spreeBreaker.spreeEndedTic >>
		    spreeBreaker.spreeEndedLevel >> spreeBreaker.spreeEndedType;
	}
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

	// Don't count if the player killed themselves, or if the player killed a teammate.
	if (target && target->id == source->player->id)
		return;

	if (target && G_IsTeamGame() && source->player &&
	    source->player->userinfo.team == target->userinfo.team)
		return;

	if (clientside && network_game)
		return;

	// Check for spree interval, update the spree map with updates
	// If the gamemode isn't coop
	if (G_IsCoopGame())
		return;

	bool update = manager.recordPlayerKill(source->player);

#ifdef CLIENT_APP
	// Don't announce sprees if the client has showing them disabled
	if (!cl_showsprees || (!cl_showofflinesprees && !network_game) ||
	    (!sv_showsprees && network_game))
		return;
#endif

	if (displayplayer_id == source->player->id && update)
	{
		// Play the game sfx first.
		const SpreeRecord_t& record = manager.getSpreeRecord(source->player->id);
		// Play the gamesfx sound from the multi kill first.
		if (!record.spree.gameSfxToken.empty() &&
		    S_FindSound(record.spree.gameSfxToken.c_str()) != -1)
			S_Sound(CHAN_GAMEINFO, record.spree.gameSfxToken.c_str(), 1, ATTN_NONE);

		// Play the announcer sound for the new multi kill
		// S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

void P_ProcessSpreeDamage(const player_t* source, const AActor* target,
                          const int totalDamage)
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

	// Chipping away at our own friendly monsters doesn't count, the same way killing a
	// teammate doesn't count towards a kill spree or multi kill.
	if (P_IsFriendlyDamage(source, target))
		return;

	bool update = manager.recordPlayerDamage(source, totalDamage);

#ifdef CLIENT_APP
	// Don't announce sprees if the client has showing them disabled
	if (!cl_showsprees || (!cl_showofflinesprees && !network_game) ||
	    (!sv_showsprees && network_game))
		return;
#endif

	if (displayplayer_id == source->id && update)
	{
		// Play the game sfx first.
		const SpreeRecord_t& record = manager.getSpreeRecord(source->id);
		// Play the gamesfx sound from the multi kill first.
		if (!record.spree.gameSfxToken.empty() &&
		    S_FindSound(record.spree.gameSfxToken.c_str()) != -1)
			S_Sound(CHAN_GAMEINFO, record.spree.gameSfxToken.c_str(), 1, ATTN_NONE);

		// Play the announcer sound for the new multi kill
		// S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

SpreeHudLines_t P_GetSpreeHudLines(const int displayPlayerId)
{
	SpreeManager& manager = SpreeManager::getInstance();
	SpreeHudLines_t lines;

	// A spree for the player we're watching is the big line, and is never echoed on
	// the small line. Repeats of the top level are the one exception - those are only
	// ever announced small.
	const SpreeRecord_t& ownSpree = manager.getSpreeRecord(displayPlayerId);
	const bool hasOwnSpree = ownSpree.playerId != -1;

	int smallTic = -1;

	if (hasOwnSpree)
	{
		if (ownSpree.stillDominating)
		{
			lines.smallSpree = &ownSpree;
			smallTic = ownSpree.spreeStartTic;
		}
		else
		{
			lines.bigSpree = &ownSpree;
		}
	}

	// The small line only has room for one message, so take the latest of whichever of
	// our own repeated spree, another player's spree, or the last spree breaker.
	const SpreeRecord_t& otherSpree = manager.getLatestSpreeRecord(displayPlayerId);

	if (otherSpree.playerId != -1 && otherSpree.spreeStartTic > smallTic)
	{
		lines.smallSpree = &otherSpree;
		smallTic = otherSpree.spreeStartTic;
	}

	const SpreeBreaker_t& breaker = manager.getSpreeBreaker();

	if (breaker.spreeEndedPlayerId != -1 && breaker.spreeEndedTic > smallTic)
	{
		lines.smallSpree = nullptr;
		lines.smallBreaker = &breaker;
	}

	return lines;
}

void P_TicSprees()
{
	SpreeManager::getInstance().expireOldSprees();
}

void P_SerializeSprees(FArchive& arc)
{
	SpreeManager::getInstance().serialize(arc);
}
