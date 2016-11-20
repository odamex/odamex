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
// Abstraction for reading data from files stored on disk, in memory buffers,
// or compressed.
//
//-----------------------------------------------------------------------------

#ifndef __RES_FILEACCESSOR_H__
#define __RES_FILEACCESSOR_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m_ostring.h"
#include "m_fileio.h"

// ============================================================================
//
// FileAccessor abstract base class interface
//
// ============================================================================
//
// Abstraction for reading from a file. The functions for reading a single
// integer value handle endian conversion from the file's endian format to
// the native format for the system.
//
class FileAccessor
{
public:
	virtual ~FileAccessor() {}

	virtual bool valid() const = 0;

	virtual const OString& getFileName() const = 0;

	virtual size_t size() const = 0;

	virtual void seek(size_t pos) = 0;

	virtual size_t getSeekPosition() const = 0;

	virtual bool eof() const = 0;

	virtual size_t read(void* data, size_t length) const = 0;

	virtual bool read(uint8_t* value) const
	{
		return read(value, sizeof(*value)) == sizeof(*value);
	}

	virtual bool read(int8_t* value) const
	{
		return read(value, sizeof(*value)) == sizeof(*value);
	}

	virtual bool read(uint16_t* value) const
	{
		return read(value, sizeof(*value)) == sizeof(*value);
	}

	virtual bool read(int16_t* value) const
	{
		return read(value, sizeof(*value)) == sizeof(*value);
	}

	virtual bool read(uint32_t* value) const
	{
		return read(value, sizeof(*value)) == sizeof(*value);
	}

	virtual bool read(int32_t* value) const
	{
		return read(value, sizeof(*value)) == sizeof(*value);
	}
};


// ============================================================================
//
// DiskFileAccessor class interface & implementation
//
// ============================================================================
//
// Abstraction for reading from a file located on the disk.
//

class DiskFileAccessor : public FileAccessor
{
public:
	DiskFileAccessor(const OString& filename) :
		mFileName(filename)
	{
		mFileHandle = fopen(mFileName.c_str(), "rb");
	}

	virtual ~DiskFileAccessor()
	{
		if (mFileHandle)
			fclose(mFileHandle);
	}

	virtual bool valid() const
	{
		return mFileHandle != NULL;
	}

	virtual const OString& getFileName() const
	{
		return mFileName;
	}

	virtual size_t size() const
	{
		if (mFileHandle)	
			return M_FileLength(mFileHandle);
		return 0;
	}

	virtual void seek(size_t pos)
	{
		if (mFileHandle)
			fseek(mFileHandle, pos, SEEK_SET);
	}

	virtual size_t getSeekPosition() const
	{
		if (mFileHandle)
			return ftell(mFileHandle);
		return -1;
	}

	virtual bool eof() const
	{
		return feof(mFileHandle);
	}

	virtual size_t read(void* data, size_t length) const
	{
		if (mFileHandle)
			return fread(data, 1, length, mFileHandle);
		return 0;
	}

private:
	FILE*				mFileHandle;
	const OString		mFileName;
};


// ============================================================================
//
// MemoryFileAccessor class interface & implementation
//
// ============================================================================
//
// Abstraction for reading from a file located in a memory buffer.
//

class MemoryFileAccessor : public FileAccessor
{
public:
	MemoryFileAccessor(const OString& filename, const void* buffer, size_t length) :
		mBuffer(static_cast<const uint8_t*>(buffer)),
		mLength(length), mSeekPos(0), mFileName(filename)
	{ }

	virtual ~MemoryFileAccessor() {}

	virtual bool valid() const
	{
		return mBuffer != NULL; 
	}

	virtual const OString& getFileName() const
	{
		return mFileName;
	}

	virtual size_t size() const
	{
		return mLength;
	}

	virtual void seek(size_t pos)
	{
		mSeekPos = pos;
	}

	virtual size_t getSeekPosition() const
	{
		if (!eof())
			return mSeekPos;
		return -1;
	}

	virtual bool eof() const
	{
		return mSeekPos >= mLength;
	}

	virtual size_t read(void* data, size_t length) const
	{
		if (mBuffer && !eof())
		{
			size_t read_cnt = std::min(mSeekPos + length, mLength) - mSeekPos;
			memcpy(data, mBuffer + mSeekPos, read_cnt);
			mSeekPos += read_cnt;
			return read_cnt;
		}
		return 0;
	}

private:
	const uint8_t*		mBuffer;
	size_t				mLength;
	mutable size_t		mSeekPos;
	const OString		mFileName;
};

#endif	// __RES_FILEACCESSOR_H__

