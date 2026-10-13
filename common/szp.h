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
//
//  denis - szp<T>, the self zeroing pointer
//
//  Once upon a time, actors held raw pointers to other actors.
//
//  To destroy an object, one cycled though all the others searching for its
//  pointer and resetting every copy to NULL. Then one did the cycling for
//  the players, then the sector sound origins, and so on; with hack upon
//  hack. Ironically, zero dereferencing is what often crashed the
//  program altogether.
//
//  The idea behind szp is that all copies of one szp pointer can be made
//  to point to the same object in O(1) time. This means that having a
//  single szp of an actor, you can set them all to NULL without iteration.
//  And, as a bonus, on every pointer access, a NULL check can throw a
//  specific exception. Naturally, you should always be careful with pointers.
//
//  electricbrass - the new and improved thread-safe szp
//
//  szp now uses std::shared_ptr internally, which replaces the linked list
//  with an atomic reference count for thread-safe copying.
//
//  All copies of an szp can also be nulled safely in a multi-threaded
//  environment, as the raw pointer shared by the copies is atomic.
//
//  Note that only szp operations are thread-safe. Just like std::shared_ptr,
//  the pointed-to object will need to bring its own thread-safety mechanism
//  if it will be mutated while shared across threads.
//
//-----------------------------------------------------------------------------


#pragma once

#include <atomic>
#include <bit>
#include <functional>       // for std::hash.
#include <memory>
#include <utility>

#include "errors.h"
#include "m_stacktrace.h"

template <typename T>
class szp
{
private:
	// pointer to an atomic raw pointer
	std::shared_ptr<std::atomic<T*>> m_pointer;

public:

	szp() = default;
	explicit szp(T* target) : m_pointer{std::make_shared<std::atomic<T*>>(target)} {}

	// this should never be used
	// spawn from other pointers, or use init()
	szp& operator=(T* other) = delete;

	szp(const szp&) = default;
	szp(szp&&) noexcept = default;

	szp& operator=(const szp&) = default;
	szp& operator=(szp&&) noexcept = default;

	friend std::hash<szp<T>>;

	friend void swap(szp& lhs, szp& rhs) noexcept
	{
		using std::swap;
		swap(lhs.m_pointer,  rhs.m_pointer);
	}

	// creates the first (original) pointer
	void init(T *target)
	{
		m_pointer = std::make_shared<std::atomic<T*>>(target);
	}

	// this function can update or zero all related pointers
	void update_all(T *target)
	{
		if (not m_pointer)
			throw CRecoverableError(M_GetStacktrace("szp pointer was NULL on update_all:"));

		// all copies already have naive, so their pointers will update too
		m_pointer->store(target, std::memory_order_relaxed);
	}

	// use this instead of std::atomic's operator T because we want relaxed memory order
	T* get() const noexcept
    {
        if (not m_pointer)
            return nullptr;

        return m_pointer->load(std::memory_order_relaxed);
    }

	// use as pointer, checking validity
	T* operator->()
	{
		T* ptr = get();
		if (not ptr)
			throw CRecoverableError(M_GetStacktrace("szp pointer was NULL:"));

		return ptr;
	}

	const T* operator->() const
	{
		const T* ptr = get();
		if (not ptr)
			throw CRecoverableError(M_GetStacktrace("szp pointer was NULL:"));

		return ptr;
	}

	// use as raw pointer
	operator T*() noexcept
	{
		return get();
	}

	// use as raw pointer
	operator const T*() const noexcept
	{
		return get();
	}

	static constexpr auto POINTER_ALIGNMENT = alignof(std::atomic<T*>);
	static constexpr auto POINTER_ALIGNMENT_BITS = std::countr_zero(POINTER_ALIGNMENT);
};

// The specializations of std::hash for szp shift off the known-zero LSBs of the naive pointer
// because we want to make sure that modulus-based bucket selection for unordered_set and the
// like don't accidentally pile everything up in a single bucket.

template <typename T>
struct std::hash<szp<T>>
{
	size_t operator()(const szp<T>& objPtr) const noexcept
	{
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return reinterpret_cast<std::uintptr_t>(objPtr.m_pointer.get()) >> szp<T>::POINTER_ALIGNMENT_BITS;
	}
};
