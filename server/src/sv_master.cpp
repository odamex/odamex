// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62).
// Copyright (C) 2006-2008 by The Odamex Team.
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
//	SV_MASTER
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomstat.h"
#include "d_player.h"
#include "p_local.h"
#include "sv_main.h"
#include "sv_master.h"
#include "c_console.h"
#include "c_dispatch.h"
#include "i_system.h"
#include "md5.h"
#include "sv_ctf.h"

#define MASTERPORT			15000

// [Russell] - default master list
// This is here for complete master redundancy, including domain name failure
const char *def_masterlist[] = { "master1.odamex.net", "voxelsoft.com", NULL };

class masterserver
{
public:
	std::string	masterip;
	netadr_t	masteraddr; // address of the master server

	masterserver()
	{
	}
	
	masterserver(const masterserver &other)
		: masterip(other.masterip), masteraddr(other.masteraddr)
	{
	}

	masterserver& operator =(const masterserver &other)
	{
		masterip = other.masterip;
		masteraddr = other.masteraddr;
		return *this;
	}
};

EXTERN_CVAR (usemasters)
EXTERN_CVAR (hostname)
EXTERN_CVAR (maxclients)

EXTERN_CVAR (port)

//bond===========================
EXTERN_CVAR (timelimit)			
EXTERN_CVAR (fraglimit)			
EXTERN_CVAR (email)
EXTERN_CVAR (itemsrespawn)
EXTERN_CVAR (weaponstay)
EXTERN_CVAR (friendlyfire)
EXTERN_CVAR (allowexit)
EXTERN_CVAR (infiniteammo)
EXTERN_CVAR (nomonsters)
EXTERN_CVAR (monstersrespawn)
EXTERN_CVAR (fastmonsters)
EXTERN_CVAR (allowjump)
EXTERN_CVAR (sv_freelook)
EXTERN_CVAR (waddownload)
EXTERN_CVAR (emptyreset)
EXTERN_CVAR (cleanmaps)
EXTERN_CVAR (fragexitswitch)
//bond===========================

EXTERN_CVAR (maxplayers)
EXTERN_CVAR (password)
EXTERN_CVAR (website)

EXTERN_CVAR (natport)

buf_t     ml_message(MAX_UDP_PACKET);

std::vector<std::string> wadnames, wadhashes;
static std::vector<masterserver> masters;

//
// SV_InitMaster
//
void SV_InitMasters(void)
{
	if (!usemasters)
		Printf(PRINT_HIGH, "Masters will not be contacted because usemasters is 0\n");
    else
    {
        // [Russell] - Add some default masters
        // so we can dump them to the server cfg file if
        // one does not exist
        if (!masters.size())
		{
			int i = 0;
            while(def_masterlist[i])
                SV_AddMaster(def_masterlist[i++]);
		}
    }
}

//
// SV_AddMaster
//
bool SV_AddMaster(const char *masterip)
{
	if(strlen(masterip) >= MAX_UDP_PACKET)
		return false;

	masterserver m;
	m.masterip = masterip;

	NET_StringToAdr (m.masterip.c_str(), &m.masteraddr);

	if(!m.masteraddr.port)
		I_SetPort(m.masteraddr, MASTERPORT);

	for(size_t i = 0; i < masters.size(); i++)
	{
		if(masters[i].masterip == m.masterip)
		{
			Printf(PRINT_MEDIUM, "Master %s [%s] is already on the list", m.masterip.c_str(), NET_AdrToString(m.masteraddr));
			return false;
		}
	}
	
	if(m.masteraddr.ip[0] == 0 && m.masteraddr.ip[1] == 0 && m.masteraddr.ip[2] == 0 && m.masteraddr.ip[3] == 0)
	{
		Printf(PRINT_MEDIUM, "Failed to resolve master server: %s, not added", m.masterip.c_str(), NET_AdrToString(m.masteraddr));
		return false;
	}
	else
	{
		Printf(PRINT_MEDIUM, "Added master: %s [%s]", m.masterip.c_str(), NET_AdrToString(m.masteraddr));
		masters.push_back(m);
	}

	return true;
}

//
// SV_ArchiveMasters
//
void SV_ArchiveMasters(FILE *fp)
{
	for(size_t i = 0; i < masters.size(); i++)
		fprintf(fp, "addmaster %s\n", masters[i].masterip.c_str());
}

//
// SV_ListMasters
//
void SV_ListMasters(void)
{
	Printf(PRINT_MEDIUM, "Use addmaster/delmaster commands to modify this list");

	for(size_t index = 0; index < masters.size(); index++)
		Printf(PRINT_MEDIUM, "%s [%s]", masters[index].masterip.c_str(), NET_AdrToString(masters[index].masteraddr));
}

//
// SV_RemoveMaster
//
bool SV_RemoveMaster(const char *masterip)
{
	for(size_t index = 0; index < masters.size(); index++)
	{
		if(strnicmp(masters[index].masterip.c_str(), masterip, strlen(masterip)) == 0)
		{
			Printf(PRINT_MEDIUM, "Removed master server: %s", masters[index].masterip.c_str());
			masters.erase(masters.begin() + index);
			return true;
		}
	}

	Printf(PRINT_MEDIUM, "Failed to remove master: %s, not in list", masterip);
	return false;
}

//
// SV_UpdateMasterServer
//
void SV_UpdateMasterServer(masterserver &m)
{
		SZ_Clear(&ml_message);
		MSG_WriteLong(&ml_message, CHALLENGE);

		// send out actual port, because NAT may present an incorrect port to the master
		if(natport)
			MSG_WriteShort(&ml_message, natport);
		else
			MSG_WriteShort(&ml_message, port);

		NET_SendPacket(ml_message, m.masteraddr);
}

//
// SV_UpdateMasterServers
//
void SV_UpdateMasterServers(void)
{
	for(size_t index = 0; index < masters.size(); index++)
		SV_UpdateMasterServer(masters[index]);
}

//
// SV_UpdateMaster
//
void SV_UpdateMaster(void)
{
	if (!usemasters)
		return;

	// update master addresses from names every 3 hours
	if ((gametic % (TICRATE*60*60*3) ) == 0)
	{
		for(size_t index = 0; index < masters.size(); index++)
		{
			NET_StringToAdr (masters[index].masterip.c_str(), &masters[index].masteraddr);
			I_SetPort(masters[index].masteraddr, MASTERPORT);
		}
	}

	// Send to masters every 25 seconds
	if (gametic % (TICRATE*25))
		return;

	SV_UpdateMasterServers();
}

//
// denis - each launcher reply contains a random token so that
// the server will only allow connections with a valid token
// in order to protect itself from ip spoofing
//
struct token_t
{
	DWORD id;
	QWORD issued;
	netadr_t from;
};

#define MAX_TOKEN_AGE	(20 * TICRATE) // 20s should be enough for any client to load its wads
static std::vector<token_t> connect_tokens;

//
// SV_NewToken
//
DWORD SV_NewToken()
{
	QWORD now = I_GetTime();

	token_t token;
	token.id = rand()*time(0);
	token.issued = now;
	token.from = net_from;
	
	// find an old token to replace
	for(size_t i = 0; i < connect_tokens.size(); i++)
	{
		if(now - connect_tokens[i].issued >= MAX_TOKEN_AGE)
		{
			connect_tokens[i] = token;
			return token.id;
		}
	}

	connect_tokens.push_back(token);

	return token.id;
}

//
// SV_ValidToken
//
bool SV_IsValidToken(DWORD token)
{
	QWORD now = I_GetTime();

	for(size_t i = 0; i < connect_tokens.size(); i++)
	{
		if(connect_tokens[i].id == token
		&& NET_CompareAdr(connect_tokens[i].from, net_from)
		&& now - connect_tokens[i].issued < MAX_TOKEN_AGE)
		{
			// extend token life and confirm
			connect_tokens[i].issued = now;
			return true;
		}
	}
	
	return false;
}

//
// SV_SendServerInfo
// 
// Sends server info to a launcher
// TODO: Clean up and reinvent.
void SV_SendServerInfo()
{
	size_t i;

	SZ_Clear(&ml_message);
	
	MSG_WriteLong(&ml_message, CHALLENGE);
	MSG_WriteLong(&ml_message, SV_NewToken());

	// if master wants a key to be presented, present it we will
	if(MSG_BytesLeft() == 4)
		MSG_WriteLong(&ml_message, MSG_ReadLong());

	MSG_WriteString(&ml_message, (char *)hostname.cstring());

	byte playersingame = 0;
	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
			playersingame++;
	}

	MSG_WriteByte(&ml_message, playersingame);
	MSG_WriteByte(&ml_message, maxclients);

	MSG_WriteString(&ml_message, level.mapname);

	size_t numwads = wadnames.size();
	if(numwads > 0xff)numwads = 0xff;

	MSG_WriteByte(&ml_message, numwads - 1);

	for (i = 1; i < numwads; ++i)
		MSG_WriteString(&ml_message, wadnames[i].c_str());

	MSG_WriteBool(&ml_message, (deathmatch ? true : false));
	MSG_WriteByte(&ml_message, (BYTE)skill);
    MSG_WriteBool(&ml_message, (teamplay ? true : false));
	MSG_WriteBool(&ml_message, (ctfmode ? true : false));

	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
		{
			MSG_WriteString(&ml_message, players[i].userinfo.netname);
			MSG_WriteShort(&ml_message, players[i].fragcount);
			MSG_WriteLong(&ml_message, players[i].ping);

			if (teamplay || ctfmode)
				MSG_WriteByte(&ml_message, players[i].userinfo.team);
			else
				MSG_WriteByte(&ml_message, TEAM_NONE);
		}
	}

	for (i = 1; i < numwads; ++i)
		MSG_WriteString(&ml_message, wadhashes[i].c_str());

	MSG_WriteString(&ml_message, website.cstring());

	if (ctfmode || teamplay)
	{
		MSG_WriteLong(&ml_message, scorelimit);
		
		for(size_t i = 0; i < NUMTEAMS; i++)
		{
			MSG_WriteBool(&ml_message, (TEAMenabled[i] ? true : false));

			if (TEAMenabled[i])
				MSG_WriteLong(&ml_message, TEAMpoints[i]);
		}
	}
	
	MSG_WriteShort(&ml_message, VERSION);

//bond===========================
	MSG_WriteString(&ml_message, (char *)email.cstring());

	int timeleft = (int)(timelimit - level.time/(TICRATE*60));
	if (timeleft<0) timeleft=0;

	MSG_WriteShort(&ml_message,(int)timelimit);
	MSG_WriteShort(&ml_message,timeleft);
	MSG_WriteShort(&ml_message,(int)fraglimit);

	MSG_WriteBool(&ml_message, (itemsrespawn ? true : false));
	MSG_WriteBool(&ml_message, (weaponstay ? true : false));
	MSG_WriteBool(&ml_message, (friendlyfire ? true : false));
	MSG_WriteBool(&ml_message, (allowexit ? true : false));
	MSG_WriteBool(&ml_message, (infiniteammo ? true : false));
	MSG_WriteBool(&ml_message, (nomonsters ? true : false));
	MSG_WriteBool(&ml_message, (monstersrespawn ? true : false));
	MSG_WriteBool(&ml_message, (fastmonsters ? true : false));
	MSG_WriteBool(&ml_message, (allowjump ? true : false));
	MSG_WriteBool(&ml_message, (sv_freelook ? true : false));
	MSG_WriteBool(&ml_message, (waddownload ? true : false));
	MSG_WriteBool(&ml_message, (emptyreset ? true : false));
	MSG_WriteBool(&ml_message, (cleanmaps ? true : false));
	MSG_WriteBool(&ml_message, (fragexitswitch ? true : false));

	for (i = 0; i < players.size(); ++i)
	{
		if (players[i].ingame())
		{
			MSG_WriteShort(&ml_message, players[i].killcount);
			MSG_WriteShort(&ml_message, players[i].deathcount);
			
			int timeingame = (time(NULL) - players[i].JoinTime)/60;
			if (timeingame<0) timeingame=0;
			MSG_WriteShort(&ml_message, timeingame);
		}
	}
	
//bond===========================

    MSG_WriteLong(&ml_message, (DWORD)0x01020304);
    MSG_WriteShort(&ml_message, (WORD)maxplayers);
    
    for (i = 0; i < players.size(); ++i)
    {
        if (players[i].ingame())
        {
            MSG_WriteBool(&ml_message, (players[i].spectator ? true : false));
        }
    }

    MSG_WriteLong(&ml_message, (DWORD)0x01020305);
    MSG_WriteShort(&ml_message, strlen(password.cstring()) ? 1 : 0);
    
    // GhostlyDeath -- Send Game Version info
    MSG_WriteLong(&ml_message, GAMEVER);

	NET_SendPacket(ml_message, net_from);
}

#define TAG_ID 0xAD0
#define PROTOCOL_VERSION 1

struct CvarField_t 
{ 
    std::string Name;
    std::string Value;
} CvarField;

//
// void IntQryBuildInformation(const DWORD &ProtocolVersion)
//
// The place for the actual protocol builder
void IntQryBuildInformation(const DWORD &EqProtocolVersion)
{
    std::vector<CvarField_t> Cvars;

    cvar_t *var = GetFirstCvar();
    
    // Count our cvars and add them
    while (var)
    {
        if (var->flags() & CVAR_SERVERINFO)
        {
            CvarField.Name = var->name();
            CvarField.Value = var->cstring();
            
            Cvars.push_back(CvarField);
        }
        
        var = var->GetNext();
    }
    
    MSG_WriteByte(&ml_message, (BYTE)Cvars.size());
    
    // Write cvars
    for (size_t i = 0; i < Cvars.size(); ++i)
	{
        MSG_WriteString(&ml_message, Cvars[i].Name.c_str());
		MSG_WriteString(&ml_message, Cvars[i].Value.c_str());
	}
	
	MSG_WriteString(&ml_message, (strlen(password.cstring()) ? MD5SUM(password.cstring()).c_str() : ""));
	MSG_WriteString(&ml_message, level.mapname);
	
    int timeleft = (int)(timelimit - level.time/(TICRATE*60));
	if (timeleft < 0) 
        timeleft = 0;
        
    MSG_WriteShort(&ml_message, timeleft);
    
    MSG_WriteShort(&ml_message, TEAMpoints[it_blueflag]);
	MSG_WriteShort(&ml_message, TEAMpoints[it_redflag]);
	
	MSG_WriteByte(&ml_message, wadnames.size());
	
	for (size_t i = 0; i < wadnames.size(); ++i)
    {
        MSG_WriteString(&ml_message, wadnames[i].c_str());
        MSG_WriteString(&ml_message, wadhashes[i].c_str());
    }
    
    MSG_WriteByte(&ml_message, players.size());
    
    for (size_t i = 0; i < players.size(); ++i)
    {
        if (players[i].ingame())
        {
			MSG_WriteString(&ml_message, players[i].userinfo.netname);
			MSG_WriteShort(&ml_message, players[i].fragcount);
			MSG_WriteShort(&ml_message, players[i].ping);

			if (teamplay || ctfmode)
				MSG_WriteByte(&ml_message, players[i].userinfo.team);
			else
				MSG_WriteByte(&ml_message, TEAM_NONE);

			MSG_WriteShort(&ml_message, players[i].killcount);
			MSG_WriteShort(&ml_message, players[i].deathcount);
			
			int timeingame = (time(NULL) - players[i].JoinTime)/60;
			if (timeingame < 0) 
                timeingame = 0;
			
			MSG_WriteShort(&ml_message, timeingame);
            MSG_WriteBool(&ml_message, players[i].spectator);
        }
    }
}

//
// void IntQrySendResponse()
//
// 
DWORD IntQrySendResponse(const WORD &TagId, 
                        const BYTE &TagApplication,
                        const BYTE &TagQRId,
                        const WORD &TagPacketType)
{
    // It isn't a query, throw it away
    if (TagQRId == 2)
    {
        //Printf("Query/Response Id is Response");
        
        return 0;
    }

    // Decipher the program that sent the query
    switch (TagApplication)
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
    
    switch (TagPacketType)
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

    // Override other packet types for older enquirer version response
    if (VERSIONMAJOR(EqVersion) < VERSIONMAJOR(GAMEVER) || 
        (VERSIONMAJOR(EqVersion) < VERSIONMAJOR(GAMEVER) && VERSIONMINOR(EqVersion) < VERSIONMINOR(GAMEVER)))
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
    MSG_WriteLong(&ml_message, PROTOCOL_VERSION);
    
    // Enquirer is an old version
    if (RePacketType == 2)
    {       
        NET_SendPacket(ml_message, net_from);
        
        //Printf(PRINT_HIGH, "Application is old version\n");
        
        return 0;
    }
    
    IntQryBuildInformation(EqProtocolVersion);
    
    NET_SendPacket(ml_message, net_from);

    //Printf(PRINT_HIGH, "Success, data sent\n");

    return 0;
}

//
// void SV_QryParseEnquiry()
//
// This decodes the Tag field
DWORD SV_QryParseEnquiry(const DWORD &Tag)
{   
    // Decode the tag into its fields
    // TODO: this may not be 100% correct
    WORD TagId = ((Tag >> 20) & 0x0FFF);
    BYTE TagApplication = ((Tag >> 16) & 0x0F);
    BYTE TagQRId = ((Tag >> 12) & 0x0F);
    WORD TagPacketType = (Tag & 0xFFFF0FFF);
    
    // It is not ours 
    if (TagId != TAG_ID)
    {
        return 1;
    }
    
    return IntQrySendResponse(TagId, TagApplication, TagQRId, TagPacketType);
}


// Server appears in the server list when true.
CVAR_FUNC_IMPL (usemasters)
{
    SV_InitMasters();
}

BEGIN_COMMAND (addmaster)
{
	if (argc > 1)
	{
		SV_AddMaster(argv[1]);
	}
}
END_COMMAND (addmaster)

BEGIN_COMMAND (delmaster)
{
	if (argc > 1)
	{
		SV_RemoveMaster(argv[1]);
	}
}
END_COMMAND (delmaster)

BEGIN_COMMAND (masters)
{
	SV_ListMasters();
}
END_COMMAND (masters)


VERSION_CONTROL (sv_master_cpp, "$Id$")

