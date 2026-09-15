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

#include "MessageQueue.h"

#include "msg_pack.h"

buf_t& MessageQueue::Obtain()
{
	if (m_queue.size() > 0)
	{
		m_totalEnqueuedBytesSansMostRecent += m_queue.back().size();
	}
	else
	{
		// Shouldn't need to do this, but let's do it anyway just to be on the safe side.
		m_totalEnqueuedBytesSansMostRecent = 0;
	}

	if (not m_freeStack.empty())
	{
		m_queue.emplace_back(std::move(m_freeStack.back()));
		m_freeStack.pop_back();
		m_queue.back().clear();
	}
	else
	{
		m_queue.emplace_back(MAX_UDP_PACKET);
	}
	return m_queue.back();
}

void MessageQueue::Write(msg_t id, const std::string& msg)
{
	if (simulated_connection)
		return;

	buf_t& buffer = Obtain();
	buffer.WriteMessage(id, msg);
}

void MessageQueue::Write(const google::protobuf::Message& msg)
{
	if (simulated_connection)
		return;

	if (const auto messageId = MSG_Pack(m_serializationBuffer, msg))
	{
		Write(*messageId, m_serializationBuffer);
	}
}

void MessageQueue::PopFromQueueToFreeStack()
{
	// The total byte size does not include the back() element, and we're popping from
	// the front, so we can just count on the queue's number of elements for knowing
	// whether the element is accounted for in the byte size.

	if (m_queue.size() > 1)
	{
		m_totalEnqueuedBytesSansMostRecent -= m_queue.front().size();
	}
	m_freeStack.emplace_back(std::move(m_queue.front()));
	m_queue.pop_front();
}

bool MessageQueue::Pop()
{
	if (not m_queue.empty())
	{
		PopFromQueueToFreeStack();
		return true;
	}
	return false;
}

void MessageQueue::Clear()
{
	while (not m_queue.empty())
	{
		PopFromQueueToFreeStack();
	}

	// Shouldn't need to do this, but let's do it anyway just to be on the safe side.
	m_totalEnqueuedBytesSansMostRecent = 0;
}
