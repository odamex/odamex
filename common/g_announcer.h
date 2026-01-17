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
/// Plays when fighting is enabled, either start of a new round or weapons unlocked.
/// </summary>
static const std::string ANN_FIGHT = "fight";
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
/// This structure represents a single announcer pack, and its specified data.
/// </summary>
struct Announcer_s
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
	/// <summary>
	/// Dictionary of the sounds in this announcer pack.
	/// Each named sound refers to a sndinfo token, which is given to the
	/// sound function to play the appropriate sound.
	/// </summary>
	std::unordered_map<std::string, std::string> soundDict;
	Announcer_s() : name(""), description(""), author(""), soundDict() { }
	Announcer_s(const Announcer_s& other)
	{
		name = other.name;
		description = other.description;
		author = other.author;
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
	/// Loads default announcer information if no ONCRINFO is found.
	/// </summary>
	void loadAnnouncerDefaults();

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
		leadTrackingInitialized = false;
	}

	/// <summary>
	/// Gets whether lead tracking has been initialized.
	/// </summary>
	bool isLeadTrackingInitialized() const { return leadTrackingInitialized; }

	/// <summary>
	/// Sets lead tracking as initialized.
	/// </summary>
	void setLeadTrackingInitialized() { leadTrackingInitialized = true; }

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

	/// <summary>
	/// Gets the sndinfo token for the current announcer for the
	/// following event.
	/// </summary>
	/// <param name="token">Token for the event to play. One
	/// of the announcements above.</param>
	/// <returns>The specified sndinfo token for the current announcer, empty if invalid or unknown.</returns>
	const std::string getTokenForEvent(const std::string& event);

	/// <summary>
	/// Loads the specified announcer, or if not found, loads the default announcer.
	/// </summary>
	/// <param name="announcer">Name of the announcer to load.</param>
	void loadAnnouncerByName(const std::string& announcer);

private:
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
	/// Tracks whether lead tracking has been initialized for this game.
	/// </summary>
	bool leadTrackingInitialized = false;

	/// <summary>
	/// Dictionary of all loaded announcers, mapped by their name.
	/// </summary>
	std::unordered_map<std::string, Announcer_s> announcerDict;

	/// <summary>
	/// The currently loaded and used announcer.
	/// </summary>
	Announcer_s loadedAnnouncer = Announcer_s();
};

// In-game announcer logic functions.

/// <summary>
/// Run logic to determine to play a frag warning for this frag.
/// </summary>
void P_CheckFragWarnings();

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
/// Checks for lead changes and announces them to the display player.
/// Announces "You have the lead", "You lost the lead", or "You tied for the lead".
/// Does not run in coop game modes.
/// </summary>
void P_CheckLeadChangeAnnouncement();
