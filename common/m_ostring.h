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
OString operator+(const OString& lhs, const OString& rhs);
OString operator+(const OString& lhs, const std::string& rhs);
OString operator+(const std::string& lhs, const OString& rhs);
OString operator+(const OString& lhs, const char* rhs);
OString operator+(const char* lhs, const OString& rhs);
OString operator+(const OString& lhs, char rhs);
OString operator+(char lhs, const OString& rhs);
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
std::istream& operator>> (std::istream& is, OString& str);
std::ostream& operator<< (std::ostream& os, const OString& str);

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
		return mString;
	}


	// ------------------------------------------------------------------------
	// begin
	// ------------------------------------------------------------------------

	iterator begin();
	const_iterator begin() const;


	// ------------------------------------------------------------------------
	// end
	// ------------------------------------------------------------------------

	iterator end();
	const_iterator end() const;


	// ------------------------------------------------------------------------
	// rbegin
	// ------------------------------------------------------------------------

	reverse_iterator rbegin();
	const_reverse_iterator rbegin() const;


	// ------------------------------------------------------------------------
	// rend
	// ------------------------------------------------------------------------

	reverse_iterator rend();
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
	// resize
	// ------------------------------------------------------------------------

	void resize(size_t n);
	void resize(size_t n, char c);


	// ------------------------------------------------------------------------
	// capacity
	// ------------------------------------------------------------------------

	size_t capacity() const;


	// ------------------------------------------------------------------------
	// reserve
	// ------------------------------------------------------------------------

	void reserve(size_t n = 0);


	// ------------------------------------------------------------------------
	// clear
	// ------------------------------------------------------------------------

	void clear();


	// ------------------------------------------------------------------------
	// empty
	// ------------------------------------------------------------------------

	bool empty() const;


	// ------------------------------------------------------------------------
	// operator[]
	// ------------------------------------------------------------------------

	char& operator[] (size_t pos);
	const char& operator[] (size_t pos) const;


	// ------------------------------------------------------------------------
	// at 
	// ------------------------------------------------------------------------

	char& at(size_t pos);
	const char& at(size_t pos) const;
	

	// ------------------------------------------------------------------------
	// operator+= 
	// ------------------------------------------------------------------------

	OString& operator+= (const OString& other);
	OString& operator+= (const std::string& str);
	OString& operator+= (const char* s);
	OString& operator+= (char c);
	

	// ------------------------------------------------------------------------
	// append
	// ------------------------------------------------------------------------

	OString& append(const OString& other);
	OString& append(const std::string& str);
	OString& append(const char* s);
	OString& append(const char* s, size_t n);
	OString& append(size_t n, char c);

	template <typename InputIterator>
	OString& append(InputIterator first, InputIterator last);


	// ------------------------------------------------------------------------
	// push_back
	// ------------------------------------------------------------------------

	void push_back(char c);


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
	// insert
	// ------------------------------------------------------------------------

	OString& insert(size_t pos, const OString& other);
	OString& insert(size_t pos, const std::string& str);
	OString& insert(size_t pos, const char* s); 
	OString& insert(size_t pos, const char* s, size_t n);
	OString& insert(size_t pos, size_t n, char c);
	void insert(iterator p, size_t n, char c);
	iterator insert(iterator p, char c);

	template <typename InputIterator>
	void insert(iterator p, InputIterator first, InputIterator last);


	// ------------------------------------------------------------------------
	// erase
	// ------------------------------------------------------------------------

	OString& erase(size_t pos = 0, size_t len = npos);
	OString& erase(iterator p);
	iterator erase(iterator first, iterator last);


	// ------------------------------------------------------------------------
	// replace
	// ------------------------------------------------------------------------

	OString& replace(size_t pos, size_t len, const OString& other);
	OString& replace(iterator it1, iterator it2, const OString& other);
	OString& replace(size_t pos, size_t len, const std::string& str);
	OString& replace(iterator it1, iterator it2, const std::string& str);
	OString& replace(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen);
	OString& replace(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen);
	OString& replace(size_t pos, size_t len, const char* s);
	OString& replace(iterator it1, iterator it2, const char* s);
	OString& replace(size_t pos, size_t len, const char* s, size_t n);
	OString& replace(iterator it1, iterator it2, const char* s, size_t n);
	OString& replace(size_t pos, size_t len, size_t n, char c);
	OString& replace(iterator it1, iterator it2, size_t n, char c);

	template <typename InputIterator>
	OString& replace(iterator it1, iterator it2, InputIterator first, InputIterator last);


	// ------------------------------------------------------------------------
	// swap
	// ------------------------------------------------------------------------

	void swap(OString& other);
	void swap(std::string& str);


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
		storeHashValues();
		other.storeHashValues();
		if (mHashedId == other.mHashedId)
			return 0;
		return mString.compare(other.mString);
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

#endif	// __M_OSTRING_H__

