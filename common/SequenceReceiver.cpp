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
	if (m_currentSequence < 0)
	{
		m_currentSequence = sequence;
	}

	if (sequence >= m_currentSequence)
	{
		auto result = m_recvQueue.Acquire(sequence);
		if (result.second)      // Was this NOT a repeated reception?
		{
			SequenceQueueEntryType& entryRef = result.first->second;
			entryRef.sequence = sequence;
			entryRef.buf.swap(io_bufferRef);
			return true;
		}
	}
	return false;
}

int SequenceReceiver::NextPacket(buf_t& io_bufferRef)
{
	// This is deliberately restrictive.  We do NOT want to process packets
	// "from the future."  We want to keep a strict sequence to try to be as
	// deterministic as possible.
	auto iter = m_recvQueue.find(m_currentSequence);
	if (iter != m_recvQueue.end())
	{
		io_bufferRef.swap(iter->second.buf);
		m_recvQueue.Release(iter);
		return m_currentSequence++;
	}
	return -1;
}
