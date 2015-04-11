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
#include "res_main.h"
#include "res_fileaccessor.h"
#include "res_container.h"

#include "m_ostring.h"
#include "hashtable.h"
#include <vector>
#include "m_fileio.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "w_ident.h"

#include "i_system.h"
#include "z_zone.h"


// Create a dummy ResourceId instance that will be used to indicate a
// ResourceId is invalid.
const ResourceId ResourceManager::RESOURCE_NOT_FOUND = ResourceId();

static bool Res_CheckWadFile(const OString& filename);
static bool Res_CheckDehackedFile(const OString& filename);

static bool Res_IsMapLumpName(const OString& name);


typedef OHashTable<ResourceId, void*> CacheTable;
static CacheTable cache_table(4096);

static ResourceManager resource_manager;



//
// Res_ValidateFlat
//
// Returns true if the given lump data appears to be a valid flat.
//
static bool Res_ValidateFlat(const void* data, size_t length)
{
	return length == 64 * 64 || length == 128 * 128 || length == 256 * 256;
}


//
// Res_ValidatePatch
//
// Returns true if the given lump data appears to be a valid graphic patch.
//
static bool Res_ValidatePatch(const void* data, size_t length)
{
	if (length > 2 + 2)
	{
		// examine the patch header (width & height)
		int16_t width = LESHORT(*(int16_t*)((uint8_t*)data + 0));
		int16_t height = LESHORT(*(int16_t*)((uint8_t*)data + 2));

		if (width > 0 && height > 0 && width <= 2048 && height <= 2048 && length > (unsigned)(4 + 4 * width))
		{
			// verify all of the entries in the patch's column offset table are valid
			const int32_t* offset_table = (int32_t*)((uint8_t*)data + 4);
			const int32_t min_offset = 4 + 4 * width, max_offset = length - 1;

			for (int i = 0; i < width; i++)
			{
				int32_t offset = LELONG(offset_table[i]);
				if (offset < min_offset || offset > max_offset)
					return false;
			}

			return true;
		}
	}

	return false;
}


//
// Res_ValidateWad
//
// Returns true if the given lump data appears to be a valid WAD file.
// TODO: test more than just identifying string.
//
static bool Res_ValidateWad(const void* data, size_t length)
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
// Res_ValidateDehacked
//
// Returns true if the given lump data appears to be a valid DeHackEd file.
//
static bool Res_ValidateDehacked(const void* data, size_t length)
{
	const char magic_str[] = "Patch File for DeHackEd v";
	size_t magic_len = strlen(magic_str);

	return length >= magic_len && strnicmp((const char*)data, magic_str, magic_len) == 0;
}


//
// Res_ValidatePCSpeakerSound
//
// Returns true if the given lump data appears to be a valid PC speaker
// sound effect. The lump starts with a 4 byte header indicating the length
// of the sound effect in bytes and then is followed by that number of bytes
// of sound data.
//
static bool Res_ValidatePCSpeakerSound(const void* data, size_t length)
{
	int16_t* magic = (int16_t*)((uint8_t*)data + 0);
	int16_t* sample_length = (int16_t*)((uint8_t*)data + 2);
	return length >= 4 && LESHORT(*magic) == 0 && LESHORT(*sample_length) + 4 == (int16_t)length;
}


//
// Res_ValidateSound
//
// Returns true if the given lump data appears to be a valid DMX sound effect.
// The lump starts with a two byte version number (0x0003).
//
// User-created tools to convert to the DMX sound effect format typically
// do not use the correct number of padding bytes in the header and as such
// cannot be used to validate a sound lump.
//
static bool Res_ValidateSound(const void* data, size_t length)
{
	uint16_t* magic = (uint16_t*)((uint8_t*)data + 0);
	return length >= 2 && LESHORT(*magic) == 3;
}


// ****************************************************************************


// ============================================================================
//
// SingleLumpResourceContainer class implementation
//
// ============================================================================

//
// SingleLumpResourceContainer::SingleLumpResourceContainer
//
// Use the filename as the name of the lump and attempt to determine which
// directory to insert the lump into.
//
SingleLumpResourceContainer::SingleLumpResourceContainer(
	FileAccessor* file,
	const ResourceContainerId& container_id,
	ResourceManager* manager) :
		mResourceContainerId(container_id), mFile(file)
{
	if (getLumpCount() > 0)
	{
		const OString& filename = mFile->getFileName();

		// the filename serves as the lump_name
		OString lump_name = M_ExtractFileName(StdStringToUpper(filename));

		// rename lump_name to DEHACKED if it's a DeHackEd file
		if (Res_CheckDehackedFile(filename))
			lump_name = "DEHACKED";

		const LumpId lump_id = static_cast<LumpId>(0);
		ResourcePath path = Res_MakeResourcePath(lump_name, global_directory_name);
		manager->addResource(path, mResourceContainerId, lump_id);
	}
}


//
// SingleLumpResourceContainer::getFileName
//
const OString& SingleLumpResourceContainer::getFileName() const
{
	return mFile->getFileName();
}


//
// SingleLumpResourceContainer::getLumpCount
//
size_t SingleLumpResourceContainer::getLumpCount() const
{
	return mFile->valid() ? 1 : 0;
}


//
// SingleLumpResourceContainer::getLumpLength
//
size_t SingleLumpResourceContainer::getLumpLength(const ResourceId& res_id) const
{
	if (checkLump(res_id))
		return mFile->size();
	return 0;
}


//
// SingleLumpResourceContainer::readLump
//
size_t SingleLumpResourceContainer::readLump(const ResourceId& res_id, void* data, size_t length) const
{
	length = std::min(length, getLumpLength(res_id));
	if (checkLump(res_id))
		return mFile->read(data, length);
	return 0;
}



// ============================================================================
//
// WadResourceContainer class implementation
//
// ============================================================================

//
// WadResourceContainer::WadResourceContainer
//
// Reads the lump directory from the WAD file and registers all of the lumps
// with the ResourceManager. If the WAD file has an invalid directory, no lumps
// will be registered and getLumpCount() will return 0.
//
WadResourceContainer::WadResourceContainer(
	FileAccessor* file,
	const ResourceContainerId& container_id,
	ResourceManager* manager) :
		mResourceContainerId(container_id),
		mFile(file),
		mDirectory(NULL),
		mIsIWad(false)
{
	size_t file_length = mFile->size();

	uint32_t magic;
	if (!mFile->read(&magic))
	{
		cleanup();
		return;
	}
	magic = LELONG(magic);
	if (magic != ('I' | ('W' << 8) | ('A' << 16) | ('D' << 24)) && 
		magic != ('P' | ('W' << 8) | ('A' << 16) | ('D' << 24)))
	{
		cleanup();
		return;
	}

	int32_t wad_lump_count;
	if (!mFile->read(&wad_lump_count))
	{
		cleanup();
		return;
	}
	wad_lump_count = LELONG(wad_lump_count);
	if (wad_lump_count < 1)
	{
		cleanup();
		return;
	}

	int32_t wad_table_offset;
	if (!mFile->read(&wad_table_offset) || wad_table_offset < 0)
	{
		cleanup();
		return;
	}
	wad_table_offset = LELONG(wad_table_offset);

	// The layout for a lump entry is:
	//    int32_t offset
	//    int32_t length
	//    char    name[8]
	const size_t wad_lump_record_length = 4 + 4 + 8;

	size_t wad_table_length = wad_lump_count * wad_lump_record_length;
	if (wad_table_offset < 12 || wad_table_length + wad_table_offset > file_length)
	{
		cleanup();
		return;
	}

	// read the WAD lump directory
	mDirectory = new ContainerDirectory(wad_lump_count);

	mFile->seek(wad_table_offset);
	for (size_t wad_lump_num = 0; wad_lump_num < (size_t)wad_lump_count; wad_lump_num++)
	{
		int32_t offset, length;
		mFile->read(&offset);
		offset = LELONG(offset);
		mFile->read(&length);
		length = LELONG(length);

		char name[8];
		mFile->read(name, 8);

		mDirectory->addEntryInfo(OStringToUpper(OString(name, 8)), length, offset);
	}


	OString map_name;

	// Examine each lump and decide which path it belongs in
	// and then register it with the resource manager.
	for (size_t wad_lump_num = 0; wad_lump_num < (size_t)wad_lump_count; wad_lump_num++)
	{
		size_t offset = mDirectory->getOffset(wad_lump_num);
		size_t length = mDirectory->getLength(wad_lump_num);
		const OString& name = mDirectory->getName(wad_lump_num);
		ResourcePath path = global_directory_name;

		// check that the lump doesn't extend past the end of the file
		if (offset + length <= file_length)
		{
			// [SL] Determine if this is a map marker (by checking if a map-related lump such as
			// SEGS follows this one). If so, move all of the subsequent map lumps into
			// this map's directory.
			bool is_map_lump = Res_IsMapLumpName(name);
			bool is_map_marker = !is_map_lump && Res_IsMapLumpName(mDirectory->next(wad_lump_num));

			if (is_map_marker)
				map_name = name;
			else if (!is_map_lump)
				map_name.clear();

			// add the lump to the appropriate namespace
			if (!map_name.empty())
				path = Res_MakeResourcePath(map_name, maps_directory_name);
			else if (mDirectory->between(wad_lump_num, "F_START", "F_END") ||
					mDirectory->between(wad_lump_num, "FF_START", "FF_END"))
				path = flats_directory_name;
			else if (mDirectory->between(wad_lump_num, "S_START", "S_END") ||
					mDirectory->between(wad_lump_num, "SS_START", "SS_END"))
				path = sprites_directory_name;
			else if (mDirectory->between(wad_lump_num, "P_START", "P_END") ||
					mDirectory->between(wad_lump_num, "PP_START", "PP_END"))
				path = patches_directory_name;
			else if (mDirectory->between(wad_lump_num, "C_START", "C_END"))
				path = colormaps_directory_name;
			else if (mDirectory->between(wad_lump_num, "TX_START", "TX_END"))
				path = textures_directory_name;
			else if (mDirectory->between(wad_lump_num, "HI_START", "HI_END"))
				path = hires_directory_name;
			else
			{
				// Examine the lump to identify its type
				uint8_t* data = new uint8_t[length];
				mFile->seek(offset);
				if (mFile->read(data, length) == length)
				{
					if (name.compare(0, 2, "DP") == 0 && Res_ValidatePCSpeakerSound(data, length))
						path = sounds_directory_name;
					else if (name.compare(0, 2, "DS") == 0 && Res_ValidateSound(data, length))
						path = sounds_directory_name;
				}

				delete [] data;
			}
			
			path += name;
			manager->addResource(path, mResourceContainerId, wad_lump_num);
		}
	}

	mIsIWad = W_IsIWAD(file->getFileName());
}


//
// WadResourceContainer::~WadResourceContainer
//
WadResourceContainer::~WadResourceContainer()
{
	cleanup();
}


//
// WadResourceContainer::cleanup
//
// Frees the memory used by the container.
//
void WadResourceContainer::cleanup()
{
	delete mDirectory;
	mDirectory = NULL;
	mIsIWad = false;
}


//
// WadResourceContainer::getFileName
//
const OString& WadResourceContainer::getFileName() const
{
	return mFile->getFileName();
}


//
// WadResourceContainer::getLumpCount
//
// Returns the number of lumps in the WAD file or returns 0 if
// the WAD file is invalid.
//
size_t WadResourceContainer::getLumpCount() const
{
	if (mDirectory)
		return mDirectory->size();
	return 0;
}


//
// WadResourceContainer::getLumpLength
//
size_t WadResourceContainer::getLumpLength(const ResourceId& res_id) const
{
	if (checkLump(res_id))
		return mDirectory->getLength(res_id.getLumpId());
	return 0;
}


//
// WadResourceContainer::readLump
//
size_t WadResourceContainer::readLump(const ResourceId& res_id, void* data, size_t length) const
{
	length = std::min(length, getLumpLength(res_id));
	if (checkLump(res_id))
	{
		size_t offset = mDirectory->getOffset(res_id.getLumpId());
		mFile->seek(offset);
		return mFile->read(data, length);
	}
	return 0;
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
static const size_t initial_lump_count = 4096;

ResourceManager::ResourceManager() :
	mResourceIdLookup(2 * initial_lump_count),
	mResourcePathLookup(2 * initial_lump_count)
{
	mResourceIds.reserve(initial_lump_count);
}


//
// ResourceManager::~ResourceManager
//
ResourceManager::~ResourceManager()
{
	closeAllResourceFiles();
}


//
// ResourceManager::openResourceFile
//
// Opens a resource file and caches the directory of lump names for queries.
// 
void ResourceManager::openResourceFile(const OString& filename)
{
	ResourceContainerId container_id = mContainers.size();

	if (!M_FileExists(filename))
		return;

	FileAccessor* file = new DiskFileAccessor(filename);
	ResourceContainer* container = NULL;

	if (Res_CheckWadFile(filename))
		container = new WadResourceContainer(file, container_id, this);
	else
		container = new SingleLumpResourceContainer(file, container_id, this);

	// check that the resource container has valid lumps
	if (container && container->getLumpCount() == 0)
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
// ResourceManager::closeAllResourceFiles
//
// Closes all open resource files. This should be called prior to switching
// to a new set of resource files.
//
void ResourceManager::closeAllResourceFiles()
{
	mResourcePathLookup.clear();
	mResourceIds.clear();

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
const ResourceId& ResourceManager::addResource(
		const ResourcePath& path,
		const ResourceContainerId& container_id,
		const LumpId& lump_id)
{
	size_t index = mResourceIds.size();
	mResourceIds.push_back(ResourceId(path, container_id, lump_id));
	ResourceId& res_id = mResourceIds[index];
	res_id.mIndex = index;

	// Add the index to the ResourceIdLookupTable
	// ResourceIdLookupTable uses a ResourcePath key and value that is a std::vector
	// of indices to the mResourceIds std::vector.
	ResourceIdLookupTable::iterator it = mResourceIdLookup.find(path);
	if (it != mResourceIdLookup.end())
	{
		// A resource with the same path already exists. Add this ResourceId
		// to the end of the list of ResourceIds with a matching path.
		ResourceIdIndexList& index_list = it->second;
		assert(!index_list.empty());
		index_list.push_back(index);
	}
	else
	{
		// No other resources with the same path exist yet. Create a new list
		// for ResourceIds with a matching path.
		ResourceIdIndexList index_list;
		index_list.push_back(index);
		mResourceIdLookup.insert(std::make_pair(path, index_list));
	}
	
	// Add the path to the ResourcePathLookupTable
	mResourcePathLookup.insert(std::make_pair(index, path));

	assert(res_id.valid());
	assert(getResourceId(path) == res_id);
	return res_id;
}


//
// ResourceManager::getResourceId
//
// Retrieves the ResourceId for a given resource path name. If more than
// one ResourceId match the given path name, the ResouceId from the most
// recently loaded resource file will be returned.
//
const ResourceId& ResourceManager::getResourceId(const ResourcePath& path) const
{
	ResourceIdLookupTable::const_iterator it = mResourceIdLookup.find(path);
	if (it != mResourceIdLookup.end())
	{
		const ResourceIdIndexList& index_list = it->second;
		assert(!index_list.empty());
		size_t index = index_list.back();
		if (index < mResourceIds.size())
			return mResourceIds[index];
	}
	
	return ResourceManager::RESOURCE_NOT_FOUND;
}


//
// ResourceManager::getAllResourceIds
//
// Returns a std::vector of ResourceIds that match a given resource path name.
// If there are no matches, an empty std::vector will be returned.
//
const ResourceIdList ResourceManager::getAllResourceIds(const ResourcePath& path) const
{
	ResourceIdList res_ids;

	ResourceIdLookupTable::const_iterator it = mResourceIdLookup.find(path);
	if (it != mResourceIdLookup.end())
	{
		const ResourceIdIndexList& index_list = it->second;
		assert(!index_list.empty());
		for (ResourceIdIndexList::const_iterator it = index_list.begin(); it != index_list.end(); ++it)
		{
			size_t index = *it;
			if (index < mResourceIds.size())
				res_ids.push_back(mResourceIds[index]);
		}
	}

	return res_ids;
}


//
// ResourceManager::visible
//
// Determine if no other resources have overridden this resource by having
// the same resource path.
//
bool ResourceManager::visible(const ResourceId& res_id) const
{
	const ResourcePath& path = res_id.getResourcePath();
	return getResourceId(path) == res_id;
}


//
// ResourceManager::dump
//
// Print information about each resource in all of the open resource
// files.
//
void ResourceManager::dump() const
{
	for (std::vector<ResourceId>::const_iterator it = mResourceIds.begin(); it != mResourceIds.end(); ++it)
	{
		const ResourceId& res_id = *it;
		assert(res_id.valid());
		assert(res_id.mIndex < mResourceIds.size());
		assert(res_id.mIndex == static_cast<size_t>(&res_id - &mResourceIds[0]));

		const ResourcePath& path = res_id.getResourcePath();
		assert(!OString(path).empty());

		const ResourceContainerId& container_id = res_id.getResourceContainerId();
		assert(container_id < mContainers.size());
		const ResourceContainer* container = mContainers[container_id];
		assert(container);

		Printf(PRINT_HIGH,"0x%04X %c %s [%u] [%s]\n",
				(unsigned int)res_id.mIndex,
				visible(res_id) ? '*' : 'x',
				OString(path).c_str(),
				(unsigned int)container->getLumpLength(res_id),
				container->getFileName().c_str());
	}
}


// ============================================================================
//
// Externally visible functions
//
// ============================================================================

//
// Res_OpenResourceFile
//
// Opens a resource file and caches the directory of lump names for queries.
// 
void Res_OpenResourceFile(const OString& filename)
{
	resource_manager.openResourceFile(filename);
}


//
// Res_CloseAllResourceFiles
//
// Closes all open resource files and clears the cache. This should be called prior to switching
// to a new set of resource files.
//
void Res_CloseAllResourceFiles()
{
	resource_manager.closeAllResourceFiles();

	// free all of the memory used by cached lump data
	cache_table.clear();
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
// Res_CheckFileHelper
//
// Helper function that opens a file and reads the first length bytes of
// the file and then passes the data to the function func and returns the
// result.
//
static bool Res_CheckFileHelper(const OString& filename, bool (*func)(const void*, size_t), size_t length)
{
	FILE* fp = fopen(filename.c_str(), "rb");
	if (fp == NULL)
		return false;

	char* data = new char[length];
	size_t read_cnt = fread(data, 1, length, fp);
	bool valid = read_cnt == length && func(data, length);
	delete [] data;
	fclose(fp);

	return valid;
}


//
// Res_CheckWadFile
//
// Checks that the first four bytes of a file are "IWAD" or "PWAD"
//
static bool Res_CheckWadFile(const OString& filename)
{
	const size_t length = 4;	// length of WAD identifier ("IWAD" or "PWAD")
	return Res_CheckFileHelper(filename, &Res_ValidateWad, length);
}


//
// Res_CheckDehackedFile
//
// Checks that the first line of the file contains a DeHackEd header.
//
static bool Res_CheckDehackedFile(const OString& filename)
{
	const size_t length = strlen("Patch File for DeHackEd v");	// length of DeHackEd identifier
	return Res_CheckFileHelper(filename, &Res_ValidateDehacked, length);
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
const ResourceId& Res_GetResourceId(const OString& name, const OString& directory)
{
	const ResourcePath path = Res_MakeResourcePath(name, directory);
	return resource_manager.getResourceId(path);
}


//
// Res_GetAllResourceIds
//
// Fills the supplied vector with the ResourceId of any lump whose name matches the
// given string. An empty vector indicates that there were no matches.
//
const ResourceIdList Res_GetAllResourceIds(const OString& name, const OString& directory)
{
	const ResourcePath path = Res_MakeResourcePath(name, directory);
	return resource_manager.getAllResourceIds(path);
}


//
// Res_GetLumpName
//
// Looks for the name of the resource lump that matches id. If the lump is not
// found, an empty string is returned.
//
const OString& Res_GetLumpName(const ResourceId& res_id)
{
	if (res_id.valid())
		return res_id.getResourcePath().last();
	
	static const OString empty_string;
	return empty_string;
}


//
// Res_GetResourceFileName
//
// Returns the name of the resource file that the given resource belongs to.
//
const OString& Res_GetResourceFileName(const ResourceId& res_id)
{
	if (res_id.valid())
	{
		const ResourceContainerId& container_id = res_id.getResourceContainerId();
		const ResourceContainer* container = resource_manager.getResourceContainer(container_id);
		if (container)
			return container->getFileName();
	}
	static const OString empty_string;
	return empty_string;
}


//
// Res_CheckLump
//
// Verifies that the given LumpId matches a valid resource lump.
//
bool Res_CheckLump(const ResourceId& res_id)
{
	if (res_id.valid())
	{
		const ResourceContainerId& container_id = res_id.getResourceContainerId();
		const ResourceContainer* container = resource_manager.getResourceContainer(container_id);
		if (container)
			return container->checkLump(res_id);
	}
	return false;
}
	

//
// Res_GetLumpLength
//
// Returns the length of the resource lump that matches res_id. If the lump is
// not found, 0 is returned.
//
size_t Res_GetLumpLength(const ResourceId& res_id)
{
	if (res_id.valid())
	{
		const ResourceContainerId& container_id = res_id.getResourceContainerId();
		const ResourceContainer* container = resource_manager.getResourceContainer(container_id);
		if (container)
			return container->getLumpLength(res_id);
	}
	return 0;
}


//
// Res_IsMapLumpName
//
// Returns true if the given lump name is a name reserved for map lumps.
//
static bool Res_IsMapLumpName(const OString& name)
{
	static const OString things_lump_name("THINGS");
	static const OString linedefs_lump_name("LINEDEFS");
	static const OString sidedefs_lump_name("SIDEDEFS");
	static const OString vertexes_lump_name("VERTEXES");
	static const OString segs_lump_name("SEGS");
	static const OString ssectors_lump_name("SSECTORS");
	static const OString nodes_lump_name("NODES");
	static const OString sectors_lump_name("SECTORS");
	static const OString reject_lump_name("REJECT");
	static const OString blockmap_lump_name("BLOCKMAP");
	static const OString behavior_lump_name("BEHAVIOR");

	const OString uname = OStringToUpper(name);

	return uname == things_lump_name || uname == linedefs_lump_name || uname == sidedefs_lump_name ||
		uname == vertexes_lump_name || uname == segs_lump_name || uname == ssectors_lump_name ||
		uname == nodes_lump_name || uname == sectors_lump_name || uname == reject_lump_name ||
		uname == blockmap_lump_name || uname == behavior_lump_name;
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
	return Res_GetResourceId(mapname, directory).valid();
}


//
// Res_GetMapResourceId
//
// Returns the ResourceId of a lump belonging to the given map. Internally, the
// lumps for a particular map are stored in that map's namespace and
// only the map lumps from the same resource file as the map marker
// should be used.
//
const ResourceId& Res_GetMapResourceId(const OString& lump_name, const OString& mapname)
{
	ResourcePath directory = Res_MakeResourcePath(mapname, maps_directory_name);
	const ResourceId& map_marker_res_id = Res_GetResourceId(mapname, directory);
	const ResourceId& map_lump_res_id = Res_GetResourceId(lump_name, directory);
	if (map_lump_res_id.valid() && map_marker_res_id.valid() &&
		map_lump_res_id.getResourceContainerId() == map_marker_res_id.getResourceContainerId())
		return map_lump_res_id;
	return ResourceManager::RESOURCE_NOT_FOUND;
}


//
// Res_ReadLump
//
// Reads the resource lump that matches res_id and copies its contents into data.
// The number of bytes read is returned, or 0 is returned if the lump
// is not found. The variable data must be able to hold the size of the lump,
// as determined by Res_GetLumpLength.
//
size_t Res_ReadLump(const ResourceId& res_id, void* data)
{
	if (res_id.valid())
	{
		const ResourceContainerId& container_id = res_id.getResourceContainerId();
		const ResourceContainer* container = resource_manager.getResourceContainer(container_id);
		if (container)
			return container->readLump(res_id, data, container->getLumpLength(res_id));
	}
	return 0;
}


//
// Res_CacheLump
//
// Allocates space on the zone heap for the resource lump that matches res_id
// and reads it into the newly allocated memory. An entry in the resource
// cache table is added so that the next time this resource is read, it can
// be fetched from the cache table instead of from the resource file.
//
// The tag parameter is used to specify the allocation tag type to pass
// to Z_Malloc.
//
void* Res_CacheLump(const ResourceId& res_id, int tag)
{
	void* data_ptr = NULL;

	if (res_id.valid())
	{
		data_ptr = cache_table[res_id];

		if (data_ptr)
		{
			Z_ChangeTag(data_ptr, tag);
		}
		else
		{
			void** owner_ptr = &cache_table[res_id];
			size_t lump_length = Res_GetLumpLength(res_id);
			cache_table[res_id] = data_ptr = Z_Malloc(lump_length + 1, tag, owner_ptr);
			Res_ReadLump(res_id, data_ptr);

			uint8_t* terminator = (uint8_t*)data_ptr + lump_length;
			*terminator = 0;
		}
	}
	return data_ptr; 
}



BEGIN_COMMAND(dump_resources)
{
	resource_manager.dump();
}
END_COMMAND(dump_resources)

VERSION_CONTROL (res_main_cpp, "$Id: res_main.cpp $")
