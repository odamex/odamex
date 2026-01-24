#pragma once

#include <iostream>
#include <iso646.h>
#include <vector>

#include "QueueEntryType.h"

class SequenceSender
{
    public:

    // This iterator can be invalidated if things are acked or new Send Packets are obtained.
    class UnackedIterator
    {
        public:
            explicit UnackedIterator(SequenceSender* i_sequencer):
                m_sequencer(i_sequencer),
                m_count    (0),
                m_index    (-1)
            {}

            QueueEntryType* Next()
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

                    QueueEntryType* candidate = &m_sequencer->m_sendQueue[m_index++];
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

        // Sender functions
        buf_t& ObtainSendPacket(int sequence)
        {
            const int desiredIndex = sequence % m_sendQueue.size();

            QueueEntryType& entryRef = m_sendQueue[desiredIndex];

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

            entryRef.isAwaiting  = true;
            entryRef.sequence       = sequence;
            entryRef.buf.clear();

            return entryRef.buf;
        }

        // Returns true if the acknowledgement a previously unacknowledged message has become acknowledged.
        // Returns false otherwise.
        bool Acknowledge(int sequence)
        {
            const int desiredIndex = sequence % m_sendQueue.size();
            bool      isFreshAck   = false;

            QueueEntryType& entryRef = m_sendQueue[desiredIndex];

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
            return isFreshAck;
        }

        UnackedIterator IterateUnackedPackets()
        {
            return UnackedIterator(this);
        }

        // Receiver functions

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

        std::vector<QueueEntryType> m_sendQueue;

        int m_unackedCount;     // The number of sent packets that have not yet been acked.
        int m_smallestUnacked;  // The smallest sequence number that has yet to be acked.
};
