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

#include <iostream>
#include <sstream>
#include <cstdlib>

#include "net_packet.h"

using namespace std;

namespace agOdalaunch {

/*
   Create a packet to send, which in turn will
   receive data from the server

NOTE: All packet classes derive from this, so if some
packets require different data, you'll have
to override this
*/
int32_t ServerBase::Query(int32_t Timeout)
{
	string Address = Socket.GetRemoteAddress();   

	if (Address != "")
	{
		Socket.ClearBuffer();

		Socket.Write32(challenge);

		if(!Socket.SendData(Timeout))
			return 0;

		if (!Socket.GetData(Timeout))
			return 0;

		Ping = Socket.GetPing();

		if (!Parse())
			return 0;

		return 1;
	}

	return 0;
}

/*
   Read a packet received from a master server

   If this already contains server addresses
   it will be freed and reinitialized (horrible I know)
   */
int32_t MasterServer::Parse()
{

	// begin reading

	uint32_t temp_response;

	Socket.Read32(temp_response);

	if (temp_response != response)
	{
		Socket.ClearBuffer();

		return 0;
	}

	int16_t server_count;

	Socket.Read16(server_count);

	if (!server_count)
	{
		Socket.ClearBuffer();

		return 0;
	}

	// Add on to any servers already in the list
	if (server_count)
		for (int16_t i = 0; i < server_count; i++)
		{
			addr_t address;
			uint8_t ip1, ip2, ip3, ip4;

			Socket.Read8(ip1);
			Socket.Read8(ip2);
			Socket.Read8(ip3);
			Socket.Read8(ip4);   

			ostringstream stream;

			stream << (int)ip1 << "." << (int)ip2 << "." << (int)ip3 << "." << (int)ip4;
			address.ip = stream.str();

			Socket.Read16(address.port);

			address.custom = false;

			size_t j = 0;

			// Don't add the same address more than once.
			for (j = 0; j < addresses.size(); ++j)
			{
				if (addresses[j].ip == address.ip && 
						addresses[j].port == address.port)
				{
					break;
				}
			}

			// didn't find it, so add it
			if (j == addresses.size())
				addresses.push_back(address);
		}

	if (Socket.BadRead())
	{
		Socket.ClearBuffer();

		return 0;
	}

	Socket.ClearBuffer();

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

	Ping = 0;
}

/* 
   Inclusion/Removal macros of certain fields, it is MANDATORY to remove these
   with every new major/minor version
   */

// Specifies when data was added to the protocol, the parameter is the
// introduced revision
// NOTE: this one is different from the servers version for a reason
#define QRYNEWINFO(INTRODUCED) \
	if (ProtocolVersion >= INTRODUCED)

// Specifies when data was removed from the protocol, first parameter is the
// introduced revision and the last one is the removed revision
#define QRYRANGEINFO(INTRODUCED,REMOVED) \
	if (ProtocolVersion >= INTRODUCED && ProtocolVersion < REMOVED)

//
// Server::ReadInformation()
//
// Read information built for us by the server
void Server::ReadInformation(const uint8_t &VersionMajor, 
                             const uint8_t &VersionMinor,
                             const uint8_t &VersionPatch,
                             const uint32_t &ProtocolVersion)
{
	Info.VersionMajor = VersionMajor;
	Info.VersionMinor = VersionMinor;
	Info.VersionPatch = VersionPatch;
	Info.VersionProtocol = ProtocolVersion;

	Socket.Read32(Info.PTime);
	Socket.Read32(Info.VersionRealProtocol);
	Socket.Read32(Info.VersionRevision);

	uint8_t CvarCount;

	Socket.Read8(CvarCount);

	for (size_t i = 0; i < CvarCount; ++i)
	{
		Cvar_t Cvar;

		Socket.ReadString(Cvar.Name);
		Socket.ReadString(Cvar.Value);

		// Filter out important information for us to use, it'd be nicer to have
		// a launcher-side cvar implementation though
		if (Cvar.Name == "sv_hostname")
		{
			Info.Name = Cvar.Value;

			continue;
		}
		else if (Cvar.Name == "sv_maxplayers")
		{
			Info.MaxPlayers = (uint8_t)atoi(Cvar.Value.c_str());

			continue;
		}
		else if (Cvar.Name == "sv_maxclients")
		{
			Info.MaxClients = (uint8_t)atoi(Cvar.Value.c_str());

			continue;
		}
		else if (Cvar.Name == "sv_gametype")
		{
			Info.GameType = (GameType_t)atoi(Cvar.Value.c_str());

			continue;
		}
		else if (Cvar.Name == "sv_scorelimit")
		{
			Info.ScoreLimit = atoi(Cvar.Value.c_str());

			continue;
		}

		Info.Cvars.push_back(Cvar);
	}

	Socket.ReadString(Info.PasswordHash);
	Socket.ReadString(Info.CurrentMap);
	Socket.Read16(Info.TimeLeft);

	// Teams
	uint8_t TeamCount;

	Socket.Read8(TeamCount);

	for (size_t i = 0; i < TeamCount; ++i)
	{
		Team_t Team;

		Socket.ReadString(Team.Name);
		Socket.Read32(Team.Colour);
		Socket.Read16(Team.Score);

		Info.Teams.push_back(Team);
	}

	// Dehacked/Bex files
	uint8_t PatchCount;

	Socket.Read8(PatchCount);

	for (size_t i = 0; i < PatchCount; ++i)
	{
		string Patch;

		Socket.ReadString(Patch);

		Info.Patches.push_back(Patch);
	}

	// Wad files
	uint8_t WadCount;

	Socket.Read8(WadCount);

	for (size_t i = 0; i < WadCount; ++i)
	{
		Wad_t Wad;

		Socket.ReadString(Wad.Name);
		Socket.ReadString(Wad.Hash);

		Info.Wads.push_back(Wad);
	}

	// Player information
	uint8_t PlayerCount;

	Socket.Read8(PlayerCount);

	for (size_t i = 0; i < PlayerCount; ++i)
	{
		Player_t Player;

		Socket.ReadString(Player.Name);
		Socket.Read8(Player.Team);
		Socket.Read16(Player.Ping);
		Socket.Read16(Player.Time);
		Socket.ReadBool(Player.Spectator);
		Socket.Read16(Player.Frags);
		Socket.Read16(Player.Kills);
		Socket.Read16(Player.Deaths);

		Info.Players.push_back(Player);
	}
}

//
// Server::TranslateResponse()
//
// Figures out the response from the server, deciding whether to use this data
// or not
int32_t Server::TranslateResponse(const uint16_t &TagId, 
                                  const uint8_t &TagApplication,
                                  const uint8_t &TagQRId,
                                  const uint16_t &TagPacketType)
{
	// It isn't a response
	if (TagQRId != 2)
	{
		//wxLogDebug(wxT("Query/Response Id is not valid"));

		return 0;
	}

	switch (TagApplication)
	{
		// Enquirer
		case 1:
			{
				//wxLogDebug(wxT("Application is Enquirer"));

				return 0;
			}
			break;

			// Client
		case 2:
			{
				//wxLogDebug(wxT("Application is Client"));

				return 0;
			}
			break;

			// Server
		case 3:
			{
				//wxLogDebug(wxT("Application is Server"));
			}
			break;

			// Master Server
		case 4:
			{
				//wxLogDebug(wxT("Application is Master Server"));

				return 0;
			}
			break;

			// Unknown
		default:
			{
				//wxLogDebug(wxT("Application is Unknown"));

				return 0;
			}
			break;
	}

	if (TagPacketType == 2)
	{
		// Launcher is an old version
		return 0;
	}

	uint32_t SvVersion;
	uint32_t SvProtocolVersion;

	Socket.Read32(SvVersion);
	Socket.Read32(SvProtocolVersion);

	if ((VERSIONMAJOR(SvVersion) < VERSIONMAJOR(VERSION)) || 
	    (VERSIONMAJOR(SvVersion) <= VERSIONMAJOR(VERSION) && VERSIONMINOR(SvVersion) < VERSIONMINOR(VERSION)))
	{
		// Server is an older version
		return 0;
	}

	ReadInformation(VERSIONMAJOR(SvVersion), 
	                VERSIONMINOR(SvVersion), 
	                VERSIONPATCH(SvVersion),
	                SvProtocolVersion);

	if (Socket.BadRead())
	{        
		// Bad packet data encountered
		return 0;
	}

	// Success
	return 1;
}

int32_t Server::Parse()
{   
	Socket.Read32(Info.Response);

	// Decode the tag into its fields
	// TODO: this may not be 100% correct
	uint16_t TagId = ((Info.Response >> 20) & 0x0FFF);
	uint8_t TagApplication = ((Info.Response >> 16) & 0x0F);
	uint8_t TagQRId = ((Info.Response >> 12) & 0x0F);
	uint16_t TagPacketType = (Info.Response & 0xFFFF0FFF);

	if (TagId == TAG_ID)
	{
		int32_t Result = TranslateResponse(TagId, 
		                                   TagApplication, 
		                                   TagQRId, 
		                                   TagPacketType);

		Socket.ClearBuffer();

		m_ValidResponse = Result ? true : false;

		return Result;
	}

	m_ValidResponse = false;

	Info.Response = 0;

	Socket.ClearBuffer();

	return 0;
}

int32_t Server::Query(int32_t Timeout)
{
	string Address = Socket.GetRemoteAddress();   

	if (Address != "")
	{
		Socket.ClearBuffer();

		ResetData();

		Socket.Write32(challenge);
		Socket.Write32(VERSION);
		Socket.Write32(PROTOCOL_VERSION);
		Socket.Write32(Info.PTime);

		if(!Socket.SendData(Timeout))
			return 0;

		if (!Socket.GetData(Timeout))
			return 0;

		Ping = Socket.GetPing();

		if (!Parse())
			return 0;

		return 1;
	}

	return 0;
}

} // namespace
