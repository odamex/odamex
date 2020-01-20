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

#include "resources/res_container.h"
#include "resources/res_main.h"
#include "resources/res_resourcepath.h"
#include "resources/res_fileaccessor.h"
#include "resources/res_identifier.h"

#include "m_ostring.h"
#include "w_ident.h"
#include "cmdlib.h"
#include "i_system.h"

#include "zlib.h"


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
// Res_TransformResourcePath
//
// Modifies a file path from a filesystem directory or an archive to meet
// the criteria used by ResourceManager. This includes normalizing
// path separators & truncating the lump name to 8 upper-case characters.
//
static std::string Res_TransformResourcePath(const std::string& path)
{
	std::string new_path(path);

	// Replace native filesystem path separator with the in-game path separator
	for (size_t i = 0; i < new_path.length(); i++)
		if (new_path[i] == '/' || new_path[i] == '\\')
			new_path[i] = ResourcePath::DELIMINATOR;
	// Add leading '/'
	if (new_path[0] != ResourcePath::DELIMINATOR)
		new_path = ResourcePath::DELIMINATOR + new_path;

	// Transform the filename portion of the path into a well-formed lump name.
	// eg, 8 chars, no filename extension, all caps.
	//
	// Note that the "/SCRIPTS/" directory will need the full filename preserved,
	// including extension and > 8 chars
	if (new_path.find("/SCRIPTS/") != 0)
	{
		size_t last_deliminator = new_path.find_last_of(ResourcePath::DELIMINATOR);
		if (last_deliminator != std::string::npos)
		{
			size_t start_of_lump_name = last_deliminator + 1;
			// Trim the filename extension
			new_path.replace(new_path.find_first_of(".", start_of_lump_name), std::string::npos, "");
			// Capitalize the lump name
			std::transform(new_path.begin() + start_of_lump_name, new_path.end(),
							new_path.begin() + start_of_lump_name, toupper);
			// Truncate the lump name to 8 chars
			new_path = new_path.substr(0, start_of_lump_name + 8);

			// Handle a specific use-case where a sprite lump name should contain a backslash
			// character. See https://zdoom.org/wiki/Using_ZIPs_as_WAD_replacement.
			if (new_path.find("/SPRITES/") == 0)
				for (size_t i = start_of_lump_name; i < new_path.length(); i++)
					if (new_path[i] == '^')
						new_path[i] = '\\';
		}
	}
	return new_path;
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
SingleLumpResourceContainer::SingleLumpResourceContainer(FileAccessor* file) :
		mFile(file),
		mResourceId(ResourceId::INVALID_ID)
{ }


//
// SingleLumpResourceContainer::SingleLumpResourceContainer
//
// Use the filename as the name of the lump and attempt to determine which
// directory to insert the lump into.
//
SingleLumpResourceContainer::SingleLumpResourceContainer(const OString& path) :
		SingleLumpResourceContainer(new DiskFileAccessor(path))
{ }


//
// SingleLumpResourceContainer::~SingleLumpResourceContainer
//
SingleLumpResourceContainer::~SingleLumpResourceContainer()
{
	delete mFile;
}


//
// SingleLumpResourceContainer::getResourceCount
//
uint32_t SingleLumpResourceContainer::getResourceCount() const
{
	return mFile->valid() ? 1 : 0;
}


//
// SingleLumpResourceContainer::addResources
//
void SingleLumpResourceContainer::addResources(ResourceManager* manager)
{
	if (mFile->valid())
	{
		const OString& filename = mFile->getFileName();

		// the filename serves as the lump_name
		std::string base_filename;
		M_ExtractFileBase(filename, base_filename);
		OString lump_name = OStringToUpper(base_filename.c_str(), 8);

		// rename lump_name to DEHACKED if it's a DeHackEd file
		if (Res_IsDehackedFile(filename))
			lump_name = "DEHACKED";

		ResourcePath path = Res_MakeResourcePath(lump_name, global_directory_name);
		mResourceId = manager->addResource(path, this);
	}
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
WadResourceContainer::WadResourceContainer(const OString& path) :
		WadResourceContainer(new DiskFileAccessor(path))
{
}


//
// WadResourceContainer::WadResourceContainer
//
// Reads the lump directory from the WAD file and registers all of the lumps
// with the ResourceManager. If the WAD file has an invalid directory, no lumps
// will be registered and getResourceCount() will return 0.
//
// This constructor uses a FileAccessor, allowing the WAD to be read from
// memory or from disk. The WadResourceContainer will own the FileAccessor
// pointer and is responsible for de-allocating it.
//
WadResourceContainer::WadResourceContainer(FileAccessor* file) :
		mFile(file),
		mDirectory(256),
		mLumpIdLookup(256)
{
	if (!readWadDirectory())
		mDirectory.clear();

	buildMarkerRecords();
}


//
// WadResourceContainer::~WadResourceContainer
//
WadResourceContainer::~WadResourceContainer()
{
	delete mFile;
}


//
// WadResourceContainer::readWadDirectory
//
// Reads the directory of a WAD file and returns false if there
// are errors parsing the WAD directory or populates the instance's
// ContainerDirectory if successful.
//
bool WadResourceContainer::readWadDirectory()
{
	// The layout for a WAD header is:
	//    uint32_t magic (IWAD/PWAD)
	//    int32_t  wad_lump_count
	//    int32_t  wad_table_offset
	const uint32_t wad_header_length = 4 + 4 + 4;

	// The layout for a lump entry is:
	//    int32_t offset
	//    int32_t length
	//    char    name[8]
	const uint32_t wad_lump_record_length = 4 + 4 + 8;

	uint8_t header[wad_header_length];
	if (mFile->read(header, wad_header_length) != wad_header_length)
		return false;

	uint32_t magic = LELONG(*(uint32_t*)(header + 0));
	if (magic != ('I' | ('W' << 8) | ('A' << 16) | ('D' << 24)) && 
		magic != ('P' | ('W' << 8) | ('A' << 16) | ('D' << 24)))
		return false;

	int32_t wad_lump_count = LELONG(*(int32_t*)(header + 4));
	uint32_t wad_table_length = wad_lump_count * wad_lump_record_length;
	if (wad_lump_count < 1)
		return false;

	int32_t wad_table_offset = LELONG(*(int32_t*)(header + 8));
	if (wad_table_offset < (int32_t)wad_header_length || wad_table_length + wad_table_offset > mFile->size())
		return false;

	// read the WAD lump directory
	uint8_t* wad_directory = new uint8_t[wad_table_length];

	mFile->seek(wad_table_offset);

	if (mFile->read(wad_directory, wad_table_length) == wad_table_length)
	{
		mDirectory.reserve(wad_lump_count);

		const uint8_t* ptr = wad_directory;
		for (uint32_t wad_lump_num = 0; wad_lump_num < (uint32_t)wad_lump_count; wad_lump_num++, ptr += wad_lump_record_length)
		{
			WadDirectoryEntry entry;
			entry.offset = LELONG(*(int32_t*)(ptr + 0));
			entry.length = LELONG(*(int32_t*)(ptr + 4));
			entry.path = OStringToUpper((char*)(ptr + 8), 8);
			mDirectory.addEntry(entry);
		}

		delete [] wad_directory;
		return true;
	}

	delete [] wad_directory;
	return false;
}


//
// WadResourceContainer::buildMarkerRecords
//
void WadResourceContainer::buildMarkerRecords()
{
	OString last_marker_prefix;
	LumpId last_lump_id = mDirectory.INVALID_LUMP_ID;

	for (ContainerDirectory<WadDirectoryEntry>::const_iterator it = mDirectory.begin(); it != mDirectory.end(); ++it)
	{
		const WadDirectoryEntry& entry = *it;
		if (isMarker(entry.path))
		{
			const OString current_marker_prefix(getMarkerPrefix(entry.path));
			LumpId current_lump_id = mDirectory.getLumpId(it);

			// TODO: attempt to repair broken WADs that have missing markers

			if (getMarkerType(entry.path) == END_MARKER)
			{
				MarkerRange range;
				range.start = last_lump_id;
				range.end = current_lump_id;
				mMarkers[current_marker_prefix] = range;
			}
			last_marker_prefix = current_marker_prefix;
			last_lump_id = current_lump_id;
		}
	}
}


//
// WadResourceContainer::assignPathBasedOnMarkers
//
// Determines which markers a lump is between (if any) and assigns a
// path for the resource.
//
const ResourcePath& WadResourceContainer::assignPathBasedOnMarkers(LumpId lump_id) const
{
	for (MarkerRangeLookupTable::const_iterator it = mMarkers.begin(); it != mMarkers.end(); ++it)
	{
		const MarkerRange& range = it->second;
		if (lump_id > range.start && lump_id < range.end)
		{
			const OString& prefix = it->first;
			if (prefix == "F" || prefix == "FF")
				return flats_directory_name;
			if (prefix == "S" || prefix == "SS")
				return sprites_directory_name;
			if (prefix == "P" || prefix == "PP")
				return patches_directory_name;
			if (prefix == "C")
				return colormaps_directory_name;
			if (prefix == "TX")
				return textures_directory_name;
			if (prefix == "HI")
				return hires_directory_name;
		}
	}
	return global_directory_name;
}


//
// WadResourceContaer::isLumpMapMarker
//
// Returns true if the given lump is a map marker.
//
// The logic is as follows:
//   if the next lump has a map lump name (like SSECTORS or THINGS)
//   and the current lump does not have a map lump name, the
//   current lump is a map marker.
//
bool WadResourceContainer::isLumpMapMarker(LumpId lump_id) const
{
	if (mDirectory.validate(lump_id))
		if (Res_IsMapLumpName(mDirectory.getEntry(lump_id)->path))
			return false;

	LumpId next_lump_id = lump_id + 1;
	if (mDirectory.validate(next_lump_id))
		return Res_IsMapLumpName(mDirectory.getEntry(next_lump_id)->path);

	return false;
}


//
// WadResourceContainer::getResourceCount
//
// Returns the number of lumps in the WAD file or returns 0 if
// the WAD file is invalid.
//
uint32_t WadResourceContainer::getResourceCount() const
{
	return mDirectory.size();
}


//
// WadResourceContainer::addResources
//
void WadResourceContainer::addResources(ResourceManager* manager)
{
	OString map_name;

	for (ContainerDirectory<WadDirectoryEntry>::const_iterator it = mDirectory.begin(); it != mDirectory.end(); ++it)
	{
		const WadDirectoryEntry& entry = *it;
		const LumpId lump_id = mDirectory.getLumpId(it);

		ResourcePath base_path = global_directory_name;

		// check that the lump doesn't extend past the end of the file
		if (entry.offset + entry.length <= mFile->size())
		{
			// [SL] Determine if this is a map marker (by checking if a map-related lump such as
			// SEGS follows this one). If so, move all of the subsequent map lumps into
			// this map's directory
			bool is_map_marker = isLumpMapMarker(lump_id);
			bool is_map_lump = Res_IsMapLumpName(entry.path);

			if (is_map_marker)
				map_name = entry.path;
			else if (!is_map_lump)
				map_name.clear();

			// add the lump to the appropriate namespace
			if (!map_name.empty())
				base_path = Res_MakeResourcePath(map_name, maps_directory_name);
			else
				base_path = assignPathBasedOnMarkers(lump_id);

			if (base_path == global_directory_name)
			{
				// Examine the lump to identify its type
				uint8_t* data = new uint8_t[entry.length];
				mFile->seek(entry.offset);
				if (mFile->read(data, entry.length) == entry.length)
				{
					WadResourceIdentifier identifier;
					base_path = identifier.identifyByContents(entry.path, data, entry.length);
				}

				delete [] data;
			}
			
			const ResourcePath full_path = base_path + entry.path;
			const ResourceId res_id = manager->addResource(full_path, this);
			mLumpIdLookup.insert(std::make_pair(res_id, lump_id));
		}
	}
}


//
// WadResourceContainer::getResourceSize
//
uint32_t WadResourceContainer::getResourceSize(const ResourceId res_id) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory.validate(lump_id))
		return mDirectory.getEntry(lump_id)->length;
	return 0;
}


//
// WadResourceContainer::loadResource
//
uint32_t WadResourceContainer::loadResource(void* data, const ResourceId res_id, uint32_t size) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory.validate(lump_id))
	{
		const WadDirectoryEntry* entry = mDirectory.getEntry(lump_id);
		size = std::min(size, entry->length);
		if (size > 0)
		{
			mFile->seek(entry->offset);
			return mFile->read(data, size); 
		}
	}
	return 0;
}


// ============================================================================
//
// DirectoryResourceContainer class implementation
//
// ============================================================================

bool compare_filesystem_directory_entries(const FileSystemDirectoryEntry& entry1, const FileSystemDirectoryEntry& entry2)
{
	return entry1.path < entry2.path;
}

//
// DirectoryResourceContainer::DirectoryResourceContainer
//
//
DirectoryResourceContainer::DirectoryResourceContainer(const OString& path) :
		mPath(),
		mDirectory(256),
		mLumpIdLookup(256)
{
	// trim trailing path separators from the path
	size_t len = path.length();
	while (len != 0 && path[len - 1] == PATHSEPCHAR)
		--len;
	mPath = path.substr(0, len);

	addEntries();
}


//
// DirectoryResourceContainer::~DirectoryResourceContainer
//
DirectoryResourceContainer::~DirectoryResourceContainer()
{
}


//
// DirectoryResourceContainer::getResourceCount
//
// Returns the number of lumps in the WAD file or returns 0 if
// the WAD file is invalid.
//
uint32_t DirectoryResourceContainer::getResourceCount() const
{
	return mDirectory.size();
}


//
// DirectoryResourceContainer::addResources
//
void DirectoryResourceContainer::addResources(ResourceManager* manager)
{
	for (ContainerDirectory<FileSystemDirectoryEntry>::const_iterator it = mDirectory.begin(); it != mDirectory.end(); ++it)
	{
		const FileSystemDirectoryEntry& entry = *it;

		if (isEmbeddedWadFile(entry))
		{
			// Add the WAD as a separate container
			std::string full_path = std::string(mPath) + std::string(entry.path);
			FileAccessor* diskfile = new DiskFileAccessor(full_path);
			ResourceContainer* container = new WadResourceContainer(diskfile);
			manager->addResourceContainer(container, this, global_directory_name, entry.path);
		}
		else
		{
			// Transform the file path to fit with ResourceManager semantics
			const ResourcePath resource_path = Res_TransformResourcePath(entry.path);

			const ResourceId res_id = manager->addResource(resource_path, this);
			const LumpId lump_id = mDirectory.getLumpId(entry.path);
			mLumpIdLookup.insert(std::make_pair(res_id, lump_id));
		}
	}
}


//
// DirectoryResourceContainer::isEmbeddedWadFile
//
bool DirectoryResourceContainer::isEmbeddedWadFile(const FileSystemDirectoryEntry& entry)
{
	std::string ext;
	M_ExtractFileExtension(entry.path, ext);
	if (iequals(ext, "WAD"))
	{
		// will not consider any lump less than 28 in size 
		// (valid wad header, plus at least one lump in the directory)
		if (entry.length < 28)
			return false;

		// only allow embedded WAD files in the root directory
		size_t last_path_separator = entry.path.find_last_of(ResourcePath::DELIMINATOR);
		if (last_path_separator != 0 && last_path_separator != std::string::npos)
			return false;

		return true;
	}

	return false;
}


//
// DirectoryResourceContainer::getResourceSize
//
uint32_t DirectoryResourceContainer::getResourceSize(const ResourceId res_id) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory.validate(lump_id))
		return mDirectory.getEntry(lump_id)->length;
	return 0;
}


//
// DirectoryResourceContainer::loadResource
//
uint32_t DirectoryResourceContainer::loadResource(void* data, const ResourceId res_id, uint32_t size) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory.validate(lump_id))
	{
		size = std::min(size, getResourceSize(res_id));
		if (size > 0)
		{
			const FileSystemDirectoryEntry* entry = mDirectory.getEntry(lump_id);
			std::string full_path = std::string(mPath) + std::string(entry->path);
			DiskFileAccessor file_accessor(full_path);
			return file_accessor.read(data, size);
		}
	}
	return 0;
}


//
// DirectoryResourceContainer::readEntries
//
void DirectoryResourceContainer::addEntries()
{
	std::vector<FileSystemDirectoryEntry> tmp_entries;

	std::vector<std::string> files = M_ListDirectoryContents(mPath, 16);
	for (size_t i = 0 ; i < files.size(); i++)
	{
		tmp_entries.push_back(FileSystemDirectoryEntry());
		FileSystemDirectoryEntry& entry = tmp_entries.back();
		entry.path = files[i].substr(mPath.length());
		entry.length = M_FileLength(files[i]);
	}

	// sort the directory entries by filename
	std::sort(tmp_entries.begin(), tmp_entries.end(), compare_filesystem_directory_entries);

	// add the directory entries to this container
	for (std::vector<FileSystemDirectoryEntry>::const_iterator it = tmp_entries.begin(); it != tmp_entries.end(); ++it)
		mDirectory.addEntry(*it);
}


// ============================================================================
//
// ZipResourceContainer class implementation
//
// ============================================================================

bool compare_zip_directory_entries(const ZipDirectoryEntry& entry1, const ZipDirectoryEntry& entry2)
{
	return entry1.path < entry2.path;
}


//
// ZipResourceContainer::ZipResourceContainer
//
ZipResourceContainer::ZipResourceContainer(const OString& path) :
		ZipResourceContainer(new DiskFileAccessor(path))
{ }


//
// ZipResourceContainer::ZipResourceContainer
//
ZipResourceContainer::ZipResourceContainer(FileAccessor* file) :
		mFile(file),
		mDirectory(256),
		mLumpIdLookup(256)
{
    readCentralDirectory();
	//addEmbeddedResourceContainers(manager);
}


//
// ZipResourceContainer::~ZipResourceContainer
//
ZipResourceContainer::~ZipResourceContainer()
{
	delete mFile;
}


//
// ZipResourceContainer::findEndOfCentralDirectory
//
// Derived from ZDoom, which derived it from Quake 3 unzip.c, where it is
// named unzlocal_SearchCentralDir and is derived from unzip.c by Gilles 
// Vollant, under the following BSD-style license (which ZDoom does NOT 
// properly preserve in its own code):
//
// unzip.h -- IO for uncompress .zip files using zlib
// Version 0.15 beta, Mar 19th, 1998,
//
// Copyright (C) 1998 Gilles Vollant
//
// I WAIT FEEDBACK at mail info@winimage.com
// Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
size_t ZipResourceContainer::findEndOfCentralDirectory() const
{
	const uint32_t BUFREADCOMMENT = 0x400;
    uint8_t buf[BUFREADCOMMENT + 4];
    size_t position_found = 0;
    size_t file_size = mFile->size();
    size_t max_back = std::min<uint32_t>(0xFFFF, file_size);
    size_t back_read = 4;

    mFile->seek(0);

    while (back_read < max_back)
    {
        if (back_read + BUFREADCOMMENT > max_back)
            back_read = max_back;
        else
            back_read += BUFREADCOMMENT;

        size_t read_position = file_size - back_read;
        size_t read_size = std::min<size_t>(BUFREADCOMMENT + 4, file_size - read_position);

        if (!mFile->seek(read_position))
            break;

        if (mFile->read(buf, read_size) != read_size)
            break;

        for (size_t i = read_size - 3; (i--) > 0; )
        {
            if (buf[i] == 'P' && buf[i+1] == 'K' && buf[i+2] == 5 && buf[i+3] == 6)
            {
                position_found = read_position + i;
                break;
            }
        }

        if (position_found != 0)
            break;
    }

    return position_found;
}


//
// ZipResourceContainer::readCentralDirectory
//
// Read the end-of-central-directory data structure, which indicates where
// the central directory starts and how many entries it contains.
//
bool ZipResourceContainer::readCentralDirectory()
{
    // Locate the central directory
    size_t central_dir_end = findEndOfCentralDirectory();
    if (central_dir_end == 0)
		return false; 

	uint8_t* buffer = new uint8_t[ZIP_END_OF_DIR_SIZE];

    // Read the central directory contents
	if (!mFile->seek(central_dir_end) || mFile->read(buffer, ZIP_END_OF_DIR_SIZE) != ZIP_END_OF_DIR_SIZE)
		return false;

	uint16_t disk_num = LESHORT(*(uint16_t*)(buffer + 4));
	uint16_t central_directory_disk_num = LESHORT(*(uint16_t*)(buffer + 6));
	uint16_t num_entries_on_disk = LESHORT(*(uint16_t*)(buffer + 8));
	uint16_t num_entries_total = LESHORT(*(uint16_t*)(buffer + 10));
	uint32_t dir_size = LELONG(*(uint32_t*)(buffer + 12));
	uint32_t dir_offset = LELONG(*(uint32_t*)(buffer + 16));

	delete [] buffer;

    // Basic sanity checks
    // Multi-disk zips aren't supported
	if (num_entries_on_disk != num_entries_total || disk_num != 0 || central_directory_disk_num != 0)
        return false;

	addDirectoryEntries(dir_offset, dir_size, num_entries_total);

    return true;
}


//
// ZipResourceContainer::addEntries
//
// Reads the entries in the central directory and addes them to ResourceManager.
//
void ZipResourceContainer::addDirectoryEntries(uint32_t offset, uint32_t length, uint16_t num_entries)
{
	static const uint32_t ZF_ENCRYPTED = 0x01;

	std::vector<ZipDirectoryEntry> tmp_entries;
	tmp_entries.reserve(num_entries);
	mDirectory.reserve(num_entries);

    uint8_t* buffer = new uint8_t[length];
	if (!mFile->seek(offset) || mFile->read(buffer, length) != length)
		return;

	const uint8_t* ptr = buffer;
	for (uint16_t i = 0; i < num_entries; i++)
	{
		if (ptr + ZIP_CENTRAL_DIR_SIZE - buffer > length)
			break;

		// check for valid entry header
		if (ptr[0] != 'P' || ptr[1] != 'K' || ptr[2] != 1 || ptr[3] != 2)
			break;

		uint16_t flags = LESHORT(*(uint16_t*)(ptr + 8));
		uint16_t method = LESHORT(*(uint16_t*)(ptr + 10));
		uint32_t compressed_length = LELONG(*(uint32_t*)(ptr + 20));
		uint32_t uncompressed_length = LELONG(*(uint32_t*)(ptr + 24));
		uint16_t name_length = LESHORT(*(uint16_t*)(ptr + 28));
		uint16_t extra_length = LESHORT(*(uint16_t*)(ptr + 30));
		uint16_t comment_length = LESHORT(*(uint16_t*)(ptr + 32));
		uint32_t local_offset = LELONG(*(uint32_t*)(ptr + 42));
	
		if (ptr + ZIP_CENTRAL_DIR_SIZE + name_length - buffer > length)
			break;

		std::string name((char*)(ptr + ZIP_CENTRAL_DIR_SIZE + 0), name_length);

		// Convert path separators to '/'
		for (size_t j = 0; j < name.length(); j++)
			if (name[j] == '\\' || name[j] == '/')
				name[j] = ResourcePath::DELIMINATOR;

		ptr += ZIP_CENTRAL_DIR_SIZE + name_length + extra_length + comment_length;

		// skip directories
		if (name[name_length - 1] == ResourcePath::DELIMINATOR && uncompressed_length == 0)
			continue;

		// skip unsupported compression methods
		if (method != METHOD_DEFLATE && method != METHOD_STORED)
			continue;

		// skip encrypted entries
		if (flags & ZF_ENCRYPTED)
			continue;

		ZipDirectoryEntry entry;
		entry.path = name;
		entry.length = uncompressed_length;
		entry.compression_method = method;
		entry.compressed_length = compressed_length;
		entry.local_offset = local_offset;

		tmp_entries.push_back(entry);
	}
    delete [] buffer;

	// sort the central directory entries by filename
	std::sort(tmp_entries.begin(), tmp_entries.end(), compare_zip_directory_entries);

	for (std::vector<ZipDirectoryEntry>::const_iterator it = tmp_entries.begin(); it != tmp_entries.end(); ++it)
		mDirectory.addEntry(*it);
}


//
// ZipResourceContainer::calculateEntryOffset
//
// Determines the offset into the zipfile for the start of the
// compressed data for the given entry.
//
uint32_t ZipResourceContainer::calculateEntryOffset(const ZipDirectoryEntry* entry) const
{
    uint32_t offset = INVALID_OFFSET;
    uint8_t* buffer = new uint8_t[ZIP_LOCAL_FILE_SIZE];

    if (mFile->seek(entry->local_offset) && mFile->read(buffer, ZIP_LOCAL_FILE_SIZE) == ZIP_LOCAL_FILE_SIZE)
    {
        if (buffer[0] == 'P' && buffer[1] == 'K' && buffer[2] == 3 && buffer[3] == 4)
        {
            uint16_t name_length = LESHORT(*(uint16_t*)(buffer + 26));
            uint16_t extra_length = LESHORT(*(uint16_t*)(buffer + 28));
            offset = entry->local_offset + ZIP_LOCAL_FILE_SIZE + name_length + extra_length;
        }
    }

    delete [] buffer;
    return offset;
}


//
// ZipResourceContainer::addResources
//
void ZipResourceContainer::addResources(ResourceManager* manager)
{
	for (ContainerDirectory<ZipDirectoryEntry>::const_iterator it = mDirectory.begin(); it != mDirectory.end(); ++it)
	{
		const ZipDirectoryEntry& entry = *it;

		if (isEmbeddedWadFile(entry))
		{
			// Load the WAD into memory and add it as a separate container
			uint8_t* data = new uint8_t[entry.length];
			loadEntryData(&entry, data, entry.length);

			FileAccessor* memfile = new MemoryFileAccessor(entry.path, data, entry.length);

			ResourceContainer* container = new WadResourceContainer(memfile);
			manager->addResourceContainer(container, this, global_directory_name, entry.path);
		}
		else
		{
			const ResourcePath resource_path = Res_TransformResourcePath(entry.path);

			const ResourceId res_id = manager->addResource(resource_path, this);
			const LumpId lump_id = mDirectory.getLumpId(entry.path);
			mLumpIdLookup.insert(std::make_pair(res_id, lump_id));
		}
	}
}


//
// ZipResourceContainer::isEmbeddedWadFile
//
bool ZipResourceContainer::isEmbeddedWadFile(const ZipDirectoryEntry& entry)
{
	std::string ext;
	M_ExtractFileExtension(entry.path, ext);
	if (iequals(ext, "WAD"))
	{
		// will not consider any lump less than 28 in size 
		// (valid wad header, plus at least one lump in the directory)
		if (entry.length < 28)
			return false;

		// only allow embedded WAD files in the root directory
		size_t last_path_separator = entry.path.find_last_of(ResourcePath::DELIMINATOR);
		if (last_path_separator != 0 && last_path_separator != std::string::npos)
			return false;

		return true;
	}

	return false;
}


//
// ZipResourceContainer::getResourceCount
//
uint32_t ZipResourceContainer::getResourceCount() const
{
	return mDirectory.size();
}


//
// ZipResourceContainer::getResourceSize
//
uint32_t ZipResourceContainer::getResourceSize(const ResourceId res_id) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory.validate(lump_id))
		return mDirectory.getEntry(lump_id)->length;
	return 0;
}


//
// ZipResourceContainer::loadResource
//
uint32_t ZipResourceContainer::loadResource(void* data, const ResourceId res_id, uint32_t size) const
{
	LumpId lump_id = getLumpId(res_id);
	if (mDirectory.validate(lump_id))
	{
		const ZipDirectoryEntry* entry = mDirectory.getEntry(lump_id);
		size = std::min(size, entry->length);
		if (size > 0)
			return loadEntryData(entry, data, size);
	}
	return 0;
}


//
// ZipResourceContainer::loadEntryData
//
uint32_t ZipResourceContainer::loadEntryData(const ZipDirectoryEntry* entry, void* data, uint32_t size) const
{
	uint32_t offset = calculateEntryOffset(entry);
	if (mFile->seek(offset))
	{
		if (entry->compression_method == METHOD_STORED)
			return mFile->read(data, size);

		if (entry->compression_method == METHOD_DEFLATE)
		{
			size_t total_bytes_read = 0;
			int status_code;
			z_stream mStream;
			mStream.zalloc = NULL;
			mStream.zfree  = NULL;

			status_code = inflateInit2(&mStream, -MAX_WBITS);
			if (status_code != Z_OK)
				I_Error("ZIPDeflateReader: inflateInit2 failed with code %d", status_code); 

			mStream.next_out  = static_cast<Bytef*>(data);
			mStream.avail_out = static_cast<uInt>(size);

			const size_t BUFF_SIZE = 4096;
			uint8_t buf[BUFF_SIZE];

			while (mStream.avail_out && !mFile->eof())
			{
				size_t bytes_read = mFile->read(buf, BUFF_SIZE);
				mStream.next_in  = buf;
				mStream.avail_in = bytes_read;
				status_code = inflate(&mStream, Z_SYNC_FLUSH);
				if (status_code != Z_OK && status_code != Z_STREAM_END)
					I_Error("ZIPDeflateReader::read: invalid deflate stream");
				total_bytes_read += bytes_read;
			}

			return total_bytes_read;
		}
	}
	return 0;
}


VERSION_CONTROL (res_container_cpp, "$Id$")