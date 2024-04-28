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

#pragma once

#include "huffman.h"
#include "i_net.h"
#include "ocircularbuffer.h"

class SVCMessages
{
	struct reliableMessage_s
	{
		uint16_t messageID = 0;
		bool acked = true;
		dtime_t lastSent = 0;
		svc_t header = svc_invalid;
		std::string data;
	};

	struct unreliableMessage_s
	{
		bool sent = true;
		svc_t header = svc_invalid;
		std::string data;
	};

	struct sentPacket_s
	{
		uint32_t packetID = 0;
		bool acked = true;
		size_t size = 0;
		std::vector<uint16_t> reliableIDs;
		std::vector<unreliableMessage_s*> unreliables;
		void clear()
		{
			packetID = 0;
			acked = true;
			size = 0;
			reliableIDs.clear();
			unreliables.clear();
		}
	};

	OCircularBuffer<reliableMessage_s, BIT(10)> m_reliableMessages;
	OCircularQueue<unreliableMessage_s, BIT(10)> m_unreliableMessages;
	OCircularBuffer<sentPacket_s, BIT(10)> m_sentPackets;

	/**
	 * @brief ID to use for next packet.
	 */
	uint32_t m_nextPacketID = 0;

	/**
	 * @brief ID to use for next reliable message.
	 */
	uint16_t m_nextReliableID = 0;

	/**
	 * @brief The most recent reliable message that the client has not acked.
	 *        It is assumed that every reliable message packet before this
	 *        one was acked by the client, in order.
	 */
	uint16_t m_reliableNoAck = 0;

	/**
	 * @brief Return a sent packet for a given packet ID, with no error checking.
	 */
	sentPacket_s& sentPacket(const uint32_t id);

	/**
	 * @brief Return a sent packet for a given packet ID, or null if there is
	 *        no matching packet.
	 */
	sentPacket_s* validSentPacket(const uint32_t id);

	/**
	 * @brief Return a reliable message for a given reliable ID, with no error checking.
	 */
	reliableMessage_s& reliableMessage(const uint16_t id);

	/**
	 * @brief Return a reliable message for a given message ID, or null if there
	 *        is no matching message.
	 */
	reliableMessage_s* validReliableMessage(const uint16_t id);

  public:
	struct debug_s
	{
		uint32_t nextPacketID;
		uint16_t nextReliableID;
		uint16_t reliableNoAck;
	};

	/**
	 * @brief Clear the message container.
	 */
	void clear();

	/**
	 * @brief Queue a reliable message to be sent.
	 */
	void queueReliable(const google::protobuf::Message& msg);

	/**
	 * @brief Queue an unreliable message to be sent.
	 */
	void queueUnreliable(const google::protobuf::Message& msg);

	/**
	 * @brief Given the state of messages, construct and write a single packet to the
	 *        buffer.
	 *
	 * @param buf Buffer to write to.
	 * @param time Time to consider when preventing resends.
	 * @param reliable True if we should send reliable packets.
	 * @return True if a packet was queued, false if no viable messages made it in.
	 */
	bool writePacket(buf_t& buf, dtime_t time, bool reliable);

	/**
	 * @brief Process package acknowledgements from the client.
	 *
	 * @param packetAck Most recent packet acknowledged.
	 * @param packetAckBits Bitfield of packets previous to packetAck that
	 *                      have also been acked.
	 * @return False if ack was sent from new connection, otherwise true.
	 */
	bool clientAck(const uint32_t packetAck, const uint32_t packetAckBits);

	/**
	 * @brief Return internal message data.
	 */
	debug_s debug();
};

struct client_t
{
	netadr_t address;

	buf_t netbuf = MAX_UDP_PACKET;
	buf_t reliablebuf = MAX_UDP_PACKET;

	// protocol version supported by the client
	short version = 0;
	int packedversion = 0;

	int sequence = 0;
	int last_sequence = 0;
	byte packetnum = 0;

	int rate = 0;
	int reliable_bps = 0; // bytes per second
	int unreliable_bps = 0;

	int last_received = 0; // for timeouts

	int lastcmdtic = 0;
	int lastclientcmdtic = 0;

	std::string digest;     // randomly generated string that the client must use for any
	                        // hashes it sends back
	bool allow_rcon = false;        // allow remote admin
	bool displaydisconnect = true; // display disconnect message when disconnecting

	huffman_server compressor; // denis - adaptive huffman compression

	SVCMessages msg;

	client_t() = default;
	client_t(client_t&& other) = default;
	PREVENT_COPY(client_t);
};
