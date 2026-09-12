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
//  Packet Sequencer code
//
//-----------------------------------------------------------------------------
#pragma once

#include "i_net.h"

#include "PacketHeaderType.h"

// This is sized with a theoretical max tolerance assuming a client with about
// 300 msec of latency with a throughput of 800 KB/s.
//
const size_t DEFAULT_RELIABILITY_QUEUE_SIZE = 256;

/// This type defines the per-packet data that's relevant for managing reliability.
struct SequenceQueueEntryType
{
	buf_t               buf                 { MAX_UDP_PACKET }; ///< The actual data payload that needs reliability.
	PacketHeaderType    header;                                 ///< This packet's header.
	int                 lastRetransmitTic   { -1 };             ///< The tic number on which this packet was last retransmitted.
	bool                isAwaiting          { false };          ///< True if this packet needs yet to be acked.
};
