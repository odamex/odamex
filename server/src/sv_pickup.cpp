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


#include "odamex.h"

#include <algorithm>
#include <sstream>

#include "c_dispatch.h"
#include "cmdlib.h"
#include "m_fixed.h"
#include "d_netinf.h"
#include "d_player.h"
#include "m_random.h"
#include "sv_main.h"
#include "p_local.h"

EXTERN_CVAR(sv_gametype)
EXTERN_CVAR(sv_teamsinplay)

// Distribute X number of players between teams.
bool Pickup_DistributePlayers(size_t num_players, std::string &error) {
	// This function shouldn't do anything unless you're in a teamgame.
	if (!G_IsTeamGame()) {
		error = "Server is not in a team game.";
		return false;
	}

	// We can't distribute more than MAXPLAYERS so don't even try.
	if (num_players > MAXPLAYERS) {
		error = "Can't distribute that many players.";
		return false;
	}

	// Track all eligible players.
	std::vector<std::reference_wrapper<player_t>> eligible;
	for (auto& player : players) {
		if (validplayer(player) && player.ingame() &&
		    (!(player.spectator) || (player.spectator && player.ready))) {
			eligible.emplace_back(player);
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
	std::shuffle(eligible.begin(), eligible.end(), rng);
	eligible.resize(num_players);

	const int teamCount = sv_teamsinplay.asInt();
	std::vector<team_t> team_order;
	team_order.reserve(teamCount);

	for (int i = 0; i < teamCount; i++)
		team_order.push_back(static_cast<team_t>(i));

	// and the teams too, to make sure which team gets an odd one out is random
	std::shuffle(team_order.begin(), team_order.end(), rng);

	// Rip through our eligible vector, forcing players in the vector
	// onto alternating teams.
	for (size_t i = 0; i < eligible.size(); i ++) {
		player_t &player = eligible[i];
		team_t dest_team = team_order[i % teamCount];

		// Force-join the player if he's spectating.
		SV_SetPlayerSpec(player, false, true);

		// Switch player to the proper team, ensure the correct color,
		// and then update everyone else in the game about it.
		//
		// [SL] Kill the player if they are switching teams so they don't end up
		// holding their own team's flags
		if (player.mo && player.userinfo.team != dest_team)
			P_DamageMobj(player.mo, 0, 0, 1000, 0);

		SV_ForceSetTeam(player, dest_team);
		SV_CheckTeam(player);
		for (auto & pit : players) {
			SV_SendUserInfo(player, &(pit.client));
		}
	}

	// Force-spectate everyone who is not eligible.
	for (auto& player : players) {
		if (std::find(eligible.begin(), eligible.end(), &player) == eligible.end()) {
			SV_SetPlayerSpec(player, true, true);
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
		PrintFmt(PRINT_HIGH, "{}\n", error);
		return;
	}

	if (!Pickup_DistributePlayers(num_players, error)) {
		PrintFmt(PRINT_HIGH, "{}\n", error);
	}
} END_COMMAND (randpickup)

BEGIN_COMMAND (randcaps) {
	std::string error;
	if (!Pickup_DistributePlayers(sv_teamsinplay.asInt(), error)) {
		PrintFmt(PRINT_HIGH, "{}\n", error);
	}
} END_COMMAND (randcaps)

// randomize all players' teams
nonstd::expected<void, std::string> Pickup_DistributeAllPlayers() {
	// This function shouldn't do anything unless you're in a teamgame.
	if (!G_IsTeamGame()) {
		return nonstd::make_unexpected("Server is not in a team game.");
	}

	// Track all eligible players.
	std::vector<std::reference_wrapper<player_t>> eligible;
	for (auto& player : players) {
		if (validplayer(player)) {
			eligible.emplace_back(player);
		}
	}

	if (eligible.empty()) {
		return nonstd::make_unexpected("No eligible players for distribution.");
	}

	const int teamCount = sv_teamsinplay.asInt();
	std::vector<team_t> team_order;
	team_order.reserve(teamCount);

	for (int i = 0; i < teamCount; i++) {
		team_order.push_back(static_cast<team_t>(i));
	}

	// Jumble up our eligible players
	std::shuffle(eligible.begin(), eligible.end(), rng);
	// and the teams too, to make sure which team gets an odd one out is random
	std::shuffle(team_order.begin(), team_order.end(), rng);

	// Rip through our eligible vector, forcing players in the vector
	// onto alternating teams.
	for (size_t i = 0; i < eligible.size(); i++) {
		player_t& player = eligible[i];

		SV_ForceSetTeam(player, team_order[i % teamCount]);
		SV_CheckTeam(player);

		for (auto& pit : players) {
			SV_SendUserInfo(player, &(pit.client));
		}
	}

	return {};
}