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

		enum SenderModeEnum
		{
			NORMAL,
			RECOVERY
		};

		explicit SequenceSender(size_t i_initialSize) :
			m_sendQueue      (i_initialSize),
			m_nextSequence   (0),
			m_unackedCount   (0),
			m_smallestUnacked(0),
            m_mode           (NORMAL)
		{
		}

		SequenceSender() :
			SequenceSender(DEFAULT_RELIABILITY_QUEUE_SIZE)
		{
		}

		SenderModeEnum GetMode() const { return m_mode; }

		struct QueueEntryResultType
		{
			buf_t* buffer   {nullptr};
			int    sequence {-1};
		};

		// Grab a slot in the reliability sequence and prepare it for transmission.
		// This class does not manage the timestamps themselves -
		// the caller must manage these and specify them when obtaining a reliability
		// slot.
		//
		QueueEntryResultType ObtainSendPacket(int currentTic=-1)
		{

			// Recovery mode is exceedingly simple for now.  We just don't send anything
			// until the send buffer comes back down to size.
			if (m_mode == NORMAL)
			{
				const int desiredSequence = m_nextSequence;
				const int desiredIndex    = desiredSequence % m_sendQueue.size();

				SequenceQueueEntryType& entryRef = m_sendQueue[desiredIndex];

				if (entryRef.sequence > -1 and entryRef.isAwaiting)
				{
					m_mode = RECOVERY;
					DPrintFmt("I'm exhausted!  dropping packet {} while awaiting ack for {}\n", desiredSequence, entryRef.sequence);

					// We take no further action here.  We are considering this new packet DROPPED.
					// We have to start recovering.
				}
				else
				{
					if (m_unackedCount == 0)
					{
						m_smallestUnacked = desiredSequence;
					}
					++m_unackedCount;

					m_nextSequence = desiredSequence + 1;

					entryRef.isAwaiting     = true;
					entryRef.sequence       = desiredSequence;
					entryRef.originatingTic = currentTic;
					entryRef.buf.clear();

					return QueueEntryResultType {& entryRef.buf, entryRef.sequence};
				}
			}
            return QueueEntryResultType();
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
					// In this particular case, we're so far behind that we're about to run out of send queue
					// space.
					//
					// This should never happen because if we have to drop reliable packets, we drop new ones
					// not old ones.
					DPrintFmt("Wow, that's an old acknowledgement!  Should never happen!  seq: {} cur: {}\n", sequence, entryRef.sequence);
				}
				else if (sequence > entryRef.sequence)
				{
					// Not sure how this would ever happen, but we need to be sure and log something if it does.
					DPrintFmt("Received a packet from the future?!?! seq: {} cur: {}\n", sequence, entryRef.sequence);
				}
				else
				{
					if (not entryRef.isAwaiting)
					{
						// This happens because we're seeing multiple acks for the same message.  This happens when
						// the client is far behind enough that it's acked a message, but the sender has already
						// sent retries.  It could in fact happen multiple times, depending on how far back we're
						// talking.  Could be due to congestion or just a very long latency.
						//
						// TODO:  Consider using this as a quality metric.  Are we spamming retries too much?
						//DPrintFmt("Stale ack: {}\n", sequence);
					}
					else
					{
						isFreshAck = true;
						entryRef.isAwaiting = false;
						--m_unackedCount;
						m_mode = NORMAL;
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

		int m_nextSequence;     // The sequence number to assign to the next requested packet.
		int m_unackedCount;     // The number of sent packets that have not yet been acked.
		int m_smallestUnacked;  // The smallest sequence number that has yet to be acked.

		SenderModeEnum m_mode;
};
