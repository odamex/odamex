// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2026 by The Odamex Team.
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
//  Canary socket for detecting abnormal client disconnections.
//
//-----------------------------------------------------------------------------

#include "CanarySocket.h"

#include <algorithm>
#include <iso646.h>
#include <string.h>

#ifdef _WIN32
#   define SETSOCKOPTCAST(x) ((const char *)(x))
#   define GETSOCKOPTCAST(x) ((char *)(x))
using socklen_t = int;

#else
#   include <netinet/tcp.h>
#   include <unistd.h>
#   define closesocket(x) close((x))
#   define SETSOCKOPTCAST(x) ((const void *)(x))
#   define GETSOCKOPTCAST(x) ((void *)(x))
#endif

#include "odamex.h"

CanarySocketServer::CanarySocketServer(int i_tcpPort) :
	m_serverSocket(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
{
	if (m_serverSocket >= 0)
	{
		sockaddr_in address;

		memset (&address, 0, sizeof(address));
		address.sin_family      = AF_INET;
		address.sin_addr.s_addr = INADDR_ANY;
		address.sin_port        = htons(static_cast<unsigned short>(i_tcpPort));

		if (bind(m_serverSocket,
		         reinterpret_cast<sockaddr*>(&address),
		         sizeof(address)) == 0)
		{
			if (listen(m_serverSocket, 10) == 0)
			{
				return;
			}
		}

		closesocket(m_serverSocket);
		m_serverSocket = CANARY_BAD_SOCKET;
	}
}

CanarySocketServer::~CanarySocketServer()
{
	for (auto& canary : m_canaries)
	{
		closesocket(canary.socket);
	}
}

CanarySocketServer::iterator CanarySocketServer::FindDead()
{
	if (m_serverSocket == CANARY_BAD_SOCKET)
	{
		return m_canaries.end();
	}

	fd_set sockets;
	FD_ZERO(&sockets);
	FD_SET(m_serverSocket, &sockets);
	CANARY_SOCKET_INT greatestSocketValue = m_serverSocket;

	for (auto& canary : m_canaries)
	{
		FD_SET(canary.socket, &sockets);
		greatestSocketValue = std::max(greatestSocketValue, canary.socket);
	}

	timeval noWait     = {0, 0};
	int     numSockets = select(greatestSocketValue + 1, &sockets, nullptr, nullptr, &noWait);

	if (numSockets > 0)
	{
		if (FD_ISSET(m_serverSocket, &sockets))
		{
			--numSockets;

			sockaddr_in address;
			sockaddr_in dataAddress;

			socklen_t   addressLength = sizeof(address);
			const CANARY_SOCKET_INT clientSocket = accept(m_serverSocket, reinterpret_cast<sockaddr*>(&address), &addressLength);

			int enable = 1;
			int interval = 10;
			if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE, SETSOCKOPTCAST(&enable), static_cast<socklen_t>(sizeof(enable))))
			{
				PrintFmt("Failed to enable SO_KEEPALIVE on the canary: {}\n", strerror(errno));
			}
			else if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPIDLE, SETSOCKOPTCAST(&interval), static_cast<socklen_t>(sizeof(interval))))
			{
				PrintFmt("Failed to set TCP_KEEPIDLE on the canary: {}\n", strerror(errno));
			}
			else if (setsockopt(clientSocket, IPPROTO_TCP, TCP_KEEPINTVL, SETSOCKOPTCAST(&interval), static_cast<socklen_t>(sizeof(interval))))
			{
				PrintFmt("Failed to set TCP_KEEPINTVL on the canary: {}\n", strerror(errno));
			}

			dataAddress = address;

			recv(clientSocket, reinterpret_cast<char*>(&dataAddress.sin_port), sizeof(dataAddress.sin_port), MSG_WAITALL);

			const int playerId = m_connectCallback ? m_connectCallback(dataAddress) : -1;

			m_canaries.emplace_back(playerId, clientSocket);
		}

		// As we find dead canaries, move them to the end of the vector so that we can easily just erase
		// them without moving too much data.
		iterator firstDeadCanary = m_canaries.end();
		iterator iter = m_canaries.begin();
		while (iter != firstDeadCanary and numSockets > 0)
		{
			if (FD_ISSET(iter->socket, &sockets))
			{
				using std::swap;

				--numSockets;
				--firstDeadCanary;
				swap(*iter, *firstDeadCanary);
			}
			else
			{
				++iter;
			}
		}
		return firstDeadCanary;
	}
	return m_canaries.end();
}

CanarySocketClient::~CanarySocketClient()
{
	if (m_socket >= 0)
	{
		closesocket(m_socket);
	}
}

bool CanarySocketClient::Connect(const sockaddr_in& i_toAddress, const sockaddr_in& i_dataAddress)
{
	if (m_socket == CANARY_BAD_SOCKET)
	{
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_socket >= 0)
		{
#ifdef _WIN32
			DWORD timeout = 2000;       // Winsock says the value must be in msec.
#else
			timeval timeout;
			timeout.tv_sec  = 2;
			timeout.tv_usec = 0;
#endif
			if (setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, SETSOCKOPTCAST(&timeout), static_cast<socklen_t>(sizeof(timeout))))
			{
				PrintFmt("Failed to set SO_RCVTIMEO on the canary: {}\n", strerror(errno));
			}
			else if (setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, SETSOCKOPTCAST(&timeout), static_cast<socklen_t>(sizeof(timeout))))
			{
				PrintFmt("Failed to set SO_SNDTIMEO on the canary: {}\n", strerror(errno));
			}
			if (connect(m_socket, reinterpret_cast<const sockaddr*>(&i_toAddress), sizeof(i_toAddress)) == 0)
			{
				// sin_port is already in network byte order.
				if (send(m_socket,
				         reinterpret_cast<const char*>(&i_dataAddress.sin_port),
				         sizeof(i_dataAddress.sin_port),
				         0) == sizeof(i_dataAddress.sin_port))
				{
					return true;
				}
			}
			else
			{
				PrintFmt("Connecting without canary\n");
			}
			closesocket(m_socket);
			m_socket = CANARY_BAD_SOCKET;
		}
	}
	return false;
}
