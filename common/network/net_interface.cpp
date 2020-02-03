// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2015 by The Odamex Team.
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
//
// Abstraction for low-level network mangement 
//
//-----------------------------------------------------------------------------

#include "doomdef.h"
#include "m_ostring.h"
#include "m_random.h"
#include "c_cvars.h"
#include "cmdlib.h"
#include "hashtable.h"

#include "network/net_platform.h"
#include "network/net_log.h"
#include "network/net_socketaddress.h"
#include "network/net_interface.h"
#include "network/net_upnp.h"

#include "network/net_platform.h"

#ifdef _XBOX
	#include "i_xbox.h"
#endif

#ifdef GEKKO
	#include "i_wii.h"
#endif

static const uint16_t MAX_INCOMING_SIZE = 8192;		// in bytes
static const uint16_t MAX_OUTGOING_SIZE = 8192;

EXTERN_CVAR(net_ip)
EXTERN_CVAR(net_port)
EXTERN_CVAR(net_maxrate)

// ============================================================================
//
// NetInterface Implementation
//
// Abstraction of low-level socket communication. Maintains Connection objects
// for all existing connections.
//
// ============================================================================

// ----------------------------------------------------------------------------
// Public functions
// ----------------------------------------------------------------------------

NetInterface::NetInterface() :
	mInitialized(false), mSocket(INVALID_SOCKET), mUPNPInterface(NULL)
{
#ifdef CLIENT_APP
	mHostType = HOST_CLIENT;
#else
	mHostType = HOST_SERVER;
#endif
}


NetInterface::~NetInterface()
{
	closeInterface();
}


//
// NetInterface::openInterface
//
// Binds the interface to a port and opens a socket. If it is not possible to
// bind to desired_port, it will attempt to bind to one of the next 32
// sequential port numbers.
//
bool NetInterface::openInterface(const OString& ip_string, uint16_t desired_port)
{
	if (mInitialized)
		closeInterface();		// already open? close first.

	#ifdef _WIN32
	WSADATA wsad;
	WSAStartup(MAKEWORD(2,2), &wsad);
	#endif

	uint32_t desired_ip = SocketAddress(ip_string).getIPAddress();
	if (desired_ip == 0)
		Net_Printf("NetInterface::openInterface: binding to all local network interfaces.");

	// open a socket and bind it to a port and set mLocalAddress
	openSocket(desired_ip, desired_port);

	if (isSocketValid())
	{
		assignLocalAddress();
		Net_Printf("NetInterface::openInterface: opened socket at %s.", mLocalAddress.getCString());

		if (mHostType == HOST_SERVER)
			mUPNPInterface = new UPNPInterface(mLocalAddress);

		setReceivingBufferSize(65536);
		setSendingBufferSize(65536);

		mInitialized = true;
	}

	return mInitialized;
}


//
// NetInterace::closeInterface
//
// Releases the binding to a port and closes the socket.
//
bool NetInterface::closeInterface()
{
	if (mInitialized)
	{
		delete mUPNPInterface;
		mUPNPInterface = NULL;

		closeSocket();
		Net_Printf("NetInterface::closeInterface: closed socket at %s.", mLocalAddress.getCString());

		#ifdef _WIN32
		WSACleanup();
		#endif

		mLocalAddress.clear();
		mInitialized = false;
	}
	return true;
}


//
// NetInterface::openSocket
//
// Binds the interface to a port and opens a socket. If it is not possible to
// bind to desired_port, it will attempt to bind to one of the next 32
// sequential port numbers. mLocalAddress is set to the address of the opened
// socket.
//
void NetInterface::openSocket(uint32_t desired_ip, uint16_t desired_port)
{
	const uint16_t MAX_PORT_CHECKS = 32;

	mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (!isSocketValid())
	{
		Net_Error("NetInterface::openSocket: unable to open socket!");
		return;
	}

	// [SL] Use INADDR_ANY (all interfaces) for binding purposes if no valid interface address is provided
	if (desired_ip == 0)
		desired_ip = INADDR_ANY;

	SocketAddress tmp_address;
	tmp_address.setIPAddress(desired_ip);

	// try up to MAX_PORT_CHECKS port numbers until we find an unbound one
	for (uint16_t port = desired_port; port < desired_port + MAX_PORT_CHECKS; port++)
	{
		tmp_address.setPort(port);
		const struct sockaddr_in* sockaddress = &tmp_address.getSockAddrIn();
		if (bind(mSocket, (const sockaddr*)sockaddress, sizeof(sockaddr_in)) == 0)
		{
			// Set the socket as non-blocking	
			const uint32_t NON_BLOCKING = 1;
			#ifdef _WIN32
			if (ioctlsocket(mSocket, FIONBIO, &NON_BLOCKING) == 0)
				return;		// successfully bound the socket to a port
			#else
			if (fcntl(mSocket, F_SETFL, O_NONBLOCK, NON_BLOCKING) == 0)
				return;		// successfully bound the socket to a port
			#endif

			// Not able to set the socket as non-blocking. Print a message and error out.
			Net_Error("NetInterface::openSocket: failed to set socket to non-blocking mode for %s!", tmp_address.getCString());
			break;
		}
	}

	// unable to bind to a port
	closeSocket();
	tmp_address.setPort(desired_port);
	Net_Error("NetInterface::openSocket: unable to bind socket for %s!", tmp_address.getCString());
}


//
// NetInterface::closeSocket
//
// Closes mSocket if it's open.
//
void NetInterface::closeSocket()
{
	if (isSocketValid())
	{
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}
}


//
// NetInterface::isSocketValid
//
bool NetInterface::isSocketValid() const
{
	return mSocket != INVALID_SOCKET;
}


//
// NetInterface::assignLocalAddress
//
void NetInterface::assignLocalAddress()
{
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(mSocket, (struct sockaddr *)&sin, &len) == 0)
		mLocalAddress = SocketAddress(sin);
	else
		Net_Warning("NetInterface::assignLocalAddress: unable to discover local socket address!");
}


//
// NetInterface::setReceivingBufferSize
//
bool NetInterface::setReceivingBufferSize(uint32_t size)
{
	if (setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, SETSOCKOPTCAST(&size), (int)sizeof(size)) == 0)
		return true;

	Net_Warning("NetInterface::setReceivingBufferSize: setsockopt SO_RCVBUF: %s", strerror(errno));
	return false;
}


//
// NetInterface::setSendingBufferSize
//
bool NetInterface::setSendingBufferSize(uint32_t size)
{
	if (setsockopt(mSocket, SOL_SOCKET, SO_SNDBUF, SETSOCKOPTCAST(&size), (int)sizeof(size)) == 0)
		return true;

	Net_Warning("NetInterface::setReceivingBufferSize: setsockopt SO_SNDBUF: %s", strerror(errno));
	return false;
}


//
// NetInterface::socketSend
//
// Low-level function to send data to a remote host over UDP.
//
bool NetInterface::socketSend(const SocketAddress& dest, const uint8_t* data, uint16_t size) const
{
	if (size == 0 || !dest.isValid())
		return false;

	Net_LogPrintf(LogChan_Interface, "sending %u bytes to %s.", size, dest.getCString());
	const struct sockaddr_in& saddr = dest.getSockAddrIn();

	// pass the packet data off to the transport layer (UDP)
	int ret = sendto(mSocket, data, size, 0, (const struct sockaddr*)&saddr, sizeof(sockaddr_in));

	if (ret != size)
	{
		Net_Warning("NetInterface::socketSend: unable to send packet to destination %s.", dest.getCString()); 

		#ifdef _WIN32
		int err = WSAGetLastError();
		if (err == WSAEWOULDBLOCK)		// wouldblock is silent
			return false;
		#else
		if (errno == EWOULDBLOCK)
			return false;
		if (errno == ECONNREFUSED)
			return false;
		#endif
	}

	return true;
}


//
// NetInterface::socketRecv
//
// Low-level function to receive data from a remote host. The source parameter
// is set to the socket address of the sending host. The data buffer must be
// large enough to accomodate MAX_INCOMING_SIZE bytes.
//
bool NetInterface::socketRecv(SocketAddress& source, uint8_t* data, uint16_t& size) const
{
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);

	// receives a packet from the transport layer (UDP)
	int ret = recvfrom(mSocket, data, MAX_INCOMING_SIZE, 0, (struct sockaddr *)&from, &fromlen);

	source = from;

	if (ret < 0)
	{
		size = 0;

		#ifdef _WIN32
		errno = WSAGetLastError();

		if (errno == WSAEWOULDBLOCK)
			return false;

		if (errno == WSAECONNRESET)
			return false;

		if (errno == WSAEMSGSIZE)
		{
			Net_Warning("NetInterface::socketRecv: oversize packet from %s", source.getCString());
			return false;
		}
		#else
		if (errno == EWOULDBLOCK)
			return false;
		if (errno == ECONNREFUSED)
			return false;
		#endif

		Net_Warning("NetInterface::socketRecv: error code %s", strerror(errno));
		return false;
	}

	size = ret;
	Net_LogPrintf(LogChan_Interface, "received %u bytes from %s.", size, source.getCString());
	return true;
}


//
// NetInterface::getHostType
//
// Returns whether this host is a client or a server.
//
HostType_t NetInterface::getHostType() const
{
	return mHostType;
}


const SocketAddress& NetInterface::getLocalAddress() const
{
	return mLocalAddress;
}