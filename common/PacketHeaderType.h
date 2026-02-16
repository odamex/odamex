// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//-----------------------------------------------------------------------------sx
#pragma once

#include <cstdint>

#include "i_net.h"

/// The canonical type for the header we put at the start of every packet.
/// Use this type to pack and/or unpack headers.
struct PacketHeaderType
{
	const static size_t PACKET_SEQUENCE_INDEX      = 0;
	const static size_t PACKET_RELIABLE_SIZE_INDEX = 4;
	const static size_t PACKET_FLAG_INDEX          = 6;
	const static size_t PACKET_MESSAGE_INDEX       = 7;
	const static size_t PACKET_HEADER_SIZE         = PACKET_MESSAGE_INDEX;

	int32_t  sequence;
	uint16_t reliableSize;
	uint8_t  flags;

	explicit PacketHeaderType(int32_t i_sequence) :
		sequence    (i_sequence),
		reliableSize(0),
		flags       (0)
	{
	}

	PacketHeaderType() :
		PacketHeaderType(-1)
	{
	}

	explicit PacketHeaderType(buf_t& io_buf) :
		sequence    (io_buf.ReadLong()),
		reliableSize(io_buf.ReadShort()),
		flags       (io_buf.ReadByte())
	{
	}

	void Unpack(buf_t& io_buf)
	{
		sequence     = io_buf.ReadLong();
		reliableSize = io_buf.ReadShort();
		flags        = io_buf.ReadByte();
	}

	void Pack(buf_t& io_buf)
	{
		io_buf.WriteLong (sequence);
		io_buf.WriteShort(reliableSize);
		io_buf.WriteByte (flags);
	}
};
