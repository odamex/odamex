// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2020 by The Odamex Team.
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
//	System specific network interface stuff.
//
//-----------------------------------------------------------------------------


#include "odamex.h"

/* [Petteri] Check if compiling for Win32:	*/
//#if defined(__WINDOWS__) || defined(__NT__) || defined(_MSC_VER) || defined(WIN32)
//#	define WIN32
//#endif
/* Follow #ifdef __WIN32__ marks */

#include <stdlib.h>

#include <sstream>

/* [Petteri] Use Winsock for Win32: */
#include "win32inc.h"
#ifdef _WIN32
    #ifndef _XBOX
    	#include <winsock2.h>
        #include <ws2tcpip.h>
    #endif // !_XBOX
#else
#ifdef GEKKO // Wii/GC
#	include <network.h>
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <netdb.h>
#	include <sys/ioctl.h>
#endif // GEKKO
#	include <sys/types.h>
#	include <errno.h>
#	include <unistd.h>
#	include <sys/time.h>
#endif // WIN32

#ifndef _WIN32
typedef int SOCKET;
#ifndef GEKKO
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#endif
#define closesocket close
#define ioctlsocket ioctl
#define Sleep(x)	usleep (x * 1000)
#endif

#ifdef _WIN32
#define SETSOCKOPTCAST(x) ((const char *)(x))
#else
#define SETSOCKOPTCAST(x) ((const void *)(x))
#endif

#include <google/protobuf/message.h>


#include "i_system.h"
#include "i_net.h"
#include "svc_map.h"
#include "d_player.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#ifdef GEKKO
#include "i_wii.h"
#endif

#include "minilzo.h"

#ifdef ODA_HAVE_MINIUPNP
#include "miniupnpc/miniwget.h"
#include "miniupnpc/miniupnpc.h"
#include "miniupnpc/upnpcommands.h"
#endif

unsigned int	inet_socket;
int         	localport;
netadr_t    	net_from;   // address of who sent the packet

buf_t       net_message(MAX_UDP_PACKET);
extern bool	simulated_connection;

// buffer for compression/decompression
// can't be static to a function because some
// of the functions
buf_t compressed, decompressed;
lzo_byte wrkmem[LZO1X_1_MEM_COMPRESS];

EXTERN_CVAR(port)

msg_info_t clc_info[clc_max + 1];
msg_info_t svc_info[svc_max + 1];

#ifdef ODA_HAVE_MINIUPNP
EXTERN_CVAR(sv_upnp)
EXTERN_CVAR(sv_upnp_discovertimeout)
EXTERN_CVAR(sv_upnp_description)
EXTERN_CVAR(sv_upnp_internalip)
EXTERN_CVAR(sv_upnp_externalip)

static struct UPNPUrls urls;
static struct IGDdatas data;

static bool is_upnp_ok = false;

void init_upnp (void)
{
	struct UPNPDev * devlist;
	struct UPNPDev * dev;
	char * descXML;
	int descXMLsize = 0;
	int res = 0;

	char IPAddress[40];
	int r;

	if (!sv_upnp)
		return;

	memset(&urls, 0, sizeof(struct UPNPUrls));
	memset(&data, 0, sizeof(struct IGDdatas));

	Printf(PRINT_HIGH, "UPnP: Discovering router (max 1 unit supported)\n");

#if MINIUPNPC_API_VERSION < 14
	devlist = upnpDiscover(sv_upnp_discovertimeout.asInt(), NULL, NULL, 0, 0, &res);
#else
	devlist = upnpDiscover(sv_upnp_discovertimeout.asInt(), NULL, NULL, 0, 0, 2, &res);
#endif


	if (!devlist || res != UPNPDISCOVER_SUCCESS)
    {
		Printf(PRINT_WARNING, "UPnP: Router not found or timed out, error %d\n",
            res);

		is_upnp_ok = false;

		return;
	}

	dev = devlist;

	while (dev)
	{
		if (strstr (dev->st, "InternetGatewayDevice"))
			break;
		dev = dev->pNext;
	}

	if (!dev)
		dev = devlist; /* defaulting to first device */

	//Printf(PRINT_HIGH, "UPnP device :\n"
	  //	  " desc: %s\n st: %s\n",
		//	dev->descURL, dev->st);

#if MINIUPNPC_API_VERSION < 16
	descXML = (char *)miniwget(dev->descURL, &descXMLsize, 0);
#else
	descXML = (char *)miniwget(dev->descURL, &descXMLsize, 0, &res);
#endif

	if (descXML)
	{
		parserootdesc (descXML, descXMLsize, &data);
		free (descXML);
		descXML = NULL;
		GetUPNPUrls (&urls, &data, dev->descURL, 0);
	}

	freeUPNPDevlist(devlist);

	r = UPNP_GetExternalIPAddress(urls.controlURL, data.first.servicetype,
			IPAddress);

	if (r != 0)
	{
		Printf(PRINT_HIGH,
			"UPnP: Router found but unable to get external IP address\n");

		is_upnp_ok = false;
	}
	else
	{
		Printf(PRINT_HIGH, "UPnP: Router found, external IP address is: %s\n",
			IPAddress);

		// Store ip address just in case admin wants it
		sv_upnp_externalip.ForceSet(IPAddress);

		is_upnp_ok = true;
	}
}

void upnp_add_redir (const char * addr, int port)
{
	char port_str[16];
	int r;

	if (!sv_upnp || !is_upnp_ok)
		return;

	if (urls.controlURL == NULL)
		return;

	sprintf(port_str, "%d", port);

	// Set a description if none exists
	if (!sv_upnp_description.cstring()[0])
	{
		std::stringstream desc;

		desc << "Odasrv " << "(" << addr << ":" << port_str << ")";

		sv_upnp_description.Set(desc.str().c_str());
	}

	r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
			port_str, port_str, addr, sv_upnp_description.cstring(), "UDP", NULL, 0);

	if (r != 0)
	{
		Printf(PRINT_HIGH, "UPnP: AddPortMapping failed: %d\n", r);

		is_upnp_ok = false;
	}
	else
	{
		Printf(PRINT_HIGH, "UPnP: Port mapping added to router: %s",
			sv_upnp_description.cstring());

		is_upnp_ok = true;
	}
}

void upnp_rem_redir (int port)
{
	char port_str[16];
	int r;

	if (!is_upnp_ok)
		return;

	if(urls.controlURL == NULL)
		return;

	sprintf(port_str, "%d", port);
	r = UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype,
		port_str, "UDP", 0);

	if (r != 0)
	{
		Printf(PRINT_HIGH, "UPnP: DeletePortMapping failed: %d\n", r);
		is_upnp_ok = false;
	}
	else
		is_upnp_ok = true;
}
#endif

//
// UDPsocket
//
SOCKET UDPsocket (void)
{
	SOCKET s;

	// allocate a socket
	s = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET)
     	I_FatalError ("can't create socket");

	return s;
}

//
// BindToLocalPort
//
void BindToLocalPort (SOCKET s, u_short wanted)
{
	int v;
	struct sockaddr_in address;

	memset (&address, 0, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	u_short next = wanted;

	// denis - try several ports
	do
	{
		address.sin_port = htons(next++);

		v = bind (s, (sockaddr *)&address, sizeof(address));

		if(next > wanted + 32)
		{
			I_FatalError ("BindToPort: error");
			return;
		}
	}while (v == SOCKET_ERROR);

	char tmp[32] = "";
	sprintf(tmp, "%d", next - 1);
	port.ForceSet(tmp);

#ifdef ODA_HAVE_MINIUPNP
    std::string ip = NET_GetLocalAddress();

    if (!ip.empty())
    {
        sv_upnp_internalip.Set(ip.c_str());

        Printf(PRINT_HIGH, "UPnP: Internal IP address is: %s\n", ip.c_str());

        upnp_add_redir(ip.c_str(), next - 1);
    }
    else
    {
        Printf(PRINT_HIGH, "UPnP: Could not get first internal IP address, "
            "UPnP will not function\n");

        is_upnp_ok = false;
    }
#endif

	Printf(PRINT_HIGH, "Bound to local port %d\n", next - 1);
}


void CloseNetwork (void)
{
#ifdef ODA_HAVE_MINIUPNP
    upnp_rem_redir (port);
#endif

	closesocket (inet_socket);
#ifdef _WIN32
	WSACleanup ();
#endif
}


// this is from Quake source code :)

void SockadrToNetadr (struct sockaddr_in *s, netadr_t *a)
{
	 memcpy(&(a->ip), &(s->sin_addr), sizeof(struct in_addr));
     a->port = s->sin_port;
}

void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s)
{
     memset (s, 0, sizeof(*s));
     s->sin_family = AF_INET;

	 memcpy(&(s->sin_addr), &(a->ip), sizeof(struct in_addr));
     s->sin_port = a->port;
}

char *NET_AdrToString (netadr_t a)
{
     static  char    s[64];

     sprintf (s, "%i.%i.%i.%i:%i", a.ip[0], a.ip[1], a.ip[2], a.ip[3], ntohs(a.port));

     return s;
}

bool NET_StringToAdr (const char *s, netadr_t *a)
{
	 struct hostent  *h;
	 struct sockaddr_in sadr;
	 char	*colon;
	 char	copy[256];


	 memset (&sadr, 0, sizeof(sadr));
	 sadr.sin_family = AF_INET;

	 sadr.sin_port = 0;

	 strncpy (copy, s, sizeof(copy) - 1);
	 copy[sizeof(copy) - 1] = 0;

	 // strip off a trailing :port if present
	 for (colon = copy ; *colon ; colon++)
		if (*colon == ':')
		{
			*colon = 0;
			sadr.sin_port = htons(atoi(colon+1));
		}

	if (! (h = gethostbyname(copy)) )
		return 0;

	*(int *)&sadr.sin_addr = *(int *)h->h_addr_list[0];

	SockadrToNetadr (&sadr, a);

	return true;
}

bool NET_CompareAdr (netadr_t a, netadr_t b)
{
	if (a.ip[0] == b.ip[0] && a.ip[1] == b.ip[1] && a.ip[2] == b.ip[2] && a.ip[3] == b.ip[3] && a.port == b.port)
		return true;

	return false;
}

#ifdef _WIN32
typedef int socklen_t;
#endif

int NET_GetPacket (void)
{
	int				  ret;
	struct sockaddr_in   from;
	socklen_t			fromlen;

	fromlen = sizeof(from);
	net_message.clear();
	ret = recvfrom (inet_socket, (char *)net_message.ptr(), net_message.maxsize(), 0, (struct sockaddr *)&from, &fromlen);

	if (ret == -1)
	{
#ifdef _WIN32
		errno = WSAGetLastError();

		if (errno == WSAEWOULDBLOCK)
			return false;

		if (errno == WSAECONNRESET)
			return false;

		if (errno == WSAEMSGSIZE)
		{
			 Printf (PRINT_HIGH, "Warning:  Oversize packet from %s\n",
							 NET_AdrToString (net_from));
			 return false;
		}

		Printf (PRINT_HIGH, "NET_GetPacket: %s\n", strerror(errno));
		return false;
#else
		if (errno == EWOULDBLOCK)
			return false;
		if (errno == ECONNREFUSED)
			return false;

		Printf (PRINT_HIGH, "NET_GetPacket: %s\n", strerror(errno));
		return false;
#endif
	}
	net_message.setcursize(ret);
	SockadrToNetadr (&from, &net_from);

	return ret;
}

int NET_SendPacket (buf_t &buf, netadr_t &to)
{
	int				   ret;
	struct sockaddr_in	addr;

	// [SL] 2011-07-06 - Don't try to send a packet if we're not really connected
	// (eg, a netdemo is being played back)
	if (simulated_connection)
	{
		buf.clear();
		return 0;
	}

	NetadrToSockadr (&to, &addr);

#ifdef GEKKO
	ret = sendto(inet_socket, (const char *)buf.ptr(), buf.size(), 0, (struct sockaddr *)&addr, 8);	// 8 is important for online
#else
	ret = sendto(inet_socket, (const char *)buf.ptr(), buf.size(), 0, (struct sockaddr *)&addr, sizeof(addr));
#endif

	buf.clear();

	if (ret == -1)
	{
#ifdef _WIN32
		  int err = WSAGetLastError();

		  // wouldblock is silent
		  if (err == WSAEWOULDBLOCK)
			  return 0;
#else
		  if (errno == EWOULDBLOCK)
			  return 0;
		  if (errno == ECONNREFUSED)
			  return 0;
		  Printf (PRINT_HIGH, "NET_SendPacket: %s\n", strerror(errno));
#endif
	}

	return ret;
}


#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

std::string NET_GetLocalAddress (void)
{
	static char buff[HOST_NAME_MAX];
    hostent *ent;
    struct in_addr addr;

	gethostname(buff, HOST_NAME_MAX);
	buff[HOST_NAME_MAX - 1] = 0;

    ent = gethostbyname(buff);

    // Return the first, IPv4 address
    if (ent && ent->h_addrtype == AF_INET && ent->h_addr_list[0] != NULL)
    {
        addr.s_addr = *(u_long *)ent->h_addr_list[0];

		std::string ipstr = inet_ntoa(addr);
		Printf(PRINT_HIGH, "Bound to IP: %s\n", ipstr.c_str());
		return ipstr;
    }
	else
	{
		Printf(PRINT_HIGH, "Could not look up host IP address from hostname\n");
		return "";
	}
}


void SZ_Clear (buf_t *buf)
{
	buf->clear();
}

void SZ_Write (buf_t *b, const void *data, int length)
{
	b->WriteChunk((const char *)data, length);
}

void SZ_Write (buf_t *b, const byte *data, int startpos, int length)
{
	b->WriteChunk((const char *)data, length, startpos);
}

//
// MSG_WriteMarker
//
// denis - use this function to mark the start of your server message
// as it allows for better debugging and optimization of network code
//
// [ML] 8/4/10: Moved to sv_main and slightly modified to provide an adequate
//      but temporary fix for bug 594 until netcode_bringup2 is complete.
//      Thanks to spleen for providing good brainpower!
//
// [SL] 2011-07-17 - Moved back to i_net.cpp so that it can be used by
// both client & server code.  Client has a stub function for SV_SendPackets.
//
void SV_SendPackets(void);

//
// MSG_WriteMarker
//
// denis - use this function to mark the start of your client message
// as it allows for better debugging and optimization of network code
//
void MSG_WriteMarker (buf_t *b, clc_t c)
{
	if (simulated_connection)
		return;
	b->WriteByte((byte)c);
}

void MSG_WriteByte (buf_t *b, byte c)
{
	if (simulated_connection)
		return;
	b->WriteByte((byte)c);
}


void MSG_WriteChunk (buf_t *b, const void *p, unsigned l)
{
	if (simulated_connection)
		return;
	b->WriteChunk((const char *)p, l);
}

void MSG_WriteSVC(buf_t* b, const google::protobuf::Message& msg)
{
	if (simulated_connection)
		return;

	static std::string buffer;
	if (!msg.SerializeToString(&buffer))
	{
		Printf(
		    PRINT_WARNING,
		    "WARNING: Could not serialize message \"%s\".  This is most likely a bug.\n",
		    msg.GetDescriptor()->full_name().c_str());
		return;
	}

	// Do we actaully have room for this upcoming message?
	const size_t MAX_HEADER_SIZE = 4; // header + 3 bytes for varint size.
	if (b->cursize + MAX_HEADER_SIZE + msg.ByteSize() >= MAX_UDP_SIZE)
		SV_SendPackets();

	svc_t header = SVC_ResolveDescriptor(msg.GetDescriptor());
	if (header == svc_noop)
	{
		Printf(PRINT_WARNING,
		       "WARNING: Could not find svc header for message \"%s\".  This is most "
		       "likely a bug.\n",
		       msg.GetDescriptor()->full_name().c_str());
		return;
	}

#if 0
	Printf("%s (%d)\n, %s\n",
		::svc_info[header].getName(), msg.ByteSize(),
		msg.ShortDebugString().c_str());
#endif

	b->WriteByte(header);
	b->WriteUnVarint(buffer.size());
	b->WriteChunk(buffer.data(), buffer.size());
}

/**
 * @brief Broadcast message to all players.
 * 
 * @param buf Type of buffer to broadcast in, per player.
 * @param msg Message to broadcast to all players.
 * @param skip If passed, skip this player id.
 */
void MSG_BroadcastSVC(const clientBuf_e buf, const google::protobuf::Message& msg,
                      const int skipPlayer)
{
	if (simulated_connection)
		return;

	static std::string buffer;
	if (!msg.SerializeToString(&buffer))
	{
		Printf(
		    PRINT_WARNING,
		    "WARNING: Could not serialize message \"%s\".  This is most likely a bug.\n",
		    msg.GetDescriptor()->full_name().c_str());
		return;
	}

	svc_t header = SVC_ResolveDescriptor(msg.GetDescriptor());
	if (header == svc_noop)
	{
		Printf(PRINT_WARNING,
		       "WARNING: Could not find svc header for message \"%s\".  This is most "
		       "likely a bug.\n",
		       msg.GetDescriptor()->full_name().c_str());
		return;
	}

	for (Players::iterator it = ::players.begin(); it != ::players.end(); ++it)
	{
		if (!it->ingame())
			continue;

		if (static_cast<int>(it->id) == skipPlayer)
			continue;

		// Select the correct buffer.
		buf_t* b = buf == CLBUF_RELIABLE ? &it->client.reliablebuf : &it->client.netbuf;

		// Do we actaully have room for this upcoming message?
		const size_t MAX_HEADER_SIZE = 4; // header + 3 bytes for varint size.
		if (b->cursize + MAX_HEADER_SIZE + msg.ByteSize() >= MAX_UDP_SIZE)
			SV_SendPackets();

		b->WriteByte(header);
		b->WriteUnVarint(buffer.size());
		b->WriteChunk(buffer.data(), buffer.size());
	}
}

void MSG_WriteShort (buf_t *b, short c)
{
	if (simulated_connection)
		return;
	b->WriteShort(c);
}


void MSG_WriteLong (buf_t *b, int c)
{
	if (simulated_connection)
		return;
	b->WriteLong(c);
}

void MSG_WriteUnVarint(buf_t* b, unsigned int uv)
{
	if (simulated_connection)
		return;
	b->WriteUnVarint(uv);
}

void MSG_WriteVarint(buf_t* b, int v)
{
	if (simulated_connection)
		return;
	b->WriteVarint(v);
}

//
// MSG_WriteBool
//
// Write an boolean value to a buffer
void MSG_WriteBool(buf_t *b, bool Boolean)
{
	if (simulated_connection)
		return;
	MSG_WriteByte(b, Boolean ? 1 : 0);
}

//
// MSG_WriteFloat
//
// Write a floating point number to a buffer
void MSG_WriteFloat(buf_t *b, float Float)
{
	if (simulated_connection)
		return;

    std::stringstream StringStream;

    StringStream << Float;

	MSG_WriteString(b, (char *)StringStream.str().c_str());
}

//
// MSG_WriteString
//
// Write a string to a buffer and null terminate it
void MSG_WriteString (buf_t *b, const char *s)
{
	if (simulated_connection)
		return;
	b->WriteString(s);
}

unsigned int toInt(char c)
{
  if (c >= '0' && c <= '9') return      c - '0';
  if (c >= 'A' && c <= 'F') return 10 + c - 'A';
  if (c >= 'a' && c <= 'f') return 10 + c - 'a';
  return -1;
}

//
// MSG_WriteHexString
//
// Converts a hexidecimal string to its binary representation
void MSG_WriteHexString(buf_t *b, const char *s)
{
    byte output[255];

    // Nothing to write?
    if (!(s && (*s)))
    {
        MSG_WriteByte(b, 0);
        return;
    }

    const size_t numdigits = strlen(s) / 2;

    if (numdigits > ARRAY_LENGTH(output))
    {
        Printf (PRINT_HIGH, "MSG_WriteHexString: too many digits\n");
        return;
    }

    for (size_t i = 0; i < numdigits; ++i)
    {
        output[i] = (char)(16 * toInt(s[2*i]) + toInt(s[2*i+1]));
    }

    MSG_WriteByte(b, (byte)numdigits);

    MSG_WriteChunk(b, output, numdigits);
}

int MSG_BytesLeft(void)
{
	return net_message.BytesLeftToRead();
}

int MSG_ReadByte (void)
{
    return net_message.ReadByte();
}

int MSG_NextByte (void)
{
	return net_message.NextByte();
}

void *MSG_ReadChunk (const size_t &size)
{
	return net_message.ReadChunk(size);
}

size_t MSG_SetOffset (const size_t &offset, const buf_t::seek_loc_t &loc)
{
    return net_message.SetOffset(offset, loc);
}

// Output buffer size for LZO compression, extra space in case uncompressable
#define OUT_LEN(a)      ((a) + (a) / 16 + 64 + 3)

// size above which packets get compressed (empirical), does not apply to adaptive compression
#define MINILZO_COMPRESS_MINPACKETSIZE	0xFF

//
// MSG_DecompressMinilzo
//
bool MSG_DecompressMinilzo ()
{
	// decompress back onto the receive buffer
	size_t left = MSG_BytesLeft();

	if(decompressed.maxsize() < net_message.maxsize())
		decompressed.resize(net_message.maxsize());

	lzo_uint newlen = net_message.maxsize();

	unsigned int r = lzo1x_decompress_safe (net_message.ptr() + net_message.BytesRead(), left, decompressed.ptr(), &newlen, NULL);

	if(r != LZO_E_OK)
	{
		Printf(PRINT_HIGH, "Error: minilzo packet decompression failed with error %X\n", r);
		return false;
	}

	net_message.clear();
	memcpy(net_message.ptr(), decompressed.ptr(), newlen);

	net_message.cursize = newlen;

	return true;
}

//
// MSG_CompressMinilzo
//
bool MSG_CompressMinilzo (buf_t &buf, size_t start_offset, size_t write_gap)
{
	if(buf.size() < MINILZO_COMPRESS_MINPACKETSIZE)
		return false;

	lzo_uint outlen = OUT_LEN(buf.maxsize() - start_offset - write_gap);
	size_t total_len = outlen + start_offset + write_gap;

	if(compressed.maxsize() < total_len)
		compressed.resize(total_len);

	int r = lzo1x_1_compress (buf.ptr() + start_offset,
							  buf.size() - start_offset,
							  compressed.ptr() + start_offset + write_gap,
							  &outlen,
							  wrkmem);

	// worth the effort?
	if(r != LZO_E_OK || outlen >= (buf.size() - start_offset - write_gap))
		return false;

	memcpy(compressed.ptr(), buf.ptr(), start_offset);

	SZ_Clear(&buf);
	MSG_WriteChunk(&buf, compressed.ptr(), outlen + start_offset + write_gap);

	return true;
}

//
// MSG_DecompressAdaptive
//
bool MSG_DecompressAdaptive (huffman &huff)
{
	// decompress back onto the receive buffer
	size_t left = MSG_BytesLeft();

	if(decompressed.maxsize() < net_message.maxsize())
		decompressed.resize(net_message.maxsize());

	size_t newlen = net_message.maxsize();

	bool r = huff.decompress (net_message.ptr() + net_message.BytesRead(), left, decompressed.ptr(), newlen);

	if(!r)
		return false;

	net_message.clear();
	memcpy(net_message.ptr(), decompressed.ptr(), newlen);

	net_message.cursize = newlen;

	return true;
}

//
// MSG_CompressAdaptive
//
bool MSG_CompressAdaptive (huffman &huff, buf_t &buf, size_t start_offset, size_t write_gap)
{
	size_t outlen = OUT_LEN(buf.maxsize() - start_offset - write_gap);
	size_t total_len = outlen + start_offset + write_gap;

	if(compressed.maxsize() < total_len)
		compressed.resize(total_len);

	bool r = huff.compress (buf.ptr() + start_offset,
							  buf.size() - start_offset,
							  compressed.ptr() + start_offset + write_gap,
							  outlen);

	// worth the effort?
	if(!r || outlen >= (buf.size() - start_offset - write_gap))
		return false;

	memcpy(compressed.ptr(), buf.ptr(), start_offset);

	SZ_Clear(&buf);
	MSG_WriteChunk(&buf, compressed.ptr(), outlen + start_offset + write_gap);

	return true;
}

int MSG_ReadShort (void)
{
    return net_message.ReadShort();
}

int MSG_ReadLong (void)
{
	return net_message.ReadLong();
}

unsigned int MSG_ReadUnVarint()
{
	return net_message.ReadUnVarint();
}

int MSG_ReadVarint()
{
	return net_message.ReadVarint();
}

//
// MSG_ReadBool
//
// Read a boolean value
bool MSG_ReadBool(void)
{
    int Value = net_message.ReadByte();

    if (Value < 0 || Value > 1)
    {
        DPrintf("MSG_ReadBool: Value is not 0 or 1, possibly corrupted packet");

        return (Value ? true : false);
    }

    return (Value ? true : false);
}

//
// MSG_ReadString
//
// Read a null terminated string
const char *MSG_ReadString (void)
{
	return net_message.ReadString();
}

//
// MSG_ReadFloat
//
// Read a floating point number
float MSG_ReadFloat(void)
{
    std::stringstream StringStream;
    float Float;

    StringStream << MSG_ReadString();

    StringStream >> Float;

    return Float;
}

/**
 * @brief Initialize a svc_info member.
 * 
 * @detail do-while is used to force a semicolon afterwards.
 */
#define SVC_INFO(n)                    \
	do                                 \
	{                                  \
		::svc_info[n].id = n;          \
		::svc_info[n].msgName = #n;    \
		::svc_info[n].msgFormat = "x"; \
	} while (false)

/**
 * @brief Initialize a clc_info member.
 *
 * @detail do-while is used to force a semicolon afterwards.
 */
#define CLC_INFO(n)                    \
	do                                 \
	{                                  \
		::clc_info[n].id = n;          \
		::clc_info[n].msgName = #n;    \
		::clc_info[n].msgFormat = "x"; \
	} while (false)

//
// InitNetMessageFormats
//
static void InitNetMessageFormats()
{
	// Server Messages.
	SVC_INFO(svc_noop);
	SVC_INFO(svc_disconnect);
	SVC_INFO(svc_playerinfo);
	SVC_INFO(svc_moveplayer);
	SVC_INFO(svc_updatelocalplayer);
	SVC_INFO(svc_levellocals);
	SVC_INFO(svc_pingrequest);
	SVC_INFO(svc_updateping);
	SVC_INFO(svc_spawnmobj);
	SVC_INFO(svc_disconnectclient);
	SVC_INFO(svc_loadmap);
	SVC_INFO(svc_consoleplayer);
	SVC_INFO(svc_explodemissile);
	SVC_INFO(svc_removemobj);
	SVC_INFO(svc_userinfo);
	SVC_INFO(svc_updatemobj);
	SVC_INFO(svc_spawnplayer);
	SVC_INFO(svc_damageplayer);
	SVC_INFO(svc_killmobj);
	SVC_INFO(svc_fireweapon);
	SVC_INFO(svc_updatesector);
	SVC_INFO(svc_print);
	SVC_INFO(svc_playermembers);
	SVC_INFO(svc_teammembers);
	SVC_INFO(svc_activateline);
	SVC_INFO(svc_movingsector);
	SVC_INFO(svc_playsound);
	SVC_INFO(svc_reconnect);
	SVC_INFO(svc_exitlevel);
	SVC_INFO(svc_touchspecial);
	SVC_INFO(svc_forceteam);
	SVC_INFO(svc_switch);
	SVC_INFO(svc_say);
	SVC_INFO(svc_spawnhiddenplayer);
	SVC_INFO(svc_updatedeaths);
	SVC_INFO(svc_ctfrefresh);
	SVC_INFO(svc_ctfevent);
	SVC_INFO(svc_serversettings);
	SVC_INFO(svc_connectclient);
    SVC_INFO(svc_midprint);
	SVC_INFO(svc_servergametic);
	SVC_INFO(svc_inttimeleft);
	SVC_INFO(svc_fullupdatedone);
	SVC_INFO(svc_railtrail);
	SVC_INFO(svc_playerstate);
	SVC_INFO(svc_levelstate);
	SVC_INFO(svc_resetmap);
	SVC_INFO(svc_playerqueuepos);
	SVC_INFO(svc_fullupdatestart);
	SVC_INFO(svc_lineupdate);
	SVC_INFO(svc_sectorproperties);
	SVC_INFO(svc_linesideupdate);
	SVC_INFO(svc_mobjstate);
	SVC_INFO(svc_damagemobj);
	SVC_INFO(svc_executelinespecial);
	SVC_INFO(svc_executeacsspecial);
	SVC_INFO(svc_thinkerupdate);
	SVC_INFO(svc_netdemocap);
	SVC_INFO(svc_netdemostop);
	SVC_INFO(svc_netdemoloadsnap);
	SVC_INFO(svc_vote_update);
	SVC_INFO(svc_maplist);
	SVC_INFO(svc_maplist_update);
	SVC_INFO(svc_maplist_index);
	SVC_INFO(svc_toast);
	SVC_INFO(svc_hordeinfo);
	SVC_INFO(svc_max);

	// Client Messages.
	CLC_INFO(clc_abort);
	CLC_INFO(clc_reserved1);
	CLC_INFO(clc_disconnect);
	CLC_INFO(clc_say);
	CLC_INFO(clc_move);
	CLC_INFO(clc_userinfo);
	CLC_INFO(clc_pingreply);
	CLC_INFO(clc_rate);
	CLC_INFO(clc_ack);
	CLC_INFO(clc_rcon);
	CLC_INFO(clc_rcon_password);
	CLC_INFO(clc_changeteam);
	CLC_INFO(clc_ctfcommand);
	CLC_INFO(clc_spectate);
	CLC_INFO(clc_wantwad);
	CLC_INFO(clc_kill);
	CLC_INFO(clc_cheat);
	CLC_INFO(clc_callvote);
	CLC_INFO(clc_maplist);
	CLC_INFO(clc_maplist_update);
	CLC_INFO(clc_getplayerinfo);
	CLC_INFO(clc_netcmd);
	CLC_INFO(clc_spy);
	CLC_INFO(clc_privmsg);
	CLC_INFO(clc_max);
}

#undef SVC_INFO
#undef CLC_INFO

CVAR_FUNC_IMPL(net_rcvbuf)
{
	int n = var.asInt();
	if (setsockopt(inet_socket, SOL_SOCKET, SO_RCVBUF, SETSOCKOPTCAST(&n), (int) sizeof(n)) == -1) {
		Printf(PRINT_HIGH, "setsockopt SO_RCVBUF: %s", strerror(errno));
	} else {
		Printf(PRINT_HIGH, "net_rcvbuf set to %d\n", n);
	}
}

CVAR_FUNC_IMPL(net_sndbuf)
{
	int n = var.asInt();
	if (setsockopt(inet_socket, SOL_SOCKET, SO_SNDBUF, SETSOCKOPTCAST(&n), (int) sizeof(n)) == -1) {
		Printf (PRINT_HIGH, "setsockopt SO_SNDBUF: %s", strerror(errno));
	} else {
		Printf(PRINT_HIGH, "net_sndbuf set to %d\n", n);
	}
}


//
// InitNetCommon
//
void InitNetCommon(void)
{
   unsigned long _true = true;

#ifdef _WIN32
   WSADATA   wsad;
   WSAStartup( MAKEWORD(2,2), &wsad );
#endif

   inet_socket = UDPsocket ();

    #ifdef ODA_HAVE_MINIUPNP
    init_upnp();
    #endif

   BindToLocalPort (inet_socket, localport);
   if (ioctlsocket(inet_socket, FIONBIO, &_true) == -1)
       I_FatalError ("UDPsocket: ioctl FIONBIO: %s", strerror(errno));

	// enter message information into message info structs
	InitNetMessageFormats();

   SZ_Clear(&net_message);
}

//
// NetWaitOrTimeout
//
// denis - yields CPU control briefly; shorter wait when data is available
//
bool NetWaitOrTimeout(size_t ms)
{
	struct timeval timeout = {0, int(1000*ms) + 1};
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(inet_socket, &fds);

	int ret = select(inet_socket + 1, &fds, NULL, NULL, &timeout);

	if(ret == 1)
		return true;

	#ifdef _WIN32
		// handle SOCKET_ERROR
		if(ret == SOCKET_ERROR)
			Printf(PRINT_HIGH, "select returned SOCKET_ERROR: %d\n", WSAGetLastError());
	#else
		// handle -1
		if(ret == -1 && ret != EINTR)
			Printf(PRINT_HIGH, "select returned -1: %s\n", strerror(errno));
	#endif

	return false;
}

void I_SetPort(netadr_t &addr, int port)
{
   addr.port = htons(port);
}

VERSION_CONTROL (i_net_cpp, "$Id$")
