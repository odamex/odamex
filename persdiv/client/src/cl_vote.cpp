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
//	Clientside voting-specific stuff.
//
//-----------------------------------------------------------------------------

#include <sstream>

#include "c_vote.h"
#include "cl_vote.h"

#include "c_dispatch.h"
#include "cl_main.h"
#include "cl_maplist.h"
#include "cmdlib.h"
#include "i_net.h"
#include "i_system.h"

//////// VOTING STATE ////////

// Return a singleton reference for the class.
VoteState& VoteState::instance() {
	static VoteState singleton;
	return singleton;
}

void VoteState::set(const vote_state_t &vote_state) {
	this->visible = true;
	this->result = vote_state.result;
	this->votestring = vote_state.votestring;
	this->countdown = vote_state.countdown;
	this->countdown_ms = I_MSTime();
	this->yes = vote_state.yes;
	this->yes_needed = vote_state.yes_needed;
	this->no = vote_state.no;
	this->no_needed = vote_state.no_needed;
	this->abs = vote_state.abs;
}

bool VoteState::get(vote_state_t &vote_state) {
	if (!this->visible) {
		return false;
	}

	// If the vote is decided, fade the vote out after 5 seconds
	if (this->result != VOTE_UNDEC && this->countdown_ms + (1000 * 5) < I_MSTime()) {
		this->visible = false;
		return false;
	}

	short countdown = (this->countdown / 35) - ((I_MSTime() - this->countdown_ms) / 1000);

	// If the vote is undecided, don't show the vote HUD if countdown has
	// reached 0 without an update from the server.
	if (this->result == VOTE_UNDEC && countdown <= 0) {
		this->visible = false;
		return false;
	}

	vote_state.result = this->result;
	vote_state.votestring = this->votestring;
	// Adjust the countdown based on how much time has passed since we
	// were last sent the state of the vote.
	vote_state.countdown = countdown;
	vote_state.yes = this->yes;
	vote_state.yes_needed = this->yes_needed;
	vote_state.no = this->no;
	vote_state.no_needed = this->no_needed;
	vote_state.abs = this->abs;

	return true;
}

//////// CALLBACKS & ERRBACKS ////////

void CMD_MapVoteErrback(const std::string &error) {
	Printf(PRINT_HIGH, "callvote failed: %s\n", error.c_str());
}

void CMD_MapVoteCallback(const query_result_t &result) {
	if (result.empty()) {
		CMD_MapVoteErrback("No maps were found that match your requested map.");
		return;
	}

	if (result.size() > 1) {
		CMD_MapVoteErrback("There were too many maps that match your requested map, please be more specific.");
		return;
	}

	// Send the callvote packet to the server.
	std::ostringstream index;
	index << result[0].first;

	MSG_WriteMarker(&net_buffer, clc_callvote);
	MSG_WriteByte(&net_buffer, VOTE_MAP);
	MSG_WriteByte(&net_buffer, 1);
	MSG_WriteString(&net_buffer, index.str().c_str());
}

void CMD_RandmapVoteErrback(const std::string &error) {
	Printf(PRINT_HIGH, "callvote failed: %s\n", error.c_str());
}

void CMD_RandmapVoteCallback(const query_result_t &result) {
	if (result.empty()) {
		CMD_MapVoteErrback("Maplist is empty.");
		return;
	}

	if (result.size() == 1) {
		CMD_MapVoteErrback("The maplist only has one map in it.");
		return;
	}

	MSG_WriteMarker(&net_buffer, clc_callvote);
	MSG_WriteByte(&net_buffer, VOTE_RANDMAP);
	MSG_WriteByte(&net_buffer, 0);
}

//////// CONSOLE COMMANDS ////////

/**
 * Send a vote request to the server.
 */
BEGIN_COMMAND(callvote) {
	// Dumb question, but are we even connected to a server?
	if (!connected) {
		Printf(PRINT_HIGH, "callvote failed: You are not connected to a server.\n");
		return;
	}

	std::vector<std::string> arguments = VectorArgs(argc, argv);

	// Sending a VOTE_NONE will query the server for a
	// list of types of votes that are allowed.
	vote_type_t votecmd = VOTE_NONE;

	if (!arguments.empty()) {
		// Match our first argument up with a vote type.
		std::string votecmd_s = arguments[0];
		for (unsigned char i = (VOTE_NONE + 1);i < VOTE_MAX;++i) {
			if (votecmd_s.compare(vote_type_cmd[i]) == 0) {
				// Found it.  Set our votecmd and get rid of our
				// first argument, since we don't need it anymore.
				votecmd = (vote_type_t)i;
				arguments.erase(arguments.begin());
			}
		}

		if (votecmd == VOTE_NONE) {
			// We passed an argument but it wasn't a valid vote type.
			Printf(PRINT_HIGH, "callvote failed: Invalid vote \"%s\".\n", votecmd_s.c_str());
			return;
		}
	}

	// Prepare the specific vote command.
	switch (votecmd) {
	case VOTE_NONE:
	case VOTE_FORCESTART:
	case VOTE_RANDCAPS:
	case VOTE_NEXTMAP:
	case VOTE_RESTART:
	case VOTE_COINFLIP:
		// No arguments are necessary.
		arguments.resize(0);
		break;
	case VOTE_FORCESPEC:
	case VOTE_RANDPICKUP:
	case VOTE_FRAGLIMIT:
	case VOTE_SCORELIMIT:
	case VOTE_TIMELIMIT:
		// Only one argument is necessary.
		arguments.resize(1);
		break;
	case VOTE_KICK:
		// We can have 0 to 255 arguments.
		if (arguments.size() > 255) {
			arguments.resize(255);
		}
		break;
	case VOTE_MAP:
		// If we have no arguments, ask for more.
		if (arguments.empty()) {
			Printf(PRINT_HIGH, "callvote failed: \"map\" callvote needs a maplist index or unambiguous map name.\n");
			return;
		}

		// Instead of sending the vote here, we run a deferred maplist
		// query to find the passed map in the maplist.
		MaplistCache::instance().defer_query(arguments, &CMD_MapVoteCallback,
											 &CMD_MapVoteErrback);
		return;
	case VOTE_RANDMAP:
		// No arguments are necessary.
		arguments.resize(0);

		// Instead of sending the vote here, we run a deferred maplsit
		// query to see if we have a maplist at all.
		MaplistCache::instance().defer_query(arguments, &CMD_RandmapVoteCallback,
											 &CMD_RandmapVoteErrback);
		return;
	default:
		DPrintf("callvote: Unknown uncaught votecmd %d.\n", votecmd);
		return;
	}

	MSG_WriteMarker(&net_buffer, clc_callvote);
	MSG_WriteByte(&net_buffer, (byte)votecmd);
	MSG_WriteByte(&net_buffer, (byte)(arguments.size()));
	for (std::vector<std::string>::iterator it = arguments.begin();
		 it != arguments.end();++it) {
		MSG_WriteString(&net_buffer, it->c_str());
	}
} END_COMMAND(callvote)

/**
 * Sends a "yes" vote to the server.
 */
BEGIN_COMMAND(vote_yes) {
	if (!connected) {
		Printf(PRINT_HIGH, "vote failed: You are not connected to a server.\n");
		return;
	}

	MSG_WriteMarker(&net_buffer, clc_vote);
	MSG_WriteBool(&net_buffer, true);
} END_COMMAND(vote_yes)

/**
 * Sends a "no" vote to the server.
 */
BEGIN_COMMAND(vote_no) {
	if (!connected) {
		Printf(PRINT_HIGH, "vote failed: You are not connected to a server.\n");
		return;
	}

	MSG_WriteMarker(&net_buffer, clc_vote);
	MSG_WriteBool(&net_buffer, false);
} END_COMMAND(vote_no)

void CL_VoteUpdate(void) {
	vote_state_t vote_state;
	vote_state.result = (vote_result_t)MSG_ReadByte();
	vote_state.votestring = MSG_ReadString();
	vote_state.countdown = MSG_ReadShort();
	vote_state.yes = MSG_ReadByte();
	vote_state.yes_needed = MSG_ReadByte();
	vote_state.no = MSG_ReadByte();
	vote_state.no_needed = MSG_ReadByte();
	vote_state.abs = MSG_ReadByte();

	VoteState::instance().set(vote_state);
}
