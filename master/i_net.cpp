// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62)
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
//	Master Server communicator
//
//-----------------------------------------------------------------------------


#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#include "i_net.h"

#ifndef _WIN32
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define closesocket close
#define ioctlsocket ioctl
#define Sleep(x) usleep(x * 1000)
#else
typedef int socklen_t;
#endif


int net_socket;
int localport;
netadr_t net_from;   // address of who sent the packet

buf_t net_message(MAX_UDP_PACKET);

//
// UDPsocket - Create a dual-stack IPv4/IPv6 socket
//
SOCKET UDPsocket(void)
{
	SOCKET s;

	// Try to create an IPv6 socket first (dual-stack)
	s = socket(PF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
	{
		// Fall back to IPv4 if IPv6 is not available
		s = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (s == INVALID_SOCKET)
		{
			printf("can't create socket\n");
			return INVALID_SOCKET;
		}
		return s;
	}

	// Set IPV6_V6ONLY to 0 to allow dual-stack operation (IPv4 and IPv6)
	#ifdef IPV6_V6ONLY
	int ipv6_only = 0;
	if (setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (const char *)&ipv6_only, 
		sizeof(ipv6_only)) == SOCKET_ERROR)
	{
		printf("Warning: Could not disable IPV6_V6ONLY, IPv4 compatibility may be limited\n");
	}
	#endif

	return s;
}

//
// BindToLocalPort - Bind socket to local port (IPv6 compatible)
//
void BindToLocalPort(SOCKET s, u_short port)
{
	int v;
	struct sockaddr_in6 address6;
	struct sockaddr_in address4;

	// Try to bind to IPv6 first (this includes IPv4 via dual-stack)
	memset(&address6, 0, sizeof(address6));
	address6.sin6_family = AF_INET6;
	address6.sin6_addr = in6addr_any;
	address6.sin6_port = htons(port);

	v = bind(s, (struct sockaddr *)&address6, sizeof(address6));
	
	// If IPv6 binding failed, try IPv4
	if (v == SOCKET_ERROR)
	{
		memset(&address4, 0, sizeof(address4));
		address4.sin_family = AF_INET;
		address4.sin_addr.s_addr = INADDR_ANY;
		address4.sin_port = htons(port);

		v = bind(s, (struct sockaddr *)&address4, sizeof(address4));
	}

	if (v == SOCKET_ERROR)
	{
		printf("BindToPort: error\n");
	}
}


void CloseNetwork(void)
{
	closesocket(net_socket);
#ifdef _WIN32
	WSACleanup();
#endif
}


// Convert sockaddr_in6 to netadr_t
void Sockaddr6ToNetadr(struct sockaddr_in6 *s, netadr_t *a)
{
	memcpy(&(a->ip.ipv6), &(s->sin6_addr), sizeof(struct in6_addr));
	a->family = AF_INET6;
	a->port = s->sin6_port;
}

// Convert sockaddr_in to netadr_t
void SockadrToNetadr(struct sockaddr_in *s, netadr_t *a)
{
	memcpy(&(a->ip.ipv4), &(s->sin_addr), sizeof(struct in_addr));
	a->family = AF_INET;
	a->port = s->sin_port;
}

// Convert netadr_t to sockaddr_in6
void NetadrToSockaddr6(netadr_t *a, struct sockaddr_in6 *s)
{
	memset(s, 0, sizeof(*s));
	s->sin6_family = AF_INET6;
	memcpy(&(s->sin6_addr), &(a->ip.ipv6), sizeof(struct in6_addr));
	s->sin6_port = a->port;
}

// Convert netadr_t to sockaddr_in
void NetadrToSockadr(netadr_t *a, struct sockaddr_in *s)
{
	memset(s, 0, sizeof(*s));
	s->sin_family = AF_INET;
	memcpy(&(s->sin_addr), &(a->ip.ipv4), sizeof(struct in_addr));
	s->sin_port = a->port;
}

// Convert network address to string (IPv4 or IPv6)
char *NET_AdrToString(netadr_t a, bool displayport)
{
	static char s[128];
	char address_str[128];

	if (a.isIPv6())
	{
		// Format IPv6 address
		if (inet_ntop(AF_INET6, &(a.ip.ipv6), address_str, sizeof(address_str)) == NULL)
			snprintf(address_str, sizeof(address_str), "unknown");
		
		if (displayport)
			snprintf(s, sizeof(s), "[%s]:%i", address_str, ntohs(a.port));
		else
			snprintf(s, sizeof(s), "[%s]", address_str);
	}
	else
	{
		// Format IPv4 address
		if (displayport)
			snprintf(s, sizeof(s), "%i.%i.%i.%i:%i", a.ip.ipv4[0], a.ip.ipv4[1], 
				a.ip.ipv4[2], a.ip.ipv4[3], ntohs(a.port));
		else
			snprintf(s, sizeof(s), "%i.%i.%i.%i", a.ip.ipv4[0], a.ip.ipv4[1], 
				a.ip.ipv4[2], a.ip.ipv4[3]);
	}

	return s;
}

// Parse string to network address (supports IPv4, IPv6, and IPv6 bracket notation)
bool NET_StringToAdr(const char *s, netadr_t *a)
{
	struct addrinfo hints, *result = NULL;
	char copy[256];
	char *colon = NULL;
	char *port_str = NULL;
	int port = 0;

	if (!s || !a)
		return false;

	strncpy(copy, s, sizeof(copy) - 1);
	copy[sizeof(copy) - 1] = 0;

	// Handle IPv6 bracket notation: [::1]:port
	if (copy[0] == '[')
	{
		char *bracket = strchr(copy, ']');
		if (bracket)
		{
			*bracket = 0;
			colon = bracket + 1;
			if (*colon == ':')
				port_str = colon + 1;
			// Remove the leading bracket
			memmove(copy, copy + 1, strlen(copy));
		}
	}
	else
	{
		// Handle IPv4 with port: x.x.x.x:port or hostname:port
		colon = strrchr(copy, ':');
		if (colon)
		{
			// Check if this is an IPv6 address without brackets (contains multiple colons)
			if (strchr(copy, ':') != colon)
			{
				// Multiple colons - likely IPv6, no port specified
				colon = NULL;
			}
			else
			{
				*colon = 0;
				port_str = colon + 1;
			}
		}
	}

	if (port_str)
		port = atoi(port_str);

	// Use getaddrinfo for modern address resolution (supports both IPv4 and IPv6)
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;      // Accept both IPv4 and IPv6
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;

	if (getaddrinfo(copy, NULL, &hints, &result) != 0)
		return false;

	if (!result)
		return false;

	// Use the first result
	if (result->ai_family == AF_INET6)
	{
		Sockaddr6ToNetadr((struct sockaddr_in6 *)result->ai_addr, a);
	}
	else if (result->ai_family == AF_INET)
	{
		SockadrToNetadr((struct sockaddr_in *)result->ai_addr, a);
	}
	else
	{
		freeaddrinfo(result);
		return false;
	}

	a->port = htons(port);
	freeaddrinfo(result);

	return true;
}

// Compare two network addresses
bool NET_CompareAdr(netadr_t a, netadr_t b)
{
	// Families must match
	if (a.family != b.family)
		return false;

	// Ports must match
	if (a.port != b.port)
		return false;

	// Compare addresses based on family
	if (a.isIPv6())
		return memcmp(&a.ip.ipv6, &b.ip.ipv6, sizeof(struct in6_addr)) == 0;
	else
		return memcmp(&a.ip.ipv4, &b.ip.ipv4, sizeof(struct in_addr)) == 0;
}

#ifdef _WIN32
typedef int socklen_t;
#endif

// Receive packet from network (IPv6 compatible)
int NET_GetPacket(void)
{
	int ret;
	struct sockaddr_storage from;
	socklen_t fromlen = sizeof(from);

	net_message.clear();
	ret = recvfrom(net_socket, (char *)net_message.ptr(), net_message.maxsize(), 0, 
		(struct sockaddr *)&from, &fromlen);

	if (ret == -1)
	{
		return 0;
	}

	net_message.setcursize(ret);

	// Convert sockaddr_storage to netadr_t based on address family
	if (from.ss_family == AF_INET6)
		Sockaddr6ToNetadr((struct sockaddr_in6 *)&from, &net_from);
	else if (from.ss_family == AF_INET)
		SockadrToNetadr((struct sockaddr_in *)&from, &net_from);

	return ret;
}

// Send packet to network (IPv6 compatible)
void NET_SendPacket(int length, byte *data, netadr_t to)
{
	int ret;
	struct sockaddr_storage addr;
	socklen_t addrlen;

	// Convert netadr_t to appropriate sockaddr structure
	if (to.isIPv6())
	{
		struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
		NetadrToSockaddr6(&to, addr6);
		addrlen = sizeof(struct sockaddr_in6);
	}
	else
	{
		struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
		NetadrToSockadr(&to, addr4);
		addrlen = sizeof(struct sockaddr_in);
	}

	ret = sendto(net_socket, (const char *)data, length, 0, 
		(struct sockaddr *)&addr, addrlen);
}

void I_SetPort(netadr_t &addr, int port)
{
   addr.port = htons(port);
}

void I_DoSelect(void)
{
	// Placeholder for master server select implementation
}

// Initialize network for master server
void InitNetCommon(void)
{
	unsigned long _true = true;

#ifdef _WIN32
	WSADATA wsad;
	WSAStartup(MAKEWORD(2, 2), &wsad);
#endif

	net_socket = UDPsocket();
	if (net_socket == INVALID_SOCKET)
		return;

	BindToLocalPort(net_socket, localport);

	if (ioctlsocket(net_socket, FIONBIO, &_true) == SOCKET_ERROR)
		printf("UDPsocket: ioctl FIONBIO failed\n");

	net_message.clear();
}
