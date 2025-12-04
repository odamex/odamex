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

struct MultiKillLevel_s
{
	std::string multikilltext;
	EColorRange color;
};

class MultiKillManager
{
	public:
		MultiKillManager();
		~MultiKillManager();
		static MultiKillManager& getInstance(); // returns the instantiated MultiKillManager object

		void reset(); // called when loading a new wad

		void setMultiKillLevels(const std::vector<MultiKillLevel_s> multikills, int newinterval); // called when reading SPREEDEF to input new multi kill definitions
		MultiKillLevel_s getMultiKillLevel(int level); // Gets the local multi kill level (with text and color)

		int getMultiKillInterval(); // gets the multi kill interval for players

		int getHighestMultiKillLevel(); // gets the highest multi kill level loaded

		void loadMultiKillDefaults(); // called if no SPREEDEF is found
	private:
		int multiTimeInterval;
		std::vector<MultiKillLevel_s> multiKillLevels;
};

void P_ProcessMultiKills(AActor* source, player_t* target);
void P_TicMultiKill(player_t* player);
