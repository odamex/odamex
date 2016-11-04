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
// NOTES:
// Resource names must be unique within the same resource file and namespace.
// This allows for resource names to be duplicated in different resource files
// so that resources may override those in previously loaded files. Resource
// names may also be duplicated within the same file if the resources belong
// to different namespaces. This allows a texture and a flat to have the same
// name without ambiguity.
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include "resources/res_main.h"
#include "resources/res_fileaccessor.h"
#include "resources/res_container.h"
#include "resources/res_texture.h"
#include "resources/res_cache.h"
#include "resources/res_resourceloader.h"

#include "m_ostring.h"
#include "hashtable.h"
#include <vector>
#include "m_fileio.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "w_wad.h"
#include "w_ident.h"

#include "i_system.h"
#include "z_zone.h"

static ResourceManager resource_manager;

//
// Res_ValidateWadData
//
// Returns true if the given lump data appears to be a valid WAD file.
// TODO: test more than just identifying string.
//
bool Res_ValidateWadData(const void* data, uint32_t length)
{
	if (length >= 4)
	{
		uint32_t magic = LELONG(*(uint32_t*)((uint8_t*)data + 0));
		return magic == ('I' | ('W' << 8) | ('A' << 16) | ('D' << 24)) ||
				magic == ('P' | ('W' << 8) | ('A' << 16) | ('D' << 24));
	}
	return false;
}


//
// Res_ValidateDehackedData
//
// Returns true if the given lump data appears to be a valid DeHackEd file.
//
bool Res_ValidateDehackedData(const void* data, uint32_t length)
{
	const char magic_str[] = "Patch File for DeHackEd v";
	uint32_t magic_len = strlen(magic_str);

	return length >= magic_len && strnicmp((const char*)data, magic_str, magic_len) == 0;
}


//
// Res_ValidatePCSpeakerSoundData
//
// Returns true if the given lump data appears to be a valid PC speaker
// sound effect. The lump starts with a 4 byte header indicating the length
// of the sound effect in bytes and then is followed by that number of bytes
// of sound data.
//
bool Res_ValidatePCSpeakerSoundData(const void* data, uint32_t length)
{
	int16_t* magic = (int16_t*)((uint8_t*)data + 0);
	int16_t* sample_length = (int16_t*)((uint8_t*)data + 2);
	return length >= 4 && LESHORT(*magic) == 0 && LESHORT(*sample_length) + 4 == (int16_t)length;
}


//
// Res_ValidateSoundData
//
// Returns true if the given lump data appears to be a valid DMX sound effect.
// The lump starts with a two byte version number (0x0003).
//
// User-created tools to convert to the DMX sound effect format typically
// do not use the correct number of padding bytes in the header and as such
// cannot be used to validate a sound lump.
//
bool Res_ValidateSoundData(const void* data, uint32_t length)
{
	uint16_t* magic = (uint16_t*)((uint8_t*)data + 0);
	return length >= 2 && LESHORT(*magic) == 3;
}


//
// Res_CheckFileHelper
//
// Helper function that opens a file and reads the first length bytes of
// the file and then passes the data to the function func and returns the
// result.
//
static bool Res_CheckFileHelper(const OString& filename, bool (*func)(const void*, uint32_t), uint32_t length)
{
	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		return false;

	char* data = new char[length];
	uint32_t read_cnt = fread(data, 1, length, fp);
	bool valid = read_cnt == length && func(data, length);
	delete [] data;
	fclose(fp);

	return valid;
}


//
// Res_IsWadFile
//
// Checks that the first four bytes of a file are "IWAD" or "PWAD"
//
bool Res_IsWadFile(const OString& filename)
{
	const uint32_t length = 4;	// length of WAD identifier ("IWAD" or "PWAD")
	return Res_CheckFileHelper(filename, &Res_ValidateWadData, length);
}


//
// Res_IsDehackedFile
//
// Checks that the first line of the file contains a DeHackEd header.
//
bool Res_IsDehackedFile(const OString& filename)
{
	const uint32_t length = strlen("Patch File for DeHackEd v");	// length of DeHackEd identifier
	return Res_CheckFileHelper(filename, &Res_ValidateDehackedData, length);
}


// ****************************************************************************


// ============================================================================
//
// ResourceManager class implementation
//
// ============================================================================

//
// ResourceManager::ResourceManager
//
// Set up the resource lookup tables.
//

ResourceManager::ResourceManager() :
	mRawResourceAccessor(this),
	mCache(NULL)
{ }


//
// ResourceManager::~ResourceManager
//
ResourceManager::~ResourceManager()
{
	delete mCache;
	mCache = NULL;
	closeAllResourceContainers();
}


//
// ResourceManager::openResourceContainer
//
// Opens a resource file and caches the directory of lump names for queries.
// 
void ResourceManager::openResourceContainer(const OString& filename)
{
	if (!M_FileExists(filename))
		return;

	ResourceContainerId container_id = mContainers.size();

	FileAccessor* file = new DiskFileAccessor(filename);
	ResourceContainer* container = NULL;

	if (Res_IsWadFile(filename))
		container = new WadResourceContainer(file, container_id, this);
	else
		container = new SingleLumpResourceContainer(file, container_id, this);

	// check that the resource container has valid lumps
	if (container && container->getResourceCount() == 0)
	{
		delete container;
		container = NULL;
		delete file;
		file = NULL;
	}

	if (container)
	{
		mContainers.push_back(container);
		mAccessors.push_back(file);
		mResourceFileNames.push_back(filename);
		mResourceFileHashes.push_back(W_MD5(filename));
	}
}


//
// ResourceManager::openResourceContainers
//
// Opens a set of resource files and creates a directory of resource path names
// for queries.
// 
void ResourceManager::openResourceContainers(const std::vector<std::string>& filenames)
{
	for (std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it)
		openResourceContainer(*it);
	
	// TODO: Is this the best place to initialize the ResourceCache instance?
	mCache = new ResourceCache(mResources.size());
}


//
// ResourceManager::closeAllResourceContainers
//
// Closes all open resource files. This should be called prior to switching
// to a new set of resource files.
//
void ResourceManager::closeAllResourceContainers()
{
	for (ResourceRecordTable::iterator it = mResources.begin(); it != mResources.end(); ++it)
		releaseResourceData(getResourceId(&(*it)));
	mResources.clear();

	for (std::vector<ResourceContainer*>::iterator it = mContainers.begin(); it != mContainers.end(); ++it)
		delete *it;
	mContainers.clear();

	for (std::vector<FileAccessor*>::iterator it = mAccessors.begin(); it != mAccessors.end(); ++it)
		delete *it;
	mAccessors.clear();

	mResourceFileNames.clear();
	mResourceFileHashes.clear();
}


//
// ResourceManager::addResource
//
// Adds a resource lump to the lookup tables and assigns it a new ResourceId.
//
const ResourceId ResourceManager::addResource(
		const ResourcePath& path,
		const ResourceContainer* container)
{
	mResources.push_back(ResourceRecord());
	ResourceRecord& res_rec = mResources.back();
	const ResourceId res_id = getResourceId(&res_rec);

	res_rec.mPath = path;
	res_rec.mResourceContainerId = container->getResourceContainerId();
	if (path.first() == "FLATS")
	{
		res_rec.mResourceLoader = new FlatTextureLoader(&mRawResourceAccessor, res_id);
	}
	else if (path.first() == "PATCHES")
	{
		res_rec.mResourceLoader = new PatchTextureLoader(&mRawResourceAccessor, res_id);
	}
	else
	{
		res_rec.mResourceLoader = new DefaultResourceLoader(&mRawResourceAccessor, res_id);
		// TODO: delete mResourceLoader at some point
	}

	mNameTranslator.addTranslation(path, res_id);
	
	return res_id;
}


//
// ResourceManager::getAllResourceIds
//
const ResourceIdList ResourceManager::getAllResourceIds() const
{
	ResourceIdList res_id_list;
	for (size_t i = 0; i < mResources.size(); i++)
		res_id_list.push_back(ResourceId(i));
	return res_id_list;
}


//
// ResourceManager::getResourceContainerFileName
//
const std::string& ResourceManager::getResourceContainerFileName(const ResourceId res_id) const
{
	const ResourceContainerId& container_id = getResourceContainerId(res_id);
	if (container_id < mResourceFileNames.size())
		return mResourceFileNames[container_id];
	static std::string empty_string;
	return empty_string;
}


//
// ResourceManager::getResourceSize
//
uint32_t ResourceManager::getResourceSize(const ResourceId res_id) const
{
	const ResourceRecord* res_rec = getResourceRecord(res_id);
	if (res_rec)
		return res_rec->mResourceLoader->size();
	return 0;
}


//
// ResourceManager::loadResourceData
//
// Utilizes the resource cache to quickly return resources that have
// previously been cached. Initially, resource data is loaded from its
// container and then post-processed. Then the post-processed resource data
// is cached for re-use.
//
const void* ResourceManager::loadResourceData(const ResourceId res_id, int tag)
{
	const void* data = NULL;
	if (validateResourceId(res_id))
	{
		data = mCache->getData(res_id);
		if (!data)
		{
			// Read the data if it's not already in the cache
			DPrintf("Resource cache miss for %s\n", OString(getResourcePath(res_id)).c_str());
			const ResourceRecord* res_rec = getResourceRecord(res_id);
			mCache->cacheData(res_id, res_rec->mResourceLoader, tag);
			data = mCache->getData(res_id);
		}
	}
	return data;
}


//
// ResourceManager::releaseResourceData
//
void ResourceManager::releaseResourceData(const ResourceId res_id)
{
	if (validateResourceId(res_id))
		mCache->releaseData(res_id);
}


//
// ResourceManager::dump
//
// Print information about each resource in all of the open resource
// files.
//
void ResourceManager::dump() const
{
	for (ResourceRecordTable::const_iterator it = mResources.begin(); it != mResources.end(); ++it)
	{
		const ResourceRecord& res_rec = *it;
		const ResourceId res_id = getResourceId(&res_rec);

		const ResourcePath& path = res_rec.mPath; 
		assert(!OString(path).empty());

		const ResourceContainerId& container_id = res_rec.mResourceContainerId;
		assert(container_id < mContainers.size());
		const ResourceContainer* container = mContainers[container_id];
		assert(container);

		bool cached = mCache->getData(res_id) != NULL;
		bool visible = mNameTranslator.checkNameVisibility(path, res_id);

		Printf(PRINT_HIGH,"0x%08X %c %s [%u] [%s]\n",
				(uint32_t)res_id,
				cached ? '$' : visible ? '*' : '-',
				OString(path).c_str(),
				(uint32_t)getResourceSize(res_id),
				getResourceContainerFileName(res_id).c_str());
	}
}


// ============================================================================
//
// Externally visible functions
//
// ============================================================================

//
// Res_OpenResourceFiles
//
// Opens a set of resource files and creates a directory of resource path names
// for queries.
// 
void Res_OpenResourceFiles(const std::vector<std::string>& filenames)
{
	resource_manager.openResourceContainers(filenames);
}


//
// Res_CloseAllResourceFiles
//
// Closes all open resource files and clears the cache. This should be called prior to switching
// to a new set of resource files.
//
void Res_CloseAllResourceFiles()
{
	resource_manager.closeAllResourceContainers();
}


//
// Res_GetResourceFileNames
//
// Returns a vector of file names of the currently open resource files. The
// file names include the full path.
//
const std::vector<std::string>& Res_GetResourceFileNames()
{
	return resource_manager.getResourceFileNames();
}


//
// Res_GetResourceFileHashes
//
// Returns a vector of string representations of the MD5SUM for each of the
// currently open resource files. 
//
const std::vector<std::string>& Res_GetResourceFileHashes()
{
	return resource_manager.getResourceFileHashes();
}



//
// Res_GetResourceId
//
// Looks for the resource lump that matches id. If there are more than one
// lumps that match, resource file merging rules dictate that the id of the
// matching lump in the resource file with the highest id (most recently added)
// is returned. A special token of ResourceFile::LUMP_NOT_FOUND is returned if
// there are no matching lumps.
//
const ResourceId Res_GetResourceId(const OString& name, const OString& directory)
{
	const ResourcePath path = Res_MakeResourcePath(name, directory);
	return resource_manager.getResourceId(path);
}


 //
 // Res_GetAllResourceIds
 //
 // Fills the supplied vector with the ResourceId of any lump whose name matches the
 // given path. An empty vector indicates that there were no matches.
 //
 const ResourceIdList Res_GetAllResourceIds(const ResourcePath& path)
 {
	return resource_manager.getAllResourceIds(path);
 }


//
// Res_GetResourcePath
//
const ResourcePath& Res_GetResourcePath(const ResourceId res_id)
{
	return resource_manager.getResourcePath(res_id);
}


//
// Res_GetResourceContainerFileName
//
const std::string& Res_GetResourceContainerFileName(const ResourceId res_id)
{
	return resource_manager.getResourceContainerFileName(res_id);
}


//
// Res_GetResourceName
//
// Looks for the name of the resource lump that matches id. If the lump is not
// found, an empty string is returned.
//
const OString& Res_GetResourceName(const ResourceId res_id)
{
	return resource_manager.getResourcePath(res_id).last();
}


//
// Res_CheckResource
//
// Verifies that the given ResourceId matches a valid resource lump.
//
bool Res_CheckResource(const ResourceId res_id)
{
	return resource_manager.validateResourceId(res_id);
}
	

//
// Res_GetResourceSize
//
// Returns the length of the resource lump that matches res_id. If the lump is
// not found, 0 is returned.
//
uint32_t Res_GetResourceSize(const ResourceId res_id)
{
	return resource_manager.getResourceSize(res_id);
}


//
// Res_CheckMap
//
// Checks if a given map name exists in the resource files. Internally, map
// names are stored as marker lumps and put into their own namespace.
//
bool Res_CheckMap(const OString& mapname)
{
	ResourcePath directory = Res_MakeResourcePath(mapname, maps_directory_name);
	const ResourceId res_id = Res_GetResourceId(mapname, directory);
	return resource_manager.validateResourceId(res_id);
}


//
// Res_GetMapResourceId
//
// Returns the ResourceId of a lump belonging to the given map. Internally, the
// lumps for a particular map are stored in that map's namespace and
// only the map lumps from the same resource file as the map marker
// should be used.
//
const ResourceId Res_GetMapResourceId(const OString& lump_name, const OString& mapname)
{
	ResourcePath directory = Res_MakeResourcePath(mapname, maps_directory_name);
	const ResourceId map_marker_res_id = Res_GetResourceId(mapname, directory);
	const ResourceId map_lump_res_id = Res_GetResourceId(lump_name, directory);

	if (resource_manager.validateResourceId(map_lump_res_id) &&
		resource_manager.validateResourceId(map_marker_res_id) &&
		resource_manager.getResourceContainerFileName(map_lump_res_id) ==
		resource_manager.getResourceContainerFileName(map_marker_res_id));
		return map_lump_res_id;
	return ResourceId::INVALID_ID;
}


//
// Res_LoadResource
//
// Allocates space on the zone heap for the resource lump that matches res_id
// and reads it into the newly allocated memory. An entry in the resource
// cache table is added so that the next time this resource is read, it can
// be fetched from the cache table instead of from the resource file.
//
// The tag parameter is used to specify the allocation tag type to pass
// to Z_Malloc.
//
void* Res_LoadResource(const ResourceId res_id, int tag)
{
	return (void*)resource_manager.loadResourceData(res_id, tag);
}


//
// Res_ReleaseResource
//
// Frees the memory allocated for the lump's cached data.
//
void Res_ReleaseResource(const ResourceId res_id)
{
	resource_manager.releaseResourceData(res_id);
}


BEGIN_COMMAND(dump_resources)
{
	resource_manager.dump();
}
END_COMMAND(dump_resources)
