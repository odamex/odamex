// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_main.h $
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

#include <stdlib.h>
#include "m_ostring.h"
#include "hashtable.h"
#include <vector>
#include <string>

#include "res_resourcepath.h"
#include "res_container.h"

#include "z_zone.h"

#include "res_resourceid.h"

// Forward class declarations
class ResourceManager;
class ResourceContainer;
class FileAccessor;
class ContainerDirectory;
class ResourceCache;
class ResourceLoader;


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

bool Res_ValidateWadData(const void* data, uint32_t length);
bool Res_ValidateDehackedData(const void* data, uint32_t length);
bool Res_ValidatePCSpeakerSoundData(const void* data, uint32_t length);
bool Res_ValidateSoundData(const void* data, uint32_t length);

bool Res_IsWadFile(const OString& filename);
bool Res_IsDehackedFile(const OString& filename);


// ============================================================================
//
// ResourceNameTranslator
//
// Provides translation from ResourcePaths to ResourceIds
//
// ============================================================================

class ResourceNameTranslator
{
public:
	ResourceNameTranslator();
	virtual ~ResourceNameTranslator() { }

	virtual void addTranslation(const ResourcePath& path, const ResourceId res_id);

	virtual const ResourceId translate(const ResourcePath& path) const;

	virtual const ResourceIdList getAllTranslations(const ResourcePath& path) const;

	virtual bool checkNameVisibility(const ResourcePath& path, const ResourceId res_id) const;

private:
	typedef OHashTable<ResourcePath, ResourceIdList> ResourceIdLookupTable;
	ResourceIdLookupTable			mResourceIdLookup;
};


// ============================================================================
//
// TextureResourceNameTranslator
//
// Provides translation from ResourcePaths to ResourceIds for texture resources.
//
// ============================================================================

class TextureResourceNameTranslator : public ResourceNameTranslator
{
public:
	TextureResourceNameTranslator();
	virtual ~TextureResourceNameTranslator() { }

	virtual const ResourceId translate(const ResourcePath& path) const;

private:
	typedef std::vector<ResourcePath> ResourcePathList;
	static ResourcePathList mAlternativeDirectories;
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

	const std::vector<std::string>& getResourceFileHashes() const
	{
		return mResourceFileHashes;
	}

	void openResourceContainers(const std::vector<std::string>& filenames);

	void closeAllResourceContainers();

	const ResourceId addResource(const ResourcePath& path, const ResourceContainer* container);

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

	const ResourcePath& getResourcePath(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mPath;
		return ResourcePath::getEmptyResourcePath();
	}

	const std::string& getResourceContainerFileName(const ResourceId res_id) const;

	uint32_t getResourceSize(const ResourceId res_id) const;

	const void* loadResourceData(const ResourceId res_id, int tag = PU_CACHE);

	void releaseResourceData(const ResourceId res_id);

	void dump() const;


private:
	struct ResourceRecord
	{
		ResourceRecord& operator=(const ResourceRecord& other)
		{
			if (&other != this)
			{
				mPath = other.mPath;
				mResourceContainerId = other.mResourceContainerId;
				mResourceLoader = other.mResourceLoader;
			}
			return *this;
		}

		ResourcePath			mPath;
		ResourceContainerId		mResourceContainerId;
		const ResourceLoader*	mResourceLoader;
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
			assert(res_rec->mResourceContainerId < mContainers.size());
			assert(mContainers[res_rec->mResourceContainerId] != NULL);
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

	const ResourceContainerId getResourceContainerId(const ResourceId res_id) const
	{
		const ResourceRecord* res_rec = getResourceRecord(res_id);
		if (res_rec)
			return res_rec->mResourceContainerId;
		return static_cast<ResourceContainerId>(-1);		// an invalid ResourceContainerId
	}


public:
	//
	// RawResourceAccessor class
	//
	// Provides access to the resources without post-processing. Instances of
	// this class are typically used by the ResourceLoader hierarchy to read the
	// raw resource data and then perform their own post-processign.
	//
	class RawResourceAccessor
	{
	public:
		RawResourceAccessor(const ResourceManager* manager) :
			mResourceManager(manager)
		{ }

		uint32_t getResourceSize(const ResourceId res_id) const
		{
			const ResourceContainerId container_id = mResourceManager->getResourceContainerId(res_id);
			const ResourceContainer* container = mResourceManager->mContainers[container_id];
			return container->getResourceSize(res_id);
		}

		void loadResource(const ResourceId res_id, void* data, uint32_t size) const
		{
			const ResourceContainerId container_id = mResourceManager->getResourceContainerId(res_id);
			const ResourceContainer* container = mResourceManager->mContainers[container_id];
			container->loadResource(data, res_id, size);
		}
	
	private:
		const ResourceManager*		mResourceManager;
	};

private:
	std::vector<ResourceContainer*>	mContainers;

	std::vector<FileAccessor*>		mAccessors;
	std::vector<std::string>		mResourceFileNames;
	std::vector<std::string>		mResourceFileHashes;

	TextureResourceNameTranslator			mNameTranslator;
	RawResourceAccessor				mRawResourceAccessor;
	ResourceCache*					mCache;

	// ---------------------------------------------------------------------------
	// Private helper functions
	// ---------------------------------------------------------------------------

	void openResourceContainer(const OString& filename);
};



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

	virtual bool validate() const
	{	return true;	}

	virtual uint32_t size() const = 0;
	virtual void load(void* data) const = 0;
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
	DefaultResourceLoader(const ResourceManager::RawResourceAccessor* accessor, const ResourceId res_id);
	virtual ~DefaultResourceLoader() { }

	virtual uint32_t size() const;
	virtual void load(void* data) const;

private:
	const ResourceManager::RawResourceAccessor*	mRawResourceAccessor;
	const ResourceId			mResourceId;
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

const ResourceIdList Res_GetAllResourceIds(const ResourcePath& path);

const OString& Res_GetResourceName(const ResourceId res_id);

const std::string& Res_GetResourceContainerFileName(const ResourceId res_id);


const ResourcePath& Res_GetResourcePath(const ResourceId res_id);

// ----------------------------------------------------------------------------
// Res_CheckResource
// ----------------------------------------------------------------------------

bool Res_CheckResource(const ResourceId res_id);

static inline bool Res_CheckResource(const OString& name, const OString& directory = global_directory_name)
{
	return Res_CheckResource(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_GetResourceSize
// ----------------------------------------------------------------------------

uint32_t Res_GetResourceSize(const ResourceId res_id);

static inline uint32_t Res_GetResourceSize(const OString& name, const OString& directory = global_directory_name)
{
	return Res_GetResourceSize(Res_GetResourceId(name, directory));
}


// ----------------------------------------------------------------------------
// Res_LoadResource
// ----------------------------------------------------------------------------

void* Res_LoadResource(const ResourceId res_id, int tag = PU_CACHE);

static inline void* Res_LoadResource(const OString& name, int tag = PU_CACHE)
{
	return Res_LoadResource(Res_GetResourceId(name), tag);
}


// ----------------------------------------------------------------------------
// Res_ReleaseResource
// ----------------------------------------------------------------------------

void Res_ReleaseResource(const ResourceId res_id);

static inline void Res_ReleaseResource(const OString& name)
{
	Res_ReleaseResource(Res_GetResourceId(name));
}

bool Res_CheckMap(const OString& mapname);
const ResourceId Res_GetMapResourceId(const OString& lump_name, const OString& mapname);



#endif	// __RES_MAIN_H__

