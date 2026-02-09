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

#include "odamex.h"

#include "w_wad.h"
#include "oscanner.h"
#include "gstrings.h"
#include "g_announcer.h"
#include "g_gametype.h"
#include "g_levelstate.h"
#include "s_sound.h"
#include "p_horde.h"
#include "p_tick.h"

AnnouncerManager& AnnouncerManager::getInstance()
{
	static AnnouncerManager instance;
	return instance;
}

AnnouncerManager::AnnouncerManager()
{

}

AnnouncerManager::~AnnouncerManager()
{

}

void AnnouncerManager::reset()
{
	announcerDict.clear();
	loadedAnnouncer = Announcer_s();
}

bool AnnouncerManager::isAnnouncerLoaded(const std::string& announcer) const
{
	return announcerDict.find(announcer) != announcerDict.end();
}

bool AnnouncerManager::namedTokenExists(const std::string& tokenName)
{
	// Build a list of all tokens if we haven't already.
	if (allTokens.size() == 0)
	{
		allTokens.insert(allTokens.end(), announcerCTFTokens.begin(), announcerCTFTokens.end());
		allTokens.insert(allTokens.end(), announcerHordeTokens.begin(), announcerHordeTokens.end());
		allTokens.insert(allTokens.end(), announcerSurvivalTokens.begin(), announcerSurvivalTokens.end());
		allTokens.insert(allTokens.end(), announcerCountdownTokens.begin(), announcerCountdownTokens.end());
		allTokens.insert(allTokens.end(), announcerTimeWarningsTokens.begin(), announcerTimeWarningsTokens.end());
		allTokens.insert(allTokens.end(), announcerFirstBloodTokens.begin(), announcerFirstBloodTokens.end());
		allTokens.insert(allTokens.end(), announcerFragTrackingTokens.begin(), announcerFragTrackingTokens.end());
		allTokens.insert(allTokens.end(), announcerLeadTrackingTokens.begin(), announcerLeadTrackingTokens.end());
		allTokens.insert(allTokens.end(), announcerResultTrackingTokens.begin(), announcerResultTrackingTokens.end());
	}

	if (std::find(allTokens.begin(), allTokens.end(), tokenName) != allTokens.end())
	{
		return true;
	}

	return false;
}

std::string AnnouncerManager::getLeftAnnouncer(const std::string& currentAnnouncer) const
{
	if (announcerDict.empty())
	{
		return "";
	}
	if (announcerDict.size() == 1)
	{
		return announcerDict.begin()->first;
	}

	auto it = announcerDict.find(currentAnnouncer);

	if (it == announcerDict.end())
	{
		return announcerDict.begin()->first;
	}
	else if (it == announcerDict.begin())
	{
		// Wrap around to the end
		it = std::prev(announcerDict.end());

		return it->first;
	}
	else
	{
		it = std::prev(it);
		return it->first;
	}
}

std::string AnnouncerManager::getRightAnnouncer(const std::string& currentAnnouncer) const
{
	if (announcerDict.empty())
	{
		return "";
	}
	if (announcerDict.size() == 1)
	{
		return announcerDict.begin()->first;
	}
	auto it = announcerDict.find(currentAnnouncer);
	if (it == announcerDict.end())
	{
		return announcerDict.begin()->first;
	}
	else
	{
		it = std::next(it);

		if (it == announcerDict.end())
		{
			// Wrap around to the beginning
			return announcerDict.begin()->first;
		}
		else
		{
			return it->first;
		}
	}
}

const AnnouncerMetaData_s& AnnouncerManager::getAnnouncerMetadata(const std::string& announcer)
{
	auto it = announcerDict.find(announcer);
	if (it != announcerDict.end())
	{
		return it->second.metadata;
	}
	return emptyMetadata;
}

void AnnouncerManager::loadAnnouncerDefaults()
{
	Announcer_s defaultAnnouncer;

	AnnouncerMetaData_s metadata = AnnouncerMetaData_s();

	metadata.name = "Official Odamex Announcer";
	metadata.description = "The official Odamex announcer pack.";
	metadata.author = "Manc";

	// Fill in default sounds
	// Possessive CTF Announcements
	defaultAnnouncer.soundDict[ANN_YOURFLAGTAKEN] = "officialvox/your/flag/take";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGTAKEN] = "officialvox/enemy/flag/take";
	defaultAnnouncer.soundDict[ANN_YOURFLAGDROPPED] = "officialvox/your/flag/drop";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGDROPPED] = "officialvox/enemy/flag/drop";
	defaultAnnouncer.soundDict[ANN_YOURFLAGISBEINGRETURNED] = "officialvox/your/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGISBEINGRETURNED] = "officialvox/enemy/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_YOURFLAGRETURNED] = "officialvox/your/flag/return";
	defaultAnnouncer.soundDict[ANN_ENEMYFLAGRETURNED] = "officialvox/enemy/flag/return";
	defaultAnnouncer.soundDict[ANN_YOURTEAMSCORES] = "officialvox/your/score";
	defaultAnnouncer.soundDict[ANN_ENEMYTEAMSCORES] = "officialvox/enemy/score";

	// Team Based CTF Announcements
	defaultAnnouncer.soundDict[ANN_REDFLAGTAKEN] = "officialvox/red/flag/take";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGTAKEN] = "officialvox/blue/flag/take";
	defaultAnnouncer.soundDict[ANN_GREENFLAGTAKEN] = "officialvox/green/flag/take";
	defaultAnnouncer.soundDict[ANN_REDFLAGDROPPED] = "officialvox/red/flag/drop";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGDROPPED] = "officialvox/blue/flag/drop";
	defaultAnnouncer.soundDict[ANN_GREENFLAGDROPPED] = "officialvox/green/flag/drop";
	defaultAnnouncer.soundDict[ANN_REDFLAGISBEINGRETURNED] = "officialvox/red/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGISBEINGRETURNED] = "officialvox/blue/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_GREENFLAGISBEINGRETURNED] = "officialvox/green/flag/manualreturn";
	defaultAnnouncer.soundDict[ANN_REDFLAGRETURNED] = "officialvox/red/flag/return";
	defaultAnnouncer.soundDict[ANN_BLUEFLAGRETURNED] = "officialvox/blue/flag/return";
	defaultAnnouncer.soundDict[ANN_GREENFLAGRETURNED] = "officialvox/green/flag/return";
	defaultAnnouncer.soundDict[ANN_REDTEAMSCORES] = "officialvox/red/score";
	defaultAnnouncer.soundDict[ANN_BLUETEAMSCORES] = "officialvox/blue/score";
	defaultAnnouncer.soundDict[ANN_GREENTEAMSCORES] = "officialvox/green/score";

	// Horde Mode Announcements
	defaultAnnouncer.soundDict[ANN_HORDEBOSSSPAWN] = "officialvox/horde/bossspawn";
	defaultAnnouncer.soundDict[ANN_LASTPLAYERALIVE] = "officialvox/lastplayeralive";
	defaultAnnouncer.soundDict[ANN_REVIVEDPLAYER] = "officialvox/horde/revivedplayer";

	// General Announcements
	defaultAnnouncer.soundDict[ANN_FIVEMINUTEWARNING] = "officialvox/fiveminutewarning";
	defaultAnnouncer.soundDict[ANN_ONEMINUTEWARNING] = "officialvox/oneminutewarning";
	defaultAnnouncer.soundDict[ANN_THREEFRAGSLEFT] = "officialvox/threefragsleft";
	defaultAnnouncer.soundDict[ANN_TWOFRAGSLEFT] = "officialvox/twofragsleft";
	defaultAnnouncer.soundDict[ANN_ONEFRAGLEFT] = "officialvox/onefragleft";
	defaultAnnouncer.soundDict[ANN_FIVE] = "officialvox/five";
	defaultAnnouncer.soundDict[ANN_FOUR] = "officialvox/four";
	defaultAnnouncer.soundDict[ANN_THREE] = "officialvox/three";
	defaultAnnouncer.soundDict[ANN_TWO] = "officialvox/two";
	defaultAnnouncer.soundDict[ANN_ONE] = "officialvox/one";
	defaultAnnouncer.soundDict[ANN_FIGHT] = "officialvox/fight";
	defaultAnnouncer.soundDict[ANN_PLAYERELIMINATED] = "officialvox/playereliminated";
	defaultAnnouncer.soundDict[ANN_FIRSTBLOOD] = "officialvox/firstblood";

	// Lead Change Announcements
	defaultAnnouncer.soundDict[ANN_YOUHAVETHELEAD] = "officialvox/youhavethelead";
	defaultAnnouncer.soundDict[ANN_YOULOSTTHELEAD] = "officialvox/youlostthelead";
	defaultAnnouncer.soundDict[ANN_YOUTIEDFORTHELEAD] = "officialvox/youtiedforthelead";

	// Match Result Announcements
	defaultAnnouncer.soundDict[ANN_YOUWIN] = "officialvox/youwin";
	defaultAnnouncer.soundDict[ANN_YOULOSE] = "officialvox/youlose";
	defaultAnnouncer.soundDict[ANN_YOUTIED] = "officialvox/youtied";

	// Multi Kill Announcements
	defaultAnnouncer.soundDict["multi 2"] = "officialvox/multi/doublekill";
	defaultAnnouncer.soundDict["multi 3"] = "officialvox/multi/triplekill";
	defaultAnnouncer.soundDict["multi 4"] = "officialvox/multi/multikill";
	defaultAnnouncer.soundDict["multi 5"] = "officialvox/multi/ultrakill";
	defaultAnnouncer.soundDict["multi 6"] = "officialvox/multi/overkill";
	defaultAnnouncer.soundDict["multi 7"] = "officialvox/multi/megakill";
	defaultAnnouncer.soundDict["multi 8"] = "officialvox/multi/monsterkill";
	defaultAnnouncer.soundDict["multi 9"] = "officialvox/multi/mythickill";
	defaultAnnouncer.soundDict["multi 10"] = "officialvox/multi/killionaire";
	defaultAnnouncer.soundDict["multi 11"] = "officialvox/multi/terminator";

	// Spree Announcements
	defaultAnnouncer.soundDict["spree 1"] = "officialvox/spree/killingspree";
	defaultAnnouncer.soundDict["spree 2"] = "officialvox/spree/rampage";
	defaultAnnouncer.soundDict["spree 3"] = "officialvox/spree/dominating";
	defaultAnnouncer.soundDict["spree 4"] = "officialvox/spree/unstoppable";
	defaultAnnouncer.soundDict["spree 5"] = "officialvox/spree/untouchable";
	defaultAnnouncer.soundDict["spree 6"] = "officialvox/spree/legendary";

	defaultAnnouncer.metadata = metadata;

	announcerDict[metadata.name] = defaultAnnouncer;

	loadedAnnouncer = announcerDict[metadata.name];
}

const std::string AnnouncerManager::getTokenForEvent(const std::string& event)
{
	auto it = loadedAnnouncer.soundDict.find(event);
	if (it != loadedAnnouncer.soundDict.end())
	{
		return it->second;
	}
	return "";
}

void AnnouncerManager::loadAnnouncerByName(const std::string& announcer)
{
	auto it = announcerDict.find(announcer);
	if (it != announcerDict.end())
	{
		loadedAnnouncer = it->second;
	}
	else
	{
		// Load default announcer if nothing is found.
		if (announcerDict.empty())
		{
			loadAnnouncerDefaults();
		}
		else
		{
			loadedAnnouncer = announcerDict.begin()->second;
		}
	}
}

void AnnouncerManager::loadAnnouncers(const std::unordered_map<std::string, Announcer_s> newAnnouncers)
{
	for (auto& it : newAnnouncers)
	{
		if (announcerDict.find(it.first) == announcerDict.end())
		{
			announcerDict[it.first] = it.second;
		}
		else
		{
			// Merge existing announcer with new one.
			Announcer_s& existingAnnouncer = announcerDict[it.first];
			// Update metadata
			existingAnnouncer.metadata = it.second.metadata;
			// Merge sound dictionaries
			for (auto& soundIt : it.second.soundDict)
			{
				existingAnnouncer.soundDict[soundIt.first] = soundIt.second;
			}
		}
	}
}

void AnnouncerManager::resetFragWarnings()
{
	fragWarning3Announced = false;
	fragWarning2Announced = false;
	fragWarning1Announced = false;
}

void AnnouncerManager::announceFragWarning3()
{
	fragWarning3Announced = true;
	std::string sound = getTokenForEvent(ANN_THREEFRAGSLEFT);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void AnnouncerManager::announceFragWarning2()
{
	fragWarning2Announced = true;
	fragWarning3Announced = true;
	std::string sound = getTokenForEvent(ANN_TWOFRAGSLEFT);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void AnnouncerManager::announceFragWarning1()
{
	fragWarning1Announced = true;
	fragWarning2Announced = true;
	fragWarning3Announced = true;
	std::string sound = getTokenForEvent(ANN_ONEFRAGLEFT);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void AnnouncerManager::announceFiveMinuteWarning()
{
	std::string sound = getTokenForEvent(ANN_FIVEMINUTEWARNING);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void AnnouncerManager::announceOneMinuteWarning()
{
	std::string sound = getTokenForEvent(ANN_ONEMINUTEWARNING);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

EXTERN_CVAR(sv_fraglimit)

#ifdef CLIENT_APP
EXTERN_CVAR(snd_announcefragtracking)
#endif

void P_CheckFragWarnings()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

#ifdef CLIENT_APP
	if (!snd_announcefragtracking)
		return;
#endif

	if (!G_UsesFraglimit() || sv_fraglimit <= 0)
		return;

	// Find the leading score (player or team)
	int leadingScore = 0;

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	if (G_IsTeamGame())
	{
		TeamsView tv = TeamQuery().sortScore().filterSortMax().execute();
		if (!tv.empty())
			leadingScore = tv.front()->Points;
	}
	else
	{
		PlayerResults pr = PlayerQuery().sortFrags().filterSortMax().execute();
		if (pr.count > 0)
			leadingScore = pr.players.front()->fragcount;
	}

	int fragsRemaining = sv_fraglimit.asInt() - leadingScore;

	if (fragsRemaining > 3)
	{
		// Reset warnings when more than 3 frags away
		instance.resetFragWarnings();
	}
	else if (fragsRemaining == 3 && !instance.hasFragWarning3BeenAnnounced())
	{
		instance.announceFragWarning3();
	}
	else if (fragsRemaining == 2 && !instance.hasFragWarning2BeenAnnounced())
	{
		instance.announceFragWarning2();
	}
	else if (fragsRemaining == 1 && !instance.hasFragWarning1BeenAnnounced())
	{
		instance.announceFragWarning1();
	}
}

EXTERN_CVAR(sv_timelimit)

#ifdef CLIENT_APP
EXTERN_CVAR(snd_announcetimewarnings)
#endif

void P_CheckTimeWarnings()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

	// Only during active gameplay
	if (::levelstate.getState() != LevelState::INGAME)
		return;

	if (sv_timelimit <= 0.0f)
		return;

#ifdef CLIENT_APP
	if (!snd_announcetimewarnings)
		return;
#endif

	// Only run once per second
	if (!P_AtInterval(TICRATE))
		return;

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	// Calculate remaining time in seconds
	int endingTic = G_GetEndingTic();
	int remainingTics = endingTic - ::level.time;
	int remainingSeconds = remainingTics / TICRATE;

	// Check for 5 minute warning (299 seconds, so it announces when the clock reads 5:00)
	if (remainingSeconds == (5 * 60) - 1)
	{
		instance.announceFiveMinuteWarning();
	}
	// Check for 1 minute warning (59 seconds, so it announces when the clock hits 1:00)
	else if (remainingSeconds == 60 - 1)
	{
		instance.announceOneMinuteWarning();
	}
}

EXTERN_CVAR(g_lives)

#ifdef CLIENT_APP
EXTERN_CVAR(snd_announcecountdown)
#endif

void P_CheckFightAnnouncement()
{
	// Only play sounds on the client
	if (!::clientside || !::multiplayer)
		return;

	// Don't announce fight if we're a spectator
	if (consoleplayer().spectator)
		return;

#ifdef CLIENT_APP
	if (!snd_announcecountdown)
		return;
#endif

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	// Check if we've already announced
	if (instance.hasFightBeenAnnounced())
		return;

	// Check if it's time to announce (when level.time reaches ingame start time)
	int ingameStartTime = ::levelstate.getIngameStartTime();

	if (::levelstate.getState() != LevelState::INGAME)
		return;

	// Start of the round happened 3 seconds ago and we didn't announce?
	// Forget about it
	if (::level.time > ingameStartTime + (3 * TICRATE))
	{
		instance.setFightAnnounced();
		return;
	}

	if (::level.time < ingameStartTime)
		return;

	instance.setFightAnnounced();
	std::string sound = instance.getTokenForEvent(ANN_FIGHT);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void P_CheckCountdownAnnouncements()
{
	// Only play sounds on the client
	if (!::clientside || !::multiplayer)
		return;

	// Don't announce fight if we're a spectator
	if (consoleplayer().spectator)
		return;

#ifdef CLIENT_APP
	if (!snd_announcecountdown)
		return;
#endif

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	// If we got reset to warmup (player chickened out)
	// Reset the countdown so we hear it again.
	if (::levelstate.getState() == LevelState::WARMUP)
	{
		instance.resetCountdownAnnouncements();
	}

	int countdown = ::levelstate.getCountdown();

	// Check if this countdown has already been announced
	if (instance.hasCountdownBeenAnnounced(countdown))
		return;

	// Get the appropriate sound token
	std::string token;
	switch (countdown)
	{
	case 5: token = ANN_FIVE; break;
	case 4: token = ANN_FOUR; break;
	case 3: token = ANN_THREE; break;
	case 2: token = ANN_TWO; break;
	case 1: token = ANN_ONE; break;
	default: return;
	}

	instance.setCountdownAnnounced(countdown);
	std::string sound = instance.getTokenForEvent(token);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

#ifdef CLIENT_APP
EXTERN_CVAR(snd_announcesurvival)
#endif

void P_CheckPlayerEliminatedAnnouncement(const player_t* player)
{
	// Only play sounds on the client
	if (!::clientside)
		return;

#ifdef CLIENT_APP
	if (!snd_announcesurvival)
		return;
#endif

	if (!player)
		return;

	// Only announce if lives are enabled and the player is out of lives
	if (!(g_lives && player->lives <= 0))
		return;

	// Only announce for the display player
	if (player->id != displayplayer_id)
		return;

	std::string sound = AnnouncerManager::getInstance().getTokenForEvent(ANN_PLAYERELIMINATED);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

#ifdef CLIENT_APP
EXTERN_CVAR(snd_announceleadtracking)
#endif

// Helper function to determine current lead state for the display player
static bool P_GetDisplayPlayerLeadState(bool& outHasLead, bool& outIsTied)
{
	outHasLead = false;
	outIsTied = false;

	player_t& displayPlayer = idplayer(displayplayer_id);

	// Make sure the display player is valid and in the game
	if (!validplayer(displayPlayer) || !displayPlayer.ingame())
		return false;

	if (G_IsTeamGame())
	{
		if (G_IsLivesGame() && sv_gametype != GM_CTF)
		{
			// Team game with lives - check team lives
			TeamsView tv = TeamQuery().sortLives().filterSortMax().execute();
			if (tv.empty())
				return false;
			if (tv.size() > 1)
			{
				outIsTied = true;
				for (const auto& team : tv)
				{
					if (team->Team == displayPlayer.userinfo.team)
					{
						outHasLead = true;
						break;
					}
				}
			}
			else
			{
				outHasLead = (tv.front()->Team == displayPlayer.userinfo.team);
			}
		}
		else
		{
			// Team game - check team scores
			TeamsView tv = TeamQuery().sortScore().filterSortMax().execute();
			if (tv.empty())
				return false;
			if (tv.size() > 1)
			{
				outIsTied = true;
				for (const auto& team : tv)
				{
					if (team->Team == displayPlayer.userinfo.team)
					{
						outHasLead = true;
						break;
					}
				}
			}
			else
			{
				outHasLead = (tv.front()->Team == displayPlayer.userinfo.team);
			}
		}
	}
	else
	{
		if (G_IsLivesGame())
		{
			// FFA game with lives - check individual lives
			PlayerResults pr = PlayerQuery().sortLives().filterSortMax().execute();
			if (pr.count == 0)
				return false;
			if (pr.count > 1)
			{
				outIsTied = true;
				for (const auto& player : pr.players)
				{
					if (player->id == displayplayer_id)
					{
						outHasLead = true;
						break;
					}
				}
			}
			else
			{
				outHasLead = (pr.players.front()->id == displayplayer_id);
			}
		}
		else
		{
			// FFA game - check individual frags
			PlayerResults pr = PlayerQuery().sortFrags().filterSortMax().execute();
			if (pr.count == 0)
				return false;
			if (pr.count > 1)
			{
				outIsTied = true;
				for (const auto& player : pr.players)
				{
					if (player->id == displayplayer_id)
					{
						outHasLead = true;
						break;
					}
				}
			}
			else
			{
				outHasLead = (pr.players.front()->id == displayplayer_id);
			}
		}
	}

	return true;
}

// Call this BEFORE a score change to capture current lead state
void P_CaptureLeadState()
{
	// Only on client
	if (!::clientside || !::multiplayer)
		return;

	// No lead announcements in coop game modes
	if (G_IsCoopGame())
		return;

	// Only check during active gameplay
	if (::levelstate.getState() != LevelState::INGAME)
	{
		AnnouncerManager::getInstance().resetLeadTracking();
		return;
	}

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	bool hasLead = false;
	bool isTied = false;

	if (P_GetDisplayPlayerLeadState(hasLead, isTied))
	{
		instance.setDisplayPlayerHasLead(hasLead);
		instance.setLeadTied(isTied);
	}
}

// Call this AFTER a score change to check for lead changes and announce
void P_CheckLeadChangeAnnouncement()
{
	// Only play sounds on the client
	if (!::clientside || !::multiplayer)
		return;

#ifdef CLIENT_APP
	if (!snd_announceleadtracking)
		return;
#endif

	// No lead announcements in coop game modes
	if (G_IsCoopGame())
		return;

	// Only check during active gameplay
	if (::levelstate.getState() != LevelState::INGAME)
		return;

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	// Get the previous state (captured before score change)
	bool previouslyHadLead = instance.doesDisplayPlayerHaveLead();
	bool previouslyTied = instance.isLeadTied();

	// Get current state (after score change)
	bool newHasLead = false;
	bool newIsTied = false;

	if (!P_GetDisplayPlayerLeadState(newHasLead, newIsTied))
		return;

	std::string sound;

	if (newHasLead && !previouslyHadLead)
	{
		// Gained the lead
		if (newIsTied)
			sound = instance.getTokenForEvent(ANN_YOUTIEDFORTHELEAD);
		else
			sound = instance.getTokenForEvent(ANN_YOUHAVETHELEAD);
	}
	else if (!newHasLead && previouslyHadLead)
	{
		// Lost the lead
		sound = instance.getTokenForEvent(ANN_YOULOSTTHELEAD);
	}
	else if (newHasLead && previouslyHadLead)
	{
		// Still has the lead - check for tie status changes
		if (newIsTied && !previouslyTied)
			sound = instance.getTokenForEvent(ANN_YOUTIEDFORTHELEAD);
		else if (!newIsTied && previouslyTied)
			sound = instance.getTokenForEvent(ANN_YOUHAVETHELEAD);
	}

	// Update stored state for next comparison
	instance.setDisplayPlayerHasLead(newHasLead);
	instance.setLeadTied(newIsTied);

	// Play announcement if we have one
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

#ifdef CLIENT_APP
EXTERN_CVAR(snd_announcefirstblood)
#endif

void P_CheckFirstBloodAnnouncement()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

#ifdef CLIENT_APP
	if (!snd_announcefirstblood)
		return;
#endif

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	// No first blood unless in active gameplay
	if (::levelstate.getState() != LevelState::INGAME)
		return;

	// Check if already announced this game
	if (instance.hasFirstBloodBeenAnnounced())
		return;

	// Only play in non-duel deathmatch games
	if ((sv_gametype != GM_DM && sv_gametype != GM_TEAMDM) || G_IsDuelGame())
		return;

	instance.setFirstBloodAnnounced();
	std::string sound = instance.getTokenForEvent(ANN_FIRSTBLOOD);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

#ifdef CLIENT_APP
EXTERN_CVAR (snd_announcehorde)
#endif

void P_CheckBossSpawnAnnouncement()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

	if (!G_IsHordeMode())
		return;

#ifdef CLIENT_APP
	if (!snd_announcehorde)
		return;
#endif

	const hordeInfo_t& info = P_HordeInfo();

	if (info.hasBoss() && info.bossTic() != 0)
		return;

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	std::string sound = instance.getTokenForEvent(ANN_HORDEBOSSSPAWN);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void P_CheckLastPlayerAliveAnnouncement()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

	// Only in games with lives
	if (!g_lives)
		return;

	// Must be in active gameplay
	if (::levelstate.getState() != LevelState::INGAME)
		return;

	if (G_IsCoopGame())
	{
		// Check how many players have lives
		PlayerResults pr = PlayerQuery().hasLives().execute();

		// In coop, reset if more than 1 player has lives
		if (pr.count > 1)
			return;
		// Need exactly 1 player with lives
		if (pr.count != 1 || P_NumPlayersInGame() <= 1)
			return;
		// Check if the last player alive is the display player
		if (pr.players.front()->id != consoleplayer_id)
			return;
	}
	else if (G_IsTeamGame())
	{
		const player_t& player = consoleplayer();

		if (!validplayer(player) || !player.ingame())
			return;

		PlayerResults livingplayers = PlayerQuery().onTeam(player.userinfo.team).hasLives().execute();

		if (livingplayers.count > 0 ||
		    livingplayers.players.front()->id != consoleplayer_id)
		{
			// Either teammates are still alive, or the last player alive is not the console player
			return;
		}
	}
	else
	{
		// Only announce player alive messages for coop and team games
		return;
	}

	std::string sound = AnnouncerManager::getInstance().getTokenForEvent(ANN_LASTPLAYERALIVE);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}
