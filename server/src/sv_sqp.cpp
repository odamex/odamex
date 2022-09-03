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
//  Query protocol for server
//
//-----------------------------------------------------------------------------


#include "odamex.h"


#include "sv_sqp.h"

#include "d_main.h"
#include "d_player.h"
#include "md5.h"
#include "p_ctf.h"
#include "g_gametype.h"

static buf_t ml_message(MAX_UDP_PACKET);

EXTERN_CVAR(join_password)
EXTERN_CVAR(sv_timelimit)
EXTERN_CVAR(sv_teamsinplay)

struct CvarField_t
{
	std::string Name;
	std::string Value;
	cvartype_t Type;
} CvarField;

// The TAG identifier, changing this to a new value WILL break any application
// trying to contact this one that does not have the exact same value
#define TAG_ID 0xAD0

// When a change to the protocol is made, this value must be incremented
#define PROTOCOL_VERSION 7

/*
    Inclusion/Removal macros of certain fields, it is MANDATORY to remove these
    with every new major/minor version
*/

// Specifies when data was added to the protocol, the parameter is the
// introduced revision
// NOTE: this one is different from the launchers version for a reason
#define QRYNEWINFO(INTRODUCED) \
    if (EqProtocolVersion >= INTRODUCED || EqProtocolVersion >= PROTOCOL_VERSION)

// Specifies when data was removed from the protocol, first parameter is the
// introduced revision and the last one is the removed revision
#define QRYRANGEINFO(INTRODUCED,REMOVED) \
    if (EqProtocolVersion >= INTRODUCED && EqProtocolVersion < REMOVED)

//
// IntQryBuildInformation()
//
// Protocol building routine, the passed parameter is the enquirer version
static void IntQryBuildInformation(const DWORD& EqProtocolVersion,
                                   const DWORD& EqTime)
{
	std::vector<CvarField_t> Cvars;

	// bond - time
	MSG_WriteLong(&ml_message, EqTime);

	// The servers real protocol version
	// bond - real protocol
	MSG_WriteLong(&ml_message, PROTOCOL_VERSION);

	// Built revision of server
	// TODO: Remove guard before next release
	QRYNEWINFO(7)
	{
		// Send the detailed version - version number was in PROTOCOL_VERSION.
		MSG_WriteString(&ml_message, NiceVersionDetails());
	}
	else
	{
		MSG_WriteLong(&ml_message, -1);
	}

	cvar_t* var = GetFirstCvar();

	// Count our cvars and add them
	while(var)
	{
		if(var->flags() & CVAR_SERVERINFO)
		{
			CvarField.Name = var->name();
			CvarField.Type = var->type();
			CvarField.Value = var->cstring();

			// Skip empty strings
			if(CvarField.Type == CVARTYPE_STRING && var->cstring()[0] != '\0')
			{
				Cvars.push_back(CvarField);
				goto next;
			}

			// Skip other types with 0
			if(var->value() != 0.0f)
				Cvars.push_back(CvarField);
		}

next:
		var = var->GetNext();
	}

	// Cvar count
	MSG_WriteByte(&ml_message, (BYTE)Cvars.size());

	// Write cvars
	for(size_t i = 0; i < Cvars.size(); ++i)
	{
		MSG_WriteString(&ml_message, Cvars[i].Name.c_str());

		// Type field
		MSG_WriteByte(&ml_message, (byte)Cvars[i].Type);

		switch(Cvars[i].Type)
		{
		case CVARTYPE_BYTE:
		{
			MSG_WriteByte(&ml_message, (byte)atoi(Cvars[i].Value.c_str()));
		}
		break;

		case CVARTYPE_WORD:
		{
			MSG_WriteShort(&ml_message, (short)atoi(Cvars[i].Value.c_str()));
		}
		break;

		case CVARTYPE_INT:
		{
			MSG_WriteLong(&ml_message, (int)atoi(Cvars[i].Value.c_str()));
		}
		break;

		case CVARTYPE_FLOAT:
		case CVARTYPE_STRING:
		{
			MSG_WriteString(&ml_message, Cvars[i].Value.c_str());
		}
		break;

		case CVARTYPE_NONE:
		case CVARTYPE_MAX:
		default:
			break;
		}
	}

	MSG_WriteHexString(&ml_message, strlen(join_password.cstring()) ? MD5SUM(join_password.cstring()).c_str() : "");

	MSG_WriteString(&ml_message, level.mapname.c_str());

	int timeleft = (int)(sv_timelimit - level.time/(TICRATE*60));

	if(timeleft < 0)
		timeleft = 0;

	// TODO: Remove guard on next release and reset protocol version
	// TODO: Incorporate code above into block
	// Only send timeleft if sv_timelimit has been set
	QRYNEWINFO(6)
	{
		if (sv_timelimit.asInt())
			MSG_WriteShort(&ml_message, timeleft);
	}
	else
		MSG_WriteShort(&ml_message, timeleft);

	// Teams
	if(G_IsTeamGame())
	{
		// Team data
		int teams = sv_teamsinplay.asInt();
		MSG_WriteByte(&ml_message, teams);

		for (int i = 0; i < teams; i++)
		{
			TeamInfo* teamInfo = GetTeamInfo((team_t)i);
			MSG_WriteString(&ml_message, teamInfo->ColorString.c_str());
			MSG_WriteLong(&ml_message, teamInfo->Color);
			MSG_WriteShort(&ml_message, teamInfo->Points);
		}
	}

	// Patch files
	MSG_WriteByte(&ml_message, patchfiles.size());

	for(size_t i = 0; i < patchfiles.size(); ++i)
	{
		MSG_WriteString(&ml_message,
		                D_CleanseFileName(::patchfiles[i].getBasename()).c_str());
	}

	// Wad files
	MSG_WriteByte(&ml_message, wadfiles.size());

	for(size_t i = 0; i < wadfiles.size(); ++i)
	{
		MSG_WriteString(&ml_message,
		                D_CleanseFileName(::wadfiles[i].getBasename(), "wad").c_str());
		MSG_WriteHexString(&ml_message, ::wadfiles[i].getMD5().getHexCStr());
	}

	MSG_WriteByte(&ml_message, players.size());

	// Player info
	for(Players::iterator it = players.begin(); it != players.end(); ++it)
	{
		MSG_WriteString(&ml_message, it->userinfo.netname.c_str());

		for (int i = 3; i >= 0; i--)
			MSG_WriteByte(&ml_message, it->userinfo.color[i]);

		if(G_IsTeamGame())
			MSG_WriteByte(&ml_message, it->userinfo.team);

		MSG_WriteShort(&ml_message, it->ping);

		int timeingame = (time(NULL) - it->JoinTime) / 60;

		if(timeingame < 0)
			timeingame = 0;

		MSG_WriteShort(&ml_message, timeingame);

		// FIXME - Treat non-players (downloaders/others) as spectators too for now
		bool spectator;

		spectator = (it->spectator ||
					 ((it->playerstate != PST_LIVE) &&
					  (it->playerstate != PST_DEAD) &&
					  (it->playerstate != PST_REBORN)));

		MSG_WriteBool(&ml_message, spectator);

		MSG_WriteShort(&ml_message, it->fragcount);
		MSG_WriteShort(&ml_message, it->killcount);
		MSG_WriteShort(&ml_message, it->deathcount);
	}
}

//
// IntQrySendResponse()
//
// Sends information regarding the type of information we received (ie: it will
// send data that is wanted by the enquirer program)
static DWORD IntQrySendResponse(const WORD& TagId,
                                const BYTE& TagApplication,
                                const BYTE& TagQRId,
                                const WORD& TagPacketType)
{
	// It isn't a query, throw it away
	if(TagQRId != 1)
	{
		//Printf("Query/Response Id is not valid");

		return 0;
	}

	// Decipher the program that sent the query
	switch(TagApplication)
	{
	case 1:
	{
		//Printf(PRINT_HIGH, "Application is Enquirer");
	}
	break;

	case 2:
	{
		//Printf(PRINT_HIGH, "Application is Client");
	}
	break;

	case 3:
	{
		//Printf(PRINT_HIGH, "Application is Server");
	}
	break;

	case 4:
	{
		//Printf(PRINT_HIGH, "Application is Master Server");
	}
	break;

	default:
	{
		//Printf("Application is Unknown");
	}
	break;
	}

	DWORD ReTag = 0;
	WORD ReId = TAG_ID;
	BYTE ReApplication = 3;
	BYTE ReQRId = 2;
	WORD RePacketType = 0;

	switch(TagPacketType)
	{
	// Request version
	case 1:
	{
		RePacketType = 1;
	}
	break;

	// Information request
	case 2:
	{
		RePacketType = 3;
	}
	break;
	}

	// Begin enquirer version translation
	DWORD EqVersion = MSG_ReadLong();
	DWORD EqProtocolVersion = MSG_ReadLong();
	DWORD EqTime = MSG_ReadLong();

	// Prevent possible divide by zero
	if(!EqVersion)
	{
		return 0;
	}

	// Override other packet types for older enquirer version response
	if (VERMAJ(EqVersion) < VERMAJ(GAMEVER))
	{
		RePacketType = 2;
	}

	// Encode our tag
	ReTag = (((ReId << 20) & 0xFFF00000) |
	         ((ReApplication << 16) & 0x000F0000) |
	         ((ReQRId << 12) & 0x0000F000) | (RePacketType & 0x00000FFF));

	// Clear our message buffer for a response
	SZ_Clear(&ml_message);

	MSG_WriteLong(&ml_message, ReTag);
	MSG_WriteLong(&ml_message, GAMEVER);

	// Enquirer requested the version info of the server or the server
	// determined it is an old version
	if(RePacketType == 1 || RePacketType == 2)
	{
		// bond - real protocol
		MSG_WriteLong(&ml_message, PROTOCOL_VERSION);

		MSG_WriteLong(&ml_message, EqTime);

		NET_SendPacket(ml_message, net_from);

		//Printf(PRINT_HIGH, "Application is old version\n");

		return 0;
	}

	// If the enquirer protocol is newer, send the latest information the
	// server supports, otherwise send information built to the their version
	if(EqProtocolVersion > PROTOCOL_VERSION)
	{
		EqProtocolVersion = PROTOCOL_VERSION;

		MSG_WriteLong(&ml_message, PROTOCOL_VERSION);
	}
	else
		MSG_WriteLong(&ml_message, EqProtocolVersion);

	IntQryBuildInformation(EqProtocolVersion, EqTime);

	NET_SendPacket(ml_message, net_from);

	//Printf(PRINT_HIGH, "Success, data sent\n");

	return 0;
}

//
// SV_QryParseEnquiry()
//
// This decodes the Tag field
DWORD SV_QryParseEnquiry(const DWORD& Tag)
{
	// Decode the tag into its fields
	// TODO: this may not be 100% correct
	WORD TagId = ((Tag >> 20) & 0x0FFF);
	BYTE TagApplication = ((Tag >> 16) & 0x0F);
	BYTE TagQRId = ((Tag >> 12) & 0x0F);
	WORD TagPacketType = (Tag & 0xFFFF0FFF);

	// It is not ours
	if(TagId != TAG_ID)
	{
		return 1;
	}

	return IntQrySendResponse(TagId, TagApplication, TagQRId, TagPacketType);
}

VERSION_CONTROL(sv_sqp_cpp, "$Id$")
