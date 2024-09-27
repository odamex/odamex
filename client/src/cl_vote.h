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
//	Clientside voting-specific stuff.
//
//-----------------------------------------------------------------------------

#pragma once

#include "c_vote.h"

// A class to actually deal with keeping track of voting state and ensuring
// that the proper variables are set.
class VoteState {
private:
	bool visible;
	vote_result_t result;
	std::string votestring;
	short countdown;
	QWORD countdown_ms;
	byte yes;
	byte yes_needed;
	byte no;
	byte no_needed;
	byte abs;
public:
	VoteState() : visible(false) { };
	static VoteState& instance(void);
	void set(const vote_state_t &vote_state);
	bool get(vote_state_t &vote_state);
};
