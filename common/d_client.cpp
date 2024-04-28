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

SVCMessages::sentPacket_s& SVCMessages::sentPacket(const uint32_t id)
{
	return m_sentPackets[id];
}

SVCMessages::sentPacket_s* SVCMessages::validSentPacket(const uint32_t id)
{
	sentPacket_s* sent = &sentPacket(id);
	if (sent->packetID != id)
		return nullptr;
	return sent;
}

SVCMessages::reliableMessage_s& SVCMessages::reliableMessage(const uint16_t id)
{
	return m_reliableMessages[id];
}

SVCMessages::reliableMessage_s* SVCMessages::validReliableMessage(const uint16_t id)
{
	reliableMessage_s* sent = &reliableMessage(id);
	if (sent->messageID != id)
		return nullptr;
	return sent;
}

void SVCMessages::clear()
{
	m_reliableMessages.clear();
	m_unreliableMessages.clear();
	m_sentPackets.clear();
	m_nextPacketID = 0;
	m_nextReliableID = 0;
	m_reliableNoAck = 0;
}

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

void SVCMessages::queueUnreliable(const google::protobuf::Message& msg)
{
	unreliableMessage_s& queued = m_unreliableMessages.push();

	queued.sent = false;
	queued.header = SVC_ResolveDescriptor(msg.GetDescriptor());
	msg.SerializeToString(&queued.data);
}

bool SVCMessages::writePacket(buf_t& buf, const dtime_t time, const bool reliable)
{
	// Queue the packet for sending.
	sentPacket_s& sent = sentPacket(m_nextPacketID);
	sent.clear();

	if (reliable)
	{
		// Walk the message array starting with the oldest un-acked.
		const uint16_t LENGTH = m_nextReliableID - m_reliableNoAck;
		for (uint16_t i = 0; i < LENGTH; i++)
		{
			reliableMessage_s* queue = validReliableMessage(m_reliableNoAck + i);
			if (queue == nullptr || queue->acked)
			{
				// Message either does not exist or was acked already.
				continue;
			}

			if (queue->lastSent + RELIABLE_TIMEOUT >= time)
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
			queue->lastSent = time;
		}
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

bool SVCMessages::clientAck(const uint32_t packetAck, const uint32_t packetAckBits)
{
	if (packetAck == 0 && packetAckBits == 0)
		return false;

	// Acknowledge a packet and all reliable messages attached to it.
	const auto clientAckPacket = [&](const uint32_t checkAck) {
		// See if we have a packet in the system that matches.
		sentPacket_s* packet = validSentPacket(checkAck);
		if (!packet || packet->acked)
			return;

		// Ack the packet!
		packet->acked = true;

		// Ack all messages attached to the packet.
		for (auto relID : packet->reliableIDs)
		{
			reliableMessage_s* msg = validReliableMessage(relID);
			if (msg == nullptr || msg->acked)
				continue;

			msg->acked = true;
		}
	};

	// Ack the most recent packet.
	clientAckPacket(packetAck);

	// Loop through the last 32 messages and ack those if the bit is set.
	for (size_t i = 0; i < 32; i++)
	{
		uint32_t bitPacketID = packetAck - i - 1;
		if ((packetAckBits & BIT(i)) != 0)
		{
			clientAckPacket(bitPacketID);
		}
		bitPacketID -= 1;
	}

	// What is the most recently un-acked reliable message?
	for (uint16_t i = m_reliableNoAck;; i++)
	{
		reliableMessage_s* msg = validReliableMessage(i);
		if (msg == nullptr || !msg->acked)
		{
			m_reliableNoAck = i;
			break;
		}
	}

	return true;
}

SVCMessages::debug_s SVCMessages::debug()
{
	debug_s rvo;
	rvo.nextPacketID = m_nextPacketID;
	rvo.nextReliableID = m_nextReliableID;
	rvo.reliableNoAck = m_reliableNoAck;
	return rvo;
}

#ifdef SERVER_APP

#include "c_dispatch.h"
#include "d_player.h"

BEGIN_COMMAND(debugnet)
{
	if (::players.empty())
	{
		PrintFmt("No players are connected.\n");
		return;
	}

	for (auto& player : ::players)
	{
		SVCMessages::debug_s deb = player.client->msg.debug();

		PrintFmt("                  === {: 3}:{} ===\n", int(player.id), player.userinfo.netname);
		PrintFmt("           next packet ID: {}\n", deb.nextPacketID);
		PrintFmt("         next reliable ID: {}\n", deb.nextReliableID);
		PrintFmt("first unACKed reliable ID: {}\n", deb.reliableNoAck);
	}
}
END_COMMAND(debugnet)

#endif
