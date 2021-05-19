// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2021 by The Odamex Team.
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
//   Wrapper class around a char[8] to make dealing with lump names quicker
//   and easier. Under most cases, the lump name will guaranteed to be completely
//   uppercase.
//
//-----------------------------------------------------------------------------

#include "olumpname.h"

#include "doomtype.h"

#include <cstring>
#include <stdexcept>

// private functions

void OLumpName::MakeDataPresentable()
{
	// make _data uppercase
	for (int i = 0; i < 8; ++i)
		_data[i] = static_cast<char>(toupper(_data[i]));

	// ensure last char is escape character
	_data[8] = '\0';
}

// constructors/assignment operators

OLumpName::OLumpName()
{
	std::memset(_data, '\0', 9);
}

OLumpName::OLumpName(const OLumpName& other)
{
	std::memcpy(_data, other._data, 8);
	MakeDataPresentable();
}

OLumpName::OLumpName(const char* other)
{
	std::strncpy(_data, other, 8);
	MakeDataPresentable();
}

OLumpName::OLumpName(const std::string& other)
{
	std::strncpy(_data, other.data(), 8);
	MakeDataPresentable();
}

OLumpName& OLumpName::operator=(const OLumpName& other)
{
	if (this != &other)
	{
		std::memcpy(_data, other._data, 8);
		MakeDataPresentable();
	}

	return *this;
}

OLumpName& OLumpName::operator=(const char* other)
{
	std::strncpy(_data, other, 8);
	MakeDataPresentable();

	return *this;
}

OLumpName& OLumpName::operator=(const std::string& other)
{
	std::strncpy(_data, other.data(), 8);
	MakeDataPresentable();

	return *this;
}

// iterators

// todo

// capacity

size_t OLumpName::size() const
{
	size_t i = 0;
	for (; i < 8; ++i)
	{
		if (_data[i] == '\0')
		{
			break;
		}
	}
	
	return i;
}

size_t OLumpName::length() const
{
	return size();
}

void OLumpName::clear()
{
	_data[0] = '\0';
}

bool OLumpName::empty() const
{
	return _data[0] == '\0';
}

// element access
//
// WARNING: If you use any of the non-const element access functions, the uppercase
//			guarantee will no longer apply! You can edit the char's to be whatever you
//			want at that point. Be careful!

char& OLumpName::at(const size_t pos)
{
	const size_t s = size();
	
	if (pos > 7 || pos > s)
	{
		char buffer[80];
		sprintf(buffer,
				"Attempted to access OLumpName at position %llu when the size was %llu",
				pos, s);
		
		throw std::out_of_range(buffer);
	}

	return _data[pos];
}

const char& OLumpName::at(const size_t pos) const
{
	const size_t s = size();

	if (pos > 7 || pos > s)
	{
		char buffer[80];
		sprintf(buffer,
		        "Attempted to access OLumpName at position %llu when the size was %llu",
		        pos, s);

		throw std::out_of_range(buffer);
	}

	return _data[pos];
}

char& OLumpName::operator[](const size_t pos)
{
	return _data[pos];
}

const char& OLumpName::operator[](const size_t pos) const
{
	return _data[pos];
}

// string operations

const char* OLumpName::c_str() const
{
	return _data;
}

const char* OLumpName::data() const
{
	return _data;
}

int OLumpName::compare(const OLumpName& other) const
{
	return !stricmp(_data, other._data);
}

int OLumpName::compare(const char* other) const
{
	return !stricmp(_data, other);
}

int OLumpName::compare(const std::string& other) const
{
	return !stricmp(_data, other.data());
}

bool operator==(const OLumpName& lhs, const OLumpName& rhs)
{
	return !stricmp(lhs._data, rhs._data);
}

bool operator==(const OLumpName& lhs, const char* rhs)
{
	return !stricmp(lhs._data, rhs);
}

bool operator==(const OLumpName& lhs, const std::string& rhs)
{
	return !stricmp(lhs._data, rhs.data());
}
