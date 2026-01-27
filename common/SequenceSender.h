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
	public:

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
			SequenceQueueEntryType* Next()
			{
				if (m_count >= m_sequencer->m_unackedCount)
				{
					return nullptr;
				}

				if (m_index < 0)
				{
					m_index = m_sequencer->m_smallestUnacked % m_sequencer->m_sendQueue.size();
				}

				for (size_t i = 0; i < m_sequencer->m_sendQueue.size(); ++i)
				{
					if (m_index == static_cast<int>(m_sequencer->m_sendQueue.size()))
					{
						m_index = 0;
					}

					SequenceQueueEntryType* candidate = &m_sequencer->m_sendQueue[m_index++];
					if (candidate->isAwaiting)
					{
						++m_count;
						return candidate;
					}
				}
				return nullptr;
			}

		protected:
			SequenceSender* m_sequencer;   // non-owning pointer.
			int          m_index;
			int          m_count;
	};

	public:
		explicit SequenceSender(size_t i_initialSize) :
			m_sendQueue      (i_initialSize),
			m_unackedCount   (0),
			m_smallestUnacked(0)
		{
		}

		SequenceSender() :
			SequenceSender(DEFAULT_RELIABILITY_QUEUE_SIZE)
		{
		}

		// Grab a slot in the reliability sequence and prepare it for transmission.
		// This class does not manage the sequence numbers or timestamps themselves -
		// the caller must manage these and specify them when obtaining a reliability
		// slot.  If a sequence number is requested that corresponds to an existing
		// message, that existing message is discarded, regardless of whether or not
		// it is awaiting acknowledgement.
		//
		// The overall algorithm assumes that sequence numbers will only ever be
		// specified in *contiguous* ascending order.  If a sequence number is
		// specified with a value less than 0 or the sequence of any previously-
		// obtained slot, the behavior is undefined.
		//
		// Returns a reference to the buffer that the caller must fill out with data
		// and send to the intended recipient.
		buf_t& ObtainSendPacket(int sequence, int currentTic=-1)
		{
			const int desiredIndex = sequence % m_sendQueue.size();

			SequenceQueueEntryType& entryRef = m_sendQueue[desiredIndex];

			if (entryRef.sequence > -1 and entryRef.isAwaiting)
			{
				DPrintFmt("Done goofed!  Wrapped around, dropping ancient message: {} cur: {}\n", entryRef.sequence, sequence);

				// We don't increment the Unacked Count in this case, because we're replacing
				// an old unacked message with a new unacked message.

				entryRef.isAwaiting = false; // Clear so that we probe forward for the next unacked.
				AdvanceSmallestUnacked();
			}
			else
			{
				if (m_unackedCount == 0)
				{
					m_smallestUnacked = sequence;
				}
				++m_unackedCount;
			}

			entryRef.isAwaiting     = true;
			entryRef.sequence       = sequence;
			entryRef.originatingTic = currentTic;
			entryRef.buf.clear();

			return entryRef.buf;
		}

		// This function declares that the packet associated with the given sequence number has been
		// acknowledged by its intended recipient.
		//
		// Returns true if the acknowledgement a previously unacknowledged message has become acknowledged.
		// Returns false otherwise.
		bool Acknowledge(int sequence)
		{
			bool isFreshAck = false;

			// Older clients send acks for unreliable messages as well.
			// Just ignore those.
			if (sequence >= 0)
			{
				const int desiredIndex = sequence % m_sendQueue.size();
				SequenceQueueEntryType& entryRef = m_sendQueue[desiredIndex];

				if (sequence < entryRef.sequence)
				{
					DPrintFmt("Wow, that's an old acknowledgement!  seq: {} cur: {}\n", sequence, entryRef.sequence);
				}
				else if (sequence > entryRef.sequence)
				{
					DPrintFmt("Can't get fooled again!  (future?!?!) seq: {} cur: {}\n", sequence, entryRef.sequence);
				}
				else
				{
					if (not entryRef.isAwaiting)
					{
						//DPrintFmt("Stale ack: {}\n", sequence);
					}
					else
					{
						isFreshAck = true;
						entryRef.isAwaiting = false;
						--m_unackedCount;
					}
				}

				AdvanceSmallestUnacked();
			}
			return isFreshAck;
		}

		UnackedIterator IterateUnackedPackets()
		{
			return UnackedIterator(this);
		}

		int GetPendingAckCount() const
		{
			return m_unackedCount;
		}
	protected:

		void AdvanceSmallestUnacked()
		{
			if (m_unackedCount)
			{
				for (size_t i = 0; i < m_sendQueue.size(); ++i)
				{
					const auto& checkRef = m_sendQueue[m_smallestUnacked % m_sendQueue.size()];

					if (checkRef.isAwaiting and m_smallestUnacked == checkRef.sequence)
					{
						break;
					}
					++m_smallestUnacked;
				}
			}
		}

		std::vector<SequenceQueueEntryType> m_sendQueue;

		int m_unackedCount;     // The number of sent packets that have not yet been acked.
		int m_smallestUnacked;  // The smallest sequence number that has yet to be acked.
};
