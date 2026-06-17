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
//-----------------------------------------------------------------------------


#pragma once

#include "m_stacktrace.h"

template <typename T>
class szp
{
	// pointer to a common raw pointer
	T **naive;

	// circular linked list
	szp *prev, *next;

	// this should never be used
	// spawn from other pointers, or use init()
	szp &operator=(T *other) = delete;

	// utility function to remove oneself from the linked list
	void inline unlink()
	{
		if(!next)
			return;

		next->prev = prev;
		prev->next = next;

		if(!naive)
			return;

		// last in ring?
		if(this == next)
			delete naive;

		naive = NULL;
	}

public:

	// use as pointer, checking validity
	inline T* operator ->()
	{
		if(!naive || !*naive)
			throw CRecoverableError(M_GetStacktrace("szp pointer was NULL:"));

		return *naive;
	}

	const inline T* operator ->() const
	{
		if(!naive || !*naive)
			throw CRecoverableError(M_GetStacktrace("szp pointer was NULL:"));

		return *naive;
	}

	// use as raw pointer
	inline operator T*()
	{
		if(!naive)
			return NULL;
		else
			return *naive;
	}

	// use as raw pointer
	inline operator const T*() const
	{
		if(!naive)
			return NULL;
		else
			return *naive;
	}

	// this function can update or zero all related pointers
	void update_all(T *target)
	{
		if(!naive)
			throw CRecoverableError(M_GetStacktrace("szp pointer was NULL on update_all:"));

		// all copies already have naive, so their pointers will update too
		*naive = target;
	}

	// copy a pointer and add self to the "i have this pointer" list
	inline szp &operator =(szp other)
	{
		// itself?
		if(&other == this || other.naive == naive)
			return *this;

		unlink();

		if(!other.prev || !other.next || !other.naive)
		{
			next = prev = this;
			return *this;
		}

		// link
		naive = other.naive;
		prev = other.next->prev;
		next = other.next;
		prev->next = next->prev = this;

		return *this;
	}

	// creates the first (original) pointer
	void init(T *target)
	{
		unlink();

		// Please note that by using a naive call to `new`, and the fact that we're
		// in C++17 and up, __STDCPP_DEFAULT_NEW_ALIGNMENT__ applies and can be relied
		// on to know how many least-significant bits of our address will be zero.
		// This will be important for our specialization of std::hash.
		//
		naive = new T*(target);

		// first link
		prev = next = this;
	}

	// cheap constructor
	inline szp()
		: naive(NULL), prev(NULL), next(NULL)
	{ }

	// copy constructor
	inline szp(const szp &other)
		: naive(NULL)
	{
		if(!other.prev || !other.next || !other.naive)
		{
			prev = next = this;
			return;
		}

		// link
		naive = other.naive;
		prev = other.next->prev;
		next = other.next;
		prev->next = next->prev = this;
	}

	// unlink from circular list on destruction
	inline ~szp()
	{
		unlink();
	}

	friend std::hash<szp<T>>;

};

namespace std
{
    template <typename U>
    constexpr bool IS_8_BIT_ALIGNED = __STDCPP_DEFAULT_NEW_ALIGNMENT__ == 1;

    template <typename U>
    constexpr bool IS_16_BIT_ALIGNED = __STDCPP_DEFAULT_NEW_ALIGNMENT__ == 2;

    template <typename U>
    constexpr bool IS_32_BIT_ALIGNED = __STDCPP_DEFAULT_NEW_ALIGNMENT__ == 4;

    template <typename U>
    constexpr bool IS_64_BIT_ALIGNED = __STDCPP_DEFAULT_NEW_ALIGNMENT__ == 8;

    template <typename U>
    constexpr bool IS_128_BIT_ALIGNED = __STDCPP_DEFAULT_NEW_ALIGNMENT__ == 16;

    template <typename T>
        requires IS_8_BIT_ALIGNED<T>
    struct hash<szp<T>>
    {
        size_t operator()(const szp<T>& objPtr) const noexcept
        {
            return reinterpret_cast<size_t>(objPtr.naive);
        }
    };

    template <typename T>
        requires IS_16_BIT_ALIGNED<T>
    struct hash<szp<T>>
    {
        size_t operator()(const szp<T>& objPtr) const noexcept
        {
            return reinterpret_cast<size_t>(objPtr.naive) >> 1;
        }
    };

    template <typename T>
        requires IS_32_BIT_ALIGNED<T>
    struct hash<szp<T>>
    {
        size_t operator()(const szp<T>& objPtr) const noexcept
        {
            return reinterpret_cast<size_t>(objPtr.naive) >> 2;
        }
    };

    template <typename T>
        requires IS_64_BIT_ALIGNED<T>
    struct hash<szp<T>>
    {
        size_t operator()(const szp<T>& objPtr) const noexcept
        {
            return reinterpret_cast<size_t>(objPtr.naive) >> 3;
        }
    };

    template <typename T>
        requires IS_128_BIT_ALIGNED<T>
    struct hash<szp<T>>
    {
        size_t operator()(const szp<T>& objPtr) const noexcept
        {
            // Uncomment the following to prove that this overload of std::hash
            // is actually being used in the code automatically.
            //
            //static_assert(alignof(T) == 2);
            return reinterpret_cast<size_t>(objPtr.naive) >> 4;
        }
    };
}

