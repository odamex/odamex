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

#pragma once

#include <functional>
#include <vector>

#ifdef _WIN32
#   define  WIN32_LEAN_AND_MEAN
#   include <winsock2.h>
#   include <ws2ipdef.h>            // For the keep alive socket options under IPPROTO_TCP
#   define  CANARY_SOCKET_INT SOCKET
#   define  CANARY_BAD_SOCKET INVALID_SOCKET

// Ugh.  Why, Microsoft, why?  Time to cleanup after the winsock inclusion...
#   ifdef max
#       undef max
#   endif
#   ifdef min
#       undef min
#   endif
#   ifdef MAXCHAR
#       undef MAXCHAR
#   endif
#   ifdef MAXSHORT
#       undef MAXSHORT
#   endif
#   ifdef MAXINT
#       undef MAXINT
#   endif
#   ifdef MAXUINT
#       undef MAXUINT
#   endif
#   ifdef MAXLONG
#       undef MAXLONG
#   endif
#   ifdef MINCHAR
#       undef MINCHAR
#   endif
#   ifdef MINSHORT
#       undef MINSHORT
#   endif
#   ifdef MININT
#       undef MININT
#   endif
#   ifdef MINUINT
#       undef MINUINT
#   endif
#   ifdef MINLONG
#       undef MINLONG
#   endif

#else
#   include <netinet/in.h>
#   include <sys/socket.h>
#   define  CANARY_SOCKET_INT int
#   define  CANARY_BAD_SOCKET -1
#endif

/// This class is a collection of "Canaries" that are actually TCP connections that let the server
/// know if any remote clients disconnect suddenly and ungracefully.
///
/// The only data communicated over the TCP connection is a UDP port number in network byte order,
/// immediately after the connection is established.  From that point forward the connection goes
/// slient and is monitored for being closed by the client itself or by the client's host OS.
class CanarySocketServer
{
	public:
		struct PlayerSocketType
		{
			sockaddr_in       udpAddr;
			CANARY_SOCKET_INT socket;
			bool              isAlive;

			PlayerSocketType(const sockaddr_in& i_udpAddr, CANARY_SOCKET_INT i_socket) :
				udpAddr (i_udpAddr),
				socket  (i_socket),
				isAlive (true)
			{
			}
		};

		using iterator = std::vector<PlayerSocketType>::iterator;

		explicit CanarySocketServer(int i_tcpPort);
		~CanarySocketServer();

		iterator end() { return m_canaries.end(); }

		/// Call out "Bring out your dead!" once per tic.  Will also handle new clients and make callbacks if needed.
		/// Returns an iterator to the first dead canary if there are any.  Put it on the cart once done with
		/// it and repeat until you reach end().
		iterator FindDead();
		iterator PutOnCart(iterator i_deadCanaryIter) { return m_canaries.erase(i_deadCanaryIter); }

	protected:

		std::vector<PlayerSocketType> m_canaries;
		CANARY_SOCKET_INT             m_serverSocket;
};

/// This class is the client's mechanism for establishing the "Canary" on the server side.
class CanarySocketClient
{
	public:
		~CanarySocketClient();

		/// Establish a connection to the Canary server.  Connect to the given i_toAddress,
		/// and supply the local UDP i_dataAddress that this client uses to communicate game traffic.
		/// Returns true if a new connection was successfully established, false otherwise.
		bool Connect(const sockaddr_in& i_toAddress, const sockaddr_in& i_dataAddress);

	protected:
		CANARY_SOCKET_INT m_socket { CANARY_BAD_SOCKET };
};
