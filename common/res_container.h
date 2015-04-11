// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_container.h $
//
// Copyright (C) 2006-2014 by The Odamex Team.
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
//
//-----------------------------------------------------------------------------

#ifndef __RES_CONTAINER_H__
#define __RES_CONTAINER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include "m_ostring.h"

class ContainerDirectory
{
public:
	ContainerDirectory(const size_t initial_size = 4096) :
		mNameLookup(2 * initial_size)
	{
		mEntries.reserve(initial_size);
	}

	size_t size() const
	{
		return mEntries.size();
	}

	void addEntryInfo(const OString& name, size_t length, size_t offset = 0)
	{
		mEntries.push_back(EntryInfo());
		EntryInfo* entry = &mEntries.back();
		entry->name = name;
		entry->length = length;
		entry->offset = offset;

		size_t index = getIndex(entry);
		assert(index != INVALID_INDEX);
		mNameLookup.insert(std::make_pair(name, index));
	}

	size_t getLength(size_t index) const
	{
		return mEntries[index].length;
	}

	size_t getOffset(size_t index) const
	{
		return mEntries[index].offset;
	}

	const OString& getName(size_t index) const
	{
		return mEntries[index].name;
	}

	bool between(size_t index, const OString& start, const OString& end) const
	{
		size_t start_index = getIndex(start);
		size_t end_index = getIndex(end);

		if (index == INVALID_INDEX || start_index == INVALID_INDEX || end_index == INVALID_INDEX)
			return false;
		return start_index < index && index < end_index;
	}

	bool between(const OString& name, const OString& start, const OString& end) const
	{
		return between(getIndex(name), start, end);
	}

	const OString& next(size_t index) const
	{
		if (index != INVALID_INDEX && index + 1 < mEntries.size())
			return mEntries[index + 1].name;
		static OString empty_string;
		return empty_string; 
	}

	const OString& next(const OString& name) const
	{
		return next(getIndex(name));
	}

	void sortByName()
	{ }

private:
	static const size_t INVALID_INDEX = static_cast<size_t>(-1);

	struct EntryInfo
	{
		OString		name;
		size_t		length;
		size_t		offset;
	};

	typedef OHashTable<OString, size_t> NameLookupTable;
	NameLookupTable		mNameLookup;

	typedef std::vector<EntryInfo> EntryInfoList;
	EntryInfoList		mEntries;

	size_t getIndex(const OString& name) const
	{
		NameLookupTable::const_iterator it = mNameLookup.find(OStringToUpper(name));
		if (it != mNameLookup.end())
			return getIndex(&mEntries[it->second]);
		return INVALID_INDEX;
	}

	size_t getIndex(const EntryInfo* entry) const
	{
		if (!mEntries.empty() && static_cast<size_t>(entry - &mEntries.front()) < mEntries.size())
			return static_cast<size_t>(entry - &mEntries.front());
		return INVALID_INDEX;
	}
};

#endif	// __RES_CONTAINER_H__
