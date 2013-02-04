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
//	Voting-specific stuff.
//
//-----------------------------------------------------------------------------

#ifndef __C_VOTE__
#define __C_VOTE__

/**
 * An enum used for keeping track of the state of the vote as well as
 * internally keeping track of individual player votes.
 */
typedef enum {
	VOTE_UNDEC,
	VOTE_NO,
	VOTE_YES,
	VOTE_INTERRUPT,
	VOTE_ABANDON
} vote_result_t;

/**
 * An enum used for sending the vote type over the wire.
 */
typedef enum {
	VOTE_NONE, // Reserved
	VOTE_KICK,
	VOTE_FORCESPEC,
	VOTE_FORCESTART,
	VOTE_RANDCAPS,
	VOTE_RANDPICKUP,
	VOTE_MAP,
	VOTE_NEXTMAP,
	VOTE_RANDMAP,
	VOTE_RESTART,
	VOTE_FRAGLIMIT,
	VOTE_SCORELIMIT,
	VOTE_TIMELIMIT,
	VOTE_COINFLIP,
	VOTE_MAX // Reserved
} vote_type_t;

extern const char* vote_type_cmd[];

#endif
