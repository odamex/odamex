// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2024 by The Odamex Team.
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
//  Server voting-specific stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_vote.h"
#include "d_player.h"

class Vote
{
protected:
	byte caller_id;
	unsigned int countdown;
	std::string error;
	vote_result_t result;
	std::map<int, vote_result_t> tally;
	std::string votestring;
public:
	Vote(const char* cname, const cvar_t* ccvar) : countdown(0), error(""), result(VOTE_UNDEC), votestring(""), name(cname), cvar(ccvar) { };
	virtual ~Vote() { };
	const char* name;
	const cvar_t* cvar;
	unsigned int get_countdown() const
	{
		return this->countdown;
	}
	std::string get_error() const
	{
		return this->error;
	}
	vote_result_t get_result() const
	{
		return this->result;
	}
	std::string get_votestring() const
	{
		return this->votestring;
	}
	vote_result_t check();
	size_t count_yes() const;
	size_t count_no() const;
	size_t count_abs() const;
	size_t calc_yes(const bool noabs = false) const;
	size_t calc_no() const;
	vote_state_t serialize() const;
	void ev_disconnect(player_t &player);
	bool ev_tic();
	bool init(const std::vector<std::string> &args, const player_t &player);
	void parse(vote_result_t vote_result);
	bool vote(player_t &player, bool ballot);
	bool setup_check_cvar();
	// Subclass this method with checks that should run before a vote is
	// started.  If the vote can start, you should store enough state to
	// successfully execute the vote once the vote is over, set the
	// votestring, and return true.  Otherwise, set an error and return false.
	virtual bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		return true;
	}
	// Subclass this method with checks that should run every tic.  If the
	// vote should be aborted, set an error and return false.  Otherwise,
	// return true.
	virtual bool tic()
	{
		return true;
	}
	// Subclass this method with the actual commands that should be executed
	// if the vote passes.
	virtual bool exec()
	{
		return true;
	}
};

void SV_Callvote(player_t &player);
void SV_VoteCmd(player_t& player, const std::vector<std::string>& args);
void Vote_Disconnect(player_t &player);
void Vote_Runtic();
