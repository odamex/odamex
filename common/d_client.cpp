// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Player client functionality.
//
//-----------------------------------------------------------------------------

#include "odamex.h"

#include "d_client.h"

client_s::client_s()
    : version(0), packedversion(0), sequence(0), last_sequence(0), packetnum(0), rate(0),
      reliable_bps(0), unreliable_bps(0), last_received(0), lastcmdtic(0),
      lastclientcmdtic(0), netbuf(MAX_UDP_PACKET), reliablebuf(MAX_UDP_PACKET),
      allow_rcon(false), displaydisconnect(true)
{
	ArrayInit(address.ip, 0);
	address.port = 0;
	address.pad = 0;

	for (size_t i = 0; i < ARRAY_LENGTH(oldpackets); i++)
	{
		oldpackets[i].sequence = -1;
		oldpackets[i].data.resize(MAX_UDP_PACKET);
	}
}

client_s::client_s(const client_s& other)
    : address(other.address), netbuf(other.netbuf), reliablebuf(other.reliablebuf),
      version(other.version), packedversion(other.packedversion),
      sequence(other.sequence), last_sequence(other.last_sequence),
      packetnum(other.packetnum), rate(other.rate), reliable_bps(other.reliable_bps),
      unreliable_bps(other.unreliable_bps), last_received(other.last_received),
      lastcmdtic(other.lastcmdtic), lastclientcmdtic(other.lastclientcmdtic),
      digest(other.digest), allow_rcon(false), displaydisconnect(true),
      compressor(other.compressor)
{
	for (size_t i = 0; i < ARRAY_LENGTH(oldpackets); i++)
	{
		oldpackets[i] = other.oldpackets[i];
	}
}

/**
 * @brief Get the old message for the given sequence, or NULL if that sequence
 *        does not exist in the buffer.
 */
oldMessage_s* client_s::getOldMessage(const uint16_t seq)
{
	const uint16_t idx = seq % ARRAY_LENGTH(m_oldMessages);
	if (m_oldMessages[idx].sequence != seq)
		return NULL;

	return &m_oldMessages[idx];
}

/**
 * @brief Insert a message into the old messages buffer.
 */
void client_s::setOldMessage(const uint16_t seq, const std::string& msg)
{
	const uint16_t idx = seq % ARRAY_LENGTH(m_oldMessages);
	m_oldMessages[idx].sequence = seq;
	m_oldMessages[idx].message = msg;
}
