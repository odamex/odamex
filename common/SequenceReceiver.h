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
#pragma once

#include <algorithm>
#include <deque>
#include <functional>
#include <memory_resource>
#include <optional>
#include <unordered_map>
#include <vector>

#include "SequenceQueueEntryType.h"

class SequenceReceiver
{
	public:

		explicit SequenceReceiver(size_t i_initialSize,
		                          const std::pmr::polymorphic_allocator<SequenceQueueEntryType>& i_allocator = {}) :
			m_receiveTable    { i_initialSize, i_allocator },
			m_currentSequence { 0 }
		{
		}

		// This function records the receipt of a reliable message with the given
		// header (including the sequence number) and the data payload.  The message
		// is accepted only if 1. it does not pre-date the most-recently processed
		// packet obtained via NextPacket(), and 2. has not already been received.
		//
		// If the message is accepted, the data payload in the given buffer is Read
		// into the table, and true is returned.  Otherwise, false is returned and the
		// given buffer is left unread.
		bool RegisterReliablePacket(const PacketHeaderType& i_header, size_t i_size, buf_t& io_bufferRef);

		bool RegisterBestEffortPacket(const PacketHeaderType& i_header, size_t i_size, buf_t& io_bufferRef);

		// Fetches the next packet in the sequence of received reliable messages.
		// The ordering of messages returned by repeated calls to this function is
		// dictated by the sequence numbers given to RegisterReceivePacket().  The
		// header of the fetched packet is returned.  If a "break" in the sequence
		// is encountered, nullopt is returned.  If no messages are pending, nullopt
		// is returned.
		//
		// Messages obtained and processed in accordance with this function will be
		// in the correct sequence, even if they were provided to RegisterReceivePacket()
		// out-of-order.
		std::optional<PacketHeaderType> NextPacket(buf_t& io_bufferRef);

	protected:

		struct PacketQueue
		{
			SequenceQueueEntryType                  reliable;     // Only one reliable message per sequence number.
			std::pmr::deque<SequenceQueueEntryType> bestEffort;

			explicit PacketQueue(const std::pmr::polymorphic_allocator<SequenceQueueEntryType>& i_allocator) :
				bestEffort { i_allocator }
			{
			}
		};

		using ReceiveTableType = std::pmr::unordered_map<int, PacketQueue, std::identity>;

		ReceiveTableType::iterator ObtainReceivePacket(int sequence);

		ReceiveTableType m_receiveTable;

		int m_currentSequence;  // Index of the place to store the next received packet.
};
