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
// Network buffer that permits bitlevel reading and writing
//
//-----------------------------------------------------------------------------

#include "doomtype.h"
#include "doomdef.h"
#include "m_ostring.h"

#include "network/net_bitstream.h"

// ============================================================================
//
// BitStream class Implementation
//
// ============================================================================

// ----------------------------------------------------------------------------
// Public functions
// ----------------------------------------------------------------------------

BitStream::BitStream() :
	mWritten(0), mRead(0), mWriteOverflow(false), mReadOverflow(false)
{ }


BitStream::BitStream(const BitStream &other) :
	mWritten(other.mWritten), mRead(other.mRead),
	mWriteOverflow(other.mWriteOverflow),
	mReadOverflow(other.mReadOverflow)
{
	memcpy(mBuffer, other.mBuffer, other.bytesWritten());
}


BitStream::~BitStream()
{ }


BitStream& BitStream::operator=(const BitStream& other)
{
	if (this != &other)
	{
		mWritten = other.mWritten;
		mRead = other.mRead;
		mWriteOverflow = other.mWriteOverflow;
		mReadOverflow = other.mReadOverflow;
		memcpy(mBuffer, other.mBuffer, other.bytesWritten());
	}

	return *this;
}


//
// BitStream::clear
//
// Resets the read and write pointers in the buffer.
//
void BitStream::clear()
{
	mWritten = mRead = 0;
	mWriteOverflow = mReadOverflow = false;
}


//
// BitStream::bytesRead
//
// Calculates how many bytes have been read from the buffer. This includes
// only partially filled bytes.
//
uint16_t BitStream::bytesRead() const
{
	return (mRead + 0x07) >> 3;
}


//
// BitStream::bytesWritten
//
// Calculates how many bytes have been written to the buffer. This includes
// only partially filled bytes.
//
uint16_t BitStream::bytesWritten() const
{
	return (mWritten + 0x07) >> 3;
}


//
// BitStream::bitsRead
//
// Returns how many bits have been read from the buffer.
//
uint16_t BitStream::bitsRead() const
{
	return mRead;
}


//
// BitStream::bitsWritten
//
// Returns how many bits have been written to the buffer.
//
uint16_t BitStream::bitsWritten() const
{
	return mWritten;
}


//
// BitStream::readSize
//
// Calculates how many bits can be read from the buffer given the buffer size
// and the current read pointer.
//
uint16_t BitStream::readSize() const
{
	if (mWritten > mRead)
		return mWritten - mRead;
	else
		return 0;
}


//
// BitStream::writeSize
//
// Calculates how many bits can be written to the buffer given the buffer size
// and the current write pointer.
//
uint16_t BitStream::writeSize() const
{
	if (mWritten < BitStream::MAX_CAPACITY)
		return BitStream::MAX_CAPACITY - mWritten;
	else
		return 0;
}


// 
// BitStream::writeBits
//
// Writes the bitcount lowest bits of val to the buffer,
// most significant bit first. All of the public write member functions
// use this function as their core.
//
void BitStream::writeBits(int val, uint16_t bitcount)
{
	bitcount = std::min<uint16_t>(bitcount, 32);

	if (mCheckWriteOverflow(bitcount))
		return;

	// Odamex uses little-endian byte ordering for network transmission.
	val = LELONG(val);

	while (bitcount)
	{
		uint32_t* ptr = (uint32_t*)mBuffer + (mWritten >> 5);

		uint16_t startbit = mWritten & 31;
		uint16_t bitstowrite = std::min<uint16_t>(32 - startbit, bitcount);

		if (startbit == 0)
			*ptr = 0;		// if starting a new byte, clear it first

        // write the bitstowrite most significant remaining bits of val to mBuffer
        *ptr |= (val >> (bitcount - bitstowrite)) << (32 - startbit - bitstowrite);
        // mask off the bits from val that we've already written to mBuffer
        val &= ((1 << (bitcount - bitstowrite)) - 1);

		bitcount -= bitstowrite;
		mWritten += bitstowrite;
	}
}


//
// BitStream::peekBits
//
// Examines bitcount bits from the buffer, most significant bit first. This
// does not advance the read position.
//
int BitStream::peekBits(uint16_t bitcount) const
{
	bitcount = std::min<uint16_t>(bitcount, 32);

	if (mCheckReadOverflow(bitcount))
		return 0;

	int val = 0;
	uint16_t readpos = mRead;

	while (bitcount)
	{
		const uint32_t* ptr = (uint32_t*)mBuffer + (readpos >> 5);

		uint16_t startbit = readpos & 31;
		uint16_t bitstoread = std::min<uint16_t>(32 - startbit, bitcount);

		uint32_t mask = ((uint64_t(1) << bitstoread) - 1) << startbit;
		val |= (LELONG(*ptr) & mask) >> startbit;

		bitcount -= bitstoread;
		readpos += bitstoread;
	}

	return val;
}


//
// BitStream::readBits
//
// Reads bitcount bits from the buffer, most significant bit first. All of
// the public read member functions use this function as their core.
//
int BitStream::readBits(uint16_t bitcount)
{
	bitcount = std::min<uint16_t>(bitcount, 32);

	if (mCheckReadOverflow(bitcount))
		return 0;

	int val = peekBits(bitcount);
	mRead += bitcount;
	return val;
}


void BitStream::writeBit(int val)
{
	writeBits(val, 1);
}


bool BitStream::readBit()
{
	return static_cast<bool>(readBits(1));
}


void BitStream::writeS8(int val)
{
	writeBits(val, 8);
}


int8_t BitStream::readS8()
{
	return static_cast<int8_t>(readBits(8));
}


void BitStream::writeU8(int val)
{
	writeBits(val, 8);
}


uint8_t BitStream::readU8()
{
	return static_cast<uint8_t>(readBits(8));
}


void BitStream::writeS16(int val)
{
	writeBits(val, 16);
}


int16_t BitStream::readS16()
{
	return static_cast<int16_t>(readBits(16));
}


void BitStream::writeU16(int val)
{
	writeBits(val, 16);
}


uint16_t BitStream::readU16()
{
	return static_cast<uint16_t>(readBits(16));
}


void BitStream::writeS32(int val)
{
	writeBits(val, 32);
}


int32_t BitStream::readS32()
{
	return static_cast<int32_t>(readBits(32));
}


void BitStream::writeU32(int val)
{
	writeBits(val, 32);
}


uint32_t BitStream::readU32()
{
	return static_cast<uint32_t>(readBits(32));
}


void BitStream::writeString(const OString& str)
{
	if (mCheckWriteOverflow((str.length() + 1) << 3))
		return;

	for (uint16_t i = 0; i < str.length(); i++)
		writeU8(str[i]);

	writeU8(0);
}


OString BitStream::readString()
{
	const size_t bufsize = BitStream::MAX_CAPACITY / 8;
	char data[bufsize];

	for (uint16_t i = 0; i < bufsize && !mCheckReadOverflow(8); i++)
	{
		data[i] = readS8();
		if (data[i] == 0)
			break;
	}

	// make sure the string is properly terminated no matter what
	data[bufsize - 1] = 0;

	return OString(data);
}


//
// BitStream::writeFloat
//
// Writes a single-precision float to the buffer.
//
// The value is encoded in IEEE-754 floating point format for portability
// and then written to the buffer. Note that this method uses 32 bits for
// a single-precision float. Platforms that use IEEE-754 as their internal
// floating point storage method can simply cast the float to an unsigned int
// as a speed-up.
//
// Based on public domain code by Brian "Beej Jorgensen" Hall, found at:
// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
//
void BitStream::writeFloat(float val)
{
	static const size_t bits = 32;
	static const size_t expbits = 8;

	// handle this special case
	if (val == 0.0f)
		return writeBits(0, bits);

	unsigned int packedsign = ((val < 0.0f) ? 1 : 0) << (bits - 1);
	long double fnorm = (val < 0.0f) ? -val : val;
	int shift = 0;

	unsigned int exp, significand;
	unsigned int significandbits = bits - expbits - 1;

	// get the normalized form of val and track the exponent
	while (fnorm >= 2.0f)
	{
		fnorm /= 2.0f;
		shift++;
	}
	
	while (fnorm < 1.0f)
	{
		fnorm *= 2.0f;
		shift--;
	}

	fnorm -= 1.0f;

	// calculate the binary form (non-float) of the significand data
	significand = fnorm * ((1LL << significandbits) + 0.5f);

	// get the biased exponent
	exp = shift + ((1 << (expbits - 1)) - 1); // shift + bias

	unsigned int packed = packedsign | (exp << (bits - expbits - 1)) | significand;
	writeBits(packed, bits);	
}


//
// BitStream::readFloat
//
// Reads a single-precision float from the buffer.
//
// The float is unpacked from 32-bit IEEE-754 format into the native
// single-precision float format.
//
// Based on public domain code by Brian "Beej Jorgensen" Hall, found at:
// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#serialization
//
float BitStream::readFloat()
{
	static const size_t bits = 32;
	static const size_t expbits = 8;

	long double val;
	long long shift;
	unsigned int bias;
	unsigned int significandbits = bits - expbits - 1;

	unsigned int packed = readBits(bits);

	// handle special case
	if (packed == 0)
		return 0.0f;

	// pull the significand
	val = (packed & ((1LL << significandbits) - 1)); // mask
	val /= (1LL << significandbits); // convert back to float
	val += 1.0f; // add the one back on

	// deal with the exponent
	bias = (1 << (expbits - 1)) - 1;
	shift = ((packed >> significandbits) & ((1LL << expbits) - 1)) - bias;
	while (shift > 0)
	{
		val *= 2.0f;
		shift--;
	}
	while (shift < 0)
	{
		val /= 2.0f;
		shift++;
	}

	// handle the sign
	if ((packed >> (bits - 1)) & 1)
		return val * -1.0f;
	else
		return val;
}


//
// BitStream::writeColor
//
// Writes a color value in ARGB8888 format.
//
void BitStream::writeColor(const argb_t color)
{
	writeBits(color.getb(), 8);
	writeBits(color.getg(), 8);
	writeBits(color.getr(), 8);
	writeBits(color.geta(), 8);
}


//
// BitStream::readColor
//
// Reads a color value in ARGB8888 format.
//
argb_t BitStream::readColor()
{
	uint8_t b = readBits(8);
	uint8_t g = readBits(8);
	uint8_t r = readBits(8);
	uint8_t a = readBits(8);
	return argb_t(a, r, g, b);
}


//
// BitStream::writeBlob
//
// Writes a binary blob to the BitStream. Size is specified in bits.
//
void BitStream::writeBlob(const uint8_t *data, uint16_t size)
{
	if (mCheckWriteOverflow(size))
		return;

	if ((mWritten & 0x07) == 0)
	{
		// the buffer is currently byte aligned - use memcpy for speed
		uint8_t *ptr = mBuffer + (mWritten >> 3);
		memcpy(ptr, data, size >> 3);
		mWritten += (size & ~0x07);
	}
	else
	{
		// we have to use the slower writeBits method
		for (uint16_t i = 0; i < size >> 3; i++)
			writeBits(data[i], 8);
	}

	// write any remaining bits
	if (size & 0x07)
		writeBits(data[size >> 3], size & 0x07);
}


//
// BitStream::readBlob
//
// Reads a binary blob from the BitStream. Size is specified in bits.
//
void BitStream::readBlob(uint8_t *data, uint16_t size)
{
	if (mCheckReadOverflow(size))
		return;

	if ((mRead & 0x07) == 0)
	{
		// the buffer is currently byte aligned - use memcpy for speed
		uint8_t *ptr = mBuffer + (mRead >> 3);
		memcpy(data, ptr, size >> 3);
		mRead += (size & ~0x07);
	}
	else
	{
		// we have to use the slower readBits method
		for (uint16_t i = 0; i < size >> 3; i++)
			data[i] = readBits(8);
	}

	// read any remaining bits
	if (size & 0x07)
		data[size >> 3] = readBits(size & 0x07);
}


bool BitStream::peekBit() const
{
	return static_cast<bool>(peekBits(1));
}


int8_t BitStream::peekS8() const
{
	return static_cast<int8_t>(peekBits(8));
}


uint8_t BitStream::peekU8() const
{
	return static_cast<uint8_t>(peekBits(8));
}


int16_t BitStream::peekS16() const
{
	return static_cast<int8_t>(peekBits(16));
}


uint16_t BitStream::peekU16() const
{
	return static_cast<uint16_t>(peekBits(16));
}


int32_t BitStream::peekS32() const
{
	return static_cast<int32_t>(peekBits(32));
}


uint32_t BitStream::peekU32() const
{
	return static_cast<uint32_t>(peekBits(32));
}


//
// BitStream::getRawData
//
// Returns a const handle to the internal storage buffer. This function should
// only be used when writing the contents of the stream to a network socket.
//
const uint8_t* BitStream::getRawData() const
{
	return mBuffer;
}


// ----------------------------------------------------------------------------
// Private member functions
// ----------------------------------------------------------------------------

//
// BitStream::mCheckWriteOverflow
//
// Returns true and sets mWriteOverflow if writing s bits would write past the
// end of the buffer.
//
bool BitStream::mCheckWriteOverflow(uint16_t s) const
{
	if (mWritten + s > BitStream::MAX_CAPACITY)
		mWriteOverflow = true;

	return mWriteOverflow;
}

//
// BitStream::mCheckReadOverflow
//
// Returns true and sets mReadOverflow if reading s bits would read past the
// end of the buffer.
//
bool BitStream::mCheckReadOverflow(uint16_t s) const
{
	if (mRead + s > BitStream::MAX_CAPACITY)
		mReadOverflow = true;
	
	return mReadOverflow;
}


VERSION_CONTROL (net_bitstream_cpp, "$Id$")

