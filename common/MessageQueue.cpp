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
//  Queue of Messsage buffers with reuse pool
//
//-----------------------------------------------------------------------------

#include "MessageQueue.h"

buf_t& MessageQueue::Obtain()
{
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

void MessageQueue::Emplace(buf_t& io_str)
{
	buf_t& obtainedBuffer = MessageQueue::Obtain();

	obtainedBuffer.swap(io_str);
	io_str.clear();
}

void MessageQueue::PopFromQueueToFreeStack()
{
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
}
