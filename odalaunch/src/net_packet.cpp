// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2006-2007 by The Odamex Team.
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

BufferedSocket ServerBase::Socket;

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
    
    wxInt32 temp_response = Socket.Read32();
    
    if (temp_response != response)
        return 0;
        
    // only free the array if the response is valid
    // the coder may still want the addresses for refreshing
    //if (&addresses != NULL)
    //    free(addresses);
        
    server_count = Socket.Read16();
    
    if (!server_count)
        return 0;
    
    // allocate an array with the amount of addresses
    addresses = (addr_t *)calloc(server_count, sizeof(addr_t));
        
    if (addresses == NULL)
        return 0;
    
    for (wxInt32 i = 0; i < server_count; i++)
    {
        addresses[i].ip[0] = Socket.Read8();
        addresses[i].ip[1] = Socket.Read8();
        addresses[i].ip[2] = Socket.Read8();
        addresses[i].ip[3] = Socket.Read8();
            
        addresses[i].port = Socket.Read16();
    }
    
    Socket.ClearRecvBuffer();
    
    return 1;
}

// Server constructor
Server::Server()
{   
    // please keep this clean
    challenge = SERVER_CHALLENGE;
    response = SERVER_RESPONSE;
    
    info.name = _T("");
    info.numplayers = 0;
	info.maxplayers = 0;
	info.map = _T("");
	info.numpwads = 0;
    
    info.iwad = _T("");
    info.pwads = NULL;

	info.gametype = 0;
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
}

Server::~Server()
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
}

wxInt32 Server::Parse()
{   
    wxInt32 temp_response = Socket.Read32();
    
    if (temp_response != response)
        return 0;
        
    int i = 0;
        
    // token
    Socket.Read32();
        
    info.name = Socket.ReadString();
    info.numplayers = Socket.Read8();
    info.maxplayers = Socket.Read8();
    info.map = Socket.ReadString();
    info.numpwads = Socket.Read8() - 1; // needs protocol fix, thanks anark
    info.iwad = Socket.ReadString();
        
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
            for (i = 0; i < info.numpwads; i++)
                info.pwads[i] = Socket.ReadString();
    }
        
    info.gametype = Socket.Read8();
    info.gameskill = Socket.Read8();
    info.teamplay = Socket.Read8() ? true : false;
    info.ctf = Socket.Read8() ? true : false;
        
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
                info.playerinfo[i].name = Socket.ReadString();
                info.playerinfo[i].frags = Socket.Read16();
                    info.playerinfo[i].ping = Socket.Read32();
                    info.playerinfo[i].team = Socket.Read8();
            }
    }
        
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
            for (i = 0; i < info.numpwads; i++) // dummy to maintain compatibility for now
                info.wad_hashes[i] = Socket.ReadString();
    }
        
    info.webaddr = Socket.ReadString();
        
    if ((info.teamplay) || (info.ctf && info.teamplay))
    {
        info.teamplayinfo.scorelimit = Socket.Read32();
            
        info.teamplayinfo.blue = Socket.Read8() ? true : false;
        if (info.teamplayinfo.blue)
            info.teamplayinfo.bluescore = Socket.Read32();
            
        info.teamplayinfo.red = Socket.Read8() ? true : false;
        if (info.teamplayinfo.red)
            info.teamplayinfo.redscore = Socket.Read32();
            
        info.teamplayinfo.gold = Socket.Read8() ? true : false;
        if (info.teamplayinfo.gold)
            info.teamplayinfo.goldscore = Socket.Read32();
    }
    
    // version
    Socket.Read16();
    
    Socket.ClearRecvBuffer();
    
    return 1;
}
