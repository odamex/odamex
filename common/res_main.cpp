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

#include "res_main.h"
#include <stdlib.h>

#include "m_ostring.h"
#include "hashtable.h"
#include <vector>
#include "m_fileio.h"
#include "cmdlib.h"


// ============================================================================
//
// LumpLookupTable class implementation
//
// Performs insertion and querying of resource file lump names.
//
// ============================================================================

class LumpLookupTable
{
public:
	LumpLookupTable() :
			mNameTable(INITIAL_SIZE * 2),
			mIdTable(INITIAL_SIZE * 2),
			mRecordPool(INITIAL_SIZE), mNextRecord(0)
	{ }

	void clear()
	{
		mNameTable.clear();
		mIdTable.clear();
		mNextRecord = 0;
	}

	void addLump(const OString& lumpname, const ResourceFile::LumpId id)
	{
		if (mNextRecord >= mRecordPool.size())
			mRecordPool.resize(mRecordPool.size() * 2);

		LumpRecord* rec = &mRecordPool[mNextRecord++];
		rec->id = id;
		rec->next = NULL;

		// if there is already a record with this name, add it to this record's linked list
		NameLookupTable::iterator it = mNameTable.find(lumpname);
		if (it != mNameTable.end())
			rec->next = it->second;

		// add the record to the name lookup table
		mNameTable[lumpname] = rec;

		// add an entry to the id lookup table
		// don't have to worry about duplicate keys for LumpIds
		mIdTable.insert(std::make_pair(id, lumpname));
	}


	//
	// LumpLookupTable::lookupByName
	//
	// Returns the LumpId of the lump matching the given name. If more than
	// one lumps match, the most recently added resource file is
	// given precedence. If no matches are found, LUMP_NOT_FOUND is returned.
	//
	const ResourceFile::LumpId lookupByName(const OString& lumpname) const
	{
		NameLookupTable::const_iterator it = mNameTable.find(lumpname);
		if (it != mNameTable.end())
			return it->second->id;
		return ResourceFile::LUMP_NOT_FOUND;
	}


	//
	// LumpLookuptable::lookupById
	//
	// Returns the lump name matching the given LumpId. If no matches are
	// found, an empty string is returned.
	//
	const OString& lookupById(const ResourceFile::LumpId id) const
	{
		IdLookupTable::const_iterator it = mIdTable.find(id);
		if (it != mIdTable.end())
			return it->second;

		static OString empty_string;
		return empty_string;
	}


	//
	// LumpLookupTable::queryByName
	//
	// Fills the supplied vector with the LumpId of any lump whose name
	// matches the given string. An empty vector indicates that there were
	// no matches.
	//
	void queryByName(const OString& lumpname, std::vector<ResourceFile::LumpId>& ids) const
	{
		ids.clear();
		NameLookupTable::const_iterator it = mNameTable.find(lumpname);
		if (it != mNameTable.end())
		{
			const LumpRecord* rec = it->second;
			while (rec)
			{
				ids.push_back(rec->id);
				rec = rec->next;
			}
		}
	}

private:
	static const size_t INITIAL_SIZE = 8192;

	struct LumpRecord
	{
		ResourceFile::LumpId	id;
		LumpRecord*				next;	
	};

	typedef OHashTable<OString, LumpRecord*> NameLookupTable;
	typedef OHashTable<ResourceFile::LumpId, OString> IdLookupTable;

	NameLookupTable				mNameTable;
	IdLookupTable				mIdTable;
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
								const ResourceFile::ResourceFileId res_id,
								LumpLookupTable* lump_lookup_table) :
		mResourceFileId(res_id),
		mFileHandle(NULL), mFileName(filename),
		mFileLength(0)
{
	mFileHandle = fopen(mFileName.c_str(), "rb");
	if (mFileHandle == NULL)
		return;

	mFileLength = M_FileLength(mFileHandle);

	if (getLumpCount() > 0)
	{
		// TODO: trim path and file extension off filename
		const OString name(filename);

		LumpId id = mResourceFileId << ResourceFile::LUMP_ID_BITS; 
		lump_lookup_table->addLump(name, id);
	}
}


SingleLumpResourceFile::~SingleLumpResourceFile()
{
	cleanup();
}


size_t SingleLumpResourceFile::readLump(const LumpId id, void* data) const
{
	if (checkLump(id) && mFileHandle != NULL)
	{
		fseek(mFileHandle, 0, SEEK_SET);
		size_t read_cnt = fread(data, 1, mFileLength, mFileHandle);
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
								const ResourceFile::ResourceFileId res_id,
								LumpLookupTable* lump_lookup_table) :
		mResourceFileId(res_id),
		mFileHandle(NULL), mFileName(filename),
		mRecords(NULL), mRecordCount(0),
		mIsIWad(false)
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

	struct wad_lump_record_t
	{
		int32_t		offset;
		int32_t		size;
		char		name[8];
	};

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

	mRecords = new LumpRecord[wad_lump_count];

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

			LumpId id = (mResourceFileId << ResourceFile::LUMP_ID_BITS) | index;
			lump_lookup_table->addLump(name, id);
		}
	}

	delete [] wad_table;

	mIsIWad = W_IsIWAD(mFileName, W_MD5(mFileName));
}


WadResourceFile::~WadResourceFile()
{
	cleanup();
}


bool WadResourceFile::checkLump(const LumpId id) const
{
	ResourceFile::ResourceFileId res_id = id >> ResourceFile::LUMP_ID_BITS;
	unsigned int wad_lump_num = id & ResourceFile::LUMP_ID_MASK;

	return res_id == getResourceFileId() && wad_lump_num < getLumpCount();
}


size_t WadResourceFile::getLumpLength(const LumpId id) const
{
	if (checkLump(id))
	{
		unsigned int wad_lump_num = id & ResourceFile::LUMP_ID_MASK;
		return mRecords[wad_lump_num].size;
	}

	return 0;
}

	
size_t WadResourceFile::readLump(const LumpId id, void* data) const
{
	if (checkLump(id) && mFileHandle != NULL)
	{
		unsigned int wad_lump_num = id & ResourceFile::LUMP_ID_MASK;
		fseek(mFileHandle, mRecords[wad_lump_num].offset, SEEK_SET);
		size_t read_cnt = fread(data, 1, mRecords[wad_lump_num].size, mFileHandle);
		return read_cnt;
	}

	return 0;
}


// ****************************************************************************

static LumpLookupTable lump_lookup_table;
static std::vector<ResourceFile*> ResourceFiles;

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
	const char* magic_str = "Patch File for DeHackEd v3.0";

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
	ResourceFile::ResourceFileId res_id = ResourceFiles.size();

	if (!M_FileExists(filename))
		return;

	ResourceFile* res_file = NULL;

	if (Res_CheckWadFile(filename))
		res_file = new WadResourceFile(filename, res_id, &lump_lookup_table);
	else if (Res_CheckDehackedFile(filename))
		res_file = new SingleLumpResourceFile(filename, res_id, &lump_lookup_table);

	// check that the resource file contains valid lumps
	if (res_file && res_file->getLumpCount() == 0)
	{
		delete res_file;
		res_file = NULL;
	}

	if (res_file)
		ResourceFiles.push_back(res_file);
}


//
// Res_CloseAllResourceFiles
//
// Closes all open resource files. This should be called prior to switching
// to a new set of resource files.
//
void Res_CloseAllResourceFiles()
{
	lump_lookup_table.clear();

	for (std::vector<ResourceFile*>::iterator it = ResourceFiles.begin(); it != ResourceFiles.end(); ++it)
		delete *it;
	ResourceFiles.clear();
}


//
// Res_GetLumpId
//
// Looks for the resource lump that matches id. If there are more than one
// lumps that match, resource file merging rules dictate that the id of the
// matching lump in the resource file with the highest id (most recently added)
// is returned. A special token of ResourceFile::LUMP_NOT_FOUND is returned if
// there are no matching lumps.
//
ResourceFile::LumpId Res_GetLumpId(const OString& lumpname)
{
	return lump_lookup_table.lookupByName(lumpname);
}


//
// Res_GetLumpName
//
// Looks for the name of the resource lump that matches id. If the lump is not
// found, an empty string is returned.
//
const OString& Res_GetLumpName(const ResourceFile::LumpId id)
{
	return lump_lookup_table.lookupById(id);
}


//
// Res_CheckLump
//
// Verifies that the given LumpId matches a valid resource lump.
//
bool Res_CheckLump(const ResourceFile::LumpId id)
{
	ResourceFile::ResourceFileId res_id = id >> ResourceFile::LUMP_ID_BITS;
	return res_id < ResourceFiles.size() && ResourceFiles[res_id]->checkLump(id);
}
	

//
// Res_GetLumpLength
//
// Returns the length of the resource lump that matches id. If the lump is
// not found, 0 is returned.
//
size_t Res_GetLumpLength(const ResourceFile::LumpId id)
{
	ResourceFile::ResourceFileId res_id = id >> ResourceFile::LUMP_ID_BITS;
	if (res_id < ResourceFiles.size())
		return ResourceFiles[res_id]->getLumpLength(id);
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
size_t Res_ReadLump(const ResourceFile::LumpId id, void* data)
{
	ResourceFile::ResourceFileId res_id = id >> ResourceFile::LUMP_ID_BITS;
	if (res_id < ResourceFiles.size())
		return ResourceFiles[res_id]->readLump(id, data);
	return 0;
}


//
// Res_QueryLumpName
//
// Fills the supplied vector with the LumpId of any lump whose name matches the
// given string. An empty vector indicates that there were no matches.
//
void Res_QueryLumpName(const OString& lumpname, std::vector<ResourceFile::LumpId>& ids)
{
	lump_lookup_table.queryByName(lumpname, ids);
}

VERSION_CONTROL (res_main_cpp, "$Id: res_main.cpp $")
