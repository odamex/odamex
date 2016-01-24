// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_container.cpp $
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

#include "res_container.h"
#include "res_main.h"
#include "res_fileaccessor.h"

#include "m_ostring.h"
#include "w_ident.h"

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
		mResourceContainerId(container_id), mFile(file),
		mResourceId(ResourceManager::RESOURCE_NOT_FOUND)
{
	if (mFile->valid())
	{
		const OString& filename = mFile->getFileName();

		// the filename serves as the lump_name
		OString lump_name = M_ExtractFileName(OStringToUpper(filename));

		// rename lump_name to DEHACKED if it's a DeHackEd file
		if (Res_IsDehackedFile(filename))
			lump_name = "DEHACKED";

		ResourcePath path = Res_MakeResourcePath(lump_name, global_directory_name);
		mResourceId = manager->addResource(path, this);
	}
}


//
// SingleLumpResourceContainer::getResourceCount
//
uint32_t SingleLumpResourceContainer::getResourceCount() const
{
	return mFile->valid() ? 1 : 0;
}


//
// SingleLumpResourceContainer::getResourceSize
//
uint32_t SingleLumpResourceContainer::getResourceSize(const ResourceId res_id) const
{
	if (res_id == mResourceId)
		return mFile->size();
	return 0;
}


//
// SingleLumpResourceContainer::loadResource
//
uint32_t SingleLumpResourceContainer::loadResource(void* data, const ResourceId res_id, uint32_t size) const
{
	size = std::min(size, getResourceSize(res_id));
	if (size > 0)
		return mFile->read(data, size); 
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
// will be registered and getResourceCount() will return 0.
//
WadResourceContainer::WadResourceContainer(
	FileAccessor* file,
	const ResourceContainerId& container_id,
	ResourceManager* manager) :
		mResourceContainerId(container_id),
		mFile(file),
		mDirectory(NULL),
		mLumpIdLookup(256),
		mIsIWad(false)
{
	mDirectory = readWadDirectory();
	if (!mDirectory)
	{
		cleanup();
		return;
	}

	OString map_name;

	// Examine each lump and decide which path it belongs in
	// and then register it with the resource manager.
	for (ContainerDirectory::const_iterator it = mDirectory->begin(); it != mDirectory->end(); ++it)
	{
		const LumpId lump_id = mDirectory->getLumpId(it);

		uint32_t offset = mDirectory->getOffset(lump_id);
		uint32_t length = mDirectory->getLength(lump_id);
		const OString& name = mDirectory->getName(lump_id);
		ResourcePath path = global_directory_name;

		// check that the lump doesn't extend past the end of the file
		if (offset + length <= mFile->size())
		{
			// [SL] Determine if this is a map marker (by checking if a map-related lump such as
			// SEGS follows this one). If so, move all of the subsequent map lumps into
			// this map's directory.
			bool is_map_lump = Res_IsMapLumpName(name);
			bool is_map_marker = !is_map_lump && Res_IsMapLumpName(mDirectory->next(lump_id));

			if (is_map_marker)
				map_name = name;
			else if (!is_map_lump)
				map_name.clear();

			// add the lump to the appropriate namespace
			if (!map_name.empty())
				path = Res_MakeResourcePath(map_name, maps_directory_name);
			else if (mDirectory->between(lump_id, "F_START", "F_END") ||
					mDirectory->between(lump_id, "FF_START", "FF_END"))
				path = flats_directory_name;
			else if (mDirectory->between(lump_id, "S_START", "S_END") ||
					mDirectory->between(lump_id, "SS_START", "SS_END"))
				path = sprites_directory_name;
			else if (mDirectory->between(lump_id, "P_START", "P_END") ||
					mDirectory->between(lump_id, "PP_START", "PP_END"))
				path = patches_directory_name;
			else if (mDirectory->between(lump_id, "C_START", "C_END"))
				path = colormaps_directory_name;
			else if (mDirectory->between(lump_id, "TX_START", "TX_END"))
				path = textures_directory_name;
			else if (mDirectory->between(lump_id, "HI_START", "HI_END"))
				path = hires_directory_name;
			else
			{
				// Examine the lump to identify its type
				uint8_t* data = new uint8_t[length];
				mFile->seek(offset);
				if (mFile->read(data, length) == length)
				{
					if (name.compare(0, 2, "DP") == 0 && Res_ValidatePCSpeakerSoundData(data, length))
						path = sounds_directory_name;
					else if (name.compare(0, 2, "DS") == 0 && Res_ValidateSoundData(data, length))
						path = sounds_directory_name;
				}

				delete [] data;
			}
			
			path += name;
			const ResourceId res_id = manager->addResource(path, this);
			mLumpIdLookup.insert(std::make_pair(res_id, lump_id));
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
// WadResourceContainer::readWadDirectory
//
// Reads the directory of a WAD file and returns a new instance of
// ContainerDirectory if successful or NULL otherwise.
//
ContainerDirectory* WadResourceContainer::readWadDirectory()
{
	uint32_t magic;
	if (!mFile->read(&magic))
		return NULL;

	magic = LELONG(magic);
	if (magic != ('I' | ('W' << 8) | ('A' << 16) | ('D' << 24)) && 
		magic != ('P' | ('W' << 8) | ('A' << 16) | ('D' << 24)))
		return NULL;

	int32_t wad_lump_count;
	if (!mFile->read(&wad_lump_count))
		return NULL;

	wad_lump_count = LELONG(wad_lump_count);
	if (wad_lump_count < 1)
		return NULL;

	int32_t wad_table_offset;
	if (!mFile->read(&wad_table_offset) || wad_table_offset < 0)
		return NULL;

	wad_table_offset = LELONG(wad_table_offset);

	// The layout for a lump entry is:
	//    int32_t offset
	//    int32_t length
	//    char    name[8]
	const uint32_t wad_lump_record_length = 4 + 4 + 8;

	uint32_t wad_table_length = wad_lump_count * wad_lump_record_length;
	if (wad_table_offset < 12 || wad_table_length + wad_table_offset > mFile->size())
		return NULL;

	// read the WAD lump directory
	ContainerDirectory* directory = new ContainerDirectory(wad_lump_count);

	mFile->seek(wad_table_offset);
	for (uint32_t wad_lump_num = 0; wad_lump_num < (uint32_t)wad_lump_count; wad_lump_num++)
	{
		int32_t offset, length;
		mFile->read(&offset);
		offset = LELONG(offset);
		mFile->read(&length);
		length = LELONG(length);

		char name[8];
		mFile->read(name, 8);

		if (offset >= 12 && length >= 0)
			directory->addEntryInfo(OStringToUpper(name, 8), length, offset);
	}

	return directory;
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
	mLumpIdLookup.clear();
	mIsIWad = false;
}


//
// WadResourceContainer::getResourceCount
//
// Returns the number of lumps in the WAD file or returns 0 if
// the WAD file is invalid.
//
uint32_t WadResourceContainer::getResourceCount() const
{
	if (mDirectory)
		return mDirectory->size();
	return 0;
}


//
// WadResourceContainer::getResourceSize
//
uint32_t WadResourceContainer::getResourceSize(const ResourceId res_id) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory && mDirectory->validate(lump_id))
		return mDirectory->getLength(lump_id);
	return 0;
}


//
// WadResourceContainer::loadResource
//
uint32_t WadResourceContainer::loadResource(void* data, const ResourceId res_id, uint32_t size) const
{
	size = std::min(size, getResourceSize(res_id));
	if (size > 0)
	{
		LumpId lump_id = getLumpId(res_id);
		uint32_t offset = mDirectory->getOffset(lump_id);
		mFile->seek(offset);
		return mFile->read(data, size); 
	}
	return 0;
}

VERSION_CONTROL (res_container_cpp, "$Id: res_container.cpp $")

