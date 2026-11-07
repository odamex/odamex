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
//   Historically this was a fixed char[9], limiting lump names to 8
//   characters. Archive and directory resources support long file names,
//   so the storage is now dynamically sized. For compatibility with legacy
//   callers, names shorter than 8 characters are NUL-padded so that reads
//   of elements [0..8] and fixed 8-byte writes of c_str() remain valid.
//
//-----------------------------------------------------------------------------

#pragma once

#include <string>
#include <string_view>

class OLumpName
{
	// Logical name (uppercase), always padded with NULs to a total backing
	// length of at least 9 so that legacy element access and fixed-width
	// serialization of short names stay in bounds. size() is the number of
	// characters before the first NUL.
	std::string m_data;

	void assign(const char* str, size_t max_len);

  public:

	// constructors/assignment operators
	OLumpName();
	OLumpName(const OLumpName& other);
	OLumpName(const char* other);
	OLumpName(const std::string& other);
	OLumpName& operator=(const OLumpName& other);
	OLumpName& operator=(const char* other);
	OLumpName& operator=(const std::string& other);
	OLumpName& operator=(std::string_view other);

	// capacity
	[[nodiscard]] size_t size() const;
	[[nodiscard]] size_t length() const;
	void clear();
	[[nodiscard]] bool empty() const;

	// element access
	//
	// WARNING: If you use any of the non-const element access functions, the uppercase
	//			guarantee will no longer apply! You can edit the char's to be whatever you
	//			want at that point. Be careful!
	[[nodiscard]] char& at(const size_t pos);
	[[nodiscard]] const char& at(const size_t pos) const;
	[[nodiscard]] char& operator[](const size_t pos);
	[[nodiscard]] const char& operator[](const size_t pos) const;
	[[nodiscard]] OLumpName substr(const size_t pos = 0, size_t npos = std::string::npos) const;

	// string operations
	[[nodiscard]] const char* c_str() const;
	[[nodiscard]] const char* data() const;
	inline operator std::string_view() const { return { m_data.data(), size() }; };
	// Note: comparison operations are done without regard to case sensitivity.
	[[nodiscard]] int compare(const OLumpName& other) const;
	[[nodiscard]] int compare(const char* other) const;
	[[nodiscard]] int compare(const std::string& other) const;
	[[nodiscard]] int compare(std::string_view other) const;
	friend bool operator==(const OLumpName& lhs, const OLumpName& rhs);
	friend bool operator==(const OLumpName& lhs, const char* rhs);
	friend bool operator==(const OLumpName& lhs, std::string_view rhs);
	friend bool operator==(const OLumpName& lhs, const std::string& rhs);
	friend bool operator!=(const OLumpName& lhs, const OLumpName& rhs);
	friend bool operator!=(const OLumpName& lhs, const char* rhs);
	friend bool operator!=(const OLumpName& lhs, const std::string& rhs);
	friend bool operator!=(const OLumpName& lhs, std::string_view rhs);

	// for allowing use as keys in OHashTable and std::unordered_map
	friend struct std::hash<OLumpName>;
};

[[nodiscard]] bool operator==(const OLumpName& lhs, const OLumpName& rhs);
[[nodiscard]] bool operator==(const OLumpName& lhs, const char* rhs);
[[nodiscard]] bool operator==(const OLumpName& lhs, const std::string& rhs);
[[nodiscard]] bool operator==(const OLumpName& lhs, std::string_view rhs);

template <>
struct std::hash<OLumpName>
{
	auto operator()(const OLumpName& lumpname) const
	{
		size_t val = 0;
		for (const char* s = lumpname.m_data.c_str(); *s != 0; s++)
			val = val * 101 + *s;
		return val;
	}
};

auto inline format_as(const OLumpName& s) { return s.data(); }

// TODO: make it possible for this to be consteval
inline OLumpName operator""_ln(const char* s, size_t)
{
	return OLumpName(s);
}
