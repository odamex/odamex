// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//
// A static array implementation utilizing unique IDs for access. 
//
//-----------------------------------------------------------------------------

#ifndef __SARRAY_H__
#define __SARRAY_H__

#include <algorithm>
#include <iterator>
#include <cassert>

#include <cstdlib>
#include <ctime>

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
	typedef unsigned int SlotNumber;

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
		generic_iterator(ISAT& sarray) :
			mSArray(sarray), mSlot(NOT_FOUND)
		{ }

		generic_iterator(ISAT& sarray, SArrayId id) :
			mSArray(sarray), mSlot(NOT_FOUND)
		{
			if (id != NOT_FOUND)
				mSlot = mSArray.getSlot(id);
		}

		generic_iterator(ISAT& sarray, const VT& item) :
			mSArray(sarray)
		{
			mSlot = mSArray.getSlot(item);
		}

		// allow implicit converstion from iterator to const_iterator
		inline operator ConstThisClass() const
		{
			return ConstThisClass(mSArray, mSlot);
		}

		inline bool operator== (const ThisClass& other) const
		{
			return &mSArray == &other.mSArray && mSlot == other.mSlot;
		}

		inline bool operator!= (const ThisClass& other) const
		{
			return !(operator==(other));
		}

		inline IVT& operator* ()
		{
			return mSArray.mItemRecords[mSlot].mItem;
		}

		inline IVT* operator-> ()
		{
			return &(mSArray.mItemRecords[mSlot].mItem);
		}

		inline ThisClass& operator++ ()
		{
			mSlot = mSArray.nextUsed(++mSlot);
			return *this;
		}

		inline ThisClass operator++ (int)
		{
			generic_iterator temp(*this);
			return temp.operator++ ();
		}

		inline ThisClass& operator+= (unsigned int n)
		{
			while (n--)
				operator++ ();
			return *this;
		}

		inline ThisClass operator+ (unsigned int n) const
		{
			generic_iterator temp(*this);
			return temp.operator+= (n);
		}

		inline IVT& operator[] (unsigned int n)
		{
			generic_iterator temp(operator+ (n));
			return temp.operator* ();
		}

		inline bool operator< (const ThisClass& other) const
		{
			assert(&mSArray == &other.mSArray);
			return mSlot < other.mSlot;
		}

		inline bool operator<= (const ThisClass& other) const
		{
			assert(&mSArray == &other.mSArray);
			return mSlot <= other.mSlot;
		}

		inline bool operator> (const ThisClass& other) const
		{
			assert(&mSArray == &other.mSArray);
			return mSlot > other.mSlot;
		}

		inline bool operator>= (const ThisClass& other) const
		{
			assert(&mSArray == &other.mSArray);
			return mSlot >= other.mSlot;
		}

	private:
		ISAT&			mSArray;
		SlotNumber		mSlot;
	};



	// ------------------------------------------------------------------------
	// SArray functions
	// ------------------------------------------------------------------------

	//
	// SArray::SArray
	//
	// Initializes the container to the specified size.
	//
	SArray(unsigned int size) :
		mItemRecords(NULL), mSize(0)
	{
		clear();
		resize(size > MAX_SIZE ? MAX_SIZE : size);
	}

	//
	// SArray:SArray
	//
	// Initializes the container as a copy of the given SArray
	//
	SArray(const SArrayType& other) :
		mItemRecords(NULL), mSize(0)
	{
		clear();
		resize(other.mSize);
		copyFrom(other);
	}

	//
	// SArray::~SArray
	//
	// Frees the memory used by the storage container.
	//
	~SArray()
	{
		delete [] mItemRecords;
	}

	//
	// SArray::operator=
	//
	// Copies the contents of the given SArray to this one.
	//
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

	//
	// SArray::clear
	//
	// Removes all elements from the container but does leaves the allocated
	// memory in-tact. Any IDs assigned prior to clearing will be invalidated.
	//
	void clear()
	{
		mUsed = 0;
		mNextUnused = 0;
		mFreeHead = NOT_FOUND;

		// Set mIdKey to a random value to further help stale IDs handed out
		// before clear was called.
		srand(time(NULL));
		mIdKey = MIN_KEY + (rand() % (MAX_KEY - MIN_KEY));

		for (unsigned int i = 0; i < mSize; i++)
			mItemRecords[i].mId = NOT_FOUND;
	}

	//
	// SArray::empty
	//
	// Returns true if the container is empty.
	//
	inline bool empty() const
	{
		return mUsed == 0;
	}

	//
	// SArray::size
	//
	// Returns the number of items currently stored in the container.
	//
	inline size_t size() const
	{
		return mUsed;
	}

	//
	// SArray::max_size
	//
	// Returns the maximum size that the container can grow to. This number
	// is directly based on the template parameter N.
	//
	inline size_t max_size() const
	{
		return MAX_SIZE;
	}

	//
	// SArray::capacity
	//
	// Returns the current storage container size. This capacity can change as
	// the storage container is resized to accommodate additional insertions,
	// up to a maximum capacity dictated by the max_size function.
	//
	inline size_t capacity() const
	{
		return mSize;
	}

	//
	// SArray::begin
	//
	// Returns an iterator to the first item stored in the container.
	//
	inline iterator begin()
	{
		if (empty())
			return end();
		return iterator(*this, mItemRecords[nextUsed(0)].mId);
	}

	//
	// SArray::begin
	//
	inline const_iterator begin() const
	{
		if (empty())
			return end();
		return const_iterator(*this, mItemRecords[nextUsed(0)].mId);
	}

	//
	// SArray::end
	//
	inline iterator end()
	{
		return iterator(*this, NOT_FOUND);
	}

	//
	// SArray::end
	//
	inline const_iterator end() const
	{
		return const_iterator(*this, NOT_FOUND);
	}	

	//
	// SArray::validate
	//
	// Indicates whether the given ID represents a valid item in the
	// storage container.
	//
	inline bool validate(const SArrayId id) const
	{
		return getSlot(id) != NOT_FOUND;
	}

	//
	// SArray::find
	//
	// Returns an iterator to the item matching the given ID.
	//
	inline iterator find(const SArrayId id)
	{
		return iterator(*this, id);
	}

	//
	// SArray::find
	//
	inline const_iterator find(const SArrayId id) const
	{
		return const_iterator(*this, id);
	}

	//
	// SArray::get
	//
	// Returns the item matching the given ID. Note that passing an invalid ID
	// results in undefined behavior.
	//
	inline VT& get(const SArrayId id)
	{
		SlotNumber slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}

	//
	// SArray::get
	//
	inline const VT& get(const SArrayId id) const
	{
		SlotNumber slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}
		
	//
	// SArray::operator[]
	//
	// Returns the item matching the given ID. Note that passing an invalid ID
	// results in undefined behavior.
	inline VT& operator[](const SArrayId id)
	{
		return get(id);
	}

	inline const VT& operator[](const SArrayId id) const
	{
		return get(id);
	}

	//
	// SArray::getId
	//
	// Returns the ID for the given item. Note that passing an item that is not
	// a reference to an item obtained through the get accessor results in
	// undefined behavior.
	//
	inline SArrayId getId(const VT& item) const
	{
		assert(getSlot(item) != NOT_FOUND);
		return mItemRecords[getSlot(item)].mId;
	}

	//
	// SArray::insert
	//
	// Inserts an uninitialized item into the container and returns the ID for
	// the item.
	//
	inline SArrayId insert()
	{
		SlotNumber slot = insertSlot();
		return mItemRecords[slot].mId;
	}

	//
	// SArray::insert
	//
	// Inserts a copy of the given item into the container and returns the ID
	// for the item.
	//
	inline SArrayId insert(const VT& item)
	{
		SlotNumber slot = insertSlot();
		mItemRecords[slot].mItem = item;
		return mItemRecords[slot].mId;
	}

	//
	// SArray::erase
	//
	// Removes the item matching the given ID from the container. Note that the
	// item's destructor will not be called until this SArray instance is
	// deleted or goes out of scope.
	//
	inline void erase(const SArrayId id)
	{
		SlotNumber slot = getSlot(id);
		assert(slot != NOT_FOUND);
		eraseSlot(slot);
	}

	//
	// SArray::erase
	//
	// Removes the given item from the container. Note that the item's destructor
	// will not be called until this SArray instance is deleted or goes out of scope.
	// Additionally, passing an item that is not a reference to an item obtained
	// through the get accessor results in undefined behavior.
	//
	inline void erase(const VT& item)
	{
		SlotNumber slot = getSlot(item);
		assert(slot != NOT_FOUND);
		eraseSlot(slot);
	}

	//
	// SArray::erase
	//
	// Removes the item pointed to by the given iterator. Note that the item's
	// destructor will not be called until this SArray instance is deleted or
	// goes out of scope.
	//
	inline void erase(iterator it)
	{
		assert(it.slot != NOT_FOUND);
		eraseSlot(it.mSlot);
	}

	//
	// SArray::erase
	//
	// Remove all of the items between the given iterators, inclusive. Note that
	// the items' destructors will not be called until this SArray instance is
	// deleted or goes out of scope.
	//
	inline void erase(iterator it1, iterator it2)
	{
		while (it1 != it2)
		{
			erase(it1);
			++it1;
		}
	}

private:
	//
	// SArray::resize
	//
	// Resizes the storage array mItemReocrds to the new specified size.
	//
	inline void resize(unsigned int newsize)
	{
		assert(newsize > mSize);
		assert(newsize <= MAX_SIZE);

		ItemRecord* newitemrecords = new ItemRecord[newsize];
		for (SlotNumber i = 0; i < mNextUnused; i++)
		{
			newitemrecords[i].mItem = mItemRecords[i].mItem;
			newitemrecords[i].mId = mItemRecords[i].mId;
		}
		for (SlotNumber i = mNextUnused; i < newsize; i++)
			newitemrecords[i].mId = NOT_FOUND;
		
		delete [] mItemRecords;
		mSize = newsize;
		mItemRecords = newitemrecords;
		assert(mItemRecords != NULL);
	}

	//
	// SArray::getSlot
	//
	// Returns the slot portion of the given ID, verifying that the key portion
	// of the ID is correct.
	//
	inline SlotNumber getSlot(const SArrayId id) const
	{
		SlotNumber slot = id & SLOT_MASK;
		assert(slot < mSize);
		if (slotUsed(slot) && mItemRecords[slot].mId == id)
			return slot;
		return NOT_FOUND;
	}

	//
	// SArray::getSlot
	//
	// Returns the slot occupied by the given item. The item must be a reference
	// returned by one of the class's accessor functions.
	//
	inline SlotNumber getSlot(const VT& item) const
	{
		SlotNumber slot = (ItemRecord*)(&item) - mItemRecords;
		if (slot < mSize && slotUsed(slot))
			return slot;
		return NOT_FOUND;
	}

	//
	// SArray::generateId
	//
	// Creates a new ID number from a combination of mIdKey and the
	// given slot number. mIdKey is then incremented, handling wrap-around.
	//
	inline SArrayId generateId(SlotNumber slot)
	{
		assert(slot < mSize);
		SArrayId id = (mIdKey << SLOT_BITS) | slot;
		mIdKey++;
		if (mIdKey > MAX_KEY)
			mIdKey = MIN_KEY;
		return id;
	}

	//
	// SArray::slotUsed
	//
	// Indicates if the given slot is currently in use.
	//
	inline bool slotUsed(SlotNumber slot) const
	{
		return ((mItemRecords[slot].mId >> SLOT_BITS) >= MIN_KEY);
	}

	//
	// SArray::nextUsed
	//
	// Returns the slot number for the next slot in use following given slot.
	//
	inline SlotNumber nextUsed(SlotNumber slot) const
	{
		while (slot < mNextUnused && !slotUsed(slot))
			slot++;
		assert(slot < mSize);
		return (slot < mNextUnused) ? slot : NOT_FOUND;
	}

	//
	// SArray::prevUsed
	//
	// Returns the slot number for the slot in use that preceeds the given slot.
	//
	inline SlotNumber prevUsed(SlotNumber slot) const
	{
		while (slot > 0 && !slotUsed(slot))
			slot--;
		if (slot == 0 && !slotUsed(slot))
			return NOT_FOUND;
		return slot;
	}

	//
	// SArray::insertSlot
	//
	// Determines the next availible slot for item insertion. If the current
	// storage array mItemRecords is full, it will be resized if possible.
	// Tracking variables mFreeHead and mNextUnused are updated as needed.
	// The number of the slot availible for insertion is returned.
	//
	inline SlotNumber insertSlot()
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

		SlotNumber slot = mFreeHead;
		if (slot != NOT_FOUND)
			mFreeHead = mItemRecords[slot].mId;
		else
			slot = mNextUnused++;
		
		assert(slot < mSize);
		mItemRecords[slot].mId = generateId(slot);
		mUsed++;
		return slot;
	}

	//
	// SArray::eraseSlot
	//
	// Marks the given slot as being unused and updates the tracking variables
	// mFreeHead and mUsed.
	//
	inline void eraseSlot(SlotNumber slot)
	{
		assert(slot < mSize);
		if (slotUsed(slot))
		{
			mItemRecords[slot].mId = mFreeHead;
			mFreeHead = slot;
			mUsed--;
		}
	}

	//
	// SArray::copyFrom
	//
	// Helper function for the assignment operator and copy constructor. Handles
	// copying the contents of another SArray to this one.
	//
	inline void copyFrom(const SArrayType& other)
	{
		mSize = other.mSize;
		for (SlotNumber i = 0; i < mNextUnused; i++)
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
	SlotNumber		mNextUnused;
	SlotNumber		mFreeHead;
	unsigned int	mIdKey;
};

#endif	// __SARRAY_H__
