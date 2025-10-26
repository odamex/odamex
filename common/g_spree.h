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

struct spree_s
{
	std::string spreeText;
	EColorRange color;
};

class SpreeManager
{
	public:
		SpreeManager();
		~SpreeManager();
	  static SpreeManager& getInstance(); // returns the instantiated SpreeManager
	                                      // object

		void reset(); // called when loading a new wad

		void setSpreeLevels(const std::vector<spree_s> sprees,
	                               int newinterval); // called when reading SPREEDEF to
	                                                 // input new spree definitions
	  spree_s getSpreeLevel(
	        int level); // Gets the local spree level (with text and color)

		int getSpreeInterval(); // gets the multi kill interval for players

		int getHighestSpreeLevel(); // gets the highest multi kill level loaded

		void loadSpreeDefaults(); // called if no SPREEDEF is found
	private:
		int spreeKillInterval;
		std::vector<spree_s> spreeLevels;
};

void G_ProcessSprees(AActor* source, player_t* target);
void G_TicSprees(player_t* player);
