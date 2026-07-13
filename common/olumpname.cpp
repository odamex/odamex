// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   Wrapper class around a string to make dealing with lump names quicker
//   and easier. The lump name is guaranteed to be completely uppercase.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "olumpname.h"


#include <stdexcept>

// The minimum backing length: room for a classic 8-character lump name plus
// a NUL terminator. Legacy callers read elements [0..8] of short names and
// serialize a fixed 8 bytes of c_str(), so short names stay padded to this.
static const size_t MIN_BACKING_LENGTH = 9;

// private functions

//
// OLumpName::assign
//
// Copies up to max_len characters (stopping at any NUL), uppercases them,
// and pads the backing string so that at least MIN_BACKING_LENGTH bytes are
// always addressable.
//
void OLumpName::assign(const char* str, size_t max_len)
{
	m_data.clear();

	if (str)
	{
		for (size_t i = 0; i < max_len && str[i] != '\0'; i++)
			m_data += static_cast<char>(toupper(str[i]));
	}

	if (m_data.length() < MIN_BACKING_LENGTH)
		m_data.resize(MIN_BACKING_LENGTH, '\0');
}

// constructors/assignment operators

OLumpName::OLumpName()
{
	assign(NULL, 0);
}

OLumpName::OLumpName(const OLumpName& other) :
	m_data(other.m_data)
{ }

OLumpName::OLumpName(const char* other)
{
	assign(other, other ? strlen(other) : 0);
}

OLumpName::OLumpName(const std::string& other)
{
	assign(other.data(), other.length());
}

OLumpName& OLumpName::operator=(const OLumpName& other)
{
	if (this != &other)
		m_data = other.m_data;

	return *this;
}

OLumpName& OLumpName::operator=(const char* other)
{
	assign(other, other ? strlen(other) : 0);

	return *this;
}

OLumpName& OLumpName::operator=(const std::string& other)
{
	assign(other.data(), other.length());

	return *this;
}

OLumpName& OLumpName::operator=(std::string_view other)
{
	assign(other.data(), other.length());

	return *this;
}

// capacity

size_t OLumpName::size() const
{
	const size_t nul = m_data.find('\0');
	return nul == std::string::npos ? m_data.length() : nul;
}

size_t OLumpName::length() const
{
	return size();
}

void OLumpName::clear()
{
	assign(NULL, 0);
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

	if (pos > s)
	{
		throw std::out_of_range(fmt::format("Attempted to access OLumpName at position {} when the size was {}", pos, s));
	}

	return m_data[pos];
}

const char& OLumpName::at(const size_t pos) const
{
	const size_t s = size();

	if (pos > s)
	{
		throw std::out_of_range(fmt::format("Attempted to access OLumpName at position {} when the size was {}", pos, s));
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

OLumpName OLumpName::substr(const size_t pos, size_t npos) const
{
	const size_t s = size();

	if (pos > s)
	{
		throw std::out_of_range(fmt::format("Attempted to access OLumpName at position {} when the size was {}", pos, s));
	}
	if (npos > s - pos)
	{
		npos = s - pos;
	}
	OLumpName out;
	out.assign(m_data.data() + pos, npos);
	return out;
}

// string operations

const char* OLumpName::c_str() const
{
	return m_data.c_str();
}

const char* OLumpName::data() const
{
	return m_data.data();
}

int OLumpName::compare(const OLumpName& other) const
{
	return !stricmp(m_data.c_str(), other.m_data.c_str());
}

int OLumpName::compare(const char* other) const
{
	return !stricmp(m_data.c_str(), other);
}

int OLumpName::compare(const std::string& other) const
{
	return !stricmp(m_data.c_str(), other.data());
}

int OLumpName::compare(std::string_view other) const
{
	return size() == other.length() &&
	       !strnicmp(m_data.c_str(), other.data(), other.length());
}

bool operator==(const OLumpName& lhs, const OLumpName& rhs)
{
	return !stricmp(lhs.m_data.c_str(), rhs.m_data.c_str());
}

bool operator==(const OLumpName& lhs, const char* rhs)
{
	return !stricmp(lhs.m_data.c_str(), rhs);
}

bool operator==(const OLumpName& lhs, const std::string& rhs)
{
	return !stricmp(lhs.m_data.c_str(), rhs.data());
}

bool operator==(const OLumpName& lhs, std::string_view rhs)
{
	return lhs.size() == rhs.length() &&
	       !strnicmp(lhs.m_data.c_str(), rhs.data(), rhs.length());
}

bool operator!=(const OLumpName& lhs, const OLumpName& rhs)
{
	return !(lhs == rhs);
}

bool operator!=(const OLumpName& lhs, const char* rhs)
{
	return !(lhs == rhs);
}

bool operator!=(const OLumpName& lhs, const std::string& rhs)
{
	return !(lhs == rhs);
}

bool operator!=(const OLumpName& lhs, std::string_view rhs)
{
	return !(lhs == rhs);
}
