// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
// A limited replacement for std::string featuring string interning.
// When an OString is compared for equality with another OString, an integer
// hash of the string is used for extremely quick comparison rather than
// the normal strcmp style of comparison.
//
//-----------------------------------------------------------------------------

#ifndef __M_OSTRING_H__
#define __M_OSTRING_H__

#include <cstddef>
#include <string>
#include <iostream>
#include <memory>
#include <cassert>
#include <stdio.h>

#include "doomtype.h"
#include "sarray.h"
#include "hashtable.h"


class OString;

// Forward declarations for non-member functions
bool operator== (const OString& lhs, const OString& rhs);
bool operator== (const OString& lhs, const std::string& rhs);
bool operator== (const std::string& lhs, const OString& rhs);
bool operator== (const OString& lhs, const char* rhs);
bool operator== (const char* lhs, const OString& rhs);
bool operator!= (const OString& lhs, const OString& rhs);
bool operator!= (const OString& lhs, const std::string& rhs);
bool operator!= (const std::string& lhs, const OString& rhs);
bool operator!= (const OString& lhs, const char* rhs);
bool operator!= (const char* lhs, const OString& rhs);
bool operator< (const OString& lhs, const OString& rhs);
bool operator< (const OString& lhs, const std::string& rhs);
bool operator< (const std::string& lhs, const OString& rhs);
bool operator< (const OString& lhs, const char* rhs);
bool operator< (const char* lhs, const OString& rhs);
bool operator<= (const OString& lhs, const OString& rhs);
bool operator<= (const OString& lhs, const std::string& rhs);
bool operator<= (const std::string& lhs, const OString& rhs);
bool operator<= (const OString& lhs, const char* rhs);
bool operator<= (const char* lhs, const OString& rhs);
bool operator> (const OString& lhs, const OString& rhs);
bool operator> (const OString& lhs, const std::string& rhs);
bool operator> (const std::string& lhs, const OString& rhs);
bool operator> (const OString& lhs, const char* rhs);
bool operator> (const char* lhs, const OString& rhs);
bool operator>= (const OString& lhs, const OString& rhs);
bool operator>= (const OString& lhs, const std::string& rhs);
bool operator>= (const std::string& lhs, const OString& rhs);
bool operator>= (const OString& lhs, const char* rhs);
bool operator>= (const char* lhs, const OString& rhs);

namespace std {
	void swap(::OString& x, ::OString& y);
}



// ============================================================================
//
// OString
//
// ============================================================================

class OString
{
public:

	// ------------------------------------------------------------------------
	// member types
	// ------------------------------------------------------------------------

	typedef char								value_type;
	typedef std::char_traits<char>				traits_type;
	typedef std::allocator<char>				allocator_type;

	typedef char&								reference;
	typedef const char&							const_reference;
	typedef char*								pointer;
	typedef const char*							const_pointer;

	typedef ptrdiff_t							difference_type;
	typedef size_t								size_type;

	typedef std::string::const_iterator			const_iterator;
	typedef std::string::const_reverse_iterator	const_reverse_iterator;


	// ------------------------------------------------------------------------
	// constructor
	// ------------------------------------------------------------------------

	OString();
	OString(const OString& other);
	OString(const std::string& str);
	OString(const OString& other, size_t pos, size_t len = npos);
	OString(const std::string& str, size_t pos, size_t len = npos);
	OString(const char* s, size_t n = npos);
	OString(size_t n, char c);

	template <class InputIterator>
	OString(InputIterator first, InputIterator last);

	
	// ------------------------------------------------------------------------
	// destructor 
	// ------------------------------------------------------------------------

	~OString();

	// ------------------------------------------------------------------------
	// operator=
	// ------------------------------------------------------------------------

	OString& operator= (const OString& other);
	OString& operator= (const std::string& str);
	OString& operator= (const char* s);
	OString& operator= (char c);


	// ------------------------------------------------------------------------
	// operator std::string
	// ------------------------------------------------------------------------

	inline operator std::string() const
	{
		return getString();
	}


	// ------------------------------------------------------------------------
	// begin
	// ------------------------------------------------------------------------

	const_iterator begin() const;


	// ------------------------------------------------------------------------
	// end
	// ------------------------------------------------------------------------

	const_iterator end() const;


	// ------------------------------------------------------------------------
	// rbegin
	// ------------------------------------------------------------------------

	const_reverse_iterator rbegin() const;


	// ------------------------------------------------------------------------
	// rend
	// ------------------------------------------------------------------------

	const_reverse_iterator rend() const;


	// ------------------------------------------------------------------------
	// size
	// ------------------------------------------------------------------------

	size_t size() const;


	// ------------------------------------------------------------------------
	// length
	// ------------------------------------------------------------------------

	size_t length() const;


	// ------------------------------------------------------------------------
	// max_size
	// ------------------------------------------------------------------------

	size_t max_size() const;


	// ------------------------------------------------------------------------
	// capacity
	// ------------------------------------------------------------------------

	size_t capacity() const;


	// ------------------------------------------------------------------------
	// empty
	// ------------------------------------------------------------------------

	bool empty() const;


	// ------------------------------------------------------------------------
	// clear
	// ------------------------------------------------------------------------

	void clear();


	// ------------------------------------------------------------------------
	// operator[]
	// ------------------------------------------------------------------------

	const char& operator[] (size_t pos) const;


	// ------------------------------------------------------------------------
	// at 
	// ------------------------------------------------------------------------

	const char& at(size_t pos) const;
	

	// ------------------------------------------------------------------------
	// assign
	// ------------------------------------------------------------------------

	OString& assign(const OString& other)
	{
		const StringIdType old_id = mId;
		mId = mEmptyStringId;

		if (other.mId != mEmptyStringId)
		{
			StringRecord* inc_rec = &mStrings->get(other.mId);
			increaseRefCount(inc_rec);
			mId = other.mId;
		}

		if (old_id != mEmptyStringId)
		{
			StringRecord* dec_rec = &mStrings->get(old_id);
			decreaseRefCount(dec_rec);
			if (dec_rec->mRefCount == 0)
				removeString(dec_rec);
		}
		return *this;
	}

	OString& assign(const std::string& str)
	{
		return assign(str.c_str());
	}

	OString& assign(const char* s, size_t n = npos)
	{
		const StringIdType old_id = mId;
		mId = mEmptyStringId;

		if (s[0] != 0)		// not empty string
		{
			StringRecord* inc_rec = insertString(s, n);
			increaseRefCount(inc_rec);
			mId = mStrings->getId(*inc_rec);
		}

		if (old_id != mEmptyStringId)
		{
			StringRecord* dec_rec = &mStrings->get(old_id);
			decreaseRefCount(dec_rec);
			if (dec_rec->mRefCount == 0)
				removeString(dec_rec);
		}
		return *this;
	}

	OString& assign(size_t n, char c)
	{
		return assign(std::string(n, c));
	}

	template <typename InputIterator>
	OString& assign(InputIterator first, InputIterator last)
	{
		return assign(std::string(first, last));
	}


	// ------------------------------------------------------------------------
	// swap
	// ------------------------------------------------------------------------

	void swap(OString& other);


	// ------------------------------------------------------------------------
	// c_str
	// ------------------------------------------------------------------------

	inline const char* c_str() const
	{
		return getString().c_str();
	}


	// ------------------------------------------------------------------------
	// data
	// ------------------------------------------------------------------------

	inline const char* data() const
	{
		return getString().data();
	}


	// ------------------------------------------------------------------------
	// get_allocator
	// ------------------------------------------------------------------------

	allocator_type get_allocator() const;


	// ------------------------------------------------------------------------
	// copy
	// ------------------------------------------------------------------------

	size_t copy(char* s, size_t len, size_t pos = 0) const;


	// ------------------------------------------------------------------------
	// find
	// ------------------------------------------------------------------------

	size_t find(const OString& other, size_t pos = 0) const;
	size_t find(const std::string& str, size_t pos = 0) const;
	size_t find(const char* s, size_t pos = 0) const;
	size_t find(const char* s, size_t pos, size_t n) const;
	size_t find(char c, size_t pos = 0) const;


	// ------------------------------------------------------------------------
	// rfind
	// ------------------------------------------------------------------------

	size_t rfind(const OString& other, size_t pos = npos) const;
	size_t rfind(const std::string& str, size_t pos = npos) const;
	size_t rfind(const char* s, size_t pos = npos) const;
	size_t rfind(const char* s, size_t pos, size_t n) const;
	size_t rfind(char c, size_t pos = npos) const;


	// ------------------------------------------------------------------------
	// find_first_of
	// ------------------------------------------------------------------------

	size_t find_first_of(const OString& other, size_t pos = 0) const;
	size_t find_first_of(const std::string& str, size_t pos = 0) const;
	size_t find_first_of(const char* s, size_t pos = 0) const;
	size_t find_first_of(const char* s, size_t pos, size_t n) const;
	size_t find_first_of(char c, size_t pos = 0) const;


	// ------------------------------------------------------------------------
	// find_last_of
	// ------------------------------------------------------------------------

	size_t find_last_of(const OString& other, size_t pos = npos) const;
	size_t find_last_of(const std::string& str, size_t pos = npos) const;
	size_t find_last_of(const char* s, size_t pos = npos) const;
	size_t find_last_of(const char* s, size_t pos, size_t n) const;
	size_t find_last_of(char c, size_t pos = npos) const;


	// ------------------------------------------------------------------------
	// find_first_not_of
	// ------------------------------------------------------------------------

	size_t find_first_not_of(const OString& other, size_t pos = 0) const;
	size_t find_first_not_of(const std::string& str, size_t pos = 0) const;
	size_t find_first_not_of(const char* s, size_t pos = 0) const;
	size_t find_first_not_of(const char* s, size_t pos, size_t n) const;
	size_t find_first_not_of(char c, size_t pos = 0) const;


	// ------------------------------------------------------------------------
	// find_last_not_of
	// ------------------------------------------------------------------------

	size_t find_last_not_of(const OString& other, size_t pos = npos) const;
	size_t find_last_not_of(const std::string& str, size_t pos = npos) const;
	size_t find_last_not_of(const char* s, size_t pos = npos) const;
	size_t find_last_not_of(const char* s, size_t pos, size_t n) const;
	size_t find_last_not_of(char c, size_t pos = npos) const;

	// ------------------------------------------------------------------------
	// substr
	// ------------------------------------------------------------------------

	OString substr(size_t pos = 0, size_t len = npos) const;

	// ------------------------------------------------------------------------
	// compare
	// ------------------------------------------------------------------------

	inline int compare(const OString& other) const
	{
		if (mId == other.mId)
			return 0;
		return getString().compare(other.getString());
	}

	int compare(size_t pos, size_t len, const OString& other) const;
	int compare(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen) const;
	int compare(const std::string& str) const;
	int compare(size_t pos, size_t len, const std::string& str) const;
	int compare(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen) const;
	int compare(const char* s) const;
	int compare(size_t pos, size_t len, const char* s) const;
	int compare(size_t pos, size_t len, const char* s, size_t n) const;


	// ------------------------------------------------------------------------
	// equals
	// ------------------------------------------------------------------------

	inline bool equals(const OString& other) const
	{
		return mId == other.mId;
	}


	// ------------------------------------------------------------------------
	// iequals
	// ------------------------------------------------------------------------

	inline bool iequals(const OString& other) const
	{
		const char* ptr1 = getString().c_str();
		const char* ptr2 = other.getString().c_str();

		while (*ptr1 && *ptr2)
		{
			if (toupper(*ptr1) != toupper(*ptr2))
				return false;
			ptr1++;	ptr2++;
		}

		return (*ptr1 == 0 && *ptr2 == 0);
	}


	// ------------------------------------------------------------------------
	// debugging functions
	// ------------------------------------------------------------------------

	static void printStringTable();


	// ------------------------------------------------------------------------
	// member constants
	// ------------------------------------------------------------------------

	static const size_t npos = -1;
	static const size_t MAX_STRINGS = 65536;


private:
	struct StringRecord {
		StringRecord(const std::string& str = "", size_t pos = 0, size_t length = npos) :
			mString(str, pos, length), mRefCount(0)
		{ }

		std::string				mString;
		uint32_t				mRefCount;
	};


	// ------------------------------------------------------------------------
	// internal typedefs
	// ------------------------------------------------------------------------

	typedef unsigned int StringIdType;
	typedef unsigned int HashedStringType;

	typedef SArray<StringRecord> StringTable;
	typedef OHashTable<HashedStringType, StringIdType> StringLookupTable;


	// ------------------------------------------------------------------------
	// private variables
	// ------------------------------------------------------------------------

	StringIdType				mId;

	static bool					mInitialized;
	static StringTable*			mStrings;
	static StringLookupTable*	mStringLookup;
	static std::string*			mEmptyString;
	static const StringIdType	mEmptyStringId = 0;
	

	// ------------------------------------------------------------------------
	// startup / shutdown
	// ------------------------------------------------------------------------

	static void startup();
	static void shutdown();


	// ------------------------------------------------------------------------
	// hash
	//
	// Generates a 32-bit hash value from a string.
	// ------------------------------------------------------------------------

	inline static HashedStringType hash(const char* s, size_t n = npos) 
	{
		HashedStringType val = 0;
		for (; *s != 0 && n != 0; s++, n--)
			val = val * 101 + *s;
		return val;
	}


	// ------------------------------------------------------------------------
	// lookupByHash
	//
	// Checks if a string is already in the string table.
	// ------------------------------------------------------------------------

	inline StringRecord* lookupByHash(const HashedStringType hash_value) 
	{
		StringLookupTable::const_iterator lookupit = mStringLookup->find(hash_value);
		if (lookupit != mStringLookup->end())
		{
			const StringIdType id = lookupit->second;
			StringTable::iterator it = mStrings->find(id);
			if (it != mStrings->end())
				return &(*it);
		}
		return NULL;	// not_found
	}
	

	// ------------------------------------------------------------------------
	// increaseRefCount
	//
	// Increments the reference counter for the given string record;
	// ------------------------------------------------------------------------

	inline void increaseRefCount(StringRecord* rec)
	{
		assert(rec != NULL);
		assert(rec->mRefCount >= 0);
		rec->mRefCount++;
		assert(rec->mRefCount >= 1);
	}


	// ------------------------------------------------------------------------
	// decreaseRefCount
	//
	// Decrements the reference counter for the given string record;
	// ------------------------------------------------------------------------

	inline void decreaseRefCount(StringRecord* rec)
	{
		assert(rec != NULL);
		assert(rec->mRefCount >= 1);
		rec->mRefCount--;
		assert(rec->mRefCount >= 0);
	}


	// ------------------------------------------------------------------------
	// insertString
	//
	// Adds a string entry to the string table and returns a new StringRecord.
	// If the string already exists in the string table, nothing is added.
	// ------------------------------------------------------------------------

	inline StringRecord* insertString(const char* str, size_t length = npos)
	{
		assert(str != NULL);

		// is this string already in the table?
		const HashedStringType hash_value = hash(str, length);
		StringRecord* rec = lookupByHash(hash_value);

		if (!rec)
		{
			assert(mStrings->size() < OString::MAX_STRINGS);
			const StringIdType id = mStrings->insert(StringRecord(str, 0, length));
			rec = &mStrings->get(id);
			mStringLookup->insert(std::pair<HashedStringType, StringIdType>(hash_value, id));
		}	
		return rec;
	}

	
	// ------------------------------------------------------------------------
	// removeString
	//
	// Removes a string entry from the string table.
	// ------------------------------------------------------------------------

	inline void removeString(StringRecord* rec)
	{
		assert(rec != NULL);
		assert(rec->mRefCount == 0);

		const StringIdType old_id = mStrings->getId(*rec);
		const HashedStringType hash_value = hash(rec->mString.c_str());
		mStringLookup->erase(hash_value);
		rec->mString.clear();	// allow std::string to free unused strings
		mStrings->erase(old_id);
	}


	// ------------------------------------------------------------------------
	// getString
	//
	// Retrieves the string matching mId
	// ------------------------------------------------------------------------

	inline const std::string& getString() const
	{
		assert(mInitialized);
		if (mId == mEmptyStringId)
			return *mEmptyString;

		assert(mStrings->find(mId) != mStrings->end());
		return mStrings->get(mId).mString; 
	}


	// ------------------------------------------------------------------------
	// non-member friend functions
	// ------------------------------------------------------------------------

	friend struct hashfunc<OString>;
};


// ----------------------------------------------------------------------------
// hash function for OHashTable class
// ----------------------------------------------------------------------------

template <> struct hashfunc<OString>
{   size_t operator()(const OString& str) const { return str.mId; } };



// ----------------------------------------------------------------------------
// utility functions 
// ----------------------------------------------------------------------------

OString OStringToUpper(const char* s, size_t n = OString::npos);
OString OStringToUpper(const OString& str);
OString OStringToLower(const char* s, size_t n = OString::npos);
OString OStringToLower(const OString& str);

#endif	// __M_OSTRING_H__

