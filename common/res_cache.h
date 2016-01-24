// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_cache.h $
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
// Provides in-memory cachign of game resources loaded from disk.
//
//-----------------------------------------------------------------------------

#ifndef __res_cache_h__
#define __res_cache_h__

#include "res_resourceid.h"
#include "z_zone.h"

class ResourceCache
{
public:
	ResourceCache(size_t count) :
		mResourceCount(count),
		mData(NULL)
	{
		mData = new void*[mResourceCount];
		memset((unsigned char*)mData, 0, sizeof(*mData) * mResourceCount);
	}

	~ResourceCache()
	{
		clear();
		delete [] mData;
	}

	void clear()
	{
		for (size_t i = 0; i < mResourceCount; i++)
			releaseData(ResourceId(i));
	}

	void cacheData(const ResourceId res_id, const ResourceLoader* loader, int tag = PU_STATIC)
	{
		assert(res_id != ResourceId::INVALID_ID && res_id < mResourceCount);
		assert(loader != NULL);
		mData[res_id] = allocMemory(loader->size(), tag, &(mData[res_id]));
		loader->load(mData[res_id]);
	}

	const void* getData(const ResourceId res_id) const
	{
		assert(res_id != ResourceId::INVALID_ID && res_id < mResourceCount);
		return mData[res_id];
	}

	void releaseData(const ResourceId res_id)
	{
		assert(res_id != ResourceId::INVALID_ID && res_id < mResourceCount);
		freeMemory(mData[res_id]);
		mData[res_id] = NULL;
	}

private:
	size_t				mResourceCount;
	void**				mData;

	void* allocMemory(size_t size, int tag, void** owner)
	{
		// From the C99 Standard:
		// If the size of the space requested is zero, the behavior is implementation-
		// deﬁned: either a null pointer is returned, or the behavior is as if the
		// size were some nonzero value, except that the returned pointer shall not be
		// used to access an object.
		//
		// [SL] return NULL for zero-sized allocations because it is easy to find when
		// the returned pointer is "used to access an object".
		if (size == 0)
			return NULL;

		// [RH] Allocate one byte more than necessary for the
		//		lump and set the extra byte to zero so that
		//		various text parsing routines can just call
		//		Res_LoadResource() and not choke.
		
		// [SL]: 2016-01-24 Additionally, D_LoadDehLump() also relies on an extra
		// byte being allocated.

		// TODO: [SL] 2015-04-22 This hack should be removed when the text parsing
		// routines are fixed.
		size += 1;

		void* data = Z_Malloc(size, tag, owner);

		// TODO: [SL] 2016-01-18 This hack should be removed when the text parsing
		// routines are fixed.
		((unsigned char*)data)[size - 1] = 0;

		return data;
	}

	void freeMemory(void* data)
	{
		if (data)
			Z_Free(data);
	}
};

#endif	// __res_cache_h__
