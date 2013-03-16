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
//  Server voting-specific stuff.
//
//-----------------------------------------------------------------------------

#ifndef __SV_VOTE__
#define __SV_VOTE__

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
	unsigned int get_countdown(void)
	{
		return this->countdown;
	}
	std::string get_error(void)
	{
		return this->error;
	}
	vote_result_t get_result(void)
	{
		return this->result;
	}
	std::string get_votestring(void)
	{
		return this->votestring;
	}
	vote_result_t check(void);
	size_t count_yes(void);
	size_t count_no(void);
	size_t count_abs(void);
	size_t calc_yes(const bool noabs = false);
	size_t calc_no(void);
	void ev_disconnect(player_t &player);
	bool ev_tic(void);
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
	virtual bool tic(void)
	{
		return true;
	}
	// Subclass this method with the actual commands that should be executed
	// if the vote passes.
	virtual bool exec(void)
	{
		return true;
	}
};

void SV_Callvote(player_t &player);
void SV_Vote(player_t &player);

void Vote_Disconnect(player_t &player);
void Vote_Runtic(void);

#endif
