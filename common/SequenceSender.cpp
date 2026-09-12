// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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

SequenceSender::ObtainResultType SequenceSender::ObtainSendPacket()
{
	SequenceQueueEntryType& newEntryRef = m_sendTable[m_nextSequence];

	newEntryRef.isAwaiting           = true;
	newEntryRef.header.sequence      = m_nextSequence;
	newEntryRef.lastRetransmitTic    = -1;

	m_unackedSequences.push_back(m_nextSequence);
	++m_nextSequence;

	return ObtainResultType {.bufferRef = newEntryRef.buf,
	                         .headerRef = newEntryRef.header};
}

bool SequenceSender::Acknowledge(int sequence)
{
	// Older clients send acks for unreliable messages as well.
	// Just ignore those.
	if (sequence >= 0)
	{
		if (m_sendTable.erase(sequence))
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
