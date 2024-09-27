// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 2006-2024 by The Odamex Team.
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
	
	// capacity
	size_t size() const;
	size_t length() const;
	void clear();
	bool empty() const;
	
	// element access
	//
	// WARNING: If you use any of the non-const element access functions, the uppercase
	//			guarantee will no longer apply! You can edit the char's to be whatever you
	//			want at that point. Be careful!
	char& at(const size_t pos);
	const char& at(const size_t pos) const;
	char& operator[](const size_t pos);
	const char& operator[](const size_t pos) const;

	// string operations
	const char* c_str() const;
	const char* data() const;
	// Note: comparison operations are done without regard to case sensitivity.
	int compare(const OLumpName& other) const;
	int compare(const char* other) const;
	int compare(const std::string& other) const;
	friend bool operator==(const OLumpName& lhs, const OLumpName& rhs);
	friend bool operator==(const OLumpName& lhs, const char* rhs);
	friend bool operator==(const OLumpName& lhs, const std::string& rhs);
	friend bool operator!=(const OLumpName& lhs, const OLumpName& rhs);
	friend bool operator!=(const OLumpName& lhs, const char* rhs);
	friend bool operator!=(const OLumpName& lhs, const std::string& rhs);
};
