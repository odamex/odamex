// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2014 by The Odamex Team.
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
// A static array implementation utilizing unique IDs for access. 
//
//-----------------------------------------------------------------------------

#ifndef __SARRAY_H__
#define __SARRAY_H__

#include <algorithm>
#include <iterator>
#include <cassert>

// ============================================================================
//
// SArray 
//
// Lightweight fixed-size array implementation with iterator.
// The class provides many desirable characteristics:
//		Constant time insertion & removal
//		Dense storage for good cache usage
//		Fixed-size to eliminate memory allocation/deallocation time
//		Generation of ID-based handles rather than raw pointers
//		Iterators that are STL conformant for use with <algorithm>
//
// ------------------------------------------------------------------------
// 
// Notes:
//
// There are a fixed number of slots for the array, between 1 and MAX_SIZE.
// Each slot has an item (of type VT) and an ID field. Unique IDs are
// delegated to slots upon insertion of a new item and are composed of two
// separate partitions: The highest KEY_BITS bits are from mIdKey, which is
// incremented each time an item is inserted. The lowest SLOT_BITS bits are
// simply the slot number.
//
// A linked list of slots that have been freed is maintained. For slots
// that have been freed, a slot's ID field contains the slot number of the
// next slot in the list of freed slots. A slot can be identified as being
// freed by having 0x0 for the key portion of the ID field or by having
// its ID equal to NOT_FOUND.
//
// To maintain data density for good cache usage, it is preferable to resuse
// free slots rather than use a new slot when inserting an item. This
// prevents memory fragmentation and also allows for quicker iteration since
// the iteration scheme will iterate through all slots that have been
// assigned at one time.
//
// ============================================================================

typedef unsigned int SArrayId;

// ----------------------------------------------------------------------------
// SArray interface & inline implementation
// ----------------------------------------------------------------------------

template <typename VT, int N = 16>
class SArray
{
private:
	typedef SArray<VT, N> SArrayType;

	struct ItemRecord
	{
		VT				mItem;
		SArrayId		mId;
	};

public:
	// ------------------------------------------------------------------------
	// SArray::iterator & const_iterator implementation
	// ------------------------------------------------------------------------

	template <typename IVT, typename ISAT> class generic_iterator;
	typedef generic_iterator<VT, SArrayType> iterator;
	typedef generic_iterator<const VT, const SArrayType> const_iterator;

	template <typename IVT, typename ISAT>
	class generic_iterator : public std::iterator<std::forward_iterator_tag, SArray>
	{
	private:
		// typedef for easier-to-read code
		typedef generic_iterator<IVT, ISAT> ThisClass;
		typedef generic_iterator<const IVT, const ISAT> ConstThisClass;

	public:
		generic_iterator() :
			mSlot(0), mSArray(NULL)
		{ }

		// allow implicit converstion from iterator to const_iterator
		operator ConstThisClass() const
		{
			return ConstThisClass(mSlot, mSArray);
		}

		bool operator== (const ThisClass& other) const
		{
			return mSlot == other.mSlot && mSArray == other.mSArray;
		}

		bool operator!= (const ThisClass& other) const
		{
			return !(operator==(other));
		}

		IVT& operator* ()
		{
			return mSArray->mItemRecords[mSlot].mItem;
		}

		IVT* operator-> ()
		{
			return &(mSArray->mItemRecords[mSlot].mItem);
		}

		ThisClass& operator++ ()
		{
			mSlot = mSArray->nextUsed(++mSlot);
			return *this;
		}

		ThisClass operator++ (int)
		{
			generic_iterator temp(*this);
			mSlot = mSArray->nextUsed(++mSlot);
			return temp;
		}

	private:
		friend class SArray<VT>;

		generic_iterator(unsigned int slot, ISAT* sarray) :
			mSlot(slot), mSArray(sarray)
		{ }

		unsigned int	mSlot;
		ISAT*			mSArray;
	};



	// ------------------------------------------------------------------------
	// SArray functions
	// ------------------------------------------------------------------------

	SArray(unsigned int size)
	{
		mSize = size > MAX_SIZE ? MAX_SIZE : size;
		mItemRecords = new ItemRecord[mSize];
		clear();
	}

	SArray(const SArrayType& other)
	{
		mItemRecords = new ItemRecord[other.mSize];
		copyFrom(other);
	}

	~SArray()
	{
		delete [] mItemRecords;
	}

	SArray& operator= (const SArrayType& other)
	{
		if (mSize != other.mSize)
		{
			delete [] mItemRecords;
			mItemRecords = new ItemRecord[other.mSize];
		}

		copyFrom(other);
		return *this;
	}

	void clear()
	{
		mUsed = 0;
		mNextUnused = 0;
		mFreeHead = NOT_FOUND;
		mIdKey = MIN_KEY;
	}

	bool empty() const
	{
		return mUsed == 0;
	}

	size_t size() const
	{
		return mUsed;
	}

	size_t max_size() const
	{
		return MAX_SIZE;
	}

	size_t capacity() const
	{
		return mSize;
	}

	iterator begin()
	{
		return iterator(nextUsed(0), this);
	}

	const_iterator begin() const
	{
		return const_iterator(nextUsed(0), this);
	}

	iterator end()
	{
		return iterator(NOT_FOUND, this);
	}

	const_iterator end() const
	{
		return const_iterator(NOT_FOUND, this);
	}	

	bool validate(const SArrayId id) const
	{
		return getSlot(id) != NOT_FOUND;
	}

	iterator find(const SArrayId id)
	{
		return iterator(getSlot(id), this);
	}

	const_iterator find(const SArrayId id) const
	{
		return const_iterator(getSlot(id), this);
	}

	VT& get(const SArrayId id)
	{
		unsigned int slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}

	const VT& get(const SArrayId id) const
	{
		unsigned int slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}
		
	VT& operator[](const SArrayId id)
	{
		unsigned int slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}

	const VT& operator[](const SArrayId id) const
	{
		unsigned int slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}

	const SArrayId getId(const VT& item) const
	{
		unsigned int slot = getSlot(item);
		assert(slot != NOT_FOUND);
		return mItemRecords[getSlot(item)].mId;
	}

	const SArrayId insert()
	{
		unsigned int slot = insertSlot();
		return mItemRecords[slot].mId;
	}

	const SArrayId insert(const VT& item)
	{
		unsigned int slot = insertSlot();
		mItemRecords[slot].mItem = item;
		return mItemRecords[slot].mId;
	}

	inline void erase(const SArrayId id)
	{
		unsigned int slot = getSlot(id);
		assert(slot != NOT_FOUND);
		eraseSlot(slot);
	}

	inline void erase(const VT& item)
	{
		unsigned int slot = getSlot(item);
		assert(slot != NOT_FOUND);
		eraseSlot(slot);
	}

	inline void erase(iterator it)
	{
		assert(it.slot != NOT_FOUND);
		eraseSlot(it.mSlot);
	}

	inline void erase(iterator it1, iterator it2)
	{
		while (it1 != it2)
		{
			erase(it1);
			++it1;
		}
	}

private:
	inline void resize(unsigned int newsize)
	{
		ItemRecord* newitemrecords = new ItemRecord[newsize];
		for (unsigned int i = 0; i < mNextUnused; i++)
		{
			newitemrecords[i].mItem = mItemRecords[i].mItem;
			newitemrecords[i].mId = mItemRecords[i].mId;
		}
		
		delete [] mItemRecords;
		mSize = newsize;
		mItemRecords = newitemrecords;
		assert(mItemRecords != NULL);
		assert(mSize <= MAX_SIZE);
	}

	inline unsigned int getSlot(const SArrayId id) const
	{
		unsigned int slot = id & SLOT_MASK;
		assert(slot < mSize);
		if (mItemRecords[slot].mId == id)
			return slot;
		return NOT_FOUND;
	}

	inline unsigned int getSlot(const VT& item) const
	{
		unsigned int slot = (ItemRecord*)(&item) - mItemRecords;
		if (slot < mSize && slotUsed(slot))
			return slot;
		return NOT_FOUND;
	}

	inline const SArrayId generateId(unsigned int slot)
	{
		assert(slot < mSize);
		SArrayId id = (mIdKey << SLOT_BITS) | slot;
		mIdKey++;
		if (mIdKey > MAX_KEY)
			mIdKey = MIN_KEY;
		return id;
	}

	inline bool slotUsed(unsigned int slot) const
	{
		return ((mItemRecords[slot].mId >> SLOT_BITS) >= MIN_KEY);
	}

	inline unsigned int nextUsed(unsigned int slot) const
	{
		while (slot < mNextUnused && !slotUsed(slot))
			slot++;
		assert(slot < mSize);
		return (slot < mNextUnused) ? slot : NOT_FOUND;
	}

	inline unsigned int insertSlot()
	{
		// need to resize?
		if (mUsed == mSize)
		{
			unsigned int newsize = 2 * mSize > MAX_SIZE ? MAX_SIZE : 2 * mSize;
			// is it full and not able to be resized?
			assert(mSize != newsize);
			if (mSize == newsize)
				return NOT_FOUND;
			else
				resize(newsize);
		}

		unsigned int slot = mFreeHead;
		if (slot != NOT_FOUND)
			mFreeHead = mItemRecords[slot].mId;
		else
			slot = mNextUnused++;
		
		assert(slot < mSize);
		mItemRecords[slot].mId = generateId(slot);
		mUsed++;
		return slot;
	}

	inline void eraseSlot(unsigned int slot)
	{
		assert(slot < mSize);
		if (slotUsed(slot))
		{
			mItemRecords[slot].mId = mFreeHead;
			mFreeHead = slot;
			mUsed--;
		}
	}

	inline void copyFrom(const SArrayType& other)
	{
		mSize = other.mSize;
		for (unsigned int i = 0; i < mNextUnused; i++)
		{
			mItemRecords[i].mItem = other.mItemRecords[i].mItem;
			mItemRecords[i].mId = other.mItemRecords[i].mId;
		}

		mUsed = other.mUsed;
		mNextUnused = other.mNextUnused;
		mFreeHead = other.mFreeHead;
		mIdKey = other.mIdKey;
	}

	static const unsigned int SLOT_BITS = N; 
	static const unsigned int KEY_BITS = 32 - SLOT_BITS;
	static const unsigned int MAX_SIZE = 1 << SLOT_BITS;

	static const unsigned int MIN_KEY = 2;
	static const unsigned int MAX_KEY = (1 << KEY_BITS) - 1;

	static const unsigned int MIN_SLOT = 0;
	static const unsigned int MAX_SLOT = (1 << SLOT_BITS) - 1; 
	static const unsigned int SLOT_MASK = (1 << SLOT_BITS) - 1;

	static const unsigned int NOT_FOUND = (1 << SLOT_BITS) | MAX_SLOT;

	ItemRecord*		mItemRecords;
	unsigned int	mSize;
	unsigned int	mUsed;
	unsigned int	mNextUnused;
	unsigned int	mFreeHead;
	unsigned int	mIdKey;
};

#endif	// __SARRAY_H__

