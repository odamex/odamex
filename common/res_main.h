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

#ifndef __RES_MAIN_H__
#define __RES_MAIN_H__

#include <stdlib.h>
#include "m_ostring.h"
#include "hashtable.h"
#include <vector>

#include "w_wad.h"

class LumpLookupTable;

// ****************************************************************************

// ============================================================================
//
// ResourceFile abstract base class interface
//
// ============================================================================

class ResourceFile
{
public:
	typedef unsigned int LumpId;
	typedef unsigned int ResourceFileId;

	static const int LUMP_ID_BITS = 20;
	static const int RESOURCE_FILE_ID_BITS = 32 - LUMP_ID_BITS;
	static const LumpId LUMP_ID_MASK = (1 << LUMP_ID_BITS) - 1;
	static const LumpId LUMP_NOT_FOUND = -1;

	ResourceFile() { }
	virtual ~ResourceFile() { }

	virtual const ResourceFile::ResourceFileId getResourceFileId() const = 0;
	virtual const OString& getFileName() const = 0;

	virtual bool isIWad() const { return false; }

	virtual size_t getLumpCount() const = 0;

	virtual bool checkLump(const ResourceFile::LumpId id) const = 0;

	virtual size_t getLumpLength(const ResourceFile::LumpId id) const = 0;

	virtual size_t readLump(const ResourceFile::LumpId id, void* data) const = 0;
};

// ****************************************************************************

// ============================================================================
//
// SingleLumpResourceFile abstract base class interface
//
// ============================================================================

class SingleLumpResourceFile : public ResourceFile
{
public:
	SingleLumpResourceFile(const OString& filename,
						const ResourceFile::ResourceFileId res_id,
						LumpLookupTable* lump_lookup_table);

	virtual ~SingleLumpResourceFile();

	virtual const ResourceFile::ResourceFileId getResourceFileId() const
	{
		return mResourceFileId;
	}

	virtual const OString& getFileName() const
	{
		return mFileName;
	}

	virtual size_t getLumpCount() const
	{
		return mFileLength > 0 ? 1 : 0;
	}

	virtual bool checkLump(const LumpId id) const
	{
		ResourceFile::ResourceFileId res_id = id >> ResourceFile::LUMP_ID_BITS;
		unsigned int wad_lump_num = id & ResourceFile::LUMP_ID_MASK;

		return res_id == getResourceFileId() && wad_lump_num < getLumpCount();
	}

	virtual size_t getLumpLength(const LumpId id) const
	{
		return mFileLength;
	}

	virtual size_t readLump(const LumpId id, void* data) const;

private:
	void cleanup()
	{
		if (mFileHandle != NULL)
			fclose(mFileHandle);
		mFileHandle = NULL;
	}

	ResourceFile::ResourceFileId	mResourceFileId;

	mutable FILE*					mFileHandle;
	const OString&					mFileName;
	size_t							mFileLength;
};


// ****************************************************************************

// ============================================================================
//
// WadResourceFile abstract base class interface
//
// ============================================================================

class WadResourceFile : public ResourceFile
{
public:
	WadResourceFile(const OString& filename, 
					const ResourceFile::ResourceFileId res_id,
					LumpLookupTable* lump_lookup_table);
	
	virtual ~WadResourceFile();

	virtual const ResourceFile::ResourceFileId getResourceFileId() const
	{
		return mResourceFileId;
	}

	virtual const OString& getFileName() const
	{
		return mFileName;
	}

	virtual bool isIWad() const
	{
		return mIsIWad;
	}

	virtual size_t getLumpCount() const
	{
		return mRecordCount;
	}

	virtual bool checkLump(const LumpId id) const;

	virtual size_t getLumpLength(const LumpId id) const;
		
	virtual size_t readLump(const LumpId id, void* data) const;

private:
	void cleanup()
	{
		if (mFileHandle != NULL)
			fclose(mFileHandle);
		mFileHandle = NULL;

		delete [] mRecords;
		mRecords = NULL;
	}

	ResourceFile::ResourceFileId	mResourceFileId;
	mutable FILE*					mFileHandle;
	const OString&					mFileName;

	struct LumpRecord
	{
		size_t			offset;
		size_t			size;
	};

	LumpRecord*						mRecords;
	size_t							mRecordCount;

	bool							mIsIWad;
};


// ****************************************************************************

void Res_OpenResourceFile(const OString& filename);

void Res_CloseAllResourceFiles();

ResourceFile::LumpId Res_GetLumpId(const OString& lumpname);

const OString& Res_GetLumpName(const ResourceFile::LumpId id);

bool Res_CheckLump(const ResourceFile::LumpId id);

static inline bool Res_CheckLump(const OString& lumpname)
{
	return Res_CheckLump(Res_GetLumpId(lumpname));
}

size_t Res_GetLumpLength(const ResourceFile::LumpId id);

static inline size_t Res_GetLumpLength(const OString& lumpname)
{
	return Res_GetLumpLength(Res_GetLumpId(lumpname));
}

size_t Res_ReadLump(const ResourceFile::LumpId id, void* data);

static inline size_t Res_ReadLump(const OString& lumpname, void* data)
{
	return Res_ReadLump(Res_GetLumpId(lumpname), data);
}

void Res_QueryLumpName(const OString& lumpname, std::vector<ResourceFile::LumpId>& ids);

#endif	// __RES_MAIN_H__

