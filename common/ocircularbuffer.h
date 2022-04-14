// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//     This is a buffer with a fixed size that wraps around on itself.
//
//-----------------------------------------------------------------------------

#pragma once

#include "odamex.h"

template <typename TYPE, size_t SIZE>
class OCircularBuffer
{
	static const size_t m_mask = SIZE - 1;
	TYPE m_buffer[SIZE];

  public:
	OCircularBuffer()
	{
		// Check if SIZE is power of two.
		if (SIZE && !(SIZE & (SIZE - 1)))
			throw std::logic_error("circular buffer size is not power of 2");

		ArrayInit(m_buffer, TYPE());
	}

	OCircularBuffer(const OCircularBuffer& other)
	{
		for (size_t i = 0; i < SIZE; i++)
		{
			m_buffer[i] = other.m_buffer[i];
		}
	}

	TYPE& operator[](const ptrdiff_t idx)
	{
		// This works because our masks are power of two.
		return m_buffer[(size_t(0) + idx) & m_mask];
	}
};

/**
 * @brief A circular queue built on top of OCircularBuffer that conforms
 *        to enough of std::queue to satisfy the author.
 */
template <typename TYPE, size_t SIZE>
class OCircularQueue
{
	OCircularBuffer<TYPE, SIZE> m_queue;
	ptrdiff_t m_head;
	ptrdiff_t m_tail;

  public:
	OCircularQueue() : m_queue(), m_head(0), m_tail(0) { }
	TYPE& front() { return m_queue[m_head]; }
	const TYPE& front() const { return m_queue[m_head]; }
	TYPE& back() { return m_queue[m_tail - 1]; }
	const TYPE& back() const { return m_queue[m_tail - 1]; }
	bool empty() const { return m_head == m_tail; }
	size_t size() const { return size_t(m_head - m_tail); }

	TYPE& push()
	{
		TYPE& rvo = m_queue[m_tail];
		m_tail += 1;

		// Swallow up the head if the circular buffer can't hold it.
		if (size() > SIZE)
			m_head += 1;

		return rvo;
	}

	void pop()
	{
		if (!empty())
			m_head += 1;
	}

	TYPE& operator[](const size_t pos) { return m_queue[m_head + pos]; }
	const TYPE& operator[](const size_t pos) const { m_queue[m_head + pos]; }
};
