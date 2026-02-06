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

#include "SequenceSender.h"

#include "doomfunc.h"

SequenceQueueEntryType* SequenceSender::UnackedIterator::Next()
{
    if (m_iter == m_sequencer->m_unackedSequences.end())
    {
        return nullptr;
    }

    const int sequence = *(m_iter++);

    const auto tableIter = m_sequencer->m_sendTable.find(sequence);
    if (tableIter != m_sequencer->m_sendTable.end())
    {
        // check for isAwaiting?  Do we even need that now?
        return & tableIter->second;
    }
    return nullptr;
}

SequenceSender::SequenceSender(size_t i_initialSize) :
	m_sendTable      (i_initialSize),
	m_nextSequence   (0),
	m_mode           (NORMAL)
{
    m_sendTable.max_load_factor(3.0f);   // why not...?
}

SequenceSender::QueueEntryResultType SequenceSender::ObtainSendPacket(int currentTic)
{
        decltype(m_sendTable)::iterator iter;
        if (not m_freePackets.empty())
        {
            auto insertResult = m_sendTable.insert( { m_nextSequence, std::move(m_freePackets.back()) } );
            m_freePackets.pop_back();

            iter = insertResult.first;
        }
        else
        {
            auto insertResult = m_sendTable.emplace(m_nextSequence, MAX_UDP_PACKET);
            iter = insertResult.first;
        }

        m_unackedSequences.push_back(m_nextSequence);
        ++m_nextSequence;

        iter->second.isAwaiting        = true;
        iter->second.sequence          = iter->first;
        iter->second.originatingTic    = currentTic;
        iter->second.lastRetransmitTic = -1;
        iter->second.buf.clear();

        return QueueEntryResultType {& iter->second.buf, iter->second.sequence};
}

bool SequenceSender::Acknowledge(int sequence)
{
	// Older clients send acks for unreliable messages as well.
	// Just ignore those.
	if (sequence >= 0)
	{
        auto iter = m_sendTable.find(sequence);

        if (iter != m_sendTable.end())
        {
            auto unackIter = std::find(m_unackedSequences.begin(),
                                       m_unackedSequences.end(),
                                       sequence);
            if (unackIter != m_unackedSequences.end())
            {
                m_unackedSequences.erase(unackIter);
            }

            m_freePackets.push_back(std::move(iter->second));
            m_sendTable.erase(iter);
            return true;
        }
    }
    return false;
}
