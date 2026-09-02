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

#include <functional>
#include <memory_resource>
#include <unordered_map>
#include <vector>

#include "SequenceQueueEntryType.h"

class SequenceSender
{
	public:     // Types.

		// This class iterates over the *unacknowledged* reliable messages.
		// This iterator can be invalidated if things are acked or new Send Packets are obtained.
		class UnackedIterator
		{
			public:
				explicit UnackedIterator(SequenceSender* i_sequencer):
					m_sequencer(i_sequencer),
					m_iter     (i_sequencer->m_unackedSequences.begin())
				{}

				// Returns the next unacknowledged message.  After the last unacknowledged message
				// has been returned, subsequent calls return nullptr.
				SequenceQueueEntryType* Next();

			protected:
				SequenceSender*            m_sequencer;   // non-owning pointer.
				std::vector<int>::iterator m_iter;
		};

		enum SenderModeEnum
		{
			NORMAL,
			RECOVERY,
			CRITICAL_FAILURE,
		};

		struct ObtainResultType
		{
			buf_t&            bufferRef;
			PacketHeaderType& headerRef;
		};

	public:  // Functions.

		explicit SequenceSender(size_t i_initialSize, const std::pmr::polymorphic_allocator<SequenceQueueEntryType> & i_allocator = {}) :
			m_sendTable { i_initialSize, i_allocator }
		{
		}

		void SetMode(SenderModeEnum i_mode) { m_mode = i_mode; }
		SenderModeEnum GetMode() const { return m_mode; }

		// Grab a slot in the reliability sequence and prepare it for transmission.
		// This class does not manage the timestamps themselves -
		// the caller must manage these and specify them when obtaining a reliability
		// slot.
		//
		ObtainResultType ObtainSendPacket();

		// This function declares that the packet associated with the given sequence number has been
		// acknowledged by its intended recipient.
		//
		// Returns true if the acknowledgement a previously unacknowledged message has become acknowledged.
		// Returns false otherwise.
		bool Acknowledge(int sequence);

		UnackedIterator IterateUnackedPackets() { return UnackedIterator(this); }

		int GetPendingAckCount() const { return static_cast<int>(m_sendTable.size()); }

		int MostRecentAcquiredSequence() const { return m_nextSequence - 1; }

	protected:

		std::vector<int> m_unackedSequences;

		std::pmr::unordered_map<int, SequenceQueueEntryType, std::identity> m_sendTable;

		int             m_nextSequence  { 0 };                 // The sequence number to assign to the next requested packet.
		SenderModeEnum  m_mode          { NORMAL };
};
