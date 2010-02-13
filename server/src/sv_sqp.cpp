// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2009 by The Odamex Team.
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
//	Query protocol for server
//
//-----------------------------------------------------------------------------

#include <string>
#include <vector>

#include "sv_sqp.h"

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

extern std::vector<std::string> patchfiles, wadnames, wadhashes;
static buf_t ml_message(MAX_UDP_PACKET);

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

EXTERN_CVAR (teamsinplay)

EXTERN_CVAR (maxplayers)
EXTERN_CVAR (password)
EXTERN_CVAR (website)

EXTERN_CVAR (natport)

struct CvarField_t 
{ 
    std::string Name;
    std::string Value;
} CvarField;

// The TAG identifier, changing this to a new value WILL break any application 
// trying to contact this one that does not have the exact same value
#define TAG_ID 0xAD0

// When a change to the protocol is made, this value must be incremented
#define PROTOCOL_VERSION 1

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
static void IntQryBuildInformation(const DWORD &EqProtocolVersion)
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
    
    // Cvar count
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
    
    // Team data
    MSG_WriteByte(&ml_message, 2);
    
    // Blue
    MSG_WriteString(&ml_message, "Blue");
    MSG_WriteLong(&ml_message, 0x000000FF);
    MSG_WriteShort(&ml_message, (short)TEAMpoints[it_blueflag]);

    MSG_WriteString(&ml_message, "Red");
    MSG_WriteLong(&ml_message, 0xFF000000);
    MSG_WriteShort(&ml_message, (short)TEAMpoints[it_redflag]);

    // TODO: When real dynamic teams are implemented
    //byte TeamCount = (byte)teamsinplay;
    //MSG_WriteByte(&ml_message, TeamCount);
    
    //for (byte i = 0; i < TeamCount; ++i)
    //{
        // TODO - Figure out where the info resides
        //MSG_WriteString(&ml_message, "");
        //MSG_WriteLong(&ml_message, 0);
        //MSG_WriteShort(&ml_message, TEAMpoints[i]);        
    //}

	// Patch files	
	MSG_WriteByte(&ml_message, patchfiles.size());
	
	for (size_t i = 0; i < patchfiles.size(); ++i)
	{
        MSG_WriteString(&ml_message, patchfiles[i].c_str());
	}
	
	// Wad files
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
            MSG_WriteByte(&ml_message, players[i].userinfo.team);
			MSG_WriteShort(&ml_message, players[i].ping);

			int timeingame = (time(NULL) - players[i].JoinTime)/60;
			if (timeingame < 0) 
                timeingame = 0;
			MSG_WriteShort(&ml_message, timeingame);

            MSG_WriteBool(&ml_message, players[i].spectator);
            MSG_WriteShort(&ml_message, players[i].fragcount);
			MSG_WriteShort(&ml_message, players[i].killcount);
			MSG_WriteShort(&ml_message, players[i].deathcount);		
        }
    }
}

//
// IntQrySendResponse()
// 
// Sends information regarding the type of information we received (ie: it will
// send data that is wanted by the enquirer program)
static DWORD IntQrySendResponse(const WORD &TagId, 
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
        (VERSIONMAJOR(EqVersion) <= VERSIONMAJOR(GAMEVER) && VERSIONMINOR(EqVersion) < VERSIONMINOR(GAMEVER)))
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
    
    // Enquirer is an old version
    if (RePacketType == 2)
    {       
        MSG_WriteLong(&ml_message, PROTOCOL_VERSION);

        NET_SendPacket(ml_message, net_from);
        
        //Printf(PRINT_HIGH, "Application is old version\n");
        
        return 0;
    }
    
    // If the enquirer protocol is newer, send the latest information the
    // server supports, otherwise send information built to the their version
    if (EqProtocolVersion > PROTOCOL_VERSION)
    {
        EqProtocolVersion = PROTOCOL_VERSION;
        
        MSG_WriteLong(&ml_message, PROTOCOL_VERSION);
    }
    else
        MSG_WriteLong(&ml_message, EqProtocolVersion);
    
    IntQryBuildInformation(EqProtocolVersion);
    
    NET_SendPacket(ml_message, net_from);

    //Printf(PRINT_HIGH, "Success, data sent\n");

    return 0;
}

//
// SV_QryParseEnquiry()
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

VERSION_CONTROL (sv_sqp_cpp, "$Id$")
