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
//   spree events.
//
//-----------------------------------------------------------------------------

#pragma once
#include <algorithm>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "d_player.h"

class FArchive;

void SexMessage(const char* from, char* to, gender_t gender, std::string_view victim,
                std::string_view killer, std::string_view spree);

/// <summary>
/// The tic a spree, spree breaker or multi kill starts fading out on.
/// </summary>
constexpr int SPREE_FADE_TICS = 3 * TICRATE;

/// <summary>
/// The tic a spree, spree breaker or multi kill stops being displayed.
/// </summary>
constexpr int SPREE_DISPLAY_TICS = 4 * TICRATE;

/// <summary>
/// Type of spree breaker to determine the spree obituary used.
/// </summary>
enum SpreeBreakerType
{
	BR_SELF,
	BR_PLAYER,
	BR_MONSTER
};

/// <summary>
/// Structure to represent a spree's level and its associated data.
/// </summary>
struct Spree_s
{
	std::string spreeText;
	std::string spreeBroadcastText;
	std::string gameSfxToken;
	EColorRange color;
	Spree_s() : spreeText(""), spreeBroadcastText(""), gameSfxToken(""), color(CR_GRAY) { }
	Spree_s(std::string SpreeText, std::string BroadcastText, std::string GameSfxToken, EColorRange Color)
	    : spreeText(SpreeText), spreeBroadcastText(BroadcastText), gameSfxToken(GameSfxToken), color(Color)
	{
	}
};

/// <summary>
/// A record of a player's current spree.
/// </summary>
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

/// <summary>
/// A record of a spree breaker event.
/// </summary>
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

	int spreeEndedLevel;

	SpreeBreakerType spreeEndedType;

	SpreeBreaker_t()
	    : spreeEndedName(""), spreeEndedPlayerId(-1), spreeEndedTeam(TEAM_NONE),
	      spreeEnderName(""), spreeEnderPlayerId(-1), spreeEnderTeam(TEAM_NONE),
	      spreeEndedBroadcastText(""), spreeEnded(""), spreeEndedColor(CR_GRAY),
	      spreeEnderMonster(false), endedPoints(0), spreeEndedTic(0),
	      spreeEndedLevel(-1), spreeEndedType(BR_SELF)
	{
	}
	SpreeBreaker_t(
			std::string SpreeEndedName, int SpreeEndedPlayerId, team_t SpreeEndedTeam,
			std::string SpreeEnderName, int SpreeEnderPlayerId, team_t SpreeEnderTeam,
			std::string SpreeEndedBroadcastText, std::string SpreeEnded, EColorRange SpreeEndedColor,
			bool SpreeEnderMonster, int EndedPoints, int SpreeEndedTic,
			int SpreeEndedLevel, SpreeBreakerType BreakerType)
	    : spreeEndedName(SpreeEndedName), spreeEndedPlayerId(SpreeEndedPlayerId),
	      spreeEndedTeam(SpreeEndedTeam), spreeEnderName(SpreeEnderName),
	      spreeEnderPlayerId(SpreeEnderPlayerId), spreeEnderTeam(SpreeEnderTeam),
	      spreeEndedBroadcastText(SpreeEndedBroadcastText), spreeEnded(SpreeEnded),
	      spreeEndedColor(SpreeEndedColor), spreeEnderMonster(SpreeEnderMonster),
	      endedPoints(EndedPoints), spreeEndedTic(SpreeEndedTic), spreeEndedLevel(SpreeEndedLevel),
	      spreeEndedType(BreakerType)
	{
	}
};

/// <summary>
/// Structure to represent a new spree paradigm.
/// </summary>
struct NewSprees_s
{
	std::vector<Spree_s> newSprees;
	int newKillInterval;
	int newDamageInterval;
	std::string newRepeatingSpreeText = "";
	std::string newSpreeEndPlayer = "";
	std::string newSpreeEndSelf = "";
	std::string newSpreeEndMonster = "";

	NewSprees_s()
	    : newSprees(), newKillInterval(0), newDamageInterval(0),
	      newRepeatingSpreeText(""),
	      newSpreeEndPlayer(""), newSpreeEndSelf(""), newSpreeEndMonster("")
	{
	}
	NewSprees_s(std::vector<Spree_s> Sprees, int NewKillInterval, int NewDamageInterval,
	            std::string NewRepeatingSpreeText, std::string NewSpreeEndPlayer,
	            std::string NewSpreeEndSelf, std::string NewSpreeEndMonster)
	    : newSprees(Sprees), newKillInterval(NewKillInterval),
	      newDamageInterval(NewDamageInterval),
	      newRepeatingSpreeText(NewRepeatingSpreeText),
	      newSpreeEndPlayer(NewSpreeEndPlayer), newSpreeEndSelf(NewSpreeEndSelf),
	      newSpreeEndMonster(NewSpreeEndMonster)
	{
	}
};

/// <summary>
/// A singleton class to manage sprees and associated spree bookkeeping.
/// </summary>
class SpreeManager
{
public:
	SpreeManager();
	~SpreeManager();

	/// <summary>
	/// Gets the only instance of this singleton class.
	/// </summary>
	/// <returns>A pointer to the only allowable SpreeManager object.</returns>
	static SpreeManager& getInstance();

	/// <summary>
	/// Resets the status of the manager, called when loading a new wad with a SPREEDEF
	/// lump.
	/// </summary>
	void reset();

	/// <summary>
	/// Erases the current sprees. Called when entering a new map or game.
	/// </summary>
	void clearSprees();

	/// <summary>
	/// Creates a new spree list reading from a SPREEDEF lump.
	/// </summary>
	/// <param name="newSprees">New finished spree paradigm, in order, as read from a SPREEDEF lump.</param>
	void setSpreeLevels(const NewSprees_s& newSprees);

	/// <summary>
	/// Loads default spree information if no SPREEDEF is found.
	/// </summary>
	void loadSpreeDefaults();

	/// <summary>
	/// Cleans up any old sprees or spree breakers.
	/// </summary>
	void expireOldSprees(); // Runs every tic to clean up old sprees.

	/// <summary>
	/// Gets a current spree for a player.
	/// </summary>
	/// <param name="playerId">Player ID of the player to look up the spree record for.</param>
	/// <returns>The player's spree record, or a zeroed out struct.</returns>
	const SpreeRecord_t& getSpreeRecord(const int playerId);

	/// <summary>
	/// Gets the latest spree record excluding the current player.
	/// </summary>
	/// <param name="notPlayerId">Get any spree except for this player id</param>
	/// <returns>The latest spree record that isn't the specified player id's,
	/// or an empty one if not found.</returns>
	const SpreeRecord_t& getLatestSpreeRecord(const int notPlayerId);

	/// <summary>
	/// Records a single kill for a player, adds it to the kills since last death dictionary,
	/// and updates the spree record accordingly.
	/// <br />
	/// If the player doesn't have a spree record yet, create one.
	/// <br />
	/// If the player has reached a higher spree level, update it.
	/// </summary>
	/// <param name="player">A pointer to the player object to record this kill.</param>
	/// <returns>True if this kill event resulted in an upgraded spree level.</returns>
	bool recordPlayerKill(const player_t* player);

	/// <summary>
	/// Records a damage event for a player, adds it to the damage since last death dictionary,
	/// and updates the spree record accordingly.
	/// <br />
	/// If the player doesn't have a spree record yet, create one.
	/// <br />
	/// If the player has reached a higher spree level, update it.
	/// </summary>
	/// <param name="player">A pointer to the player object to record this damage.</param>
	/// <param name="totalDamage">Amount of damage to record.</param>
	/// <returns>True if this damage event resulted in an upgraded spree level.</returns>
	bool recordPlayerDamage(const player_t* player, const int totalDamage);

	/// <summary>
	/// Gets the current SpreeBreaker_t object.
	/// </summary>
	/// <returns>The current SpreeBreaker_t object.</returns>
	const SpreeBreaker_t& getSpreeBreaker();

	/// <summary>
	/// Using the spree ender and the player whomst spree has ended, this function handles logic
	/// of what kind of breaker it is,
	/// then sets the current SpreeBreaker_t object.
	/// </summary>
	/// <param name="source">The spree ender.</param>
	/// <param name="target">The player whomst spree has ended. (RIP)</param>
	void setSpreeBreaker(const AActor* source, const player_t* target);

	/// <summary>
	/// Sets the current SpreeBreaker_t object from a raw state,
	/// where we don't know the conditions, we just know the breaker information.
	/// <br />
	/// Generally used by clients to set spree breaker information when receiving from the
	/// server.
	/// </summary>
	/// <param name="breaker">A generated spreeBreaker_t struct.</param>
	/// <param name="level">The spree level the player was on when their spree ended.</param>
	/// <param name="type">Type of spree breaker.</param>
	/// <param name="ticsAgo">How many tics ago the spree was broken.</param>
	void setRawSpreeBreaker(
	    const SpreeBreaker_t& breaker,
			const int level,
	    const SpreeBreakerType type,
	    const int ticsAgo);

	/// <summary>
	/// Sets a raw state of a player's spree.
	/// <br />
	/// Used to set a player's spree when receiving from the server.
	/// </summary>
	/// <param name="playerId">Player ID of the player to set the spree.</param>
	/// <param name="newSpreeLevel">New level of spree to </param>
	/// <param name="ticsAgo">How many tics ago the spree level was achieved.</param>
	/// <returns>True if the update resulted in an upgraded spree level.</returns>
	bool setRawSpree(const int playerId, const int newSpreeLevel, const int ticsAgo);

	/// <summary>
	/// Checks if the user has a current spree active.
	/// </summary>
	/// <param name="playerid">ID of the player to check.</param>
	/// <returns>True if the player has an active spree.</returns>
	bool hasSpree(const int playerid);

	/// <summary>
	/// Removes a spree for a specific player.
	/// </summary>
	/// <param name="playerid">ID of the player to remove the spree record from.</param>
	void removeSpree(const int playerid);

	/// <summary>
	/// Erases all points for a player (generally after a player death)
	/// </summary>
	/// <param name="playerid">Player ID of the player to erase points from.</param>
	void erasePoints(const int playerid);

	/// <summary>
	/// Resets everyones points after map change/restart/new round.
	/// </summary>
	void clearPoints();

	/// <summary>
	/// Gets current amount of points for a specific player.
	/// </summary>
	/// <param name="playerid">Player ID of the player to get points for.</param>
	/// <returns>Amount of points for the specified player.</returns>
	int getPoints(const int playerid);

	/// <summary>
	/// Gets every active spree record, keyed by player ID.
	/// </summary>
	/// <returns>All current spree records.</returns>
	[[nodiscard]] const std::unordered_map<int, SpreeRecord_t>& getSpreeRecords() const;

	/// <summary>
	/// Reads or writes every active spree and the last spree breaker.
	/// <param name="arc">Archive to serialize to or from.</param>
	void serialize(FArchive& arc);

private:
	/// <summary>
	/// Adds points (kills or damage depending on gamemode) to a player
	/// </summary>
	/// <param name="playerid">playerid of the player to add points to.</param>
	/// <param name="points">Amount of points to add.</param>
	void addPoints(const int playerid, const int points);

	/// <summary>
	/// Gets a specific spree level by the amount of kills the player has.
	/// </summary>
	/// <param name="kills">Amount of kills to check the spree level.</param>
	/// <returns>The spree level that corresponds to the amount of kills.</returns>
	int getSpreeLevelByKills(const int kills);

	/// <summary>
	/// Gets a specific spree level by the amount of damage the player has.
	/// </summary>
	/// <param name="damage">Amount of damage to check the spree level.</param>
	/// <returns>The spree level that corresponds to the amount of damage.</returns>
	int getSpreeLevelByDamage(const int damage);

	/// <summary>
	/// A reusable function to check if a player's spree level has changed.
	/// </summary>
	/// <param name="playerId">ID of the player to check.</param>
	/// <param name="playerName">Net name of the player.</param>
	/// <param name="newSpreeLevel">The spree level to check against the Spree record.</param>
	/// <param name="tic">The tic the check began.</param>
	/// <returns>True of the specific level is higher than the spree record, indicating an upgraded spree level.</returns>
	bool checkForSpreeUpdates(const int playerId, const std::string playerName,
	                          const int newSpreeLevel, const int tic); // Gets the spree level by the
	                                                                   // amount of kills the player has.

	/// <summary>
	/// Gets the associated data of a spree level.
	/// If higher than the max level, get the highest one.
	/// </summary>
	/// <param name="level">Spree level to get data for.</param>
	/// <returns>The spree level specified, or a zeroed out struct if invalid.</returns>
	const Spree_s& getSpreeLevel(const int level); // Gets the local spree level (with text and color)

	/// <summary>
	/// Gets the kill spree interval for players -- as in, the amount of kills needed to upgrade to a new spree level.
	/// </summary>
	/// <returns>The kill spree interval.</returns>
	int getSpreeKillInterval();

	/// <summary>
	/// Gets the damage spree interval for players -- as in, the amount of damage needed to
	/// upgrade to a new spree level.
	/// </summary>
	/// <returns>The damage spree interval.</returns>
	int getSpreeDamageInterval();

	/// <summary>
	/// Gets the highest spree level possible.
	/// </summary>
	/// <returns>The highest spree level possible in the current configuration.</returns>
	int getHighestSpreeLevel();

	/// <summary>
	/// Runs sexmessage on a spreebreaker record to get the final broadcast text.
	/// </summary>
	/// <param name="breaker">Spree breaker to modify</param>
	/// <param name="type">Type of the spree breaker.</param>
	void setBreakerLanguage(
			SpreeBreaker_t& breaker,
			const SpreeBreakerType type);

	/// <summary>
	/// Runs sexmessage on a spree record to get the final broadcast text.
	/// </summary>
	/// <param name="record">The spree record for the player.</param>
	/// <param name="playerId">ID of the player who has the spree.</param>
	void setSpreeRecordLanguage(SpreeRecord_t& record, const int playerId);

	/// <summary>
	/// Kill spree interval -- how many kills to reach a new spree level. PVP
	/// </summary>
	int spreeKillInterval;

	/// <summary>
	/// Damage spree interval -- how much damage to reach a new spree level. PVE
	/// </summary>
	int spreeDamageInterval;

	/// <summary>
	/// Levels of sprees configured during wad load
	/// </summary>
	std::vector<Spree_s> spreeLevels;

	/// <summary>
	/// Text to display when a player is repeating their
	/// highest spree level.
	/// </summary>
	std::string repeatingSpreeText;

	/// <summary>
	/// Text for when a spree has ended by the hands of another player.
	/// </summary>
	std::string spreeEndPlayer;

	/// <summary>
	/// Text for when a spree has ended by suicide. Whoopsie!
	/// </summary>
	std::string spreeEndSelf;

	/// <summary>
	/// Text for when a spree has ended by a monster.
	/// </summary>
	std::string spreeEndMonster;

	/// <summary>
	/// Dictionary of all spree levels each player has achieved.
	/// </summary>
	std::unordered_map<int, SpreeRecord_t> spreeRecord;

	/// <summary>
	/// Dictionary to keep track of all players points (kills/damage depending on mode)
	/// since last death.
	/// </summary>
	std::unordered_map<int, int> pointsSinceLastDeath;

	/// <summary>
	/// Updated with the latest spree to be broken.
	/// </summary>
	SpreeBreaker_t spreeBreaker;

	/// <summary>
	/// An invalid spree record.
	/// </summary>
	SpreeRecord_t emptyRecord;

	/// <summary>
	/// An empty spree level.
	/// </summary>
	Spree_s emptySpree;
};

/// <summary>
/// G_ProcessSpreeKill occurs after a kill to determine
/// if this kill is an interval for a killing spree.
///
/// If so, show the spree text to the source player if he's the camera player.
/// Also, remove any spree from a player who was killed, and process any spree breakers.
/// </summary>
/// <param name="source">The killer (if a monster/player, null if environment/zombie
/// projectile)</param>
/// <param name="target">The victim</param>
void P_ProcessSpreeKill(const AActor* source, const player_t* target);

/// <summary>
/// G_ProcessSpreeDamage occurs after a player deals some damage to determine
/// if this damage is an interval for a killing spree.
///
/// If so, show the spree text to the source player if he's the camera player.
/// Also, remove any spree from a player who was killed, and process any spree breakers.
/// </summary>
/// <param name="source">The damager (must be a player)</param>
/// <param name="target">The thing being damaged, so friendly monsters can be skipped.</param>
/// <param name="totalDamage">The total damage dealt during this event.</param>
void P_ProcessSpreeDamage(const player_t* source, const AActor* target,
                          const int totalDamage);

/// <summary>
/// What the spree HUD should draw, resolved from the current spree records and the
/// last spree breaker. Kept apart from the drawing so the rules can be tested.
/// </summary>
struct SpreeHudLines_t
{
	/// <summary>
	/// Spree to draw as the big line, or null for none.
	/// </summary>
	const SpreeRecord_t* bigSpree = nullptr;

	/// <summary>
	/// Spree to draw on the small line, or null for none.
	/// </summary>
	const SpreeRecord_t* smallSpree = nullptr;

	/// <summary>
	/// Spree breaker to draw on the small line, or null for none. Takes the small line
	/// over smallSpree when both are set.
	/// </summary>
	const SpreeBreaker_t* smallBreaker = nullptr;
};

/// <summary>
/// Works out which sprees the HUD should be showing for a given player.
/// <br />
/// The watched player's own spree is the big line and is never echoed small, except a
/// repeat of the top level, which only ever goes small. The small line holds whichever
/// of that repeat, another player's spree and the last spree breaker happened last.
/// </summary>
/// <param name="displayPlayerId">ID of the player being watched.</param>
/// <returns>The lines to draw, any of which may be null.</returns>
SpreeHudLines_t P_GetSpreeHudLines(const int displayPlayerId);

/// <summary>
/// Handles internal ticking for spree bookkeeping.
/// </summary>
void P_TicSprees();

/// <summary>
/// Serializes spree bookkeeping in and out of netdemo snapshots.
/// </summary>
/// <param name="arc">Archive to serialize to or from.</param>
void P_SerializeSprees(FArchive& arc);
