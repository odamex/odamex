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


#include "odamex.h"

#include "olumpname.h"


#include <stdexcept>

// private functions

void OLumpName::MakeDataPresentable()
{
	// make m_data uppercase
	for (int i = 0; i < 8; ++i)
		m_data[i] = static_cast<char>(toupper(m_data[i]));

	// ensure last char is escape character
	m_data[8] = '\0';
}

// constructors/assignment operators

OLumpName::OLumpName()
{
	memset(m_data, '\0', 9);
}

OLumpName::OLumpName(const OLumpName& other)
{
	memcpy(m_data, other.m_data, 8);
	MakeDataPresentable();
}

OLumpName::OLumpName(const char* other)
{
	strncpy(m_data, other, 8);
	MakeDataPresentable();
}

OLumpName::OLumpName(const std::string& other)
{
	strncpy(m_data, other.data(), 8);
	MakeDataPresentable();
}

OLumpName& OLumpName::operator=(const OLumpName& other)
{
	if (this != &other)
	{
		memcpy(m_data, other.m_data, 8);
		MakeDataPresentable();
	}

	return *this;
}

OLumpName& OLumpName::operator=(const char* other)
{
	strncpy(m_data, other, 8);
	MakeDataPresentable();

	return *this;
}

OLumpName& OLumpName::operator=(const std::string& other)
{
	strncpy(m_data, other.data(), 8);
	MakeDataPresentable();

	return *this;
}

// capacity

size_t OLumpName::size() const
{
	size_t i = 0;
	for (; i < 8; ++i)
	{
		if (m_data[i] == '\0')
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
	m_data[0] = '\0';
}

bool OLumpName::empty() const
{
	return m_data[0] == '\0';
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
		snprintf(buffer, 80,
				"Attempted to access OLumpName at position %lu when the size was %lu",
				pos, s);
		
		throw std::out_of_range(buffer);
	}

	return m_data[pos];
}

const char& OLumpName::at(const size_t pos) const
{
	const size_t s = size();

	if (pos > 7 || pos > s)
	{
		char buffer[80];
		snprintf(buffer, 80,
		        "Attempted to access OLumpName at position %lu when the size was %lu",
		        pos, s);

		throw std::out_of_range(buffer);
	}

	return m_data[pos];
}

char& OLumpName::operator[](const size_t pos)
{
	return m_data[pos];
}

const char& OLumpName::operator[](const size_t pos) const
{
	return m_data[pos];
}

// string operations

const char* OLumpName::c_str() const
{
	return m_data;
}

const char* OLumpName::data() const
{
	return m_data;
}

int OLumpName::compare(const OLumpName& other) const
{
	return !stricmp(m_data, other.m_data);
}

int OLumpName::compare(const char* other) const
{
	return !stricmp(m_data, other);
}

int OLumpName::compare(const std::string& other) const
{
	return !stricmp(m_data, other.data());
}

bool operator==(const OLumpName& lhs, const OLumpName& rhs)
{
	return !stricmp(lhs.m_data, rhs.m_data);
}

bool operator==(const OLumpName& lhs, const char* rhs)
{
	return !stricmp(lhs.m_data, rhs);
}

bool operator==(const OLumpName& lhs, const std::string& rhs)
{
	return !stricmp(lhs.m_data, rhs.data());
}

bool operator!=(const OLumpName& lhs, const OLumpName& rhs)
{
	return stricmp(lhs.m_data, rhs.m_data);
}

bool operator!=(const OLumpName& lhs, const char* rhs)
{
	return stricmp(lhs.m_data, rhs);
}

bool operator!=(const OLumpName& lhs, const std::string& rhs)
{
	return stricmp(lhs.m_data, rhs.data());
}
