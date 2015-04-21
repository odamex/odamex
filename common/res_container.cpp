// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: res_container.cpp $
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
		mResourceContainerId(container_id), mFile(file)
{
	if (getLumpCount() > 0)
	{
		const OString& filename = mFile->getFileName();

		// the filename serves as the lump_name
		OString lump_name = M_ExtractFileName(OStringToUpper(filename));

		// rename lump_name to DEHACKED if it's a DeHackEd file
		if (Res_IsDehackedFile(filename))
			lump_name = "DEHACKED";

		const LumpId lump_id = 0;
		ResourcePath path = Res_MakeResourcePath(lump_name, global_directory_name);
		manager->addResource(path, this, lump_id);
	}
}


//
// SingleLumpResourceContainer::getLumpCount
//
uint32_t SingleLumpResourceContainer::getLumpCount() const
{
	return mFile->valid() ? 1 : 0;
}


//
// SingleLumpResourceContainer::getLumpLength
//
uint32_t SingleLumpResourceContainer::getLumpLength(const LumpId lump_id) const
{
	if (lump_id < getLumpCount())
		return mFile->size();
	return 0;
}


//
// SingleLumpResourceContainer::readLump
//
uint32_t SingleLumpResourceContainer::readLump(const LumpId lump_id, void* data, uint32_t length) const
{
	length = std::min(length, getLumpLength(lump_id));
	if (length > 0)
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
	uint32_t file_length = mFile->size();

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
	const uint32_t wad_lump_record_length = 4 + 4 + 8;

	uint32_t wad_table_length = wad_lump_count * wad_lump_record_length;
	if (wad_table_offset < 12 || wad_table_length + wad_table_offset > file_length)
	{
		cleanup();
		return;
	}

	// read the WAD lump directory
	mDirectory = new ContainerDirectory(wad_lump_count);

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
			mDirectory->addEntryInfo(OStringToUpper(name, 8), length, offset);
	}


	OString map_name;

	// Examine each lump and decide which path it belongs in
	// and then register it with the resource manager.
	for (ContainerDirectory::const_iterator it = mDirectory->begin(); it != mDirectory->end(); ++it)
	{
		const LumpId wad_lump_num = mDirectory->getLumpId(it);

		uint32_t offset = mDirectory->getOffset(wad_lump_num);
		uint32_t length = mDirectory->getLength(wad_lump_num);
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
					if (name.compare(0, 2, "DP") == 0 && Res_ValidatePCSpeakerSoundData(data, length))
						path = sounds_directory_name;
					else if (name.compare(0, 2, "DS") == 0 && Res_ValidateSoundData(data, length))
						path = sounds_directory_name;
				}

				delete [] data;
			}
			
			path += name;
			manager->addResource(path, this, wad_lump_num);
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
// WadResourceContainer::getLumpCount
//
// Returns the number of lumps in the WAD file or returns 0 if
// the WAD file is invalid.
//
uint32_t WadResourceContainer::getLumpCount() const
{
	if (mDirectory)
		return mDirectory->size();
	return 0;
}


//
// WadResourceContainer::getLumpLength
//
uint32_t WadResourceContainer::getLumpLength(const LumpId lump_id) const
{
	if (mDirectory && mDirectory->validate(lump_id))
		return mDirectory->getLength(lump_id);
	return 0;
}


//
// WadResourceContainer::readLump
//
uint32_t WadResourceContainer::readLump(const LumpId lump_id, void* data, uint32_t length) const
{
	length = std::min(length, getLumpLength(lump_id));
	if (length > 0)
	{
		uint32_t offset = mDirectory->getOffset(lump_id);
		mFile->seek(offset);
		return mFile->read(data, length); 
	}
	return 0;
}

VERSION_CONTROL (res_container_cpp, "$Id: res_container.cpp $")

