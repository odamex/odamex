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

static const dtime_t RELIABLE_TIMEOUT = 100;
static const size_t RELIABLE_HEADER_SIZE = sizeof(byte) + sizeof(uint16_t);
static const size_t UNRELIABLE_HEADER_SIZE = sizeof(byte);

/**
 * @brief Return a sent packet for a given packet ID, with no error checking.
 */
SVCMessages::sentPacket_s& SVCMessages::sentPacket(const uint32_t id)
{
	return m_sentPackets[id];
}

/**
 * @brief Return a sent packet for a given packet ID, or null if there is
 *        no matching packet.
 */
SVCMessages::sentPacket_s* SVCMessages::validSentPacket(const uint32_t id)
{
	sentPacket_s* sent = &sentPacket(id);
	if (sent->packetID != id)
		return nullptr;
	return sent;
}

/**
 * @brief Return a reliable message for a given reliable ID, with no error checking.
 */
SVCMessages::reliableMessage_s& SVCMessages::reliableMessage(const uint16_t id)
{
	return m_reliableMessages[id];
}

/**
 * @brief Return a reliable message for a given message ID, or null if there
 *        is no matching message.
 */
SVCMessages::reliableMessage_s* SVCMessages::validReliableMessage(const uint16_t id)
{
	reliableMessage_s* sent = &reliableMessage(id);
	if (sent->messageID != id || sent->acked)
		return nullptr;
	return sent;
}

SVCMessages::SVCMessages()
    : m_reliableMessages(), m_unreliableMessages(), m_sentPackets(), m_nextPacketID(0),
      m_nextReliableID(0), m_reliableNoAck(0)
{
}

SVCMessages::SVCMessages(const SVCMessages& other)
    : m_reliableMessages(other.m_reliableMessages),
      m_unreliableMessages(other.m_unreliableMessages),
      m_sentPackets(other.m_sentPackets), m_nextPacketID(other.m_nextPacketID),
      m_nextReliableID(other.m_nextReliableID), m_reliableNoAck(other.m_reliableNoAck)
{
}

/**
 * @brief Clear the message container.
 */
void SVCMessages::clear()
{
	auto reliableMessages = decltype(m_reliableMessages)();
	std::swap(m_reliableMessages, reliableMessages);
	auto unreliableMessages = decltype(m_unreliableMessages)();
	std::swap(m_unreliableMessages, unreliableMessages);
	auto sentPackets = decltype(m_sentPackets)();
	std::swap(m_sentPackets, sentPackets);
	m_nextPacketID = 0;
	m_nextReliableID = 0;
	m_reliableNoAck = 0;
}

/**
 * @brief Queue a reliable message to be sent.
 */
void SVCMessages::queueReliable(const google::protobuf::Message& msg)
{
	// Queue the message.
	reliableMessage_s& queued = reliableMessage(m_nextReliableID);
	queued.acked = false;
	queued.messageID = m_nextReliableID;
	queued.lastSent = 0ULL - RELIABLE_TIMEOUT;
	queued.header = SVC_ResolveDescriptor(msg.GetDescriptor());
	msg.SerializeToString(&queued.data);

	// Advance Message ID.
	m_nextReliableID += 1;
}

/**
 * @brief Queue an unreliable message to be sent.
 */
void SVCMessages::queueUnreliable(const google::protobuf::Message& msg)
{
	unreliableMessage_s queued = m_unreliableMessages.push();
	queued.sent = false;
	queued.header = SVC_ResolveDescriptor(msg.GetDescriptor());
	msg.SerializeToString(&queued.data);
}

/**
 * @brief Given the state of messages, construct and write a single packet to the buffer.
 *
 * @param buf Buffer to write to.
 * @return True if a packet was queued, false if no viable messages made it in.
 */
bool SVCMessages::writePacket(buf_t& buf)
{
	const dtime_t TIME = I_MSTime();

	// Queue the packet for sending.
	sentPacket_s& sent = sentPacket(m_nextPacketID);
	sent.packetID = m_nextPacketID;
	sent.size = 0;
	sent.reliableIDs.clear();

	// Walk the message array starting with the oldest un-acked.
	const uint32_t LENGTH = m_nextReliableID - m_reliableNoAck;
	for (uint32_t i = 0; i < LENGTH; i++)
	{
		reliableMessage_s* queue = validReliableMessage(m_reliableNoAck + i);
		if (queue == NULL)
		{
			// Invalid message.
			continue;
		}

		if (queue->lastSent + RELIABLE_TIMEOUT >= TIME)
		{
			// Don't rapid-fire resends.
			continue;
		}

		// Size of data plus header.
		const size_t TOTAL_SIZE = RELIABLE_HEADER_SIZE + queue->data.size();
		if (sent.size + TOTAL_SIZE > MAX_UDP_SIZE)
		{
			// Too big to fit.
			continue;
		}

		sent.size += TOTAL_SIZE;
		sent.reliableIDs.push_back(queue->messageID);

		// Ensure we don't resend immediately.
		queue->lastSent = TIME;
	}

	// See if we can fit in any other unreliable messages.
	for (size_t i = 0; i < m_unreliableMessages.size(); i++)
	{
		unreliableMessage_s& msg = m_unreliableMessages[i];
		if (msg.sent)
		{
			// Already sent.
			continue;
		}

		// Size of data plus header.
		const size_t TOTAL_SIZE = UNRELIABLE_HEADER_SIZE + msg.data.size();
		if (sent.size + TOTAL_SIZE > MAX_UDP_SIZE)
		{
			// Too big to fit.
			continue;
		}

		sent.size += TOTAL_SIZE;
		sent.unreliables.push_back(&msg);
	}

	// If there is a contiguous string of unreliable messages at the start
	// of the queue, pop them so we can reduce the iteration length next time.
	while (!m_unreliableMessages.empty() && m_unreliableMessages.front().sent)
	{
		m_unreliableMessages.pop();
	}

	if (sent.size == 0)
	{
		// No messages were queued.
		return false;
	}

	// Assemble the message for sending.
	buf.clear();
	buf.WriteLong(int(m_nextPacketID)); // Packet ID
	buf.WriteByte(0);                   // Empty space for flags.

	// Write out the individual reliable messages.
	StringTokens debugRels, debugUnrels;
	for (size_t i = 0; i < sent.reliableIDs.size(); i++)
	{
		const reliableMessage_s& msg = reliableMessage(sent.reliableIDs[i]);
		const byte header = svc::ToByte(msg.header, true);
		buf.WriteByte(header);
		buf.WriteShort(uint16_t(msg.messageID)); // reliable only
		buf.WriteChunk(msg.data.data(), uint32_t(msg.data.size()));

		// DEBUG!
		debugRels.push_back(svc_info[msg.header].msgName);
	}

	// Write out the individual unreliable messages.
	for (size_t i = 0; i < sent.unreliables.size(); i++)
	{
		const unreliableMessage_s& msg = *sent.unreliables[i];
		const byte header = svc::ToByte(msg.header, false);
		buf.WriteByte(header);
		buf.WriteChunk(msg.data.data(), uint32_t(msg.data.size()));

		// DEBUG!
		debugUnrels.push_back(svc_info[msg.header].msgName);
	}
	sent.unreliables.clear(); // No need to keep wild pointers around.

	Printf("[%u] sz:%zu R:%s U:%s\n", m_nextPacketID, buf.cursize,
	       JoinStrings(debugRels, ", ").c_str(), JoinStrings(debugUnrels, ", ").c_str());

	m_nextPacketID += 1;
	return true;
}

/**
 * @brief Process package acknowledgements from the client.
 *
 * @param packetAck Most recent packet acknowledged.
 * @param packetAckBits Bitfield of packets previous to packetAck that
 *                      have also been acked.
 * @return False if ack was sent from new connection, otherwise true.
 */
bool SVCMessages::clientAck(const uint32_t packetAck, const uint32_t packetAckBits)
{
	if (packetAck == 0 && packetAckBits == 0)
		return false;

	// See if we have a packet in the system that matches.
	sentPacket_s* packet = validSentPacket(packetAck);
	if (!packet)
		return true;

	// We do - ack the packet and all messages attached to it.
	for (size_t i = 0; i < packet->reliableIDs.size(); i++)
	{
		const uint32_t msgID = packet->reliableIDs[i];
		reliableMessage_s* msg = validReliableMessage(msgID);
		msg->acked = true;
	}

	return true;
}

client_t::client_t()
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

client_t::client_t(const client_t& other)
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
