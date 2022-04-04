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
			throw std::exception("circular buffer size is not power of 2");

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
