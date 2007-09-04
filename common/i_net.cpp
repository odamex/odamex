// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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


/* [Petteri] Check if compiling for Win32:	*/
//#if defined(__WINDOWS__) || defined(__NT__) || defined(_MSC_VER) || defined(WIN32)
//#	define WIN32
//#endif
/* Follow #ifdef __WIN32__ marks */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* [Petteri] Use Winsock for Win32: */
#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <winsock.h>
#else
#	include <sys/types.h>
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <errno.h>
#	include <unistd.h>
#	include <netdb.h>
#	include <sys/ioctl.h>
#	include <sys/time.h>
#endif

#ifndef _WIN32
typedef int SOCKET;
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define closesocket close
#define ioctlsocket ioctl
#define Sleep(x)	usleep (x * 1000)
#endif

#include "doomtype.h"

#include "i_system.h"

#include "d_event.h"
#include "d_net.h"
#include "m_argv.h"
#include "m_alloc.h"
#include "m_swap.h"
#include "doomstat.h"
#include "d_player.h"
#include "g_game.h"
#include "i_net.h"

#include "minilzo.h"

int         net_socket;
int         localport;
netadr_t    net_from;   // address of who sent the packet

buf_t       net_message(MAX_UDP_PACKET);
size_t      msg_readcount;
int         msg_badread;

// buffer for compression/decompression
// can't be static to a function because some
// of the functions
buf_t compressed, decompressed;
lzo_byte wrkmem[LZO1X_1_MEM_COMPRESS];

CVAR(port,		"0", CVAR_NOSET | CVAR_NOENABLEDISABLE)

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

		if(next > wanted + 16)
		{
			I_FatalError ("BindToPort: error");
			return;
		}
	}while (v == SOCKET_ERROR);

	char tmp[32] = "";
	sprintf(tmp, "%d", next - 1);
	port.ForceSet(tmp);

	Printf(PRINT_HIGH, "Bound to local port %d\n", next - 1);
}


void CloseNetwork (void)
{
	closesocket (net_socket);
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
     char    *colon;
     char    copy[256];


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

     if (copy[0] >= '0' && copy[0] <= '9')
     {
          *(int *)&sadr.sin_addr = inet_addr(copy);
     }
     else
     {
          if (! (h = gethostbyname(copy)) )
                return 0;
          *(int *)&sadr.sin_addr = *(int *)h->h_addr_list[0];
     }

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
    int                  ret;
    struct sockaddr_in   from;
    socklen_t            fromlen;

    fromlen = sizeof(from);
    ret = recvfrom (net_socket, (char *)net_message.ptr(), net_message.maxsize(), 0, (struct sockaddr *)&from, &fromlen);

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

    msg_readcount = 0;
    msg_badread = false;

    return ret;
}

void NET_SendPacket (buf_t &buf, netadr_t &to)
{
    int                   ret;
    struct sockaddr_in    addr;

    NetadrToSockadr (&to, &addr);

	ret = sendto (net_socket, (const char *)buf.ptr(), buf.size(), 0, (struct sockaddr *)&addr, sizeof(addr));

	buf.clear();

    if (ret == -1)
    {
#ifdef _WIN32
          int err = WSAGetLastError();

          // wouldblock is silent
          if (err == WSAEWOULDBLOCK)
              return;
#else
          if (errno == EWOULDBLOCK)
              return;
          if (errno == ECONNREFUSED)
              return;
          Printf (PRINT_HIGH, "NET_SendPacket: %s\n", strerror(errno));
#endif
    }
}

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 512
#endif

void NET_GetLocalAddress (void)
{
	char buff[HOST_NAME_MAX];
	struct sockaddr_in address;
	socklen_t namelen;
	netadr_t net_local_adr;

	gethostname(buff, HOST_NAME_MAX);
	buff[HOST_NAME_MAX - 1] = 0;

	NET_StringToAdr (buff, &net_local_adr);

	namelen = sizeof(address);
	if (getsockname (net_socket, (struct sockaddr *)&address, &namelen) == -1)
        Printf (PRINT_HIGH, "NET_Init: getsockname:", strerror(errno));

	net_local_adr.port = address.sin_port;

	Printf(PRINT_HIGH, "IP address %s\n", NET_AdrToString (net_local_adr) );
}


void SZ_Clear (buf_t *buf)
{
	buf->clear();
}


byte *SZ_GetSpace(buf_t *b, int length)
{
	return b->SZ_GetSpace(length);
}

void SZ_Write (buf_t *b, const void *data, int length)
{
    memcpy (SZ_GetSpace(b, length), data, length);
}

void SZ_Write (buf_t *b, const byte *data, int startpos, int length)
{
	data += startpos;
    memcpy (SZ_GetSpace(b, length), data, length);
}

//
// MSG_WriteMarker
//
// denis - use this function to mark the start of your server message
// as it allows for better debugging and optimization of network code
//
void MSG_WriteMarker (buf_t *b, svc_t c)
{
    byte    *buf;

    buf = SZ_GetSpace (b, 1);
    buf[0] = c;
}

//
// MSG_WriteMarker
//
// denis - use this function to mark the start of your client message
// as it allows for better debugging and optimization of network code
//
void MSG_WriteMarker (buf_t *b, clc_t c)
{
    byte    *buf;

    buf = SZ_GetSpace (b, 1);
    buf[0] = c;
}

void MSG_WriteByte (buf_t *b, int c)
{
    byte    *buf;

    buf = SZ_GetSpace (b, 1);
    buf[0] = c;
}


void MSG_WriteChunk (buf_t *b, const void *p, unsigned l)
{
    byte    *buf;

    buf = SZ_GetSpace (b, l);
    memcpy(buf, p, l);
}


void MSG_WriteShort (buf_t *b, int c)
{
    byte    *buf;

    buf = SZ_GetSpace (b, 2);
    buf[0] = c&0xff;
    buf[1] = c>>8;
}


void MSG_WriteLong (buf_t *b, int c)
{
    byte    *buf;

    buf = SZ_GetSpace(b, 4);
    buf[0] = c&0xff;
    buf[1] = (c>>8)&0xff;
    buf[2] = (c>>16)&0xff;
    buf[3] = c>>24;
}


void MSG_WriteString (buf_t *b, const char *s)
{
    if (!s)
        MSG_WriteByte(b, 0);
    else
	{
        SZ_Write (b, s, strlen(s));
        MSG_WriteByte(b, 0);
	}
}

int MSG_BytesLeft(void)
{
	if(net_message.cursize < msg_readcount)
		return 0;

	return net_message.cursize - msg_readcount;
}

int MSG_ReadByte (void)
{
    int     c;

    if (msg_readcount+1 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = (unsigned char)net_message.data[msg_readcount];
    msg_readcount++;

    return c;
}


int MSG_NextByte (void)
{
    int     c;

    if (msg_readcount+1 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = (unsigned char)net_message.data[msg_readcount];

    return c;
}

void *MSG_ReadChunk (size_t &size)
{
    if (msg_readcount+size > net_message.cursize)
    {
        msg_badread = true;
    }

    void *c = net_message.data + msg_readcount;
    msg_readcount += size;

    return c;
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

	unsigned int r = lzo1x_decompress_safe (net_message.ptr() + msg_readcount, left, decompressed.data, &newlen, NULL);

	if(r != LZO_E_OK)
	{
		msg_badread = false;
		msg_readcount = net_message.cursize;
		Printf(PRINT_HIGH, "Error: minilzo packet decompression failed with error %X\n", r);
		return false;
	}

	memcpy(net_message.ptr(), decompressed.data, newlen);

	msg_badread = false;
	msg_readcount = 0;
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

	bool r = huff.decompress (net_message.ptr() + msg_readcount, left, decompressed.ptr(), newlen);

	if(!r)
		return false;

	memcpy(net_message.ptr(), decompressed.ptr(), newlen);

	msg_badread = false;
	msg_readcount = 0;
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
    int     c;

    if (msg_readcount+2 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = (short)(net_message.data[msg_readcount]
    + (net_message.data[msg_readcount+1]<<8));

    msg_readcount += 2;

    return c;
}

int MSG_ReadLong (void)
{
    int c;

	if (msg_readcount+4 > net_message.cursize)
    {
        msg_badread = true;
        return -1;
    }

    c = net_message.data[msg_readcount]
     + (net_message.data[msg_readcount+1]<<8)
     + (net_message.data[msg_readcount+2]<<16)
     + (net_message.data[msg_readcount+3]<<24);

    msg_readcount += 4;

    return c;
}

char *MSG_ReadString (void)
{
    static char     string[2048];
    signed char     c;
    unsigned int    l;

    l = 0;
	if(MSG_BytesLeft())
    do
    {
       c = (signed char)MSG_ReadByte();
       if (c == 0)
           break;
       string[l] = c;
       l++;
	} while (l < sizeof(string)-1 && MSG_BytesLeft());

    string[l] = 0;

    return string;
}


//
// InitNetCommon
//
void InitNetCommon(void)
{
   unsigned long _true = true;

#ifdef _WIN32
   WSADATA   wsad;
   WSAStartup( 0x0101, &wsad );
#endif

   net_socket = UDPsocket ();
   BindToLocalPort (net_socket, localport);
   if (ioctlsocket (net_socket, FIONBIO, &_true) == -1)
       I_FatalError ("UDPsocket: ioctl FIONBIO: %s", strerror(errno));

   SZ_Clear(&net_message);
}

//
// NetWaitOrTimeout
//
// denis - yields CPU control briefly; shorter wait when data is available
//
bool NetWaitOrTimeout(size_t ms)
{
	struct timeval timeout = {0, (1000*ms) + 1};
	fd_set fds;

	FD_ZERO(&fds);
	FD_SET(net_socket, &fds);

	int ret = select(net_socket + 1, &fds, NULL, NULL, &timeout);

	if(ret == 1)
		return true;

	#ifdef WIN32
		// handle SOCKET_ERROR
		if(ret == SOCKET_ERROR)
			Printf(PRINT_HIGH, "select returned SOCKET_ERROR: %d\n", WSAGetLastError());
	#else
		// handle -1
		if(ret < 0)
			Printf(PRINT_HIGH, "select returned %d: %d\n", ret, errno);
	#endif

	return false;
}

void I_SetPort(netadr_t &addr, int port)
{
   addr.port = htons(port);
}

VERSION_CONTROL (i_net_cpp, "$Id$")

