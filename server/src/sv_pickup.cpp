// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2012 by Alex Mayfield.
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
//  Serverside "pickup" functionality.  Used to distribute players
//  between teams.
//
//-----------------------------------------------------------------------------

#include <algorithm>
#include <sstream>
#include <vector>

#include "c_cvars.h"
#include "c_dispatch.h"
#include "cmdlib.h"
#include "m_fixed.h"
#include "d_netinf.h"
#include "d_player.h"
#include "m_random.h"
#include "sv_main.h"
#include "p_local.h"

EXTERN_CVAR(sv_gametype)

// Distribute X number of players between teams.
bool Pickup_DistributePlayers(size_t num_players, std::string &error) {
	// This function shouldn't do anything unless you're in a teamgame.
	if (!(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)) {
		error = "Server is not in a team game.";
		return false;
	}

	// We can't distribute more than MAXPLAYERS so don't even try.
	if (num_players > MAXPLAYERS) {
		error = "Can't distribute that many players.";
		return false;
	}

	// Track all eligible players.
	std::vector<size_t> eligible;

	for (size_t i = 0;i < players.size();i++) {
		if (validplayer(players[i]) && players[i].ingame() &&
		    (!players[i].spectator || (players[i].spectator && players[i].ready))) {
			eligible.push_back(i);
		}
	}

	if (eligible.empty()) {
		error = "No eligible players for distribution.";
		return false;
	}

	if (eligible.size() < num_players) {
		error = "Not enough eligible players for distribution.";
		return false;
	}

	// Jumble up our eligible players and cut the number of
	// eligible players to the passed number.
	std::random_shuffle(eligible.begin(), eligible.end());
	eligible.resize(num_players);

	// Rip through our eligible vector, forcing players in the vector
	// onto alternating teams.
	team_t dest_team = TEAM_BLUE;
	for (size_t i = 0;i < eligible.size();i++) {
		player_t &player = players[eligible[i]];

		// Force-join the player if he's spectating.
		SV_SetPlayerSpec(player, false, true);

		// Is the last player an odd-one-out?  Randomize
		// the team he is put on.
		if ((eligible.size() % 2) == 1 && i == (eligible.size() - 1)) {
			dest_team = (team_t)(P_Random() % NUMTEAMS);
		}

		// Switch player to the proper team, ensure the correct color,
		// and then update everyone else in the game about it.
		//
		// [SL] Kill the player if they are switching teams so they don't end up
		// holding their own team's flags
		if (player.mo && player.userinfo.team != dest_team)
			P_DamageMobj(player.mo, 0, 0, 1000, 0);

		SV_ForceSetTeam(player, dest_team);
		SV_CheckTeam(player);
		for (size_t j = 0;j< players.size();j++) {
			SV_SendUserInfo(player, &clients[j]);
		}

		if (dest_team == TEAM_BLUE) {
			dest_team = TEAM_RED;
		} else {
			dest_team = TEAM_BLUE;
		}
	}

	// Force-spectate everyone who is not eligible.
	for (size_t i = 0;i < players.size();i++) {
		if (std::find(eligible.begin(), eligible.end(), i) == eligible.end()) {
			SV_SetPlayerSpec(players[i], true, true);
		}
	}

	return true;
}

bool CMD_RandpickupCheck(const std::vector<std::string> &args,
						 std::string &error, size_t &num_players) {
	if (args.empty()) {
		error = "randcaps needs a single argument, the total number of desired players in game.";
		return false;
	}

	std::istringstream buffer(args[0]);
	buffer >> num_players;
	if (!buffer) {
		error = "Number of players needs to be a numeric value.";
		num_players = 0;
		return false;
	}

	return true;
}

BEGIN_COMMAND (randpickup) {
	std::vector<std::string> arguments = VectorArgs(argc, argv);
	std::string error;

	size_t num_players;
	if (!CMD_RandpickupCheck(arguments, error, num_players)) {
		Printf(PRINT_HIGH, "%s\n", error.c_str());
		return;
	}

	if (!Pickup_DistributePlayers(num_players, error)) {
		Printf(PRINT_HIGH, "%s\n", error.c_str());
	}
} END_COMMAND (randpickup)

BEGIN_COMMAND (randcaps) {
	std::string error;
	if (!Pickup_DistributePlayers(2, error)) {
		Printf(PRINT_HIGH, "%s\n", error.c_str());
	}
} END_COMMAND (randcaps)
