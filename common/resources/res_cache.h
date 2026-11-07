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
// Provides in-memory caching of game resources loaded from disk.
//
//-----------------------------------------------------------------------------

#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>

#include "resources/res_resourceid.h"
#include "z_zone.h"

//
// ResourceBlock
//
// One cached resource that owns its own data.
//
class ResourceBlock
{
public:
	explicit ResourceBlock(uint32_t size) :
		mSize(size), mData(new uint8_t[size + 1])
	{
		mData[size] = 0;
	}

	uint8_t* data()
	{
		return mData.get();
	}

	const uint8_t* data() const
	{
		return mData.get();
	}

	uint32_t size() const
	{
		return mSize;
	}

private:
	uint32_t                   mSize;
	std::unique_ptr<uint8_t[]> mData;
};


//
// ResourceCache
//
// One slot per resource, holding its block once it has been loaded. A slot is
// published only after its block is filled, so the lock-free read in getData
// never sees a resource half-loaded -- loading and releasing are serialized by
// the caller.
//
// The cache owns its memory. A resource stays loaded until releaseData,
// releaseLevelData or clear frees it.
//
class ResourceCache
{
public:
	explicit ResourceCache(size_t count) :
		mResourceCount(count),
		mSlots(new std::atomic<ResourceBlock*>[count]),
		mLevelTagged(count, false)
	{
		for (size_t i = 0; i < mResourceCount; i++)
			mSlots[i].store(nullptr, std::memory_order_relaxed);
	}

	~ResourceCache()
	{
		clear();
	}

	ResourceCache(const ResourceCache&) = delete;
	ResourceCache& operator=(const ResourceCache&) = delete;

	void clear()
	{
		for (size_t i = 0; i < mResourceCount; i++)
			releaseData(ResourceId(static_cast<uint32_t>(i)));
		mLevelData.clear();
	}

	//
	// getData
	//
	// The lock-free read: a resource whose load has finished, or NULL.
	//
	const uint8_t* getData(const ResourceId res_id) const
	{
		assert(res_id != ResourceId::INVALID_ID and res_id < mResourceCount);
		const ResourceBlock* block = mSlots[res_id].load(std::memory_order_acquire);
		return block != nullptr ? block->data() : nullptr;
	}

	//
	// cacheData
	//
	// Publishes a filled block into an empty slot and returns its bytes. A
	// block cached with a level tag is released by releaseLevelData, which
	// stands in for the zone freeing PU_LEVEL memory at a level change.
	//
	const uint8_t* cacheData(const ResourceId res_id, std::unique_ptr<ResourceBlock> block, zoneTag_e tag)
	{
		assert(res_id != ResourceId::INVALID_ID and res_id < mResourceCount);
		assert(block != nullptr);
		assert(mSlots[res_id].load(std::memory_order_relaxed) == nullptr);

		if (tag >= PU_LEVEL and tag <= PU_LEVELMAX)
		{
			mLevelTagged[res_id] = true;
			mLevelData.push_back(res_id);
		}

		const uint8_t* data = block->data();
		mSlots[res_id].store(block.release(), std::memory_order_release);
		return data;
	}

	void releaseData(const ResourceId res_id)
	{
		assert(res_id != ResourceId::INVALID_ID and res_id < mResourceCount);
		mLevelTagged[res_id] = false;
		delete mSlots[res_id].exchange(nullptr, std::memory_order_acq_rel);
	}

	//
	// releaseLevelData
	//
	// Releases every resource still cached with a level tag.
	//
	void releaseLevelData()
	{
		for (const ResourceId res_id : mLevelData)
			if (mLevelTagged[res_id])
				releaseData(res_id);
		mLevelData.clear();
	}

private:
	size_t										mResourceCount;
	std::unique_ptr<std::atomic<ResourceBlock*>[]>	mSlots;

	// Which slots hold level-tagged data, and the order they were cached in.
	std::vector<bool>							mLevelTagged;
	std::vector<ResourceId>						mLevelData;
};
