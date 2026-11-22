// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
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

#pragma once

#include <vector>
#include "hashtable.h"


class ResourceId
{
public:
	constexpr ResourceId(uint32_t value = -1) :
		mValue(value)
	{ }

	constexpr ResourceId(const ResourceId& other) = default;
	constexpr ResourceId& operator=(const ResourceId& other) = default;

	constexpr operator uint32_t() const
	{
		return mValue;
	}

	// Convenience helpers mirroring the old lumpHandle_t interface.
	[[nodiscard]] constexpr bool empty() const
	{
		return mValue == static_cast<uint32_t>(-1);
	}

	constexpr void clear()
	{
		mValue = static_cast<uint32_t>(-1);
	}

	static const ResourceId INVALID_ID;

private:
	uint32_t mValue;
};

using ResourceIdList = std::vector<ResourceId>;

// A ResourceKey is a hash of the resource's path instead of
// the order a resource is loaded in, which is what ResourceId is.
// 
// This can be used to look up a resource without needing to know
// its load order.
// 
// 0 is no resource.
using ResourceKey = uint64_t;

// ----------------------------------------------------------------------------
// hash function for OHashTable class
// ----------------------------------------------------------------------------

template <> struct std::hash<ResourceId>
{   size_t operator()(const ResourceId res_id) const { return static_cast<uint32_t>(res_id); } };
