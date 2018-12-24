// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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

#ifndef __RES_RESOURCEPATH_H__
#define __RES_RESOURCEPATH_H__

#include <stdlib.h>
#include "m_ostring.h"
#include <vector>
#include <string>
#include <sstream>

// ============================================================================
//
// ResourcePath class interface
//
// ============================================================================

class ResourcePath
{
public:
	ResourcePath() :
		mItemCount(0), mAbsolute(false)
	{ }

	ResourcePath(const OString& path)
	{
		parseString(path);
	}

	ResourcePath(const std::string& path)
	{
		parseString(path);
	}

	ResourcePath(const char* path)
	{
		parseString(path);
	}

	ResourcePath(const ResourcePath& other)
	{
		operator=(other);
	}

	static ResourcePath& getEmptyResourcePath()
	{
		static ResourcePath empty_path;
		return empty_path;
	}

	ResourcePath& operator=(const ResourcePath& other)
	{
		mAbsolute = other.mAbsolute;
		for (mItemCount = 0; mItemCount < other.mItemCount; mItemCount++)
			mItems[mItemCount] = other.mItems[mItemCount];
		return *this;
	}

	ResourcePath& operator+=(const ResourcePath& other)
	{
		if (other.mAbsolute)
			return operator=(other);

		for (size_t i = 0; i < other.mItemCount && mItemCount < ResourcePath::MAX_ITEMS; i++)
			addItem(other.mItems[i]);

		return *this;
	}

	ResourcePath operator+(const ResourcePath& other) const
	{
		ResourcePath result(*this);
		result.operator+=(other);
		return result;
	}

	void append(const ResourcePath& other)
	{
		operator+=(other);
	}

	OString toString() const
	{
		std::string output;
		for (size_t i = 0; i < mItemCount; i++)
		{
			if (i > 0 || mAbsolute)
				output += ResourcePath::DELIMINATOR;
			output += std::string(mItems[i]);
		}
		return OString(output);
	}

	operator OString() const
	{
		return toString();
	}

	bool operator==(const ResourcePath& other) const
	{
		if (mItemCount == other.mItemCount)
		{
			for (size_t i = 0; i < mItemCount; i++)
				if (mItems[i] != other.mItems[i])
					return false;
			return true;
		}
		return false;
	}

	size_t size() const
	{
		return mItemCount;
	}

	bool empty() const
	{
		return size() == 0;
	}

	const OString& first() const
	{
		if (mItemCount > 0)
			return mItems[0];
		static const OString empty_string;
		return empty_string;
	}

	const OString& last() const
	{
		if (mItemCount > 0)
			return mItems[mItemCount - 1];
		static const OString empty_string;
		return empty_string;
	}

	OString& operator[](size_t index)
	{
		return mItems[index];
	}

	const OString& operator[](size_t index) const
	{
		return mItems[index];
	}

private:
	//
	// ResourcePath::addItem
	//
	// Adds the token to the end of the item list. It also handles special
	// item names "." and "..".
	//
	void addItem(const std::string& token)
	{
		if (token.compare("..") == 0)
		{
			if (mItemCount > 0 && mItems[mItemCount - 1].compare("..") != 0)
				mItemCount--;
			else if (!mAbsolute)
				mItems[mItemCount++] = OStringToUpper(token);
		}
		else if (!token.empty() && token.compare(".") != 0)
		{
			mItems[mItemCount++] = OStringToUpper(token);
		}
	}

	//
	// ResourcePath::parseString
	//
	// Splits the given path string into tokens and adds them to the item list.
	//
	void parseString(const std::string& path)
	{
		mItemCount = 0;
		mAbsolute = !path.empty() && path[0] == ResourcePath::DELIMINATOR;

		std::stringstream ss(path);
		std::string token;
		while (std::getline(ss, token, ResourcePath::DELIMINATOR) && mItemCount < ResourcePath::MAX_ITEMS)
			addItem(token);
	}

	static const char DELIMINATOR = '/';
	static const size_t MAX_ITEMS = 16;

	OString mItems[MAX_ITEMS];
	unsigned char mItemCount;
	bool mAbsolute;

	// ------------------------------------------------------------------------
	// non-member friend functions
	// ------------------------------------------------------------------------

	friend struct hashfunc<ResourcePath>;
};

typedef std::vector<ResourcePath> ResourcePathList;

// ----------------------------------------------------------------------------
// hash function for OHashTable class
// ----------------------------------------------------------------------------

template <> struct hashfunc<ResourcePath>
{	unsigned int operator()(const ResourcePath& path) const { return __hash_cstring(path.toString().c_str()); } };



//
// Res_MakeResourcePath
//
// Appends a directory name to a lump name to compose a resource path.
//
static inline ResourcePath Res_MakeResourcePath(const OString& name, const OString& directory)
{
	return ResourcePath(directory) + ResourcePath(name);
}

//
// Res_DoesPathStartWithBase
//
static inline bool Res_DoesPathStartWithBase(const ResourcePath& base, const ResourcePath& path)
{
	if (path.size() < base.size())
		return false;
	for (size_t i = 0; i < base.size(); i++)
		if (path[i] != base[i])
			return false;
	return true;
}

#endif	// __RES_RESOURCEPATH_H__
