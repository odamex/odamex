// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2022 by The Odamex Team.
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
//   [Blair] This system is used to replay certain messages on the client
//   when messages arrive before the object referencing them does.
//   This is usually due to high load, lag, or initial map reset
//   which heavily throttles the initial item send.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "cl_replay.h"
#include "cl_main.h"

#include "p_mobj.h"
#include "p_local.h"
#include "infomap.h"

extern int world_index;
extern int last_svgametic;

//
// ClientReplay::getInstance
//
// Singleton pattern
// Returns a pointer to the only allowable ClientReplay object
//

ClientReplay& ClientReplay::getInstance()
{
	static ClientReplay instance;
	return instance;
}

ClientReplay::~ClientReplay()
{
	ClientReplay::reset();
}

ClientReplay::ClientReplay()
{
	replayed = false;
	replayDoneCounter = TICRATE * 7;
	firstReadyTic = 0;
}

//
// ClientReplay::reset
//
// Erases the replay queues.
//

void ClientReplay::reset()
{
	itemReplayStack.clear();
}

//
// ClientReplay::enabled
//
// Denotes whether an online game is running on the client.
//

bool ClientReplay::enabled()
{
	return (clientside && multiplayer && connected);
}

//
// ClientReplay::replayedTic
//
// Denotes whether an item was replayed during the last tic.
//

bool ClientReplay::wasReplayed()
{
	return replayed;
}

//
// ClientReplay::recordReplayItem
//
// Records a netid and gametic for an item that was picked up,
// but did not exist on the clientside.
//
void ClientReplay::recordReplayItem(int tic, const uint32_t netId)
{
	itemReplayStack.push_back(std::make_pair(tic, netId));
}

//
// ClientReplay::removeReplayItem
//
// Deletes a replay item from the replay queue.
//
void ClientReplay::removeReplayItem(const std::pair<int, uint32_t> replayItem)
{
	std::vector<std::pair<int, uint32_t> >::iterator it = itemReplayStack.begin();
	while (it != itemReplayStack.end())
	{
		if (replayItem == *it)
		{
			itemReplayStack.erase(it);
			break;
		}
		else
		{
				++it;
		}
	}
}

//
// ClientReplay::itemReplayThink
//
// Runs logic for the current tic
// 
void ClientReplay::itemReplay()
{
	if (itemReplayStack.empty() || !consoleplayer().mo || !enabled() ||
	    consoleplayer_id != displayplayer_id || consoleplayer().spectator)
		return;

	player_t& player = consoleplayer();

	weaponstate_t state = P_GetWeaponState(&player);

	if (state == readystate && firstReadyTic <= 0)
	{
		firstReadyTic = ::last_svgametic;
	}
	else if (state != readystate)
	{
		firstReadyTic = 0;
	}

	if (replayed && --replayDoneCounter <= 0)
	{
		replayDoneCounter = TICRATE * 7;
		replayed = false;
	}

	std::vector<std::pair<int, uint32_t> >::iterator it = itemReplayStack.begin();
	while (it != itemReplayStack.end())
	{
		if (it->first + MAX_REPLAY_TIC_LENGTH < static_cast<uint32_t>(::last_svgametic))
		{
			it = itemReplayStack.erase(it);
			continue;
		}

		AActor* mo = P_FindThingById(it->second);

		// Invalid? Try again another day!
		if (!mo)
		{
			++it;
			continue;
		}

		// Let's not do dropped weapons for now
		if (mo->flags & MF_DROPPED)
		{
			it = itemReplayStack.erase(it);
			continue;
		}

		std::string weaponname = P_MobjToName(mo->type);
		weapontype_t weapontype = P_NameToWeapon(weaponname);
		bool weaponSwitch = P_CheckSwitchWeapon(&player, weapontype);

		P_GiveSpecial(&player, mo);

		replayed = true;

		int ticDelta = (::last_svgametic - firstReadyTic);

		firstReadyTic = 0;

		// Cycle the raise/lower by the tics elapsed since to get us up to current
		// But have special logic here to not skip ahead,
		// and do nothing if it wasn't going to switch our weapon anyway.
		if (P_SpecialIsWeapon(mo) && state == readystate && weaponSwitch)
		{
			for (int i = 0; i < ticDelta; ++i)
			{
					P_MovePsprites(&player);
			}
		}

		it = itemReplayStack.erase(it);
	}
}
