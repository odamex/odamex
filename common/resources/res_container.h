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
// Game resource file abstractions
//
//-----------------------------------------------------------------------------

#ifndef __RES_CONTAINER_H__
#define __RES_CONTAINER_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <vector>
#include "m_ostring.h"
#include "hashtable.h"

#include "resources/res_resourceid.h"
#include "resources/res_resourcepath.h"

typedef uint32_t ResourceContainerId;
typedef uint32_t LumpId;


// forward declarations
class FileAccessor;
class ResourceManager;


// ============================================================================
//
// ContainerDirectory class interface & implementation
//
// ============================================================================
//
// A generic resource container directory for querying information about
// game resource lumps.
//

class ContainerDirectory
{
private:
	struct EntryInfo
	{
		ResourcePath	path;
		uint32_t		length;
		uint32_t		offset;
	};

	typedef std::vector<EntryInfo> EntryInfoList;

public:
	typedef EntryInfoList::iterator iterator;
	typedef EntryInfoList::const_iterator const_iterator;

	static const LumpId INVALID_LUMP_ID = static_cast<LumpId>(-1);

	ContainerDirectory(const size_t initial_size = 4096) :
		mPathLookup(2 * initial_size)
	{
		mEntries.reserve(initial_size);
	}

	iterator begin()
	{
		return mEntries.begin();
	}

	const_iterator begin() const
	{
		return mEntries.begin();
	}

	iterator end()
	{
		return mEntries.end();
	}

	const_iterator end() const
	{
		return mEntries.end();
	}

	const LumpId getLumpId(iterator it) const
	{
		return it - begin();
	}

	const LumpId getLumpId(const_iterator it) const
	{
		return it - begin();
	}

	LumpId getLumpId(const ResourcePath& path) const
	{
		PathLookupTable::const_iterator it = mPathLookup.find(path);
		if (it != mPathLookup.end())
			return getLumpId(&mEntries[it->second]);
		return INVALID_LUMP_ID;
	}

	LumpId getLumpId(const EntryInfo* entry) const
	{
		if (!mEntries.empty() && static_cast<LumpId>(entry - &mEntries.front()) < mEntries.size())
			return static_cast<LumpId>(entry - &mEntries.front());
		return INVALID_LUMP_ID;
	}

	size_t size() const
	{
		return mEntries.size();
	}

	bool validate(const LumpId lump_id) const
	{
		return lump_id < mEntries.size();
	}

	void addEntryInfo(const ResourcePath& path, uint32_t length, uint32_t offset = 0)
	{
		mEntries.push_back(EntryInfo());
		EntryInfo* entry = &mEntries.back();
		entry->path = path;
		entry->length = length;
		entry->offset = offset;

		const LumpId lump_id = getLumpId(entry);
		assert(lump_id != INVALID_LUMP_ID);
		mPathLookup.insert(std::make_pair(path, lump_id));
	}

	uint32_t getLength(const LumpId lump_id) const
	{
		return mEntries[lump_id].length;
	}

	uint32_t getOffset(const LumpId lump_id) const
	{
		return mEntries[lump_id].offset;
	}

	const ResourcePath& getPath(const LumpId lump_id) const
	{
		return mEntries[lump_id].path;
	}

	bool between(const LumpId lump_id, const ResourcePath& start, const ResourcePath& end) const
	{
		LumpId start_lump_id = getLumpId(start);
		LumpId end_lump_id = getLumpId(end);

		if (lump_id == INVALID_LUMP_ID || start_lump_id == INVALID_LUMP_ID || end_lump_id == INVALID_LUMP_ID)
			return false;
		return start_lump_id < lump_id && lump_id < end_lump_id;
	}

	bool between(const ResourcePath& path, const ResourcePath& start, const ResourcePath& end) const
	{
		return between(getLumpId(path), start, end);
	}

	const ResourcePath& next(const LumpId lump_id) const
	{
		if (lump_id != INVALID_LUMP_ID && lump_id + 1 < mEntries.size())
			return mEntries[lump_id + 1].path;
		static ResourcePath empty_path;
		return empty_path;
	}

	const ResourcePath& next(const ResourcePath& path) const
	{
		return next(getLumpId(path));
	}

private:
	EntryInfoList		mEntries;

	typedef OHashTable<ResourcePath, LumpId> PathLookupTable;
	PathLookupTable		mPathLookup;
};


// ============================================================================
//
// ResourceContainer abstract base class interface
//
// ============================================================================

class ResourceContainer
{
public:
	ResourceContainer() { }
	virtual ~ResourceContainer() { }

	virtual const ResourceContainerId& getResourceContainerId() const = 0;

	virtual bool isIWad() const { return false; }

	virtual uint32_t getResourceCount() const = 0;

	virtual uint32_t getResourceSize(const ResourceId res_id) const = 0;

	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const = 0;
};

// ============================================================================
//
// SingleLumpResourceContainer abstract base class interface
//
// ============================================================================

class SingleLumpResourceContainer : public ResourceContainer
{
public:
	SingleLumpResourceContainer(
			FileAccessor* file,
			const ResourceContainerId& container_id,
			ResourceManager* manager);

	virtual ~SingleLumpResourceContainer() {}

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual uint32_t getResourceCount() const;

	virtual uint32_t getResourceSize(const ResourceId res_id) const;

	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

private:
	ResourceContainerId		mResourceContainerId;
	FileAccessor*			mFile;
	ResourceId				mResourceId;
};


// ============================================================================
//
// WadResourceContainer abstract base class interface
//
// ============================================================================

class WadResourceContainer : public ResourceContainer
{
public:
	WadResourceContainer(
			FileAccessor* file,
			const ResourceContainerId& container_id,
			ResourceManager* manager);
	
	virtual ~WadResourceContainer();

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual bool isIWad() const
	{
		return mIsIWad;
	}

	virtual uint32_t getResourceCount() const;

	virtual uint32_t getResourceSize(const ResourceId res_id) const;
		
	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

private:
	void cleanup();
	void addResourcesToManager(ResourceManager* manager);

	ResourceContainerId		mResourceContainerId;
	FileAccessor*			mFile;

	ContainerDirectory*		mDirectory;

	typedef OHashTable<ResourceId, LumpId> LumpIdLookupTable;
	LumpIdLookupTable		mLumpIdLookup;

	bool					mIsIWad;

	const LumpId getLumpId(const ResourceId res_id) const
	{
		LumpIdLookupTable::const_iterator it = mLumpIdLookup.find(res_id);
		if (it != mLumpIdLookup.end())
			return it->second;
		return ContainerDirectory::INVALID_LUMP_ID;
	}

	ContainerDirectory* readWadDirectory();

    typedef enum {START_MARKER, END_MARKER} MarkerType;
	struct MarkerRecord
	{
		OString name;
		MarkerType type;
		LumpId lump_id;
	};

	typedef std::vector<MarkerRecord> MarkerList;

	bool isMarker(const ResourcePath& path) const
	{
		const OString& name = path.last();
		return name == "F_START" || name == "FF_START" ||
				name == "F_END" || name == "FF_END" ||
				name == "S_START" || name == "SS_START" ||
				name == "S_END" || name == "SS_END" ||
				name == "P_START" || name == "PP_START" ||
				name == "P_END" || name == "PP_END" ||
				name == "C_START" || name == "C_END" ||
				name == "TX_START" || name == "TX_END" ||
				name == "HI_START" || name == "HI_END";
	}

    OString getMarkerPrefix(const ResourcePath& path) const
	{
		const OString& name = path.last();
		return name.substr(0, name.find("_"));
	}

	MarkerType getMarkerType(const ResourcePath& path) const
	{
		const OString& name = path.last();
		if (name.find("_START") != std::string::npos)
			return START_MARKER;
		else
			return END_MARKER;
	}

	MarkerRecord buildMarkerRecord(const ResourcePath& path) const
	{
		const OString& name = path.last();
		MarkerRecord marker_record;
		marker_record.name = name;
		marker_record.lump_id = mDirectory->getLumpId(path);
		marker_record.type = getMarkerType(path);
		return marker_record;
	}

	void buildMarkerRecords();

	struct MarkerRange
	{
		LumpId start;
		LumpId end;
	};

	typedef OHashTable<OString, MarkerRange> MarkerRangeLookupTable;
	MarkerRangeLookupTable mMarkers;

	const ResourcePath& assignPathBasedOnMarkers(LumpId lump_id) const;
};


// ============================================================================
//
// CompositeTextureResourceContainer abstract base class interface
//
// ============================================================================

class CompositeTextureResourceContainer : public ResourceContainer
{
public:
	CompositeTextureResourceContainer(
			const ResourceContainerId& container_id,
			ResourceManager* manager);
	
	virtual ~CompositeTextureResourceContainer();

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual uint32_t getResourceCount() const;

	virtual uint32_t getResourceSize(const ResourceId res_id) const;
		
	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

private:
	void cleanup();
	size_t countTexturesInDefinition(ResourceId res_id);
	void addTexturesFromDefinition(ResourceId res_id);

	ResourceContainerId		mResourceContainerId;
	ResourceManager*		mResourceManager;
	ContainerDirectory*		mDirectory;
};


#endif	// __RES_CONTAINER_H__
