//
// OdaTV - Allow multiple clients to watch the same server
// without creating extra traffic on that server
//
// This proxy will create the first connection transparently,
// but other connections will be hidden from the server
//

#include <string.h>
#include <vector>
#include <iostream>

#include "../../master/i_net.h"

std::vector<netadr_t> spectators;
netadr_t net_server;

buf_t challenge_message(MAX_UDP_PACKET), first_message(MAX_UDP_PACKET);

void OnInitTV()
{
	NET_StringToAdr("voxelsoft.com:10666", &net_server);	
}

void OnClientTV(int i)
{
	if(i == 0)
	{
		// only forward the first client's packet
		NET_SendPacket(net_message.cursize, net_message.data, net_server);
	}
	else	
	{
		// TODO should keep client's tick info
		// TODO and also catch client disconnect messages
	}
}

void OnNewClientTV()
{
	spectators.push_back(net_from);

	if(spectators.size() == 1)
	{
		// only forward the first client's packets
		NET_SendPacket(net_message.cursize, net_message.data, net_server);
		std::cout << "first client connect from " << NET_AdrToString(net_from) << std::endl;
	}
	else
	{
		// send back to the non-first client a fake challenge
		NET_SendPacket(challenge_message.cursize, challenge_message.data, net_from);
		NET_SendPacket(first_message.cursize, first_message.data, net_from);
		std::cout << "shadow client connect from " << NET_AdrToString(net_from) << std::endl;
	}
}

void OnPacketTV()
{
	if(NET_CompareAdr(net_from, net_server))
	{
		// if this is a server connect message, keep a copy for other clients
		int t = MSG_ReadLong();
		if(t == CHALLENGE)
		{
			std::cout << "first client connected" << std::endl;
			challenge_message = net_message;
		}
		if(t == 0)
		{
			first_message = net_message;
		}

		// replicate server packet to all connected clients
		for(int i = 0; i < spectators.size(); i++)
		{
			NET_SendPacket(net_message.cursize, net_message.data, spectators[i]);
		}
	}
	else
	{
		// is this an existing client?
		for(int i = 0; i < spectators.size(); i++)
		{
			if(NET_CompareAdr(net_from, spectators[i]))
			{
				OnClientTV(i);
				return;
			}
		}

		// must be a new client
		OnNewClientTV();
	}
}

