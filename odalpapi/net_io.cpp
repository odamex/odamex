// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2026 by The Odamex Team.
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
//  Low-level socket and buffer class
//
// AUTHORS:
//  Russell Rice (russell at odamex dot net)
//  Michael Wood (mwoodj at knology dot net)
//
//-----------------------------------------------------------------------------

#include "net_io.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <time.h>
#include <errno.h>

#include "net_utils.h"
#include "net_error.h"

#ifdef _WIN32
#define AI_ALL 0x00000100
#else
#include <unistd.h>
#define closesocket close
const int INVALID_SOCKET = -1;
#endif

using namespace std;

namespace odalpapi
{

BufferedSocket::BufferedSocket() :  m_BadRead(false), m_BadWrite(false),
	m_Socket(INVALID_SOCKET), m_RemoteAddressLength(0), m_SendPing(0), m_ReceivePing(0)
{
	m_Broadcast = false;
	memset(&m_RemoteAddress, 0, sizeof(m_RemoteAddress));

	m_SocketBuffer = new byte[MAX_PAYLOAD];

	if(m_SocketBuffer == NULL)
		NET_ReportError("Failed to allocate m_SocketBuffer!");

}

BufferedSocket::~BufferedSocket()
{
	DestroySocket();

	delete[] m_SocketBuffer;
}

// System-specific Initialize and shutdown functions
bool BufferedSocket::InitializeSocketAPI()
{
#ifdef _WIN32
	WSADATA wsaData;

	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
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

bool BufferedSocket::CreateSocket()
{
	DestroySocket();

	const int family = m_Broadcast ? AF_INET :
		reinterpret_cast<const sockaddr*>(&m_RemoteAddress)->sa_family;

	if(family != AF_INET && family != AF_INET6)
	{
		NET_ReportError("No valid remote address has been configured");
		return false;
	}

	m_Socket = socket(family, SOCK_DGRAM, IPPROTO_UDP);

	if(m_Socket == INVALID_SOCKET)
	{
		NET_ReportError(REPERR_NO_ARGS);
		return false;
	}

	if(m_Broadcast)
	{
		int optval = 1;
		int result = setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST,
		                        (char*)&optval, sizeof(optval));

		if(result != 0)
		{
			NET_ReportError(REPERR_NO_ARGS);
			DestroySocket();
			return false;
		}

		// IPv4 broadcast discovery listens on the legacy discovery port.
		sockaddr_in localAddress;
		memset(&localAddress, 0, sizeof(localAddress));
		localAddress.sin_family = AF_INET;
		localAddress.sin_port = htons(11510);
		localAddress.sin_addr.s_addr = htonl(INADDR_ANY);

		result = ::bind(m_Socket, (sockaddr*)&localAddress,
		                sizeof(localAddress));

		if(result != 0)
		{
			NET_ReportError(REPERR_NO_ARGS);
			DestroySocket();
			return false;
		}
	}

	return true;
}

void BufferedSocket::SetBroadcast(bool enabled)
{
	m_Broadcast = enabled;
}

void BufferedSocket::DestroySocket()
{
	if(m_Socket != INVALID_SOCKET)
	{
		if(closesocket(m_Socket) != 0)
			NET_ReportError("Could not close socket: %d", m_Socket);

		m_Socket = INVALID_SOCKET;
	}
}

void BufferedSocket::SetRemoteAddress(const string& Address, const uint16_t& Port)
{
	addrinfo hints;
	addrinfo* result = NULL;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	memset(&m_RemoteAddress, 0, sizeof(m_RemoteAddress));
	m_RemoteAddressLength = 0;

	const string service = std::to_string(Port);
	if(getaddrinfo(Address.c_str(), service.c_str(), &hints, &result) != 0)
	{
		NET_ReportError("Could not resolve remote address: %s", Address.c_str());
		return;
	}

	for(addrinfo* current = result; current; current = current->ai_next)
	{
		if((current->ai_family == AF_INET || current->ai_family == AF_INET6) &&
		   current->ai_addrlen <= sizeof(m_RemoteAddress))
		{
			memcpy(&m_RemoteAddress, current->ai_addr, current->ai_addrlen);
			m_RemoteAddressLength = static_cast<int>(current->ai_addrlen);
			break;
		}
	}

	freeaddrinfo(result);

	if(m_RemoteAddressLength == 0)
		NET_ReportError("No usable IPv4 or IPv6 address found for %s", Address.c_str());
}

bool BufferedSocket::SetRemoteAddress(const string& Address)
{
	string Host;
	uint16_t Port = 0;

	if(OdaAddrToComponents(Address, Host, Port) != 0 || Host.empty() || !Port)
		return false;

	SetRemoteAddress(Host, Port);
	return m_RemoteAddressLength != 0;
}

void BufferedSocket::GetRemoteAddress(string& Address, uint16_t& Port) const
{
	char addressBuffer[INET6_ADDRSTRLEN] = {};
	const sockaddr* remote =
		reinterpret_cast<const sockaddr*>(&m_RemoteAddress);

	if(remote->sa_family == AF_INET)
	{
		const sockaddr_in* address =
			reinterpret_cast<const sockaddr_in*>(&m_RemoteAddress);
		if(inet_ntop(AF_INET, &address->sin_addr, addressBuffer,
		             sizeof(addressBuffer)) == NULL)
			Address.clear();
		else
			Address = addressBuffer;
		Port = ntohs(address->sin_port);
	}
	else if(remote->sa_family == AF_INET6)
	{
		const sockaddr_in6* address =
			reinterpret_cast<const sockaddr_in6*>(&m_RemoteAddress);
		if(inet_ntop(AF_INET6, &address->sin6_addr, addressBuffer,
		             sizeof(addressBuffer)) == NULL)
			Address.clear();
		else
		{
			Address = addressBuffer;
			if(address->sin6_scope_id != 0)
				Address += "%" + std::to_string(address->sin6_scope_id);
		}
		Port = ntohs(address->sin6_port);
	}
	else
	{
		Address.clear();
		Port = 0;
	}
}

string BufferedSocket::GetRemoteAddress() const
{
	string address;
	uint16_t port = 0;
	GetRemoteAddress(address, port);

	if(address.find(':') != string::npos)
		return "[" + address + "]:" + std::to_string(port);

	return address + ":" + std::to_string(port);
}

int32_t BufferedSocket::SendData(const int32_t& Timeout)
{
	int32_t BytesSent;

	m_BufferSize = m_BufferPos;

	if(!m_BufferSize)
		return 0;

	if(CreateSocket() == false)
		return 0;

	if(m_RemoteAddressLength == 0)
	{
		NET_ReportError("Cannot send without a valid remote address");
		return 0;
	}

	BytesSent = sendto(m_Socket, (const char*)m_SocketBuffer,
	                   static_cast<int>(m_BufferSize), 0,
	                   reinterpret_cast<const sockaddr*>(&m_RemoteAddress),
	                   m_RemoteAddressLength);

	// set the start ping
	m_SendPing = GetMillisNow();

	if(BytesSent < 0)
	{
		NET_ReportError(REPERR_NO_ARGS);
	}

	// return the amount of bytes sent
	return BytesSent;
}

int32_t BufferedSocket::GetData(const int32_t& Timeout)
{
	int32_t BytesReceived;
	int32_t          res;
	fd_set           readfds;
	struct timeval   tv;
	bool             DestroyMe = false;
#ifdef _WIN32
	int              fromlen;
#else
	socklen_t        fromlen;
#endif

	// Wait for read with timeout
	if(Timeout > 0)
	{
		FD_ZERO(&readfds);
		FD_SET(m_Socket, &readfds);
		tv.tv_sec = Timeout / 1000;
		tv.tv_usec = (Timeout % 1000) * 1000; // convert milliseconds to microseconds
		res = select(m_Socket+1, &readfds, NULL, NULL, &tv);

		if(res <= 0) // Timeout or error
			DestroyMe = true;
	}

	if(DestroyMe == true)
	{
		if(res == -1)
			NET_ReportError(REPERR_NO_ARGS);

		m_SendPing = 0;
		m_ReceivePing = 0;

		return -1;
	}

	memset(&m_RemoteAddress, 0, sizeof(m_RemoteAddress));
	fromlen = sizeof(m_RemoteAddress);

	BytesReceived = recvfrom(m_Socket, (char*)m_SocketBuffer,
	                         static_cast<int>(MAX_PAYLOAD), 0,
	                         reinterpret_cast<sockaddr*>(&m_RemoteAddress),
	                         &fromlen);
	if(BytesReceived >= 0)
		m_RemoteAddressLength = fromlen;

	// -1 = Error; 0 = Closed Connection
	if(BytesReceived <= 0)
	{
		NET_ReportError(REPERR_NO_ARGS);

		m_SendPing = 0;
		m_ReceivePing = 0;

		return -2;
	}

	m_BufferSize = BytesReceived;

	// Reset buffers position
	ResetBuffer();

	// apply the receive ping
	m_ReceivePing = GetMillisNow();

	if(m_BufferSize > 0)
	{
		m_BadRead = false;

#ifdef ODAMEX_DEBUG
		NET_ReportError("bytes received: %d", m_BufferSize);
#endif

		// return bytes received
		return m_BufferSize;
	}

	m_SendPing = 0;
	m_ReceivePing = 0;


	return -3;
}

bool BufferedSocket::ReadHexString(string& str)
{
	std::stringstream hash;

	uint8_t size;

	if(!Read8(size))
		return false;

	for(uint8_t i = 0; i < size; ++i)
	{
		uint8_t ch;

		if(!CanRead(1))
		{
			NET_ReportError("End of buffer reached!");

			str = "";

			m_BadRead = true;

			return false;
		}

		if(!Read8(ch))
			return false;

		hash
		        << std::setw(2)
		        << std::setfill('0')
		        << std::hex
		        << std::uppercase
		        << (short)ch;
	}

	str = hash.str();

	return true;
}

bool BufferedSocket::ReadString(string& str)
{
	signed char ch;

	if(!CanRead(1))
	{
		NET_ReportError("End of buffer reached!");

		str = "";

		m_BadRead = true;

		return false;
	}

	// ooh, a priming read!
	bool isRead = Read8(ch);

	while(ch != '\0' && isRead)
	{
		str += ch;

		Read8(ch);

		isRead = CanRead(1);
	}

	if(!isRead)
	{
		NET_ReportError("End of buffer reached!");

		str = "";

		m_BadRead = true;

		return false;
	}

	return true;
}

bool BufferedSocket::ReadBool(bool& val)
{
	if(!CanRead(1))
	{
		NET_ReportError("ReadBool: End of buffer reached!");

		val = false;

		m_BadRead = true;

		return false;
	}

	int8_t value = 0;

	Read8(value);

	if(value < 0 || value > 1)
	{
		NET_ReportError("Value is not 0 or 1, possibly corrupted packet");

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

bool BufferedSocket::Read32(int32_t& Int32)
{
	if(!CanRead(4))
	{
		Int32 = 0;

		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::Read16(int16_t& Int16)
{
	if(!CanRead(2))
	{
		Int16 = 0;

		NET_ReportError("End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Int16 = m_SocketBuffer[m_BufferPos] +
	        (m_SocketBuffer[m_BufferPos+1] << 8);

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Read8(int8_t& Int8)
{
	if(!CanRead(1))
	{
		Int8 = 0;

		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::Read32(uint32_t& Uint32)
{
	if(!CanRead(4))
	{
		Uint32 = 0;

		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::Read16(uint16_t& Uint16)
{
	if(!CanRead(2))
	{
		Uint16 = 0;

		NET_ReportError("End of buffer reached!");

		m_BadRead = true;

		return false;
	}

	Uint16 = m_SocketBuffer[m_BufferPos] +
	         (m_SocketBuffer[m_BufferPos+1] << 8);

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Read8(uint8_t& Uint8)
{
	if(!CanRead(1))
	{
		Uint8 = 0;

		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::WriteString(const string& str)
{
	if(!CanWrite(str.length() + 1))
	{
		NET_ReportError("End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	// Copy the string plus null terminator
	memcpy(&m_SocketBuffer[m_BufferPos], str.c_str(), str.length() + 1);

	m_BufferPos += str.length();

	return true;
}

bool BufferedSocket::WriteBool(const bool& val)
{
	if(!CanWrite(1))
	{
		NET_ReportError("End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	return Write8((int8_t)(val ? 1 : 0));
}

//
// Signed writes
//

bool BufferedSocket::Write32(const int32_t& Int32)
{
	if(!CanWrite(4))
	{
		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::Write16(const int16_t& Int16)
{
	if(!CanWrite(2))
	{
		NET_ReportError("End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Int16 & 0xff;
	m_SocketBuffer[m_BufferPos+1] = Int16 >> 8;

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Write8(const int8_t& Int8)
{
	if(!CanWrite(1))
	{
		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::Write32(const uint32_t& Uint32)
{
	if(!CanWrite(4))
	{
		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::Write16(const uint16_t& Uint16)
{
	if(!CanWrite(2))
	{
		NET_ReportError("End of buffer reached!");

		m_BadWrite = true;

		return false;
	}

	m_SocketBuffer[m_BufferPos] = Uint16 & 0xff;
	m_SocketBuffer[m_BufferPos+1] = Uint16 >> 8;

	m_BufferPos += 2;

	return true;
}

bool BufferedSocket::Write8(const uint8_t& Uint8)
{
	if(!CanWrite(1))
	{
		NET_ReportError("End of buffer reached!");

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

bool BufferedSocket::CanRead(const size_t& Bytes)
{
	return m_BufferPos + Bytes > m_BufferSize ? 0 : 1;
}

bool BufferedSocket::CanWrite(const size_t& Bytes)
{
	return m_BufferPos + Bytes > MAX_PAYLOAD ? 0 : 1;
}

void BufferedSocket::ClearBuffer()
{
	m_BufferSize = 0;
	m_BufferPos = 0;
}

} // namespace
