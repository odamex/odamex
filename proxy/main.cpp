#include <string.h>
#include <vector>

#ifdef UNIX
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#endif

#ifdef WIN32
#include <winsock.h>
#include <time.h>
#define usleep(n) Sleep(n/1000)
#endif

#include "../master/i_net.h"

netadr_t net_local, net_remote;

void OnInit()
{
	NET_StringToAdr("127.0.0.1:10667", &net_local);
	NET_StringToAdr("voxelsoft.com:10666", &net_remote);
}

void OnPacket()
{
	if(NET_CompareAdr(net_from, net_local))
	{
		NET_SendPacket(net_message.cursize, net_message.data, net_remote);
	}
	else if(NET_CompareAdr(net_from, net_remote))
	{
		NET_SendPacket(net_message.cursize, net_message.data, net_local);
	}
}

typedef void (*fp)();

struct protocol_t
{
	const char *name;
	fp onInit;
	fp onPacket;
};

void OnPacketTV();
void OnInitTV();

int main()
{
	//protocol_t protocol = {"transparent", OnInit, OnPacket};
	protocol_t protocol = {"odatv", OnInitTV, OnPacketTV};

	// Create a UDP socket
	localport = 10999;
	InitNetCommon();

	protocol.onInit();

	while (true)
	{
		while (NET_GetPacket())
		{
			protocol.onPacket();
		}

		usleep(1);
	}

	CloseNetwork();
}

