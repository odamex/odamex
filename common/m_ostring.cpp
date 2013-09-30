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

#include "doomtype.h"
#include "m_ostring.h"

#include <cstddef>
#include <string>
#include <iostream>

// ------------------------------------------------------------------------
// OString Constructors 
// ------------------------------------------------------------------------

OString::OString()	:
	mDirty(true)
{ }

OString::OString(const OString& other) :
	mString(other.mString), mDirty(other.mDirty),
	mHashedId(other.mHashedId), mUpperHashedId(other.mUpperHashedId)
{ }

OString::OString(const std::string& str) :
	mString(str), mDirty(true) 
{ }

OString::OString(const OString& other, size_t pos, size_t len) :
	mString(other.mString, pos, len), mDirty(true) 
{ }

OString::OString(const std::string& str, size_t pos, size_t len) :
	mString(str, pos, len), mDirty(true) 
{ }

OString::OString(const char* s) :
	mString(s), mDirty(true) 
{ }

OString::OString(const char* s, size_t n) :
	mString(s, n), mDirty(true) 
{ }

OString::OString(size_t n, char c) :
	mString(n, c), mDirty(true) 
{ }

template <class InputIterator>
OString::OString(InputIterator first, InputIterator last) :
	mString(first, last), mDirty(true)
{ }


// ------------------------------------------------------------------------
// OString::operator=
// ------------------------------------------------------------------------

OString& OString::operator= (const OString& other)
{
	return assign(other);
}

OString& OString::operator= (const std::string& str)
{
	return assign(str);
}

OString& OString::operator= (const char* s)
{
	return assign(s);
}

OString& OString::operator= (char c)
{
	return assign(1, c);
}


// ------------------------------------------------------------------------
// OString::begin
// ------------------------------------------------------------------------

OString::iterator OString::begin()
{
	mDirty = true;
	return mString.begin();
}

OString::const_iterator OString::begin() const
{
	return mString.begin();
}


// ------------------------------------------------------------------------
// OString::end
// ------------------------------------------------------------------------

OString::iterator OString::end()
{
	return mString.end();
}

OString::const_iterator OString::end() const
{
	return mString.end();
}


// ------------------------------------------------------------------------
// OString::rbegin
// ------------------------------------------------------------------------

OString::reverse_iterator OString::rbegin()
{
	mDirty = true;
	return mString.rbegin();
}

OString::const_reverse_iterator OString::rbegin() const
{
	return mString.rbegin();
}


// ------------------------------------------------------------------------
// OString::rend
// ------------------------------------------------------------------------

OString::reverse_iterator OString::rend()
{
	return mString.rend();
}

OString::const_reverse_iterator OString::rend() const
{
	return mString.rend();
}


// ------------------------------------------------------------------------
// OString::size
// ------------------------------------------------------------------------

size_t OString::size() const
{
	return mString.size();
}


// ------------------------------------------------------------------------
// OString::length
// ------------------------------------------------------------------------

size_t OString::length() const
{
	return mString.length();
}


// ------------------------------------------------------------------------
// OString::max_size
// ------------------------------------------------------------------------

size_t OString::max_size() const
{
	return mString.max_size();
}


// ------------------------------------------------------------------------
// OString::resize
// ------------------------------------------------------------------------

void OString::resize(size_t n)
{
	mString.resize(n);
	mDirty = true;
}

void OString::resize(size_t n, char c)
{
	mString.resize(n, c);
	mDirty = true;
}


// ------------------------------------------------------------------------
// OString::capacity
// ------------------------------------------------------------------------

size_t OString::capacity() const
{
	return mString.capacity();
}


// ------------------------------------------------------------------------
// OString::reserve
// ------------------------------------------------------------------------

void OString::reserve(size_t n)
{
	mString.reserve(n);
}


// ------------------------------------------------------------------------
// OString::clear
// ------------------------------------------------------------------------

void OString::clear()
{
	mString.clear();
	mDirty = true;
}


// ------------------------------------------------------------------------
// OString::empty
// ------------------------------------------------------------------------

bool OString::empty() const
{
	return mString.empty();
}


// ------------------------------------------------------------------------
// OString::operator[]
// ------------------------------------------------------------------------

char& OString::operator[] (size_t pos)
{
	mDirty = true;
	return mString.operator[] (pos);
}

const char& OString::operator[] (size_t pos) const
{
	return mString.operator[] (pos);
}


// ------------------------------------------------------------------------
// OString::at 
// ------------------------------------------------------------------------

char& OString::at(size_t pos)
{
	mDirty = true;
	return mString.at(pos);
}

const char& OString::at(size_t pos) const
{
	return mString.at(pos);
}


// ------------------------------------------------------------------------
// OString::operator+= 
// ------------------------------------------------------------------------

OString& OString::operator+= (const OString& other)
{
	return append(other);
}

OString& OString::operator+= (const std::string& str)
{
	return append(str);
}

OString& OString::operator+= (const char* s)
{
	return append(s);
}

OString& OString::operator+= (char c)
{
	return append(1, c);
}

// ------------------------------------------------------------------------
// OString::append
// ------------------------------------------------------------------------

OString& OString::append(const OString& other)
{
	mDirty = true;
	mString.append(other.mString);
	return *this;
}

OString& OString::append(const std::string& str)
{
	mDirty = true;
	mString.append(str);
	return *this;
}

OString& OString::append(const char* s)
{
	mDirty = true;
	mString.append(s);
	return *this;
}

OString& OString::append(const char* s, size_t n)
{
	mDirty = true;
	mString.append(s, n);
	return *this;
}

OString& OString::append(size_t n, char c)
{
	mDirty = true;
	mString.append(n, c);
	return *this;
}

template <typename InputIterator>
OString& OString::append(InputIterator first, InputIterator last)
{
	mDirty = true;
	mString.append(first, last);
	return *this;
}


// ------------------------------------------------------------------------
// push_back
// ------------------------------------------------------------------------

void OString::push_back(char c)
{
	mDirty = true;
	mString.push_back(c);
}


// ------------------------------------------------------------------------
// OString::assign
// ------------------------------------------------------------------------

OString& OString::assign(const OString& other)
{
	mString.assign(other.mString);
	mDirty = other.mDirty;
	mHashedId = other.mHashedId;
	mUpperHashedId = other.mUpperHashedId;
	return *this;
}

OString& OString::assign(const std::string& str)
{
	mString.assign(str);
	mDirty = true;
	return *this;
}

OString& OString::assign(const char* s)
{
	mDirty = true;
	mString.assign(s);
	return *this;
}

OString& OString::assign(const char* s, size_t n)
{
	mDirty = true;
	mString.assign(s, n);
	return *this;
}

OString& OString::assign(size_t n, char c)
{
	mDirty = true;
	mString.assign(n, c);
	return *this;
}

template <typename InputIterator>
OString& OString::assign(InputIterator first, InputIterator last)
{
	mDirty = true;
	mString.assign(first, last);
	return *this;
}


// ------------------------------------------------------------------------
// OString::insert
// ------------------------------------------------------------------------

OString& OString::insert(size_t pos, const OString& other)
{
	mDirty = true;
	mString.insert(pos, other.mString);
	return *this;
}

OString& OString::insert(size_t pos, const std::string& str) 
{
	mDirty = true;
	mString.insert(pos, str); 
	return *this;
}

OString& OString::insert(size_t pos, const char* s) 
{
	mDirty = true;
	mString.insert(pos, s); 
	return *this;
}

OString& OString::insert(size_t pos, const char* s, size_t n) 
{
	mDirty = true;
	mString.insert(pos, s, n); 
	return *this;
}

OString& OString::insert(size_t pos, size_t n, char c) 
{
	mDirty = true;
	mString.insert(pos, n, c);
	return *this;
}

void OString::insert(iterator p, size_t n, char c)
{
	mDirty = true;
	mString.insert(p, n, c);
}

OString::iterator OString::insert(iterator p, char c)
{
	mDirty = true;
	return iterator(mString.insert(p, c));
}

template <typename InputIterator>
void OString::insert(iterator p, InputIterator first, InputIterator last)
{
	mDirty = true;
	mString.insert(p, first, last);
}


// ------------------------------------------------------------------------
// OString::erase
// ------------------------------------------------------------------------

OString& OString::erase(size_t pos, size_t len)
{
	mDirty = true;
	mString.erase(pos, len);
	return *this;
}

OString& OString::erase(OString::iterator p)
{
	mDirty = true;
	mString.erase(p);
	return *this;
}

OString::iterator OString::erase(OString::iterator first, OString::iterator last)
{
	mDirty = true;
	return mString.erase(first, last);
}


// ------------------------------------------------------------------------
// OString::replace
// ------------------------------------------------------------------------

OString& OString::replace(size_t pos, size_t len, const OString& other)
{
	mDirty = true;
	mString.replace(pos, len, other.mString);
	return *this;
}

OString& OString::replace(OString::iterator it1, OString::iterator it2, const OString& other)
{
	mDirty = true;
	mString.replace(it1, it2, other.mString);
	return *this;
}

OString& OString::replace(size_t pos, size_t len, const std::string& str)
{
	mDirty = true;
	mString.replace(pos, len, str);
	return *this;
}

OString& OString::replace(OString::iterator it1, OString::iterator it2, const std::string& str)
{
	mDirty = true;
	mString.replace(it1, it2, str);
	return *this;
}

OString& OString::replace(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen)
{
	mDirty = true;
	mString.replace(pos, len, other.mString, subpos, sublen);
	return *this;
}

OString& OString::replace(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen)
{
	mDirty = true;
	mString.replace(pos, len, str, subpos, sublen);
	return *this;
}

OString& OString::replace(size_t pos, size_t len, const char* s)
{
	mDirty = true;
	mString.replace(pos, len, s);
	return *this;
}

OString& OString::replace(OString::iterator it1, OString::iterator it2, const char* s)
{
	mDirty = true;
	mString.replace(it1, it2, s);
	return *this;
}

OString& OString::replace(size_t pos, size_t len, const char* s, size_t n)
{
	mDirty = true;
	mString .replace(pos, len, s, n);
	return *this;
}

OString& OString::replace(OString::iterator it1, OString::iterator it2, const char* s, size_t n)
{
	mDirty = true;
	mString.replace(it1, it2, s, n);
	return *this;
}

OString& OString::replace(size_t pos, size_t len, size_t n, char c)
{
	mDirty = true;
	mString.replace(pos, len, n, c);
	return *this;
}

OString& OString::replace(OString::iterator it1, OString::iterator it2, size_t n, char c)
{
	mDirty = true;
	mString.replace(it1, it2, n, c);
	return *this;
}

template <typename InputIterator>
OString& OString::replace(OString::iterator it1, OString::iterator it2, InputIterator first, InputIterator last)
{
	mDirty = true;
	mString.replace(it1, it2, first, last);
	return *this;
}


// ------------------------------------------------------------------------
// OString::swap
// ------------------------------------------------------------------------

void OString::swap(OString& other)
{
	std::swap(mDirty, other.mDirty);
	std::swap(mHashedId, other.mHashedId);
	std::swap(mUpperHashedId, other.mUpperHashedId);
	mString.swap(other.mString);
}

void OString::swap(std::string& str)
{
	mDirty = true;
	mString.swap(str);
}


// ------------------------------------------------------------------------
// OString::get_allocator
// ------------------------------------------------------------------------

OString::allocator_type OString::get_allocator() const
{
	return mString.get_allocator();
}


// ------------------------------------------------------------------------
// OString::copy
// ------------------------------------------------------------------------

size_t OString::copy(char* s, size_t len, size_t pos) const
{
	return mString.copy(s, len, pos);
}


// ------------------------------------------------------------------------
// OString::find
// ------------------------------------------------------------------------

size_t OString::find(const OString& other, size_t pos) const
{
	return mString.find(other.mString, pos);
}

size_t OString::find(const std::string& str, size_t pos) const
{
	return mString.find(str, pos);
}

size_t OString::find(const char* s, size_t pos) const
{
	return mString.find(s, pos);
}

size_t OString::find(const char* s, size_t pos, size_t n) const
{
	return mString.find(s, pos, n);
}

size_t OString::find(char c, size_t pos) const
{
	return mString.find(c, pos);
}


// ------------------------------------------------------------------------
// OString::rfind
// ------------------------------------------------------------------------

size_t OString::rfind(const OString& other, size_t pos) const
{
	return mString.rfind(other.mString, pos);
}

size_t OString::rfind(const std::string& str, size_t pos) const
{
	return mString.rfind(str, pos);
}

size_t OString::rfind(const char* s, size_t pos) const
{
	return mString.rfind(s, pos);
}

size_t OString::rfind(const char* s, size_t pos, size_t n) const
{
	return mString.rfind(s, pos, n);
}

size_t OString::rfind(char c, size_t pos) const
{
	return mString.rfind(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_first_of
// ------------------------------------------------------------------------

size_t OString::find_first_of(const OString& other, size_t pos) const
{
	return mString.find_first_of(other.mString, pos);
}

size_t OString::find_first_of(const std::string& str, size_t pos) const
{
	return mString.find_first_of(str, pos);
}

size_t OString::find_first_of(const char* s, size_t pos) const
{
	return mString.find_first_of(s, pos);
}

size_t OString::find_first_of(const char* s, size_t pos, size_t n) const
{
	return mString.find_first_of(s, pos, n);
}

size_t OString::find_first_of(char c, size_t pos) const
{
	return mString.find_first_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_last_of
// ------------------------------------------------------------------------

size_t OString::find_last_of(const OString& other, size_t pos) const
{
	return mString.find_last_of(other.mString, pos);
}

size_t OString::find_last_of(const std::string& str, size_t pos) const
{
	return mString.find_last_of(str, pos);
}

size_t OString::find_last_of(const char* s, size_t pos) const
{
	return mString.find_last_of(s, pos);
}

size_t OString::find_last_of(const char* s, size_t pos, size_t n) const
{
	return mString.find_last_of(s, pos, n);
}

size_t OString::find_last_of(char c, size_t pos) const
{
	return mString.find_last_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_first_not_of
// ------------------------------------------------------------------------

size_t OString::find_first_not_of(const OString& other, size_t pos) const
{
	return mString.find_first_not_of(other.mString, pos);
}

size_t OString::find_first_not_of(const std::string& str, size_t pos) const
{
	return mString.find_first_not_of(str, pos);
}

size_t OString::find_first_not_of(const char* s, size_t pos) const
{
	return mString.find_first_not_of(s, pos);
}

size_t OString::find_first_not_of(const char* s, size_t pos, size_t n) const
{
	return mString.find_first_not_of(s, pos, n);
}

size_t OString::find_first_not_of(char c, size_t pos) const
{
	return mString.find_first_not_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_last_not_of
// ------------------------------------------------------------------------

size_t OString::find_last_not_of(const OString& other, size_t pos) const
{
	return mString.find_last_not_of(other.mString, pos);
}

size_t OString::find_last_not_of(const std::string& str, size_t pos) const
{
	return mString.find_last_not_of(str, pos);
}

size_t OString::find_last_not_of(const char* s, size_t pos) const
{
	return mString.find_last_not_of(s, pos);
}

size_t OString::find_last_not_of(const char* s, size_t pos, size_t n) const
{
	return mString.find_last_not_of(s, pos, n);
}

size_t OString::find_last_not_of(char c, size_t pos) const
{
	return mString.find_last_not_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::substr
// ------------------------------------------------------------------------

OString OString::substr(size_t pos, size_t len) const
{
	return OString(mString.substr(pos, len));
}


// ------------------------------------------------------------------------
// OString::compare
// ------------------------------------------------------------------------

int OString::compare(size_t pos, size_t len, const OString& other) const
{
	return mString.compare(pos, len, other.mString);
}

int OString::compare(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen) const
{
	return mString.compare(pos, len, other.mString, subpos, sublen);
}

int OString::compare(const std::string& str) const
{
	return mString.compare(str);
}

int OString::compare(size_t pos, size_t len, const std::string& str) const
{
	return mString.compare(pos, len, str);
}

int OString::compare(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen) const
{
	return mString.compare(pos, len, str, subpos, sublen);
}

int OString::compare(const char* s) const
{
	return mString.compare(s);
}

int OString::compare(size_t pos, size_t len, const char* s) const
{
	return mString.compare(pos, len, s);
}

int OString::compare(size_t pos, size_t len, const char* s, size_t n) const
{
	return mString.compare(pos, len, s, n);
}


// ------------------------------------------------------------------------
// operator+
// ------------------------------------------------------------------------

OString operator+(const OString& lhs, const OString& rhs)
{
	return OString(operator+(lhs.mString, rhs.mString));
}

OString operator+(const OString& lhs, const std::string& rhs)
{
	return OString(operator+(lhs.mString, rhs));
}

OString operator+(const std::string& lhs, const OString& rhs)
{
	return OString(operator+(lhs, rhs.mString));
}

OString operator+(const OString& lhs, const char* rhs)
{
	return OString(operator+(lhs.mString, rhs));
}

OString operator+(const char* lhs, const OString& rhs)
{
	return OString(operator+(lhs, rhs.mString));
}

OString operator+(const OString& lhs, char rhs)
{
	return OString(operator+(lhs.mString, rhs));
}

OString operator+(char lhs, const OString& rhs)
{
	return OString(operator+(lhs, rhs.mString));
}

// ------------------------------------------------------------------------
// operator==
// ------------------------------------------------------------------------

bool operator== (const OString& lhs, const OString& rhs)
{
	return lhs.equals(rhs);
}

bool operator== (const OString& lhs, const std::string& rhs)
{
	return lhs.compare(rhs) == 0;
}

bool operator== (const std::string& lhs, const OString& rhs)
{
	return rhs.compare(lhs) == 0;
}

bool operator== (const OString& lhs, const char* rhs)
{
	return lhs.compare(rhs) == 0;
}

bool operator== (const char* lhs, const OString& rhs)
{
	return rhs.compare(lhs) == 0;
}


// ------------------------------------------------------------------------
// operator!=
// ------------------------------------------------------------------------

bool operator!= (const OString& lhs, const OString& rhs)
{
	return !(lhs.equals(rhs));
}

bool operator!= (const OString& lhs, const std::string& rhs)
{
	return lhs.compare(rhs) != 0;
}

bool operator!= (const std::string& lhs, const OString& rhs)
{
	return rhs.compare(lhs) != 0;
}

bool operator!= (const OString& lhs, const char* rhs)
{
	return lhs.compare(rhs) != 0;
}

bool operator!= (const char* lhs, const OString& rhs)
{
	return rhs.compare(lhs) != 0;
}


// ------------------------------------------------------------------------
// operator<
// ------------------------------------------------------------------------

bool operator< (const OString& lhs, const OString& rhs)
{
	return lhs.compare(rhs) < 0;
}

bool operator< (const OString& lhs, const std::string& rhs)
{
	return lhs.compare(rhs) < 0;
}

bool operator< (const std::string& lhs, const OString& rhs)
{
	return rhs.compare(lhs) > 0;
}

bool operator< (const OString& lhs, const char* rhs)
{
	return lhs.compare(rhs) < 0;
}

bool operator< (const char* lhs, const OString& rhs)
{
	return rhs.compare(lhs) > 0;
}


// ------------------------------------------------------------------------
// operator<=
// ------------------------------------------------------------------------

bool operator<= (const OString& lhs, const OString& rhs)
{
	return lhs.equals(rhs) || lhs.compare(rhs) < 0;
}

bool operator<= (const OString& lhs, const std::string& rhs)
{
	return lhs.compare(rhs) < 0;
}

bool operator<= (const std::string& lhs, const OString& rhs)
{
	return rhs.compare(lhs) > 0;
}

bool operator<= (const OString& lhs, const char* rhs)
{
	return lhs.compare(rhs) < 0;
}

bool operator<= (const char* lhs, const OString& rhs)
{
	return rhs.compare(lhs) > 0;
}


// ------------------------------------------------------------------------
// operator>
// ------------------------------------------------------------------------

bool operator> (const OString& lhs, const OString& rhs)
{
	return lhs.compare(rhs) > 0;
}

bool operator> (const OString& lhs, const std::string& rhs)
{
	return lhs.compare(rhs) > 0;
}

bool operator> (const std::string& lhs, const OString& rhs)
{
	return rhs.compare(lhs) < 0;
}

bool operator> (const OString& lhs, const char* rhs)
{
	return lhs.compare(rhs) > 0;
}

bool operator> (const char* lhs, const OString& rhs)
{
	return rhs.compare(lhs) < 0;
}


// ------------------------------------------------------------------------
// operator>=
// ------------------------------------------------------------------------

bool operator>= (const OString& lhs, const OString& rhs)
{
	return lhs.equals(rhs) || lhs.compare(rhs) > 0;
}

bool operator>= (const OString& lhs, const std::string& rhs)
{
	return lhs.compare(rhs) > 0;
}

bool operator>= (const std::string& lhs, const OString& rhs)
{
	return rhs.compare(lhs) < 0;
}

bool operator>= (const OString& lhs, const char* rhs)
{
	return lhs.compare(rhs) > 0;
}

bool operator>= (const char* lhs, const OString& rhs)
{
	return rhs.compare(lhs) < 0;
}

// ------------------------------------------------------------------------
// swap 
// ------------------------------------------------------------------------

namespace std {
void swap(::OString& x, ::OString& y)
{
	x.swap(y);
}

void swap(::OString& x, string& y)
{
	x.swap(y);
}

void swap(string& x, ::OString& y)
{
	y.mDirty = true;
	x.swap(y.mString);
}
};

// ------------------------------------------------------------------------
// operator>> 
// ------------------------------------------------------------------------

std::istream& operator>> (std::istream& is, ::OString& str)
{
	str.mDirty = true;
	return std::operator>> (is, str.mString);
}


// ------------------------------------------------------------------------
// operator<< 
// ------------------------------------------------------------------------

std::ostream& operator<< (std::ostream& os, const ::OString& str)
{
	return std::operator<< (os, str.mString);
}


// ------------------------------------------------------------------------
// getline
// ------------------------------------------------------------------------

namespace std {
istream& getline(istream& is, ::OString& str)
{
	str.mDirty = true;
	return getline(is, str.mString);
}

istream& getline(istream& is, ::OString& str, char delim)
{
	str.mDirty = true;
	return getline(is, str.mString, delim);
}
};

VERSION_CONTROL (m_ostring_cpp, "$Id:$")

