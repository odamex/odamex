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

static uint32_t mask[] = {
	0x00000000, 0x00000001, 0x00000003, 0x00000007,
	0x0000000F, 0x0000001F, 0x0000003F, 0x0000007F,
	0x000000FF, 0x000001FF, 0x000003FF, 0x000007FF,
	0x00000FFF, 0x00001FFF, 0x00003FFF, 0x00007FFF,
	0x0000FFFF, 0x0001FFFF, 0x0003FFFF, 0x0007FFFF,
	0x000FFFFF, 0x001FFFFF, 0x003FFFFF, 0x007FFFFF,
	0x00FFFFFF, 0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF,
	0x0FFFFFFF, 0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF,
	0xFFFFFFFF
};


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
{
	#if 0
	testWriteBits();
	testReadBits();
	#endif
}


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
// BitStream::testWriteBits
//
// Unit test for writing an arbitrary number of bits to the buffer.
//
void BitStream::testWriteBits()
{
	clear();

	uint32_t val = 0x04030201;
	writeBits(val, 32);
	assert(mBuffer[0] == 0x01);
	assert(mBuffer[1] == 0x02);
	assert(mBuffer[2] == 0x03);
	assert(mBuffer[3] == 0x04);

    val = 0x0201;
	writeBits(val, 16);
	assert(mBuffer[4] == 0x01);
	assert(mBuffer[5] == 0x02);

	val = 0x0AFF;
	writeBits(val, 12);
	assert(mBuffer[6] == 0xFF);
	assert(mBuffer[7] == 0xA0);

	val = 0xA9;
	writeBits(val, 8);
	assert(mBuffer[7] = 0xAA);
	assert(mBuffer[8] = 0x90);

	clear();
}


//
// BitStream::testReadBits
//
// Unit test for reading an arbitrary number of bits from the buffer.
//
void BitStream::testReadBits()
{
	clear();

	uint32_t val = 0x04030201;
	writeBits(val, 32);
	assert(readBits(32) == val);

    val = 0x0201;
	writeBits(val, 16);
	assert(readBits(16) == val);

	val = 0x0AFF;
	writeBits(val, 12);
	assert(readBits(12) == val);

	val = 0xA9;
	writeBits(val, 8);
	assert(readBits(8) == val);

	clear();
}


// 
// BitStream::writeBits
//
// Writes the bitcount lowest bits of val to the buffer in little-endian byte
// ordering.
//
// All of the public write member functions use this function as their core.
//
void BitStream::writeBits(uint32_t val, uint16_t bitcount)
{
	bitcount = std::min<uint16_t>(bitcount, 32);

	if (mCheckWriteOverflow(bitcount))
		return;

	uint32_t* ptr = (uint32_t*)mBuffer + (mWritten >> 5);

	// start with a clear scratch pad
	uint64_t scratch = 0;

	// load the scratch pad from mBuffer if it's not blank
	uint16_t scratch_bits_written = mWritten & 31;
	if (scratch_bits_written > 0)
		scratch = uint64_t(BELONG(*ptr)) << 32;

	mWritten += bitcount;

	while (bitcount)
	{
		// Write the least-significant byte (or remainder) to the
		// scratch pad. We have to write one byte at a time to maintain
		// little-endian order.
		uint16_t bits_to_write = std::min<uint16_t>(bitcount, 8);

		// Get the next 8 least significant bits. If there are less than 8 remaining bits,
		// shift the remaining bits to the left.
		uint64_t lsb = val & mask[bits_to_write];

		// Write the lowest 8 bits to the scratch pad
		scratch |= lsb << (64 - scratch_bits_written - bits_to_write);

		val >>= bits_to_write;
		scratch_bits_written += bits_to_write;
		bitcount -= bits_to_write;
	}

	// flush the scratch pad to mBuffer
	*ptr = BELONG(scratch >> 32);
	if (scratch_bits_written > 32)
		*(++ptr) = BELONG(scratch & mask[32]);
}


//
// BitStream::peekBits
//
// Examines bitcount bits from the buffer, most significant bit first. This
// does not advance the read position.
//
uint32_t BitStream::peekBits(uint16_t bitcount) const
{
	bitcount = std::min<uint16_t>(bitcount, 32);

	if (mCheckReadOverflow(bitcount))
		return 0;

	uint32_t val = 0;

	uint16_t scratch_bits_read = mRead & 31;

	uint32_t* ptr = (uint32_t*)mBuffer + (mRead >> 5);
	uint64_t scratch = uint64_t(BELONG(*ptr)) << 32;
	if (scratch_bits_read + bitcount > 32)
		scratch |= uint64_t(BELONG(*(++ptr)));

	uint16_t val_offset = 0;
	while (bitcount)
	{
		uint16_t bits_to_read = std::min<uint16_t>(bitcount, 8);

		// Read the 8 next least-significant bits
		uint64_t lsb = (scratch >> (64 - scratch_bits_read - bits_to_read)) & mask[bits_to_read];

		// Add to output
		val |= (lsb << val_offset);

		val_offset += bits_to_read;
		scratch_bits_read += bits_to_read;
		bitcount -= bits_to_read;
	}

	return val;
}


//
// BitStream::readBits
//
// Reads bitcount bits from the buffer, most significant bit first. All of
// the public read member functions use this function as their core.
//
uint32_t BitStream::readBits(uint16_t bitcount)
{
	bitcount = std::min<uint16_t>(bitcount, 32);

	if (mCheckReadOverflow(bitcount))
		return 0;

	uint32_t val = peekBits(bitcount);
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

