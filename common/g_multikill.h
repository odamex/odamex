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

#pragma once
#include <string>
#include <vector>

#include "d_player.h"

/// <summary>
/// Structure to represent a level of multi kill.
/// </summary>
struct MultiKillLevel_s
{
	std::string multikilltext;
	EColorRange color;
	MultiKillLevel_s() : multikilltext(""), color(CR_GRAY) { }
	MultiKillLevel_s(std::string MultiKillText, EColorRange Color)
	    : multikilltext(MultiKillText), color(Color)
	{
	}
};

/// <summary>
/// A record of a player's multi kill status.
/// </summary>
struct MultiKillTics_s
{
	int ticsRemaining;
	int multiKills;
	int lastKillTime;

	MultiKillTics_s() : ticsRemaining(0), multiKills(0), lastKillTime(0) { }
	MultiKillTics_s(int TicsRemaining, int MultiKills, int LastKillTime)
	    : ticsRemaining(TicsRemaining), multiKills(MultiKills), lastKillTime(LastKillTime)
	{
	}
};

/// <summary>
/// A singleton class to manage multi kill levels and intervals, and bookkeeping
/// of player multi kill statuses.
/// </summary>
class MultiKillManager
{
public:
	MultiKillManager();
	~MultiKillManager();

	/// <summary>
	/// Gets the only instance of this singleton class.
	/// </summary>
	/// <returns>A pointer to the only allowable MultiKillManager object.</returns>
	static MultiKillManager& getInstance();

	/// <summary>
	/// Resets the status of the manager, called when loading a new wad with a SPREEDEF
	/// lump.
	/// </summary>
	void reset();

	/// <summary>
	/// Creates a new MultiKillLevel list and interval,
	/// as if reading a SPREEDEF to create a new multi kill level paradigm.
	/// </summary>
	/// <param name="multikills">The completed MultiKillLevels in order in a vector.</param>
	/// <param name="newinterval">Multi kill interval in seconds.</param>
	void setMultiKillLevels(const std::vector<MultiKillLevel_s> multikills, int newinterval);

	/// <summary>
	/// Gets the highest multi kill level.
	/// If higher than the max level, get the highest one.
	/// If the array isnt populated, return it empty.
	/// </summary>
	/// <param name="level">Multi kill level to return detailed information
	/// for.</param> <returns>The multi kill level specified, or empty if
	/// invalid.</returns>
	MultiKillLevel_s getMultiKillLevel(int level);

	/// <summary>
	/// Sets defaults for loading multi kills. Typically runs if a SPREEDEF is not found.
	/// </summary>
	void loadMultiKillDefaults();

	/// <summary>
	/// Gets the current multi kill status (kills, and tics) for a player.
	/// </summary>
	/// <param name="playerid">ID of the player to get current multi kill status.</param>
	/// <returns>Multi kill status for the specified player id, or empty if invalid.</returns>
	MultiKillTics_s getMultiKills(const int playerid);

	/// <summary>
	/// Adds a single kill to a player's current multi kill
	/// </summary>
	/// <param name="playerid">Player ID of the player to add a kill to.</param>
	void addKill(const int playerid);

	/// <summary>
	/// Tics a player's multi kills
	/// </summary>
	/// <param name="playerid">Player ID of the player to tic a multi kill status.</param>
	void ticPlayerMultiKill(const int playerid);

	/// <summary>
	/// Resets player multi kill tics and kills after death
	/// </summary>
	/// <param name="playerid">Player ID of the player to erase multi kill status.</param>
	void eraseMultiKills(const int playerid);

	/// <summary>
	/// Resets everyones multi kill tics and kills after map
	/// change/restart/new round
	/// </summary>
	void clearMultiTics();
private:
	/// <summary>
	/// Gets the multi kill interval to trigger multi kills.
	/// </summary>
	/// <returns>The multi kill interval.</returns>
	int getMultiKillInterval();

	/// <summary>
	/// Gets the highest multi kill level.
	/// </summary>
	/// <returns>The highest multi kill level available.</returns>
	int getHighestMultiKillLevel();

	/// <summary>
	/// Integer that represents, in tics, the max time interval allowed
	/// between kills to keep up a multi kill streak.
	/// </summary>
	int multiTimeInterval;

	/// <summary>
	/// The detailed multi kill levels for mutli kill streaks.
	/// Populated by reading SPREEDEF, or by loading defaults.
	/// </summary>
	std::vector<MultiKillLevel_s> multiKillLevels;

	/// <summary>
	/// Bookkeeping dictionary for multi kills and multi kill tics per player.
	/// </summary>
	std::unordered_map<int, MultiKillTics_s> mutliKillPlayerDict;
};

/// <summary>
/// P_ProcessMultiKills occurs after a kill to determine
/// if this kill is an interval for a kill streak.
///
/// If so, show the kill streak text to the source player if he's the camera player.
/// Also, remove any kill streak from the killed player, and reset the timer for the
/// source player (if any).
/// </summary>
/// <param name="source">The killer (if a monster/player, null if environment/zombie
/// projectile)</param> <param name="target">The victim</param>
void P_ProcessMultiKills(AActor* source, player_t* target);

/// <summary>
/// Handles ticking players for multi kills.
/// </summary>
/// <param name="player">Player to tick.</param>
void P_TicMultiKill(player_t* player);
