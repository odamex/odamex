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

struct spree_s
{
  std::string spreeText;
  std::string spreeBroadcastText;
  EColorRange color;
};

struct spree_record
{
  std::string playerName;
  int playerId;
  int spreeLevel;
  int spreeStartTic;
  bool stillDominating;
};

struct spree_breaker
{
  std::string spreeEndedName;
  int spreeEndedPlayerId;
  team_t spreeEndedTeam;

  std::string spreeEnderName;
  int spreeEnderPlayerId;
  team_t spreeEnderTeam;
  bool spreeEnderMonster;

  int spreeEndedTic;
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

    void setSpreeLevels(const std::vector<spree_s> sprees,
                        int newKillinterval,    // called when reading SPREEDEF to
                        int newDamageInterval); // input new spree definitions

    spree_s getSpreeLevel(
          int level); // Gets the local spree level (with text and color)

		int getSpreeLevelByKills(int kills); // Gets the spree level by the amount of kills the player has.

		int getSpreeLevelByDamage(int damage); // Gets the spree level by the amount of damage the player has.

    int getSpreeInterval(); // gets the multi kill interval for players

    int getHighestSpreeLevel(); // gets the highest multi kill level loaded

    void loadSpreeDefaults(); // called if no SPREEDEF is found

    spree_record getSpreeRecord(int playerId); // gets a current spree for a player

    spree_record getLatestSpreeRecord(int notPlayerId); // gets the latest spree excluding the current player

    bool recordPlayerKill(const player_t* player); // Records a single kill for a player

    spree_breaker getSpreeBreaker(); // gets the current spree breaker

    void setSpreeBreaker(AActor* source,
                         player_t* target); // sets the current spree breaker

    bool hasSpree(const int playerid);
    void removeSpree(const int playerid);

  private:
    int spreeKillInterval; // PVP
    int spreeDamageInterval; // PVE

    std::vector<spree_s> spreeLevels; // Levels of sprees configured during wad load

    std::unordered_map<int, spree_record> spreeRecord; // Actually controls the spree display for the player.
    spree_breaker spreeBreaker; // Updated with the last spree to be broken.
};

void G_ProcessSpreeKill(AActor* source, player_t* target);
void G_TicSprees();
