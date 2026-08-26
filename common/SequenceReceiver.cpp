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
SequenceReceiver::ReceiveTableType::iterator SequenceReceiver::ObtainReceivePacket(int sequence)
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

bool SequenceReceiver::RegisterReliablePacket(const PacketHeaderType& i_header, size_t i_size, buf_t& io_bufferRef)
{
	auto iter = ObtainReceivePacket(i_header.sequence);
	if (iter != m_receiveTable.end())
	{
		auto& entryRef = iter->second;

		// Only one reliable packet allowed per sequence number.
		if (entryRef.reliable.header.sequence < 0)
		{
			entryRef.reliable.header = i_header;
			entryRef.reliable.buf.WriteChunk(io_bufferRef.ReadChunk(i_size), i_size);
			return true;
		}
	}
	return false;
}

bool SequenceReceiver::RegisterBestEffortPacket(const PacketHeaderType& i_header, size_t i_size, buf_t& io_bufferRef)
{
	auto iter = ObtainReceivePacket(i_header.sequence);
	if (iter != m_receiveTable.end())
	{
		// Best-effort is never retransmitted.  There's no need to check for duplication.
		SequenceQueueEntryType& entryRef = iter->second.bestEffort.emplace_back();
		entryRef.header = i_header;
		entryRef.buf.WriteChunk(io_bufferRef.ReadChunk(i_size), i_size);
		return true;
	}
	return false;
}

std::optional<PacketHeaderType> SequenceReceiver::NextPacket(buf_t& io_bufferRef)
{
	std::optional<PacketHeaderType> fetchedHeader;

	// This is deliberately restrictive.  We do NOT want to process packets
	// "from the future."  We want to keep a strict sequence to try to be as
	// deterministic as possible.
	auto iter = m_receiveTable.find(m_currentSequence);
	if (iter != m_receiveTable.end() and iter->second.reliable.header.sequence >= 0)
	{
		// isAwaiting is co-opted to denote that the reliable packet has been processed.
		if (not iter->second.reliable.isAwaiting)
		{
			iter->second.reliable.isAwaiting = true;
			io_bufferRef.swap(iter->second.reliable.buf);
			fetchedHeader = iter->second.reliable.header;
		}
		else
		{
			if (not iter->second.bestEffort.empty())
			{
				{
					auto& entry = iter->second.bestEffort.front();
					fetchedHeader = entry.header;
					io_bufferRef.swap(entry.buf);
				}
				iter->second.bestEffort.pop_front();
			}
		}

		if (iter->second.bestEffort.empty())
		{
			m_receiveTable.erase(iter);
			m_currentSequence++;
		}
	}

	// TODO: NonReliable, monotonic sequence with skips
	return fetchedHeader;
}
