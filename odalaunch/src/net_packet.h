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
//  AUTHOR:	Russell Rice (russell at odamex dot net)
//
//-----------------------------------------------------------------------------


#ifndef NET_PACKET_H
#define NET_PACKET_H

#include <wx/mstream.h>
#include <wx/datstrm.h>
#include <wx/log.h>

#include <vector>

#include "net_io.h"

#define MASTER_CHALLENGE    777123
#define MASTER_RESPONSE     777123
#define SERVER_CHALLENGE    0xAD011002

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

#define VERSION (0*256+41)
#define PROTOCOL_VERSION 9

#define TAG_ID 0xAD0

struct Cvar_t
{
    wxString Name;
    wxString Value;
};

struct Wad_t
{
    wxString Name;
    wxString Hash;
};

struct Player_t
{
    wxString Name;
    wxInt16 Frags;
    wxUint16 Ping;
    wxUint8 Team;
    wxUint16 Kills;
    wxUint16 Deaths;
    wxUint16 Time;
    bool Spectator;
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
    wxUint32 Response; // Launcher specific: Server response
    wxUint8 VersionMajor; // Launcher specific: Version fields
    wxUint8 VersionMinor;
    wxUint8 VersionPatch;
    wxUint32 VersionProtocol;
    wxString Name; // Launcher specific: Server name
    wxUint8 MaxClients; // Launcher specific: Maximum clients
    wxUint8 MaxPlayers; // Launcher specific: Maximum players
    GameType_t GameType; // Launcher specific: Game type
    wxUint16 ScoreLimit; // Launcher specific: Score limit
    std::vector<Cvar_t> Cvars;
    wxString PasswordHash;
    wxString CurrentMap;
    wxUint16 TimeLeft;
    wxInt16 BlueScore;
    wxInt16 RedScore;
    wxInt16 GoldScore;
    std::vector<Wad_t> Wads;
    std::vector<Player_t> Players;
};

class ServerBase  // [Russell] - Defines an abstract class for all packets
{
    protected:      
        BufferedSocket Socket;
        
        // Magic numbers
        wxUint32 challenge;
        wxUint32 response;
           
        // The time in milliseconds a packet was received
        wxUint32 Ping;
    public:
        // Constructor
        ServerBase() 
        {
            Ping = 0;           
        }
        
        // Destructor
        virtual ~ServerBase()
        {
        }
        
        // Parse a packet, the parameter is the packet
        virtual wxInt32 Parse() { return -1; }
        
        // Query the server
        wxInt32 Query(wxInt32 Timeout);
        
		void SetAddress(const wxString &Address, const wxInt16 &Port) 
		{ 
		    Socket.SetRemoteAddress(Address, Port);
        }
        
		wxString GetAddress() { return Socket.GetRemoteAddress(); }
		wxUint32 GetPing() const { return Ping; }
};

class MasterServer : public ServerBase  // [Russell] - A master server packet
{
    private:
        // Address format structure
        typedef struct
        {
            wxString    ip;
            wxUint16    port;
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
                              wxString &Address, 
                              wxUint16 &Port)
        {
            if (Index < addresses.size())
            {
                Address = addresses[Index].ip;
                Port = addresses[Index].port;
                
                return addresses[Index].custom;
            }
            
            return false;
        }
        
        void AddMaster(const wxString &Address, const wxUint16 &Port)
        {
            addr_t Master = { Address, Port, true };
            
            if ((Master.ip != _T("")) && (Master.port != 0))
                masteraddresses.push_back(Master);
        }
        
        void QueryMasters(const wxUint32 &Timeout)
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
        
        void AddCustomServer(const wxString &Address, const wxUint16 &Port)
        {
            addr_t cs;
                    
            cs.ip = Address;
            cs.port = Port;
            cs.custom = true;
            
            // Don't add the same address more than once.
            for (wxUint32 i = 0; i < addresses.size(); ++i)
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
        
        wxInt32 Parse();
};

class Server : public ServerBase  // [Russell] - A single server
{           
   
    public:
        ServerInfo_t Info;
        
        Server();
        
        void ResetData();
        
        virtual  ~Server();
        
        wxInt32 Query(wxInt32 Timeout);
        
        void ReadInformation(const wxUint8 &VersionMajor, 
                             const wxUint8 &VersionMinor,
                             const wxUint8 &VersionPatch,
                             const wxUint32 &ProtocolVersion);
        
        wxInt32 TranslateResponse(const wxUint16 &TagId, 
                                  const wxUint8 &TagApplication,
                                  const wxUint8 &TagQRId,
                                  const wxUint16 &TagPacketType);

        wxInt32 Parse();
};

#endif // NETPACKET_H
