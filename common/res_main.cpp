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




#include "res_main.h"
#include <stdlib.h>

#include "m_ostring.h"
#include "hashtable.h"
#include <vector>
#include "m_fileio.h"
#include "cmdlib.h"

#include "i_system.h"
#include "z_zone.h"

static bool Res_CheckWadFile(const OString& filename);
static bool Res_CheckDehackedFile(const OString& filename);


static const int RESOURCE_FILE_ID_BITS = 8;
static const int RESOURCE_FILE_ID_SHIFT = 32 - RESOURCE_FILE_ID_BITS;
static const int NAMESPACE_ID_BITS = 5;
static const int NAMESPACE_ID_SHIFT = RESOURCE_FILE_ID_SHIFT - NAMESPACE_ID_BITS;
static const int LUMP_ID_BITS = 32 - RESOURCE_FILE_ID_BITS - NAMESPACE_ID_BITS;
static const int LUMP_ID_SHIFT = NAMESPACE_ID_SHIFT - LUMP_ID_BITS; 


//
// Res_GetResourceFileId
//
// Extracts the ResourceFileId portion of a ResourceId.
//
static inline ResourceFileId Res_GetResourceFileId(const ResourceId res_id)
{
	return (res_id >> RESOURCE_FILE_ID_SHIFT) & ((1 << RESOURCE_FILE_ID_BITS) - 1); 
}


//
// Res_GetNameSpaceId
//
// Extracts the NameSpaceId portion of a ResourceId.
//
static inline NameSpaceId Res_GetNameSpaceId(const ResourceId res_id)
{
	return (res_id >> NAMESPACE_ID_SHIFT) & ((1 << NAMESPACE_ID_BITS) - 1);
}


//
// Res_GetLumpId
//
// Extracts the lump identifier portion of a ResourceId.
//
static inline uint32_t Res_GetLumpId(const ResourceId res_id)
{
	return (res_id >> LUMP_ID_SHIFT) & ((1 << LUMP_ID_BITS) - 1); 
}


//
// Res_CreateResourceId
//
// Creates a ResourceId given the ResourceFileId, NameSpaceId and lump
// identifier for a resource.
//
static inline ResourceId Res_CreateResourceId(const ResourceFileId file_id, const NameSpaceId namespace_id,
												const uint32_t lump_number)
{
	return ((file_id & ((1 << RESOURCE_FILE_ID_BITS) - 1)) << RESOURCE_FILE_ID_SHIFT) |
			((namespace_id & ((1 << NAMESPACE_ID_BITS) - 1)) << NAMESPACE_ID_SHIFT) |
			((lump_number & ((1 << LUMP_ID_BITS) - 1)) << LUMP_ID_SHIFT);
}



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
	static const char iwad_magic_str[] = "IWAD";
	static const char pwad_magic_str[] = "PWAD";
	const char* identifier = (const char*)data;

	return length > 4 && (strncmp(identifier, iwad_magic_str, 4) == 0
						|| strncmp(identifier, pwad_magic_str, 4) == 0);
}



// ****************************************************************************

// ============================================================================
//
// LumpLookupTable class implementation
//
// Performs insertion and querying of resource file lump names.
//
// ============================================================================

typedef std::pair<OString, OString> LumpName;

// hashing function for OHashTable to use the LumpName typedef
template <> struct hashfunc<LumpName>
{
	size_t operator()(const LumpName& lumpname) const
	{
		const hashfunc<OString> func;
		return func(lumpname.first) ^ func(lumpname.second);
	}
};


class LumpLookupTable
{
public:
	LumpLookupTable() :
			mNameTable(INITIAL_SIZE * 2),
			mIdTable(INITIAL_SIZE * 2),
			mNameSpaces(64),
			mRecordPool(INITIAL_SIZE), mNextRecord(0)
	{
		clear();
	}


	void clear()
	{
		mNameTable.clear();
		mIdTable.clear();
		mNextRecord = 0;

		// clear all namespaces and add the default ones
		mNameSpaces.clear();
		NameSpaceId namespace_id = 0;
		mNameSpaces.insert(std::make_pair("GLOBAL", namespace_id++));
		mNameSpaces.insert(std::make_pair("PATCHES", namespace_id++));
		mNameSpaces.insert(std::make_pair("GRAPHICS", namespace_id++));
		mNameSpaces.insert(std::make_pair("FLATS", namespace_id++));
		mNameSpaces.insert(std::make_pair("SPRITES", namespace_id++));
		mNameSpaces.insert(std::make_pair("TEXTURES", namespace_id++));
		mNameSpaces.insert(std::make_pair("HIRES", namespace_id++));
		mNameSpaces.insert(std::make_pair("SOUNDS", namespace_id++));
		mNameSpaces.insert(std::make_pair("MUSIC", namespace_id++));
		mNameSpaces.insert(std::make_pair("COLORMAPS", namespace_id++));
		mNameSpaces.insert(std::make_pair("ACS", namespace_id++));
	}


	//
	// LumpLookupTable::addNameSpace
	//
	// Inserts a new named namespace into the namespace lookup table,
	// assigning it the next unused ID number.
	//
	void addNameSpace(const OString& namespace_name)
	{
		NameSpaceId namespace_id = mNameSpaces.size();
		mNameSpaces.insert(std::make_pair(namespace_name, namespace_id));
	}


	//
	// LumpLookupTable::lookupNameSpaceByName
	//
	// Returns the namespace ID number for the give named namespace.
	//
	NameSpaceId lookupNameSpaceByName(const OString& namespace_name)
	{
		NameSpaceLookupTable::const_iterator it = mNameSpaces.find(namespace_name);
		if (it != mNameSpaces.end())
			return it->second;
		return lookupNameSpaceByName(global_namespace_name);
	}


	//
	// LumpLookupTable::lookupNameSpaceById
	//
	// Returns the namespace name for the given namespace ID number.
	// This uses a linear search so it's speed is not optimal.
	//
	const OString& lookupNameSpaceById(const NameSpaceId id)
	{
		for (NameSpaceLookupTable::const_iterator it = mNameSpaces.begin(); it != mNameSpaces.end(); ++it)
		{
			if (it->second == id)
				return it->first;
		}

		return global_namespace_name; 
	}


	//
	// LumpLookupTable::addLump
	//
	// Inserts a lump name entry into the lookup table.
	//
	void addLump(const ResourceId res_id, const OString& name, const OString& namespace_name = global_namespace_name) 
	{
		if (mNextRecord >= mRecordPool.size())
			mRecordPool.resize(mRecordPool.size() * 2);

		LumpRecord* rec = &mRecordPool[mNextRecord++];
		rec->res_id = res_id;
		rec->next = NULL;

		LumpName lumpname(name, namespace_name);

		// if there is already a record with this name, add it to this record's linked list
		NameLookupTable::iterator it = mNameTable.find(lumpname);
		if (it != mNameTable.end())
			rec->next = it->second;

		// add the record to the name lookup table
		mNameTable.insert(std::make_pair(lumpname, rec));

		// add an entry to the id lookup table
		// don't have to worry about duplicate keys for ResourceIds
		mIdTable.insert(std::make_pair(res_id, name));
	}


	//
	// LumpLookupTable::lookupByName
	//
	// Returns the ResourceId of the lump matching the given name. If more than
	// one lumps match, the most recently added resource file is
	// given precedence. If no matches are found, LUMP_NOT_FOUND is returned.
	//
	const ResourceId lookupByName(const OString& name, const OString& namespace_name) const
	{
		const LumpName lumpname(name, namespace_name);
		NameLookupTable::const_iterator it = mNameTable.find(lumpname);
		if (it != mNameTable.end())
			return it->second->res_id;
		return ResourceFile::LUMP_NOT_FOUND;
	}


	//
	// LumpLookuptable::lookupById
	//
	// Returns the lump name matching the given ResourceId. If no matches are
	// found, an empty string is returned.
	//
	const OString& lookupById(const ResourceId res_id) const
	{
		IdLookupTable::const_iterator it = mIdTable.find(res_id);
		if (it != mIdTable.end())
			return it->second;

		static OString empty_string;
		return empty_string;
	}


	//
	// LumpLookupTable::queryByName
	//
	// Fills the supplied vector with the ResourceId of any lump whose name
	// matches the given string. An empty vector indicates that there were
	// no matches.
	//
	void queryByName(std::vector<ResourceId>& res_ids, const OString& name, const OString& namespace_name) const
	{
		res_ids.clear();

		const LumpName lumpname(name, namespace_name);
		NameLookupTable::const_iterator it = mNameTable.find(lumpname);
		if (it != mNameTable.end())
		{
			const LumpRecord* rec = it->second;
			while (rec)
			{
				res_ids.push_back(rec->res_id);
				rec = rec->next;
			}
		}

		std::reverse(res_ids.begin(), res_ids.end());
	}

private:
	static const size_t INITIAL_SIZE = 8192;

	struct LumpRecord
	{
		ResourceId		res_id;
		LumpRecord*		next;	
	};

	typedef OHashTable<LumpName, LumpRecord*> NameLookupTable;
	typedef OHashTable<ResourceId, OString> IdLookupTable;
	typedef OHashTable<OString, NameSpaceId> NameSpaceLookupTable;

	NameLookupTable				mNameTable;
	IdLookupTable				mIdTable;
	NameSpaceLookupTable		mNameSpaces;

	std::vector<LumpRecord>		mRecordPool;
	size_t						mNextRecord;
};


// ****************************************************************************


// ============================================================================
//
// SingleLumpResourceFile class implementation
//
// ============================================================================

SingleLumpResourceFile::SingleLumpResourceFile(const OString& filename,
								const ResourceFileId file_id, LumpLookupTable* lump_lookup_table) :
		mResourceFileId(file_id),
		mFileHandle(NULL), mFileName(filename),
		mFileLength(0)
{
	mFileHandle = fopen(mFileName.c_str(), "rb");
	if (mFileHandle == NULL)
		return;

	mFileLength = M_FileLength(mFileHandle);

	if (getLumpCount() > 0)
	{
		const NameSpaceId namespace_id = lump_lookup_table->lookupNameSpaceByName(global_namespace_name);
		ResourceId res_id = Res_CreateResourceId(mResourceFileId, namespace_id, 0);

		// the filename serves as the lumpname
		OString lumpname = M_ExtractFileName(StdStringToUpper(filename));

		// rename lumpname to DEHACKED if it's a DeHackEd file
		if (Res_CheckDehackedFile(mFileName))
			lumpname = "DEHACKED";

		lump_lookup_table->addLump(res_id, lumpname, global_namespace_name);
	}
}


SingleLumpResourceFile::~SingleLumpResourceFile()
{
	cleanup();
}


bool SingleLumpResourceFile::checkLump(const ResourceId res_id) const
{
	return getResourceFileId() == Res_GetResourceFileId(res_id) &&
			Res_GetLumpId(res_id) < getLumpCount();
}


size_t SingleLumpResourceFile::readLump(const ResourceId res_id, void* data, size_t length) const
{
	length = std::min(length, getLumpLength(res_id));

	if (checkLump(res_id) && mFileHandle != NULL)
	{
		fseek(mFileHandle, 0, SEEK_SET);
		size_t read_cnt = fread(data, 1, length, mFileHandle);
		return read_cnt;
	}

	return 0;
}




// ****************************************************************************


// ============================================================================
//
// WadResourceFile class implementation
//
// ============================================================================

WadResourceFile::WadResourceFile(const OString& filename,
								const ResourceFileId file_id,
								LumpLookupTable* lump_lookup_table) :
		mResourceFileId(file_id),
		mFileHandle(NULL), mFileName(filename),
		mRecords(NULL), mRecordCount(0),
		mIsIWad(false),
		mFlatStartNum(-1), mFlatEndNum(-1), mColorMapStartNum(-1), mColorMapEndNum(-1),
		mSpriteStartNum(-1), mSpriteEndNum(-1)
{
	mFileHandle = fopen(mFileName.c_str(), "rb");
	if (mFileHandle == NULL)
		return;

	size_t file_length = M_FileLength(mFileHandle);
	size_t read_cnt;

	const char* magic_str_iwad = "IWAD";
	const char* magic_str_pwad = "PWAD";

	char identifier[4];
	read_cnt = fread(&identifier, 1, 4, mFileHandle);
	if (read_cnt != 4 || feof(mFileHandle) || 
		(strncmp(identifier, magic_str_iwad, 4) != 0 && strncmp(identifier, magic_str_pwad, 4) != 0))
	{
		cleanup();
		return;
	}

	int32_t wad_lump_count;
	read_cnt = fread(&wad_lump_count, sizeof(wad_lump_count), 1, mFileHandle);
	wad_lump_count = LELONG(wad_lump_count);

	if (read_cnt != 1 || feof(mFileHandle))
	{
		cleanup();
		return;
	}

	int32_t wad_table_offset;
	read_cnt = fread(&wad_table_offset, sizeof(wad_table_offset), 1, mFileHandle);
	wad_table_offset = LELONG(wad_table_offset);

	if (read_cnt != 1 || feof(mFileHandle))
	{
		cleanup();
		return;
	}

	// read the WAD lump directory
	size_t wad_table_length = wad_lump_count * sizeof(wad_lump_record_t);
	if (wad_table_length + wad_table_offset > file_length)
	{
		cleanup();
		return;
	}

	wad_lump_record_t* wad_table = new wad_lump_record_t[wad_lump_count];
	fseek(mFileHandle, wad_table_offset, SEEK_SET);
	read_cnt = fread(wad_table, wad_table_length, 1, mFileHandle);

	if (read_cnt != 1 || feof(mFileHandle))
	{
		delete [] wad_table;
		cleanup();
		return;
	}

	// read the WAD directory and make note of where various markers are
	setupMarkers(wad_table, wad_lump_count);

	mRecords = new wad_lump_record_t[wad_lump_count];

	for (size_t wad_lump_num = 0; wad_lump_num < (size_t)wad_lump_count; wad_lump_num++)
	{
		size_t offset = LELONG(wad_table[wad_lump_num].offset);
		size_t size = LELONG(wad_table[wad_lump_num].size);

		// check that the lump doesn't extend past the end of the file
		if (offset + size <= file_length)
		{
			const size_t index = mRecordCount++;
			const OString name(StdStringToUpper(wad_table[wad_lump_num].name, 8));

			mRecords[index].offset = offset;
			mRecords[index].size = size;

			OString namespace_name = global_namespace_name;
			if (isLumpFlat(index))
				namespace_name = flats_namespace_name;
			else if (isLumpSprite(index))
				namespace_name = sprites_namespace_name; 

			NameSpaceId namespace_id = lump_lookup_table->lookupNameSpaceByName(namespace_name);
			ResourceId res_id = Res_CreateResourceId(mResourceFileId, namespace_id, index);
			lump_lookup_table->addLump(res_id, name, namespace_name);
		}
	}

	delete [] wad_table;

	mIsIWad = W_IsIWAD(mFileName, W_MD5(mFileName));
}


WadResourceFile::~WadResourceFile()
{
	cleanup();
}


void WadResourceFile::setupMarkers(const wad_lump_record_t* wad_table, size_t wad_lump_count)
{
	mFlatStartNum = mFlatEndNum = -1;
	mColorMapStartNum = mColorMapEndNum = -1;
	mSpriteStartNum = mSpriteEndNum = -1;

	static const OString FlatStartMarker1("F_START");
	static const OString FlatStartMarker2("FF_START");
	static const OString FlatEndMarker1("F_END");
	static const OString FlatEndMarker2("FF_END");
	static const OString ColorMapStartMarker("C_START");
	static const OString ColorMapEndMarker("C_END");
	static const OString SpriteStartMarker1("S_START");
	static const OString SpriteStartMarker2("SS_START");
	static const OString SpriteEndMarker1("S_END");
	static const OString SpriteEndMarker2("SS_END");

	size_t file_length = M_FileLength(mFileHandle);

	for (size_t wad_lump_num = 0, index = 0; wad_lump_num < wad_lump_count; wad_lump_num++)
	{
		size_t offset = LELONG(wad_table[wad_lump_num].offset);
		size_t size = LELONG(wad_table[wad_lump_num].size);

		// check that the lump doesn't extend past the end of the file
		if (offset + size <= file_length)
		{
			const OString name(StdStringToUpper(wad_table[wad_lump_num].name, 8));

			if (name == FlatStartMarker1 || name == FlatStartMarker2)
				mFlatStartNum = index;
			else if (name == FlatEndMarker1 || name == FlatEndMarker2)
				mFlatEndNum = index;
			else if (name == ColorMapStartMarker) 
				mColorMapStartNum = index;
			else if (name == ColorMapEndMarker) 
				mColorMapEndNum = index;
			else if (name == SpriteStartMarker1 || name == SpriteStartMarker2)
				mSpriteStartNum = index;
			else if (name == SpriteEndMarker1 || name == SpriteEndMarker2)
				mSpriteEndNum = index;

			index++;
		}
	}
}


bool WadResourceFile::checkLump(const ResourceId res_id) const
{
	return getResourceFileId() == Res_GetResourceFileId(res_id) &&
			Res_GetLumpId(res_id) < getLumpCount();
}


size_t WadResourceFile::getLumpLength(const ResourceId res_id) const
{
	if (checkLump(res_id))
	{
		const uint32_t wad_lump_num = Res_GetLumpId(res_id);
		return mRecords[wad_lump_num].size;
	}

	return 0;
}

	
size_t WadResourceFile::readLump(const ResourceId res_id, void* data, size_t length) const
{
	length = std::min(length, getLumpLength(res_id));

	if (checkLump(res_id) && mFileHandle != NULL)
	{
		const uint32_t wad_lump_num = Res_GetLumpId(res_id);
		fseek(mFileHandle, mRecords[wad_lump_num].offset, SEEK_SET);
		size_t read_cnt = fread(data, 1, length, mFileHandle);
		return read_cnt;
	}

	return 0;
}


bool WadResourceFile::isLumpFlat(const uint32_t wad_lump_num)
{
	return wad_lump_num > mFlatStartNum && wad_lump_num < mFlatEndNum;
}


bool WadResourceFile::isLumpSprite(const uint32_t wad_lump_num)
{
	return wad_lump_num > mSpriteStartNum && wad_lump_num < mSpriteEndNum;
}


// ****************************************************************************

static LumpLookupTable lump_lookup_table;
static std::vector<ResourceFile*> resource_files;

typedef OHashTable<ResourceId, void*> CacheTable;
static CacheTable cache_table;

//
// Res_CheckWadFile
//
// Checks that the first four bytes of a file are "IWAD" or "PWAD"
//
static bool Res_CheckWadFile(const OString& filename)
{
	const char* magic_str_iwad = "IWAD";
	const char* magic_str_pwad = "PWAD";

	FILE* fp = fopen(filename.c_str(), "rb");
	size_t read_cnt;

	if (fp == NULL)
		return false;

	char data[4];
	read_cnt = fread(&data, 1, 4, fp);
	if (read_cnt != 4)
	{
		fclose(fp);
		return false;
	}

	fclose(fp);

	return strncmp(data, magic_str_iwad, 4) == 0 || strncmp(data, magic_str_pwad, 4) == 0;
}


//
// Res_CheckDehackedFile
//
// Checks that the first line of the file contains a DeHackEd header.
//
static bool Res_CheckDehackedFile(const OString& filename)
{
	const char magic_str[] = "Patch File for DeHackEd v3.0";

	FILE* fp = fopen(filename.c_str(), "rb");
	size_t read_cnt;

	if (fp == NULL)
		return false;

	char data[30];
	size_t magic_len = strlen(magic_str);
	read_cnt = fread(&data, 1, magic_len, fp);
	if (read_cnt != magic_len)
	{
		fclose(fp);
		return false;
	}

	fclose(fp);

	return strnicmp(data, magic_str, magic_len) == 0;
}


//
// Res_OpenResourceFile
//
// Opens a resource file and caches the directory of lump names for queries.
// 
void Res_OpenResourceFile(const OString& filename)
{
	ResourceFileId file_id = resource_files.size();

	if (!M_FileExists(filename))
		return;

	ResourceFile* res_file = NULL;

	if (Res_CheckWadFile(filename))
		res_file = new WadResourceFile(filename, file_id, &lump_lookup_table);
	else if (Res_CheckDehackedFile(filename))
		res_file = new SingleLumpResourceFile(filename, file_id, &lump_lookup_table);

	// check that the resource file contains valid lumps
	if (res_file && res_file->getLumpCount() == 0)
	{
		delete res_file;
		res_file = NULL;
	}

	if (res_file)
		resource_files.push_back(res_file);
}


//
// Res_CloseAllresource_files
//
// Closes all open resource files. This should be called prior to switching
// to a new set of resource files.
//
void Res_CloseAllResourceFiles()
{
	lump_lookup_table.clear();

	for (std::vector<ResourceFile*>::iterator it = resource_files.begin(); it != resource_files.end(); ++it)
		delete *it;
	resource_files.clear();
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
ResourceId Res_GetResourceId(const OString& lumpname, const OString& namespace_name)
{
	return lump_lookup_table.lookupByName(lumpname, namespace_name);
}


//
// Res_QueryLumpName
//
// Fills the supplied vector with the LumpId of any lump whose name matches the
// given string. An empty vector indicates that there were no matches.
//
void Res_QueryLumpName(std::vector<ResourceId>& res_ids,
					const OString& lumpname, const OString& namespace_name)
{
	lump_lookup_table.queryByName(res_ids, lumpname, namespace_name);
}


//
// Res_GetLumpName
//
// Looks for the name of the resource lump that matches id. If the lump is not
// found, an empty string is returned.
//
const OString& Res_GetLumpName(const ResourceId res_id)
{
	return lump_lookup_table.lookupById(res_id);
}


//
// Res_CheckLump
//
// Verifies that the given LumpId matches a valid resource lump.
//
bool Res_CheckLump(const ResourceId res_id)
{
	ResourceFileId file_id = Res_GetResourceFileId(res_id);
	return file_id < resource_files.size() && resource_files[file_id]->checkLump(res_id);
}
	

//
// Res_GetLumpLength
//
// Returns the length of the resource lump that matches id. If the lump is
// not found, 0 is returned.
//
size_t Res_GetLumpLength(const ResourceId res_id)
{
	ResourceFileId file_id = Res_GetResourceFileId(res_id);
	if (file_id < resource_files.size())
		return resource_files[file_id]->getLumpLength(res_id);
	return 0;
}


//
// Res_ReadLump
//
// Reads the resource lump that matches id and copies its contents into data.
// The number of bytes read is returned, or 0 is returned if the lump
// is not found. The variable data must be able to hold the size of the lump,
// as determined by Res_GetLumpLength.
//
size_t Res_ReadLump(const ResourceId res_id, void* data)
{
	ResourceFileId file_id = Res_GetResourceFileId(res_id);
	if (file_id < resource_files.size())
		return resource_files[file_id]->readLump(res_id, data);
	return 0;
}

void* Res_CacheLump(const ResourceId res_id, int tag)
{
	void* data_ptr = NULL;

	CacheTable::iterator it = cache_table.find(res_id);
	if (it != cache_table.end())
	{
		data_ptr = it->second;
		Z_ChangeTag(data_ptr, tag);
	}
	else
	{
		size_t lump_length = Res_GetLumpLength(res_id);
		data_ptr = Z_Malloc(lump_length + 1, tag, &cache_table[res_id]);
		Res_ReadLump(res_id, cache_table[res_id]);
		((unsigned char*)data_ptr)[lump_length] = 0;
	}

	return data_ptr; 
}

VERSION_CONTROL (res_main_cpp, "$Id: res_main.cpp $")
