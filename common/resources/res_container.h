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
		OString			path;
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

	void clear()
	{
		mPathLookup.clear();
		mEntries.clear();
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

	LumpId getLumpId(const OString& path) const
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

	void addEntryInfo(const OString& path, uint32_t length, uint32_t offset = 0)
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
		const EntryInfo* entry = getEntry(lump_id);
		if (entry)
			return entry->length;
		return 0;
	}

	uint32_t getOffset(const LumpId lump_id) const
	{
		const EntryInfo* entry = getEntry(lump_id);
		if (entry)
			return entry->offset;
		return 0;
	}

	const OString& getPath(const LumpId lump_id) const
	{
		const EntryInfo* entry = getEntry(lump_id);
		if (entry)
			return entry->path;
		return OString::getEmptyString();
	}

private:
	EntryInfoList		mEntries;

	typedef OHashTable<OString, LumpId> PathLookupTable;
	PathLookupTable		mPathLookup;

	const EntryInfo* getEntry(const LumpId lump_id) const
	{
		if (lump_id != INVALID_LUMP_ID && lump_id < mEntries.size())
			return &mEntries[lump_id];
		return NULL;
	}
};


// ============================================================================
//
// ResourceContainer abstract base class interface
//
// ============================================================================

class ResourceContainer
{
public:
	ResourceContainer(const ResourceContainerId& container_id, ResourceManager* manager) :
							mResourceContainerId(container_id),
							mResourceManager(manager)
	{ }

	virtual ~ResourceContainer() { }

	const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	ResourceManager* getResourceManager() const
	{
		return mResourceManager;
	}

	virtual bool isIWad() const { return false; }

	virtual uint32_t getResourceCount() const = 0;

	virtual uint32_t getResourceSize(const ResourceId res_id) const = 0;

	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const = 0;

private:
	ResourceContainerId		mResourceContainerId;
	ResourceManager*		mResourceManager;
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
			const OString& path,
			const ResourceContainerId& container_id,
			ResourceManager* manager);

	virtual ~SingleLumpResourceContainer();

	virtual uint32_t getResourceCount() const;

	virtual uint32_t getResourceSize(const ResourceId res_id) const;

	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

private:
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
			const OString& path,
			const ResourceContainerId& container_id,
			ResourceManager* manager);
	
	virtual ~WadResourceContainer();

	virtual bool isIWad() const
	{
		return mIsIWad;
	}

	virtual uint32_t getResourceCount() const;

	virtual uint32_t getResourceSize(const ResourceId res_id) const;
		
	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

private:
	void addResourcesToManager(ResourceManager* manager);

	FileAccessor*			mFile;

	ContainerDirectory		mDirectory;

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

	bool readWadDirectory();

    typedef enum {START_MARKER, END_MARKER} MarkerType;

	bool isMarker(const OString& lump_name) const
	{
		return lump_name == "F_START" || lump_name == "FF_START" ||
				lump_name == "F_END" || lump_name == "FF_END" ||
				lump_name == "S_START" || lump_name == "SS_START" ||
				lump_name == "S_END" || lump_name == "SS_END" ||
				lump_name == "P_START" || lump_name == "PP_START" ||
				lump_name == "P_END" || lump_name == "PP_END" ||
				lump_name == "C_START" || lump_name == "C_END" ||
				lump_name == "TX_START" || lump_name == "TX_END" ||
				lump_name == "HI_START" || lump_name == "HI_END";
	}

    OString getMarkerPrefix(const OString& lump_name) const
	{
		return lump_name.substr(0, lump_name.find("_"));
	}

	MarkerType getMarkerType(const OString& lump_name) const
	{
		if (lump_name.find("_START") != std::string::npos)
			return START_MARKER;
		else
			return END_MARKER;
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
// DirectoryResourceContainer abstract base class interface
//
// ============================================================================

class DirectoryResourceContainer : public ResourceContainer
{
public:
	DirectoryResourceContainer(
			const OString& path,
			const ResourceContainerId& container_id,
			ResourceManager* manager);
	
	virtual ~DirectoryResourceContainer();

	virtual uint32_t getResourceCount() const;

	virtual uint32_t getResourceSize(const ResourceId res_id) const;
		
	virtual uint32_t loadResource(void* data, const ResourceId res_id, uint32_t size) const;

private:
	void cleanup();
	void addResourcesToManager(ResourceManager* manager);

	OString					mPath;

	ContainerDirectory		mDirectory;

	typedef OHashTable<ResourceId, LumpId> LumpIdLookupTable;
	LumpIdLookupTable		mLumpIdLookup;

	const LumpId getLumpId(const ResourceId res_id) const
	{
		LumpIdLookupTable::const_iterator it = mLumpIdLookup.find(res_id);
		if (it != mLumpIdLookup.end())
			return it->second;
		return ContainerDirectory::INVALID_LUMP_ID;
	}

	ContainerDirectory* readWadDirectory();
};

#endif	// __RES_CONTAINER_H__