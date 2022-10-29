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

FILE* fh;

void NETLOG(const std::string& str)
{
	return; // [AM] DISABLED

	if (!fh)
		fh = fopen("netlog.log", "w+");

	fwrite(str.data(), sizeof(char), str.length(), fh);
}

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
	if (sent->messageID != id)
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
	m_reliableMessages.clear();
	m_unreliableMessages.clear();
	m_sentPackets.clear();
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
	unreliableMessage_s& queued = m_unreliableMessages.push();

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

	NETLOG(fmt::format("[{}] time:{} nPack:{} nRel:{} rNoAck:{}\n", ::gametic, TIME,
	                   m_nextPacketID, m_nextReliableID, m_reliableNoAck));

	// Queue the packet for sending.
	sentPacket_s& sent = sentPacket(m_nextPacketID);
	sent.clear();

	// Walk the message array starting with the oldest un-acked.
	const uint32_t LENGTH = m_nextReliableID - m_reliableNoAck;
	for (uint32_t i = 0; i < LENGTH; i++)
	{
		reliableMessage_s* queue = validReliableMessage(m_reliableNoAck + i);
		if (queue == nullptr || queue->acked)
		{
			// Message either does not exist or was acked already.
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

		// Ensure we don't send again.
		msg.sent = true;
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
	for (const auto id : sent.reliableIDs)
	{
		const reliableMessage_s& msg = reliableMessage(id);
		assert(msg.header != svc_invalid);

		const byte header = svc::ToByte(msg.header, true);
		buf.WriteByte(header);
		buf.WriteShort(uint16_t(msg.messageID)); // reliable only
		buf.WriteUnVarint(uint32_t(msg.data.size()));
		buf.WriteChunk(msg.data.data(), uint32_t(msg.data.size()));
	}

	// Write out the individual unreliable messages.
	for (size_t i = 0; i < sent.unreliables.size(); i++)
	{
		const unreliableMessage_s& msg = *sent.unreliables[i];
		assert(msg.header != svc_invalid);

		const byte header = svc::ToByte(msg.header, false);
		buf.WriteByte(header);
		buf.WriteUnVarint(uint32_t(msg.data.size()));
		buf.WriteChunk(msg.data.data(), uint32_t(msg.data.size()));
	}

	sent.packetID = m_nextPacketID;
	sent.acked = false;
	sent.unreliables.clear(); // No need to keep wild pointers around.

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

	// Loop through past 33 packets.
	for (uint32_t i = 0; i < 33; i++)
	{
		const uint32_t checkAck = packetAck - i;

		// See if we have a packet in the system that matches.
		sentPacket_s* packet = validSentPacket(checkAck);
		if (!packet || packet->acked)
			continue;

		// Ack the packet!
		packet->acked = true;

		// First packet should always be checked, subsequent only if they
		// haven't been acked.
		if (i == 0 || BIT(i - 1) != 0)
		{
			// Ack all messages attached to the packet.
			for (auto relID : packet->reliableIDs)
			{
				reliableMessage_s* msg = validReliableMessage(relID);
				if (msg == nullptr || msg->acked)
					continue;

				NETLOG(fmt::format(" > Acked {}\n", relID));
				msg->acked = true;
			}
		}
	}

	// What is the most recently un-acked reliable message?
	for (uint16_t i = m_reliableNoAck;; i++)
	{
		NETLOG(fmt::format(" > Checking {}\n", i));

		reliableMessage_s* msg = validReliableMessage(i);
		if (msg == nullptr || !msg->acked)
		{
			if (msg == nullptr)
				NETLOG(fmt::format(" > {} is nullptr\n", i));
			else if (!msg->acked)
				NETLOG(fmt::format(" > {} is not acked\n", i));

			NETLOG(fmt::format(" > Advanced to {}\n", i));
			m_reliableNoAck = i;
			break;
		}
		else
		{
			NETLOG(fmt::format(" > {} is acked, advancing...\n", i));
		}
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
}
