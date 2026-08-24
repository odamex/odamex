// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by Jim Thoenen.
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
//  The canonical packet header type
//
//-----------------------------------------------------------------------------
#pragma once

#include <cstdint>

#include "i_net.h"

/// The canonical type for the header we put at the start of every packet.
/// Use this type to pack and/or unpack headers.
struct PacketHeaderType
{
	const static size_t PACKET_SEQUENCE_INDEX      = 0;
	const static size_t PACKET_ORIGINATOR_TIC      = 4;
	const static size_t PACKET_DESTINATION_TIC     = 8;
	const static size_t PACKET_RELIABLE_SIZE_INDEX = 12;
	const static size_t PACKET_FLAG_INDEX          = 14;
	const static size_t PACKET_MESSAGE_INDEX       = 16;
	const static size_t PACKET_HEADER_SIZE         = PACKET_MESSAGE_INDEX;

	const static uint16_t FLAG_COMPRESSED    =    0x1;
	const static uint16_t FLAG_HIGH_PRIORITY =    0x2;
	const static uint16_t FLAG_UNUSED_MASK   = 0xFFFC;

	int32_t  sequence;
	int32_t  originatorTic;
	int32_t  destinationTic;
	uint16_t reliableSize;
	uint16_t flags;

	bool operator==(const PacketHeaderType& other) const = default;

	explicit PacketHeaderType(int32_t i_sequence) :
		sequence        (i_sequence),
		originatorTic   (-1),
		destinationTic  (-1),
		reliableSize    (0),
		flags           (0)
	{
	}

	PacketHeaderType() :
		PacketHeaderType(-1)
	{
	}

	explicit PacketHeaderType(buf_t& io_buf) :
		sequence        (io_buf.ReadLong()),
		originatorTic   (io_buf.ReadLong()),
		destinationTic  (io_buf.ReadLong()),
		reliableSize    (io_buf.ReadShort()),
		flags           (io_buf.ReadShort())
	{
	}

	void Unpack(buf_t& io_buf)
	{
		sequence        = io_buf.ReadLong();
		originatorTic   = io_buf.ReadLong();
		destinationTic  = io_buf.ReadLong();
		reliableSize    = io_buf.ReadShort();
		flags           = io_buf.ReadShort();
	}

	void Pack(buf_t& io_buf)
	{
		io_buf.WriteLong (sequence);
		io_buf.WriteLong (originatorTic);
		io_buf.WriteLong (destinationTic);
		io_buf.WriteShort(reliableSize);
		io_buf.WriteShort(flags);
	}
};
