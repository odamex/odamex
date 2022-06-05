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

#include "odamex.h"

#include "m_buf.h"

void buf_t::WriteByte(byte b)
{
	byte* buf = GetSpace(sizeof(b));

	if (!overflowed)
	{
		*buf = b;
	}
}

void buf_t::WriteShort(short s)
{
	byte* buf = GetSpace(sizeof(s));

	if (!overflowed)
	{
		buf[0] = s & 0xff;
		buf[1] = s >> 8;
	}
}

void buf_t::WriteLong(int l)
{
	byte* buf = GetSpace(sizeof(l));

	if (!overflowed)
	{
		buf[0] = l & 0xff;
		buf[1] = (l >> 8) & 0xff;
		buf[2] = (l >> 16) & 0xff;
		buf[3] = l >> 24;
	}
}

//
// Write an unsigned varint to the wire.
//
// Use these when you want to send an int and are reasonably sure that
// the int will usually be small.
//
// [AM] If you make a change to this, please also duplicate that change
//      into varint.c in the tools directory.
//
// https://developers.google.com/protocol-buffers/docs/encoding#varints
//
void buf_t::WriteUnVarint(unsigned int v)
{
	for (;;)
	{
		// Our next byte contains 7 bits of the number.
		int out = v & 0x7F;

		// Any bits left?
		v >>= 7;
		if (v == 0)
		{
			// We're done, bail after writing this byte.
			WriteByte(out);
			return;
		}

		// Flag the last bit to indicate more is coming.
		out |= 0x80;
		WriteByte(out);
	}
}

void buf_t::WriteVarint(int v)
{
	// Zig-zag encoding for negative numbers.
	WriteUnVarint((v << 1) ^ (v >> 31));
}

void buf_t::WriteString(const char* c)
{
	if (c && *c)
	{
		size_t l = strlen(c);
		byte* buf = GetSpace(l + 1);

		if (!overflowed)
		{
			memcpy(buf, c, l + 1);
		}
	}
	else
	{
		WriteByte(0);
	}
}

void buf_t::WriteChunk(const char* c, unsigned l, int startpos)
{
	byte* buf = GetSpace(l);

	if (!overflowed)
	{
		memcpy(buf, c + startpos, l);
	}
}

int buf_t::ReadByte()
{
	if (readpos + 1 > cursize)
	{
		overflowed = true;
		return -1;
	}
	return data[readpos++];
}

int buf_t::NextByte()
{
	if (readpos + 1 > cursize)
	{
		overflowed = true;
		return -1;
	}
	return data[readpos];
}

byte* buf_t::ReadChunk(size_t size)
{
	if (readpos + size > cursize)
	{
		overflowed = true;
		return NULL;
	}
	size_t oldpos = readpos;
	readpos += size;
	return data + oldpos;
}

int buf_t::ReadShort()
{
	if (readpos + 2 > cursize)
	{
		overflowed = true;
		return -1;
	}
	size_t oldpos = readpos;
	readpos += 2;
	return (short)(data[oldpos] + (data[oldpos + 1] << 8));
}

int buf_t::ReadLong()
{
	if (readpos + 4 > cursize)
	{
		overflowed = true;
		return -1;
	}
	size_t oldpos = readpos;
	readpos += 4;
	return data[oldpos] + (data[oldpos + 1] << 8) + (data[oldpos + 2] << 16) +
	       (data[oldpos + 3] << 24);
}

//
// Read an unsigned varint off the wire.
//
// [AM] If you make a change to this, please also duplicate that change
//      into varint.c in the tools directory.
//
// https://developers.google.com/protocol-buffers/docs/encoding#varints
//
unsigned int buf_t::ReadUnVarint()
{
	unsigned char b;
	unsigned int out = 0;
	unsigned int offset = 0;

	for (;;)
	{
		// Read part of the variant.
		b = ReadByte();
		if (overflowed)
			return -1;

		// Shove the first seven bits into our output variable.
		out |= (unsigned int)(b & 0x7F) << offset;
		offset += 7;

		// Is the flag bit set?
		if (!(b & 0x80))
		{
			// Nope, we're done.
			return out;
		}

		if (offset >= 32)
		{
			// Our variant int is too big - overflow us.
			overflowed = true;
			return -1;
		}
	}
}

int buf_t::ReadVarint()
{
	unsigned int uv = ReadUnVarint();
	if (overflowed)
		return -1;

	// Zig-zag encoding for negative numbers.
	return (uv >> 1) ^ (0U - (uv & 1));
}

const char* buf_t::ReadString()
{
	byte* begin = data + readpos;

	while (ReadByte() > 0)
		;

	if (overflowed)
	{
		return "";
	}

	return (const char*)begin;
}

size_t buf_t::SetOffset(const size_t& offset, const seek_loc_t& loc)
{
	switch (loc)
	{
	case BT_SSET: {
		if (offset > cursize)
		{
			overflowed = true;
			return 0;
		}

		readpos = offset;
	}
	break;

	case BT_SCUR: {
		if (readpos + offset > cursize)
		{
			overflowed = true;
			return 0;
		}

		readpos += offset;
	}

	case BT_SEND: {
		if ((int)(readpos - offset) < 0)
		{
			// lies, an underflow occured
			overflowed = true;
			return 0;
		}

		readpos -= offset;
	}
	}

	return readpos;
}

void buf_t::resize(size_t len, bool clearbuf /* = true */)
{
	byte* olddata = data;
	data = new byte[len];
	allocsize = len;

	if (!clearbuf)
	{
		if (cursize < allocsize)
		{
			memcpy(data, olddata, cursize);
		}
		else
		{
			clear();
			overflowed = true;
			Printf(PRINT_HIGH, "buf_t::resize(): overflow\n");
		}
	}
	else
	{
		clear();
	}

	delete[] olddata;
}

byte* buf_t::GetSpace(size_t length)
{
	if (cursize + length > allocsize)
	{
		clear();
		overflowed = true;
#if defined(ODAMEX_DEBUG)
		Printf(PRINT_HIGH, "buf_t::GetSpace: overflow\n");
#endif
	}

	byte* ret = data + cursize;
	cursize += length;

	return ret;
}

std::string buf_t::debugString()
{
	return DebugByteString(&data[0], &data[cursize], &data[readpos]);
}

#if defined(ODAMEX_DEBUG)

#include "c_dispatch.h"

BEGIN_COMMAND(test_buf)
{
	buf_t buf{8};

	buf.WriteByte(8);
	buf.WriteShort(1234);
	buf.WriteLong(1234567);
	buf.WriteByte(255);

	Printf("%s\n", buf.debugString().c_str());
	buf.ReadLong();
	buf.ReadByte();
	buf.ReadByte();
	buf.ReadByte();
	Printf("%s\n", buf.debugString().c_str());
	buf.ReadByte();
	Printf("%s\n", buf.debugString().c_str());
}
END_COMMAND(test_buf)

#endif
