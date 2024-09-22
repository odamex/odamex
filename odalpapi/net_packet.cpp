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

#include <iostream>
#include <sstream>
#include <cstdlib>

#include "net_packet.h"
#include "net_error.h"
//#include "net_cvartable.h"

using namespace std;

namespace odalpapi
{

/*
   Create a packet to send, which in turn will
   receive data from the server

NOTE: All packet classes derive from this, so if some
packets require different data, you'll have
to override this
*/
int32_t ServerBase::Query(int32_t Timeout)
{
	int8_t Retry = m_RetryCount;

	if(m_Address.empty() || !m_Port)
		return 0;

	Socket->SetRemoteAddress(m_Address, m_Port);

	Socket->ClearBuffer();

	// If we didn't get it the first time, try again
	while(Retry)
	{
		Socket->Write32(challenge);

		if(!Socket->SendData(Timeout))
			return 0;

		int32_t err = Socket->GetData(Timeout);

		switch(err)
		{
		case -1:
		case -3:
		{
			Socket->ClearBuffer();
			--Retry;
			continue;
		};

		case -2:
			return 0;

		default:
			goto ok;
		}
	}

	if(!Retry)
		return 0;

ok:

	Ping = Socket->GetPing();

	if(!Parse())
		return 0;

	return 1;
}

/*
   Read a packet received from a master server
   */
int32_t MasterServer::Parse()
{
    ostringstream ipfmt;
    addr_t address = { "", 0, false };
	uint32_t temp_response;
	int16_t server_count;
    uint8_t ip1, ip2, ip3, ip4;

	// Make sure we have a valid response from the master server
	Socket->Read32(temp_response);

	if(temp_response != response)
	{
		Socket->ClearBuffer();

		return 0;
	}

	// Get the total amount of servers that this master server has registered
	Socket->Read16(server_count);

	if(!server_count)
	{
		Socket->ClearBuffer();

		return 0;
	}

	// Begin processing the list of server addresses that we have received
	for(int16_t i = 0; i < server_count; i++)
	{
        // Get the IP address and port number from the receive buffer
		Socket->Read8(ip1);
		Socket->Read8(ip2);
		Socket->Read8(ip3);
		Socket->Read8(ip4);
		Socket->Read16(address.port);

		// Format the IP address into dot notation and copy it into the address
		// structure
		ipfmt << (int)ip1 << "." << (int)ip2 << "." << (int)ip3 << "." << (int)ip4;
		address.ip = ipfmt.str();

		// Finally add the server address to the list and clear out the string 
		// stream object for further use in the next iteration
        AddServer(address);

		ipfmt.str("");
		ipfmt.clear();
	}

	// Check previous reading operations that may have failed
	if(Socket->BadRead())
	{
		Socket->ClearBuffer();

		return 0;
	}

	Socket->ClearBuffer();

	return 1;
}

// Server constructor
Server::Server()
{
	challenge = SERVER_CHALLENGE;

	ResetData();
}

Server::~Server()
{
	ResetData();
}

void Server::ResetData()
{
	m_ValidResponse = false;

	Info.Cvars.clear();
	Info.Wads.clear();
	Info.Players.clear();
	Info.Patches.clear();
	Info.Teams.clear();

	Info.Response = 0;
	Info.VersionMajor = 0;
	Info.VersionMinor = 0;
	Info.VersionPatch = 0;
	Info.VersionRevision = 0;
	Info.VersionRevStr = "";
	Info.VersionProtocol = 0;
	Info.VersionRealProtocol = 0;
	Info.PTime = 0;
	Info.Name = "";
	Info.MaxClients = 0;
	Info.MaxPlayers = 0;
	Info.ScoreLimit = 0;
	Info.GameType = GT_Cooperative;
	Info.PasswordHash = "";
	Info.CurrentMap = "";
	Info.TimeLeft = 0;
	Info.TimeLimit = 0;
	Info.Lives = 0;
	Info.Sides = 0;

	Ping = 0;
}

/*
   Inclusion/Removal macros of certain fields, it is MANDATORY to remove these
   with every new major/minor version
   */

static uint8_t VersionMajor, VersionMinor, VersionPatch;
static uint32_t ProtocolVersion;

// Specifies when data was added to the protocol, the parameter is the
// introduced revision
// NOTE: this one is different from the servers version for a reason
#define QRYNEWINFO(INTRODUCED) \
    if (ProtocolVersion >= INTRODUCED)

// Specifies when data was removed from the protocol, first parameter is the
// introduced revision and the last one is the removed revision
#define QRYRANGEINFO(INTRODUCED,REMOVED) \
    if (ProtocolVersion >= INTRODUCED && ProtocolVersion < REMOVED)

// Read cvar information
bool Server::ReadCvars()
{
	uint8_t CvarCount;

	Socket->Read8(CvarCount);

	for(size_t i = 0; i < CvarCount; ++i)
	{
		Cvar_t Cvar;

		Socket->ReadString(Cvar.Name);

		Socket->Read8(Cvar.Type);

		switch(Cvar.Type)
		{
		case CVARTYPE_BOOL:
		{
			Cvar.b = true;
		}
		break;

		case CVARTYPE_BYTE:
		{
			Socket->Read8(Cvar.i8);
		}
		break;

		case CVARTYPE_WORD:
		{
			Socket->Read16(Cvar.i16);
		}
		break;

		case CVARTYPE_INT:
		{
			Socket->Read32(Cvar.i32);
		}
		break;

		case CVARTYPE_FLOAT:
		case CVARTYPE_STRING:
		{
			Socket->ReadString(Cvar.Value);
		}
		break;

		case CVARTYPE_NONE:
		case CVARTYPE_MAX:
		default:
			break;
		}

		// Filter out important information for us to use, it'd be nicer to have
		// a launcher-side cvar implementation though
		if(Cvar.Name == "sv_hostname")
		{
			Info.Name = Cvar.Value;

			continue;
		}
		else if(Cvar.Name == "sv_maxplayers")
		{
			Info.MaxPlayers = Cvar.ui8;

			continue;
		}
		else if(Cvar.Name == "sv_maxclients")
		{
			Info.MaxClients = Cvar.ui8;

			continue;
		}
		else if(Cvar.Name == "sv_gametype")
		{
			Info.GameType = (GameType_t)Cvar.ui8;

			continue;
		}
		else if(Cvar.Name == "sv_scorelimit")
		{
			Info.ScoreLimit = Cvar.ui16;

			continue;
		}
		else if(Cvar.Name == "sv_timelimit")
		{
			// Add this to the cvar list as well
			Info.TimeLimit = Cvar.ui16;
		}
		else if (Cvar.Name == "g_lives")
		{
			Info.Lives = Cvar.ui16;
		}
		else if(Cvar.Name == "g_sides")
		{
			Info.Sides = Cvar.ui16;
		}

		Info.Cvars.push_back(Cvar);
	}

	return true;
}

// Read information built for us by the server
void Server::ReadInformation()
{
	Info.VersionMajor = VersionMajor;
	Info.VersionMinor = VersionMinor;
	Info.VersionPatch = VersionPatch;
	Info.VersionProtocol = ProtocolVersion;

	// bond - time
	Socket->Read32(Info.PTime);

	// The servers real protocol version
	// bond - real protocol
	Socket->Read32(Info.VersionRealProtocol);

	// Revision number of server
    // TODO: Remove guard before next release
	QRYNEWINFO(7)
	{
        Socket->ReadString(Info.VersionRevStr);
	}
	else
        Socket->Read32(Info.VersionRevision);
    
	// Read cvar data
	ReadCvars();

	Socket->ReadHexString(Info.PasswordHash);

	Socket->ReadString(Info.CurrentMap);

	// TODO: Remove guard for next release and update protocol version
	QRYNEWINFO(6)
	{
		if(Info.TimeLimit)
			Socket->Read16(Info.TimeLeft);
	}
	else
		Socket->Read16(Info.TimeLeft);

	// Teams
	if(Info.GameType == GT_TeamDeathmatch ||
	        Info.GameType == GT_CaptureTheFlag)
	{
		uint8_t TeamCount;

		Socket->Read8(TeamCount);

		for(size_t i = 0; i < TeamCount; ++i)
		{
			Team_t Team;

			Socket->ReadString(Team.Name);
			Socket->Read32(Team.Colour);
			Socket->Read16(Team.Score);

			Info.Teams.push_back(Team);
		}
	}

	// Dehacked/Bex files
	uint8_t PatchCount;

	Socket->Read8(PatchCount);

	for(size_t i = 0; i < PatchCount; ++i)
	{
		string Patch;

		Socket->ReadString(Patch);

		Info.Patches.push_back(Patch);
	}

	// Wad files
	uint8_t WadCount;

	Socket->Read8(WadCount);

	for(size_t i = 0; i < WadCount; ++i)
	{
		Wad_t Wad;

		Socket->ReadString(Wad.Name);
		Socket->ReadHexString(Wad.Hash);

		Info.Wads.push_back(Wad);
	}

	// Player information
	uint8_t PlayerCount;

	Socket->Read8(PlayerCount);

	for(size_t i = 0; i < PlayerCount; ++i)
	{
		Player_t Player;

		Socket->ReadString(Player.Name);
		Socket->Read32(Player.Colour);

		if(Info.GameType == GT_TeamDeathmatch ||
		        Info.GameType == GT_CaptureTheFlag)
		{
			Socket->Read8(Player.Team);
		}

		Socket->Read16(Player.Ping);
		Socket->Read16(Player.Time);
		Socket->ReadBool(Player.Spectator);
		Socket->Read16(Player.Frags);
		Socket->Read16(Player.Kills);
		Socket->Read16(Player.Deaths);

		Info.Players.push_back(Player);
	}
}

//
// Server::TranslateResponse()
//
// Figures out the response from the server, deciding whether to use this data
// or not
int32_t Server::TranslateResponse(const uint16_t& TagId,
                                  const uint8_t& TagApplication,
                                  const uint8_t& TagQRId,
                                  const uint16_t& TagPacketType)
{
	// It isn't a response
	if(TagQRId != 2)
	{
		//wxLogDebug("Query/Response Id is not valid"));

		return 0;
	}

	switch(TagApplication)
	{
	// Enquirer
	case 1:
	{
		//wxLogDebug("Application is Enquirer"));

		return 0;
	}
	break;

	// Client
	case 2:
	{
		//wxLogDebug("Application is Client"));

		return 0;
	}
	break;

	// Server
	case 3:
	{
		//wxLogDebug("Application is Server"));
	}
	break;

	// Master Server
	case 4:
	{
		//wxLogDebug("Application is Master Server"));

		return 0;
	}
	break;

	// Unknown
	default:
	{
		//wxLogDebug("Application is Unknown"));

		return 0;
	}
	break;
	}

	if(TagPacketType == 2)
	{
		// Launcher is an old version
		NET_ReportError("Launcher is too old to parse the data from Server %s",
		                Socket->GetRemoteAddress().c_str());

		return 0;
	}

	uint32_t SvVersion;
	uint32_t SvProtocolVersion;

	Socket->Read32(SvVersion);
	Socket->Read32(SvProtocolVersion);

	// Prevent possible divide by zero
	if(!SvVersion)
	{
		return 0;
	}

	int svmaj, svmin, svpat, olmaj, olmin, olpat;
	DISECTVERSION(SvVersion, svmaj, svmin, svpat);
	DISECTVERSION(VERSION, olmaj, olmin, olpat);

	// [AM] Show current major and next major versions.  This allows a natural
	//      upgrade path without signing us up for supporting every version of
	//      the SQP in perpituity.
	if (svmaj < olmaj || svmaj > olmaj + 1)
	{
		NET_ReportError("Server %s is version %d.%d.%d which is not supported\n",
		                Socket->GetRemoteAddress().c_str(),
		                svmaj, svmin, svpat);

		return 0;
	}

	VersionMajor = VERSIONMAJOR(SvVersion);
	VersionMinor = VERSIONMINOR(SvVersion);
	VersionPatch = VERSIONPATCH(SvVersion);
	ProtocolVersion = SvProtocolVersion;

	ReadInformation();

	if(Socket->BadRead())
	{
		// Bad packet data encountered
		NET_ReportError("Data from Server %s was out of sequence, please report!\n",
		                Socket->GetRemoteAddress().c_str());

		return 0;
	}

	// Success
	return 1;
}

int32_t Server::Parse()
{
	Socket->Read32(Info.Response);

	// Decode the tag into its fields
	// TODO: this may not be 100% correct
	uint16_t TagId = ((Info.Response >> 20) & 0x0FFF);
	uint8_t TagApplication = ((Info.Response >> 16) & 0x0F);
	uint8_t TagQRId = ((Info.Response >> 12) & 0x0F);
	uint16_t TagPacketType = (Info.Response & 0xFFFF0FFF);

	if(TagId == TAG_ID)
	{
		int32_t Result = TranslateResponse(TagId,
		                                   TagApplication,
		                                   TagQRId,
		                                   TagPacketType);

		Socket->ClearBuffer();

		m_ValidResponse = Result ? true : false;

		return Result;
	}

	m_ValidResponse = false;

	Info.Response = 0;

	Socket->ClearBuffer();

	return 0;
}

int32_t Server::Query(int32_t Timeout)
{
	int8_t Retry = m_RetryCount;

	if(m_Address.empty() || !m_Port)
		return 0;

	Socket->SetRemoteAddress(m_Address, m_Port);

	Socket->ClearBuffer();

	ResetData();

	// If we didn't get it the first time, try again
	while(Retry)
	{
		Socket->Write32(challenge);
		Socket->Write32(VERSION);
		Socket->Write32(PROTOCOL_VERSION);
		// bond - time
		Socket->Write32(Info.PTime);

		if(!Socket->SendData(Timeout))
			return 0;

		int32_t err = Socket->GetData(Timeout);

		switch(err)
		{
		case -1:
		case -3:
		{
			Socket->ResetBuffer();
			Socket->ResetSize();
			--Retry;
			continue;
		};

		case -2:
			return 0;

		default:
			goto ok;
		}
	}

	if(!Retry)
		return 0;

ok:

	Ping = Socket->GetPing();

	if(!Parse())
		return 0;

	return 1;
}

// Send network-wide broadcasts
void MasterServer::QueryBC(const uint32_t& Timeout)
{
	BufferedSocket BCSocket;

	BCSocket.ClearBuffer();

	BCSocket.SetRemoteAddress("255.255.255.255", 10666);

	// TODO: it might be better to create actual Server objects so we do not
	// have to requery when we get past this stage
	BCSocket.Write32(SERVER_VERSION_CHALLENGE);
	BCSocket.Write32(VERSION);
	BCSocket.Write32(PROTOCOL_VERSION);

	BCSocket.Write32(0);

	BCSocket.SetBroadcast(true);

	BCSocket.SendData(Timeout);

	while(BCSocket.GetData(Timeout) > 0)
	{
		addr_t address = { "", 0, false};

		BCSocket.GetRemoteAddress(address.ip, address.port);

		AddServer(address);
	}
}

} // namespace

