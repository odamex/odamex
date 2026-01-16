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

void AnnouncerManager::loadAnnouncerDefaults()
{
	Announcer_s defaultAnnouncer;

	defaultAnnouncer.name = "Odamex Official Announcer";
	defaultAnnouncer.description = "The official Odamex announcer pack.";
	defaultAnnouncer.author = "Manc";

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
	defaultAnnouncer.soundDict[ANN_FIGHT] = "officialvox/fight";
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
	defaultAnnouncer.soundDict["multi 1"] = "officialvox/spree/killingspree";
	defaultAnnouncer.soundDict["multi 2"] = "officialvox/spree/rampage";
	defaultAnnouncer.soundDict["multi 3"] = "officialvox/spree/dominating";
	defaultAnnouncer.soundDict["multi 4"] = "officialvox/spree/unstoppable";
	defaultAnnouncer.soundDict["multi 5"] = "officialvox/spree/untouchable";
	defaultAnnouncer.soundDict["multi 6"] = "officialvox/spree/legendary";

	announcerDict[defaultAnnouncer.name] = defaultAnnouncer;

	loadedAnnouncer = announcerDict[defaultAnnouncer.name];
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

EXTERN_CVAR(sv_fraglimit)

void P_CheckFragWarnings()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

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

EXTERN_CVAR(g_lives)

void P_CheckFightAnnouncement()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	// Check if it's time to announce (when level.time reaches ingame start time)
	int ingameStartTime = ::levelstate.getIngameStartTime();
	if (ingameStartTime == 0)
	{
		// Reset the flag when not in-game so it can trigger again next game
		instance.resetFightAnnouncement();
		return;
	}

	// Check if we've already announced
	if (instance.hasFightBeenAnnounced())
		return;

	if (::level.time != ingameStartTime)
		return;

	instance.setFightAnnounced();
	std::string sound = instance.getTokenForEvent(ANN_FIGHT);
	if (!sound.empty() && S_FindSound(sound.c_str()) != -1)
		S_Sound(CHAN_ANNOUNCER, sound.c_str(), 1, ATTN_NONE);
}

void P_CheckCountdownAnnouncements()
{
	// Only play sounds on the client
	if (!::clientside)
		return;

	AnnouncerManager& instance = AnnouncerManager::getInstance();

	int countdown = ::levelstate.getCountdown();

	// Reset flags when countdown is over or above 5
	if (countdown == 0 || countdown > 5)
	{
		instance.resetCountdownAnnouncements();
		return;
	}

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

void P_CheckPlayerEliminatedAnnouncement(const player_t* player)
{
	// Only play sounds on the client
	if (!::clientside)
		return;

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
