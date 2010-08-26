// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2010 by The Odamex Team.
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
//
// AUTHORS: 
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at huntsvegas dot org)
//
//-----------------------------------------------------------------------------


#ifndef NET_IO_H
#define NET_IO_H

#include <wx/socket.h>
#include <wx/mstream.h>
#include <wx/datstrm.h>
#include <wx/timer.h>
#include <wx/tokenzr.h>

#ifdef __WXMSW__
    #include <windows.h>
    #include <winsock.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/wait.h>
    #include <netdb.h>
#endif

#define MAX_PAYLOAD 8192

// Used for platforms such as windows (where you must initialize WSA before
// using the sockets api)
bool InitializeSocketAPI();
void ShutdownSocketAPI();

class BufferedSocket
{
    private:        
        // the internal buffers, 2 for a reason
        void *m_ReceiveBuffer;
       	void *m_SendBuffer;
        wxMemoryInputStream *m_ReceiveBufferHandler;
        wxMemoryOutputStream *m_SendBufferHandler;

        bool m_BadRead;
        bool m_BadWrite;
        
        // Endianess switch for buffers
        static const wxByte BigEndian;
        
        // the socket
        int m_Socket;
        
        // local address
        struct sockaddr_in m_LocalAddress;

        // outgoing address (server)
        struct sockaddr_in m_RemoteAddress;
               
        wxUint32 m_SendPing, m_ReceivePing;
        
        void SetSendPing(const wxUint32 &i) { m_SendPing = i; }
        void SetRecvPing(const wxUint32 &i) { m_ReceivePing = i; }
        
        void CheckError();
        
        bool CreateSocket();
        void DestroySocket();
    public:
        BufferedSocket(); // Create a blank instance with stuff initialized

        virtual ~BufferedSocket(); // "Choose! Choose the form of the destructor!"
                
        // Set the outgoing address
        void SetRemoteAddress(const wxString &Address, const wxInt16 &Port);
        // Set the outgoing address in "address:port" format
        bool SetRemoteAddress(const wxString &Address);
        // Gets the outgoing address
        void GetRemoteAddress(wxString &Address, wxUint16 &Port) const;
        // Gets the outgoing address in "address:port" format
        wxString GetRemoteAddress() const;

        // Send/receive data
        wxInt32 SendData(const wxInt32 &Timeout);
        wxInt32 GetData(const wxInt32 &Timeout);
        
        // a method for a round-trip time in milliseconds
        wxUint32 GetPing() { return (m_ReceivePing - m_SendPing); }
        
        // Read values
        bool ReadString(wxString &);
        bool ReadBool(bool &);
        // Signed reads
        bool Read32(wxInt32 &);
        bool Read16(wxInt16 &);
        bool Read8(wxInt8 &);
        // Unsigned reads
        bool Read32(wxUint32 &);
        bool Read16(wxUint16 &);
        bool Read8(wxUint8 &);
        
        bool BadRead() { return m_BadRead; }
        
        // Write values
        bool WriteString(const wxString &);
        bool WriteBool(const bool &);
        // Signed writes
        bool Write32(const wxInt32 &);
        bool Write16(const wxInt16 &);
        bool Write8(const wxInt8 &);
        // Unsigned writes
        bool Write32(const wxUint32 &);
        bool Write16(const wxUint16 &);
        bool Write8(const wxUint8 &);
        
        bool BadWrite() { return m_BadWrite; }
        
        // Reset buffer positions to 0
        void ResetRecvBuffer() { m_ReceiveBufferHandler->SeekI(0, wxFromStart); } ;
        void ResetSendBuffer() { m_SendBufferHandler->SeekO(0, wxFromStart); };
        void ResetBuffers() { ResetRecvBuffer(); ResetSendBuffer(); };
        
        // Can read or write X bytes to a buffer
        bool CanRead(const size_t &);
        bool CanWrite(const size_t &);
        
        // Clear buffers
        void ClearRecvBuffer();
        void ClearSendBuffer();
        void ClearBuffers() { ClearRecvBuffer(); ClearSendBuffer(); };
};

#endif
