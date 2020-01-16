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
template <typename T>
class ContainerDirectory
{
private:

	typedef std::vector<T> EntryList;

public:
	typedef typename EntryList::iterator iterator;
	typedef typename EntryList::const_iterator const_iterator;

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

	void reserve(const size_t new_size)
	{
		if (new_size > mEntries.size())
		{
			mEntries.reserve(new_size);
			mPathLookup.reserve(new_size);
		}
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

	LumpId getLumpId(const T* entry) const
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

	void addEntryInfo(const T& entry)
	{
		mEntries.push_back(entry);

		const LumpId lump_id = getLumpId(&mEntries.back());
		assert(lump_id != INVALID_LUMP_ID);
		mPathLookup.insert(std::make_pair(entry.path, lump_id));
	}

	const T* getEntry(const LumpId lump_id) const
	{
		if (lump_id != INVALID_LUMP_ID && lump_id < mEntries.size())
			return &mEntries[lump_id];
		return NULL;
	}

private:
	EntryList mEntries;

	typedef OHashTable<OString, LumpId> PathLookupTable;
	PathLookupTable mPathLookup;
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

struct WadDirectoryEntry
{
	OString			path;
	uint32_t		length;
	uint32_t		offset;
};


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
	virtual void addResourcesToManager(ResourceManager* manager);

	FileAccessor* mFile;

	ContainerDirectory<WadDirectoryEntry> mDirectory;

	typedef OHashTable<ResourceId, LumpId> LumpIdLookupTable;
	LumpIdLookupTable mLumpIdLookup;

	bool mIsIWad;

	const LumpId getLumpId(const ResourceId res_id) const
	{
		LumpIdLookupTable::const_iterator it = mLumpIdLookup.find(res_id);
		if (it != mLumpIdLookup.end())
			return it->second;
		return mDirectory.INVALID_LUMP_ID;
	}

	bool readWadDirectory();

	bool isLumpMapMarker(LumpId lump_id) const;

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
// SingleMapWadResourceContainer abstract base class interface
//
// ============================================================================

class SingleMapWadResourceContainer : public WadResourceContainer
{
public:
	SingleMapWadResourceContainer(
			const OString& path,
			const ResourceContainerId& container_id,
			ResourceManager* manager);
	
	virtual ~SingleMapWadResourceContainer();

private:
	virtual void addResourcesToManager(ResourceManager* manager);

	OString		mMapName;
};


// ============================================================================
//
// DirectoryResourceContainer abstract base class interface
//
// ============================================================================

struct FileSystemDirectoryEntry
{
	OString			path;
	uint32_t		length;
};


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

	OString mPath;

	ContainerDirectory<FileSystemDirectoryEntry> mDirectory;

	typedef OHashTable<ResourceId, LumpId> LumpIdLookupTable;
	LumpIdLookupTable mLumpIdLookup;

	const LumpId getLumpId(const ResourceId res_id) const
	{
		LumpIdLookupTable::const_iterator it = mLumpIdLookup.find(res_id);
		if (it != mLumpIdLookup.end())
			return it->second;
		return mDirectory.INVALID_LUMP_ID;
	}
};

#endif	// __RES_CONTAINER_H__