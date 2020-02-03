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
// Miscellaneous definitions and functions common to the networking subsystem.
//
//-----------------------------------------------------------------------------

#ifndef __NET_TYPE_H__
#define __NET_TYPE_H__

#include "doomtype.h"
#include <cstring>
#include <queue>

#include "network/net_bitstream.h"


// ============================================================================
//
// BitField
//
// Efficiently stores a boolean value for up to N different fields.
//
// ============================================================================

template <uint32_t N>
class BitField
{
public:
	BitField()
	{
		clear();
	}

	BitField(const BitField<N>& other)
	{
		memcpy(mData, other.mData, BYTE_COUNT);
	}

	BitField<N>& operator=(const BitField<N>& other)
	{
		if (&other != this)
			memcpy(mData, other.mData, BYTE_COUNT);
		return *this;
	}

	uint16_t size() const
	{
		return N;
	}

	void clear()
	{
		memset(mData, 0, BYTE_COUNT);
	}

	bool empty() const
	{
		for (uint32_t i = 0; i < BYTE_COUNT; ++i)
			if (mData[i] != 0)
				return false;
		return true;
	}

	void set(uint32_t flag)
	{
		if (flag < N)
			mData[flag >> 3] |= (1 << (flag & 0x7));
	}

	void unset(uint32_t flag)
	{
		if (flag < N)
			mData[flag >> 3] &= ~(1 << (flag & 0x7));
	}

	bool get(uint32_t flag) const
	{
		if (flag < N)
			return (mData[flag >> 3] & (1 << (flag & 0x7))) != 0;
		return false;
	}

	void read(BitStream& stream)
	{
		clear();
		for (uint32_t i = 0; i < N; i++)
			if (stream.readBit() == 1)
				set(i);
	}

	void write(BitStream& stream) const
	{
		for (uint32_t i = 0; i < N; i++)
			stream.writeBit(get(i));
	}

	bool operator== (const BitField<N>& other) const
	{
		for (uint32_t i = 0; i < BYTE_COUNT - 1; ++i)
			if (mData[i] != other.mData[i])
				return false;
		
		return true;
	}

	bool operator!= (const BitField<N>& other) const
	{
		return !operator==(other);
	}

	BitField<N> operator| (const BitField<N>& other) const
	{
		BitField<N> result(*this);
		result |= other;
		return result;
	}

	BitField<N> operator& (const BitField<N>& other) const
	{
		BitField<N> result(*this);
		result &= other;
		return result;
	}

	BitField<N> operator^ (const BitField<N>& other) const
	{
		BitField<N> result(*this);
		result ^= other;
		return result;
	}

	BitField<N> operator~ () const
	{
		BitField<N> result;
		for (uint32_t i = 0; i < BYTE_COUNT; ++i)
			result.mData[i] = ~mData[i];
		
		return result;
	}

	BitField<N>& operator|= (const BitField<N>& other)
	{
		for (uint32_t i = 0; i < BYTE_COUNT; ++i)
			mData[i] |= other.mData[i];
		return *this;
	}

	BitField<N>& operator&= (const BitField<N>& other)
	{
		for (uint32_t i = 0; i < BYTE_COUNT; ++i)
			mData[i] &= other.mData[i];
		return *this;
	}

	BitField<N>& operator^= (const BitField<N>& other)
	{
		for (uint32_t i = 0; i < BYTE_COUNT; ++i)
			mData[i] ^= other.mData[i];
		return *this;
	}

	BitField<N> operator>> (uint32_t cnt) const
	{
		BitField<N> result(*this);
		result >>= cnt;
		return result;
	}

	BitField<N> operator<< (uint32_t cnt) const
	{
		BitField<N> result(*this);
		result <<= cnt;
		return result;	
	}

	BitField<N>& operator>>= (uint32_t cnt)
	{
		if (cnt >= N)
		{
			clear();
			return *this;
		}

		const uint32_t byteshift = cnt >> 3;
		const uint32_t bitshift = cnt & 0x07;
		for (int i = 0; i < (int)BYTE_COUNT - (int)byteshift - 1; ++i)
			mData[i] = (mData[i + byteshift] >> bitshift) | (mData[i + byteshift + 1] << (8 - bitshift));
		mData[BYTE_COUNT - byteshift - 1] = mData[BYTE_COUNT - 1] >> bitshift;
		memset(mData + BYTE_COUNT - byteshift, 0, byteshift);

		return *this;
	}

	BitField<N>& operator<<= (uint32_t cnt)
	{
		if (cnt >= N)
		{
			clear();
			return *this;
		}

		const uint32_t byteshift = cnt >> 3;
		const uint32_t bitshift = cnt & 0x07;
		for (int i = (int)BYTE_COUNT - 1; i > (int)byteshift; --i)
			mData[i] = (mData[i - byteshift] << bitshift) | (mData[i - byteshift - 1] >> (8 - bitshift));
		mData[byteshift] = mData[0] << bitshift;
		memset(mData, 0, byteshift);

		return *this;
	}

private:
	static const uint32_t BYTE_COUNT = (N + 7) / 8;
	static const uint8_t LAST_BYTE_MASK = (N & 7);
	uint8_t		mData[BYTE_COUNT];
};


// ============================================================================
//
// SequenceNumber
//
// Sequence number data type, which handles integer wrap-around and
// allows for an arbitrary N-bit sequence number (up to 32 bits).
//
// ============================================================================

template <uint8_t N = 32>
class SequenceNumber
{
public:
	SequenceNumber(uint32_t seq = 0) :
		mSequence(seq & SEQUENCE_MASK)
	{ }

	void setInteger(uint32_t seq)
	{
		mSequence = seq & SEQUENCE_MASK;
	}

	uint32_t getInteger() const
	{
		return mSequence;
	}

	void read(BitStream& stream)
	{
		mSequence = stream.readBits(SEQUENCE_BITS);
	}

	void write(BitStream& stream) const
	{
		stream.writeBits(mSequence, SEQUENCE_BITS);
	}

	bool operator== (const SequenceNumber<N>& other) const
	{
		return mSequence == other.mSequence;
	}

	bool operator!= (const SequenceNumber<N>& other) const
	{
		return mSequence != other.mSequence;
	}

	bool operator> (const SequenceNumber<N>& other) const
	{
		static const uint32_t MIDPOINT = (1 << (SEQUENCE_BITS - 1));

		return	(mSequence > other.mSequence && (mSequence - other.mSequence) <= MIDPOINT) ||
				(mSequence < other.mSequence && (other.mSequence - mSequence) > MIDPOINT);
	}

	bool operator>= (const SequenceNumber<N>& other) const
	{
		return operator==(other) || operator>(other);
	}

	bool operator< (const SequenceNumber<N>& other) const
	{
		return !(operator>=(other));
	}

	bool operator<= (const SequenceNumber<N>& other) const
	{
		return !(operator>(other));
	}

	SequenceNumber<N> operator+ (const SequenceNumber<N>& other) const
	{
		SequenceNumber<N> result(*this);
		return result += other;
	}

	SequenceNumber<N> operator- (const SequenceNumber<N>& other) const
	{
		SequenceNumber<N> result(*this);
		return result -= other;
	}

	SequenceNumber<N>& operator= (const SequenceNumber<N>& other)
	{
		mSequence = other.mSequence;
		return *this;
	}

	SequenceNumber<N>& operator+= (const SequenceNumber<N>& other)
	{
		mSequence = (mSequence + other.mSequence) & SEQUENCE_MASK;
		return *this;
	}

	SequenceNumber<N>& operator-= (const SequenceNumber<N>& other)
	{
		mSequence = (mSequence - other.mSequence) & SEQUENCE_MASK;
		return *this;
	}

	SequenceNumber<N>& operator++ ()
	{
		mSequence = (mSequence + 1) & SEQUENCE_MASK;
		return *this;
	}

	SequenceNumber<N>& operator-- ()
	{
		mSequence = (mSequence - 1) & SEQUENCE_MASK;
		return *this;
	}

	SequenceNumber<N> operator++ (int)
	{
		SequenceNumber<N> result(*this);
		operator++();
		return result;
	}

	SequenceNumber<N> operator-- (int)
	{
		SequenceNumber<N> result(*this);
		operator--();
		return result;
	}

private:
	static const uint8_t	SEQUENCE_BITS = N;	// must be <= 32
	static const uint32_t	SEQUENCE_MASK = (1ULL << N) - 1;
	static const uint64_t	SEQUENCE_UNIT = (1ULL << N);

	uint32_t				mSequence;
};

template <uint8_t N>
int32_t SequenceNumberDifference(const SequenceNumber<N> a, const SequenceNumber<N> b)
{
	if (a > b)
		return (a - b).getInteger();
	else
		return -(b - a).getInteger();	
}

// ============================================================================
//
// IdGenerator
//
// Hands out unique IDs of the type SequenceNumber. The range for the IDs
// generated includes 0 through 2^N - 1. Although the IDs generated are in
// order at first, they are not guaranteed of being so since the order in
// which IDs are freed eventually impacts the IDs generated.
//
// Note that there is no error checking made when generating a new ID if
// there are no free IDs and there is no checking to prevent freeing
// an ID twice. Be careful to not use values of N larger than about 16.
//
// ============================================================================

template <uint8_t N>
class IdGenerator
{
public:
	IdGenerator()
	{
		clear();
	}

	virtual ~IdGenerator()
	{ }

	virtual bool empty() const
	{
		return mFreeIds.empty();
	}

	virtual void clear()
	{
		while (!mFreeIds.empty())
			mFreeIds.pop();

		for (uint32_t i = 0; i < mNumIds; i++)
			mFreeIds.push(i);
	}

	virtual SequenceNumber<N> generate()
	{
		// TODO: Add check for mFreeIds.empty()
		SequenceNumber<N> id = mFreeIds.front();
		mFreeIds.pop();
		return id;
	}

	virtual void free(const SequenceNumber<N>& id)
	{
		// TODO: Add check to ensure id isn't already free
		mFreeIds.push(id);
	}

private:
	static const uint32_t			mNumIds = (1 << N);
	std::queue<SequenceNumber<N> >	mFreeIds;
};


// ============================================================================
//
// SequentialIdGenerator
//
// Hands out unique IDs of the type SequenceNumber. The range for the IDs
// generated includes 0 through 2^N - 1. The IDs generated are guaranteed to
// be in sequential order. This imposes the constraint that IDs must also be
// freed in sequential order.
//
// ============================================================================

template <uint8_t N>
class SequentialIdGenerator : public IdGenerator<N>
{
public:
	SequentialIdGenerator()
	{
		clear();
	}

	virtual ~SequentialIdGenerator()
	{ }

	virtual bool empty() const
	{
		return mNextId == mFirstId;
	}

	virtual void clear()
	{
		mNextId = 0;
	}

	virtual SequenceNumber<N> generate()
	{
		// TODO: throw exception if there are no free IDs
		return mNextId++;
	}

	virtual void free(const SequenceNumber<N>& id)
	{
		// TODO: throw exception if empty or id != mFirstId
		++mFirstId;
	}

private:
	static const uint32_t		mNumIds = (1 << N);
	SequenceNumber<N>			mNextId;
	SequenceNumber<N>			mFirstId;
};

#endif	// __NET_TYPE_H__

