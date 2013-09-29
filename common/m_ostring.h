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
// Drop-in replacement for std::string featuring string interning.
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

class OString;

// Forward declarations for non-member friend functions
namespace std {
	void swap(::OString& x, ::OString& y);
	void swap(::OString& x, string& y);
	void swap(string& x, ::OString& y);

	istream& getline(istream& is, ::OString& str);
	istream& getline(istream& is, ::OString& str, char delim);
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

	typedef std::string::iterator				iterator;
	typedef std::string::const_iterator			const_iterator;
	typedef std::string::reverse_iterator		reverse_iterator;
	typedef std::string::const_reverse_iterator	const_reverse_iterator;


	// ------------------------------------------------------------------------
	// constructor
	// ------------------------------------------------------------------------

	OString()	:
		mDirty(true)
	{ }

	OString(const OString& other) :
		mString(other.mString), mDirty(other.mDirty),
		mHashedId(other.mHashedId), mUpperHashedId(other.mUpperHashedId)
	{ }

	OString(const std::string& str) :
		mString(str), mDirty(true) 
	{ }

	OString(const OString& other, size_t pos, size_t len = npos) :
		mString(other.mString, pos, len), mDirty(true) 
	{ }

	OString(const std::string& str, size_t pos, size_t len = npos) :
		mString(str, pos, len), mDirty(true) 
	{ }

	OString(const char* s) :
		mString(s), mDirty(true) 
	{ }

	OString(const char* s, size_t n) :
		mString(s, n), mDirty(true) 
	{ }

	OString(size_t n, char c) :
		mString(n, c), mDirty(true) 
	{ }

	template <class InputIterator>
	OString(InputIterator first, InputIterator last) :
		mString(first, last), mDirty(true)
	{ }


	// ------------------------------------------------------------------------
	// operator=
	// ------------------------------------------------------------------------

	inline OString& operator= (const OString& other)
	{
		return assign(other);
	}

	inline OString& operator= (const std::string& str)
	{
		return assign(str);
	}

	inline OString& operator= (const char* s)
	{
		return assign(s);
	}

	inline OString& operator= (char c)
	{
		return assign(1, c);
	}


	// ------------------------------------------------------------------------
	// begin
	// ------------------------------------------------------------------------

	inline iterator begin()
	{
		mDirty = true;
		return mString.begin();
	}

	inline const_iterator begin() const
	{
		return mString.begin();
	}


	// ------------------------------------------------------------------------
	// end
	// ------------------------------------------------------------------------

	inline iterator end()
	{
		return mString.end();
	}

	inline const_iterator end() const
	{
		return mString.end();
	}


	// ------------------------------------------------------------------------
	// rbegin
	// ------------------------------------------------------------------------

	inline reverse_iterator rbegin()
	{
		mDirty = true;
		return mString.rbegin();
	}

	inline const_reverse_iterator rbegin() const
	{
		return mString.rbegin();
	}


	// ------------------------------------------------------------------------
	// rend
	// ------------------------------------------------------------------------

	inline reverse_iterator rend()
	{
		return mString.rend();
	}

	inline const_reverse_iterator rend() const
	{
		return mString.rend();
	}


	// ------------------------------------------------------------------------
	// size
	// ------------------------------------------------------------------------

	inline size_t size() const
	{
		return mString.size();
	}


	// ------------------------------------------------------------------------
	// length
	// ------------------------------------------------------------------------

	inline size_t length() const
	{
		return mString.length();
	}


	// ------------------------------------------------------------------------
	// max_size
	// ------------------------------------------------------------------------

	inline size_t max_size() const
	{
		return mString.max_size();
	}


	// ------------------------------------------------------------------------
	// resize
	// ------------------------------------------------------------------------

	inline void resize(size_t n)
	{
		mString.resize(n);
		mDirty = true;
	}

	inline void resize(size_t n, char c)
	{
		mString.resize(n, c);
		mDirty = true;
	}


	// ------------------------------------------------------------------------
	// capacity
	// ------------------------------------------------------------------------

	inline size_t capacity() const
	{
		return mString.capacity();
	}


	// ------------------------------------------------------------------------
	// reserve
	// ------------------------------------------------------------------------

	inline void reserve(size_t n = 0)
	{
		mString.reserve(n);
	}


	// ------------------------------------------------------------------------
	// clear
	// ------------------------------------------------------------------------

	inline void clear()
	{
		mString.clear();
		mDirty = true;
	}


	// ------------------------------------------------------------------------
	// empty
	// ------------------------------------------------------------------------

	inline bool empty() const
	{
		return mString.empty();
	}


	// ------------------------------------------------------------------------
	// operator[]
	// ------------------------------------------------------------------------

	inline char& operator[] (size_t pos)
	{
		mDirty = true;
		return mString.operator[] (pos);
	}

	inline const char& operator[] (size_t pos) const
	{
		return mString.operator[] (pos);
	}


	// ------------------------------------------------------------------------
	// at 
	// ------------------------------------------------------------------------

	inline char& at(size_t pos)
	{
		mDirty = true;
		return mString.at(pos);
	}

	inline const char& at(size_t pos) const
	{
		return mString.at(pos);
	}
	

	// ------------------------------------------------------------------------
	// operator+= 
	// ------------------------------------------------------------------------

	inline OString& operator+= (const OString& other)
	{
		return append(other);
	}

	inline OString& operator+= (const std::string& str)
	{
		return append(str);
	}

	inline OString& operator+= (const char* s)
	{
		return append(s);
	}

	inline OString& operator+= (char c)
	{
		return append(1, c);
	}
	

	// ------------------------------------------------------------------------
	// append
	// ------------------------------------------------------------------------

	inline OString& append(const OString& other)
	{
		mDirty = true;
		mString.append(other.mString);
		return *this;
	}

	inline OString& append(const std::string& str)
	{
		mDirty = true;
		mString.append(str);
		return *this;
	}

	inline OString& append(const char* s)
	{
		mDirty = true;
		mString.append(s);
		return *this;
	}

	inline OString& append(const char* s, size_t n)
	{
		mDirty = true;
		mString.append(s, n);
		return *this;
	}
	
	inline OString& append(size_t n, char c)
	{
		mDirty = true;
		mString.append(n, c);
		return *this;
	}
	
	template <typename InputIterator>
	inline OString& append(InputIterator first, InputIterator last)
	{
		mDirty = true;
		mString.append(first, last);
		return *this;
	}


	// ------------------------------------------------------------------------
	// push_back
	// ------------------------------------------------------------------------

	inline void push_back(char c)
	{
		mDirty = true;
		mString.push_back(c);
	}


	// ------------------------------------------------------------------------
	// assign
	// ------------------------------------------------------------------------

	inline OString& assign(const OString& other)
	{
		mString.assign(other.mString);
		mDirty = other.mDirty;
		mHashedId = other.mHashedId;
		mUpperHashedId = other.mUpperHashedId;
		return *this;
	}

	inline OString& assign(const std::string& str)
	{
		mString.assign(str);
		mDirty = true;
		return *this;
	}

	inline OString& assign(const char* s)
	{
		mDirty = true;
		mString.assign(s);
		return *this;
	}

	inline OString& assign(const char* s, size_t n)
	{
		mDirty = true;
		mString.assign(s, n);
		return *this;
	}
	
	inline OString& assign(size_t n, char c)
	{
		mDirty = true;
		mString.assign(n, c);
		return *this;
	}
	
	template <typename InputIterator>
	inline OString& assign(InputIterator first, InputIterator last)
	{
		mDirty = true;
		mString.assign(first, last);
		return *this;
	}


	// ------------------------------------------------------------------------
	// insert
	// ------------------------------------------------------------------------

	inline OString& insert(size_t pos, const OString& other)
	{
		mDirty = true;
		mString.insert(pos, other.mString);
		return *this;
	}

	inline OString& insert(size_t pos, const std::string& str) 
	{
		mDirty = true;
		mString.insert(pos, str); 
		return *this;
	}

	inline OString& insert(size_t pos, const char* s) 
	{
		mDirty = true;
		mString.insert(pos, s); 
		return *this;
	}

	inline OString& insert(size_t pos, const char* s, size_t n) 
	{
		mDirty = true;
		mString.insert(pos, s, n); 
		return *this;
	}

	inline OString& insert(size_t pos, size_t n, char c) 
	{
		mDirty = true;
		mString.insert(pos, n, c);
		return *this;
	}

	inline void insert(iterator p, size_t n, char c)
	{
		mDirty = true;
		mString.insert(p, n, c);
	}

	inline iterator insert(iterator p, char c)
	{
		mDirty = true;
		return iterator(mString.insert(p, c));
	}

	template <typename InputIterator>
	inline void insert(iterator p, InputIterator first, InputIterator last)
	{
		mDirty = true;
		mString.insert(p, first, last);
	}


	// ------------------------------------------------------------------------
	// erase
	// ------------------------------------------------------------------------

	inline OString& erase(size_t pos = 0, size_t len = npos)
	{
		mDirty = true;
		mString.erase(pos, len);
		return *this;
	}

	inline OString& erase(iterator p)
	{
		mDirty = true;
		mString.erase(p);
		return *this;
	}
	
	inline iterator erase(iterator first, iterator last)
	{
		mDirty = true;
		return mString.erase(first, last);
	}


	// ------------------------------------------------------------------------
	// replace
	// ------------------------------------------------------------------------

	inline OString& replace(size_t pos, size_t len, const OString& other)
	{
		mDirty = true;
		mString.replace(pos, len, other.mString);
		return *this;
	}

	inline OString& replace(iterator it1, iterator it2, const OString& other)
	{
		mDirty = true;
		mString.replace(it1, it2, other.mString);
		return *this;
	}

	inline OString& replace(size_t pos, size_t len, const std::string& str)
	{
		mDirty = true;
		mString.replace(pos, len, str);
		return *this;
	}

	inline OString& replace(iterator it1, iterator it2, const std::string& str)
	{
		mDirty = true;
		mString.replace(it1, it2, str);
		return *this;
	}

	inline OString& replace(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen)
	{
		mDirty = true;
		mString.replace(pos, len, other.mString, subpos, sublen);
		return *this;
	}

	inline OString& replace(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen)
	{
		mDirty = true;
		mString.replace(pos, len, str, subpos, sublen);
		return *this;
	}

	inline OString& replace(size_t pos, size_t len, const char* s)
	{
		mDirty = true;
		mString.replace(pos, len, s);
		return *this;
	}

	inline OString& replace(iterator it1, iterator it2, const char* s)
	{
		mDirty = true;
		mString.replace(it1, it2, s);
		return *this;
	}

	inline OString& replace(size_t pos, size_t len, const char* s, size_t n)
	{
		mDirty = true;
		mString .replace(pos, len, s, n);
		return *this;
	}

	inline OString& replace(iterator it1, iterator it2, const char* s, size_t n)
	{
		mDirty = true;
		mString.replace(it1, it2, s, n);
		return *this;
	}

	inline OString& replace(size_t pos, size_t len, size_t n, char c)
	{
		mDirty = true;
		mString.replace(pos, len, n, c);
		return *this;
	}

	inline OString& replace(iterator it1, iterator it2, size_t n, char c)
	{
		mDirty = true;
		mString.replace(it1, it2, n, c);
		return *this;
	}

	template <typename InputIterator>
	inline OString& replace(iterator it1, iterator it2, InputIterator first, InputIterator last)
	{
		mDirty = true;
		mString.replace(it1, it2, first, last);
		return *this;
	}


	// ------------------------------------------------------------------------
	// swap
	// ------------------------------------------------------------------------

	inline void swap(OString& other)
	{
		std::swap(mDirty, other.mDirty);
		std::swap(mHashedId, other.mHashedId);
		std::swap(mUpperHashedId, other.mUpperHashedId);
		mString.swap(other.mString);
	}

	inline void swap(std::string& str)
	{
		mDirty = true;
		mString.swap(str);
	}


	// ------------------------------------------------------------------------
	// c_str
	// ------------------------------------------------------------------------

	inline const char* c_str() const
	{
		return mString.c_str();
	}


	// ------------------------------------------------------------------------
	// data
	// ------------------------------------------------------------------------

	inline const char* data() const
	{
		return mString.data();
	}


	// ------------------------------------------------------------------------
	// get_allocator
	// ------------------------------------------------------------------------

	inline allocator_type get_allocator() const
	{
		return mString.get_allocator();
	}


	// ------------------------------------------------------------------------
	// copy
	// ------------------------------------------------------------------------

	inline size_t copy(char* s, size_t len, size_t pos = 0) const
	{
		return mString.copy(s, len, pos);
	}


	// ------------------------------------------------------------------------
	// find
	// ------------------------------------------------------------------------

	inline size_t find(const OString& other, size_t pos = 0) const
	{
		return mString.find(other.mString, pos);
	}

	inline size_t find(const std::string& str, size_t pos = 0) const
	{
		return mString.find(str, pos);
	}

	inline size_t find(const char* s, size_t pos = 0) const
	{
		return mString.find(s, pos);
	}

	inline size_t find(const char* s, size_t pos, size_t n) const
	{
		return mString.find(s, pos, n);
	}

	inline size_t find(char c, size_t pos = 0) const
	{
		return mString.find(c, pos);
	}


	// ------------------------------------------------------------------------
	// rfind
	// ------------------------------------------------------------------------

	inline size_t rfind(const OString& other, size_t pos = npos) const
	{
		return mString.rfind(other.mString, pos);
	}

	inline size_t rfind(const std::string& str, size_t pos = npos) const
	{
		return mString.rfind(str, pos);
	}

	inline size_t rfind(const char* s, size_t pos = npos) const
	{
		return mString.rfind(s, pos);
	}

	inline size_t rfind(const char* s, size_t pos, size_t n) const
	{
		return mString.rfind(s, pos, n);
	}

	inline size_t rfind(char c, size_t pos = npos) const
	{
		return mString.rfind(c, pos);
	}


	// ------------------------------------------------------------------------
	// find_first_of
	// ------------------------------------------------------------------------

	inline size_t find_first_of(const OString& other, size_t pos = 0) const
	{
		return mString.find_first_of(other.mString, pos);
	}

	inline size_t find_first_of(const std::string& str, size_t pos = 0) const
	{
		return mString.find_first_of(str, pos);
	}

	inline size_t find_first_of(const char* s, size_t pos = 0) const
	{
		return mString.find_first_of(s, pos);
	}

	inline size_t find_first_of(const char* s, size_t pos, size_t n) const
	{
		return mString.find_first_of(s, pos, n);
	}

	inline size_t find_first_of(char c, size_t pos = 0) const
	{
		return mString.find_first_of(c, pos);
	}


	// ------------------------------------------------------------------------
	// find_last_of
	// ------------------------------------------------------------------------

	inline size_t find_last_of(const OString& other, size_t pos = npos) const
	{
		return mString.find_last_of(other.mString, pos);
	}

	inline size_t find_last_of(const std::string& str, size_t pos = npos) const
	{
		return mString.find_last_of(str, pos);
	}

	inline size_t find_last_of(const char* s, size_t pos = npos) const
	{
		return mString.find_last_of(s, pos);
	}

	inline size_t find_last_of(const char* s, size_t pos, size_t n) const
	{
		return mString.find_last_of(s, pos, n);
	}

	inline size_t find_last_of(char c, size_t pos = npos) const
	{
		return mString.find_last_of(c, pos);
	}


	// ------------------------------------------------------------------------
	// find_first_not_of
	// ------------------------------------------------------------------------

	inline size_t find_first_not_of(const OString& other, size_t pos = 0) const
	{
		return mString.find_first_not_of(other.mString, pos);
	}

	inline size_t find_first_not_of(const std::string& str, size_t pos = 0) const
	{
		return mString.find_first_not_of(str, pos);
	}

	inline size_t find_first_not_of(const char* s, size_t pos = 0) const
	{
		return mString.find_first_not_of(s, pos);
	}

	inline size_t find_first_not_of(const char* s, size_t pos, size_t n) const
	{
		return mString.find_first_not_of(s, pos, n);
	}

	inline size_t find_first_not_of(char c, size_t pos = 0) const
	{
		return mString.find_first_not_of(c, pos);
	}


	// ------------------------------------------------------------------------
	// find_last_not_of
	// ------------------------------------------------------------------------

	inline size_t find_last_not_of(const OString& other, size_t pos = npos) const
	{
		return mString.find_last_not_of(other.mString, pos);
	}

	inline size_t find_last_not_of(const std::string& str, size_t pos = npos) const
	{
		return mString.find_last_not_of(str, pos);
	}

	inline size_t find_last_not_of(const char* s, size_t pos = npos) const
	{
		return mString.find_last_not_of(s, pos);
	}

	inline size_t find_last_not_of(const char* s, size_t pos, size_t n) const
	{
		return mString.find_last_not_of(s, pos, n);
	}

	inline size_t find_last_not_of(char c, size_t pos = npos) const
	{
		return mString.find_last_not_of(c, pos);
	}

	// ------------------------------------------------------------------------
	// substr
	// ------------------------------------------------------------------------

	inline OString substr(size_t pos = 0, size_t len = npos) const
	{
		return OString(mString.substr(pos, len));
	}

	// ------------------------------------------------------------------------
	// compare
	// ------------------------------------------------------------------------

	inline int compare(const OString& other) const
	{
		storeHashValues();
		other.storeHashValues();
		if (mHashedId == other.mHashedId)
			return 0;
		return mString.compare(other.mString);
	}

	inline int compare(size_t pos, size_t len, const OString& other) const
	{
		return mString.compare(pos, len, other.mString);
	}

	inline int compare(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen) const
	{
		return mString.compare(pos, len, other.mString, subpos, sublen);
	}

	int compare(const std::string& str) const
	{
		return mString.compare(str);
	}

	inline int compare(size_t pos, size_t len, const std::string& str) const
	{
		return mString.compare(pos, len, str);
	}

	inline int compare(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen) const
	{
		return mString.compare(pos, len, str, subpos, sublen);
	}

	inline int compare(const char* s) const
	{
		return mString.compare(s);
	}

	inline int compare(size_t pos, size_t len, const char* s) const
	{
		return mString.compare(pos, len, s);
	}

	inline int compare(size_t pos, size_t len, const char* s, size_t n) const
	{
		return mString.compare(pos, len, s, n);
	}


	// ------------------------------------------------------------------------
	// equals
	// ------------------------------------------------------------------------

	inline bool equals(const OString& other) const
	{
		storeHashValues();
		other.storeHashValues();
		return mHashedId == other.mHashedId;
	}

	// ------------------------------------------------------------------------
	// iequals
	// ------------------------------------------------------------------------

	inline bool iequals(const OString& other) const
	{
		storeHashValues();
		other.storeHashValues();
		return mUpperHashedId == other.mUpperHashedId;
	}


	// ------------------------------------------------------------------------
	// member constants
	// ------------------------------------------------------------------------

	static const size_t npos = -1;

private:
	typedef unsigned int HashedIdType;

	inline void storeHashValues() const
	{
		if (mDirty)
		{
			const char* cstr = mString.c_str();
			for (mHashedId = mUpperHashedId = 0; *cstr != 0; cstr++)
			{
				mHashedId = mHashedId * 101 + *cstr;
				mUpperHashedId = mUpperHashedId * 101 + toupper(*cstr);
			}

			mDirty = false;
		}
	}

	std::string					mString;
	mutable bool				mDirty;

	mutable HashedIdType		mHashedId;
	mutable HashedIdType		mUpperHashedId;


	// ------------------------------------------------------------------------
	// non-member friends
	// ------------------------------------------------------------------------

	friend OString operator+(const OString& lhs, const OString& rhs);
	friend OString operator+(const OString& lhs, const std::string& rhs);
	friend OString operator+(const std::string& lhs, const OString& rhs);
	friend OString operator+(const OString& lhs, const char* rhs);
	friend OString operator+(const char* lhs, const OString& rhs);
	friend OString operator+(const OString& lhs, char rhs);
	friend OString operator+(char lhs, const OString& rhs);

	friend void std::swap(OString& x, OString& y);
	friend void std::swap(OString& x, std::string& y);
	friend void std::swap(std::string& x, OString& y);

	friend std::istream& operator>> (std::istream& is, OString& str);
	friend std::ostream& operator<< (std::ostream& os, const OString& str);

	friend std::istream& std::getline(std::istream& is, OString& str);
	friend std::istream& std::getline(std::istream& is, OString& str, char delim);
};


	// ------------------------------------------------------------------------
	// operator+
	// ------------------------------------------------------------------------

	inline OString operator+(const OString& lhs, const OString& rhs)
	{
		return OString(operator+(lhs.mString, rhs.mString));
	}

	inline OString operator+(const OString& lhs, const std::string& rhs)
	{
		return OString(operator+(lhs.mString, rhs));
	}

	inline OString operator+(const std::string& lhs, const OString& rhs)
	{
		return OString(operator+(lhs, rhs.mString));
	}

	inline OString operator+(const OString& lhs, const char* rhs)
	{
		return OString(operator+(lhs.mString, rhs));
	}

	inline OString operator+(const char* lhs, const OString& rhs)
	{
		return OString(operator+(lhs, rhs.mString));
	}

	inline OString operator+(const OString& lhs, char rhs)
	{
		return OString(operator+(lhs.mString, rhs));
	}

	inline OString operator+(char lhs, const OString& rhs)
	{
		return OString(operator+(lhs, rhs.mString));
	}


	// ------------------------------------------------------------------------
	// operator==
	// ------------------------------------------------------------------------

	inline bool operator== (const OString& lhs, const OString& rhs)
	{
		return lhs.equals(rhs);
	}

	inline bool operator== (const OString& lhs, const std::string& rhs)
	{
		return lhs.compare(rhs) == 0;
	}

	inline bool operator== (const std::string& lhs, const OString& rhs)
	{
		return rhs.compare(lhs) == 0;
	}

	inline bool operator== (const OString& lhs, const char* rhs)
	{
		return lhs.compare(rhs) == 0;
	}

	inline bool operator== (const char* lhs, const OString& rhs)
	{
		return rhs.compare(lhs) == 0;
	}


	// ------------------------------------------------------------------------
	// operator!=
	// ------------------------------------------------------------------------

	inline bool operator!= (const OString& lhs, const OString& rhs)
	{
		return !(lhs.equals(rhs));
	}

	inline bool operator!= (const OString& lhs, const std::string& rhs)
	{
		return lhs.compare(rhs) != 0;
	}

	inline bool operator!= (const std::string& lhs, const OString& rhs)
	{
		return rhs.compare(lhs) != 0;
	}

	inline bool operator!= (const OString& lhs, const char* rhs)
	{
		return lhs.compare(rhs) != 0;
	}

	inline bool operator!= (const char* lhs, const OString& rhs)
	{
		return rhs.compare(lhs) != 0;
	}


	// ------------------------------------------------------------------------
	// operator<
	// ------------------------------------------------------------------------

	inline bool operator< (const OString& lhs, const OString& rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	inline bool operator< (const OString& lhs, const std::string& rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	inline bool operator< (const std::string& lhs, const OString& rhs)
	{
		return rhs.compare(lhs) > 0;
	}

	inline bool operator< (const OString& lhs, const char* rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	inline bool operator< (const char* lhs, const OString& rhs)
	{
		return rhs.compare(lhs) > 0;
	}


	// ------------------------------------------------------------------------
	// operator<=
	// ------------------------------------------------------------------------

	inline bool operator<= (const OString& lhs, const OString& rhs)
	{
		return lhs.equals(rhs) || lhs.compare(rhs) < 0;
	}

	inline bool operator<= (const OString& lhs, const std::string& rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	inline bool operator<= (const std::string& lhs, const OString& rhs)
	{
		return rhs.compare(lhs) > 0;
	}

	inline bool operator<= (const OString& lhs, const char* rhs)
	{
		return lhs.compare(rhs) < 0;
	}

	inline bool operator<= (const char* lhs, const OString& rhs)
	{
		return rhs.compare(lhs) > 0;
	}


	// ------------------------------------------------------------------------
	// operator>
	// ------------------------------------------------------------------------

	inline bool operator> (const OString& lhs, const OString& rhs)
	{
		return lhs.compare(rhs) > 0;
	}

	inline bool operator> (const OString& lhs, const std::string& rhs)
	{
		return lhs.compare(rhs) > 0;
	}

	inline bool operator> (const std::string& lhs, const OString& rhs)
	{
		return rhs.compare(lhs) < 0;
	}

	inline bool operator> (const OString& lhs, const char* rhs)
	{
		return lhs.compare(rhs) > 0;
	}

	inline bool operator> (const char* lhs, const OString& rhs)
	{
		return rhs.compare(lhs) < 0;
	}


	// ------------------------------------------------------------------------
	// operator>=
	// ------------------------------------------------------------------------

	inline bool operator>= (const OString& lhs, const OString& rhs)
	{
		return lhs.equals(rhs) || lhs.compare(rhs) > 0;
	}

	inline bool operator>= (const OString& lhs, const std::string& rhs)
	{
		return lhs.compare(rhs) > 0;
	}

	inline bool operator>= (const std::string& lhs, const OString& rhs)
	{
		return rhs.compare(lhs) < 0;
	}

	inline bool operator>= (const OString& lhs, const char* rhs)
	{
		return lhs.compare(rhs) > 0;
	}

	inline bool operator>= (const char* lhs, const OString& rhs)
	{
		return rhs.compare(lhs) < 0;
	}

	// ------------------------------------------------------------------------
	// swap 
	// ------------------------------------------------------------------------

namespace std {
	inline void swap(::OString& x, ::OString& y)
	{
		x.swap(y);
	}

	inline void swap(::OString& x, string& y)
	{
		x.swap(y);
	}

	inline void swap(string& x, ::OString& y)
	{
		y.mDirty = true;
		x.swap(y.mString);
	}
};

	// ------------------------------------------------------------------------
	// operator>> 
	// ------------------------------------------------------------------------

	inline std::istream& operator>> (std::istream& is, ::OString& str)
	{
		str.mDirty = true;
		return std::operator>> (is, str.mString);
	}


	// ------------------------------------------------------------------------
	// operator<< 
	// ------------------------------------------------------------------------

	inline std::ostream& operator<< (std::ostream& os, const ::OString& str)
	{
		return std::operator<< (os, str.mString);
	}


	// ------------------------------------------------------------------------
	// getline
	// ------------------------------------------------------------------------

namespace std {
	inline istream& getline(istream& is, ::OString& str)
	{
		str.mDirty = true;
		return getline(is, str.mString);
	}

	inline istream& getline(istream& is, ::OString& str, char delim)
	{
		str.mDirty = true;
		return getline(is, str.mString, delim);
	}
};

#endif	// __M_OSTRING_H__

