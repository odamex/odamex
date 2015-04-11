// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_main.h $
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
// Game resource file management, including WAD files.
//
//-----------------------------------------------------------------------------

#ifndef __RES_MAIN_H__
#define __RES_MAIN_H__

#include <stdlib.h>
#include "m_ostring.h"
#include "hashtable.h"
#include <vector>
#include <string>
#include <sstream>

#include "w_wad.h"

// Typedefs
typedef uint32_t ResourceFileId;
typedef uint32_t ResourceContainerId;
typedef uint32_t LumpId;

// Forward class declarations
class ResourceManager;
class ResourceContainer;
class FileAccessor;
class ContainerDirectory;



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
				mItems[mItemCount++] = token;
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
	size_t mItemCount;
	bool mAbsolute;

	// ------------------------------------------------------------------------
	// non-member friend functions
	// ------------------------------------------------------------------------

	friend struct hashfunc<ResourcePath>;
};

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


// Default directory names for ZDoom zipped resource files.
// See: http://zdoom.org/wiki/Using_ZIPs_as_WAD_replacement
//
static const ResourcePath global_directory_name("/GLOBAL/");
static const ResourcePath patches_directory_name("/PATCHES/");
static const ResourcePath graphics_directory_name("/GRAPHICS/");
static const ResourcePath sounds_directory_name("/SOUNDS/");
static const ResourcePath music_directory_name("/MUSIC/");
static const ResourcePath maps_directory_name("/MAPS/");
static const ResourcePath flats_directory_name("/FLATS/");
static const ResourcePath sprites_directory_name("/SPRITES/");
static const ResourcePath textures_directory_name("/TEXTURES/");
static const ResourcePath hires_directory_name("/HIRES/");
static const ResourcePath colormaps_directory_name("/COLORMAPS/");
static const ResourcePath acs_directory_name("/ACS/");
static const ResourcePath voices_directory_name("/VOICES/");
static const ResourcePath voxels_directory_name("/VOXELS/");



// ============================================================================
//
// ResourceId class interface
//
// ============================================================================

//
// Uniquely identifies a specific game resource loaded from a resource file.
// This is analogous to a specific lump loaded in a WAD file.
//
class ResourceId
{
public:
	ResourceId() :
		mIndex(static_cast<IndexType>(-1)),
		mResourceContainerId(static_cast<ResourceContainerId>(-1)),
		mLumpId(static_cast<LumpId>(-1))
	{ }

	ResourceId(const ResourceId& other) :
		mIndex(other.mIndex), mResourcePath(other.mResourcePath),
		mResourceContainerId(other.mResourceContainerId),
		mLumpId(other.mLumpId)
	{ }

	ResourceId(const ResourcePath& path, const ResourceContainerId& container_id, const LumpId& lump_id) :
		mIndex(static_cast<IndexType>(-1)),
		mResourcePath(path),
		mResourceContainerId(container_id),
		mLumpId(lump_id)
	{ }

	const ResourcePath& getResourcePath() const
	{
		return mResourcePath;
	}

	const LumpId& getLumpId() const
	{
		return mLumpId;
	}

	const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	ResourceId& operator=(const ResourceId& other)
	{
		if (this == &other)
			return *this;
		mIndex = other.mIndex;
		mResourcePath = other.mResourcePath;
		mResourceContainerId = other.mResourceContainerId;
		mLumpId = other.mLumpId;
		return *this;
	}

	bool operator==(const ResourceId& other) const
	{	return mIndex == other.mIndex;	}

	bool operator!=(const ResourceId& other) const
	{	return mIndex != other.mIndex;	}

	bool operator>(const ResourceId& other) const
	{	return mIndex > other.mIndex;	}

	bool operator>=(const ResourceId& other) const
	{	return mIndex >= other.mIndex;	}

	bool operator<(const ResourceId& other) const
	{	return mIndex < other.mIndex;	}

	bool operator<=(const ResourceId& other) const
	{	return mIndex <= other.mIndex;	}

	bool valid() const
	{	return mIndex != static_cast<IndexType>(-1);	}

private:
	friend class ResourceContainer;
	friend class ResourceManager;

	typedef size_t IndexType;
	IndexType				mIndex;

	ResourcePath			mResourcePath;

	ResourceContainerId		mResourceContainerId;
	LumpId					mLumpId;

	// ------------------------------------------------------------------------
	// non-member friend functions
	// ------------------------------------------------------------------------

	friend struct hashfunc<ResourceId>;
};

// ----------------------------------------------------------------------------
// hash function for OHashTable class
// ----------------------------------------------------------------------------

template <> struct hashfunc<ResourceId>
{	unsigned int operator()(const ResourceId& res_id) const { return static_cast<unsigned int>(res_id.mIndex); } };


typedef std::vector<ResourceId> ResourceIdList;



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

	virtual const OString& getFileName() const = 0;

	virtual bool isIWad() const { return false; }

	virtual size_t getLumpCount() const = 0;

	virtual bool checkLump(const ResourceId& res_id) const
	{
		return res_id.getResourceContainerId() == getResourceContainerId() &&
			res_id.getLumpId() < getLumpCount();
	}

	virtual size_t getLumpLength(const ResourceId& res_id) const = 0;

	virtual size_t readLump(const ResourceId& res_id, void* data, size_t length) const = 0;
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

	virtual const OString& getFileName() const;

	virtual size_t getLumpCount() const;

	virtual size_t getLumpLength(const ResourceId& res_id) const;

	virtual size_t readLump(const ResourceId& res_id, void* data, size_t length) const;

private:
	ResourceContainerId		mResourceContainerId;
	FileAccessor*			mFile;
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
			const ResourceContainerId& file_id,
			ResourceManager* manager);
	
	virtual ~WadResourceContainer();

	virtual const ResourceContainerId& getResourceContainerId() const
	{
		return mResourceContainerId;
	}

	virtual const OString& getFileName() const;

	virtual bool isIWad() const
	{
		return mIsIWad;
	}

	virtual size_t getLumpCount() const;

	virtual size_t getLumpLength(const ResourceId& res_id) const;
		
	virtual size_t readLump(const ResourceId& res_id, void* data, size_t length) const;

private:
	void cleanup();

	ResourceContainerId		mResourceContainerId;
	FileAccessor*			mFile;

	ContainerDirectory*		mDirectory;

	bool					mIsIWad;
};


// ============================================================================
//
// ResourceManager class interface
//
// ============================================================================
//
// Manages a collection of ResourceFiles.
//

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	static const ResourceId RESOURCE_NOT_FOUND;

	bool validateResourceId(const ResourceId& res_id) const;

	void openResourceFile(const OString& filename);

	const std::vector<std::string>& getResourceFileNames() const
	{
		return mResourceFileNames;
	}

	const std::vector<std::string>& getResourceFileHashes() const
	{
		return mResourceFileHashes;
	}

	void closeAllResourceFiles();

	const ResourceId& addResource(
			const ResourcePath& path,
			const ResourceContainerId& container_id,
			const LumpId& lump_id);

	const ResourceId& getResourceId(const ResourcePath& path) const;
	const ResourceId& getResourceId(const OString& name, const OString& directory) const
	{
		return getResourceId(Res_MakeResourcePath(name, directory));
	}

	const ResourceIdList getAllResourceIds(const ResourcePath& path) const;
	const ResourceIdList getAllResourceIds(const OString& name, const OString& directory) const
	{
		return getAllResourceIds(Res_MakeResourcePath(name, directory));
	}

	const ResourceContainer* getResourceContainer(const ResourceContainerId& container_id) const
	{
		if (container_id < mContainers.size())
			return mContainers[container_id];
		return NULL;
	}

	void dump() const;

private:
	ResourceIdList					mResourceIds;

	std::vector<ResourceContainer*>	mContainers;
	std::vector<FileAccessor*>		mAccessors;
	std::vector<std::string>		mResourceFileNames;
	std::vector<std::string>		mResourceFileHashes;

	// Map resource pathnames to indices into mResourceIds
	typedef std::vector<size_t> ResourceIdIndexList;
	typedef OHashTable<ResourcePath, ResourceIdIndexList> ResourceIdLookupTable;
	ResourceIdLookupTable			mResourceIdLookup;

	// Map indicies into mResourceIds to resource pathnames
	typedef OHashTable<size_t, ResourcePath> ResourcePathLookupTable;
	ResourcePathLookupTable			mResourcePathLookup;


	bool visible(const ResourceId& res_id) const;
};


// ============================================================================
//
// Externally visible functions
//
// ============================================================================

//
// Res_GetEngineResourceFileName
//
// Returns the file name for the engine's resource file. Use this function
// rather than hard-coding the file name.
//
static inline const OString& Res_GetEngineResourceFileName()
{
	static const OString& filename("ODAMEX.WAD");
	return filename;
}

void Res_OpenResourceFile(const OString& filename);
void Res_CloseAllResourceFiles();

const std::vector<std::string>& Res_GetResourceFileNames();
const std::vector<std::string>& Res_GetResourceFileHashes();


const ResourceId& Res_GetResourceId(const OString& name, const OString& directory = global_directory_name);

const ResourceIdList Res_GetAllResourceIds(const OString& name, const OString& directory = global_directory_name); 

const OString& Res_GetLumpName(const ResourceId& res_id);

const OString& Res_GetResourceFileName(const ResourceId& res_id);

// ----------------------------------------------------------------------------
// Res_CheckLump
// ----------------------------------------------------------------------------

bool Res_CheckLump(const ResourceId& res_id);

static inline bool Res_CheckLump(const OString& name, const OString& directory = global_directory_name)
{
	return Res_CheckLump(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_GetLumpLength
// ----------------------------------------------------------------------------

size_t Res_GetLumpLength(const ResourceId& res_id);

static inline size_t Res_GetLumpLength(const OString& name, const OString& directory = global_directory_name)
{
	return Res_GetLumpLength(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_ReadLump
// ----------------------------------------------------------------------------

size_t Res_ReadLump(const ResourceId& res_id, void* data);

static inline size_t Res_ReadLump(const OString& name, void* data)
{
	return Res_ReadLump(Res_GetResourceId(name), data);
}


// ----------------------------------------------------------------------------
// Res_CacheLump
// ----------------------------------------------------------------------------

void* Res_CacheLump(const ResourceId& res_id, int tag);

static inline void* Res_CacheLump(const OString& name, int tag)
{
	return Res_CacheLump(Res_GetResourceId(name), tag);
}

bool Res_CheckMap(const OString& mapname);
const ResourceId& Res_GetMapResourceId(const OString& lump_name, const OString& mapname);



#endif	// __RES_MAIN_H__

