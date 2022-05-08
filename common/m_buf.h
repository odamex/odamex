// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//  Buffer that can be read from and written to a type at a time.  Used
//  by the networking subsystem.
//
//-----------------------------------------------------------------------------

#pragma once

class buf_t
{
  public:
	byte* data;
	size_t allocsize, cursize, readpos;
	bool overflowed; // set to true if the buffer size failed
  public:
	// Buffer seeking flags
	enum seek_loc_t
	{
		BT_SSET, // From beginning
		BT_SCUR, // From current position
		BT_SEND, // From end
	};

	buf_t() : data(0), allocsize(0), cursize(0), readpos(0), overflowed(false) { }

	buf_t(size_t len)
	    : data(new byte[len]), allocsize(len), cursize(0), readpos(0), overflowed(false)
	{
	}

	buf_t(const buf_t& other)
	    : data(new byte[other.allocsize]), allocsize(other.allocsize),
	      cursize(other.cursize), readpos(other.readpos), overflowed(other.overflowed)

	{
		if (!overflowed)
			for (size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];
	}

	~buf_t()
	{
		delete[] data;
		data = NULL;
	}

	buf_t& operator=(const buf_t& other)
	{
		// Avoid self-assignment
		if (this == &other)
			return *this;

		delete[] data;

		data = new byte[other.allocsize];
		allocsize = other.allocsize;
		cursize = other.cursize;
		overflowed = other.overflowed;
		readpos = other.readpos;

		if (!overflowed)
			for (size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];

		return *this;
	}

	size_t BytesRead() const { return readpos; }

	size_t BytesLeftToRead() const
	{
		return overflowed || cursize < readpos ? 0 : cursize - readpos;
	}

	byte* ptr() { return data; }

	size_t size() const { return cursize; }

	size_t maxsize() const { return allocsize; }

	void setcursize(size_t len) { cursize = len > allocsize ? allocsize : len; }

	void clear()
	{
		cursize = 0;
		readpos = 0;
		overflowed = false;
	}

	void WriteByte(byte b);
	void WriteShort(short s);
	void WriteLong(int l);
	void WriteUnVarint(unsigned int v);
	void WriteVarint(int v);
	void WriteString(const char* c);
	void WriteChunk(const char* c, unsigned l, int startpos = 0);
	int ReadByte();
	int NextByte();
	byte* ReadChunk(size_t size);
	int ReadShort();
	int ReadLong();
	unsigned int ReadUnVarint();
	int ReadVarint();
	const char* ReadString();
	size_t SetOffset(const size_t& offset, const seek_loc_t& loc);
	void resize(size_t len, bool clearbuf = true);
	byte* GetSpace(size_t length);
};
