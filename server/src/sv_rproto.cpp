// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2015 by The Odamex Team.
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
#include <stdio.h>
#include <stdlib.h>
#include <boost/thread.hpp>

#include "doomtype.h"
#include "doomstat.h"
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

boost::thread_specific_ptr<buf_t> plain; // denis - todo - call_terms destroys these statics on quit
boost::thread_specific_ptr<buf_t> sendd;

//
// SV_CompressPacket
//
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
void SV_CompressPacket(buf_t &send, unsigned int reserved, client_t *cl)
{
	size_t orig_size = send.size();

	if (plain.get() == nullptr) {
		plain.reset(new buf_t(MAX_UDP_PACKET));
	}
	if(plain->maxsize() < send.maxsize())
		plain->resize(send.maxsize());
	
	plain->setcursize(send.size());
	
	memcpy(plain->ptr(), send.ptr(), send.size());

	byte method = 0;

	int need_gap = 2; // for svc_compressed and method, below
#if 0
	if(MSG_CompressAdaptive(cl->compressor.get_codec(), send, reserved, need_gap))
	{
		reserved += need_gap;
		need_gap = 0;

		method |= adaptive_mask;

		if(cl->compressor.get_codec_id())
			method |= adaptive_select_mask;
	}
#endif
	//DPrintf("SV_CompressPacket stage 2: %x %d\n", (int)method, (int)send.size());

	if(MSG_CompressMinilzo(send, reserved, need_gap))
		method |= minilzo_mask;

	if((method & adaptive_mask) || (method & minilzo_mask))
	{
#if 0
		if(cl->compressor.packet_sent(cl->sequence - 1, plain->ptr() + sizeof(int), plain->size() - sizeof(int)))
			method |= adaptive_record_mask;
#endif
		send.ptr()[sizeof(int)] = svc_compressed;
		send.ptr()[sizeof(int) + 1] = method;
	}

	//DPrintf("SV_CompressPacket %x %d (from plain %d)\n", (int)method, (int)send.size(), (int)orig_size);
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

EXTERN_CVAR(sv_waddownloadcap)

//
// SV_SendPacket
//
bool SV_SendPacket(player_t &pl)
{
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

	size_t rel = cl->reliablebuf.cursize, unr = cl->netbuf.cursize;
	int iters = 0;

	if (sendd.get() == nullptr) {
		sendd.reset(new buf_t(MAX_UDP_PACKET));
	}

	// send several packets while we have data to send:
	const int cl_rate = cl->rate * 1000;
	const int max_iters = (cl_rate / NET_PACKET_MAX);
	while (cl->reliablebuf.cursize + cl->netbuf.cursize > 0) {
		// put a cap on this so we don't get stuck building enormous chains of packets:
		if (++iters >= max_iters) {
			cl->netbuf.clear();
			break;
		}

		// determine our current bandwidth:
		int bps = (int) ((double) ((cl->unreliable_bps + cl->reliable_bps) * TICRATE) / (double) ((gametic % 35) + 1));
		if (bps > cl_rate) {
			cl->netbuf.clear();
			break;
		}

		// find the max message size we could send that keeps us at or below the rate limit:
		size_t remaining_budget = cl_rate - bps;
		size_t pkt_remaining = MIN((size_t)NET_PACKET_MAX, remaining_budget);

		// start building the packet:

		// 1. fill up packet with as much reliable-channel data as possible:
		size_t reliablesize = MIN(cl->reliablebuf.cursize, (size_t)pkt_remaining);
		// trim off to the message boundary within the byte stream:
		size_t reliabletrim = cl->reliablebuf.FloorMarker(reliablesize);

		// save the reliable message
		// it will be retransmitted, if it's missed

		// the end of the buffer is reached
		if (cl->relpackets.cursize + reliabletrim >= cl->relpackets.maxsize())
			cl->relpackets.cursize = 0;

		// copy the beginning and the size of a packet to the buffer
		cl->packetbegin[cl->packetnum] = cl->relpackets.cursize;
		cl->packetsize[cl->packetnum] = reliabletrim;
		cl->packetseq[cl->packetnum] = cl->sequence;

		if (reliabletrim)
			SZ_Write(&cl->relpackets, cl->reliablebuf.data, reliabletrim);

		// compose the packet's data:
		sendd->clear();

		cl->packetnum++; // packetnum will never be more than 255
		// because sizeof(packetnum) == 1. Don't need
		// to use &0xff. Cool, eh? ;-)

		// copy sequence
		MSG_WriteLong(sendd.get(), cl->sequence++);

		// copy the reliable message to the packet first:
		if (reliabletrim) {
			SZ_Write(sendd.get(), cl->reliablebuf.data, reliabletrim);
			// trim out the messages we wrote from the reliablebuf:
			cl->reliablebuf.TrimLeft(reliabletrim);
			// record for rate limiting:
			cl->reliable_bps += reliabletrim;
		}

		// check if any space left for unreliable messages:
		pkt_remaining -= (int)sendd->cursize;
		size_t unreliabletrim = 0;
		if (pkt_remaining > 0 && cl->netbuf.cursize > 0) {
			// find the max message size we could send that keeps us at or below the rate limit:
			size_t bps = (size_t) ((double) ((cl->unreliable_bps + cl->reliable_bps) * TICRATE) /
								   (double) ((gametic % 35) + 1));

			size_t remaining_budget = cl_rate - bps;
			size_t unreliablesize = MIN((size_t) pkt_remaining, remaining_budget);

			// find the highest message marker before the remaining byte count offset:
			unreliabletrim = cl->netbuf.FloorMarker(unreliablesize);

			// add the unreliable part:
			if (unreliabletrim) {
				SZ_Write(sendd.get(), cl->netbuf.data, unreliabletrim);
				// trim out the messages we wrote from the unreliablebuf:
				cl->netbuf.TrimLeft(unreliabletrim);
				// record for rate limiting:
				cl->unreliable_bps += unreliabletrim;
			}
		}

#if 1
		// compress the packet, but not the sequence id
		if (sendd->size() > sizeof(int))
			SV_CompressPacket(*sendd.get(), sizeof(int), cl);
#endif

		if (log_packetdebug) {
			Printf(PRINT_LOW | PRINT_RCON_MUTE, "SV_SendPacket: ply %3u, tic %07u, rel %4u/%4u, unr %4u/%4u\n",
					pl.id, gametic, reliabletrim, rel, unreliabletrim, unr);
		}

#ifdef SIMULATE_LATENCY
		SV_SendPacketDelayed(sendd, pl);
#else
		NET_SendPacket(*sendd.get(), cl->address);
#endif
	}

	return true;
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

			for (n=0; n<256; n++)
				if (cl->packetseq[n] == seq)
				{
					needfullupdate = false;
					break;
				}

			if  (needfullupdate)
			{
				// do full update
				DPrintf("need full update\n");
				cl->last_sequence = sequence;
				return;
			}

			MSG_WriteMarker(&cl->reliablebuf, svc_missedpacket);
			MSG_WriteLong(&cl->reliablebuf, seq);
			MSG_WriteShort(&cl->reliablebuf, cl->packetsize[n]);
			if (cl->packetsize[n])
				SZ_Write (&cl->reliablebuf, cl->relpackets.data, 
					cl->packetbegin[n], cl->packetsize[n]);

			if (cl->reliablebuf.overflowed)
			{
				// do full update
				DPrintf("reliablebuf overflowed, need full update\n");
				cl->last_sequence = sequence;
				return;
			}

			//if (cl->reliablebuf.cursize >= NET_PACKET_ROLLOVER)
				SV_SendPacket(player);
		}
	}

	cl->last_sequence = sequence;
}

VERSION_CONTROL (sv_rproto_cpp, "$Id$")

