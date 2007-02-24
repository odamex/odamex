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
//	Low-level socket and buffer class
//	AUTHOR: Russell Rice (russell at odamex dot net)  
//
//-----------------------------------------------------------------------------


#ifndef NETIO_H
#define NETIO_H

#include <wx/socket.h>
#include <wx/mstream.h>
#include <wx/datstrm.h>
#include <wx/timer.h>
#include <wx/tokenzr.h>

#define MAX_PAYLOAD 8192 // don't make it 8192

class BufferedSocket
{
    private:        
        // the internal buffers, 2 for a reason
        wxMemoryInputStream  *recv_buf;
        wxMemoryOutputStream *send_buf;
        
        // Endianess switch for buffers
        static const wxByte BigEndian;
        
        // the socket
        wxDatagramSocket     *Socket;
        
        // local address
        wxIPV4address local_addr;
        
        // outgoing address (server)
        wxIPV4address       to_addr;
               
        wxInt32 SendPing, RecvPing;
        
        void SetSendPing(wxInt32 i) { SendPing = i; }
        void SetRecvPing(wxInt32 i) { RecvPing = i; }
        
        // we need to do something with this, one day
        void CheckError();
        
    public:
        BufferedSocket(); // Create a blank instance with stuff initialized

        virtual ~BufferedSocket(); // "Choose! Choose the form of the destructor!"
        
        // Set the outgoing address
        virtual void SetAddress(wxString Address, wxInt16 Port) { to_addr.Hostname(Address); to_addr.Service(Port); }
        //virtual wxInt32 SetAddress(wxString AddressAndPort);
        wxString    GetAddress(); // Get the outgoing address

        // Send/receive data using the outgoing address 
        wxInt32 SendData(wxInt32 Timeout);
        wxInt32 GetData(wxInt32 Timeout);
        
        // a method for a round-trip time in milliseconds
        inline wxInt32 GetPing() { return (RecvPing - SendPing); }
        
        // Read/Write values to the internal buffer
        wxChar      *ReadString();
        wxInt32     Read32();
        wxInt16     Read16();
        wxInt8      Read8();
        
        void    WriteString(wxString);
        void    Write32(wxInt32);
        void    Write16(wxInt16);
        void    Write8(wxInt8);
        
        // Reset buffer positions to 0
        void    ResetRecvBuffer() { recv_buf->SeekI(0, wxFromStart); } ;
        void    ResetSendBuffer() { send_buf->SeekO(0, wxFromStart); };
        void    ResetBuffers() { ResetRecvBuffer(); ResetSendBuffer(); };
        
        // Clear buffers
        void        ClearRecvBuffer();
        void        ClearSendBuffer();
        void ClearBuffers() { ClearRecvBuffer(); ClearSendBuffer(); };
        
               
};

#endif
