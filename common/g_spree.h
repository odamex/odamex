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
//   spree events.
//
//-----------------------------------------------------------------------------

#pragma once
#include <string>
#include <vector>
#include <map>

enum SpreeBreakerType
{
	BR_SELF,
	BR_PLAYER,
	BR_MONSTER
};

struct Spree_s
{
	std::string spreeText;
	std::string spreeBroadcastText;
	EColorRange color;
	Spree_s() : spreeText(""), spreeBroadcastText(""), color(CR_GRAY) { }
	Spree_s(std::string SpreeText, std::string BroadcastText, EColorRange Color)
	    : spreeText(SpreeText), spreeBroadcastText(BroadcastText), color(Color)
	{
	}
};

struct SpreeRecord_t
{
	std::string playerName;
	int playerId;
	int spreeLevel;
	Spree_s spree;
	int spreeStartTic;
	bool stillDominating;
	SpreeRecord_t() : playerName(""), playerId(-1), spreeLevel(-1), spree(), spreeStartTic(0), stillDominating(false) { }
	SpreeRecord_t(std::string PlayerName, int PlayerId, int SpreeLevel, Spree_s Spree, int SpreeStartTic, bool StillDominating)
	    : playerName(PlayerName), playerId(PlayerId), spreeLevel(SpreeLevel),
	      spree(Spree), spreeStartTic(SpreeStartTic), stillDominating(StillDominating)
	{
	}
};

struct SpreeBreaker_t
{
	std::string spreeEndedName;
	int spreeEndedPlayerId;
	team_t spreeEndedTeam;

	std::string spreeEnderName;
	int spreeEnderPlayerId;
	team_t spreeEnderTeam;

	std::string spreeEndedBroadcastText;
	std::string spreeEnded;
	EColorRange spreeEndedColor;

	bool spreeEnderMonster;

	int endedPoints;

	int spreeEndedTic;
	SpreeBreaker_t()
	    : spreeEndedName(""), spreeEndedPlayerId(-1), spreeEndedTeam(TEAM_NONE),
	      spreeEnderName(""), spreeEnderPlayerId(-1), spreeEnderTeam(TEAM_NONE),
	      spreeEndedBroadcastText(""), spreeEnded(""), spreeEndedColor(CR_GRAY),
	      spreeEnderMonster(false), endedPoints(0), spreeEndedTic(0)
	{
	}
	SpreeBreaker_t(
			std::string SpreeEndedName, int SpreeEndedPlayerId, team_t SpreeEndedTeam,
			std::string SpreeEnderName, int SpreeEnderPlayerId, team_t SpreeEnderTeam,
			std::string SpreeEndedBroadcastText, std::string SpreeEnded, EColorRange SpreeEndedColor,
			bool SpreeEnderMonster, int EndedPoints, int SpreeEndedTic)
	    : spreeEndedName(SpreeEndedName), spreeEndedPlayerId(SpreeEndedPlayerId),
	      spreeEndedTeam(SpreeEndedTeam), spreeEnderName(SpreeEnderName),
	      spreeEnderPlayerId(SpreeEnderPlayerId), spreeEnderTeam(SpreeEnderTeam),
	      spreeEndedBroadcastText(SpreeEndedBroadcastText), spreeEnded(SpreeEnded),
	      spreeEndedColor(SpreeEndedColor), spreeEnderMonster(SpreeEnderMonster),
	      endedPoints(EndedPoints), spreeEndedTic(SpreeEndedTic)
	{
	}
};

class SpreeManager
{
public:
	SpreeManager();
	~SpreeManager();
	static SpreeManager& getInstance(); // returns the instantiated SpreeManager
	                                    // object

	void reset(); // called when loading a new wad

	void clearSprees(); // called when entering a new map or game

	void setSpreeLevels(const std::vector<Spree_s> sprees,
	                    int newKillinterval,    // called when reading SPREEDEF to
	                    int newDamageInterval); // input new spree definitions

	void loadSpreeDefaults(); // called if no SPREEDEF is found

	void expireOldSprees(); // Runs every tic to clean up old sprees.

	SpreeRecord_t getSpreeRecord(int playerId); // gets a current spree for a player

	SpreeRecord_t getLatestSpreeRecord(
	    int notPlayerId); // gets the latest spree excluding the current player

	bool recordPlayerKill(const player_t* player); // Records a single kill for a player

	bool recordPlayerDamage(const player_t* player); // Records damage for a player.

	SpreeBreaker_t getSpreeBreaker(); // gets the current spree breaker

	void setSpreeBreaker(AActor* source,
	                     player_t* target); // sets the current spree breaker
	                                        // and runs logic to see what kind it is.

	void setRawSpreeBreaker(
	    SpreeBreaker_t& breaker,
			const int level,
	    SpreeBreakerType type); // sets the spree breaker using raw data
	                            // (used when receiving from server)

	bool setRawSpree(
	    const int playerId, const int newSpreeLevel,
	    const int tic); // sets a players spree using raw data
	                    // (used when receiving from server)

	bool hasSpree(const int playerid);
	void removeSpree(const int playerid);

private:
	int getSpreeLevelByKills(
	    int kills); // Gets the spree level by the amount of kills the player has.

	int getSpreeLevelByDamage(
	    int damage); // Gets the spree level by the amount of damage the player has.

	bool checkForSpreeUpdates(const int playerId, const std::string playerName,
	                          const int newSpreeLevel, const int tic); // Gets the spree level by the
	                                                                   // amount of kills the player has.

	Spree_s getSpreeLevel(int level); // Gets the local spree level (with text and color)

	int getSpreeKillInterval(); // gets the spree kill interval for players

	int getSpreeDamageInterval(); // gets the spree kill interval for players

	int getHighestSpreeLevel(); // gets the highest spree level loaded

	int spreeKillInterval;   // PVP
	int spreeDamageInterval; // PVE

	std::vector<Spree_s> spreeLevels; // Levels of sprees configured during wad load

	std::string repeatingSpreeText; // Text to display when a player is repeating their
	                                // highest spree level.

	std::string
	    spreeEndPlayer; // Text for when a spree has ended by the hands of another player.

	std::string spreeEndSelf; // Text for when a spree has ended by suicide. Whoopsie!

	std::string spreeEndMonster; // Text for when a spree has ended by a monster.

	std::unordered_map<int, SpreeRecord_t>
	    spreeRecord;            // Actually controls the spree display for the player.
	SpreeBreaker_t spreeBreaker; // Updated with the last spree to be broken.
};

void P_ProcessSpreeKill(AActor* source, player_t* target);
void P_ProcessSpreeDamage(player_t* source, int totalDamage);
void P_TicSprees();
