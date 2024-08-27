// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
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
//	SV_RPROTO
//
//-----------------------------------------------------------------------------


#include "odamex.h"

#include "p_local.h"
#include "sv_main.h"
#include "huffman.h"
#include "i_net.h"

#ifdef SIMULATE_LATENCY
#include <thread>
#include <chrono>
#endif

QWORD I_MSTime (void);

EXTERN_CVAR (log_packetdebug)
#ifdef SIMULATE_LATENCY
EXTERN_CVAR (sv_latency)
#endif

buf_t plain(MAX_UDP_PACKET); // denis - todo - call_terms destroys these statics on quit
buf_t sendd(MAX_UDP_PACKET);

const static size_t PACKET_FLAG_INDEX = sizeof(uint32_t);
const static size_t PACKET_MESSAGE_INDEX = PACKET_FLAG_INDEX + 1;
const static size_t PACKET_HEADER_SIZE = PACKET_MESSAGE_INDEX;
const static size_t PACKET_OLD_MASK = 0xFF;

//
// CompressPacket
//
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
//
// [AM] Cleaned the old huffman calls for code clarity sake.
//
static void CompressPacket(buf_t& send, const size_t reserved, client_t* cl)
{
	if (plain.maxsize() < send.maxsize())
	{
		plain.resize(send.maxsize());
	}

	plain.setcursize(send.size());
	memcpy(plain.ptr(), send.ptr(), send.size());

	byte method = 0;
	if (MSG_CompressMinilzo(send, reserved, 0))
	{
		// Successful compression, set the compression flag bit.
		method |= SVF_COMPRESSED;
	}

	send.ptr()[PACKET_FLAG_INDEX] |= method;
	DPrintf("CompressPacket %x " "zu" "\n", method, send.size());
}

#ifdef SIMULATE_LATENCY
struct DelaySend
{
public:
	DelaySend(buf_t& data, player_t* pl)
	{
		m_data = data;
		m_pl = pl;
		m_tp = std::chrono::steady_clock::now() + std::chrono::milliseconds(sv_latency);
	}
	std::chrono::time_point<std::chrono::steady_clock> m_tp;
	buf_t m_data;
	player_t* m_pl;
};

std::queue<DelaySend> m_delayQueue;
bool m_delayThreadCreated = false;
void SV_DelayLoop()
{
	for (;;)
	{
		while (m_delayQueue.size())
		{
			int sendgametic = gametic;
			auto item = &m_delayQueue.front();

			while (std::chrono::steady_clock::now() < item->m_tp)
				std::this_thread::sleep_for(std::chrono::milliseconds(1));

			NET_SendPacket(item->m_data, item->m_pl->client.address);
			m_delayQueue.pop();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void SV_SendPacketDelayed(buf_t& packet, player_t& pl)
{
	if (!m_delayThreadCreated)
	{
		std::thread tr(SV_DelayLoop);
		tr.detach();
		m_delayThreadCreated = true;
	}
	m_delayQueue.push(DelaySend(packet, &pl));
}
#endif

//
// SV_SendPacket
//
bool SV_SendPacket(player_t &pl)
{
	int				bps = 0; // bytes per second, not bits per second

	client_t *cl = &pl.client;

	if (cl->reliablebuf.overflowed)
	{ 
		SZ_Clear(&cl->netbuf);
		SZ_Clear(&cl->reliablebuf);
	    SV_DropClient(pl);
		return false;
	}
	else
		if (cl->netbuf.overflowed)
			SZ_Clear(&cl->netbuf);

	// [SL] 2012-05-04 - Don't send empty packets - they still have overhead
	if (cl->reliablebuf.cursize + cl->netbuf.cursize == 0)
		return true;

	sendd.clear();

	// save the reliable message 
	// it will be retransmited, if it's missed
	client_t::oldPacket_t& old = cl->oldpackets[cl->sequence & PACKET_OLD_MASK];

	old.data.clear();
	if (cl->reliablebuf.cursize)
	{
		// copy the reliable data into the buffer.
		old.sequence = cl->sequence;
		SZ_Write(&old.data, cl->reliablebuf.data, cl->reliablebuf.cursize);
	}
	else
	{
		old.sequence = -1;
	}

	cl->packetnum++; // packetnum will never be more than 255
	                 // because sizeof(packetnum) == 1. Don't need
	                 // to use &0xff. Cool, eh? ;-)

	// copy sequence
	MSG_WriteLong(&sendd, cl->sequence++);
	MSG_WriteByte(&sendd, 0); // Flags, filled out later.

	// copy the reliable message to the packet first
    if (cl->reliablebuf.cursize)
    {
		SZ_Write (&sendd, cl->reliablebuf.data, cl->reliablebuf.cursize);
		cl->reliable_bps += cl->reliablebuf.cursize;
    }

	// add the unreliable part if space is available and rate value
	// allows it
	if (gametic % 35)
	    bps = (int)((double)( (cl->unreliable_bps + cl->reliable_bps) * TICRATE)/(double)(gametic%35));

    if (bps < cl->rate*1000)

	  if (cl->netbuf.cursize && (sendd.maxsize() - sendd.cursize > cl->netbuf.cursize) )
	  {
         SZ_Write (&sendd, cl->netbuf.data, cl->netbuf.cursize);
	     cl->unreliable_bps += cl->netbuf.cursize;
	  }
    
	SZ_Clear(&cl->netbuf);
	SZ_Clear(&cl->reliablebuf);
	
	// compress the packet, but not the sequence id
	if (sendd.size() > PACKET_HEADER_SIZE)
	{
		CompressPacket(sendd, PACKET_HEADER_SIZE, cl);
	}

	if (log_packetdebug)
	{
		Printf(PRINT_HIGH, "ply %03u, pkt %06u, size %04u, tic %07u, time %011u\n",
			   pl.id, cl->sequence - 1, sendd.cursize, gametic, I_MSTime());
	}

#ifdef SIMULATE_LATENCY
	SV_SendPacketDelayed(sendd, pl);
#else

	NET_SendPacket(sendd, cl->address);
#endif
	return true;
}

/**
 * @brief Send an old reliable packet with old data on the wire.
 * 
 * @param pl Player to send to.
 * @param sequence Sequence number to send.  This assumss
*/
static void SendOldPacket(player_t& pl, const int sequence)
{
	// Send buffer.
	static buf_t send(MAX_UDP_PACKET);
	send.clear();

	client_t& cl = pl.client;
	client_t::oldPacket_t& old = cl.oldpackets[sequence & PACKET_OLD_MASK];

	// This is a lot simpler than a fresh send.  Just send the data we have
	// have saved out.

	MSG_WriteLong(&send, old.sequence);
	MSG_WriteByte(&send, 0); // Flags, filled out later.

	// copy the reliable message to the packet
	if (old.data.cursize)
	{
		SZ_Write(&send, old.data.data, old.data.cursize);
		cl.reliable_bps += old.data.cursize;
	}

	// compress the packet, but not the sequence id
	if (send.size() > PACKET_HEADER_SIZE)
	{
		CompressPacket(send, PACKET_HEADER_SIZE, &cl);
	}

	NET_SendPacket(send, cl.address);
}

//
// SV_AcknowledgePacket
//
void SV_AcknowledgePacket(player_t &player)
{
	client_t *cl = &player.client;

	int sequence = MSG_ReadLong();

	cl->compressor.packet_acked(sequence);

	// packet is missed
	if (sequence - cl->last_sequence > 1)
	{
		// resend
		for (int seq = cl->last_sequence+1; seq < sequence; seq++)
		{
			int  n;
			bool needfullupdate = true;

			if (cl->oldpackets[seq & PACKET_OLD_MASK].sequence != seq)
			{
				// do full update
				DPrintf("need full update\n");
				cl->last_sequence = sequence;
				return;
			}

			SendOldPacket(player, seq);
		}
	}

	cl->last_sequence = sequence;

	if (cl->last_sequence == 0)
	{
		// [AM] Finish our connection sequence.
		SV_ConnectClient2(player);
	}
}

VERSION_CONTROL (sv_rproto_cpp, "$Id$")
