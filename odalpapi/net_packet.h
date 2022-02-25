// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//  Launcher packet structure file
//
// AUTHORS:
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------


#ifndef NET_PACKET_H
#define NET_PACKET_H

#include <cstdlib>
#include <string>
#include <vector>
#include <sstream>

// todo: replace with a generic implementation
#if 0
#include <agar/core.h> // For AG_Mutex
#include <agar/config/ag_debug.h> // Determine if Agar is compiled for debugging
#endif

#include "net_io.h"
#include "typedefs.h"
#include "threads/mutex_factory.h"

/**
 * @brief Construct a packed integer from major, minor and patch version
 *        numbers.
 *
 * @param major Major version number.
 * @param minor Minor version number - must be between 0 and 25.
 * @param patch Patch version number - must be between 0 and 9.
 */
#define MAKEVER(major, minor, patch) ((major)*256 + ((minor)*10) + (patch))

// [AM] TODO: Bring over other macros from Odamex proper.

#define DISECTVERSION(V,MAJOR,MINOR,PATCH) \
{ \
    MAJOR = (V / 256); \
    MINOR = ((V % 256) / 10); \
    PATCH = ((V % 256) % 10); \
}

#define VERSIONMAJOR(V) (V / 256)
#define VERSIONMINOR(V) ((V % 256) / 10)
#define VERSIONPATCH(V) ((V % 256) % 10)

#define VERSION (MAKEVER(11, 0, 0))
#define PROTOCOL_VERSION 8

#define TAG_ID 0xAD0

/**
 * odalpapi namespace.
 *
 * All code for the odamex launcher api is contained within the odalpapi
 * namespace.
 */
namespace odalpapi
{

const uint32_t MASTER_CHALLENGE = 777123;
const uint32_t MASTER_RESPONSE  = 777123;
const uint32_t SERVER_CHALLENGE = 0xAD011002;
const uint32_t SERVER_VERSION_CHALLENGE = 0xAD011001;

// Hints for network code optimization
typedef enum
{
	CVARTYPE_NONE = 0 // Used for no sends

	                ,CVARTYPE_BOOL
	,CVARTYPE_BYTE
	,CVARTYPE_WORD
	,CVARTYPE_INT
	,CVARTYPE_FLOAT
	,CVARTYPE_STRING

	,CVARTYPE_MAX = 255
} CvarType_t;

struct Cvar_t
{
	std::string Name;
	std::string Value;

	union
	{
		int32_t i32;
		uint32_t ui32;
		int16_t i16;
        uint16_t ui16;
		int8_t i8;
		uint8_t ui8;
		bool b;
	};

	uint8_t Type;
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
	uint32_t    Colour;
	uint16_t    Kills;
	uint16_t    Deaths;
	uint16_t    Time;
	int16_t     Frags;
	uint16_t    Ping;
	uint8_t     Team;
	bool        Spectator;
};

enum GameType_t
{
	GT_Cooperative = 0,
	GT_Deathmatch,
	GT_TeamDeathmatch,
	GT_CaptureTheFlag,
	GT_Horde,
	GT_Max
};

struct ServerInfo_t
{
	std::vector<std::string> Patches;
	std::vector<Cvar_t>      Cvars;
	std::vector<Team_t>      Teams;
	std::vector<Wad_t>       Wads;
	std::vector<Player_t>    Players;
	std::string              Name; // Launcher specific: Server name
	std::string              PasswordHash;
	std::string              CurrentMap;
	std::string              VersionRevStr;
	GameType_t               GameType; // Launcher specific: Game type
	uint32_t                 Response; // Launcher specific: Server response
	uint32_t                 VersionRevision;
	uint32_t                 VersionProtocol;
	uint32_t                 VersionRealProtocol;
	uint32_t                 PTime;
	uint16_t                 ScoreLimit; // Launcher specific: Score limit
	uint16_t                 TimeLimit;
	uint16_t                 TimeLeft;
	uint8_t                  VersionMajor; // Launcher specific: Version fields
	uint8_t                  VersionMinor;
	uint8_t                  VersionPatch;
	uint8_t                  MaxClients; // Launcher specific: Maximum clients
	uint8_t                  MaxPlayers; // Launcher specific: Maximum players
	uint16_t                 Lives;
	uint16_t                 Sides;
};

class ServerBase  // [Russell] - Defines an abstract class for all packets
{
protected:
	std::string m_Address;

	// The time in milliseconds a packet was received
	uint64_t Ping;

	BufferedSocket* Socket;

	// Magic numbers
	uint32_t challenge;
	uint32_t response;

	uint16_t m_Port;

	uint8_t m_RetryCount;

	threads::Mutex* m_Mutex;
public:
	// Constructor
	ServerBase()
	{
		Ping = 0;
		challenge = 0;
		response = 0;

		m_RetryCount = 2;

		m_Port = 0;

		Socket = NULL;
		m_Mutex = threads::MutexFactory::inst().createMutex();
	}

	// Destructor
	virtual ~ServerBase()
	{
		if(NULL != m_Mutex)
		{
			delete m_Mutex;
		}
	}

	// Parse a packet, the parameter is the packet
	virtual int32_t Parse()
	{
		return -1;
	}

	// Query the server
	int32_t Query(int32_t Timeout);

	void SetSocket(BufferedSocket* s)
	{
		Socket = s;
	}

	void SetAddress(const std::string& Address, const uint16_t& Port)
	{
		m_Address = Address;
		m_Port = Port;
	}

	std::string GetAddress() const
	{
		std::ostringstream Address;

		Address << m_Address << ":" << m_Port;

		return Address.str();
	}

	void GetAddress(std::string& Address, uint16_t& Port) const
	{
		Address = m_Address;
		Port = m_Port;
	}
	uint64_t GetPing() const
	{
		return Ping;
	}

	void SetRetries(int8_t Count)
	{
		m_RetryCount = Count;
	}

	int GetLock() { return NULL != m_Mutex ? m_Mutex->getLock() : 0; }
	int TryLock() { return NULL != m_Mutex ? m_Mutex->tryLock() : 0; }
	int Unlock() { return NULL != m_Mutex ? m_Mutex->unlock() : 0; }
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

	void QueryBC(const uint32_t& Timeout);

	// Translates a string address to an addr_t structure
	// Only modifies ip and port
	bool StrAddrToAddrT(const std::string &In, addr_t &Out)
	{
		size_t colon = In.find(':');

		if(colon == std::string::npos)
			return false;

		if(colon + 1 >= In.length())
			return false;

		Out.port = atoi(In.substr(colon + 1).c_str());
		Out.ip = In.substr(0, colon);
		
		return true;
	}
public:
	MasterServer()
	{
		challenge = MASTER_CHALLENGE;
		response = MASTER_CHALLENGE;
	}

	virtual ~MasterServer()
	{

	}

	size_t GetServerCount()
	{
		return addresses.size();
	}

	bool GetServerAddress(const size_t& Index,
	                      std::string& Address,
	                      uint16_t& Port)
	{
		if(Index < addresses.size())
		{
			Address = addresses[Index].ip;
			Port = addresses[Index].port;

			return addresses[Index].custom;
		}

		return false;
	}

	void AddMaster(const addr_t Master)
	{
		if((Master.ip.size()) && (Master.port != 0))
			masteraddresses.push_back(Master);
	}

	bool AddMaster(std::string Address)
	{
        addr_t Master;
		
		if (!StrAddrToAddrT(Address, Master))
            return false;
		
		Master.custom = true;
		
		AddMaster(Master);

		return true;
	}

	void QueryMasters(const uint32_t& Timeout, const bool& Broadcast,
	                  const int8_t& Retries)
	{
		DeleteServers();

		m_RetryCount = Retries;

		if(Broadcast)
			QueryBC(Timeout);

		for(size_t i = 0; i < masteraddresses.size(); ++i)
		{
			m_Address = masteraddresses[i].ip;
			m_Port = masteraddresses[i].port;

			Query(Timeout);
		}
	}

	size_t GetMasterCount()
	{
		return masteraddresses.size();
	}

	bool IsCustomServer(size_t &Index)
	{
        if(Index < addresses.size())
		{
		    return addresses[Index].custom;
		}
		
		return false;
	}
	
	bool IsCustomServer(const std::string &Address)
	{
	    std::vector<addr_t>::const_iterator i;
	    addr_t ServerAddr;

	    if (!StrAddrToAddrT(Address, ServerAddr))
            return false;

        for (i = addresses.begin(); i != addresses.end(); ++i)
        {
            if (i->ip == ServerAddr.ip && 
                i->port == ServerAddr.port)
            {
                if (i->custom)
                    return true;
            }
        }
        
        return false;
	}
	
	void AddServer(const std::string& Address, const uint16_t& Port,
	               const bool& Custom = false)
	{
		addr_t cs;

		cs.ip = Address;
		cs.port = Port;
		cs.custom = Custom;

		AddServer(cs);
	}

	void AddServer(const addr_t& cs)
	{
		// Don't add the same address more than once.
		for(size_t i = 0; i < addresses.size(); ++i)
		{
			if(addresses[i].ip == cs.ip &&
			        addresses[i].port == cs.port &&
			        addresses[i].custom == cs.custom)
			{
				return;
			}
		}

		addresses.push_back(cs);
	}

	bool DeleteServer(const size_t& Index)
	{
		if(Index < addresses.size())
		{
			addresses.erase(addresses.begin() + Index);

			return true;
		}

		return false;
	}

	void DeleteServers(const bool& Custom = false)
	{
		size_t i = 0;

		while(i < addresses.size())
		{
			if(addresses[i].custom == Custom)
			{
				DeleteServer(i);

				continue;
			}

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

	void ReadInformation();

	int32_t TranslateResponse(const uint16_t& TagId,
	                          const uint8_t& TagApplication,
	                          const uint8_t& TagQRId,
	                          const uint16_t& TagPacketType);

	bool GotResponse() const
	{
		return m_ValidResponse;
	}

	int32_t Parse();

protected:
	bool ReadCvars();

	bool m_ValidResponse;
};

} // namespace

#endif // NETPACKET_H
