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

	uint32_t m_nextPacketID;
	uint16_t m_nextReliableID;
	uint16_t m_reliableNoAck;

	sentPacket_s& sentPacket(const uint32_t id);
	sentPacket_s* validSentPacket(const uint32_t id);
	reliableMessage_s& reliableMessage(const uint16_t id);
	reliableMessage_s* validReliableMessage(const uint16_t id);

  public:
	SVCMessages();
	SVCMessages(const SVCMessages& other);
	void clear();
	void queueReliable(const google::protobuf::Message& msg);
	void queueUnreliable(const google::protobuf::Message& msg);
	bool writePacket(buf_t& buf, const dtime_t time);
	bool clientAck(const uint32_t packetAck, const uint32_t packetAckBits);

	struct debug_s
	{
		uint32_t nextPacketID;
		uint16_t nextReliableID;
		uint16_t reliableNoAck;
	};
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
