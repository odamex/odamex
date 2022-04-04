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

#include <google/protobuf/message.h>

#include "i_system.h"
#include "svc_map.h"

client_s::client_s()
    : version(0), packedversion(0), sequence(0), last_sequence(0), packetnum(0), rate(0),
      reliable_bps(0), unreliable_bps(0), last_received(0), lastcmdtic(0),
      lastclientcmdtic(0), netbuf(MAX_UDP_PACKET), reliablebuf(MAX_UDP_PACKET),
      allow_rcon(false), displaydisconnect(true), m_nextPacketID(0), m_nextMessageID(0),
      m_oldestMessageNoACK(0)
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
      compressor(other.compressor), m_sentPackets(other.m_sentPackets),
      m_queuedMessages(other.m_queuedMessages), m_nextPacketID(0), m_nextMessageID(0),
      m_oldestMessageNoACK(0)
{
	for (size_t i = 0; i < ARRAY_LENGTH(oldpackets); i++)
	{
		oldpackets[i] = other.oldpackets[i];
	}
}

void client_s::queueReliable(const google::protobuf::Message& msg)
{
	baseQueueMessage(msg, true);
}

void client_s::queueUnreliable(const google::protobuf::Message& msg)
{
	baseQueueMessage(msg, false);
}

/**
 * @brief Given the state of messages, construct and write a single packet to the buffer.
 *
 * @param buf Buffer to write to.
 * @return True if a packet was queued, false if no viable messages made it in.
 */
bool client_s::writePacket(buf_t& buf)
{
	const dtime_t TIME = I_MSTime();

	// Queue the packet for sending.
	sentPacket_s& sent = sentPacket(m_nextPacketID);
	sent.packetID = m_nextPacketID;
	sent.size = 0;
	sent.messages.clear();

	// Walk the message array starting with the oldest un-acked.
	const uint32_t LENGTH = m_nextMessageID - m_oldestMessageNoACK;
	for (uint32_t i = 0; i < LENGTH; i++)
	{
		client_s::queuedMessage_s* queue = validQueuedMessage(m_oldestMessageNoACK + i);
		if (queue == NULL)
			continue; // Invalid message.

		if (queue->lastSent + 100 >= TIME)
			continue; // Don't rapid-fire resends.

		// Size of data plus header.
		const size_t TOTAL_SIZE = 1 + queue->data.size();

		if (sent.size + TOTAL_SIZE > MAX_UDP_SIZE)
			continue; // Too big to fit.

		sent.size += TOTAL_SIZE;
		sent.messages.push_back(queue->messageID);
	}

	if (sent.size == 0)
		return false; // No messages were queued.

	// Assemble the message for sending.
	buf.clear();
	buf.WriteLong(int(m_nextPacketID)); // Packet ID
	buf.WriteByte(0);                   // Empty space for flags.

	// Write out the individual messages.
	for (size_t i = 0; i < sent.messages.size(); i++)
	{
		client_s::queuedMessage_s& msg = queuedMessage(sent.messages[i]);
		buf.WriteByte(msg.header);
		buf.WriteChunk(msg.data.data(), msg.data.size());
	}

	m_nextPacketID += 1;
	return true;
}

client_s::sentPacket_s& client_s::sentPacket(const uint32_t id)
{
	return m_sentPackets[id];
}

client_s::sentPacket_s* client_s::validSentPacket(const uint32_t id)
{
	sentPacket_s* sent = &sentPacket(id);
	if (sent->packetID != id)
		return NULL;
	return sent;
}

client_s::queuedMessage_s& client_s::queuedMessage(const uint32_t id)
{
	return m_queuedMessages[id];
}

client_s::queuedMessage_s* client_s::validQueuedMessage(const uint32_t id)
{
	queuedMessage_s* sent = &queuedMessage(id);
	if (sent->messageID != id)
		return NULL;
	return sent;
}

void client_s::baseQueueMessage(const google::protobuf::Message& msg, const bool reliable)
{
	// Queue the message.
	queuedMessage_s& queued = queuedMessage(m_nextMessageID);
	queued.messageID = m_nextMessageID;
	queued.reliable = reliable;
	queued.lastSent = I_MSTime();
	queued.header = SVC_ResolveDescriptor(msg.GetDescriptor());
	msg.SerializeToString(&queued.data);

	// Advance Message ID.
	m_nextMessageID += 1;
}
