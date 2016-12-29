// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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

#include "cmdlib.h"
#include "c_dispatch.h"
#include "d_player.h"
#include "g_warmup.h"
#include "sv_main.h"
#include "sv_maplist.h"
#include "sv_pickup.h"
#include "sv_vote.h"
#include "d_main.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

EXTERN_CVAR(sv_gametype)

EXTERN_CVAR(sv_fraglimit)
EXTERN_CVAR(sv_scorelimit)
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_warmup_pugs)

EXTERN_CVAR(sv_vote_countabs)
EXTERN_CVAR(sv_vote_majority)
EXTERN_CVAR(sv_vote_speccall)
EXTERN_CVAR(sv_vote_specvote)
EXTERN_CVAR(sv_vote_timelimit)
EXTERN_CVAR(sv_vote_timeout)

EXTERN_CVAR(sv_callvote_coinflip)
EXTERN_CVAR(sv_callvote_forcespec)
EXTERN_CVAR(sv_callvote_forcestart)
EXTERN_CVAR(sv_callvote_kick)
EXTERN_CVAR(sv_callvote_map)
EXTERN_CVAR(sv_callvote_nextmap)
EXTERN_CVAR(sv_callvote_randcaps)
EXTERN_CVAR(sv_callvote_randmap)
EXTERN_CVAR(sv_callvote_randpickup)
EXTERN_CVAR(sv_callvote_restart)
EXTERN_CVAR(sv_callvote_fraglimit)
EXTERN_CVAR(sv_callvote_scorelimit)
EXTERN_CVAR(sv_callvote_timelimit)

// Vote class goes here
Vote *vote = 0;

// Checks a particular vote to see if it's been enabled by the server
bool Vote::setup_check_cvar()
{
	if (!*(this->cvar))
	{
		std::ostringstream buffer;
		buffer << this->name << " vote has been disabled by the server.";
		this->error = buffer.str();
		return false;
	}
	return true;
}

//////// VOTE SUBCLASSES ////////

class CoinflipVote : public Vote
{
public:
	CoinflipVote() : Vote("coinflip", &sv_callvote_coinflip) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		this->votestring = "coinflip";
		return true;
	}
	bool exec(void)
	{
		std::string result;
		CMD_CoinFlip(result);
		SV_BroadcastPrintf(PRINT_HIGH, "%s\n", result.c_str());
		return true;
	}
};

class ForcespecVote : public Vote
{
private:
	byte id;
	std::string netname;
public:
	ForcespecVote() : Vote("forcespec", &sv_callvote_forcespec) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		// Run forcespec command check.
		size_t pid;
		if (!CMD_ForcespecCheck(args, this->error, pid))
		{
			return false;
		}

		// Stop the player from trying to forcespec himself.
		if (pid == player.id)
		{
			this->error = "You can't vote forcespec yourself!  Try 'spectate' instead.";
			return false;
		}

		// Store forcespec information
		this->id = pid;
		this->netname = idplayer(pid).userinfo.netname;

		// Create votestring
		std::ostringstream buffer;
		buffer << "forcespec " << this->netname << " (id:" << pid << ")";
		this->votestring = buffer.str();

		return true;
	}
	bool tic()
	{
		if (!validplayer(idplayer(this->id)))
		{
			std::ostringstream buffer;
			buffer << this->netname << " left the server.";
			this->error = buffer.str();
			return false;
		}
		if (idplayer(this->id).spectator)
		{
			std::ostringstream buffer;
			buffer << this->netname << " became a spectator on his own.";
			this->error = buffer.str();
			return false;
		}
		return true;
	}
	bool exec(void)
	{
		SV_SetPlayerSpec(idplayer(this->id), true);
		return true;
	}
};

class ForcestartVote : public Vote
{
public:
	ForcestartVote() : Vote("forcestart", &sv_callvote_forcestart) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		if (warmup.get_status() != Warmup::WARMUP)
		{
			this->error = "Game is not in warmup mode.";
			return false;
		}

		this->votestring = "forcestart";
		return true;
	}
	bool tic()
	{
		if (warmup.get_status() != Warmup::WARMUP)
		{
			this->error = "No need to force start, game is about to start.";
			return false;
		}

		return true;
	}
	bool exec(void)
	{
		AddCommandString("forcestart");
		return true;
	}
};


class FraglimitVote : public Vote
{
private:
	unsigned int fraglimit;
public:
	FraglimitVote() : Vote("fraglimit", &sv_callvote_fraglimit) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		unsigned int fraglimit;

		if (!Vote::setup_check_cvar())
			return false;

		// Do we have at least one argument?
		if (args.size() < 1)
		{
			this->error = "fraglimit needs a second argument.";
			return false;
		}

		// Is the fraglimit a numeric value?
		std::istringstream buffer(args[0].c_str());
		buffer >> fraglimit;
		if (!buffer)
		{
			this->error = "fraglimit must be a number.";
			return false;
		}

		// Is the fraglimit positive?
		if (args[0].length() > 0 && args[0][0] == '-')
		{
			this->error = "fraglimit must be 0 or a positive number.";
			return false;
		}

		std::ostringstream vote_string;
		vote_string << "fraglimit " << fraglimit;

		this->fraglimit = fraglimit;
		this->votestring = vote_string.str();
		return true;
	}
	bool exec(void)
	{
		sv_fraglimit.Set(this->fraglimit);
		return true;
	}
};

class KickVote : public Vote
{
private:
	std::string caller;
	byte id;
	std::string netname;
	std::string reason;
public:
	KickVote() : Vote("kick", &sv_callvote_kick) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		// Run kick command check.
		size_t pid;
		if (!CMD_KickCheck(args, this->error, pid, this->reason))
		{
			return false;
		}

		// Stop the player from trying to kick himself.
		if (pid == player.id)
		{
			this->error = "You can't votekick yourself!  Try 'disconnect' instead.";
			return false;
		}

		// Store kick information
		this->caller = player.userinfo.netname;
		this->id = pid;
		this->netname = idplayer(pid).userinfo.netname;

		// Create votestring
		std::ostringstream buffer;
		buffer << "kick " << this->netname << " (id:" << (int)this->id << ")";
		if (!this->reason.empty())
		{
			buffer << " \"" << this->reason << "\"";
		}
		this->votestring = buffer.str();

		return true;
	}
	bool tic()
	{
		if (!validplayer(idplayer(this->id)))
		{
			std::ostringstream buffer;
			buffer << this->netname << " left the server.";
			this->error = buffer.str();
			return false;
		}
		return true;
	}
	bool exec(void)
	{
		std::ostringstream buffer;
		if (this->reason.empty())
		{
			buffer << "Votekick called by " << this->caller << " passed.";
		}
		else
		{
			buffer << "Votekick called by " << this->caller << " passed: \""<< this->reason << "\".";
		}
		SV_KickPlayer(idplayer(this->id), buffer.str());
		return true;
	}
};

class MapVote : public Vote
{
private:
	size_t index;
	byte version;
public:
	MapVote() : Vote("map", &sv_callvote_map) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		size_t index;

		if (!Vote::setup_check_cvar())
			return false;

		// We need at least one argument.
		if (args.empty())
		{
			this->error = "map needs at least one argument.";
			return false;
		}

		// For a mapvote, we are passed one argument
		// that is a maplist index.
		std::istringstream buffer(args[0].c_str());
		buffer >> index;
		if (!buffer)
		{
			this->error = "passed maplist index must be a number.";
			return false;
		}

		// Make sure our index is valid and grab the maplist
		// entry associated with the index.
		maplist_entry_t maplist_entry;
		if (!Maplist::instance().get_map_by_index(index, maplist_entry))
		{
			this->error = Maplist::instance().get_error();
			return false;
		}
		this->index = index;
		this->version = Maplist::instance().get_version();

		// Construct our argstring from the map index.
		std::ostringstream vsbuffer;
		vsbuffer << "map ";

		for (std::vector<std::string>::const_iterator it = maplist_entry.wads.begin();
			it!= maplist_entry.wads.end(); ++it)
			vsbuffer << D_CleanseFileName(*it) << " ";

		vsbuffer << maplist_entry.map;
		this->votestring = vsbuffer.str();
		return true;
	}
	bool tic(void)
	{
		if (this->version != Maplist::instance().get_version())
		{
			this->error = "Maplist modified, vote aborted.";
			return false;
		}
		return true;
	}
	bool exec(void)
	{
		G_ChangeMap(this->index);
		return true;
	}
};

class NextmapVote : public Vote
{
public:
	NextmapVote() : Vote("nextmap", &sv_callvote_nextmap) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		// We don't need to keep any state, since "nextmap" takes no arguments
		// and has no failure condition.
		this->votestring = "nextmap";
		return true;
	}
	bool exec(void)
	{
		G_ChangeMap();
		return true;
	}
};

class RandcapsVote : public Vote
{
public:
	RandcapsVote() : Vote("randcaps", &sv_callvote_randcaps) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		// We don't really care about any state until the vote passes.
		this->votestring = "randcaps";
		return true;
	}
	bool exec(void)
	{
		return Pickup_DistributePlayers(2, this->error, sv_warmup_pugs);
	}
};

class RandmapVote : public Vote
{
public:
	RandmapVote() : Vote("randmap", &sv_callvote_randmap) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		// Does the maplist actually have anything in it?
		if (Maplist::instance().empty())
		{
			this->error = "Maplist is empty.";
			return false;
		}

		// We don't need to keep any state, because the only thing we actually
		// care about is if the maplist is empty or not.
		this->votestring = "randmap";
		return true;
	}
	bool tic(void)
	{
		if (Maplist::instance().empty())
		{
			this->error = "Maplist was cleared since the vote started, vote aborted.";
			return false;
		}
		return true;
	}
	bool exec(void)
	{
		return CMD_Randmap(this->error);
	}
};

class RandpickupVote : public Vote
{
private:
	size_t num_players;
public:
	RandpickupVote() : Vote("randpickup", &sv_callvote_randpickup) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		if (!CMD_RandpickupCheck(args, this->error, this->num_players))
		{
			return false;
		}

		// No odd-numbered teams.
		if (this->num_players % 2 != 0)
		{
			this->error = "Teams must be even.";
			return false;
		}

		// Nothing below 2v2.
		if (this->num_players < 4)
		{
			this->error = "Each team must have at least 2 players.";
			return false;
		}

		// Construct argstring...turn it into a nice '#v#'.
		std::ostringstream buffer;
		buffer << "randpickup " << (this->num_players / 2) << "v" << (this->num_players / 2);
		this->votestring = buffer.str();
		return true;
	}
	bool exec(void)
	{
		return Pickup_DistributePlayers(this->num_players, this->error, false);
	}
};

class RestartVote : public Vote
{
public:
	RestartVote() : Vote("restart", &sv_callvote_restart) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		if (!Vote::setup_check_cvar())
			return false;

		// We don't need to keep any state, since "nextmap" takes no arguments
		// and has no failure condition.
		this->votestring = "restart";
		return true;
	}
	bool exec(void)
	{
		// When in warmup mode, we would rather not catch players off guard.
		warmup.reset();

		// Do a countdown-led restart.
		warmup.restart();

		return true;
	}
};

class ScorelimitVote : public Vote
{
private:
	unsigned int scorelimit;
public:
	ScorelimitVote() : Vote("scorelimit", &sv_callvote_scorelimit) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		unsigned int scorelimit;

		if (!Vote::setup_check_cvar())
			return false;

		// Does scorelimit voting make sense in this gametype?
		if (sv_gametype != GM_CTF)
		{
			this->error = "scorelimit does nothing in this gametype.";
			return false;
		}

		// Do we have at least one argument?
		if (args.size() < 1)
		{
			this->error = "scorelimit needs a second argument.";
			return false;
		}

		// Is the scorelimit a numeric value?
		std::istringstream buffer(args[0].c_str());
		buffer >> scorelimit;
		if (!buffer)
		{
			this->error = "scorelimit must be a number.";
			return false;
		}

		// Is the scorelimit positive?
		if (args[0].length() > 0 && args[0][0] == '-')
		{
			this->error = "scorelimit must be 0 or a positive number.";
			return false;
		}

		std::ostringstream vote_string;
		vote_string << "scorelimit " << scorelimit;

		this->scorelimit = scorelimit;
		this->votestring = vote_string.str();
		return true;
	}
	bool exec(void)
	{
		sv_scorelimit.Set(this->scorelimit);
		return true;
	}
};

class TimelimitVote : public Vote
{
private:
	float timelimit;
public:
	TimelimitVote() : Vote("timelimit", &sv_callvote_timelimit) { };
	bool setup(const std::vector<std::string> &args, const player_t &player)
	{
		float timelimit;

		if (!Vote::setup_check_cvar())
			return false;

		// Do we have at least one argument?
		if (args.size() < 1)
		{
			this->error = "timelimit needs a second argument.";
			return false;
		}

		// Is the timelimit a numeric value?
		std::istringstream buffer(args[0].c_str());
		buffer >> timelimit;
		if (!buffer)
		{
			this->error = "timelimit must be a number.";
			return false;
		}

		// Is the timelimit positive?
		if (args[0].length() > 0 && args[0][0] == '-')
		{
			this->error = "timelimit must be 0 or a positive number.";
			return false;
		}

		// Is the timelimit less than a minute?
		if (timelimit > 0.0f && timelimit < 1.0f)
		{
			this->error = "timelimit must either be 0 or greater than 1 minute.";
			return false;
		}

		std::ostringstream vote_string;
		vote_string << "timelimit " << timelimit;

		this->timelimit = timelimit;
		this->votestring = vote_string.str();
		return true;
	}
	bool exec(void)
	{
		sv_timelimit.Set(this->timelimit);
		return true;
	}
};

//////// VOTING FUNCTIONS ////////

// Returns if the result of a vote is a forgone conclusion.
vote_result_t Vote::check(void)
{
	// Does the tally have any entries in it?
	if (this->tally.empty())
	{
		return VOTE_ABANDON;
	}

	size_t yes = this->count_yes();
	size_t no = this->count_no();
	size_t undec = this->tally.size() - yes - no;

	if (yes >= this->calc_yes())
	{
		return VOTE_YES;
	}

	if (no >= this->calc_no())
	{
		return VOTE_NO;
	}

	// If everyone is undecided and neither of the calculations above resulted
	// in anything definitive, err on the side of no.
	if (undec == 0)
	{
		return VOTE_NO;
	}

	// If we've run out of time with an undecided vote, we need a result now.
	if (this->get_countdown() <= 0)
	{
		if (sv_vote_countabs)
		{
			// Since the vote didn't already pass and all vote calculations
			// up to now take absent voters into account, we know it failed.
			return VOTE_NO;
		}
		else
		{
			// This last calculation does not take absent voters into account.
			if (yes >= this->calc_yes(true))
			{
				return VOTE_YES;
			}
			return VOTE_NO;
		}
	}

	return VOTE_UNDEC;
}

// Tally up the number of players who are voting for the current callvote.
size_t Vote::count_yes(void)
{
	// Does the tally have any entries in it?
	if (this->tally.empty())
	{
		return 0;
	}

	int count = 0;
	std::map<int, vote_result_t>::iterator it;

	// Count the for votes.
	for (it = this->tally.begin(); it != this->tally.end(); ++it)
	{
		if ((*it).second == VOTE_YES)
		{
			count++;
		}
	}

	return count;
}

// Calculate the number of players needed for the vote to pass.  Pass true
// for the first param if you don't want to count absent voters.
size_t Vote::calc_yes(const bool noabs)
{
	size_t size;

	if (noabs)
	{
		size = this->count_yes() + this->count_no();
	}
	else
	{
		size = this->tally.size();
	}

	float f_calc = size * sv_vote_majority;
	size_t i_calc = (int)floor(f_calc + 0.5f);
	if (f_calc > i_calc - MPEPSILON && f_calc < i_calc + MPEPSILON)
	{
		return i_calc + 1;
	}
	return (int)ceil(f_calc);
}

// Tally up the number of players who are voting against the current callvote.
size_t Vote::count_no(void)
{
	// Does the tally have any entries in it?
	if (this->tally.empty())
	{
		return 0;
	}

	int count = 0;
	std::map<int, vote_result_t>::iterator it;

	// Count the against votes.
	for (it = this->tally.begin(); it != this->tally.end(); ++it)
	{
		if ((*it).second == VOTE_NO)
		{
			count++;
		}
	}

	return count;
}

// Calculate the number of players needed for the vote to fail.
size_t Vote::calc_no(void)
{
	float f_calc = this->tally.size() * (1.0f - sv_vote_majority);
	size_t i_calc = (int)floor(f_calc + 0.5f);
	if (f_calc > i_calc - MPEPSILON && f_calc < i_calc + MPEPSILON)
	{
		return i_calc;
	}
	return (int)ceil(f_calc);
}

size_t Vote::count_abs(void)
{
	return this->tally.size() - this->count_yes() - this->count_no();
}

// Handle disconnecting players.
void Vote::ev_disconnect(player_t &player)
{
	// If the player had an entry in the tally, delete it.
	if (this->tally.count(player.id) > 0)
	{
		this->tally.erase(player.id);
	}
}

// Initialize the vote and also run the vote-specific 'setup' code.  Return
// false if there is an error state and the vote shouldn't start.
bool Vote::init(const std::vector<std::string> &args, const player_t &player)
{
	// First we need to run our vote-specific checks.
	if (!this->setup(args, player))
	{
		return false;
	}

	// Make sure a spectating player can call the vote
	if (!sv_vote_speccall && player.spectator)
	{
		this->error = "Spectators cannot call votes on this server.";
		return false;
	}

	// Check the vote timeout.
	if (player.timeout_callvote > 0)
	{
		int timeout = level.time - player.timeout_callvote;
		int timeout_check, timeout_waitsec;
		if (players.size() == 1)
		{
			// A single player is made to always wait 10 seconds.
			timeout_check = 10 * TICRATE;
			timeout_waitsec = 10 - (timeout / TICRATE);
		}
		else
		{
			// If there are other players in the game, the callvoting player
			// is bound to sv_vote_timeout.
			timeout_check = sv_vote_timeout.asInt() * TICRATE;
			timeout_waitsec = sv_vote_timeout.asInt() - (timeout / TICRATE);
		}

		if (timeout < timeout_check)
		{
			std::ostringstream buffer;
			buffer << "Please wait another " << timeout_waitsec << " second" << ((timeout_waitsec != 1) ? "s" : "") << " to call a vote.";
			this->error = buffer.str();
			return false;
		}
	}

	// Set voting state
	this->caller_id = player.id;
	this->countdown = sv_vote_timelimit.asInt() * TICRATE;

	// Give everybody an "undecided" vote except the current player.
	for (Players::iterator it = players.begin();it != players.end();++it)
	{
		if (!(it->ingame()))
			continue;

		if (!sv_vote_specvote && it->spectator)
			continue;

		if (it->id == this->caller_id)
			this->tally[this->caller_id] = VOTE_YES;
		else
			this->tally[it->id] = VOTE_UNDEC;
	}

	SV_BroadcastPrintf(PRINT_HIGH, "%s has called a vote for %s.\n", player.userinfo.netname.c_str(), this->get_votestring().c_str());
	return true;
}

void SVC_GlobalVoteUpdate(void);

// Pass or fail the vote.  Most of the time this method adheres to the result
// of the check method, but can also be used to 'force pass' or 'force fail' a
// vote under special circumstances.
void Vote::parse(vote_result_t vote_result)
{
	// Make sure the clients have the final state of the vote
	// before we do anything else.
	SVC_GlobalVoteUpdate();
	for (Players::iterator it = players.begin();it != players.end();++it)
		if (validplayer(*it))
			SV_SendPacket(*it);

	if (this->tally.empty() || vote_result == VOTE_ABANDON)
	{
		SV_BroadcastPrintf(PRINT_HIGH, "Vote %s abandoned!\n", this->votestring.c_str());
		return;
	}

	size_t yes = this->count_yes();
	size_t no = this->count_no();
	size_t abs = this->count_abs();

	if (vote_result == VOTE_INTERRUPT)
	{
		SV_BroadcastPrintf(PRINT_HIGH, "Vote %s interrupted! (Yes: %d, No: %d, Abs: %d)\n", this->votestring.c_str(), yes, no, abs);
		return;
	}

	if (vote_result != VOTE_YES)
	{
		SV_BroadcastPrintf(PRINT_HIGH, "Vote %s failed! (Yes: %d, No: %d, Abs: %d)\n", this->votestring.c_str(), yes, no, abs);

		// Only set the timeout tic if the vote failed.
		player_t caller = idplayer(this->caller_id);
		if (validplayer(caller))
		{
			caller.timeout_callvote = level.time;
		}

		return;
	}

	SV_BroadcastPrintf(PRINT_HIGH, "Vote %s passed! (Yes: %d, No: %d, Abs: %d)\n", this->votestring.c_str(), yes, no, abs);

	// NOTE: This can return false if there is an error, but we're already
	//       catching errors in Vote_Runtic by seeing if the error is
	//       non-empty.  One day, I need to go over this entire class with a
	//       fine-tooth comb and smooth-over error handling in general.
	this->exec();
}

// Runs tic by tic operations for the vote.  Returns true if the vote should
// continue, otherwise returns false if the vote should be destroyed.
bool Vote::ev_tic(void)
{
	// Check to see if the vote has passed.
	this->result = this->check();

	// Run tic-specific vote functions.  Also interrupt if we're in
	// intermission or on a new map.
	if (gamestate == GS_INTERMISSION || level.time == 1 || !this->tic())
	{
		this->result = VOTE_INTERRUPT;
	}

	// Continue the countdown as long as the check returns undecided and
	// the tic function doesn't fail and interrupt the vote.
	if (this->result == VOTE_UNDEC)
	{
		this->countdown -= 1;
		return true;
	}

	// The vote is done, one way or the other.
	return false;
}

// This method is used to change the vote tally.  It will return false
// if the player already voted the same way beforehand.
bool Vote::vote(player_t &player, bool ballot)
{
	vote_result_t ballot_r = (ballot == true) ? VOTE_YES : VOTE_NO;

	// Does the user actually have an entry in the tally?
	if (this->tally.find(player.id) == this->tally.end())
	{
		if (!sv_vote_specvote && player.spectator)
		{
			SV_PlayerPrintf(PRINT_HIGH, player.id, "Spectators can't vote on this server.\n");
		}
		else
		{
			SV_PlayerPrintf(PRINT_HIGH, player.id, "You can't vote on something that was called before you joined the server.\n");
		}
		return false;
	}

	// Did the user already vote the same way before?
	if (this->tally[player.id] == ballot_r)
	{
		return false;
	}

	// Has the user voted too soon after his last vote?
	if (player.timeout_vote > 0)
	{
		int timeout = level.time - player.timeout_vote;

		// Players can only change their minds once every 3 seconds.
		int timeout_check = 3 * TICRATE;
		int timeout_waitsec = 3 - (timeout / TICRATE);

		if (timeout < timeout_check)
		{
			SV_PlayerPrintf(PRINT_HIGH, player.id, "Please wait another %d second%s to change your vote.\n",
			                timeout_waitsec, timeout_waitsec != 1 ? "s" : "");
			return false;
		}
	}

	this->tally[player.id] = ballot_r;
	player.timeout_vote = level.time;
	return true;
}

//////// COMMANDS TO CLIENT ////////

// Send a full vote update to a specific player
void SVC_VoteUpdate(player_t &player)
{
	// Keep us from segfaulting on a non-existant vote
	if (vote == 0)
	{
		return;
	}

	client_t* cl = &player.client;

	MSG_WriteMarker(&cl->netbuf, svc_vote_update);
	MSG_WriteByte(&cl->netbuf, vote->get_result());
	MSG_WriteString(&cl->netbuf, vote->get_votestring().c_str());
	MSG_WriteShort(&cl->netbuf, vote->get_countdown());
	MSG_WriteByte(&cl->netbuf, vote->count_yes());
	MSG_WriteByte(&cl->netbuf, vote->calc_yes());
	MSG_WriteByte(&cl->netbuf, vote->count_no());
	MSG_WriteByte(&cl->netbuf, vote->calc_no());
	MSG_WriteByte(&cl->netbuf, vote->count_abs());
}

// Send a full vote update to everybody
void SVC_GlobalVoteUpdate(void)
{
	for (Players::iterator it = players.begin();it != players.end();++it)
		SVC_VoteUpdate(*it);
}

//////// COMMANDS FROM CLIENT ////////

// Handle callvote commands from the client.
void SV_Callvote(player_t &player)
{
	vote_type_t votecmd = (vote_type_t)MSG_ReadByte();
	byte argc = (byte)MSG_ReadByte();

	DPrintf("SV_Callvote: Got votecmd %s from player %d, %d additional arguments.\n",
	        vote_type_cmd[votecmd], player.id, argc);

	std::vector<std::string> arguments(argc);
	for (int i = 0; i < argc; i++)
	{
		arguments[i] = std::string(MSG_ReadString());
		DPrintf("SV_Callvote: arguments[%d] = \"%s\"\n", i, arguments[i].c_str());
	}

	if (!(votecmd > VOTE_NONE && votecmd < VOTE_MAX))
	{
		// Return a list of valid votes if the client doesn't pass a vote.
		// TODO: Only return enabled votes.
		std::ostringstream buffer;
		buffer << "Valid votes are: ";
		for (int i = VOTE_NONE + 1; i < VOTE_MAX; i++)
		{
			buffer << vote_type_cmd[i];
			if (i + 1 != VOTE_MAX)
			{
				buffer << ", ";
			}
		}
		SV_PlayerPrintf(PRINT_HIGH, player.id, "%s\n", buffer.str().c_str());
		return;
	}

	// Is the player ingame?
	if (!player.ingame())
	{
		SV_PlayerPrintf(PRINT_HIGH, player.id, "You can't callvote until you're in the game.\n");
		return;
	}

	// Is the server in intermission?
	if (gamestate == GS_INTERMISSION)
	{
		SV_PlayerPrintf(PRINT_HIGH, player.id, "You can't callvote while the server is in intermission.\n");
		return;
	}

	// Is another vote already in progress?
	if (vote != 0)
	{
		SV_PlayerPrintf(PRINT_HIGH, player.id, "Another vote is already in progress.\n");
		return;
	}

	// Select the proper vote
	switch (votecmd)
	{
	case VOTE_KICK:
		vote = new KickVote;
		break;
	case VOTE_FORCESPEC:
		vote = new ForcespecVote;
		break;
	case VOTE_FORCESTART:
		vote = new ForcestartVote;
		break;
	case VOTE_RANDCAPS:
		vote = new RandcapsVote;
		break;
	case VOTE_RANDPICKUP:
		vote = new RandpickupVote;
		break;
	case VOTE_MAP:
		vote = new MapVote;
		break;
	case VOTE_NEXTMAP:
		vote = new NextmapVote;
		break;
	case VOTE_RANDMAP:
		vote = new RandmapVote;
		break;
	case VOTE_RESTART:
		vote = new RestartVote;
		break;
	case VOTE_FRAGLIMIT:
		vote = new FraglimitVote;
		break;
	case VOTE_SCORELIMIT:
		vote = new ScorelimitVote;
		break;
	case VOTE_TIMELIMIT:
		vote = new TimelimitVote;
		break;
	case VOTE_COINFLIP:
		vote = new CoinflipVote;
		break;
	default:
		return;
	}

	bool valid;
	valid = vote->init(arguments, player);

	// If we encountered an unrecoverable error with the vote,
	// abort the vote and print a message to the player.
	if (!valid)
	{
		SV_PlayerPrintf(PRINT_HIGH, player.id, "%s\n", vote->get_error().c_str());
		delete vote;
		vote = 0;
		return;
	}

	// Broadcast the vote state to every player
	SVC_GlobalVoteUpdate();
}

// Handle vote commands from the client.
void SV_Vote(player_t &player)
{
	bool ballot = MSG_ReadBool();

	// Is there even a vote going on?
	if (vote == 0)
	{
		SV_PlayerPrintf(PRINT_HIGH, player.id, "Invalid vote, no vote in progress.\n");
		return;
	}

	// Did the player actually change his vote?
	if (vote->vote(player, ballot))
	{
		SV_BroadcastPrintf(PRINT_HIGH, "%s voted %s.\n", player.userinfo.netname.c_str(), ballot == true ? "Yes" : "No");
		SVC_GlobalVoteUpdate();
	}
}

//////// EVENTS ////////

// Remove a disconnected player from the tally.
void Vote_Disconnect(player_t &player)
{
	// Is there even a vote happening?  If not, we don't really care.
	if (vote == 0)
	{
		return;
	}

	vote->ev_disconnect(player);
}

// Handles tic-by-tic maintenance of voting.
void Vote_Runtic()
{
	// Special housekeeping for intermission or a new map.
	if (level.time == 1)
	{
		// Every player has a clean slate in terms of timeouts.
		for (Players::iterator it = players.begin();it != players.end();++it)
		{
			if (!validplayer(*it))
			{
				continue;
			}

			it->timeout_callvote = 0;
			it->timeout_vote = 0;
		}
	}

	// Is there even a vote happening?
	if (vote == 0)
	{
		return;
	}

	// Run tic-by-tic maintenence of the vote.
	if (!vote->ev_tic())
	{
		// Vote is done!  Parse it with the result taken from
		// the vote iteslf.
		vote->parse(vote->get_result());

		// If there is an error message, display it.
		if (!vote->get_error().empty())
		{
			SV_BroadcastPrintf(PRINT_HIGH, "%s\n", vote->get_error().c_str());
		}

		delete vote;
		vote = 0;
		return;
	}

	// Sync the countdown every few seconds.
	if (vote->get_countdown() % (TICRATE * 5) == 0 &&
	        vote->get_countdown() != ((unsigned int)sv_vote_timelimit.asInt() * TICRATE))
	{
		SVC_GlobalVoteUpdate();
	}
}
