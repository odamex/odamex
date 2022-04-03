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

  private:
	struct sentPacket_s
	{
		uint32_t packetID;
		size_t size;
		std::vector<uint32_t> messages;
	} m_sentPackets[1024];
	uint16_t m_nextPacketID = 0;

	struct queuedMessage_s
	{
		uint32_t messageID;
		bool reliable;
		dtime_t lastSent;
		svc_t header;
		std::string data;
	} m_queuedMessages[1024];
	uint32_t m_nextMessageID = 0;
	uint32_t m_oldestMessageNoACK = 0;

	sentPacket_s& sentPacket(const uint16_t id);
	sentPacket_s* validSentPacket(const uint16_t id);
	queuedMessage_s& queuedMessage(const uint32_t id);
	queuedMessage_s* validQueuedMessage(const uint32_t id);
	void baseQueueMessage(const google::protobuf::Message& msg, const bool reliable);
};

typedef client_s client_t;
