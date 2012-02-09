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

#include "c_vote.h"
#include "c_dispatch.h"
#include "cl_main.h"
#include "i_net.h"

/**
 * Send a vote request to the server.
 */
BEGIN_COMMAND(callvote) {
	// Dumb question, but are we even connected to a server?
	if (!connected) {
		Printf(PRINT_HIGH, "Invalid callvote, you are not connected to a server.\n");
		return;
	}

	// We don't want or need more than 255 params.
	if (argc > 255) {
		Printf(PRINT_HIGH, "Invalid callvote, maximum argument count is 255.\n");
		return;
	}

	// Sending a VOTE_NONE will query the server for a
	// list of types of votes that are allowed.
	vote_type_t votecmd = VOTE_NONE;

	// If we don't want just a list of valid votes, we
	// need to set the votecmd to something valid here.
	if (argc > 1) {
		std::string votecmd_s = argv[1];

		// Match up the string with the command name.
		for (unsigned char i = (VOTE_NONE + 1);i < VOTE_MAX;++i) {
			if (votecmd_s.compare(vote_type_cmd[i]) == 0) {
				votecmd = (vote_type_t)i;
			}
		}
	} else {
		// Make sure we don't accidentally send an argc of 255.
		argc = 2;
	}

	MSG_WriteMarker(&net_buffer, clc_callvote);
	MSG_WriteByte(&net_buffer, (byte)votecmd);
	MSG_WriteByte(&net_buffer, (byte)argc - 2);
	for (unsigned int i = 2;i < argc;i++) {
		MSG_WriteString(&net_buffer, argv[i]);
	}
} END_COMMAND(callvote)

/**
 * Sends a "yes" vote to the server.
 */
BEGIN_COMMAND(vote_yes) {
	if (!connected) {
		Printf(PRINT_HIGH, "Invalid vote, you are not connected to a server.\n");
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
		Printf(PRINT_HIGH, "Invalid vote, you are not connected to a server.\n");
		return;
	}

	MSG_WriteMarker(&net_buffer, clc_vote);
	MSG_WriteBool(&net_buffer, false);
} END_COMMAND(vote_no)
