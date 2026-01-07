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

#include "odamex.h"

#include "p_mobj.h"
#include "p_local.h"
#include "s_sound.h"

#include "g_multikill.h"
#include "m_ostring.h"

MultiKillManager& MultiKillManager::getInstance()
{
	static MultiKillManager instance;
	return instance;
}

MultiKillManager::~MultiKillManager()
{
	reset();
}

MultiKillManager::MultiKillManager()
{
	multiTimeInterval = 4 * TICRATE;
	emptyLevel = MultiKillLevel_s();
	emptyTics = MultiKillTics_s();
}

void MultiKillManager::reset()
{
	multiKillLevels.clear();
	mutliKillPlayerDict.clear();
	multiTimeInterval = 4 * TICRATE;
}

void MultiKillManager::loadMultiKillDefaults()
{
	multiKillLevels.clear();
	MultiKillLevel_s emptylevel = {"", CR_GRAY};
	// First 2 levels (0 and 1), we insert empty levels as they're not multi kills.
	multiKillLevels.push_back(emptylevel); // 0
	multiKillLevels.push_back(emptylevel); // 1

	// Next, we input the next 9 levels with the default text. We don't use LANGUAGE tokens
	// since some people won't have an updated WAD.
	multiKillLevels.push_back({"Double Kill!",  CR_WHITE});    // 2
	multiKillLevels.push_back({"Triple Kill!",  CR_TAN});      // 3
	multiKillLevels.push_back({"Multi Kill!",   CR_BLUE});     // 4
	multiKillLevels.push_back({"Ultra Kill!",   CR_BRICK});    // 5
	multiKillLevels.push_back({"Overkill!",     CR_CYAN});     // 6
	multiKillLevels.push_back({"Mega Kill!",    CR_CREAM});    // 7
	multiKillLevels.push_back({"Monster Kill!", CR_ORANGE});   // 8
	multiKillLevels.push_back({"Mythic Kill!",  CR_PURPLE});   // 9
	multiKillLevels.push_back({"Killionaire!",  CR_DARKGREEN});// 10
	multiKillLevels.push_back({"Terminator!",   CR_GOLD});     // 11

	multiTimeInterval = 4 * TICRATE;
}

int MultiKillManager::getMultiKillInterval()
{
	return multiTimeInterval;
}

int MultiKillManager::getHighestMultiKillLevel()
{
	return multiKillLevels.size() - 1;
}

const MultiKillLevel_s& MultiKillManager::getMultiKillLevel(const int level)
{
	int newlevel = level;
	if (getHighestMultiKillLevel() <= 0)
		return emptyLevel;


	if (level >= multiKillLevels.size())
		newlevel = multiKillLevels.size() - 1;

	return multiKillLevels.at(newlevel);
}

void MultiKillManager::setMultiKillLevels(const std::vector<MultiKillLevel_s> multikills,
                                          const int newinterval)
{
	multiKillLevels = multikills;
	multiTimeInterval = newinterval * TICRATE;
}

// ==========================================================
// Multi kill bookkeeping functions start here.
// ==========================================================

const MultiKillTics_s& MultiKillManager::getMultiKills(const int playerid)
{
	if (mutliKillPlayerDict.find(playerid) == mutliKillPlayerDict.end())
	{
		return emptyTics;
	}
	else
	{
		return mutliKillPlayerDict[playerid];
	}
}

void MultiKillManager::addKill(const int playerid)
{
	MultiKillTics_s status = MultiKillTics_s();
	status.lastKillTime = ::gametic;
	status.ticsRemaining = multiTimeInterval;

	if (mutliKillPlayerDict.find(playerid) == mutliKillPlayerDict.end())
	{
		status.multiKills = 1;
		mutliKillPlayerDict[playerid] = status;
	}
	else
	{
		status.multiKills = mutliKillPlayerDict[playerid].multiKills + 1;
		mutliKillPlayerDict[playerid] = status;
	}
}

void MultiKillManager::ticPlayerMultiKill(const int playerid)
{
	if (mutliKillPlayerDict.find(playerid) == mutliKillPlayerDict.end())
	{
		return;
	}

	MultiKillTics_s& status = mutliKillPlayerDict[playerid];

	if (status.ticsRemaining)
	{
		status.ticsRemaining--;

		// We out of tics?
		if (status.ticsRemaining == 0)
		{
			// Remove the current multi kill as well.
			status.multiKills = 0;
		}
	}
}

void MultiKillManager::eraseMultiKills(const int playerid)
{
	if (mutliKillPlayerDict.find(playerid) == mutliKillPlayerDict.end())
	{
		return;
	}

	mutliKillPlayerDict.erase(playerid);
}

void MultiKillManager::clearMultiTics()
{
	mutliKillPlayerDict.clear();
}

// ==========================================================
// Static functions start here.
// ==========================================================

void P_ProcessMultiKills(const AActor* source, const player_t* target)
{
	MultiKillManager& manager = MultiKillManager::getInstance();

	if (target)
	{
		manager.eraseMultiKills(target->id);
	}

	if (!source || !source->player)
		return;

	manager.addKill(source->player->id);

	// Now get the player's new multi kill total
	// To see what sound we should play, if any.
	const MultiKillTics_s& status = manager.getMultiKills(source->player->id);

	if (displayplayer_id == source->player->id && status.multiKills > 1)
	{
		// Play the sound for the new multi kill
		//S_Sound(CHAN_ANNOUNCER, '', 1, ATTN_NONE);
	}
}

void P_TicMultiKill(const player_t* player)
{
	if (!player)
		return;

	MultiKillManager::getInstance().ticPlayerMultiKill(player->id);
}
