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
	bool writePacket(buf_t& buf);
	bool clientAck(const uint32_t packetAck, const uint32_t packetAckBits);
};

struct client_t
{
	netadr_t address;

	buf_t netbuf;
	buf_t reliablebuf;

	// protocol version supported by the client
	short version;
	int packedversion;

	int sequence;
	int last_sequence;
	byte packetnum;

	int rate;
	int reliable_bps; // bytes per second
	int unreliable_bps;

	int last_received; // for timeouts

	int lastcmdtic, lastclientcmdtic;

	std::string digest;     // randomly generated string that the client must use for any
	                        // hashes it sends back
	bool allow_rcon;        // allow remote admin
	bool displaydisconnect; // display disconnect message when disconnecting

	huffman_server compressor; // denis - adaptive huffman compression

	SVCMessages msg;

	client_t();
	client_t(const client_t& other);
};
