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

#include <iso646.h>

//#include "doomfunc.h"

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
}

SequenceSender::QueueEntryResultType SequenceSender::ObtainSendPacket(int currentTic)
{
    auto result = m_sendTable.Emplace(m_nextSequence);
    auto iter   = result.first;

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

        if (m_sendTable.Erase(iter))
        {
            auto unackIter = std::find(m_unackedSequences.begin(),
                                       m_unackedSequences.end(),
                                       sequence);
            if (unackIter != m_unackedSequences.end())
            {
                m_unackedSequences.erase(unackIter);
            }

            return true;
        }
    }
    return false;
}
