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
// Game resource file management, including WAD files.
//
//-----------------------------------------------------------------------------

#ifndef __RES_MAIN_H__
#define __RES_MAIN_H__

#include "ohash.h"

#include <stdlib.h>
#include <vector>
#include <string>

#include "resources/res_nametranslator.h"

#include "z_zone.h"

// Forward class declarations
class ResourceManager;
class ResourceContainer;
class FileAccessor;
class ResourceCache;
class ResourceLoader;
class RawResourceAccessor;


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
static const ResourcePath scripts_directory_name("/SCRIPTS/");
static const ResourcePath voices_directory_name("/VOICES/");
static const ResourcePath voxels_directory_name("/VOXELS/");

bool Res_IsWadFile(const OString& filename);
bool Res_IsDehackedFile(const OString& filename);


// ============================================================================
//
// RawResourceAccessor class interface
//
// ============================================================================
//
// Provides access to the resources without post-processing. Instances of
// this class are typically used by the ResourceLoader hierarchy to read the
// raw resource data and then perform their own post-processing.
//

class RawResourceAccessor
{
public:
	RawResourceAccessor(const ResourceManager* manager) :
		mResourceManager(manager)
	{ }

	const ResourcePath& getResourcePath(const ResourceId res_id) const;

	uint32_t getResourceSize(const ResourceId res_id) const;

	void loadResource(const ResourceId res_id, void* data, uint32_t size) const;

private:
	const ResourceManager*		mResourceManager;
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

	const std::vector<std::string>& getResourceFileNames() const
	{
		return mResourceFileNames;
	}

	const std::vector<OMD5Hash>& getResourceFileHashes() const;

	void addResourceContainer(
				ResourceContainer* container,
				ResourceContainer* parent,
				const ResourcePath& base_path,
				const std::string& filename);

	void openResourceFiles(const std::vector<std::string>& filenames);

	void closeAllResourceContainers();

	const ResourceId addResource(const ResourcePath& path, const ResourceContainer* container, const ResourceLoader* loader=NULL);

	bool validateResourceId(const ResourceId res_id) const
	{
		return res_id >= 0 && res_id < mResources.size();
	}

	const ResourceId getResourceId(const ResourcePath& path) const
	{
		return mNameTranslator.translate(path);
	}

	const ResourceIdList getAllResourceIds(const ResourcePath& path) const
	{
		return mNameTranslator.getAllTranslations(path);
	}

	const ResourceIdList getAllResourceIds() const;

	const ResourcePathList listResourceDirectory(const ResourcePath& path) const;

	const ResourcePath& getResourcePath(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mPath;
		return ResourcePath::getEmptyResourcePath();
	}

	const std::string& getResourceContainerFileName(const ResourceId res_id) const;

	uint32_t getResourceSize(const ResourceId res_id) const;

	const void* loadResourceData(const ResourceId res_id, zoneTag_e tag = PU_CACHE);

	void releaseResourceData(const ResourceId res_id);

	void dump() const;

	const RawResourceAccessor* getRawResourceAccessor() const
	{
		return &mRawResourceAccessor;
	}

	bool checkForSameContainer(const ResourceId res_id1, const ResourceId res_id2) const
	{
		return validateResourceId(res_id1) && validateResourceId(res_id2) &&
				getResourceContainer(res_id1) == getResourceContainer(res_id2);
	}

private:
	struct ResourceRecord
	{
		ResourceRecord& operator=(const ResourceRecord& other)
		{
			if (&other != this)
			{
				mPath = other.mPath;
				mResourceContainer = other.mResourceContainer;
				mResourceLoader = other.mResourceLoader;
			}
			return *this;
		}

		ResourcePath				mPath;
		const ResourceContainer*	mResourceContainer;
		const ResourceLoader*		mResourceLoader;
	};

	typedef std::vector<ResourceRecord> ResourceRecordTable;
	ResourceRecordTable			mResources;

	// ---------------------------------------------------------------------------
	// Private helper functions
	// ---------------------------------------------------------------------------

	const ResourceRecord* getResourceRecord(const ResourceId res_id) const
	{
		if (validateResourceId(res_id))
		{
			const ResourceRecord* res_rec = &mResources[res_id];
			assert(res_rec != NULL);
			assert(res_rec->mResourceContainer != NULL);
			return res_rec;
		}
		return NULL;
	}

	ResourceRecord* getResourceRecord(const ResourceId res_id)
	{
		return const_cast<ResourceRecord*>(static_cast<const ResourceManager&>(*this).getResourceRecord(res_id));
	}

	const ResourceId getResourceId(const ResourceRecord* res_rec) const
	{
		return res_rec - &mResources[0];
	}

	const ResourceContainer* getResourceContainer(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mResourceContainer;
		return NULL;
	}

	struct ResourceContainerRecord
	{
		ResourceContainerRecord& operator=(const ResourceContainerRecord& other)
		{
			if (&other != this)
			{
				mResourceContainer = other.mResourceContainer;
				mParent = other.mParent;
				mBasePath = other.mBasePath;
				mFileName = other.mFileName;
			}
			return *this;
		}

		ResourceContainer*		mResourceContainer;
		ResourceContainer*		mParent;
		ResourcePath			mBasePath;
		std::string				mFileName;
	};
	typedef std::vector<ResourceContainerRecord> ResourceContainerRecordTable;
	ResourceContainerRecordTable		mResourceContainers;

	const ResourceContainerRecord* getResourceContainerRecord(const ResourceContainer* container) const
	{
		// Linear search for now
		for (ResourceContainerRecordTable::const_iterator it = mResourceContainers.begin(); it != mResourceContainers.end(); ++it)
			if (it->mResourceContainer == container)
				return &(*it);
		return NULL;
	}

	void openResourceFile(const OString& filename);

	std::vector<std::string>			mResourceFileNames;
	mutable std::vector<OMD5Hash>	mResourceFileHashes;

	typedef OHashTable<size_t, OString> ResourceContainerFileNameLookup;
	ResourceContainerFileNameLookup		mResourceContainerFileNames;

	ResourceNameTranslator			mNameTranslator;

	friend class RawResourceAccessor;
	RawResourceAccessor				mRawResourceAccessor;

	ResourceCache*					mCache;

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
	static const OString filename("ODAMEX.WAD");
	return filename;
}

void Res_OpenResourceFiles(const std::vector<std::string>& filename);
void Res_CloseAllResourceFiles();

const std::vector<std::string>& Res_GetResourceFileNames();
const std::vector<OMD5Hash>& Res_GetResourceFileHashes();


const ResourcePathList Res_ListResourceDirectory(const ResourcePath& path);

const ResourceId Res_GetResourceId(const ResourcePath& path);

const ResourceId Res_GetResourceId(const OString& name, const ResourcePath& directory);

const ResourceIdList Res_GetAllResourceIds(const ResourcePath& path);

const OString& Res_GetResourceName(const ResourceId res_id);

const ResourcePath& Res_GetResourcePath(const ResourceId res_id);

// ----------------------------------------------------------------------------
// Res_CheckResource
// ----------------------------------------------------------------------------

bool Res_CheckResource(const ResourceId res_id);

static inline bool Res_CheckResource(const OString& name, const ResourcePath& directory)
{
	return Res_CheckResource(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_GetResourceSize
// ----------------------------------------------------------------------------

uint32_t Res_GetResourceSize(const ResourceId res_id);

static inline uint32_t Res_GetResourceSize(const OString& name, const ResourcePath& directory)
{
	return Res_GetResourceSize(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_LoadResource
// ----------------------------------------------------------------------------

const void* Res_LoadResource(const ResourceId res_id, zoneTag_e tag = PU_CACHE);

static inline const void* Res_LoadResource(const OString& name, zoneTag_e tag = PU_CACHE)
{
	return Res_LoadResource(Res_GetResourceId(name, global_directory_name), tag);
}


// ----------------------------------------------------------------------------
// Res_ReleaseResource
// ----------------------------------------------------------------------------

void Res_ReleaseResource(const ResourceId res_id);

static inline void Res_ReleaseResource(const OString& name)
{
	Res_ReleaseResource(Res_GetResourceId(name, global_directory_name));
}


// ----------------------------------------------------------------------------
// Res_GetResourceContainerFileName
// ----------------------------------------------------------------------------

const std::string& Res_GetResourceContainerFileName(const ResourceId res_id);


bool Res_CheckMap(const OString& mapname);
const ResourceId Res_GetMapResourceId(const OString& lump_name, const OString& mapname);


#endif	// __RES_MAIN_H__