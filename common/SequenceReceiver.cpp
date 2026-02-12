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

bool SequenceReceiver::RegisterReliablePacket(int sequence, size_t i_size, buf_t& io_bufferRef)
{
	if (sequence >= m_currentSequence)
	{
		auto result = m_reliableTable.Emplace(sequence);
		if (result.second)      // Was this NOT a repeated reception?
		{
			SequenceQueueEntryType& entryRef = result.first->second;
			entryRef.sequence = sequence;
			entryRef.buf.WriteChunk(io_bufferRef.ReadChunk(i_size), i_size);
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
	auto iter = m_reliableTable.find(m_currentSequence);
	if (iter != m_reliableTable.end())
	{
		io_bufferRef.swap(iter->second.buf);
		m_reliableTable.Erase(iter);
		return m_currentSequence++;
	}

	// TODO: NonReliable, monotonic sequence with skips
	return -1;
}
