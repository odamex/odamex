// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
//	Memory pool allocation
//	Allocates a large pool of memory and allocates blocks of it when asked.
//	Memory can only be freed by the clear() function for simplicity. If
//	the intial memory pool is exhausted, additional pools are allocated.
//
//
//-----------------------------------------------------------------------------

#pragma once

#include <memory_resource>

template <typename T>
class Pool
{
public:
	explicit Pool(size_t initial_max_count) :
		pool(initial_max_count * sizeof(T)) {}

	Pool(const Pool&) = delete;
	Pool& operator=(const Pool&) = delete;
	Pool(Pool&&) = default;
	Pool& operator=(Pool&&) = default;

	void clear()
	{
		pool.release();
	}

	[[nodiscard]]
	T* alloc(size_t count = 1)
	{
		return static_cast<T*>(pool.allocate(sizeof(T) * count, alignof(T)));
	}

private:
	std::pmr::monotonic_buffer_resource pool;
};
