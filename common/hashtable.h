// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2013 by The Odamex Team.
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
// A hash table implementation with iterator.
//
//-----------------------------------------------------------------------------

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <cassert>
#include <utility>
#include <string>

// ============================================================================
//
// OHashTable
//
// Lightweight templated hashtable implementation with iterator.
// The template parameters are:
// 		key type,
//		value type,
//		hash functor with the following signature (optional):
//			size_t operator()(KT) const;
//
// ============================================================================

// forward declaration
template <typename KT, typename VT, typename HF> class OHashTable;

// Default hash functors for integer & string types
template <typename KT>
struct hashfunc
{ };

template <> struct hashfunc<unsigned char>
{	size_t operator()(unsigned char val) const { return (size_t)val; }	};

template <> struct hashfunc<signed char>
{	size_t operator()(signed char val) const { return (size_t)val; }	};

template <> struct hashfunc<unsigned short>
{	size_t operator()(unsigned short val) const { return (size_t)val; }	};

template <> struct hashfunc<signed short>
{	size_t operator()(signed short val) const { return (size_t)val; }	};

template <> struct hashfunc<unsigned int>
{	size_t operator()(unsigned int val) const { return (size_t)val; }	};

template <> struct hashfunc<signed int>
{	size_t operator()(signed int val) const { return (size_t)val; }		};

template <> struct hashfunc<unsigned long long>
{	size_t operator()(unsigned long long val) const { return (size_t)val; }	};

template <> struct hashfunc<signed long long>
{	size_t operator()(signed long long val) const { return (size_t)val; }	};

static inline size_t __hash_cstring(const char* str)
{
	size_t val = 0;
	while (*str != 0)
		val = val * 101 + *str++;
	return val;	
}

template <> struct hashfunc<char*>
{	size_t operator()(const char* str) const { return __hash_cstring(str); } };

template <> struct hashfunc<const char*>
{	size_t operator()(const char* str) const { return __hash_cstring(str); } };

template <> struct hashfunc<std::string>
{	size_t operator()(const std::string& str) const { return __hash_cstring(str.c_str()); } };


// ----------------------------------------------------------------------------
// OHashTable interface & inline implementation
//
// The implementation is fairly straight forward. Hash collisions are resolved
// with open addressing using linear probing. Iteration through the hash table
// is done quickly by iterating through the internal array of key/value pairs.
// ----------------------------------------------------------------------------

template <typename KT, typename VT, typename HF = hashfunc<KT> >
class OHashTable
{
private:
	typedef std::pair<KT, VT> HashPairType;
	typedef OHashTable<KT, VT, HF> HashTableType;

	typedef unsigned short IndexType;
	static const size_t MAX_CAPACITY	= 65535;
	static const IndexType NOT_FOUND	= HashTableType::MAX_CAPACITY;

public:
	// ------------------------------------------------------------------------
	// OHashTable::iterator & const_iterator implementation
	// ------------------------------------------------------------------------

	template <typename IVT, typename IHTT> class generic_iterator;
	typedef generic_iterator<HashPairType, HashTableType> iterator;
	typedef generic_iterator<const HashPairType, const HashTableType> const_iterator;

	template <typename IVT, typename IHTT>
	class generic_iterator : public std::iterator<std::forward_iterator_tag, OHashTable>
	{
	private:
		// typedef for easier-to-read code
		typedef generic_iterator<IVT, IHTT> ThisClass;
		typedef generic_iterator<const IVT, const IHTT> ConstThisClass;

	public:
		generic_iterator() :
			mElementIndex(IHTT::NOT_FOUND), mHashTable(NULL)
		{ }

		// allow implicit converstion from iterator to const_iterator
		operator ConstThisClass() const
		{
			return ConstThisClass(mElementIndex, mHashTable);
		}

		bool operator== (const ThisClass& other) const
		{
			return mElementIndex == other.mElementIndex && mHashTable == other.mHashTable;
		}

		bool operator!= (const ThisClass& other) const
		{
			return !(operator==(other));
		}

		IVT& operator* ()
		{
			return mHashTable->mElements[mElementIndex];
		}

		IVT* operator-> ()
		{
			return &(operator*());
		}

		ThisClass& operator++ ()
		{
			if (++mElementIndex >= mHashTable->mUsed)
				mElementIndex = IHTT::NOT_FOUND;
			return *this;
		}

		ThisClass operator++ (int)
		{
			generic_iterator temp(*this);
			if (++mElementIndex >= mHashTable->mUsed)
				mElementIndex = IHTT::NOT_FOUND;
			return temp;
		}

		friend class OHashTable<KT, VT, HF>;

	private:
		generic_iterator(IndexType element_index, IHTT* hashtable) :
			mElementIndex(element_index), mHashTable(hashtable)
		{
			if (mElementIndex >= mHashTable->mUsed)
				mElementIndex = IHTT::NOT_FOUND;
		}

		IndexType	mElementIndex;
		IHTT*		mHashTable;
	};



	// ------------------------------------------------------------------------
	// OHashTable functions
	// ------------------------------------------------------------------------

	OHashTable(size_t size = 32) :
		mSize(0), mUsed(0), mHashTable(NULL), mElements(NULL)
	{
		resize(size);
	}

	OHashTable(const HashTableType& other) :
		mSize(0), mUsed(0), mHashTable(NULL), mElements(NULL) 
	{
		resize(other.mSize);
	}

	~OHashTable()
	{
		clear();
		delete [] mHashTable;
		delete [] mElements;
	}

	OHashTable& operator= (const HashTableType& other)
	{
		clear();
		resize(other.mSize);

		return *this;
	}

	bool empty() const
	{
		return mUsed == 0;
	}

	size_t size() const
	{
		return mUsed;
	}

	size_t count(const KT& key) const
	{
		return findElement(key) == NOT_FOUND ? 0 : 1;
	}

	void clear()
	{
		for (size_t i = 0; i < mSize; i++)
			mHashTable[i] = NOT_FOUND;

		for (size_t i = 0; i < mUsed; i++)
		{
			mElements[i].first = KT();
			mElements[i].second = VT();
		}

		mUsed = 0;
	}

	iterator begin()
	{
		return iterator(0, this);
	}

	const_iterator begin() const
	{
		return const_iterator(0, this);
	}

	iterator end()
	{
		return iterator(NOT_FOUND, this);
	}

	const_iterator end() const
	{
		return const_iterator(NOT_FOUND, this);
	}	

	iterator find(const KT& key)
	{
		return iterator(findElement(key), this);
	}

	const_iterator find(const KT& key) const
	{
		return const_iterator(findElement(key), this);
	}

	VT& operator[](const KT& key)
	{
		IndexType element_index = findElement(key);
		if (element_index == NOT_FOUND)
			element_index = insertElement(key, VT());	// no match so insert new pair
		return mElements[element_index].second;
	}

	std::pair<iterator, bool> insert(const HashPairType& hp)
	{
		unsigned int oldused = mUsed;	
		IndexType element_index = insertElement(hp.first, hp.second);
		return std::pair<iterator, bool>(iterator(element_index, this), mUsed > oldused);
	}

	template <typename Inputiterator>
	void insert(Inputiterator it1, Inputiterator it2)
	{
		while (it1 != it2)
		{
			insertElement(it1->first, it1->second);
			++it1;
		}
	}

	void erase(iterator it)
	{
		eraseElement(it.mElementIndex);	
	}

	size_t erase(const KT& key)
	{
		IndexType element_index = findElement(key);
		if (element_index == NOT_FOUND)
			return 0;

		eraseElement(element_index);
		return 1;
	}

	void erase(iterator it1, iterator it2)
	{
		// TODO
	}

private:
	inline IndexType hashKey(const KT& key) const
	{
		return mHashFunc(key) % mSize;
	}

	void resize(size_t newsize)
	{
		size_t oldsize = mSize;
		if (newsize == 0)
			newsize = 2;
		if (newsize > HashTableType::MAX_CAPACITY)
			newsize = HashTableType::MAX_CAPACITY;
		mSize = newsize;
		assert(mSize > oldsize);

		HashPairType* oldelements = mElements;
		mElements = new HashPairType[mSize];
		IndexType* oldhashtable = mHashTable;
		mHashTable = new IndexType[mSize];
		size_t oldused = mUsed;
		mUsed = 0;

		for (size_t i = 0; i < mSize; i++)
			mHashTable[i] = NOT_FOUND;

		for (size_t i = 0; i < oldused; i++)
			insertElement(oldelements[i].first, oldelements[i].second);

		delete [] oldelements;
		delete [] oldhashtable;
	}

	IndexType findBucket(const KT& key) const
	{
		IndexType start_index = hashKey(key);
		for (size_t i = 0; i < mSize; i++)
		{
			IndexType bucket = (start_index + i) % mSize;
			IndexType element_index = mHashTable[bucket];	

			if (element_index == NOT_FOUND || mElements[element_index].first == key)
				return bucket;
		}
		assert(false);
		return NOT_FOUND;		// only happens if table is full and key is not found
	}

	IndexType findElement(const KT& key) const
	{
		IndexType start_index = hashKey(key);
		for (size_t i = 0; i < mSize; i++)
		{
			IndexType bucket = (start_index + i) % mSize;
			IndexType element_index = mHashTable[bucket];	

			if (element_index == NOT_FOUND || mElements[element_index].first == key)
				return element_index;
		}
		assert(false);
		return NOT_FOUND;		// only happens if table is full and key is not found
	}

	IndexType insertElement(const KT& key, const VT& value)
	{
		// double the capacity if we're going to exceed 75% load
		if (4 * (mUsed + 1) > 3 * mSize)
			resize(2 * mSize + 1);

		IndexType bucket = findBucket(key);
		assert(bucket != NOT_FOUND);
		IndexType element_index = mHashTable[bucket];

		if (element_index == NOT_FOUND)
		{
			// add key and value pair
			mHashTable[bucket] = element_index = mUsed++;
			mElements[element_index].first = key;
			mElements[element_index].second = value;
		}
		else
		{
			// key already exists so just update the value
			mElements[element_index].second = value;
		}

		assert(element_index != NOT_FOUND);
		return element_index;
	}

	void rehashBucket(IndexType bucket)
	{
		IndexType element_index = mHashTable[bucket];
		const KT& key = mElements[element_index].first;
		IndexType new_bucket = findBucket(key);
		if (new_bucket != bucket)
		{
			mHashTable[bucket] = NOT_FOUND;
			mHashTable[new_bucket] = element_index;
		}
	}

	void eraseElement(IndexType element_index)
	{
		IndexType last_element_index = mUsed - 1;

		IndexType old_bucket = findBucket(mElements[element_index].first);
		assert(mHashTable[old_bucket] != NOT_FOUND);

		IndexType last_bucket = findBucket(mElements[last_element_index].first);
		assert(mHashTable[last_bucket] != NOT_FOUND);
	
		// Swap the key/value for the last element with the element being erased.
		// This keeps mElements contiguous.
		if (element_index < last_element_index)
		{
			mElements[element_index].first = mElements[last_element_index].first;
			mElements[element_index].second = mElements[last_element_index].second;
			mHashTable[last_bucket] = element_index;
		}

		// Set empty elements to their default values.
		// This ensures accuracy with objects that have reference counting.
		mElements[last_element_index].first = KT();
		mElements[last_element_index].second = VT();

		mHashTable[old_bucket] = NOT_FOUND;

		mUsed--;

		// Rehash all of the non-empty buckets that follow the erased bucket.
		IndexType bucket = (old_bucket + 1) % mSize;
		while (mHashTable[bucket] != NOT_FOUND)
		{
			rehashBucket(bucket);
			bucket = (bucket + 1) % mSize;
		}
	}

	size_t			mSize;
	size_t			mUsed;
	
	IndexType*		mHashTable;
	HashPairType*	mElements;

	HF				mHashFunc;		// hash key generation functor
};

#endif	// __HASHTABLE_H__

