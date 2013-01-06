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

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <time.h>
#include <errno.h>

#include <agar/core.h>

#include "net_io.h"

#ifdef _XBOX
#include "xbox_main.h"
#endif

namespace agOdalaunch {

#ifndef _WIN32
#define closesocket close
const int INVALID_SOCKET = -1;
#endif

using namespace std;

BufferedSocket::BufferedSocket() :	m_BadRead(false), m_BadWrite(false),
									m_Socket(0), m_SendPing(0), m_ReceivePing(0)
{
	m_SocketBuffer = NULL;
	memset(&m_RemoteAddress, 0, sizeof(struct sockaddr_in));
	ClearBuffer();
}

BufferedSocket::~BufferedSocket()
{
	delete m_SocketBuffer;
}

// System-specific Initialize and shutdown functions
bool BufferedSocket::InitializeSocketAPI()
{
#ifdef _WIN32
	WSADATA wsaData;

	if(WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
		return false;
#endif

	return true;
}

void BufferedSocket::ShutdownSocketAPI()
{
#ifdef _WIN32
	WSACleanup();
#endif
}

void BufferedSocket::ReportError(int line, const char *function, const char *fmt, ...)
{
	va_list ap;

	if(!function || !fmt)
		return;

	va_start(ap, fmt);

#ifdef _XBOX
	char errorstr[1024];

	Xbox::OutputDebugString("[%s:%d] BufferedSocket::%s(): ", __FILE__, line, function);
	vsprintf(errorstr, fmt, ap);
	Xbox::OutputDebugString("%s\n", errorstr);
#else
	fprintf(stderr, "[%s:%d] BufferedSocket::%s(): ", __FILE__, line, function);
	vfprintf(stderr, fmt, ap);
	fputs("\n", stderr);
#endif // _XBOX

	va_end(ap);
}
       
bool BufferedSocket::CreateSocket()
{
	DestroySocket();

	m_Socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

	if(m_Socket == INVALID_SOCKET)
	{
#ifdef _WIN32
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(GetLastError()));
#else
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(errno));
#endif
		return false;
	}

	if (connect(m_Socket,
				(struct sockaddr *)&m_RemoteAddress,
				sizeof(m_RemoteAddress)) == -1)
	{
#ifdef _WIN32
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(GetLastError()));
#else
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(errno));
#endif
		return false;
	}

	return true;
}

void BufferedSocket::DestroySocket()
{
	if (m_Socket != 0)
	{
		if (closesocket(m_Socket) != 0)
			ReportError(__LINE__, __FUNCTION__, "Could not close socket: %d", m_Socket);

		m_Socket = 0;
	}
}
	
void BufferedSocket::SetRemoteAddress(const string &Address, const int16_t &Port)
{
	struct hostent *he;

	if((he = gethostbyname((const char *)Address.c_str())) == NULL)
	{
#ifdef _WIN32
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(GetLastError()));
#else
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(errno));
#endif
		return;
    }

	m_RemoteAddress.sin_family = PF_INET;
	m_RemoteAddress.sin_port = htons(Port);
	m_RemoteAddress.sin_addr = *((struct in_addr *)he->h_addr);
	memset(m_RemoteAddress.sin_zero, '\0', sizeof m_RemoteAddress.sin_zero);
}

bool BufferedSocket::SetRemoteAddress(const string &Address)
{
	size_t colon = Address.find(':');

	if (colon == string::npos)
		return false;

	if (colon + 1 >= Address.length())
		return false;

	uint16_t Port = atoi(Address.substr(colon + 1).c_str());
	string HostIP = Address.substr(0, colon);

    SetRemoteAddress(HostIP, Port);

    return true;
}

void BufferedSocket::GetRemoteAddress(string &Address, uint16_t &Port) const
{
	Address = inet_ntoa(m_RemoteAddress.sin_addr);
	Port = ntohs(m_RemoteAddress.sin_port);
}

string BufferedSocket::GetRemoteAddress() const
{
	ostringstream rmtAddr;

	rmtAddr << inet_ntoa(m_RemoteAddress.sin_addr) << ":" << ntohs(m_RemoteAddress.sin_port);

	return rmtAddr.str();
}

int32_t BufferedSocket::SendData(const int32_t &Timeout)
{
	int32_t BytesSent;

	m_BufferSize = m_BufferPos;

	if(m_BufferSize == 0)
		return 0;

	if (CreateSocket() == false)
		return 0;

	// Creation of this will start the stop watch
	uint32_t sw = AG_GetTicks();

	// send the data
	BytesSent = send(m_Socket, (const char *)m_SocketBuffer, m_BufferSize, 0);

	// set the start ping
	m_SendPing = AG_GetTicks() - sw;

	if(BytesSent < 0 && errno > 0)
	{
#ifdef _WIN32
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(GetLastError()));
#else
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(errno));
#endif
	}

	// return the amount of bytes sent
	return BytesSent;
}

int32_t BufferedSocket::GetData(const int32_t &Timeout)
{
	fd_set           readfds;
	struct timeval   tv;
	bool             DestroyMe = false;
	uint32_t         sw = AG_GetTicks();

	// clear it
	ClearBuffer();

	// Wait for read with timeout
	if(Timeout >= 0)
	{
		int32_t res;

		FD_ZERO(&readfds);
		FD_SET(m_Socket, &readfds);
		tv.tv_sec = Timeout / 1000;
		tv.tv_usec = (Timeout % 1000) * 1000; // convert milliseconds to microseconds
		res = select(m_Socket+1, &readfds, NULL, NULL, &tv);
		if(res <=  0) // Timeout or error
			DestroyMe = true;
	}

	if (DestroyMe == true)
	{
		if(errno > 0)
		{
#ifdef _WIN32
			ReportError(__LINE__, __FUNCTION__, AG_Strerror(GetLastError()));
#else
			ReportError(__LINE__, __FUNCTION__, AG_Strerror(errno));
#endif
		}

		m_SendPing = 0;
		m_ReceivePing = 0;

		DestroySocket();

		return 0;
	}

	m_BufferSize = recv(m_Socket, (char *)m_SocketBuffer, MAX_PAYLOAD, 0);

	// -1 = Error; 0 = Closed Connection
	if(m_BufferSize == 0)
	{
#ifdef _WIN32
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(GetLastError()));
#else
		ReportError(__LINE__, __FUNCTION__, AG_Strerror(errno));
#endif

		m_SendPing = 0;
		m_ReceivePing = 0;

		DestroySocket();

		return 0;
	}

	// apply the receive ping
	m_ReceivePing = AG_GetTicks() - sw;

	if (m_BufferSize > 0)
	{
		m_BadRead = false;

		DestroySocket();

		// return bytes received   
		return m_BufferSize;
	}

	m_SendPing = 0;
	m_ReceivePing = 0;

	DestroySocket();

	return 0;
}
        
bool BufferedSocket::ReadString(string &str)
{
	signed char ch;

    if (!CanRead(1))
    {
        ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

        str = "";

        m_BadRead = true;

        return false;
    }

    ostringstream in_str;

    // ooh, a priming read!
	bool isRead = Read8(ch);

    while (ch != '\0' && isRead)
    {
        in_str << ch;

        Read8(ch);

		isRead = CanRead(1);
    }

    if (!isRead)
    {
        ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

        str = "";

        m_BadRead = true;

        return false;
    }

	str = in_str.str();

	return true;
}

bool BufferedSocket::ReadBool(bool &val)
{
	if (!CanRead(1))
	{
		ReportError(__LINE__, __FUNCTION__, "ReadBool: End of buffer reached!");

		val = false;

		m_BadRead = true;

		return false;
	}

	int8_t value = 0;

	Read8(value);

	if (value < 0 || value > 1)
	{
		ReportError(__LINE__, __FUNCTION__, "Value is not 0 or 1, possibly corrupted packet");

		val = false;

		m_BadRead = true;

		return false;
	}

	val = value ? true : false;

	return true;
}

//
// Signed reads
//

bool BufferedSocket::Read32(int32_t &Int32)
{
	if (!CanRead(4))
	{
		Int32 = 0;

		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Int32 = m_SocketBuffer[m_BufferPos] + 
			(m_SocketBuffer[m_BufferPos+1] << 8) + 
			(m_SocketBuffer[m_BufferPos+2] << 16) + 
			(m_SocketBuffer[m_BufferPos+3] << 24);

	m_BufferPos += 4;

	return true;
}

bool BufferedSocket::Read16(int16_t &Int16)
{
	if (!CanRead(2))
	{
		Int16 = 0;

		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Int16 = m_SocketBuffer[m_BufferPos] + 
			(m_SocketBuffer[m_BufferPos+1] << 8);

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Read8(int8_t &Int8)
{
	if (!CanRead(1))
	{
		Int8 = 0;

		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Int8 = m_SocketBuffer[m_BufferPos];

	m_BufferPos++;

	return true;
}

//
// Unsigned reads
//

bool BufferedSocket::Read32(uint32_t &Uint32)
{
	if (!CanRead(4))
	{
		Uint32 = 0;

		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadRead = true;

		return false;
	}

    Uint32 = m_SocketBuffer[m_BufferPos] + 
			(m_SocketBuffer[m_BufferPos+1] << 8) + 
			(m_SocketBuffer[m_BufferPos+2] << 16) + 
			(m_SocketBuffer[m_BufferPos+3] << 24);


	m_BufferPos += 4;

	return true;
}

bool BufferedSocket::Read16(uint16_t &Uint16)
{
	if (!CanRead(2))
	{
		Uint16 = 0;

		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Uint16 = m_SocketBuffer[m_BufferPos] + 
			(m_SocketBuffer[m_BufferPos+1] << 8);

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Read8(uint8_t &Uint8)
{
	if (!CanRead(1))
	{
		Uint8 = 0;

		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Uint8 = m_SocketBuffer[m_BufferPos];

	m_BufferPos++;

	return true;
}

//
// Write values
//

bool BufferedSocket::WriteString(const string &str)
{
	if (!CanWrite(str.length() + 1))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	// Copy the string plus null terminator
	memcpy(&m_SocketBuffer[m_BufferPos], str.c_str(), str.length() + 1);

	m_BufferPos += str.length();

	return true;
}

bool BufferedSocket::WriteBool(const bool &val)
{
	if (!CanWrite(1))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	return Write8((int8_t)(val ? 1 : 0));
}

//
// Signed writes
//

bool BufferedSocket::Write32(const int32_t &Int32)
{
	if (!CanWrite(4))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Int32 & 0xff;
	m_SocketBuffer[m_BufferPos+1] = (Int32 >> 8) & 0xff;
	m_SocketBuffer[m_BufferPos+2] = (Int32 >> 16) & 0xff;
	m_SocketBuffer[m_BufferPos+3] = Int32 >> 24;

	m_BufferPos += 4;

	return true;
}

bool BufferedSocket::Write16(const int16_t &Int16)
{
	if (!CanWrite(2))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Int16 & 0xff;
	m_SocketBuffer[m_BufferPos+1] = Int16 >> 8;

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Write8(const int8_t &Int8)
{
	if (!CanWrite(1))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Int8;

	m_BufferPos++;

	return true;
}

//
// Unsigned writes
//

bool BufferedSocket::Write32(const uint32_t &Uint32)
{
	if (!CanWrite(4))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Uint32 & 0xff;
	m_SocketBuffer[m_BufferPos+1] = (Uint32 >> 8) & 0xff;
	m_SocketBuffer[m_BufferPos+2] = (Uint32 >> 16) & 0xff;
	m_SocketBuffer[m_BufferPos+3] = Uint32 >> 24;

	m_BufferPos += 4;

	return true;
}

bool BufferedSocket::Write16(const uint16_t &Uint16)
{
	if (!CanWrite(2))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Uint16 & 0xff;
	m_SocketBuffer[m_BufferPos+1] = Uint16 >> 8;

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Write8(const uint8_t &Uint8)
{
	if (!CanWrite(1))
	{
		ReportError(__LINE__, __FUNCTION__, "End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Uint8;

	m_BufferPos++;

	return true;
}
        
//
// Can read or write X bytes to a buffer
//

bool BufferedSocket::CanRead(const size_t &Bytes)
{
	return m_BufferPos + Bytes > m_BufferSize ? 0 : 1;
}

bool BufferedSocket::CanWrite(const size_t &Bytes)
{
	return m_BufferPos + Bytes > MAX_PAYLOAD ? 0 : 1;
}
 
void BufferedSocket::ClearBuffer()
{
	delete m_SocketBuffer;

	m_SocketBuffer = new byte[MAX_PAYLOAD];

	if(m_SocketBuffer == NULL)
		ReportError(__LINE__, __FUNCTION__, "Failed to allocate m_SocketBuffer!");

	m_BufferSize = 0;

	ResetBuffer();
}

} // namespace
