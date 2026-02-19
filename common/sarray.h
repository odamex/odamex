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
// A static array implementation utilizing unique IDs for access.
//
//-----------------------------------------------------------------------------

#pragma once

#include <algorithm>
#include <iterator>
#include <cassert>
#include <random>

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

using SArrayId = size_t;

// ----------------------------------------------------------------------------
// SArray interface & implementation
// ----------------------------------------------------------------------------

template <typename VT, int N = 16,
	typename = std::enable_if_t<
		// VT has to be default constructible, but this check breaks OString
		// see https://www.open-std.org/JTC1/SC22/WG21/docs/cwg_active.html#2335
		// std::is_default_constructible_v<VT> &&
		std::is_copy_assignable_v<VT> &&
		std::is_copy_constructible_v<VT>
	>>
class SArray
{
private:
	struct ItemRecord
	{
		VT				mItem;
		SArrayId		mId = NOT_FOUND;
	};

	using ItemRecords = std::vector<ItemRecord>;
	using SizeType = typename ItemRecords::size_type;
	using SArrayType = SArray<VT, N>;
	using SlotNumber = SizeType;
public:
	// ------------------------------------------------------------------------
	// SArray::iterator & const_iterator implementation
	// ------------------------------------------------------------------------

	template <typename IVT, typename ISAT> class generic_iterator;
	typedef generic_iterator<VT, SArrayType> iterator;
	typedef generic_iterator<const VT, const SArrayType> const_iterator;

	template <typename IVT, typename ISAT>
	class generic_iterator
	{
	private:
		// typedef for easier-to-read code
		typedef generic_iterator<IVT, ISAT> ThisClass;
		typedef generic_iterator<const IVT, const ISAT> ConstThisClass;

	public:
		using iterator_category = std::forward_iterator_tag;
		using value_type = IVT;
		using difference_type = std::ptrdiff_t;
		using pointer = value_type*;
		using reference = value_type&;

		generic_iterator(ISAT& sarray) noexcept :
			mSArray(sarray), mSlot(NOT_FOUND)
		{ }

		generic_iterator(ISAT& sarray, SArrayId id) noexcept :
			mSArray(sarray), mSlot(NOT_FOUND)
		{
			if (id != NOT_FOUND)
				mSlot = mSArray.getSlot(id);
		}

		generic_iterator(ISAT& sarray, const VT& item) noexcept :
			mSArray(sarray)
		{
			mSlot = mSArray.getSlot(item);
		}

		// allow implicit converstion from iterator to const_iterator
		operator ConstThisClass() const noexcept
		{
			return ConstThisClass(mSArray, mSlot);
		}

		[[nodiscard]]
		bool operator== (const ThisClass& other) const noexcept
		{
			return &mSArray == &other.mSArray && mSlot == other.mSlot;
		}

		bool operator!= (const ThisClass& other) const noexcept
		{
			return !(operator==(other));
		}

		reference operator* () noexcept
		{
			return mSArray.mItemRecords[mSlot].mItem;
		}

		reference operator* () const noexcept
		{
			return mSArray.mItemRecords[mSlot].mItem;
		}

		pointer operator-> () noexcept
		{
			return &(mSArray.mItemRecords[mSlot].mItem);
		}

		pointer operator-> () const noexcept
		{
			return &(mSArray.mItemRecords[mSlot].mItem);
		}

		ThisClass& operator++ () noexcept
		{
			mSlot = mSArray.nextUsed(++mSlot);
			return *this;
		}

		ThisClass operator++ (int) noexcept
		{
			generic_iterator temp(*this);
			return temp.operator++ ();
		}

		ThisClass& operator+= (difference_type n) noexcept
		{
			while (n--)
				operator++ ();
			return *this;
		}

		ThisClass operator+ (difference_type n) const noexcept
		{
			generic_iterator temp(*this);
			return temp.operator+= (n);
		}

		reference operator[] (difference_type n) noexcept
		{
			generic_iterator temp(operator+ (n));
			return temp.operator* ();
		}

		bool operator< (const ThisClass& other) const noexcept
		{
			assert(&mSArray == &other.mSArray);
			return mSlot < other.mSlot;
		}

		bool operator<= (const ThisClass& other) const noexcept
		{
			assert(&mSArray == &other.mSArray);
			return mSlot <= other.mSlot;
		}

		bool operator> (const ThisClass& other) const noexcept
		{
			assert(&mSArray == &other.mSArray);
			return mSlot > other.mSlot;
		}

		bool operator>= (const ThisClass& other) const noexcept
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
	explicit SArray(SizeType size) :
		mSize(0)
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
		mSize(0)
	{
		clear();
		resize(other.mSize);
		copyFrom(other);
	}

	//
	// SArray:SArray
	//
	// Initializes the container by moving from the given SArray
	//
	SArray(SArrayType&& other) :
		mSize(other.mSize)
	{
		clear();
		moveFrom(std::move(other));
	}

	//
	// SArray::~SArray
	//
	// Frees the memory used by the storage container.
	//
	~SArray() {}

	//
	// SArray::operator=
	//
	// Copies the contents of the given SArray to this one.
	//
	SArray& operator= (const SArrayType& other)
	{
		if (&other == this)
		{
			return *this;
		}

		if (mSize != other.mSize)
		{
			mItemRecords.resize(other.mItemRecords.size());
		}

		copyFrom(other);
		return *this;
	}

	//
	// SArray::operator=
	//
	// Moves the contents of the given SArray to this one.
	//
	SArray& operator= (SArrayType&& other) noexcept
	{
		moveFrom(std::move(other));
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
		static std::mt19937 rng(std::random_device{}());
		std::uniform_int_distribution<decltype(mIdKey)> dist(MIN_KEY, MAX_KEY - 1);
		mIdKey = dist(rng);

		// make sure destructors are called
		// the standard guarantees that this
		// will not (de)allocate the vector storage
		mItemRecords.clear();
		mItemRecords.resize(mSize);
	}

	//
	// SArray::empty
	//
	// Returns true if the container is empty.
	//
	bool empty() const noexcept
	{
		return mUsed == 0;
	}

	//
	// SArray::size
	//
	// Returns the number of items currently stored in the container.
	//
	size_t size() const noexcept
	{
		return mUsed;
	}

	//
	// SArray::max_size
	//
	// Returns the maximum size that the container can grow to. This number
	// is directly based on the template parameter N.
	//
	static constexpr size_t max_size() noexcept
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
	size_t capacity() const noexcept
	{
		return mSize;
	}

	//
	// SArray::begin
	//
	// Returns an iterator to the first item stored in the container.
	//
	iterator begin() noexcept
	{
		if (empty())
			return end();
		return iterator(*this, mItemRecords[nextUsed(0)].mId);
	}

	//
	// SArray::begin
	//
	const_iterator begin() const noexcept
	{
		if (empty())
			return end();
		return const_iterator(*this, mItemRecords[nextUsed(0)].mId);
	}

	//
	// SArray::end
	//
	iterator end() noexcept
	{
		return iterator(*this, NOT_FOUND);
	}

	//
	// SArray::end
	//
	const_iterator end() const noexcept
	{
		return const_iterator(*this, NOT_FOUND);
	}

	//
	// SArray::validate
	//
	// Indicates whether the given ID represents a valid item in the
	// storage container.
	//
	bool validate(const SArrayId id) const noexcept
	{
		return getSlot(id) != NOT_FOUND;
	}

	//
	// SArray::find
	//
	// Returns an iterator to the item matching the given ID.
	//
	iterator find(const SArrayId id) noexcept
	{
		return iterator(*this, id);
	}

	//
	// SArray::find
	//
	const_iterator find(const SArrayId id) const noexcept
	{
		return const_iterator(*this, id);
	}

	//
	// SArray::get
	//
	// Returns the item matching the given ID. Note that passing an invalid ID
	// results in undefined behavior.
	//
	VT& get(const SArrayId id) noexcept
	{
		SlotNumber slot = getSlot(id);
		assert(slot != NOT_FOUND);
		return mItemRecords[slot].mItem;
	}

	//
	// SArray::get
	//
	const VT& get(const SArrayId id) const noexcept
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
	VT& operator[](const SArrayId id) noexcept
	{
		return get(id);
	}

	const VT& operator[](const SArrayId id) const noexcept
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
	SArrayId getId(const VT& item) const noexcept
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
	SArrayId insert()
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
	SArrayId insert(const VT& item)
	{
		SlotNumber slot = insertSlot();
		mItemRecords[slot].mItem = item;
		return mItemRecords[slot].mId;
	}

	//
	// SArray::insert
	//
	// Moves the given item into the container and returns the ID
	// for the item.
	//
	SArrayId insert(VT&& item)
	{
		SlotNumber slot = insertSlot();
		mItemRecords[slot].mItem = std::move(item);
		return mItemRecords[slot].mId;
	}

	//
	// SArray::erase
	//
	// Removes the item matching the given ID from the container. Note that the
	// item's destructor will not be called until this SArray instance is
	// deleted or goes out of scope.
	//
	void erase(const SArrayId id) noexcept
	{
		const SlotNumber slot = getSlot(id);
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
	void erase(const VT& item) noexcept
	{
		const SlotNumber slot = getSlot(item);
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
	void erase(iterator it) noexcept
	{
		assert(it.slot != NOT_FOUND);
		eraseSlot(it.mSlot);
	}

	//
	// SArray::erase
	//
	// Remove all of the items between the given iterators, inclusive. Note that
	// the items' destructors will not be called until this SArray instance is
	// deleted, goes out of scope, or is cleared.
	//
	void erase(iterator it1, iterator it2) noexcept
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
	// Resizes the storage array mItemRecords to the new specified size.
	//
	void resize(SizeType newsize)
	{
		assert(newsize <= MAX_SIZE);
		mSize = newsize;
		mItemRecords.resize(newsize);
		assert(!mItemRecords.empty());
	}

	//
	// SArray::getSlot
	//
	// Returns the slot portion of the given ID, verifying that the key portion
	// of the ID is correct.
	//
	SlotNumber getSlot(const SArrayId id) const noexcept
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
	SlotNumber getSlot(const VT& item) const noexcept
	{
		SlotNumber slot = reinterpret_cast<const ItemRecord*>(std::addressof(item)) - mItemRecords.data();
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
	SArrayId generateId(SlotNumber slot) noexcept
	{
		assert(slot < mSize);
		const SArrayId id = (mIdKey << SLOT_BITS) | slot;
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
	bool slotUsed(SlotNumber slot) const noexcept
	{
		return ((mItemRecords[slot].mId >> SLOT_BITS) >= MIN_KEY);
	}

	//
	// SArray::nextUsed
	//
	// Returns the slot number for the next slot in use following given slot.
	//
	SlotNumber nextUsed(SlotNumber slot) const noexcept
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
	SlotNumber prevUsed(SlotNumber slot) const noexcept
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
	SlotNumber insertSlot()
	{
		// need to resize?
		if (mUsed == mSize)
		{
			const SizeType newsize = 2 * mSize > MAX_SIZE ? MAX_SIZE : 2 * mSize;
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
	void eraseSlot(SlotNumber slot) noexcept
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
	void copyFrom(const SArrayType& other) noexcept(std::is_nothrow_copy_assignable_v<VT>)
	{
		mSize = other.mSize;
		for (SlotNumber i = 0; i < other.mNextUnused; i++)
		{
			mItemRecords[i].mItem = other.mItemRecords[i].mItem;
			mItemRecords[i].mId = other.mItemRecords[i].mId;
		}

		mUsed = other.mUsed;
		mNextUnused = other.mNextUnused;
		mFreeHead = other.mFreeHead;
		mIdKey = other.mIdKey;
	}

	//
	// SArray::moveFrom
	//
	// Helper function for the move assignment operator and move constructor. Handles
	// copying the contents of another SArray to this one.
	//
	void moveFrom(SArrayType&& other) noexcept
	{
		mSize = other.mSize;
		mItemRecords = std::move(other.mItemRecords);

		mUsed = other.mUsed;
		mNextUnused = other.mNextUnused;
		mFreeHead = other.mFreeHead;
		mIdKey = other.mIdKey;
	}

	static constexpr SizeType SLOT_BITS = N;
	static constexpr SizeType KEY_BITS = 32 - SLOT_BITS;
	static constexpr SizeType MAX_SIZE = 1 << SLOT_BITS;

	static constexpr SizeType MIN_KEY = 2;
	static constexpr SizeType MAX_KEY = (1 << KEY_BITS) - 1;

	static constexpr SizeType MIN_SLOT = 0;
	static constexpr SizeType MAX_SLOT = (1 << SLOT_BITS) - 1;
	static constexpr SizeType SLOT_MASK = (1 << SLOT_BITS) - 1;

	static constexpr SizeType NOT_FOUND = (1 << SLOT_BITS) | MAX_SLOT;

	ItemRecords  mItemRecords;
	SizeType     mSize;
	SizeType     mUsed;
	SlotNumber   mNextUnused;
	SlotNumber   mFreeHead;
	SizeType     mIdKey;
};
