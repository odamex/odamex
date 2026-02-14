// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2026 by The Odamex Team.
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
#include "i_net.h"

#ifdef SIMULATE_LATENCY
#include <thread>
#include <chrono>
#endif

#ifdef SIMULATE_LATENCY
EXTERN_CVAR (sv_latency)
#endif

//
// CompressPacket
//
// [Russell] - reason this was failing is because of huffman routines, so just
// use minilzo for now (cuts a packet size down by roughly 45%), huffman is the
// if 0'd sections
//
// [AM] Cleaned the old huffman calls for code clarity sake.
//

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

bool SV_MustThrottleTransmissionsForClient(client_t& client)
{
    return client.messenger.MustThrottleTransmission();
}

void SV_HandleReliableRetransmissions()
{
	for (auto& player : players)
	{
		// Players that are on their way out don't get any retries.
		if (player.playerstate == PST_DISCONNECT)
		{
			continue;
		}

		// Total hack:  We check for the player being in the first second of their connection because there's something
		// in the connection protocol that requires us to do immediate retransmits of the first few reliable messages.
		if (player.GameTime > 0)
		{
			// The following results in fractional tics rounding up.
			const int pingInTics = (player.ping * TICRATE + 999) / 1000;

			// Adjust upwards because in the real world, tic boundaries don't align and can drift.
			const int retransmitDelayInTics = pingInTics + 1;

			player.client.messenger.SetRetransmitDelay(retransmitDelayInTics);
		}
		else
		{
			player.client.messenger.SetRetransmitDelay(0);
		}

		player.client.messenger.HandleRetransmissions(gametic, player.client.address);
	}
}

//
// SV_AcknowledgePacket
//
void SV_AcknowledgePacket(player_t &player)
{
	int sequence = MSG_ReadLong();

	const bool isFresh = player.client.messenger.Acknowledge(sequence);

	//DPrintFmt("player {} tic {} ACKed seq {}\n", int(player.id), gametic, sequence);

	if (isFresh and sequence == 0)
	{
		// [AM] Finish our connection sequence.
		SV_ConnectClient2(player);
	}
}

VERSION_CONTROL (sv_rproto_cpp, "$Id$")
