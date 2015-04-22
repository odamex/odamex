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

#include "res_resourcepath.h"
#include "res_container.h"

#include "w_wad.h"
#include "z_zone.h"

// Typedefs
typedef uint32_t ResourceId;
typedef uint32_t ResourceFileId;


// Forward class declarations
class ResourceManager;
class ResourceContainer;
class FileAccessor;
class ContainerDirectory;


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



typedef std::vector<ResourceId> ResourceIdList;


bool Res_ValidateWadData(const void* data, uint32_t length);
bool Res_ValidateDehackedData(const void* data, uint32_t length);
bool Res_ValidatePCSpeakerSoundData(const void* data, uint32_t length);
bool Res_ValidateSoundData(const void* data, uint32_t length);

bool Res_IsWadFile(const OString& filename);
bool Res_IsDehackedFile(const OString& filename);


// ============================================================================
//
// ResourceLoader class interface
//
// ============================================================================
//
// Loads a resource from a ResourceContainer, performing any necessary data
// conversion and caching the resulting instance in the Zone memory system.
//

// ----------------------------------------------------------------------------
// ResourceLoader abstract base class interface
// ----------------------------------------------------------------------------

class ResourceLoader
{
public:
	virtual ~ResourceLoader() {}

	virtual bool validate(const ResourceContainer* container, const LumpId lump_id) const = 0;
	virtual uint32_t size(const ResourceContainer* container, const LumpId lump_id) const = 0;
	virtual void* load(const ResourceContainer* container, const LumpId lump_id) const = 0;
};


// ---------------------------------------------------------------------------
// DefaultResourceLoader class interface
//
// Generic resource loading functionality. Simply reads raw data and returns
// a pointer to the cached data.
// ---------------------------------------------------------------------------

class DefaultResourceLoader : public ResourceLoader
{
public:
	DefaultResourceLoader();
	virtual ~DefaultResourceLoader() { }

	virtual bool validate(const ResourceContainer* container, const LumpId lump_id) const;
	virtual uint32_t size(const ResourceContainer* container, const LumpId lump_id) const;
	virtual void* load(const ResourceContainer* container, const LumpId lump_id) const;
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

	static const ResourceId RESOURCE_NOT_FOUND = 0;

	const std::vector<std::string>& getResourceFileNames() const
	{
		return mResourceFileNames;
	}

	const std::vector<std::string>& getResourceFileHashes() const
	{
		return mResourceFileHashes;
	}

	void openResourceFiles(const std::vector<std::string>& filenames);

	void closeAllResourceFiles();

	const ResourceId addResource(
			const ResourcePath& path,
			const ResourceContainer* container,
			const LumpId lump_id);

	bool validateResourceId(const ResourceId res_id) const
	{
		return mResources.validate(res_id);
	}

	const ResourceId getResourceId(const ResourcePath& path) const;
	const ResourceId getResourceId(const OString& name, const OString& directory) const
	{
		return getResourceId(Res_MakeResourcePath(name, directory));
	}

	const ResourceIdList getAllResourceIds(const ResourcePath& path) const;
	const ResourceIdList getAllResourceIds(const OString& name, const OString& directory) const
	{
		return getAllResourceIds(Res_MakeResourcePath(name, directory));
	}

	const ResourceIdList getAllResourceIds() const;

	const ResourceId getTextureResourceId(const ResourcePath& path) const;
	const ResourceId getTextureResourceId(const OString& name, const OString& directory) const
	{
		return getTextureResourceId(Res_MakeResourcePath(name, directory));
	}

	const ResourcePath& getResourcePath(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mPath;

		static const ResourcePath empty_path;
		return empty_path;
	}

	uint32_t getLumpLength(const ResourceId res_id) const;

	uint32_t readLump(const ResourceId res_id, void* data) const;

	const void* getData(const ResourceId res_id, int tag = PU_CACHE);

	void releaseData(const ResourceId res_id);


	const ResourceContainer* getResourceContainer(const ResourceContainerId& container_id) const
	{
		if (container_id < mContainers.size())
			return mContainers[container_id];
		return NULL;
	}

	const std::string& getResourceContainerFileName(const ResourceId res_id) const;

	void dump() const;


private:
	static const size_t MAX_RESOURCES_BITS = 15;
	static const size_t MAX_RESOURCES = 1 << MAX_RESOURCES_BITS;

	struct ResourceRecord
	{
		ResourceRecord& operator=(const ResourceRecord& other)
		{
			if (&other != this)
			{
				mPath = other.mPath;
				mResourceContainerId = other.mResourceContainerId;
				mLumpId = other.mLumpId;
				mResourceLoader = other.mResourceLoader;
				mCachedData = other.mCachedData;
			}
			return *this;
		}

		ResourcePath			mPath;
		ResourceContainerId		mResourceContainerId;
		LumpId					mLumpId;
		ResourceLoader*			mResourceLoader;
		void*					mCachedData;
	};

	typedef SArray<ResourceRecord, MAX_RESOURCES_BITS> ResourceRecordTable;
	ResourceRecordTable		mResources;

	// ---------------------------------------------------------------------------
	// Private helper functions
	// ---------------------------------------------------------------------------

	const ResourceRecord* getResourceRecord(const ResourceId res_id) const
	{
		if (mResources.validate(res_id))
			return &mResources.get(res_id);
		return NULL;
	}

	const ResourceContainerId& getResourceContainerId(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mResourceContainerId;

		static const ResourceContainerId invalid_container_id = -1;
		return invalid_container_id;
	}

	const LumpId getLumpId(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mLumpId;

		static const LumpId invalid_lump_id = -1;
		return invalid_lump_id;
	}


	static const size_t MAX_RESOURCE_CONTAINERS = 255;
	std::vector<ResourceContainer*>	mContainers;
	ResourceContainerId				mTextureManagerContainerId;

	std::vector<FileAccessor*>		mAccessors;
	std::vector<std::string>		mResourceFileNames;
	std::vector<std::string>		mResourceFileHashes;

	// Map resource pathnames to ResourceIds
	typedef OHashTable<ResourcePath, ResourceIdList> ResourceIdLookupTable;
	ResourceIdLookupTable			mResourceIdLookup;


	// ---------------------------------------------------------------------------
	// Private helper functions
	// ---------------------------------------------------------------------------

	void openResourceFile(const OString& filename);

	bool visible(const ResourceId res_id) const;
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

void Res_OpenResourceFiles(const std::vector<std::string>& filename);
void Res_CloseAllResourceFiles();

const std::vector<std::string>& Res_GetResourceFileNames();
const std::vector<std::string>& Res_GetResourceFileHashes();


const ResourceId Res_GetResourceId(const OString& name, const OString& directory = global_directory_name);

const ResourceIdList Res_GetAllResourceIds(const OString& name, const OString& directory = global_directory_name); 

const ResourceId Res_GetTextureResourceId(const OString& name, const OString& directory);

const OString& Res_GetLumpName(const ResourceId res_id);

const std::string& Res_GetResourceContainerFileName(const ResourceId res_id);


const ResourcePath& Res_GetResourcePath(const ResourceId res_id);

// ----------------------------------------------------------------------------
// Res_CheckLump
// ----------------------------------------------------------------------------

bool Res_CheckLump(const ResourceId res_id);

static inline bool Res_CheckLump(const OString& name, const OString& directory = global_directory_name)
{
	return Res_CheckLump(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_GetLumpLength
// ----------------------------------------------------------------------------

uint32_t Res_GetLumpLength(const ResourceId res_id);

static inline uint32_t Res_GetLumpLength(const OString& name, const OString& directory = global_directory_name)
{
	return Res_GetLumpLength(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_ReadLump
// ----------------------------------------------------------------------------

uint32_t Res_ReadLump(const ResourceId res_id, void* data);

static inline uint32_t Res_ReadLump(const OString& name, void* data)
{
	return Res_ReadLump(Res_GetResourceId(name), data);
}


// ----------------------------------------------------------------------------
// Res_CacheLump
// ----------------------------------------------------------------------------

void* Res_CacheLump(const ResourceId res_id, int tag);

static inline void* Res_CacheLump(const OString& name, int tag)
{
	return Res_CacheLump(Res_GetResourceId(name), tag);
}


// ----------------------------------------------------------------------------
// Res_ReleaseLump
// ----------------------------------------------------------------------------

void Res_ReleaseLump(const ResourceId res_id);

static inline void Res_ReleaseLump(const OString& name)
{
	Res_ReleaseLump(Res_GetResourceId(name));
}

bool Res_CheckMap(const OString& mapname);
const ResourceId Res_GetMapResourceId(const OString& lump_name, const OString& mapname);



#endif	// __RES_MAIN_H__

