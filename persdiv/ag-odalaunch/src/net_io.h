// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2012 by The Odamex Team.
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
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#ifndef NET_IO_H
#define NET_IO_H

#ifdef _XBOX
	#include <xtl.h>
#elif _WIN32
    #include <windows.h>
    #include <winsock.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/wait.h>
    #include <netdb.h>
#endif

#include "typedefs.h"

/**
agOdalaunch namespace.

All code for the ag-odalaunch launcher is contained within the agOdalaunch
namespace.
*/
namespace agOdalaunch {

#ifndef _WIN32
typedef int SOCKET;
#endif

const size_t MAX_PAYLOAD = 8192;

typedef unsigned char byte;

class BufferedSocket
{
public:
	BufferedSocket(); // Constructor

	virtual ~BufferedSocket(); // "Choose! Choose the form of the destructor!"
                
	// Used for platforms such as windows (where you must initialize WSA before
	// using the sockets api)
	static bool InitializeSocketAPI();
	static void ShutdownSocketAPI();

	// Set the outgoing address
	void SetRemoteAddress(const std::string &Address, const int16_t &Port);
	// Set the outgoing address in "address:port" format
	bool SetRemoteAddress(const std::string &Address);
	// Gets the outgoing address
	void GetRemoteAddress(std::string &Address, uint16_t &Port) const;
	// Gets the outgoing address in "address:port" format
	std::string GetRemoteAddress() const;

	// Send/receive data
	int32_t SendData(const int32_t &Timeout);
	int32_t GetData(const int32_t &Timeout);
        
	// a method for a round-trip time in milliseconds
	uint32_t GetPing() { return (m_ReceivePing - m_SendPing); }
        
	// Read values
	bool ReadString(std::string &);
	bool ReadBool(bool &);
	// Signed reads
	bool Read32(int32_t &);
	bool Read16(int16_t &);
	bool Read8(int8_t &);
	// Unsigned reads
	bool Read32(uint32_t &);
	bool Read16(uint16_t &);
	bool Read8(uint8_t &);
        
	bool BadRead() { return m_BadRead; }
        
	// Write values
	bool WriteString(const std::string &);
	bool WriteBool(const bool &);
	// Signed writes
	bool Write32(const int32_t &);
	bool Write16(const int16_t &);
	bool Write8(const int8_t &);
	// Unsigned writes
	bool Write32(const uint32_t &);
	bool Write16(const uint16_t &);
	bool Write8(const uint8_t &);
        
	bool BadWrite() { return m_BadWrite; }
        
	// Can read or write X bytes to a buffer
	bool CanRead(const size_t &);
	bool CanWrite(const size_t &);

	// Reset Buffer
	void ResetBuffer() { m_BufferPos = 0; }

	// Clear buffer
	void ClearBuffer();

private:        
	bool CreateSocket();
	void DestroySocket();

	void ReportError(int line, const char *function, const char *fmt, ...);

	void SetSendPing(const uint32_t &i) { m_SendPing = i; }
	void SetRecvPing(const uint32_t &i) { m_ReceivePing = i; }
        
	// the socket buffer
	byte   *m_SocketBuffer;
	size_t  m_BufferSize;
	size_t  m_BufferPos;

	bool    m_BadRead;
	bool    m_BadWrite;
        
	// the socket
	SOCKET  m_Socket;
        
	// local address
	struct sockaddr_in m_LocalAddress;

	// outgoing address (server)
	struct sockaddr_in m_RemoteAddress;
               
	uint32_t m_SendPing, m_ReceivePing;
};

} // namespace

#endif
