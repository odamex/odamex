// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2011 by Randy Heit (ZDoom 1.23).
// Copyright (C) 2006-2020 by The Odamex Team.
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
// FStringTable
//
// This class manages a list of localizable strings stored in a wad file.
// It does not support adding new strings, although existing ones can
// be changed.
//
//-----------------------------------------------------------------------------

#pragma once

#include <stdlib.h>
#include <utility>

#include "hashtable.h"
#include "m_ostring.h"
#include "stringenums.h"

class StringTable
{
	// First is if the string "exists", second is the string value.
	typedef std::pair<bool, OString> OptionalString;

	struct TableEntry
	{
		// String value.
		OptionalString string;

		// Pass that string was added by.
		//
		// In some cases, we need to
		int pass;

		// Index of string.
		//
		// The old strings implementation used an enum to name all of the
		// strings, and there were (and still are) several places in the
		// code that used comparison operators on the enum index.  This
		// index is -1 if it's a custom string.
		int index;
	};
	typedef OHashTable<OString, TableEntry> StringHash;
	StringHash _stringHash;

	bool canSetPassString(int pass, const std::string& name) const;
	void clearStrings();
	void loadLanguage(const char* code, bool exactMatch, int pass, char* lump,
	                  size_t lumpLen);
	void loadStringsLump(const int lump, const char* lumpname, const bool engOnly);
	void prepareIndexes();

  public:
	StringTable() : _stringHash()
	{
	}

	//
	// Obtain a string by name.
	//
	const char* operator()(const OString& name) const
	{
		StringHash::const_iterator it = _stringHash.find(name);
		if (it != _stringHash.end())
		{
			if ((*it).second.string.first == true)
				return (*it).second.string.second.c_str();
		}

		// invalid index, return an empty cstring
		static const char emptystr = 0;
		return &emptystr;
	}

	//
	// Obtain a string by index.
	//
	const char* getIndex(int index) const
	{
		if (index >= 0 && static_cast<size_t>(index) < ARRAY_LENGTH(::stringIndexes))
		{
			const OString* name = ::stringIndexes[index];
			StringHash::const_iterator it = _stringHash.find(*name);
			if (it != _stringHash.end())
			{
				if ((*it).second.string.first == true)
					return (*it).second.string.second.c_str();
			}
		}

		// invalid index, return an empty cstring
		static const char emptystr = 0;
		return &emptystr;
	}

	//
	// Obtain an index by name.
	//
	int toIndex(const OString& name) const
	{
		StringHash::const_iterator it = _stringHash.find(name);
		if (it != _stringHash.end())
		{
			// We don't care if the string exists or not, we just want its index.
			return (*it).second.index;
		}

		// Not found.
		return -1;
	}

	void dumpStrings();
	bool hasString(const OString& name) const;
	void loadStrings(const bool engOnly);
	const OString& matchString(const OString& string) const;
	void setString(const OString& name, const OString& string);
	void setPassString(int pass, const OString& name, const OString& string);
	size_t size() const;
	static void replaceEscapes(std::string& str);
};
