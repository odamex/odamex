// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62)
// Copyright (C) 2006-2012 by The Odamex Team
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
//	Main loop
//
//-----------------------------------------------------------------------------



#if _MSC_VER == 1200
// MSVC6, disable broken warnings about truncated stl lines
#pragma warning(disable:4786)
#include <iostream>
#endif

#include <string>
#include <vector>
#include <list>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#include "i_net.h"

using namespace std;

#define MAX_SERVERS					1024
#define MAX_SERVERS_PER_IP			64
#define MAX_SERVER_AGE				5000
#define MAX_UNVERIFIED_SERVER_AGE	1000

#define LOGFILE "master_log.txt"

buf_t message(MAX_UDP_PACKET);

typedef struct server
{
	netadr_t addr;
	int age;

	// from server itself
	string hostname;
	int players, maxplayers;
	string map;
	vector<string> pwads;
	int gametype, skill, teamplay, ctfmode;
	vector<string> playernames;
	vector<int> playerfrags;
	vector<int> playerpings;
	vector<int> playerteams;

	unsigned int key_sent;
	bool pinged, verified;

	server() : age(0), players(0), maxplayers(0), gametype(0), skill(0), teamplay(0), ctfmode(0), key_sent(0), pinged(0), verified(0) { memset(&addr, 0, sizeof(addr)); }

} SServer;

list<SServer> servers;
list<SServer>::iterator ping_itr = servers.begin(); // this iterator must be updated when servers is changed

bool ipReachedLimit(netadr_t addr)
{
	list<SServer>::iterator itr;
	int verifiedservers = 0;

	// count verified servers
	for (itr = servers.begin(); itr != servers.end(); ++itr)
		if ((*itr).verified && memcmp(&(*itr).addr.ip, &addr.ip, 4) == 0)
			verifiedservers++;

	return verifiedservers >= MAX_SERVERS_PER_IP;
}

void addServer(netadr_t addr)
{
	list<SServer>::iterator itr;
	SServer temp;

	for (itr = servers.begin(); itr != servers.end(); ++itr)
	{
		if (memcmp(&(*itr).addr.ip, &addr.ip, 4) == 0)
		{
			if ((*itr).addr.port == addr.port)
			{
				(*itr).age = 0;
				(*itr).pinged = false;
				return;
			}
		}
	}

	if (servers.size() < MAX_SERVERS)
	{
		if(ipReachedLimit(addr))
			return;

		memcpy(&temp.addr, &addr, sizeof(addr));
		temp.age = 0;
		servers.push_back(temp);

		printf("Added new server: %s, %d total\n", NET_AdrToString(temp.addr), (int)servers.size());
		FILE *fp = fopen(LOGFILE, "a");

		if(fp)
		{
			fprintf(fp, "Server registered: %s, %d total\r\n", NET_AdrToString(temp.addr), (int)servers.size());
			fclose(fp);
		}
		else
			printf("Failed to write to logfile %s\n", LOGFILE);
		return;
	}

	printf("Failed to add server: %s, no slots left\n", NET_AdrToString(addr));
}

void addServerInfo(netadr_t addr)
{
	list<SServer>::iterator itr;
	size_t i;

	for (itr = servers.begin(); itr != servers.end(); ++itr)
	{
		if (memcmp(&(*itr).addr.ip, &addr.ip, 4) != 0)
			continue;

		if ((*itr).addr.port != addr.port)
			continue;

		SServer &s = *itr;

		if(!s.key_sent)
			continue;
			
		net_message.ReadLong();

		// check key against one we issued
		if((unsigned)net_message.ReadLong() != s.key_sent)
			continue;

		// do not allow too many servers
		if(ipReachedLimit((*itr).addr))
			continue;

		printf("Server info, IP = %s\n", NET_AdrToString(addr));

		s.verified = true;
		s.age = 0;

		s.hostname = net_message.ReadString();
		s.players = net_message.ReadByte();
		s.maxplayers = net_message.ReadByte();
		s.map = net_message.ReadString();

		int pwadcount = net_message.ReadByte();
		if(pwadcount < 0)
			pwadcount = 0;

		s.pwads.resize(pwadcount);

		for(i = 0; i < s.pwads.size(); i++)
			s.pwads[i] = net_message.ReadString();

		s.gametype = net_message.ReadByte();
		s.skill = net_message.ReadByte();
		s.teamplay = net_message.ReadByte();
		s.ctfmode = net_message.ReadByte();

		byte playercount = net_message.ReadByte();

		s.playernames.resize(playercount);
		s.playerfrags.resize(playercount);
		s.playerpings.resize(playercount);
		s.playerteams.resize(playercount);

		for(i = 0; i < playercount; i++)
		{
			s.playernames[i] = net_message.ReadString();
			s.playerfrags[i] = net_message.ReadShort();
			s.playerpings[i] = net_message.ReadLong();
			s.playerteams[i] = net_message.ReadByte();
		}

		break;
	}

	return;
}

void ageServers(void)
{
	list<SServer>::iterator itr;

	itr = servers.begin();

	while (itr != servers.end())
	{
		(*itr).age++;

		if((*itr).verified)
		{
			if ((*itr).age > MAX_SERVER_AGE)
			{
				printf("Remote server timed out: %s, ", NET_AdrToString((*itr).addr));
				if(ping_itr == itr)
					++ping_itr;
				itr = servers.erase(itr);
				printf("%d total\n", (int)servers.size());
			}
			else
				++itr;
		}
		else
		{
			if ((*itr).age > MAX_UNVERIFIED_SERVER_AGE)
			{
				printf("Remote server timed out: %s, ", NET_AdrToString((*itr).addr));
				if(ping_itr == itr)
					++ping_itr;
				itr = servers.erase(itr);
				printf("%d total\n", (int)servers.size());
			}
			else
				++itr;
		}
	}
}

void dumpServersToFile(const char *file = "./latest")
{
	static bool file_error = false;
	FILE *fp = fopen(file, "w");

	if(!fp)
	{
		if(!file_error)
			printf("error opening file %s for writing\n", file);
		file_error = true;
		return;
	}

	file_error = false;

	list<SServer>::iterator itr;

	itr = servers.begin();

	fprintf(fp, "\"Name\",\"Map\",\"Players/Max\",\"WADs\",\"Gametype\",\"Address:Port\"\n");

	int i = 0;

	while (itr != servers.end())
	{
		if(!(*itr).verified)
		{
			++itr;
			continue;
		}

        string detectgametype = "ERROR";
		if((*itr).gametype == 0)
			detectgametype = "COOP";
		else
			detectgametype = "DM";
		if((*itr).gametype == 1 && (*itr).teamplay == 1)
			detectgametype = "TEAM DM";
		if((*itr).ctfmode == 1)
			detectgametype = "CTF";

		string str_wads;
		for(size_t j = 0; j < (*itr).pwads.size(); j++)
		{
			str_wads += (*itr).pwads[j];
			str_wads += " ";
		}
		if(!str_wads.length())
			str_wads = " ";

		fprintf(fp, "\"%s\",\"%s\",\"%d/%d\",\"%s\",\"%s\",\"%s\"\n", (*itr).hostname.c_str(), (*itr).map.c_str(), (*itr).players, (*itr).maxplayers, str_wads.c_str(), detectgametype.c_str(), NET_AdrToString((*itr).addr, true));

		i++;
		++itr;
	}

    fclose(fp);
}

void writeServerData(void)
{
	list<SServer>::iterator itr;
	size_t num_verified = 0;

	// count verified servers
	for (itr = servers.begin(); itr != servers.end(); ++itr)
		if((*itr).verified)
			num_verified++;
	
	message.WriteShort(num_verified);

	for (itr = servers.begin(); itr != servers.end(); ++itr)
	{
		if(!(*itr).verified)
			continue;

		for (int i = 0; i < 4; ++i)
			message.WriteByte((*itr).addr.ip[i]);
		message.WriteShort(htons((*itr).addr.port));
	}
}

void daemon_init(void)
{
#ifdef UNIX
    int pid;
    FILE *fpid;

    if ((pid = fork()) != 0)
        exit(0);

    pid = getpid();
    fpid = fopen("MasterPID", "w");
	if(fpid)
	{
		fprintf(fpid, "%d\n", pid);
		fclose(fpid);
	}
#endif
}

void pingServer(SServer &s)
{
	if(s.pinged && !s.verified)
	{
		return; // have already asked and got no answer
	}
	
#ifdef WIN32
	s.key_sent = rand() * (int)GetModuleHandle(0) * time(0);
#else
	s.key_sent = rand() * getpid() * time(0);
#endif
	
	message.clear();
	message.WriteLong(LAUNCHER_CHALLENGE);
	message.WriteLong(s.key_sent);
	
	NET_SendPacket(message.cursize, message.data, s.addr);
	
	s.pinged = true;
}

int main()
{
	int challenge;
	localport = MASTERPORT;
	InitNetCommon();

	daemon_init();

	printf("Odamex Master Started\n");

	while (true)
	{
		while (NET_GetPacket())
		{
			challenge = net_message.ReadLong();

			switch (challenge)
			{
			case 0:
			case SERVER_CHALLENGE:
				if(net_message.BytesLeftToRead() > 2)
				{
					// full reply with deathmatch, wad, etc
					addServerInfo(net_from);
				}
				else
				{
					// plain contact
					if(net_message.BytesLeftToRead() == 2)
					{
						unsigned short use_port = net_message.ReadShort();
						net_from.port = htons(use_port);
					}

					addServer(net_from);
				}
			    break;
			case LAUNCHER_CHALLENGE:
				if(net_message.BytesLeftToRead() > 0)
				{
					printf("Master syncing server list (ignored), IP = %s\n", NET_AdrToString(net_from));
				}
				else
				{
					printf("Client request IP = %s\n", NET_AdrToString(net_from));
					message.clear();
					message.WriteLong(LAUNCHER_CHALLENGE);
					writeServerData();
					NET_SendPacket(message.cursize, message.data, net_from);
				}
			    break;
			default:
				break;
			}
		}

		static int counter = 0;

		usleep(50*1000); // every 50ms // denis - fixme - use proper timer, age of servers should increase even during packet sending

		ageServers();

		if(!(counter%100))
		{
			dumpServersToFile();

			if (ping_itr == servers.end())
				ping_itr = servers.begin();

			if(ping_itr != servers.end())
				pingServer(*(ping_itr++));
		}

		counter++;
	}

	servers.clear();

	CloseNetwork();

	return 0;
}
