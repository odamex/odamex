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

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <cstdint>
#include <type_traits>

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

	virtual bool seek(size_t pos) = 0;

	virtual size_t getSeekPosition() const = 0;

	virtual bool eof() const = 0;

	virtual size_t read(uint8_t* data, size_t length) const = 0;

	template <typename T>
	requires std::is_integral_v<T>
	bool read(T* value) const
	{
		return read(reinterpret_cast<uint8_t*>(value), sizeof(*value)) == sizeof(*value);
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
		mFileName(filename), mFileSize(0)
	{
		mFileHandle = fopen(mFileName.c_str(), "rb");
		if (mFileHandle)
			mFileSize = M_FileLength(mFileHandle);
	}

	~DiskFileAccessor() override
	{
		if (mFileHandle)
			fclose(mFileHandle);
	}

	bool valid() const override
	{
		return mFileHandle != NULL;
	}

	const OString& getFileName() const override
	{
		return mFileName;
	}

	size_t size() const override
	{
		return mFileSize;
	}

	bool seek(size_t pos) override
	{
		if (mFileHandle)
			return fseek(mFileHandle, pos, SEEK_SET) == 0;
		return false;
	}

	size_t getSeekPosition() const override
	{
		if (mFileHandle)
			return ftell(mFileHandle);
		return -1;
	}

	bool eof() const override
	{
		return feof(mFileHandle);
	}

	size_t read(uint8_t* data, size_t length) const override
	{
		if (mFileHandle)
			return fread(data, 1, length, mFileHandle);
		return 0;
	}

private:
	FILE*				mFileHandle;
	const OString		mFileName;
	size_t				mFileSize;
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
	MemoryFileAccessor(const OString& filename, const uint8_t* buffer, size_t length) :
		mBuffer(static_cast<const uint8_t*>(buffer)),
		mLength(length), mSeekPos(0), mFileName(filename)
	{ }

	~MemoryFileAccessor() override {}

	bool valid() const override
	{
		return mBuffer != NULL; 
	}

	const OString& getFileName() const override
	{
		return mFileName;
	}

	size_t size() const override
	{
		return mLength;
	}

	bool seek(size_t pos) override
	{
		mSeekPos = pos;
		return mSeekPos < mLength;
	}

	size_t getSeekPosition() const override
	{
		if (!eof())
			return mSeekPos;
		return -1;
	}

	bool eof() const override
	{
		return mSeekPos >= mLength;
	}

	size_t read(uint8_t* data, size_t length) const override
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
