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
//  Builders for sender-agnostic messages
//
//-----------------------------------------------------------------------------

#include "msg_message.h"

#include "PacketHeaderType.h"

odaproto::Header MSG_Header(const PacketHeaderType& i_header)
{
	odaproto::Header msg;

	msg.set_sequence        (i_header.sequence);
	msg.set_originator_tic  (i_header.originatorTic);
	msg.set_destination_tic (i_header.destinationTic);
	msg.set_reliable_size   (i_header.reliableSize);
	msg.set_flags           (i_header.flags);

	return msg;
}
