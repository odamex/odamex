// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
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
    Socket.SetAddress(to_addr.IPAddress(), to_addr.Service());
    
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
        return 0;
        
    wxInt16 server_count;
    
    Socket.Read16(server_count);
    
    if (!server_count)
        return 0;
    
    // don't delete our custom servers!
    std::vector<addr_t>::iterator addr_iter = addresses.begin();    
    
    while(addr_iter != addresses.end()) 
    {
        addr_t address = *addr_iter;
        
        if (address.custom == false)
        {
            addresses.erase(addr_iter);
            continue;
        }
        
        addr_iter++;
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
        
        addresses.push_back(address);
    }
    
    Socket.ClearRecvBuffer();
    
    return 1;
}

// Server constructor
Server::Server()
{   
    info.pwads = NULL;
    info.playerinfo = NULL;
    info.wad_hashes = NULL;
                
    ResetData();
}

Server::~Server()
{
    ResetData();
}

void Server::ResetData()
{
    // please keep this clean
    if (info.pwads != NULL)
    {
        delete[] info.pwads;
        
        info.pwads = NULL;
    }
        
    if (info.playerinfo != NULL)
    {
        delete[] info.playerinfo;
    
        info.playerinfo = NULL;
    }
    
    if (info.wad_hashes != NULL)
    {
        delete[] info.wad_hashes;
        
        info.wad_hashes = NULL;
    }

    // please keep this clean
    challenge = SERVER_CHALLENGE;
    response = SERVER_RESPONSE;
    
    info.response = 0;
    
    info.name = _T("");
    info.numplayers = 0;
	info.maxplayers = 0;
	info.map = _T("");
	info.numpwads = 0;
    
    info.iwad = _T("");
    info.pwads = NULL;

	info.gametype = -1;
	info.gameskill = 0;
	info.teamplay = false;

	info.playerinfo = NULL;
    info.wad_hashes = NULL;
    
	info.ctf = false;
	info.webaddr = _T("");
	
	info.teamplayinfo.scorelimit = 0;
	info.teamplayinfo.red = false;
	info.teamplayinfo.blue = false;
	info.teamplayinfo.gold = false;
	info.teamplayinfo.redscore = 0;
	info.teamplayinfo.bluescore = 0;
	info.teamplayinfo.goldscore = 0;
	
    info.spectating = 0;
    info.maxactiveplayers = 0;
    
    info.extrainfo = 0;
    info.passworded = false;
    
    Ping = 0;
}

wxInt32 Server::Parse()
{   
    Socket.Read32(info.response);
    
    if (info.response != response)
        return 0;
        
    int i = 0;
    wxInt32 dummyint;
    
    // token
    Socket.Read32(dummyint);
        
    Socket.ReadString(info.name);
    Socket.Read8(info.numplayers);
    Socket.Read8(info.maxplayers);
    Socket.ReadString(info.map);
    Socket.Read8(info.numpwads);
    Socket.ReadString(info.iwad);
        
    // needs to be fixed
        
    if (info.numpwads > 0)
    {          
        if (info.pwads != NULL)
        {
            delete[] info.pwads;
                
            info.pwads = new wxString [info.numpwads];
        }
        else
            info.pwads = new wxString [info.numpwads];
            
        if (info.pwads != NULL)
            for (i = 0; i < info.numpwads - 1; i++)
                Socket.ReadString(info.pwads[i]);
    }
        
    Socket.Read8(info.gametype);
    Socket.Read8(info.gameskill);
    Socket.ReadBool(info.teamplay); 
    Socket.ReadBool(info.ctf);
    
    // hack to enable teamplay if disabled and ctf is enabled
    info.teamplay |= info.ctf;
    
    if (info.numplayers > 0)
    {          
        if (info.playerinfo != NULL)
        {
            delete[] info.playerinfo;
                
            info.playerinfo = new player_t [info.numplayers];
        }
        else
            info.playerinfo = new player_t [info.numplayers];
            
        if (info.playerinfo != NULL)
            for (i = 0; i < info.numplayers; i++)
            {
                Socket.ReadString(info.playerinfo[i].name);
                Socket.Read16(info.playerinfo[i].frags);
                Socket.Read32(info.playerinfo[i].ping);
                Socket.Read8(info.playerinfo[i].team);
            }
    }
    
    Socket.ReadString(info.iwad_hash);
    
    if (info.numpwads > 0)
    {          
        if (info.wad_hashes != NULL)
        {
            delete[] info.wad_hashes;
                
            info.wad_hashes = new wxString [info.numpwads];
        }
        else
            info.wad_hashes = new wxString [info.numpwads];
            
        if (info.wad_hashes != NULL)
            for (i = 0; i < info.numpwads - 1; i++) // dummy to maintain compatibility for now
                Socket.ReadString(info.wad_hashes[i]);
    }
        
    Socket.ReadString(info.webaddr);
        
    if ((info.teamplay) || (info.ctf && info.teamplay))
    {
        Socket.Read32(info.teamplayinfo.scorelimit);
            
        Socket.ReadBool(info.teamplayinfo.blue);
        if (info.teamplayinfo.blue)
            Socket.Read32(info.teamplayinfo.bluescore);
            
        Socket.ReadBool(info.teamplayinfo.red);
        if (info.teamplayinfo.red)
            Socket.Read32(info.teamplayinfo.redscore);
            
        Socket.ReadBool(info.teamplayinfo.gold);
        if (info.teamplayinfo.gold)
             Socket.Read32(info.teamplayinfo.goldscore);
    }
    
    // version
    Socket.Read16(info.version);
    
    Socket.ReadString(info.emailaddr);
    Socket.Read16(info.timelimit);
    Socket.Read16(info.timeleft);
    Socket.Read16(info.fraglimit);

    Socket.ReadBool(info.itemrespawn);
    Socket.ReadBool(info.weaponstay);
    Socket.ReadBool(info.friendlyfire);
    Socket.ReadBool(info.allowexit);
    Socket.ReadBool(info.infiniteammo);
    Socket.ReadBool(info.nomonsters);
    Socket.ReadBool(info.monstersrespawn);
    Socket.ReadBool(info.fastmonsters);
    Socket.ReadBool(info.allowjump);
    Socket.ReadBool(info.allowfreelook);
    Socket.ReadBool(info.waddownload);
    Socket.ReadBool(info.emptyreset);
    Socket.ReadBool(info.cleanmaps);
    Socket.ReadBool(info.fragonexit);

    if ((info.numplayers) && (info.playerinfo != NULL))
    {
        for (i = 0; i < info.numplayers; i++)
        {
            Socket.Read16(info.playerinfo[i].killcount);
            Socket.Read16(info.playerinfo[i].deathcount);
            Socket.Read16(info.playerinfo[i].timeingame);
        }
    }

    Socket.Read32(info.spectating);

    if (info.spectating == 0x01020304)
    {
        Socket.Read16(info.maxactiveplayers);
        
        if ((info.numplayers) && (info.playerinfo != NULL))
        {
            for (i = 0; i < info.numplayers; ++i)
            {
                Socket.ReadBool(info.playerinfo[i].spectator);
            }
        }
    }
    else
        info.spectating = false;

    Socket.Read32(info.extrainfo);
    
    if (info.extrainfo == 0x01020305)
    {
        Socket.ReadBool(info.passworded);
    }

    Socket.ClearRecvBuffer();
    
    return 1;
}
