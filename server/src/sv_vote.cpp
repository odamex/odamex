// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2011 by The Odamex Team.
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
//	Server voting-specific stuff.
//
//-----------------------------------------------------------------------------

#include "d_player.h"
#include "sv_main.h"
#include "sv_maplist.h"
#include "sv_vote.h"

#include <algorithm>
#include <cstdlib>
#include <sstream>

#include <string>
#include <vector>

EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_scorelimit)
EXTERN_CVAR(sv_timelimit)

EXTERN_CVAR(sv_vote_majority)
EXTERN_CVAR(sv_vote_timelimit)
EXTERN_CVAR(sv_vote_timeout)

EXTERN_CVAR(sv_callvote_kick)
EXTERN_CVAR(sv_callvote_map)
EXTERN_CVAR(sv_callvote_fraglimit)
EXTERN_CVAR(sv_callvote_scorelimit)
EXTERN_CVAR(sv_callvote_timelimit)

namespace sv {

// Return a singleton reference for the class
Voting& Voting::instance() {
	static Voting singleton;
	return singleton;
}

//////// COMMANDS FROM CLIENT ////////

// Handle callvote commands from the client.
void Voting::callvote(player_t &player) {
	vote_type_t votecmd = (vote_type_t)MSG_ReadByte();
	byte argc = (byte)MSG_ReadByte();

	std::vector<std::string> arguments(argc);
	for (int i = 0;i < argc;i++) {
		arguments[i] = std::string(MSG_ReadString());
	}

	// Return a list of valid votes if the client doesn't pass a vote.
	if (votecmd == VOTE_NONE) {
		SV_PlayerPrintf(PRINT_HIGH, player.id, "Valid votes are: fraglimit, kick, map, randmap, scorelimit, timelimit.\n");
		return;
	}

	// Is another vote already in progress?
	if (vote_type != VOTE_NONE) {
		SV_PlayerPrintf(PRINT_HIGH, player.id, "Invalid callvote, another vote is already in progress.\n");
		return;
	}

	// Sanity-check the vote.
	bool valid;
	std::string error;
	std::string argstring;
	std::string reason;

	switch (votecmd) {
	case VOTE_KICK:
		if (!sv_callvote_kick) {
			error = "kick vote has been disabled by the server.";
			valid = false;
			break;
		}

		valid = cmd_kick_check(arguments, error,
							   vote_argument_size_t, reason);
		if (!valid) {
			break;
		}

		// Additional logic needed for votekicking vs console kicking.
		if (vote_argument_size_t == player.id) {
			// Stop the player from trying to kick himself.
			error = "You can't votekick yourself!  Try 'disconnect' instead.";
			valid = false;
			break;
		} else {
			// Construct a custom argstring.
			std::ostringstream buffer;
			buffer << vote_type_cmd[votecmd] << " ";
			buffer << idplayer(vote_argument_size_t).userinfo.netname;
			buffer << " (id:" << vote_argument_size_t << ")";
			argstring = buffer.str();
		}
		break;
	case VOTE_MAP:
		if (!sv_callvote_map) {
			error = "map vote has been disabled by the server.";
			valid = false;
			break;
		}
		valid = vcmd_votemap_check(arguments, argstring, error,
								   vote_argument_size_t);
		break;
	case VOTE_RANDMAP:
		if (!sv_callvote_map) {
			error = "map vote has been disabled by the server.";
			valid = false;
			break;
		}
		valid = sv::Maplist::instance().randmap_check(error);
		break;
	case VOTE_FRAGLIMIT:
		if (!sv_callvote_fraglimit) {
			error = "fraglimit vote has been disabled by the server.";
			valid = false;
			break;
		}
		valid = cv_fraglimit_check(arguments, error, vote_argument_int);
		break;
	case VOTE_SCORELIMIT:
		if (!sv_callvote_scorelimit) {
			error = "scorelimit vote has been disabled by the server.";
			valid = false;
			break;
		}
		valid = cv_scorelimit_check(arguments, error, vote_argument_int);
		break;
	case VOTE_TIMELIMIT:
		if (!sv_callvote_timelimit) {
			error = "timelimit vote has been disabled by the server.";
			valid = false;
			break;
		}
		valid = cv_timelimit_check(arguments, error, vote_argument_float);
		break;
	default:
		return;
	}

	// If we encountered an unrecoverable error with the vote,
	// abort the vote.
	if (!valid) {
		SV_PlayerPrintf(PRINT_HIGH, player.id, "%s\n", error.c_str());
		return;
	}

	// If we haven't already assembled a custom argument string,
	// assemble a plain string with the vote type and all passed arguments
	if (argstring.compare("") == 0) {
		argstring = vote_type_cmd[votecmd];
		for (unsigned int i = 0;i < arguments.size();++i) {
			argstring = argstring + " " + arguments[i];
		}
	}

	// Is the player alone?  Pass the vote automatically if so.
	if (players.size() == 1) {
		vote_type = votecmd;
		vote_argstring = argstring;
		voting_parse(VOTE_YES);
		return;
	}

	// Check timeout for this player.
	if (vote_timeout.count(player.id) > 0) {
		int timeout = level.time - vote_timeout[player.id];
		if (timeout < (sv_vote_timeout.asInt() * TICRATE)) {
			// Too soon, turn the timeout back into normal seconds for the meassage.
			timeout = sv_vote_timeout.asInt() - (timeout / TICRATE);
			SV_PlayerPrintf(PRINT_HIGH, player.id, "Please wait another %d second%s to call a vote.\n", timeout, (timeout != 1) ? "s" : "");
			return;
		}
	}

	// Set voting state
	vote_type = votecmd;
	vote_argstring = argstring;
	vote_pid = player.id;
	vote_countdown = sv_vote_timelimit.asInt() * TICRATE;

	// Give everybody an "undecided" vote except the current player.
	for (std::vector<player_t>::size_type i = 0;i != players.size();i++) {
		if (players[i].id == vote_pid) {
			vote_tally[vote_pid] = VOTE_YES;
		} else {
			vote_tally[players[i].id] = VOTE_UNDEC;
		}
	}

	SV_BroadcastPrintf(PRINT_HIGH, "%s has called a vote for %s.\n", player.userinfo.netname, argstring.c_str());
	return;
}

// Handle vote commands from the client.
void Voting::vote(player_t &player) {
	bool ballot = MSG_ReadBool();

	// Is there even a vote going on?
	if (vote_type == VOTE_NONE) {
		SV_PlayerPrintf(PRINT_HIGH, player.id, "Invalid vote, no vote in progress.\n");
		return;
	}

	// Is the player who called the vote trying to change his vote?
	if (player.id == vote_pid && ballot == false) {
		SV_BroadcastPrintf(PRINT_HIGH, "Vote %s failed! (Canceled)\n", vote_argstring.c_str());

		// We only want a voting timeout for the player who called the vote.
		// This prevents abusive players from calling a vote and then canceling
		// it to lock other players out of voting.
		vote_timeout[vote_pid] = level.time;

		voting_init();
		return;
	}

	vote_result_t ballot_r = (ballot == true) ? VOTE_YES : VOTE_NO;

	// Did the user already vote the same way before?
	if (vote_tally.count(player.id) > 0) {
		if (vote_tally[player.id] == ballot_r) {
			return;
		}
	}

	vote_tally[player.id] = ballot_r;
	SV_BroadcastPrintf(PRINT_HIGH, "%s voted %s.\n", player.userinfo.netname, ballot == true ? "Yes" : "No");
}

//////// VOTING FUNCTIONS ////////

// Start with a clean voting slate.
void Voting::voting_init(void) {
	vote_type = VOTE_NONE;
	vote_countdown = 0;
	vote_tally.clear();
}

// Tally up the number of players who are voting for the current callvote.
int Voting::voting_yes(void) {
	// Does the tally have any entries in it?
	if (vote_tally.size() == 0) {
		return 0;
	}

	int count = 0;
	std::map<int, vote_result_t>::iterator it;

	// Count the for votes.
	for (it = vote_tally.begin();it != vote_tally.end();++it) {
		if ((*it).second == VOTE_YES) {
			count++;
		}
	}

	return count;
}

// Tally up the number of players who are voting against the current callvote.
int Voting::voting_no(void) {
	// Does the tally have any entries in it?
	if (vote_tally.size() == 0) {
		return 0;
	}

	int count = 0;
	std::map<int, vote_result_t>::iterator it;

	// Count the against votes.
	for (it = vote_tally.begin();it != vote_tally.end();++it) {
		if ((*it).second == VOTE_NO) {
			count++;
		}
	}

	return count;
}

// Returns if the result of a vote is a forgone conclusion.
vote_result_t Voting::voting_check(void) {
	// Does the tally have any entries in it?
	if (vote_tally.size() == 0) {
		return VOTE_UNDEC;
	}

	int yes = voting_yes();
	int no = voting_no();
	int undec = vote_tally.size() - yes - no;

	// NOTE: This is accurate to float precision.  A more general case that
	//       survives Wolfram Alpha scrutiny would be nice.
	float epsilon = (float)1 / (MAXPLAYERS * 2);

	// If everyone has voted then go ahead and calculate the winner.
	if (undec == 0) {
		if (((float)yes / vote_tally.size() - sv_vote_majority) >= epsilon) {
			return VOTE_YES;
		} else {
			return VOTE_NO;
		}
	}

	if (((float)yes / vote_tally.size() - sv_vote_majority) >= epsilon) {
		return VOTE_YES;
	}

	if (((float)no / vote_tally.size() - sv_vote_majority) >= epsilon) {
		return VOTE_NO;
	}

	return VOTE_UNDEC;
}

// Parse the result of the vote.
void Voting::voting_parse(vote_result_t vote_result) {
	if (players.size() > 1) {
		int yes = voting_yes();
		int no = voting_no();
		int abs = vote_tally.size() - yes - no;

		// If the vote did not pass, fail it and return.
		if (vote_result != VOTE_YES) {
			SV_BroadcastPrintf(PRINT_HIGH, "Vote %s failed! (Yes: %d, No: %d, Abs: %d)\n", vote_argstring.c_str(), yes, no, abs);
			vote_timeout[vote_pid] = level.time;
			voting_init();
			return;
		}
		SV_BroadcastPrintf(PRINT_HIGH, "Vote %s passed! (Yes: %d, No: %d, Abs: %d)\n", vote_argstring.c_str(), yes, no, abs);
		vote_timeout[vote_pid] = level.time;
	} else {
		// There are only two ways to get to this branch.  Either the only player on the server
		// called a vote or two players were on a server, the caller left and the only remaining
		// player let the clock run out.
		if (vote_result != VOTE_YES) {
			SV_BroadcastPrintf(PRINT_HIGH, "Vote %s failed! (Automatic)\n", vote_argstring.c_str());
			voting_init();
			return;
		}
		SV_BroadcastPrintf(PRINT_HIGH, "Vote %s passed! (Automatic)\n", vote_argstring.c_str());
	}

	std::string error;
	switch (vote_type) {
	case VOTE_KICK:
		if (!cmd_kick(vote_argument_size_t, "Vote kick.")) {
			// TODO: Once we get ban timelimits, we ought to ban someone
			//       for longer if they dodge a votekick.
			SV_BroadcastPrintf(PRINT_HIGH, "Vote kick dodged, please alert a server administrator.\n");
		}
		break;
	case VOTE_MAP:
		sv::Maplist::instance().gotomap(vote_argument_size_t);
		break;
	case VOTE_RANDMAP:
		sv::Maplist::instance().randmap();
		break;
	case VOTE_FRAGLIMIT:
		sv_fraglimit.Set(vote_argument_int);
		SV_BroadcastPrintf(PRINT_HIGH,
						   "fraglimit changed to %d.\n", sv_fraglimit.asInt());
		break;
	case VOTE_SCORELIMIT:
		sv_scorelimit.Set(vote_argument_int);
		SV_BroadcastPrintf(PRINT_HIGH,
						   "scorelimit changed to %d.\n", sv_scorelimit.asInt());
		break;
	case VOTE_TIMELIMIT:
		sv_timelimit.Set(vote_argument_float);
		SV_BroadcastPrintf(PRINT_HIGH,
						   "timelimit changed to %g.\n", (float)sv_timelimit);
		break;
	default:
		SV_BroadcastPrintf(PRINT_HIGH, "Vote not yet implemented.\n");
		break;
	}

	// We're done.  Set up voting again.
	voting_init();
}

//////// CALLVOTE FUNCTIONS ////////

// Votemap vote check.
bool Voting::vcmd_votemap_check(std::vector<std::string> &arguments,
								std::string &argstring, std::string &error,
								size_t &index) {
	// Needs at least one argument.
	if (arguments.size() < 1) {
		error = "map needs at least one argument.";
		return false;
	}

	// Map voting runs gotomap.  Run the check command and spit
	// back a response.
	std::vector<std::string> response;
	bool ok = sv::Maplist::instance().gotomap_check(arguments, response, index);

	if (!ok) {
		error = response[0];
		return false;
	}

	// Construct our argstring from the map index.
	std::ostringstream argstream;
	argstream << "map " << sv::Maplist::instance().maplist[index].wads;
	argstream << " " << sv::Maplist::instance().maplist[index].map;
	argstring = argstream.str();

	return true;
}

// Fraglimit vote check.
bool Voting::cv_fraglimit_check(std::vector<std::string> &arguments,
								std::string &error, int &fraglimit) {
	if (arguments.size() < 1) {
		error = "fraglimit needs a second argument.";
		return false;
	}
	arguments.resize(1);

	std::istringstream buffer(arguments[0].c_str());
	buffer >> fraglimit;
	if (!buffer) {
		error = "fraglimit must be a number.";
		return false;
	}
	if (fraglimit < 0) {
		error = "fraglimit must be greater than 0.";
		return false;
	}

	return true;
}

// Scorelimit vote check.
bool Voting::cv_scorelimit_check(std::vector<std::string> &arguments,
								 std::string &error, int &scorelimit) {
	if (arguments.size() < 1) {
		error = "scorelimit needs a second argument.";
		return false;
	}
	arguments.resize(1);

	std::istringstream buffer(arguments[0].c_str());
	buffer >> scorelimit;
	if (!buffer) {
		error = "scorelimit must be a number.";
		return false;
	}
	if (scorelimit < 0) {
		error = "scorelimit must be greater than 0.";
		return false;
	}

	return true;
}

// Timelimit vote check.
bool Voting::cv_timelimit_check(std::vector<std::string> &arguments,
								std::string &error, float &timelimit) {
	if (arguments.size() < 1) {
		error = "timelimit needs a second argument.";
		return false;
	}
	arguments.resize(1);

	std::istringstream buffer(arguments[0].c_str());
	buffer >> timelimit;
	if (!buffer) {
		error = "timelimit must be a number.";
		return false;
	}
	if (timelimit < 0) {
		error = "timelimit must be greater than 0.";
		return false;
	}

	return true;
}

//////// EVENTS ////////

// Prepare a fresh level for voting.
void Voting::event_initlevel(void) {
	vote_timeout.clear();
	voting_init();
}

// Stop voting if maplist has been cleared and we're in the middle of a mapvote.
void Voting::event_clearmaplist(void) {
	if (vote_type == VOTE_MAP || vote_type == VOTE_RANDMAP) {
		SV_BroadcastPrintf(PRINT_HIGH, "Maplist cleared by server, vote canceled.\n");
		vote_timeout.clear();
		voting_init();
	}
}

// Remove a disconnected player from the tally.
void Voting::event_disconnect(player_t &player) {
	if (vote_tally.count(player.id) > 0) {
		vote_tally.erase(player.id);
	}

	// If everyone involved with a vote has left, bail out.
	if (vote_tally.size() == 0 && vote_type != VOTE_NONE) {
		SV_BroadcastPrintf(PRINT_HIGH, "Vote failed! (Automatic)\n");
		voting_init();
	}
}

// Handles tic-by-tic maintenance of voting.
void Voting::event_runtic(void) {
	// Is there even a vote happening?
	if (vote_type == VOTE_NONE) {
		return;
	}

	vote_result_t vote_result = voting_check();

	// Is the countdown still going on?
	if (vote_countdown > 0) {
		// Are we still waiting on a result?
		if (vote_result == VOTE_UNDEC) {
			// Notify players of the callvote every 5 seconds.
			if (vote_countdown % (TICRATE * 5) == 0 &&
				vote_countdown != ((unsigned int)sv_vote_timelimit.asInt() * TICRATE)) {
				SV_BroadcastPrintf(PRINT_HIGH, "%d seconds left to vote...\n", vote_countdown / (TICRATE));
			}
			vote_countdown--;
			return;
		}
	}

	voting_parse(vote_result);
}

}
