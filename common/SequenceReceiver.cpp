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

#include "SequenceReceiver.h"
std::pmr::unordered_map<int, SequenceReceiver::PacketQueue, std::identity>::iterator SequenceReceiver::ObtainReceivePacket(int sequence)
{
	if (sequence >= m_currentSequence)
	{
        auto iter = m_receiveTable.find(sequence);
        if (iter == m_receiveTable.end())
        {
            auto emplaceResult = m_receiveTable.emplace( sequence,
                                                         PacketQueue { m_receiveTable.get_allocator() } );
            iter = emplaceResult.first;
        }
        return iter;
    }
    return m_receiveTable.end();
}

bool SequenceReceiver::RegisterReliablePacket(int sequence, size_t i_size, buf_t& io_bufferRef)
{
    auto iter = ObtainReceivePacket(sequence);
    if (iter != m_receiveTable.end())
    {
        auto& entryRef = iter->second;

        // Only one reliable packet allowed per sequence number.
        if (entryRef.reliable.sequence < 0)
        {
            entryRef.reliable.sequence = sequence;
            entryRef.reliable.buf.WriteChunk(io_bufferRef.ReadChunk(i_size), i_size);
            return true;
        }
    }
    return false;
}

bool SequenceReceiver::RegisterBestEffortPacket(int sequence, size_t i_size, buf_t& io_bufferRef)
{
    auto iter = ObtainReceivePacket(sequence);
    if (iter != m_receiveTable.end())
    {
        // Best-effort is never retransmitted.  There's no need to check for duplication.
        SequenceQueueEntryType& entryRef = iter->second.bestEffort.emplace_back();
        entryRef.sequence = sequence;
        entryRef.buf.WriteChunk(io_bufferRef.ReadChunk(i_size), i_size);
        return true;
	}
	return false;
}

int SequenceReceiver::NextPacket(buf_t& io_bufferRef)
{
    int fetchedPacketSequenceNumber = -1;
	// This is deliberately restrictive.  We do NOT want to process packets
	// "from the future."  We want to keep a strict sequence to try to be as
	// deterministic as possible.
	auto iter = m_receiveTable.find(m_currentSequence);
	if (iter != m_receiveTable.end() and iter->second.reliable.sequence >= 0)
	{
        // originatingTic is used to denote that the reliable packet has been processed.
        if (iter->second.reliable.originatingTic < 0)
        {
            iter->second.reliable.originatingTic = 0;
            io_bufferRef.swap(iter->second.reliable.buf);

            fetchedPacketSequenceNumber = m_currentSequence;
        }
        else
        {
            if (iter->second.bestEffort.size() > 0)
            {
                auto& entry = iter->second.bestEffort.front();
                io_bufferRef.swap(entry.buf);
                iter->second.bestEffort.pop_front();
                fetchedPacketSequenceNumber = m_currentSequence;
            }
        }

        if (iter->second.bestEffort.empty())
        {
            m_receiveTable.erase(iter);
            m_currentSequence++;
        }
        return fetchedPacketSequenceNumber;
	}

	// TODO: NonReliable, monotonic sequence with skips
	return -1;
}
