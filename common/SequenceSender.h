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

#include <iostream>
#include <iso646.h>
#include <vector>

#include "SequenceQueueEntryType.h"

class SequenceSender
{
	public:     // Types.

		// This class iterates over the *unacknowledged* reliable messages.
		// This iterator can be invalidated if things are acked or new Send Packets are obtained.
		class UnackedIterator
		{
			public:
				explicit UnackedIterator(SequenceSender* i_sequencer):
					m_sequencer(i_sequencer),
					m_count    (0),
					m_index    (-1)
				{}

				// Returns the next unacknowledged message.  After the last unacknowledged message
				// has been returned, subsequent calls return nullptr.
				SequenceQueueEntryType* Next();

			protected:
				SequenceSender* m_sequencer;   // non-owning pointer.
				int          m_index;
				int          m_count;
		};

		enum SenderModeEnum
		{
			NORMAL,
			RECOVERY
		};

		struct QueueEntryResultType
		{
			buf_t* buffer   {nullptr};
			int    sequence {-1};
		};

		explicit SequenceSender(size_t i_initialSize);

	public:  // Functions.

		SequenceSender() :
			SequenceSender(DEFAULT_RELIABILITY_QUEUE_SIZE)
		{
		}

        void SetMode(SenderModeEnum i_mode) { m_mode = i_mode; }
		SenderModeEnum GetMode() const { return m_mode; }

		// Grab a slot in the reliability sequence and prepare it for transmission.
		// This class does not manage the timestamps themselves -
		// the caller must manage these and specify them when obtaining a reliability
		// slot.
		//
		QueueEntryResultType ObtainSendPacket(int currentTic=-1);

		// This function declares that the packet associated with the given sequence number has been
		// acknowledged by its intended recipient.
		//
		// Returns true if the acknowledgement a previously unacknowledged message has become acknowledged.
		// Returns false otherwise.
		bool Acknowledge(int sequence);

		UnackedIterator IterateUnackedPackets() { return UnackedIterator(this); }

		int GetPendingAckCount() const { return m_unackedCount; }

	protected:

		void AdvanceSmallestUnacked();

		std::vector<SequenceQueueEntryType> m_sendQueue;

		int m_nextSequence;                 // The sequence number to assign to the next requested packet.
		int m_unackedCount;                 // The number of sent packets that have not yet been acked.
		int m_smallestUnacked;              // The smallest sequence number that has yet to be acked.

		SenderModeEnum m_mode;
};
