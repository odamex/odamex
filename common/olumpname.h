// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2025 by The Odamex Team.
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

#pragma once

class OLumpName
{
	char m_data[9];

	void MakeDataPresentable();

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
	[[nodiscard]] OLumpName substr(const size_t pos = 0, size_t npos = 7) const;

	// string operations
	[[nodiscard]] const char* c_str() const;
	[[nodiscard]] const char* data() const;
	inline operator std::string_view() const { return { m_data, size() }; };
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
	friend struct hashfunc<OLumpName>;
	friend struct std::hash<OLumpName>;
};

[[nodiscard]] bool operator==(const OLumpName& lhs, const OLumpName& rhs);
[[nodiscard]] bool operator==(const OLumpName& lhs, const char* rhs);
[[nodiscard]] bool operator==(const OLumpName& lhs, const std::string& rhs);
[[nodiscard]] bool operator==(const OLumpName& lhs, std::string_view rhs);

template <>
struct hashfunc<OLumpName>
{
	auto operator()(const OLumpName& lumpname) const
	{
		const char* s = lumpname.m_data;
		size_t val = 0;
		for (size_t n = 9; *s != 0 && n != 0; s++, n--)
			val = val * 101 + *s;
		return val;
	}
};

template <>
struct std::hash<OLumpName>
{
	auto operator()(const OLumpName& lumpname) const
	{
		const char* s = lumpname.m_data;
		size_t val = 0;
		for (size_t n = 9; *s != 0 && n != 0; s++, n--)
			val = val * 101 + *s;
		return val;
	}
};

auto inline format_as(const OLumpName& s) { return s.data(); }