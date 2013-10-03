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
};



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
	OString(const char* s);
	OString(const char* s, size_t n);
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

	OString& assign(const OString& other);
	OString& assign(const std::string& str);
	OString& assign(const char* s);
	OString& assign(const char* s, size_t n);
	OString& assign(size_t n, char c);

	template <typename InputIterator>
	OString& assign(InputIterator first, InputIterator last);


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
		// TODO: actually do case insensitive comparison
		return mId == other.mId; 
	}


	// ------------------------------------------------------------------------
	// member constants
	// ------------------------------------------------------------------------

	static const size_t npos = -1;
	static const size_t MAX_STRINGS = 8192;


private:
	struct StringRecord {
		StringRecord(const std::string& str = "") :
			mString(str), mRefCount(0)
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
	typedef HashTable<HashedStringType, StringIdType> StringLookupTable;


	// ------------------------------------------------------------------------
	// private variables
	// ------------------------------------------------------------------------

	StringIdType				mId;
	static StringTable			mStrings;
	static StringLookupTable	mStringLookup;
	static const std::string	mEmptyString;
	static const StringIdType	mEmptyStringId = 0;
	

	// ------------------------------------------------------------------------
	// hash
	//
	// Generates a 32-bit hash value from a string.
	// ------------------------------------------------------------------------

	inline HashedStringType hash(const std::string& str) const
	{
		const char* cstr = str.c_str();
		HashedStringType val = 0;
		for (; *cstr != 0; cstr++)
			val = val * 101 + *cstr;
		return val;
	}


	// ------------------------------------------------------------------------
	// lookupByString
	//
	// Checks if a string is already in the string table.
	// ------------------------------------------------------------------------

	inline StringRecord* lookupByString(const std::string& str)
	{
		HashedStringType hash_value = hash(str);
		StringLookupTable::const_iterator lookupit = mStringLookup.find(hash_value);
		if (lookupit != mStringLookup.end())
		{
			StringIdType id = lookupit->second;
			StringTable::iterator it = mStrings.find(id);
			if (it != mStrings.end())
			{
				assert(it->mString.compare(str) == 0);		// verify match
				return &(*it);
			}
		}
		return NULL;	// not_found
	}


	// ------------------------------------------------------------------------
	// addString
	//
	// Adds a string entry to the string table.
	// ------------------------------------------------------------------------

	inline StringIdType addString(const std::string& str)
	{
		if (str.empty())
			return mEmptyStringId;

		// is this string already in the table?
		StringRecord* rec = lookupByString(str);
		if (rec)
		{
			rec->mRefCount++;
			return mStrings.getId(*rec);
		}
		else
		{
			StringIdType id = mStrings.insert(StringRecord(str));
			rec = &(mStrings.get(id));
			rec->mRefCount++;
			return id;
		}	
	}

	inline StringIdType addString(const OString& other)
	{
		if (other.mId != mEmptyStringId)
			mStrings.get(other.mId).mRefCount++;
		return other.mId;
	}

	
	// ------------------------------------------------------------------------
	// removeString
	//
	// Removes a string entry from the string table.
	// ------------------------------------------------------------------------

	inline void removeString(StringIdType id)
	{
		if (id == mEmptyStringId)
			return;

		StringTable::iterator it = mStrings.find(id);
		if (it != mStrings.end())
		{
			StringRecord& rec = *it;
			if (--rec.mRefCount == 0)
			{
				HashedStringType hash_value = hash(rec.mString);
				mStringLookup.erase(hash_value);
				mStrings.erase(id);	
			}
		}
	}


	// ------------------------------------------------------------------------
	// changeString
	//
	// Inserts a new string and remove the existing one.
	// ------------------------------------------------------------------------

	inline StringIdType changeString(const std::string& newstr, StringIdType oldid)
	{
		StringIdType newid = addString(newstr);
		removeString(oldid);
		return newid;
	}


	inline StringIdType changeString(const OString& other, StringIdType oldid)
	{
		StringIdType newid = addString(other);
		removeString(oldid);
		return newid;
	}


	// ------------------------------------------------------------------------
	// getString
	//
	// Retrieves the string matching mId
	// ------------------------------------------------------------------------

	inline const std::string& getString() const
	{
		if (mId == mEmptyStringId)
			return mEmptyString;

		assert(mStrings.find(mId) != mStrings.end());
		return mStrings.get(mId).mString; 
	}


	// ------------------------------------------------------------------------
	// non-member friend functions
	// ------------------------------------------------------------------------

	friend struct hashfunc<OString>;
};


// ----------------------------------------------------------------------------
// hash function for HashTable class
// ----------------------------------------------------------------------------

template <> struct hashfunc<OString>
{   size_t operator()(const OString& str) const { return str.mId; } };


#endif	// __M_OSTRING_H__

