// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_resourceid.h $
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
// ResourceId identifier for game resources
//
//-----------------------------------------------------------------------------

#ifndef __RES_RESOURCEID_H__
#define __RES_RESOURCEID_H__

#include <vector>
#include "doomtype.h"
#include "hashtable.h"

class ResourceId
{
public:
	ResourceId(uint32_t value = -1) :
		mValue(value)
	{ }

	ResourceId(const ResourceId& other) :
		mValue(other.mValue)
	{ }
	
	ResourceId& operator=(const ResourceId& other)
	{
		mValue = other.mValue;
		return *this;
	}

	operator uint32_t() const
	{
		return mValue;
	}

	static const ResourceId INVALID_ID;

private:
	uint32_t mValue;
};

typedef std::vector<ResourceId> ResourceIdList;

// ----------------------------------------------------------------------------
// hash function for OHashTable class
// ----------------------------------------------------------------------------

template <> struct hashfunc<ResourceId>
{   size_t operator()(const ResourceId res_id) const { return uint32_t(res_id); } };

#endif	// __RES_RESOURCEID_H__

