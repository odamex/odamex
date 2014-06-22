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

typedef uint32_t ResourceId;
typedef uint32_t ResourceFileId;
typedef uint32_t NameSpaceId;

typedef std::vector<OString> NameSpaceTable;

static const OString global_namespace_name("GLOBAL");
static const OString flats_namespace_name("FLATS");
static const OString patches_namespace_name("PATCHES");
static const OString sprites_namespace_name("SPRITES");


// ****************************************************************************


// ============================================================================
//
// ResourceFile abstract base class interface
//
// ============================================================================

class ResourceFile
{
public:
	static const ResourceId LUMP_NOT_FOUND = -1;

	ResourceFile() { }
	virtual ~ResourceFile() { }

	virtual const ResourceFileId getResourceFileId() const = 0;
	virtual const OString& getFileName() const = 0;

	virtual bool isIWad() const { return false; }

	virtual size_t getLumpCount() const = 0;

	virtual bool checkLump(const ResourceId res_id) const = 0;

	virtual size_t getLumpLength(const ResourceId res_id) const = 0;

	virtual size_t readLump(const ResourceId res_id, void* data, size_t length) const = 0;

	virtual size_t readLump(const ResourceId res_id, void* data) const
	{
		return readLump(res_id, data, getLumpLength(res_id));
	}
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
	SingleLumpResourceFile(const OString& filename, const ResourceFileId file_id, LumpLookupTable* lump_lookup_table);

	virtual ~SingleLumpResourceFile();

	virtual const ResourceFileId getResourceFileId() const
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

	virtual bool checkLump(const ResourceId res_id) const;

	virtual size_t getLumpLength(const ResourceId res_id) const
	{
		if (checkLump(res_id))
			return mFileLength;
		return 0;
	}

	virtual size_t readLump(const ResourceId res_id, void* data, size_t length) const;

private:
	void cleanup()
	{
		if (mFileHandle != NULL)
			fclose(mFileHandle);
		mFileHandle = NULL;
	}

	ResourceFileId		mResourceFileId;

	mutable FILE*		mFileHandle;
	const OString&		mFileName;
	size_t				mFileLength;
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
					const ResourceFileId file_id,
					LumpLookupTable* lump_lookup_table);
	
	virtual ~WadResourceFile();

	virtual const ResourceFileId getResourceFileId() const
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

	virtual bool checkLump(const ResourceId res_id) const;

	virtual size_t getLumpLength(const ResourceId res_id) const;
		
	virtual size_t readLump(const ResourceId res_id, void* data, size_t length) const;

private:
	struct wad_lump_record_t
	{
		int32_t		offset;
		int32_t		size;
		char		name[8];
	};

	void cleanup()
	{
		if (mFileHandle != NULL)
			fclose(mFileHandle);
		mFileHandle = NULL;

		delete [] mRecords;
		mRecords = NULL;
	}

	ResourceFileId		mResourceFileId;
	mutable FILE*		mFileHandle;
	const OString&		mFileName;

	wad_lump_record_t*	mRecords;
	size_t				mRecordCount;

	bool				mIsIWad;
};


// ****************************************************************************

void Res_OpenResourceFile(const OString& filename);

void Res_CloseAllResourceFiles();

ResourceId Res_GetResourceId(const OString& lumpname, const OString& namespace_name = global_namespace_name);

const OString& Res_GetLumpName(const ResourceId res_id);

bool Res_CheckLump(const ResourceId res_id);

static inline bool Res_CheckLump(const OString& lumpname)
{
	return Res_CheckLump(Res_GetResourceId(lumpname));
}

size_t Res_GetLumpLength(const ResourceId res_id);

static inline size_t Res_GetLumpLength(const OString& lumpname)
{
	return Res_GetLumpLength(Res_GetResourceId(lumpname));
}

size_t Res_ReadLump(const ResourceId id, void* data);

static inline size_t Res_ReadLump(const OString& lumpname, void* data)
{
	return Res_ReadLump(Res_GetResourceId(lumpname), data);
}

void* Res_CacheLump(const ResourceId res_id, int tag);

static inline void* Res_CacheLump(const OString& name, int tag)
{
	return Res_CacheLump(Res_GetResourceId(name), tag);
}


void Res_QueryLumpName(std::vector<ResourceId>& res_ids,
					const OString& lumpname, const OString& namespace_name = global_namespace_name);

#endif	// __RES_MAIN_H__

