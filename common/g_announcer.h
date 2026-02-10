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
//   Handles the loading of data from all ONCRINFO lumps,
//   as well as static functions to handle announcer events.
//
//-----------------------------------------------------------------------------

#pragma once

#include <map>
#include <queue>
#include <string>

#include "d_player.h"

// Tokens for the announcer sound dictionary.

// Possessive CTF Announcements

/// <summary>
/// Plays when the CTF flag on your own team is taken from its pedestal by an enemy, when
/// the possessive announcer is on.
/// </summary>
static const std::string ANN_YOURFLAGTAKEN = "yourflagtaken";
/// <summary>
/// Plays when the CTF flag on the enemy team is taken from its pedestal by a teammate (or
/// you), when the possessive announcer is on.
/// </summary>
static const std::string ANN_ENEMYFLAGTAKEN = "enemyflagtaken";
/// <summary>
/// Plays when the CTF flag on your own team is dropped, if the possessive announcer is
/// on.
/// </summary>
static const std::string ANN_YOURFLAGDROPPED = "yourflagdropped";
/// <summary>
/// Plays when the CTF flag on an enemy team is dropped, if the possessive announcer is
/// on.
/// </summary>
static const std::string ANN_ENEMYFLAGDROPPED = "enemyflagdropped";
/// <summary>
/// Plays when the CTF flag on your own team is being returned in manual CTF, when
/// possessive announcer is on.
/// </summary>
static const std::string ANN_YOURFLAGISBEINGRETURNED = "yourflagisbeingreturned";
/// <summary>
/// Plays when the CTF flag on an enemy team is being returned in manual CTF, when
/// possessive announcer is on.
/// </summary>
static const std::string ANN_ENEMYFLAGISBEINGRETURNED = "enemyflagisbeingreturned";
/// <summary>
/// Plays when the CTF flag on your own team is returned to its pedestal (socket), when
/// the possessive announcer is on.
/// </summary>
static const std::string ANN_YOURFLAGRETURNED = "yourflagreturned";
/// <summary>
/// Plays when the CTF flag on an enemy team is returned to its pedestal (socket), when
/// the possessive announcer is on.
/// </summary>
static const std::string ANN_ENEMYFLAGRETURNED = "enemyflagreturned";
/// <summary>
/// Plays when your team captures the enemies CTF flag, when the possessive announcer is
/// on.
/// </summary>
static const std::string ANN_YOURTEAMSCORES = "yourteamscores";
/// <summary>
/// Plays when the enemy team captures your CTF flag, when the possessive announcer is on.
/// </summary>
static const std::string ANN_ENEMYTEAMSCORES = "enemyteamscores";

// Team Based CTF Announcements

/// <summary>
/// Plays when the red flag is taken, when the team colors announcer is on.
/// </summary>
static const std::string ANN_REDFLAGTAKEN = "redflagtaken";
/// <summary>
/// Plays when the blue flag is taken, when the team colors announcer is on.
/// </summary>
static const std::string ANN_BLUEFLAGTAKEN = "blueflagtaken";
/// <summary>
/// Plays when the green flag is taken, when the team colors announcer is on.
/// </summary>
static const std::string ANN_GREENFLAGTAKEN = "greenflagtaken";
/// <summary>
/// Plays when the red flag is dropped, when the team colors announcer is on.
/// </summary>
static const std::string ANN_REDFLAGDROPPED = "redflagdropped";
/// <summary>
/// Plays when the blue flag is dropped, when the team colors announcer is on.
/// </summary>
static const std::string ANN_BLUEFLAGDROPPED = "blueflagdropped";
/// <summary>
/// Plays when the green flag is dropped, when the team colors announcer is on.
/// </summary>
static const std::string ANN_GREENFLAGDROPPED = "greenflagdropped";
/// <summary>
/// Plays when the red flag is being returned in manual CTF, when the team colors
/// announcer is on.
/// </summary>
static const std::string ANN_REDFLAGISBEINGRETURNED = "redflagisbeingreturned";
/// <summary>
/// Plays when the blue flag is being returned in manual CTF, when the team colors
/// announcer is on.
/// </summary>
static const std::string ANN_BLUEFLAGISBEINGRETURNED = "blueflagisbeingreturned";
/// <summary>
/// Plays when the green flag is being returned in manual CTF, when the team colors
/// announcer is on.
/// </summary>
static const std::string ANN_GREENFLAGISBEINGRETURNED = "greenflagisbeingreturned";
/// <summary>
/// Plays when the red flag has been returned to its pedestal (socket), when the team
/// colors announcer is on.
/// </summary>
static const std::string ANN_REDFLAGRETURNED = "redflagreturned";
/// <summary>
/// Plays when the blue flag has been returned to its pedestal (socket), when the team
/// colors announcer is on.
/// </summary>
static const std::string ANN_BLUEFLAGRETURNED = "blueflagreturned";
/// <summary>
/// Plays when the green flag has been returned to its pedestal (socket), when the team
/// colors announcer is on.
/// </summary>
static const std::string ANN_GREENFLAGRETURNED = "greenflagreturned";
/// <summary>
/// Plays when the red team scores a CTF flag, when the team colors announcer is on.
/// </summary>
static const std::string ANN_REDTEAMSCORES = "redteamscores";
/// <summary>
/// Plays when the blue team scores a CTF flag, when the team colors announcer is on.
/// </summary>
static const std::string ANN_BLUETEAMSCORES = "blueteamscores";
/// <summary>
/// Plays when the green team scores a CTF flag, when the team colors announcer is on.
/// </summary>
static const std::string ANN_GREENTEAMSCORES = "greenteamscores";

// Horde Mode Announcements

/// <summary>
/// Plays when a Horde boss spawns in at the end of a wave.
/// </summary>
static const std::string ANN_HORDEBOSSSPAWN = "hordebossspawn";
/// <summary>
/// Plays when the player is the last alive in a survival game.
/// </summary>
static const std::string ANN_LASTPLAYERALIVE = "lastplayeralive";
/// <summary>
/// Plays when the player has been revived by a new wave or resurrect player powerup.
/// </summary>
static const std::string ANN_REVIVEDPLAYER = "revivedplayer";

// General Announcements

/// <summary>
/// Plays when there's 5 minutes left in a time limit game.
/// </summary>
static const std::string ANN_FIVEMINUTEWARNING = "fiveminutewarning";
/// <summary>
/// Plays when there's 1 minute left in a time limit game.
/// </summary>
static const std::string ANN_ONEMINUTEWARNING = "oneminutewarning";
/// <summary>
/// Plays when there are 3 frags left in a frag limit game.
/// </summary>
static const std::string ANN_THREEFRAGSLEFT = "threefragsleft";
/// <summary>
/// Plays when there are 2 frags left in a frag limit game.
/// </summary>
static const std::string ANN_TWOFRAGSLEFT = "twofragsleft";
/// <summary>
/// Plays when there is 1 frag left in a frag limit game.
/// </summary>
static const std::string ANN_ONEFRAGLEFT = "onefragleft";
/// <summary>
/// Plays in warmup when there's 5 seconds before the match starts.
/// </summary>
static const std::string ANN_FIVE = "five";
/// <summary>
/// Plays in warmup when there's 4 seconds before the match starts.
/// </summary>
static const std::string ANN_FOUR = "four";
/// <summary>
/// Plays in warmup when there's 3 seconds before the match starts.
/// </summary>
static const std::string ANN_THREE = "three";
/// <summary>
/// Plays in warmup when there' 2 seconds before the match starts.
/// </summary>
static const std::string ANN_TWO = "two";
/// <summary>
/// Plays in warmup when there's 1 second before the match starts.
/// </summary>
static const std::string ANN_ONE = "one";
/// <summary>
/// Plays when fighting is enabled, either start of a new round or weapons unlocked.
/// </summary>
static const std::string ANN_FIGHT = "fight";
/// <summary>
/// Plays when the display player runs out of lives.
/// </summary>
static const std::string ANN_PLAYERELIMINATED = "playereliminated";
/// <summary>
/// Plays when the first frag occurs in a new game.
/// </summary>
static const std::string ANN_FIRSTBLOOD = "firstblood";

// Lead Change Announcements

/// <summary>
/// Plays when the displayplayer has the lead.
/// </summary>
static const std::string ANN_YOUHAVETHELEAD = "youhavethelead";
/// <summary>
/// Plays when the displayplayer has lost the lead.
/// </summary>
static const std::string ANN_YOULOSTTHELEAD = "youlostthelead";
/// <summary>
/// Plays when the displayplayer has tied for the lead.
/// </summary>
static const std::string ANN_YOUTIEDFORTHELEAD = "youtiedforthelead";

// Match Result Announcements

/// <summary>
/// Plays when the local player wins.
/// </summary>
static const std::string ANN_YOUWIN = "youwin";
/// <summary>
/// Plays when the local player loses.
/// </summary>
static const std::string ANN_YOULOSE = "youlose";
/// <summary>
/// Plays when the local player ties.
/// </summary>
static const std::string ANN_YOUTIED = "youtied";

/// <summary>
/// Contains tokens that would be disabled if snd_announcectf is false.
/// </summary>
static const std::vector<std::string> announcerCTFTokens = {
    ANN_YOURFLAGTAKEN,           ANN_ENEMYFLAGTAKEN,
    ANN_YOURFLAGDROPPED,         ANN_ENEMYFLAGDROPPED,
    ANN_YOURFLAGISBEINGRETURNED, ANN_ENEMYFLAGISBEINGRETURNED,
    ANN_YOURFLAGRETURNED,        ANN_ENEMYFLAGRETURNED,
    ANN_YOURTEAMSCORES,          ANN_ENEMYTEAMSCORES,

    ANN_REDFLAGTAKEN,            ANN_BLUEFLAGTAKEN,           ANN_GREENFLAGTAKEN,
    ANN_REDFLAGDROPPED,          ANN_BLUEFLAGDROPPED,         ANN_GREENFLAGDROPPED,
    ANN_REDFLAGISBEINGRETURNED,  ANN_BLUEFLAGISBEINGRETURNED, ANN_GREENFLAGISBEINGRETURNED,
    ANN_REDFLAGRETURNED,         ANN_BLUEFLAGRETURNED,        ANN_GREENFLAGRETURNED,
    ANN_REDTEAMSCORES,           ANN_BLUETEAMSCORES,          ANN_GREENTEAMSCORES
};

/// <summary>
/// Contains tokens that would be disabled if snd_announcehorde is false.
/// </summary>
static const std::vector<std::string> announcerHordeTokens = {ANN_HORDEBOSSSPAWN,
                                                              ANN_REVIVEDPLAYER};

/// <summary>
/// Contains tokens that would be disabled if snd_announcesurvival is false.
/// </summary>
static const std::vector<std::string> announcerSurvivalTokens = {
    ANN_LASTPLAYERALIVE,
                                                                 ANN_PLAYERELIMINATED};

/// <summary>
/// Contains tokens that would be disabled if snd_announcecountdown is false.
/// </summary>
static const std::vector<std::string> announcerCountdownTokens = {
    ANN_FIVE, ANN_FOUR, ANN_THREE, ANN_TWO, ANN_ONE, ANN_FIGHT};

/// <summary>
/// Contains tokens that would be disabled if snd_announcecountdown is false.
/// </summary>
static const std::vector<std::string> announcerTimeWarningsTokens = {
    ANN_ONEMINUTEWARNING, ANN_FIVEMINUTEWARNING};

/// <summary>
/// Contains tokens that would be disabled if snd_announcefirstblood is false.
/// </summary>
static const std::vector<std::string> announcerFirstBloodTokens = {ANN_FIRSTBLOOD};

/// <summary>
/// Contains tokens that would be disabled if snd_announcefragtracking is false.
/// </summary>
static const std::vector<std::string> announcerFragTrackingTokens = {
    ANN_THREEFRAGSLEFT, ANN_TWOFRAGSLEFT, ANN_ONEFRAGLEFT};

/// <summary>
/// Contains tokens that would be disabled if snd_announceleadtracking is false.
/// </summary>
static const std::vector<std::string> announcerLeadTrackingTokens = {
    ANN_YOUHAVETHELEAD, ANN_YOUTIEDFORTHELEAD, ANN_YOULOSTTHELEAD};

/// <summary>
/// Contains tokens that would be disabled if snd_announceresulttracking is false.
/// </summary>
static const std::vector<std::string> announcerResultTrackingTokens = {
    ANN_YOUWIN, ANN_YOUTIED, ANN_YOULOSE};

/// <summary>
/// Holds all the metadata for the specified announcer pack.
/// </summary>
struct AnnouncerMetaData_s
{
	/// <summary>
	/// Name of the announcer pack.
	/// Used to identify the announcer in cvars and config files.
	/// </summary>
	std::string name;
	/// <summary>
	/// Description of the announcer pack.
	/// Displayed in the announcer selection menu.
	/// </summary>
	std::string description;
	/// <summary>
	/// Author of the announcer pack.
	/// Displayed in the announcer selection menu.
	/// </summary>
	std::string author;
	AnnouncerMetaData_s() : name(""), description(""), author("") { }
	AnnouncerMetaData_s(const AnnouncerMetaData_s& other)
	{
		name = other.name;
		description = other.description;
		author = other.author;
	}
};

/// <summary>
/// This structure represents a single announcer pack, and its specified data.
/// </summary>
struct Announcer_s
{
	AnnouncerMetaData_s metadata;
	/// <summary>
	/// Dictionary of the sounds in this announcer pack.
	/// Each named sound refers to a sndinfo token, which is given to the
	/// sound function to play the appropriate sound.
	/// </summary>
	std::unordered_map<std::string, std::string> soundDict;
	Announcer_s() : metadata(), soundDict() { }
	Announcer_s(const Announcer_s& other)
	{
		metadata = other.metadata;
		soundDict = other.soundDict;
	}
};

/// <summary>
/// A singleton class to manage announcers.
/// </summary>
class AnnouncerManager
{
public:
	AnnouncerManager();
	~AnnouncerManager();

	/// <summary>
	/// Gets the only instance of this singleton class.
	/// </summary>
	/// <returns>A pointer to the only allowable AnnouncerManager object.</returns>
	static AnnouncerManager& getInstance();

	/// <summary>
	/// Resets the status of the manager, called before loading all new ONCRINFO lumps.
	/// </summary>
	void reset();

	/// <summary>
	/// Gets if there is a currently loaded announcer name available.
	/// </summary>
	/// <param name="announcer">Name of the announcer pack to check.</param>
	/// <returns>True if the name matches a currently available announcer.</returns>
	bool isAnnouncerLoaded(const std::string& announcer) const;

	/// <summary>
	/// Returns if a named token exists in our repository of named tokens.
	/// Will NOT return true for spree or multi tokens, as those are dynamically
	/// generated.
	/// </summary>
	/// <param name="tokenName">Name of the token to search.</param>
	/// <returns>True if the token is found.</returns>
	bool namedTokenExists(const std::string& tokenName);

	/// <summary>
	/// Gets the name of the announcer to the left of the current one.
	/// If the current cl_announcer is null or invalid, return the first announcer in the
	/// list.
	/// </summary>
	/// <returns>The name of the announcer to the left of this one.</returns>
	std::string getLeftAnnouncer(const std::string& currentAnnouncer) const;

	/// <summary>
	/// Gets the name of the announcer to the right of the current one.
	/// If the current cl_announcer is null or invalid, return the first announcer in the
	/// list.
	/// </summary>
	/// <returns>The name of the announcer to the right of this one.</returns>
	std::string getRightAnnouncer(const std::string& currentAnnouncer) const;

	/// <summary>
	/// Gets the metadata of the specified announcer pack.
	/// </summary>
	/// <param name="currentAnnouncer">Announcer for which to return the metadata.</param>
	/// <returns>The metadata of the specified announcer pack.</returns>
	const AnnouncerMetaData_s& getAnnouncerMetadata(const std::string& currentAnnouncer);

	/// <summary>
	/// Loads default announcer information if no ONCRINFO is found.
	/// </summary>
	void loadAnnouncerDefaults();

	/// <summary>
	/// Gets the sndinfo token for the current announcer for the
	/// following event.
	/// </summary>
	/// <param name="token">Token for the event to play. One
	/// of the announcements above.</param>
	/// <returns>The specified sndinfo token for the current announcer, empty if invalid
	/// or unknown.</returns>
	const std::string getTokenForEvent(const std::string& event);

	/// <summary>
	/// Loads the specified announcer, or if not found, loads the default announcer.
	/// </summary>
	/// <param name="announcer">Name of the announcer to load.</param>
	void loadAnnouncerByName(const std::string& announcer);

	/// <summary>
	/// Loads the map of new announcers brought in by reading an ONCRINFO lump.
	/// </summary>
	/// <param name="newAnnouncers">New announcers to add or merge.</param>
	void loadAnnouncers(const std::unordered_map<std::string, Announcer_s> newAnnouncers);

	/// <summary>
	/// Queues an announcer sound to be played. If nothing is currently playing,
	/// the sound plays immediately. Otherwise it is added to the queue and will
	/// play after the current sound's duration has elapsed.
	/// </summary>
	/// <param name="soundName">The sndinfo token of the sound to play.</param>
	void queueSound(const std::string& soundName);

	/// <summary>
	/// Called every game tic to process the announcer sound queue.
	/// Decrements the delay counter and plays the next queued sound
	/// when the current one has finished.
	/// </summary>
	void tick();

	/// <summary>
	/// Clears all queued announcer sounds and resets the delay counter.
	/// </summary>
	void clearQueue();

	/// <summary>
	/// Records a pending spree announcement for this tic. Only the highest
	/// level reached during a single tic will actually be announced.
	/// </summary>
	/// <param name="level">The spree level reached.</param>
	/// <param name="announcerSound">The sndinfo token for the announcer sound.</param>
	/// <param name="gameSfx">The sndinfo token for the game sfx sound.</param>
	void setPendingSpree(int level, const std::string& announcerSound,
	                     const std::string& gameSfx);

	/// <summary>
	/// Records a pending multi kill announcement for this tic. Only the highest
	/// level reached during a single tic will actually be announced.
	/// </summary>
	/// <param name="level">The multi kill level reached.</param>
	/// <param name="announcerSound">The sndinfo token for the announcer sound.</param>
	/// <param name="gameSfx">The sndinfo token for the game sfx sound.</param>
	void setPendingMultiKill(int level, const std::string& announcerSound,
	                         const std::string& gameSfx);

	/// <summary>
	/// Plays the highest pending spree and multi kill sounds accumulated
	/// during this tic, then resets the pending state.
	/// Should be called once per tic after all kills have been processed.
	/// </summary>
	void flushPendingSounds();

	/// <summary>
	/// Function to return whether the 3-frag warning has been announced already.
	/// </summary>
	bool hasFragWarning3BeenAnnounced() const { return fragWarning3Announced; }

	/// <summary>
	/// Function to return whether the 2-frag warning has been announced already.
	/// </summary>
	bool hasFragWarning2BeenAnnounced() const { return fragWarning2Announced; }

	/// <summary>
	/// Function to return whether the 1-frag warning has been announced already.
	/// </summary>
	bool hasFragWarning1BeenAnnounced() const { return fragWarning1Announced; }

	/// <summary>
	/// Function to announce the 3-frag warning.
	/// </summary>
	void announceFragWarning3();

	/// <summary>
	/// Function to announce the 2-frag warning.
	/// </summary>
	void announceFragWarning2();

	/// <summary>
	/// Function to announce the 1-frag warning.
	/// </summary>
	void announceFragWarning1();

	/// <summary>
	/// Resets all the frag warning variables to enable an upcoming announcement.
	/// </summary>
	void resetFragWarnings();

	/// <summary>
	/// Function to announce the 5-minute time warning.
	/// </summary>
	void announceFiveMinuteWarning();

	/// <summary>
	/// Function to announce the 1-minute time warning.
	/// </summary>
	void announceOneMinuteWarning();

	/// <summary>
	/// Function to return whether the fight announcement has been played already.
	/// </summary>
	bool hasFightBeenAnnounced() const { return fightAnnounced; }

	/// <summary>
	/// Resets the fight announcement flag to enable an upcoming announcement.
	/// </summary>
	void resetFightAnnouncement() { fightAnnounced = false; }

	/// <summary>
	/// Marks the fight announcement as having been played.
	/// </summary>
	void setFightAnnounced() { fightAnnounced = true; }

	/// <summary>
	/// Resets all countdown announcement flags.
	/// </summary>
	void resetCountdownAnnouncements()
	{
		countdown5Announced = false;
		countdown4Announced = false;
		countdown3Announced = false;
		countdown2Announced = false;
		countdown1Announced = false;
	}

	/// <summary>
	/// Checks if a specific countdown has been announced.
	/// </summary>
	bool hasCountdownBeenAnnounced(int countdown) const
	{
		switch (countdown)
		{
		case 5: return countdown5Announced;
		case 4: return countdown4Announced;
		case 3: return countdown3Announced;
		case 2: return countdown2Announced;
		case 1: return countdown1Announced;
		default: return true;
		}
	}

	/// <summary>
	/// Marks a specific countdown as announced.
	/// </summary>
	void setCountdownAnnounced(int countdown)
	{
		switch (countdown)
		{
		case 5: countdown5Announced = true; break;
		case 4: countdown4Announced = true; break;
		case 3: countdown3Announced = true; break;
		case 2: countdown2Announced = true; break;
		case 1: countdown1Announced = true; break;
		}
	}

	/// <summary>
	/// Resets lead tracking state for a new game.
	/// </summary>
	void resetLeadTracking()
	{
		currentLeaderPlayerId = 0;
		currentLeaderTeam = TEAM_NONE;
		displayPlayerHasLead = false;
		leadIsTied = false;
	}

	/// <summary>
	/// Resets the first blood announcement flag for a new game.
	/// </summary>
	void resetFirstBloodAnnouncement() { firstBloodAnnounced = false; }

	/// <summary>
	/// Gets whether first blood has already been announced.
	/// </summary>
	bool hasFirstBloodBeenAnnounced() const { return firstBloodAnnounced; }

	/// <summary>
	/// Marks first blood as having been announced.
	/// </summary>
	void setFirstBloodAnnounced() { firstBloodAnnounced = true; }

	/// <summary>
	/// Gets the current leader player ID (for FFA games).
	/// </summary>
	int getCurrentLeaderPlayerId() const { return currentLeaderPlayerId; }

	/// <summary>
	/// Sets the current leader player ID (for FFA games).
	/// </summary>
	void setCurrentLeaderPlayerId(int id) { currentLeaderPlayerId = id; }

	/// <summary>
	/// Gets the current leader team (for team games).
	/// </summary>
	team_t getCurrentLeaderTeam() const { return currentLeaderTeam; }

	/// <summary>
	/// Sets the current leader team (for team games).
	/// </summary>
	void setCurrentLeaderTeam(team_t team) { currentLeaderTeam = team; }

	/// <summary>
	/// Gets whether the display player currently has the lead.
	/// </summary>
	bool doesDisplayPlayerHaveLead() const { return displayPlayerHasLead; }

	/// <summary>
	/// Sets whether the display player has the lead.
	/// </summary>
	void setDisplayPlayerHasLead(bool hasLead) { displayPlayerHasLead = hasLead; }

	/// <summary>
	/// Gets whether the lead is currently tied.
	/// </summary>
	bool isLeadTied() const { return leadIsTied; }

	/// <summary>
	/// Sets whether the lead is tied.
	/// </summary>
	void setLeadTied(bool tied) { leadIsTied = tied; }

private:
	/// <summary>
	/// Contains every named token for announcer packs.
	/// Must run namedTokenExists() to fill this vector.
	/// </summary>
	std::vector<std::string> allTokens;
	/// <summary>
	/// Has the 3-frag warning been announced already?
	/// </summary>
	bool fragWarning3Announced = false;
	/// <summary>
	/// Has the 2-frag warning been announced already?
	/// </summary>
	bool fragWarning2Announced = false;
	/// <summary>
	/// Has the 1-frag warning been announced already?
	/// </summary>
	bool fragWarning1Announced = false;
	/// <summary>
	/// Has the fight announcement been played already?
	/// </summary>
	bool fightAnnounced = false;
	/// <summary>
	/// Countdown announcement flags (5, 4, 3, 2, 1).
	/// </summary>
	bool countdown5Announced = false;
	bool countdown4Announced = false;
	bool countdown3Announced = false;
	bool countdown2Announced = false;
	bool countdown1Announced = false;

	/// <summary>
	/// Lead tracking state for FFA games - stores the player ID of the current leader.
	/// A value of 0 means no leader has been established yet.
	/// </summary>
	int currentLeaderPlayerId = 0;

	/// <summary>
	/// Lead tracking state for team games - stores the team currently in the lead.
	/// </summary>
	team_t currentLeaderTeam = TEAM_NONE;

	/// <summary>
	/// Tracks whether the display player currently has the lead.
	/// </summary>
	bool displayPlayerHasLead = false;

	/// <summary>
	/// Tracks whether there is currently a tie for the lead.
	/// </summary>
	bool leadIsTied = false;

	/// <summary>
	/// Tracks whether the first blood announcement has been played this game.
	/// </summary>
	bool firstBloodAnnounced = false;

	/// <summary>
	/// The highest spree level pending announcement this tic. -1 means none pending.
	/// </summary>
	int pendingSpreeLevel = -1;

	/// <summary>
	/// The announcer sound token for the pending spree announcement.
	/// </summary>
	std::string pendingSpreeAnnouncerSound;

	/// <summary>
	/// The game sfx token for the pending spree announcement.
	/// </summary>
	std::string pendingSpreeGameSfx;

	/// <summary>
	/// The highest multi kill level pending announcement this tic. -1 means none pending.
	/// </summary>
	int pendingMultiKillLevel = -1;

	/// <summary>
	/// The announcer sound token for the pending multi kill announcement.
	/// </summary>
	std::string pendingMultiKillAnnouncerSound;

	/// <summary>
	/// The game sfx token for the pending multi kill announcement.
	/// </summary>
	std::string pendingMultiKillGameSfx;

	/// <summary>
	/// Queue of pending announcer sound names (sndinfo tokens) waiting to be played.
	/// </summary>
	std::queue<std::string> soundQueue;

	/// <summary>
	/// Number of tics remaining before the next queued sound can play.
	/// When this reaches 0, the next sound in the queue is played.
	/// </summary>
	int delayTicsRemaining = 0;

	/// <summary>
	/// Plays a sound immediately on the announcer channel and sets the
	/// delay counter based on the sound's duration.
	/// </summary>
	/// <param name="soundName">The sndinfo token of the sound to play.</param>
	void playAndSetDelay(const std::string& soundName);

	/// <summary>
	/// Dictionary of all loaded announcers, mapped by their name.
	/// </summary>
	std::unordered_map<std::string, Announcer_s> announcerDict;

	/// <summary>
	/// The currently loaded and used announcer.
	/// </summary>
	Announcer_s loadedAnnouncer = Announcer_s();

	/// <summary>
	/// An empty metadata structure.
	/// </summary>
	AnnouncerMetaData_s emptyMetadata = AnnouncerMetaData_s();
};

// In-game announcer logic functions.

/// <summary>
/// Run logic to determine to play a frag warning for this frag.
/// </summary>
void P_CheckFragWarnings();

/// <summary>
/// Run logic to determine to play a time warning (5 minutes or 1 minute remaining).
/// </summary>
void P_CheckTimeWarnings();

/// <summary>
/// Run logic to determine to play a 'player eliminated' announcement.
/// Only if the player who was eliminated is the display player.
/// </summary>
void P_CheckPlayerEliminatedAnnouncement(const player_t* player);

/// <summary>
/// Checks if it's time to announce 'fight' when the game starts.
/// Uses getIngameStartTime() to work clientside and in demos.
/// </summary>
void P_CheckFightAnnouncement();

/// <summary>
/// Checks if it's time to announce countdown numbers (5, 4, 3, 2, 1).
/// Uses getCountdown() to work clientside and in demos.
/// </summary>
void P_CheckCountdownAnnouncements();

/// <summary>
/// Captures the current lead state before a score change.
/// Call this BEFORE adding frags or flag captures.
/// </summary>
void P_CaptureLeadState();

/// <summary>
/// Checks for lead changes and announces them to the display player.
/// Announces "You have the lead", "You lost the lead", or "You tied for the lead".
/// Call this AFTER adding frags or flag captures.
/// Does not run in coop game modes.
/// </summary>
void P_CheckLeadChangeAnnouncement();

/// <summary>
/// Checks if this is the first frag of the round and announces "first blood".
/// Only plays in non-duel deathmatch games, client-side only.
/// </summary>
void P_CheckFirstBloodAnnouncement();

/// <summary>
/// Checks if the display player is the last player alive in a survival game.
/// If so, plays the announcer sound. Client-side only.
/// </summary>
void P_CheckLastPlayerAliveAnnouncement();

/// <summary>
/// Ticks the announcer sound queue. Should be called every game tic.
/// Processes queued sounds and plays the next one when the current sound
/// has finished playing.
/// </summary>
void P_TickAnnouncerQueue();

/// <summary>
/// Flushes pending spree and multi kill sounds accumulated during this tic.
/// Only the highest level reached for each category is announced.
/// Should be called once per tic after all kills have been processed.
/// </summary>
void P_FlushPendingAnnouncerSounds();
