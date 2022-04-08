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

struct oldPacket_s
{
	int sequence;
	buf_t data;

	oldPacket_s() : sequence(-1) { data.resize(0); }
};

struct client_s
{
	netadr_t address;

	buf_t netbuf;
	buf_t reliablebuf;

	// protocol version supported by the client
	short version;
	int packedversion;

	// for reliable protocol
	oldPacket_s oldpackets[256];

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

	client_s();
	client_s(const client_s& other);

	void queueReliable(const google::protobuf::Message& msg);
	void queueUnreliable(const google::protobuf::Message& msg);
	bool writePacket(buf_t& buf);
	bool clientAck(const uint32_t packetAck, const uint32_t packetAckBits);

  private:
	struct reliableMessage_s
	{
		uint16_t messageID;
		bool acked;
		dtime_t lastSent;
		svc_t header;
		std::string data;
		reliableMessage_s()
		    : messageID(0), acked(true), lastSent(0), header(svc_noop), data()
		{
		}
	};
	OCircularBuffer<reliableMessage_s, BIT(10)> m_reliableMessages;

	struct unreliableMessage_s
	{
		bool sent;
		svc_t header;
		std::string data;
		unreliableMessage_s() : sent(false), header(svc_noop), data() { }
	};
	OCircularQueue<unreliableMessage_s, BIT(10)> m_unreliableMessages;

	struct sentPacket_s
	{
		uint32_t packetID;
		size_t size;
		std::vector<uint16_t> reliableIDs;
		std::vector<unreliableMessage_s*> unreliables;
		sentPacket_s() : packetID(0), size(0), reliableIDs() { }
	};
	OCircularBuffer<sentPacket_s, BIT(10)> m_sentPackets;

	uint32_t m_nextPacketID;
	uint16_t m_nextReliableID;
	uint32_t m_reliableNoAck;

	sentPacket_s& sentPacket(const uint32_t id);
	sentPacket_s* validSentPacket(const uint32_t id);
	reliableMessage_s& reliableMessage(const uint16_t id);
	reliableMessage_s* validReliableMessage(const uint16_t id);
};

typedef client_s client_t;
