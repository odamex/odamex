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
#ifdef USE_BOOST
#include <boost/thread.hpp>
#endif

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

#ifdef USE_BOOST
boost::thread_specific_ptr<buf_t> plain; // denis - todo - call_terms destroys these statics on quit
boost::thread_specific_ptr<buf_t> sendd;
#else
buf_t plain(MAX_UDP_PACKET);
buf_t sendd(MAX_UDP_PACKET);
#endif

//
// SV_CompressPacket
//
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
void SV_CompressPacket(buf_t &send, unsigned int reserved, client_t *cl)
{
	size_t orig_size = send.size();

#ifdef USE_BOOST
	if (plain.get() == nullptr) {
		plain.reset(new buf_t(MAX_UDP_PACKET));
	}
#else
	buf_t *plain = &::plain;
#endif

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

#ifdef USE_BOOST
	if (sendd.get() == nullptr) {
		sendd.reset(new buf_t(MAX_UDP_PACKET));
	}
#define BUFREF(x) (x.get())
#else
	buf_t *sendd = &::sendd;
#define BUFREF(x) (x)
#endif

	sendd->clear();

	// save the reliable message
	// it will be retransmited, if it's missed

	// the end of the buffer is reached
	if (cl->relpackets.cursize + cl->reliablebuf.cursize >= cl->relpackets.maxsize())
		cl->relpackets.cursize = 0;

	// copy the beginning and the size of a packet to the buffer
	cl->packetbegin[cl->packetnum] = cl->relpackets.cursize;
	cl->packetsize[cl->packetnum] = cl->reliablebuf.cursize;
	cl->packetseq[cl->packetnum] = cl->sequence;

	if (cl->reliablebuf.cursize)
		SZ_Write (&cl->relpackets, cl->reliablebuf.data, cl->reliablebuf.cursize);


	cl->packetnum++; // packetnum will never be more than 255
	                 // because sizeof(packetnum) == 1. Don't need
	                 // to use &0xff. Cool, eh? ;-)
	// copy sequence
	MSG_WriteLong(BUFREF(sendd), cl->sequence++);

	// copy the reliable message to the packet first
    if (cl->reliablebuf.cursize)
    {
		SZ_Write (BUFREF(sendd), cl->reliablebuf.data, cl->reliablebuf.cursize);
		cl->reliable_bps += cl->reliablebuf.cursize;
    }

	// add the unreliable part if space is available and rate value
	// allows it
	if (gametic % 35)
	    bps = (int)((double)( (cl->unreliable_bps + cl->reliable_bps) * TICRATE)/(double)(gametic%35));

    if (bps < cl->rate*1000)

	  if (cl->netbuf.cursize && (sendd->maxsize() - sendd->cursize > cl->netbuf.cursize) )
	  {
         SZ_Write (BUFREF(sendd), cl->netbuf.data, cl->netbuf.cursize);
	     cl->unreliable_bps += cl->netbuf.cursize;
	  }

	SZ_Clear(&cl->netbuf);
	SZ_Clear(&cl->reliablebuf);

	// compress the packet, but not the sequence id
	if (sendd->size() > sizeof(int))
		SV_CompressPacket(*BUFREF(sendd), sizeof(int), cl);

	if (log_packetdebug)
	{
		Printf(PRINT_HIGH, "ply %03u, pkt %06u, size %04u, tic %07u, time %011u\n",
			   pl.id, cl->sequence - 1, sendd->cursize, gametic, I_MSTime());
	}

#ifdef SIMULATE_LATENCY
	SV_SendPacketDelayed(BUFREF(sendd), pl);
#else

	NET_SendPacket(*BUFREF(sendd), cl->address);
#endif
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

			if (needfullupdate)
			{
				// do full update
				DPrintf("player %3d needs full update\n", player.id);
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
				DPrintf("player %3d reliablebuf overflowed, need full update\n", player.id);
				cl->last_sequence = sequence;
				return;
			}

			if (cl->reliablebuf.cursize > 600)
				SV_SendPacket(player);

		}
	}

	cl->last_sequence = sequence;
}

VERSION_CONTROL (sv_rproto_cpp, "$Id$")

