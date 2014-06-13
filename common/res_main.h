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

typedef unsigned int LumpId;

class LumpLookupTable;

void Res_OpenResourceFile(const OString& filename);
void Res_CloseAllResourceFiles();
LumpId Res_GetLumpId(const OString& lumpname);
const OString& Res_GetLumpName(const LumpId id);
bool Res_CheckLump(const LumpId id);
size_t Res_GetLumpLength(const LumpId id);
size_t Res_ReadLump(const LumpId id, void* data);

// ****************************************************************************

// ============================================================================
//
// ResourceFile abstract base class interface
//
// ============================================================================

class ResourceFile
{
public:
	ResourceFile() { }
	virtual ~ResourceFile() { }

	virtual const OString& getFileName() const = 0;

	virtual size_t getLumpCount() const = 0;

	virtual bool checkLump(const LumpId id) const = 0;

	virtual size_t getLumpLength(const LumpId id) const = 0;

	virtual size_t readLump(const LumpId id, void* data) const = 0;

	static const LumpId LUMP_NOT_FOUND = -1;

	static const int LUMP_ID_BITS = 20;
	static const int RESOURCE_FILE_ID_BITS = 32 - LUMP_ID_BITS;
	static const LumpId LUMP_ID_MASK = (1 << LUMP_ID_BITS) - 1;
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
	WadResourceFile(const OString& filename, const LumpId first_id,
					LumpLookupTable* lump_lookup_table);
	
	virtual ~WadResourceFile();

	virtual const OString& getFileName() const
	{
		return mFileName;
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

		delete mNameTable;
		mNameTable = NULL;
	}

	struct LumpRecord
	{
		OString			name;
		size_t			offset;
		size_t			size;
	};

	LumpRecord*				mRecords;
	size_t					mRecordCount;

	typedef OHashTable<OString, unsigned int> NameLookupTable;
	NameLookupTable*		mNameTable;

	const LumpId			mStartingLumpId;

	mutable FILE*			mFileHandle;
	const OString&			mFileName;
};



#endif	// __RES_MAIN_H__

