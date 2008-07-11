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
//	Low-level socket and buffer class
//	AUTHOR: Russell Rice (russell at odamex dot net)  
//
//-----------------------------------------------------------------------------


#ifndef NET_IO_H
#define NET_IO_H

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
        
        wxChar sData[MAX_PAYLOAD];
        wxChar rData[MAX_PAYLOAD];
        
        // Endianess switch for buffers
        static const wxByte BigEndian;
        
        // the socket
        wxDatagramSocket     *Socket;
        
        // local address
        wxIPV4address local_addr;
        
        // outgoing address (server)
        wxIPV4address       to_addr;
               
        wxUint32 SendPing, RecvPing;
        
        void SetSendPing(wxUint32 i) { SendPing = i; }
        void SetRecvPing(wxUint32 i) { RecvPing = i; }
        
        // we need to do something with this, one day
        void CheckError();
        
        void CreateSocket(void);
        void DestroySocket(void);
    public:
        BufferedSocket(); // Create a blank instance with stuff initialized

        virtual ~BufferedSocket(); // "Choose! Choose the form of the destructor!"
                
        // Set the outgoing address
        void SetAddress(const wxString &Address, const wxInt16 &Port) 
        { 
            to_addr.Hostname(Address); to_addr.Service(Port); 
        }
        // Sets the outgoing address in "address:port" format
        bool SetAddress(const wxString &Address);

        wxString    GetAddress(); // Get the outgoing address

        // Send/receive data using the outgoing address 
        wxInt32 SendData(wxInt32 Timeout);
        wxInt32 GetData(wxInt32 Timeout);
        
        // a method for a round-trip time in milliseconds
        wxUint32 GetPing() { return (RecvPing - SendPing); }
        
        // Read values
        wxInt32     ReadString(wxString &);
        wxInt32     ReadBool(bool &boolval);
        // Signed reads
        wxInt32     Read32(wxInt32 &);
        wxInt32     Read16(wxInt16 &);
        wxInt32     Read8(wxInt8 &);
        // Unsigned reads
        wxInt32     Read32(wxUint32 &);
        wxInt32     Read16(wxUint16 &);
        wxInt32     Read8(wxUint8 &);
        
        // Write values
        void    WriteString(const wxString &);
        void    WriteBool(const bool &val);
        // Signed writes
        void    Write32(const wxInt32 &);
        void    Write16(const wxInt16 &);
        void    Write8(const wxInt8 &);
        // Unsigned writes
        void    Write32(const wxUint32 &);
        void    Write16(const wxUint16 &);
        void    Write8(const wxUint8 &);
        
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
