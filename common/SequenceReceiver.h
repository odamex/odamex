#pragma once

#include <algorithm>
#include <vector>

#include "QueueEntryType.h"

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

        bool RegisterReceivedPacket(int sequence, buf_t& io_bufferRef)
        {
            const int desiredIndex = sequence % m_recvQueue.size();

            QueueEntryType& entryRef = m_recvQueue[desiredIndex];

            if (m_currentSequence < 0)
            {
                m_currentSequence = sequence;
            }

            if (sequence >= m_currentSequence)
            {
                if (entryRef.sequence != sequence)
                {
                    entryRef.sequence = sequence;
                    entryRef.buf.swap(io_bufferRef);
                    return true;
                }
            }
            return false;
        }

        QueueEntryType* NextPacket()
        {
            const int desiredIndex = m_currentSequence % m_recvQueue.size();

            QueueEntryType& entryRef = m_recvQueue[desiredIndex];

            // This is deliberately restrictive.  We do NOT want to process packets
            // "from the future."  We want to keep a strict sequence to try to be as
            // deterministic as possible.
            if (m_currentSequence == entryRef.sequence)
            {
                ++m_currentSequence;
                return &entryRef;
            }
            return nullptr;
        }

    protected:

        std::vector<QueueEntryType> m_recvQueue;

        int m_currentSequence;  // Index of the place to store the next received packet.
};
