// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Launcher packet structure file
//	AUTHOR:	Russell Rice (russell at odamex dot net)
//
//-----------------------------------------------------------------------------


#include "net_packet.h"

/*
    Create a packet to send, which in turn will
    receive data from the server
    
    NOTE: All packet classes derive from this, so if some
          packets require different data, you'll have
          to override this
*/
wxInt32 ServerBase::Query(wxInt32 Timeout)
{
    wxString Address = Socket.GetAddress();   
    
    if (Address != _T(""))
    {
        Socket.ClearSendBuffer();
        
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
wxInt32 MasterServer::Parse()
{

    // begin reading
    
    wxInt32 temp_response;
    
    Socket.Read32(temp_response);
    
    if (temp_response != response)
    {
        Socket.ClearRecvBuffer();
        
        return 0;
    }
    
    wxInt16 server_count;
    
    Socket.Read16(server_count);
    
    if (!server_count)
    {
        Socket.ClearRecvBuffer();
        
        return 0;
    }
       
    // Add on to any servers already in the list
    if (server_count)
    for (wxInt16 i = 0; i < server_count; i++)
    {
        addr_t address;
        wxUint8 ip1, ip2, ip3, ip4;
        
        Socket.Read8(ip1);
        Socket.Read8(ip2);
        Socket.Read8(ip3);
        Socket.Read8(ip4);   
        
        address.ip = wxString::Format(_T("%d.%d.%d.%d"), ip1, ip2, ip3, ip4);
            
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
        Socket.ClearRecvBuffer();
        
        return 0;
    }
    
    Socket.ClearRecvBuffer();
    
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
    Info.Cvars.clear();
    Info.Wads.clear();
    Info.Players.clear();
    
    Info.Response = 0;    
    Info.Name = wxT("");
    Info.MaxPlayers = 0;
    Info.GameType = GT_Cooperative;
    Info.PasswordHash = wxT("");
    Info.CurrentMap = wxT("");
    Info.TimeLeft = 0;
    Info.BlueScore = 0;
    Info.RedScore = 0;
    Info.GoldScore = 0;
    
    Ping = 0;
}

void Server::ReadInformation(const wxUint32 &ProtocolVersion)
{
    wxUint8 CvarCount;
    
    Socket.Read8(CvarCount);
    
    for (size_t i = 0; i < CvarCount; ++i)
    {
        Cvar_t Cvar;
        
        Socket.ReadString(Cvar.Name);
        Socket.ReadString(Cvar.Value);
        
        // These fields need to be filtered out for the sake of launcher
        // functionality
        if (Cvar.Name == wxT("hostname"))
        {
            Info.Name = Cvar.Value;
            
            continue;
        }
        else if (Cvar.Name == wxT("maxplayers"))
        {
            Info.MaxPlayers = (wxUint8)wxAtoi(Cvar.Value);
            
            continue;
        }
        else if (Cvar.Name == wxT("gametype"))
        {
            Info.GameType = (GameType_t)wxAtoi(Cvar.Value);
            
            continue;
        }
        else if (Cvar.Name == wxT("scorelimit"))
        {
            Info.ScoreLimit = (GameType_t)wxAtoi(Cvar.Value);
            
            continue;
        }
        
        Info.Cvars.push_back(Cvar);
    }
    
    Socket.ReadString(Info.PasswordHash);
    Socket.ReadString(Info.CurrentMap);
    Socket.Read16(Info.TimeLeft);
    Socket.Read16(Info.BlueScore);
    Socket.Read16(Info.RedScore);
    Socket.Read16(Info.GoldScore);
    
    wxUint8 WadCount;
    
    Socket.Read8(WadCount);
    
    for (size_t i = 0; i < WadCount; ++i)
    {
        Wad_t Wad;
        
        Socket.ReadString(Wad.Name);
        Socket.ReadString(Wad.Hash);
        
        Info.Wads.push_back(Wad);
    }
    
    wxUint8 PlayerCount;
    
    Socket.Read8(PlayerCount);
    
    for (size_t i = 0; i < PlayerCount; ++i)
    {
        Player_t Player;
        
        Socket.ReadString(Player.Name);
        Socket.Read16(Player.Frags);
        Socket.Read16(Player.Ping);
        Socket.Read8(Player.Team);
        Socket.Read16(Player.Kills);
        Socket.Read16(Player.Deaths);
        Socket.Read16(Player.Time);
        Socket.ReadBool(Player.Spectator);
        
        Info.Players.push_back(Player);
    }
}

wxInt32 Server::TranslateResponse(const wxUint16 &TagId, 
                                  const wxUint8 &TagApplication,
                                  const wxUint8 &TagQRId,
                                  const wxUint16 &TagPacketType)
{
    // It isn't a response
    if (TagQRId == 2)
    {
        wxLogDebug("Query/Response Id is Query");
        
        return 0;
    }

    switch (TagApplication)
    {
        case 1:
        {
            wxLogDebug("Application is Enquirer");
            
            return 0;
        }
        break;
        
        case 2:
        {
            wxLogDebug("Application is Client");
            
            return 0;
        }
        break;

        case 3:
        {
            wxLogDebug("Application is Server");
        }
        break;

        case 4:
        {
            wxLogDebug("Application is Master Server");
            
            return 0;
        }
        break;
        
        default:
        {
            wxLogDebug("Application is Unknown");
            
            return 0;
        }
        break;
    }

    if (TagPacketType == 2)
    {
        // Launcher is an old version
        return 0;
    }

    wxUint32 SvVersion;
    wxUint32 SvProtocolVersion;
    
    Socket.Read32(SvVersion);
    Socket.Read32(SvProtocolVersion);

    if ((VERSIONMAJOR(SvVersion) < VERSIONMAJOR(VERSION)) || 
        (VERSIONMAJOR(SvVersion) < VERSIONMAJOR(VERSION) && VERSIONMINOR(SvVersion) < VERSIONMINOR(VERSION)))
    {
        // Server is an older version
        return 0;
    }

    ReadInformation(SvProtocolVersion);

    if (Socket.BadRead())
    {       
        return 0;
    }
    
    return 1;
}

wxInt32 Server::Parse()
{   
    Socket.Read32(Info.Response);
    
    // Decode the tag into its fields
    // TODO: this may not be 100% correct
    wxUint16 TagId = ((Info.Response >> 20) & 0x0FFF);
    wxUint8 TagApplication = ((Info.Response >> 16) & 0x0F);
    wxUint8 TagQRId = ((Info.Response >> 12) & 0x0F);
    wxUint16 TagPacketType = (Info.Response & 0xFFFF0FFF);
    
    if (TagId == TAG_ID)
    {
        wxInt32 Result = TranslateResponse(TagId, 
                                           TagApplication, 
                                           TagQRId, 
                                           TagPacketType);
                                           
        Socket.ClearRecvBuffer();
        
        return Result;
    }
    
    Socket.ClearRecvBuffer();
    
    return 0;
}

wxInt32 Server::Query(wxInt32 Timeout)
{
    wxString Address = Socket.GetAddress();   
    
    if (Address != _T(""))
    {
        Socket.ClearSendBuffer();
        
        Socket.Write32(challenge);
        Socket.Write32(VERSION);
        Socket.Write32(PROTOCOL_VERSION);
        
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
