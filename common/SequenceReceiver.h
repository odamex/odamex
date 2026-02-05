// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  Packet Sequencer code
//
//-----------------------------------------------------------------------------
#pragma once

#include <algorithm>
#include <vector>

#include "SequenceQueueEntryType.h"

class SequenceReceiver
{
	public:

		explicit SequenceReceiver(size_t i_initialSize) :
			m_recvQueue       (i_initialSize),
			m_currentSequence (0)
		{
		}

		SequenceReceiver() :
			SequenceReceiver(DEFAULT_RELIABILITY_QUEUE_SIZE)
		{
		}

		// This function records the receipt of a reliable message with the given
		// sequence number and data payload.  The message is accepted only if 1. it
		// does not pre-date the most-recently processed packet obtained via
		// NextPacket(), and 2. has not already been received.
		//
		// If the message is accepted, the data payload in the given buffer is moved
		// into the queue, leaving the given io_bufferRef in a valid but indeterminant
		// state, and true is returned.  Otherwise, false is returned and the given
		// buffer is left unmodified.
		bool RegisterReceivedPacket(int sequence, buf_t& io_bufferRef);

		// Returns the next packet in the sequence of received reliable messages.
		// The ordering of messages returned by repeated calls to this function is
		// dictated by the sequence numbers given to RegisterReceivePacket().  If a
		// "break" in the sequence is encountered, nullptr is returned.  If no
		// messages are pending, nullptr is returned.
		//
		// Messages obtained and processed in accordance with this function will be
		// in the correct sequence, even if they were provided to RegisterReceivePacket()
		// out-of-order.
		SequenceQueueEntryType* NextPacket();

	protected:

		std::vector<SequenceQueueEntryType> m_recvQueue;

		int m_currentSequence;  // Index of the place to store the next received packet.
};
