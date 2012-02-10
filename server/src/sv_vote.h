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

#ifndef __SV_VOTE__
#define __SV_VOTE__

#include "c_vote.h"
#include "d_player.h"

namespace sv {

typedef enum {
	VOTE_UNDEC,
	VOTE_NO,
	VOTE_YES
} vote_result_t;

class Voting {
public:
	static Voting& instance(void);
	void callvote(player_t &player);
	void vote(player_t &player);
	void event_initlevel(void);
	void event_clearmaplist(void);
	void event_disconnect(player_t &player);
	void event_runtic(void);
private:
	// State of votes
	vote_type_t vote_type; // Type of vote
	std::string vote_argstring; // Vote string
	byte vote_pid; // Player id who is casting the vote
	unsigned int vote_countdown; // Amount of time left in the vote
	std::map<int, vote_result_t> vote_tally; // Who voted and what they voted
	std::map<int, int> vote_timeout; // The last time someone started a vote
	int vote_argument_int; // Integer vote argument
	size_t vote_argument_size_t; // size_t vote argument
	float vote_argument_float; // float vote argument

	void voting_init(void);
	int voting_yes(void);
	int voting_no(void);
	vote_result_t voting_check(void);
	void voting_parse(vote_result_t vote_result);
	bool vcmd_votemap_check(std::vector<std::string> &arguments,
							std::string &argstring, std::string &error,
							size_t &index);
	bool cv_fraglimit_check(std::vector<std::string> &arguments,
							std::string &error, int &fraglimit);
	bool cv_scorelimit_check(std::vector<std::string> &arguments,
							 std::string &error, int &scorelimit);
	bool cv_timelimit_check(std::vector<std::string> &arguments,
							std::string &error, float &timelimit);
};

}

#endif
