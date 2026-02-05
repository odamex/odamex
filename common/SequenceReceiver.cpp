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

#include "SequenceReceiver.h"

bool SequenceReceiver::RegisterReceivedPacket(int sequence, buf_t& io_bufferRef)
{
	const int desiredIndex = sequence % m_recvQueue.size();

	SequenceQueueEntryType& entryRef = m_recvQueue[desiredIndex];

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

SequenceQueueEntryType* SequenceReceiver::NextPacket()
{
	const int desiredIndex = m_currentSequence % m_recvQueue.size();

	SequenceQueueEntryType& entryRef = m_recvQueue[desiredIndex];

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
