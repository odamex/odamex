// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	Launcher packet structure file
//
// AUTHORS: 
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------


#ifndef NET_PACKET_H
#define NET_PACKET_H

#include <string>
#include <vector>

#include <agar/core.h> // For AG_Mutex
#include <agar/config/ag_debug.h> // Determine if Agar is compiled for debugging

#include "net_io.h"
#include "typedefs.h"

#define ASSEMBLEVERSION(MAJOR,MINOR,PATCH) ((MAJOR) * 256 + (MINOR)(PATCH))
#define DISECTVERSION(V,MAJOR,MINOR,PATCH) \
{ \
	MAJOR = (V / 256); \
	MINOR = ((V % 256) / 10); \
	PATCH = ((V % 256) % 10); \
}

#define VERSIONMAJOR(V) (V / 256)
#define VERSIONMINOR(V) ((V % 256) / 10)
#define VERSIONPATCH(V) ((V % 256) % 10)

#define VERSION (0*256+62)
#define PROTOCOL_VERSION 1

#define TAG_ID 0xAD0

#if !AG_VERSION_ATLEAST(1,4,2)
	#define AG_MutexTryLock AG_MutexTrylock
#endif

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

const uint32_t MASTER_CHALLENGE = 777123;
const uint32_t MASTER_RESPONSE  = 777123;
const uint32_t SERVER_CHALLENGE = 0xAD011002;

struct Cvar_t
{
	std::string Name;
	std::string Value;
};

struct Wad_t
{
	std::string Name;
	std::string Hash;
};

struct Team_t
{
	std::string Name;
	uint32_t    Colour;
	int16_t     Score;
};

struct Player_t
{
	std::string Name;
	int16_t     Frags;
	uint16_t    Ping;
	uint8_t     Team;
	uint16_t    Kills;
	uint16_t    Deaths;
	uint16_t    Time;
	bool        Spectator;
};

enum GameType_t
{
	GT_Cooperative = 0
	,GT_Deathmatch
	,GT_TeamDeathmatch
	,GT_CaptureTheFlag
	,GT_Max
};

struct ServerInfo_t
{
	uint32_t                 Response; // Launcher specific: Server response
	uint8_t                  VersionMajor; // Launcher specific: Version fields
	uint8_t                  VersionMinor;
	uint8_t                  VersionPatch;
	uint32_t                 VersionProtocol;
	uint32_t                 VersionRealProtocol;
	uint32_t                 VersionRevision;
	uint32_t                 PTime;
	std::string              Name; // Launcher specific: Server name
	uint8_t                  MaxClients; // Launcher specific: Maximum clients
	uint8_t                  MaxPlayers; // Launcher specific: Maximum players
	GameType_t               GameType; // Launcher specific: Game type
	uint16_t                 ScoreLimit; // Launcher specific: Score limit
	std::vector<Cvar_t>      Cvars;
	std::string              PasswordHash;
	std::string              CurrentMap;
	uint16_t                 TimeLeft;
	std::vector<Team_t>      Teams;
	std::vector<std::string> Patches;
	std::vector<Wad_t>       Wads;
	std::vector<Player_t>    Players;
};

class ServerBase  // [Russell] - Defines an abstract class for all packets
{
protected:      
	BufferedSocket Socket;

	// Magic numbers
	uint32_t challenge;
	uint32_t response;

	// The time in milliseconds a packet was received
	uint32_t Ping;

	AG_Mutex m_Mutex;
public:
	// Constructor
	ServerBase() 
	{
		Ping = 0;
		challenge = 0;
		response = 0;

		AG_MutexInit(&m_Mutex);
	}

	// Destructor
	virtual ~ServerBase()
	{
	}

	// Parse a packet, the parameter is the packet
	virtual int32_t Parse() { return -1; }

	// Query the server
	int32_t Query(int32_t Timeout);

	void SetAddress(const std::string &Address, const int16_t &Port) 
	{ 
		Socket.SetRemoteAddress(Address, Port);
	}

	std::string GetAddress() const { return Socket.GetRemoteAddress(); }
	uint32_t GetPing() const { return Ping; }

	void GetLock() { AG_MutexLock(&m_Mutex); }
	void Unlock() { AG_MutexUnlock(&m_Mutex); }
	int TryLock() { return AG_MutexTryLock(&m_Mutex); }
};

class MasterServer : public ServerBase  // [Russell] - A master server packet
{
private:
	// Address format structure
	typedef struct
	{
		std::string ip;
		uint16_t    port;
		bool        custom;
	} addr_t;

	std::vector<addr_t> addresses;
	std::vector<addr_t> masteraddresses;
public:
	MasterServer() 
	{ 
		challenge = MASTER_CHALLENGE;
		response = MASTER_CHALLENGE;
	}

	virtual ~MasterServer() 
	{ 

	}

	size_t GetServerCount() { return addresses.size(); }

	bool GetServerAddress(const size_t &Index, 
			std::string &Address, 
			uint16_t &Port)
	{
		if (Index < addresses.size())
		{
			Address = addresses[Index].ip;
			Port = addresses[Index].port;

			return addresses[Index].custom;
		}

		return false;
	}

	void AddMaster(const std::string &Address, const uint16_t &Port)
	{
		addr_t Master = { Address, Port, true };

		if ((Master.ip.size()) && (Master.port != 0))
			masteraddresses.push_back(Master);
	}

	void QueryMasters(const uint32_t &Timeout)
	{           
		DeleteAllNormalServers();

		for (size_t i = 0; i < masteraddresses.size(); ++i)
		{
			Socket.SetRemoteAddress(masteraddresses[i].ip, masteraddresses[i].port);

			Query(Timeout);
		}
	}

	size_t GetMasterCount() { return masteraddresses.size(); }

	void DeleteAllNormalServers()
	{
		size_t i = 0;

		// don't delete our custom servers!
		while (i < addresses.size())
		{       
			if (addresses[i].custom == false)
			{
				addresses.erase(addresses.begin() + i);
				continue;
			}

			++i;
		}            
	}

	void AddCustomServer(const std::string &Address, const uint16_t &Port)
	{
		addr_t cs;

		cs.ip = Address;
		cs.port = Port;
		cs.custom = true;

		// Don't add the same address more than once.
		for (uint32_t i = 0; i < addresses.size(); ++i)
		{
			if (addresses[i].ip == cs.ip && 
					addresses[i].port == cs.port &&
					addresses[i].custom == cs.custom)
			{
				return;
			}
		}

		addresses.push_back(cs);
	}

	bool DeleteCustomServer(const size_t &Index)
	{
		if (Index < addresses.size())
		{
			if (addresses[Index].custom)
			{
				addresses.erase(addresses.begin() + Index);

				return true;
			}
			else
				return false;
		}

		return false;
	}

	void DeleteAllCustomServers()
	{
		size_t i = 0;

		while (i < addresses.size())
		{       
			if (DeleteCustomServer(i))
				continue;

			++i;
		}       
	}

	int32_t Parse();
};

class Server : public ServerBase  // [Russell] - A single server
{           
public:
	ServerInfo_t Info;

	Server();

	void ResetData();

	virtual  ~Server();

	int32_t Query(int32_t Timeout);

	void ReadInformation(const uint8_t &VersionMajor, 
			const uint8_t &VersionMinor,
			const uint8_t &VersionPatch,
			const uint32_t &ProtocolVersion);

	int32_t TranslateResponse(const uint16_t &TagId, 
			const uint8_t &TagApplication,
			const uint8_t &TagQRId,
			const uint16_t &TagPacketType);

	bool GotResponse() const { return m_ValidResponse; }

	int32_t Parse();

protected:
	bool m_ValidResponse;
};

} // namespace

#endif // NETPACKET_H
