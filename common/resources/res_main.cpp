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
// NOTES:
// Resource names must be unique within the same resource file and namespace.
// This allows for resource names to be duplicated in different resource files
// so that resources may override those in previously loaded files. Resource
// names may also be duplicated within the same file if the resources belong
// to different namespaces. This allows a texture and a flat to have the same
// name without ambiguity.
//
//-----------------------------------------------------------------------------

#include "resources/res_main.h"
#include "resources/res_texture.h"
#include "resources/res_fileaccessor.h"
#include "resources/res_filelib.h"
#include "resources/res_identifier.h"
#include "resources/res_container.h"
#include "resources/res_cache.h"
#include "resources/res_resourceloader.h"

#include "c_dispatch.h"

#include "i_system.h"

static ResourceManager resource_manager;

//
// Res_CheckFileHelper
//
// Helper function that opens a file and reads the first length bytes of
// the file and then passes the data to the function func and returns the
// result.
//
static bool Res_CheckFileHelper(const OString& filename, bool (*func)(const uint8_t*, size_t), size_t length=0)
{
	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		return false;

	const size_t file_size = M_FileLength(filename);
	if (length > file_size)
		return false;

	if (length == 0)
		length = file_size;

	uint8_t* data = new uint8_t[length];
	size_t read_cnt = fread(data, 1, length, fp);
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
	const size_t length = 4;	// length of WAD identifier ("IWAD" or "PWAD")
	return Res_CheckFileHelper(filename, &Res_ValidateWadData, length);
}


//
// Res_IsZipFile
//
// Checks that the first four bytes of a file matches a zip file signature
//
bool Res_IsZipFile(const OString& filename)
{
	const size_t length = 4;	// length of zip identifier
	return Res_CheckFileHelper(filename, &Res_ValidateZipData, length);
}


//
// Res_IsDehackedFile
//
// Checks that the first line of the file contains a DeHackEd header.
//
bool Res_IsDehackedFile(const OString& filename)
{
	const size_t length = 32; 	// long enough for DeHackEd identifier
	return Res_CheckFileHelper(filename, &Res_ValidateDehackedData, length);
}


// ============================================================================
//
// RawResourceAccessor class implementations
//
// ============================================================================


const ResourcePath& RawResourceAccessor::getResourcePath(const ResourceId res_id) const
{
	return mResourceManager->getResourcePath(res_id);
}

uint32_t RawResourceAccessor::getResourceSize(const ResourceId res_id) const
{
	const ResourceContainer* container = mResourceManager->getResourceContainer(res_id);
	return container->getResourceSize(res_id);
}

void RawResourceAccessor::loadResource(const ResourceId res_id, void* data, uint32_t size) const
{
	const ResourceContainer* container = mResourceManager->getResourceContainer(res_id);
	container->loadResource(data, res_id, size);
}


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
	closeAllResourceContainers();
}


//
// ResourceManager::addResourceContainer
//
void ResourceManager::addResourceContainer(
				ResourceContainer* container,
				ResourceContainer* parent,
				const ResourcePath& base_path,
				const std::string& filename)
{
	mResourceContainers.push_back(ResourceContainerRecord());
	ResourceContainerRecord& record = mResourceContainers.back();

	record.mResourceContainer = container;
	record.mParent = parent;
	record.mBasePath = base_path;
	if (parent)
		record.mFileName = getResourceContainerRecord(parent)->mFileName + ":" + filename;
	else
		record.mFileName = filename;

	Printf(PRINT_HIGH, "adding %s (%d %s)\n",
					record.mFileName.c_str(),
					container->getResourceCount(),
					container->getResourceCount() == 1 ? "lump" : "lumps");

	container->addResources(this);
}


//
// ResourceManager::openResourceFile
//
// Opens a resource file and caches the directory of lump names for queries.
//
void ResourceManager::openResourceFile(const OString& path)
{
	ResourceContainer* container = NULL;

	if (M_IsFile(path))
	{
		std::string ext;
		M_ExtractFileExtension(path, ext);

		if (Res_IsWadFile(path))
			container = new WadResourceContainer(path);
		else if (Res_IsZipFile(path))
			container = new ZipResourceContainer(path);
		else
			container = new SingleLumpResourceContainer(path);
	}
	else if (M_IsDirectory(path))
	{
		container = new DirectoryResourceContainer(path);
	}

	if (!container)
		return;

	addResourceContainer(container, NULL, global_directory_name, path);

	mResourceContainerFileNames.insert(std::make_pair(size_t(container), path));
	mResourceFileNames.push_back(path);

}


//
// ResourceManager::openResourceFiles
//
// Opens a set of resource files and creates a directory of resource path names
// for queries.
//
void ResourceManager::openResourceFiles(const std::vector<std::string>& filenames)
{
	assert(mResourceContainers.empty());
	mResourceContainers.reserve(filenames.size() + 1);

	for (std::vector<std::string>::const_iterator it = filenames.begin(); it != filenames.end(); ++it)
		openResourceFile(*it);

	ResourceContainer* container = new TextureManager(this);
	addResourceContainer(container, NULL, global_directory_name, "Unified Texture System");

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

	for (ResourceContainerRecordTable::iterator it = mResourceContainers.begin(); it != mResourceContainers.end(); ++it)
		delete it->mResourceContainer;
	mResourceContainers.clear();

	mResourceContainerFileNames.clear();

	mResourceFileNames.clear();
	mResourceFileHashes.clear();

	delete mCache;
	mCache = NULL;

	mNameTranslator.clear();
}


//
// ResourceManager::addResource
//
// Adds a resource lump to the lookup tables and assigns it a new ResourceId.
//
const ResourceId ResourceManager::addResource(
		const ResourcePath& path,
		const ResourceContainer* container,
		const ResourceLoader* loader)
{
	mResources.push_back(ResourceRecord());
	ResourceRecord& res_rec = mResources.back();
	const ResourceId res_id = getResourceId(&res_rec);

	res_rec.mPath = path;
	res_rec.mResourceContainer = container;
	res_rec.mResourceLoader = loader;

	mNameTranslator.addTranslation(path, res_id);

	return res_id;
}


//
// ResourceManager::getAllResourceIds
//
const ResourceIdList ResourceManager::getAllResourceIds() const
{
	ResourceIdList res_id_list;
	res_id_list.reserve(mResources.size());
	for (size_t i = 0; i < mResources.size(); i++)
		res_id_list.push_back(ResourceId(i));
	return res_id_list;
}


//
// ResourceManager::listResourceDirectory
//
// Lists the path of all resources which reside in the provided path.
//
const ResourcePathList ResourceManager::listResourceDirectory(const ResourcePath& path) const
{
	ResourcePathList path_list;
	for (size_t i = 0; i < mResources.size(); i++)
	{
		const ResourcePath& this_path = mResources[i].mPath;
		if (Res_DoesPathStartWithBase(path, this_path))
			path_list.push_back(this_path);
	}
	return path_list;
}


//
// ResourceManager::getResourceContainerFileName
//
const std::string& ResourceManager::getResourceContainerFileName(const ResourceId res_id) const
{
	const ResourceContainer* container = getResourceContainer(res_id);
	if (container)
		return getResourceContainerRecord(container)->mFileName;
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
	{
		if (res_rec->mResourceLoader)
			return res_rec->mResourceLoader->size();

		// default implementation
		const ResourceContainer* container = getResourceContainer(res_id);
		return container->getResourceSize(res_id);
	}
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
const void* ResourceManager::loadResourceData(const ResourceId res_id, zoneTag_e tag)
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

			I_BeginRead();			// indicate to the HUD to draw the loading icon (STDISK)

			void* dest = NULL;		// memory location allocated by the cache where the data should be copied to when loaded
			if (res_rec->mResourceLoader)
			{
				// Special loading strategy
				uint32_t size = res_rec->mResourceLoader->size();
				mCache->cacheData(res_id, &dest, size, tag);
				res_rec->mResourceLoader->load(dest);
			}
			else
			{
				// Default loading strategy
				const ResourceContainer* container = res_rec->mResourceContainer;
				uint32_t size = container->getResourceSize(res_id);
				mCache->cacheData(res_id, &dest, size, tag);
				container->loadResource(dest, res_id, size);
			}

			I_EndRead();			// indicate to the HUD to stop drawing the loading icon (STDISK)

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
// ResourceManager::releaseResourceData
//
const std::vector<OMD5Hash>& ResourceManager::getResourceFileHashes() const
{
	if (mResourceFileHashes.size() != mResourceFileNames.size())
	{
		mResourceFileHashes.clear();
		mResourceFileHashes.reserve(mResourceFileNames.size());
		for (size_t i = 0; i < mResourceFileNames.size(); i++)
			mResourceFileHashes.push_back(Res_MD5(mResourceFileNames[i]));
	}
	return mResourceFileHashes;
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

		const ResourceContainer* container = res_rec.mResourceContainer;
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
	Printf(PRINT_HIGH, "Res_OpenResourceFiles: Init resource data.\n");
	resource_manager.openResourceFiles(filenames);
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
const std::vector<OMD5Hash>& Res_GetResourceFileHashes()
{
	return resource_manager.getResourceFileHashes();
}


//
// Res_ListResourceDirectory
//
// Returns a list of all resource names in the given resource directory.
//
const ResourcePathList Res_ListResourceDirectory(const ResourcePath& path)
{
	return resource_manager.listResourceDirectory(path);
}


//
// Res_GetResourceId
//
const ResourceId Res_GetResourceId(const ResourcePath& path)
{
	if (path.last().length() > 8)
		Printf(PRINT_HIGH, "Warning: Res_GetResourceId searching for resource with name longer than 8 chars: %s", path.last().c_str());
	return resource_manager.getResourceId(path);
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
const ResourceId Res_GetResourceId(const OString& name, const ResourcePath& directory)
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

	if (resource_manager.checkForSameContainer(map_marker_res_id, map_lump_res_id))
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
const void* Res_LoadResource(const ResourceId res_id, zoneTag_e tag)
{
	return resource_manager.loadResourceData(res_id, tag);
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
