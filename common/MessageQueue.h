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
//  Queue of Messsage buffers with reuse pool
//
//-----------------------------------------------------------------------------
#pragma once

#include <deque>
#include <string>
#include <vector>

#include "i_net.h"

/// This class represents a simple message queue mechanism that maintains its own
/// stack of free message buffers for reuse.
class MessageQueue
{
	public:

		size_t SizeInBytes()    const { return not m_queue.empty() ? m_totalEnqueuedBytesSansMostRecent + m_queue.back().size() : m_totalEnqueuedBytesSansMostRecent; }
		size_t SizeInMessages() const { return m_queue.size(); }

		// Pushing messages
		buf_t& Obtain();
		void Emplace(buf_t& io_str);

		void Write(const google::protobuf::Message& msg);
		void Write(msg_t id, const std::string& msg);

		// Using messages.
		const buf_t& Front() const { return m_queue.front(); }

		// Popping messages.
		bool Pop();
		void Clear();

		// Call a given packer method for every message in the queue, popping messages
		// as we go.
		//
		// The packer method must return something that evaluates in a boolean context.
		// If it returns / evaluates true, then the given message is popped and the process
		// repeats until the whole queue is packed and popped.  If it returns / evaluates
		// as false, then processing stops, the given message is not popped, this Pack
		// function returns, and another call to Pack will resume from that message.
		template <typename PackerCallable>
		void Pack(PackerCallable&& packerMethod)
		{
			while (not m_queue.empty())
			{
				if (packerMethod(m_queue.front()))
				{
					PopFromQueueToFreeStack();
				}
				else
				{
					break;
				}
			}
		}

		std::string& GetSerializationBufferRef() { return m_serializationBuffer; }
	protected:

		void PopFromQueueToFreeStack();

		std::deque<buf_t>  m_queue;
		std::vector<buf_t> m_freeStack;
		std::string        m_serializationBuffer;

		// Tracking total byte size is slightly tricky because the buffer's slot in the queue
		// is created before the buffer is filled.  We don't consider a buffer's size to be
		// "finalized" until the next slot is created.
		//
		size_t m_totalEnqueuedBytesSansMostRecent { 0 };
};
