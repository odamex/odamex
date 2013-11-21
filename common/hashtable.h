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
//			unsigned int operator()(KT) const;
//
// ============================================================================

// forward declaration
template <typename KT, typename VT, typename HF> class OHashTable;

// Default hash functors for integer & string types
template <typename KT>
struct hashfunc
{ };

template <> struct hashfunc<unsigned char>
{	unsigned int operator()(unsigned char val) const { return (unsigned int)val; }	};

template <> struct hashfunc<signed char>
{	unsigned int operator()(signed char val) const { return (unsigned int)val; }	};

template <> struct hashfunc<unsigned short>
{	unsigned int operator()(unsigned short val) const { return (unsigned int)val; }	};

template <> struct hashfunc<signed short>
{	unsigned int operator()(signed short val) const { return (unsigned int)val; }	};

template <> struct hashfunc<unsigned int>
{	unsigned int operator()(unsigned int val) const { return (unsigned int)val; }	};

template <> struct hashfunc<signed int>
{	unsigned int operator()(signed int val) const { return (unsigned int)val; }		};

template <> struct hashfunc<unsigned long long>
{	unsigned int operator()(unsigned long long val) const { return (unsigned int)val; }	};

template <> struct hashfunc<signed long long>
{	unsigned int operator()(signed long long val) const { return (unsigned int)val; }	};

static inline unsigned int __hash_cstring(const char* str)
{
	unsigned int val = 0;
	while (*str != 0)
		val = val * 101 + *str++;
	return val;	
}

template <> struct hashfunc<char*>
{	unsigned int operator()(const char* str) const { return __hash_cstring(str); } };

template <> struct hashfunc<const char*>
{	unsigned int operator()(const char* str) const { return __hash_cstring(str); } };

template <> struct hashfunc<std::string>
{	unsigned int operator()(const std::string& str) const { return __hash_cstring(str.c_str()); } };


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

	typedef unsigned int IndexType;
	static const unsigned int MAX_CAPACITY	= 65536;
	static const IndexType NOT_FOUND		= HashTableType::MAX_CAPACITY;

	struct Bucket
	{
		unsigned int	order;
		HashPairType	pair;
	};

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
			mBucketNum(IHTT::NOT_FOUND), mHashTable(NULL)
		{ }

		// allow implicit converstion from iterator to const_iterator
		operator ConstThisClass() const
		{
			return ConstThisClass(mBucketNum, mHashTable);
		}

		bool operator== (const ThisClass& other) const
		{
			return mBucketNum == other.mBucketNum && mHashTable == other.mHashTable;
		}

		bool operator!= (const ThisClass& other) const
		{
			return !(operator==(other));
		}

		IVT& operator* ()
		{
			return mHashTable->mElements[mBucketNum].pair;
		}

		IVT* operator-> ()
		{
			return &(operator*());
		}

		ThisClass& operator++ ()
		{
			do {
				mBucketNum++;
			} while (mBucketNum < mHashTable->mSize && mHashTable->emptyBucket(mBucketNum));
			
			if (mBucketNum >= mHashTable->mSize)
				mBucketNum = IHTT::NOT_FOUND;
			return *this;
		}

		ThisClass operator++ (int)
		{
			generic_iterator temp(*this);
			operator++();
			return temp;
		}

		friend class OHashTable<KT, VT, HF>;

	private:
		generic_iterator(IndexType bucketnum, IHTT* hashtable) :
			mBucketNum(bucketnum), mHashTable(hashtable)
		{
			if (mBucketNum >= mHashTable->mSize)
				mBucketNum = IHTT::NOT_FOUND;
		}

		IndexType	mBucketNum;
		IHTT*		mHashTable;
	};



	// ------------------------------------------------------------------------
	// OHashTable functions
	// ------------------------------------------------------------------------

	OHashTable(unsigned int size = 256) :
		mSize(0), mSizeMask(0), mUsed(0), mElements(NULL), mNextOrder(1)
	{
		resize(size);
	}

	OHashTable(const HashTableType& other) :
		mSize(0), mSizeMask(0), mUsed(0), mElements(NULL), mNextOrder(1)
	{
		copyFromOther(other);
	}

	~OHashTable()
	{
		delete [] mElements;
	}

	OHashTable& operator= (const HashTableType& other)
	{
		copyFromOther(other);
		return *this;
	}

	bool empty() const
	{
		return mUsed == 0;
	}

	unsigned int size() const
	{
		return mUsed;
	}

	unsigned int count(const KT& key) const
	{
		return emptyBucket(findBucket(key)) ? 0 : 1;
	}

	void clear()
	{
		for (unsigned int i = 0; i < mSize; i++)
			if (!emptyBucket(i))
				mElements[i].pair = HashPairType();

		for (unsigned int i = 0; i < mSize; i++)
			mElements[i].order = 0;

		mUsed = 0;
		mNextOrder = 1;
	}

	inline iterator begin()
	{
		return iterator(0, this);
	}

	inline const_iterator begin() const
	{
		return const_iterator(0, this);
	}

	inline iterator end()
	{
		return iterator(NOT_FOUND, this);
	}

	inline const_iterator end() const
	{
		return const_iterator(NOT_FOUND, this);
	}	

	inline iterator find(const KT& key)
	{
		IndexType bucketnum = findBucket(key);
		if (emptyBucket(bucketnum))
			return end();
		return iterator(bucketnum, this);
	}

	inline const_iterator find(const KT& key) const
	{
		IndexType bucketnum = findBucket(key);
		if (emptyBucket(bucketnum))
			return end();
		return const_iterator(bucketnum, this);
	}

	inline VT& operator[](const KT& key)
	{
		IndexType bucketnum = findBucket(key);
		if (emptyBucket(bucketnum))
			bucketnum = insertElement(key, VT());	// no match so insert new pair
		return mElements[bucketnum].pair.second;
	}

	std::pair<iterator, bool> insert(const HashPairType& hp)
	{
		unsigned int oldused = mUsed;	
		IndexType bucketnum = insertElement(hp.first, hp.second);
		return std::pair<iterator, bool>(iterator(bucketnum, this), mUsed > oldused);
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
		eraseBucket(it.mBucketNum);	
	}

	unsigned int erase(const KT& key)
	{
		IndexType bucketnum = findBucket(key);
		if (emptyBucket(bucketnum))
			return 0;
		eraseBucket(bucketnum);
		return 1;
	}

	void erase(iterator it1, iterator it2)
	{
		while (it1 != it2)
		{
			eraseBucket(it1.mBucketNum);
			++it1;
		}
	}

private:
	inline bool emptyBucket(IndexType bucketnum) const
	{
		return mElements[bucketnum].order == 0;
	}

	void resize(unsigned int newsize)
	{
		unsigned int oldsize = mSize;

		// ensure newsize is in a valid range
		if (newsize < 2)
			newsize = 2;
		if (newsize > HashTableType::MAX_CAPACITY)
			newsize = HashTableType::MAX_CAPACITY;

		// ensure newsize is a power of two
		// determine number of bits needed for newsize
		newsize = newsize * 2 - 1;
		int bits = 0;
		while (newsize >>= 1)
			bits++;

		mSize = 1 << bits;
		mSizeMask = mSize - 1;
		assert(mSize > oldsize);

		Bucket* oldelements = mElements;
		mElements = new Bucket[mSize];

		mUsed = 0;
		mNextOrder = 1;

		// indicate all buckets are empty
		for (unsigned int i = 0; i < mSize; i++)
			mElements[i].order = 0;

		// copy elements to new hashtable
		// TODO: go through iteration list instead
		for (unsigned int i = 0; i < oldsize; i++)
			if (oldelements[i].order)
				insertElement(oldelements[i].pair.first, oldelements[i].pair.second);

		delete [] oldelements;
	}

	void copyFromOther(const HashTableType& other)
	{
		clear();
		resize(other.mSize);
		for (int i = 0; i < mSize; i++)
		{
			mElements[i].order = other.mElements[i].order;
			mElements[i].pair = other.mElements[i].pair;
		}

		mNextOrder = other.mNextOrder;
		mUsed = other.mUsed;
	}

	inline IndexType findBucket(const KT& key) const
	{
		IndexType bucketnum = (mHashFunc(key) * 2654435761u) & mSizeMask; 

		// [SL] NOTE: this can loop infinitely if there is no match and the table is full!
		while (!emptyBucket(bucketnum) && mElements[bucketnum].pair.first != key)
			bucketnum = (bucketnum + 1) & mSizeMask;
		return bucketnum;
	}

	IndexType insertElement(const KT& key, const VT& value)
	{
		// double the capacity if we're going to exceed 75% load
		if (4 * (mUsed + 1) > 3 * mSize)
			resize(2 * mSize);

		IndexType bucketnum = findBucket(key);

		if (emptyBucket(bucketnum))
		{
			// add key and value pair
			mElements[bucketnum].order = mNextOrder++;
			mElements[bucketnum].pair.first = key;
			mElements[bucketnum].pair.second = value;
			mUsed++;
		}
		else
		{
			// key already exists so just update the value
			mElements[bucketnum].pair.second = value;
		}

		return bucketnum;
	}

	void eraseBucket(IndexType bucketnum)
	{
		mElements[bucketnum].order = 0;
		mElements[bucketnum].pair = HashPairType();
		mUsed--;

		// Rehash all of the non-empty buckets that follow the erased bucket.
		bucketnum = (bucketnum + 1) & mSizeMask;
		while (!emptyBucket(bucketnum))
		{
			const KT& key = mElements[bucketnum].pair.first;
			unsigned int order = mElements[bucketnum].order;
			mElements[bucketnum].order = 0;

			IndexType new_bucketnum = findBucket(key);
			mElements[new_bucketnum].order = order;

			if (new_bucketnum != bucketnum)
			{
				mElements[new_bucketnum].pair = mElements[bucketnum].pair;
				mElements[bucketnum].pair = HashPairType();	
			}

			bucketnum = (bucketnum + 1) & mSizeMask;
		}
	}

	unsigned int	mSize;
	unsigned int	mSizeMask;
	unsigned int	mUsed;

	Bucket*			mElements;
	unsigned int	mNextOrder;

	HF				mHashFunc;		// hash key generation functor
};

#endif	// __HASHTABLE_H__

