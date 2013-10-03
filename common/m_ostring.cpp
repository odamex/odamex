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

// initialize static member variables
OString::StringTable OString::mStrings(OString::MAX_STRINGS);
OString::StringLookupTable OString::mStringLookup(OString::MAX_STRINGS);
const std::string OString::mEmptyString;


// ------------------------------------------------------------------------
// OString Constructors 
// ------------------------------------------------------------------------

OString::OString()
{
	addString("");
}

OString::OString(const OString& other)
{
	addString(other);
}

OString::OString(const std::string& str)
{
	addString(str);
}

OString::OString(const OString& other, size_t pos, size_t len)
{
	addString(std::string(other.getString(), pos, len));
}

OString::OString(const std::string& str, size_t pos, size_t len)
{
	addString(std::string(str, pos, len));
}

OString::OString(const char* s)
{
	addString(s);
}

OString::OString(const char* s, size_t n)
{
	addString(std::string(s, n));
}

OString::OString(size_t n, char c)
{
	addString(std::string(n, c));
}

template <class InputIterator>
OString::OString(InputIterator first, InputIterator last)
{
	addString(std::string(first, last));
}


// ------------------------------------------------------------------------
// OString Destructor
// ------------------------------------------------------------------------

OString::~OString()
{
	removeString();
}

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

OString::const_iterator OString::begin() const
{
	return getString().begin();
}


// ------------------------------------------------------------------------
// OString::end
// ------------------------------------------------------------------------

OString::const_iterator OString::end() const
{
	return getString().end();
}


// ------------------------------------------------------------------------
// OString::rbegin
// ------------------------------------------------------------------------


OString::const_reverse_iterator OString::rbegin() const
{
	return getString().rbegin();
}


// ------------------------------------------------------------------------
// OString::rend
// ------------------------------------------------------------------------

OString::const_reverse_iterator OString::rend() const
{
	return getString().rend();
}


// ------------------------------------------------------------------------
// OString::size
// ------------------------------------------------------------------------

size_t OString::size() const
{
	return getString().size();
}


// ------------------------------------------------------------------------
// OString::length
// ------------------------------------------------------------------------

size_t OString::length() const
{
	return getString().length();
}


// ------------------------------------------------------------------------
// OString::max_size
// ------------------------------------------------------------------------

size_t OString::max_size() const
{
	return getString().max_size();
}


// ------------------------------------------------------------------------
// OString::capacity
// ------------------------------------------------------------------------

size_t OString::capacity() const
{
	return getString().capacity();
}


// ------------------------------------------------------------------------
// OString::empty
// ------------------------------------------------------------------------

bool OString::empty() const
{
	return getString().empty();
}


// ------------------------------------------------------------------------
// OString::clear
// ------------------------------------------------------------------------

void OString::clear()
{
	removeString();
	addString("");
}


// ------------------------------------------------------------------------
// OString::operator[]
// ------------------------------------------------------------------------

const char& OString::operator[] (size_t pos) const
{
	return getString().operator[] (pos);
}


// ------------------------------------------------------------------------
// OString::at 
// ------------------------------------------------------------------------

const char& OString::at(size_t pos) const
{
	return getString().at(pos);
}


// ------------------------------------------------------------------------
// OString::assign
// ------------------------------------------------------------------------

OString& OString::assign(const OString& other)
{
	removeString();
	addString(other);
	return *this;
}

OString& OString::assign(const std::string& str)
{
	removeString();
	addString(str);
	return *this;
}

OString& OString::assign(const char* s)
{
	removeString();
	addString(s);
	return *this;
}

OString& OString::assign(const char* s, size_t n)
{
	removeString();
	addString(std::string(s, n));
	return *this;
}

OString& OString::assign(size_t n, char c)
{
	removeString();
	addString(std::string(n, c));
	return *this;
}

template <typename InputIterator>
OString& OString::assign(InputIterator first, InputIterator last)
{
	removeString();
	addString(std::string(first, last));
	return *this;
}


// ------------------------------------------------------------------------
// OString::swap
// ------------------------------------------------------------------------

void OString::swap(OString& other)
{
	std::swap(mId, other.mId);
}


// ------------------------------------------------------------------------
// OString::get_allocator
// ------------------------------------------------------------------------

OString::allocator_type OString::get_allocator() const
{
	return getString().get_allocator();
}


// ------------------------------------------------------------------------
// OString::copy
// ------------------------------------------------------------------------

size_t OString::copy(char* s, size_t len, size_t pos) const
{
	return getString().copy(s, len, pos);
}


// ------------------------------------------------------------------------
// OString::find
// ------------------------------------------------------------------------

size_t OString::find(const OString& other, size_t pos) const
{
	return getString().find(other.getString(), pos);
}

size_t OString::find(const std::string& str, size_t pos) const
{
	return getString().find(str, pos);
}

size_t OString::find(const char* s, size_t pos) const
{
	return getString().find(s, pos);
}

size_t OString::find(const char* s, size_t pos, size_t n) const
{
	return getString().find(s, pos, n);
}

size_t OString::find(char c, size_t pos) const
{
	return getString().find(c, pos);
}


// ------------------------------------------------------------------------
// OString::rfind
// ------------------------------------------------------------------------

size_t OString::rfind(const OString& other, size_t pos) const
{
	return getString().rfind(other.getString(), pos);
}

size_t OString::rfind(const std::string& str, size_t pos) const
{
	return getString().rfind(str, pos);
}

size_t OString::rfind(const char* s, size_t pos) const
{
	return getString().rfind(s, pos);
}

size_t OString::rfind(const char* s, size_t pos, size_t n) const
{
	return getString().rfind(s, pos, n);
}

size_t OString::rfind(char c, size_t pos) const
{
	return getString().rfind(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_first_of
// ------------------------------------------------------------------------

size_t OString::find_first_of(const OString& other, size_t pos) const
{
	return getString().find_first_of(other.getString(), pos);
}

size_t OString::find_first_of(const std::string& str, size_t pos) const
{
	return getString().find_first_of(str, pos);
}

size_t OString::find_first_of(const char* s, size_t pos) const
{
	return getString().find_first_of(s, pos);
}

size_t OString::find_first_of(const char* s, size_t pos, size_t n) const
{
	return getString().find_first_of(s, pos, n);
}

size_t OString::find_first_of(char c, size_t pos) const
{
	return getString().find_first_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_last_of
// ------------------------------------------------------------------------

size_t OString::find_last_of(const OString& other, size_t pos) const
{
	return getString().find_last_of(other.getString(), pos);
}

size_t OString::find_last_of(const std::string& str, size_t pos) const
{
	return getString().find_last_of(str, pos);
}

size_t OString::find_last_of(const char* s, size_t pos) const
{
	return getString().find_last_of(s, pos);
}

size_t OString::find_last_of(const char* s, size_t pos, size_t n) const
{
	return getString().find_last_of(s, pos, n);
}

size_t OString::find_last_of(char c, size_t pos) const
{
	return getString().find_last_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_first_not_of
// ------------------------------------------------------------------------

size_t OString::find_first_not_of(const OString& other, size_t pos) const
{
	return getString().find_first_not_of(other.getString(), pos);
}

size_t OString::find_first_not_of(const std::string& str, size_t pos) const
{
	return getString().find_first_not_of(str, pos);
}

size_t OString::find_first_not_of(const char* s, size_t pos) const
{
	return getString().find_first_not_of(s, pos);
}

size_t OString::find_first_not_of(const char* s, size_t pos, size_t n) const
{
	return getString().find_first_not_of(s, pos, n);
}

size_t OString::find_first_not_of(char c, size_t pos) const
{
	return getString().find_first_not_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::find_last_not_of
// ------------------------------------------------------------------------

size_t OString::find_last_not_of(const OString& other, size_t pos) const
{
	return getString().find_last_not_of(other.getString(), pos);
}

size_t OString::find_last_not_of(const std::string& str, size_t pos) const
{
	return getString().find_last_not_of(str, pos);
}

size_t OString::find_last_not_of(const char* s, size_t pos) const
{
	return getString().find_last_not_of(s, pos);
}

size_t OString::find_last_not_of(const char* s, size_t pos, size_t n) const
{
	return getString().find_last_not_of(s, pos, n);
}

size_t OString::find_last_not_of(char c, size_t pos) const
{
	return getString().find_last_not_of(c, pos);
}


// ------------------------------------------------------------------------
// OString::substr
// ------------------------------------------------------------------------

OString OString::substr(size_t pos, size_t len) const
{
	return OString(getString().substr(pos, len));
}


// ------------------------------------------------------------------------
// OString::compare
// ------------------------------------------------------------------------

int OString::compare(size_t pos, size_t len, const OString& other) const
{
	return getString().compare(pos, len, other.getString());
}

int OString::compare(size_t pos, size_t len, const OString& other, size_t subpos, size_t sublen) const
{
	return getString().compare(pos, len, other.getString(), subpos, sublen);
}

int OString::compare(const std::string& str) const
{
	return getString().compare(str);
}

int OString::compare(size_t pos, size_t len, const std::string& str) const
{
	return getString().compare(pos, len, str);
}

int OString::compare(size_t pos, size_t len, const std::string& str, size_t subpos, size_t sublen) const
{
	return getString().compare(pos, len, str, subpos, sublen);
}

int OString::compare(const char* s) const
{
	return getString().compare(s);
}

int OString::compare(size_t pos, size_t len, const char* s) const
{
	return getString().compare(pos, len, s);
}

int OString::compare(size_t pos, size_t len, const char* s, size_t n) const
{
	return getString().compare(pos, len, s, n);
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
};

VERSION_CONTROL (m_ostring_cpp, "$Id:$")

