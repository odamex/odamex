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
// Packet abstraction 
//
//-----------------------------------------------------------------------------

#include "doomdef.h"

#include "network/net_type.h"
#include "network/net_common.h"
#include "network/net_bitstream.h"
#include "network/net_packet.h"

// ============================================================================
//
// Packet class implementation
//
// ============================================================================

Packet::PacketDataStore Packet::mPacketData(128);

// ----------------------------------------------------------------------------
// Construction / Destruction
// ----------------------------------------------------------------------------

Packet::Packet() :
	mId(0), mData(NULL)
{ }

Packet::Packet(const Packet& other) :
	mId(other.mId), mData(other.mData)
{
	if (mData)
		mData->mRefCount++;
}

Packet::~Packet()
{
	if (mData)
	{
		mData->mRefCount--;
		if (mData->mRefCount == 0)
			mPacketData.erase(mId);
	}
}

Packet Packet::operator=(const Packet& other)
{
	if (mId != other.mId)
	{
		if (mData)
		{
			mData->mRefCount--;
			if (mData->mRefCount == 0)
				mPacketData.erase(mId);
		}
		
		mId = other.mId;
		mData = other.mData;
		if (mData)
			mData->mRefCount++;
	}

	return *this;
}
	

// ----------------------------------------------------------------------------
// Public functions
// ----------------------------------------------------------------------------

//
// Packet::clear
//
void Packet::clear()
{
	mData->clear();
}


//
// Packet::getSize
//
// Returns the size of the packet in bits, including the header.
//	
uint16_t Packet::getSize() const
{
	return mData->mPayload.bitsWritten();
}


//
// Packet::writePacketData
//
// Writes the packet contents to the given buffer. The caller indicates
// the maximum size buffer can hold (in bits) with the size parameter.
//
// Returns the number of bits written to the buffer.
//
uint16_t Packet::writePacketData(uint8_t* buf, uint16_t size) const
{
	const uint16_t packet_size = getSize();
	if (size < packet_size)
		return 0;

	mData->mPayload.readBlob(buf, size);
	return packet_size;
}


//
// Packet::readPacketData
//
// Reads the contents from the given buffer and constructs a packet
// by parsing the header and separating the payload.
//
// Returns the number of bits read from the buffer.
//
uint16_t Packet::readPacketData(const uint8_t* buf, uint16_t size)
{
	mData->clear();
	mData->mPayload.writeBlob(buf, size);
	return size;
}


// ============================================================================
//
// PacketFactory class implementation
//
// ============================================================================


//
// PacketFactory::createPacket
//
// Returns an instance of an unused Packet from the memory pool.
//
Packet PacketFactory::createPacket()
{
	Packet packet;
	packet.mId = Packet::mPacketData.insert();
	packet.mData = &(Packet::mPacketData.get(packet.mId));
	packet.mData->clear();	
	packet.mData->mRefCount = 1;
	
	return packet;
}


Packet PacketFactory::createPacket(const uint8_t* buf, uint16_t size)
{
	Packet packet = createPacket();
	packet.readPacketData(buf, size);
	return packet;
}

