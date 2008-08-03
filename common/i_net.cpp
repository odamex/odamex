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
#include <stdarg.h>

#include <sstream>

// GhostlyDeath -- VC6 requires Map and sstream doesn't seem to have anything either
#if _MSC_VER <= 1200
#include <string>
#include <map>
#endif

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

// buffer for compression/decompression
// can't be static to a function because some
// of the functions
buf_t compressed, decompressed;
lzo_byte wrkmem[LZO1X_1_MEM_COMPRESS];

EXTERN_CVAR(port)

msg_info_t clc_info[clc_max];
msg_info_t svc_info[svc_max];

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
	net_message.clear();
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
void MSG_WriteMarker (buf_t *b, svc_t c)
{
	b->WriteByte((byte)c);
}

//
// MSG_WriteMarker
//
// denis - use this function to mark the start of your client message
// as it allows for better debugging and optimization of network code
//
void MSG_WriteMarker (buf_t *b, clc_t c)
{
	b->WriteByte((byte)c);
}

void MSG_WriteByte (buf_t *b, byte c)
{
    b->WriteByte((byte)c);
}


void MSG_WriteChunk (buf_t *b, const void *p, unsigned l)
{
    b->WriteChunk((const char *)p, l);
}


void MSG_WriteShort (buf_t *b, short c)
{
    b->WriteShort(c);
}


void MSG_WriteLong (buf_t *b, int c)
{
    b->WriteLong(c);
}

//
// MSG_WriteBool
//
// Write an boolean value to a buffer
void MSG_WriteBool(buf_t *b, bool Boolean)
{
    MSG_WriteByte(b, Boolean ? 1 : 0);
}

//
// MSG_WriteFloat
//
// Write a floating point number to a buffer
void MSG_WriteFloat(buf_t *b, float Float)
{
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
	b->WriteString(s);
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

void *MSG_ReadChunk (size_t &size)
{
	return net_message.ReadChunk(size);
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

//
// MSG_ReadString
// 
// Read a null terminated string
const char *MSG_ReadString (void)
{
	return net_message.ReadString();
}
// 
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

//
// InitNetMessageFormats
//
void InitNetMessageFormats()
{
#define MSG(name, format) {name, #name, format}
   msg_info_t clc_messages[] = {
      MSG(clc_abort,              "x"),
      MSG(clc_reserved1,          "x"),
      MSG(clc_disconnect,         "x"),
      MSG(clc_say,                "bs"),
      MSG(clc_move,               "x"),
      MSG(clc_userinfo,           "x"),
      MSG(clc_svgametic,          "N"),
      MSG(clc_rate,               "N"),
      MSG(clc_ack,                "x"),
      MSG(clc_rcon,               "s"),
      MSG(clc_rcon_password,      "x"),
      MSG(clc_changeteam,         "b"),
      MSG(clc_ctfcommand,         "x"),
      MSG(clc_spectate,           "b"),
      MSG(clc_wantwad,            "ssN"),
      MSG(clc_kill,               "x"),
      MSG(clc_cheat,              "x"),
      MSG(clc_launcher_challenge, "x"),
      MSG(clc_challenge,          "x")
   };

   msg_info_t svc_messages[] = {
	MSG(svc_abort,              "x"),
	MSG(svc_full,               "x"),
	MSG(svc_disconnect,         "x"),
	MSG(svc_reserved3,          "x"),
	MSG(svc_playerinfo,         "x"),
	MSG(svc_moveplayer,         "x"),
	MSG(svc_updatelocalplayer,  "x"),
	MSG(svc_svgametic,          "x"),
	MSG(svc_updateping,         "x"),
	MSG(svc_spawnmobj,          "x"),
	MSG(svc_disconnectclient,   "x"),
	MSG(svc_loadmap,            "x"),
	MSG(svc_consoleplayer,      "x"),
	MSG(svc_mobjspeedangle,     "x"),
	MSG(svc_explodemissile,     "x"),
	MSG(svc_removemobj,         "x"),
	MSG(svc_userinfo,           "x"),
	MSG(svc_movemobj,           "x"),
	MSG(svc_spawnplayer,        "x"),
	MSG(svc_damageplayer,       "x"),
	MSG(svc_killmobj,           "x"),
	MSG(svc_firepistol,         "x"),
	MSG(svc_fireshotgun,        "x"),
	MSG(svc_firessg,            "x"),
	MSG(svc_firechaingun,       "x"),
	MSG(svc_fireweapon,         "x"),
	MSG(svc_sector,             "x"),
	MSG(svc_print,              "x"),
	MSG(svc_mobjinfo,           "x"),
	MSG(svc_updatefrags,        "x"),
	MSG(svc_teampoints,         "x"),
	MSG(svc_activateline,       "x"),
	MSG(svc_movingsector,       "x"),
	MSG(svc_startsound,         "x"),
	MSG(svc_reconnect,          "x"),
	MSG(svc_exitlevel,          "x"),
	MSG(svc_touchspecial,       "x"),
	MSG(svc_changeweapon,       "x"),
	MSG(svc_reserved42,         "x"),
	MSG(svc_corpse,             "x"),
	MSG(svc_missedpacket,       "x"),
	MSG(svc_soundorigin,        "x"),
	MSG(svc_reserved46,         "x"),
	MSG(svc_reserved47,         "x"),
	MSG(svc_forceteam,          "x"),
	MSG(svc_switch,             "x"),
	MSG(svc_reserved50,         "x"),
	MSG(svc_reserved51,         "x"),
	MSG(svc_spawnhiddenplayer,  "x"),
	MSG(svc_updatedeaths,       "x"),
	MSG(svc_ctfevent,           "x"),
	MSG(svc_serversettings,     "x"),
	MSG(svc_spectate,           "x"),
	MSG(svc_mobjstate,          "x"),
	MSG(svc_actor_movedir,      "x"),
	MSG(svc_actor_target,       "x"),
	MSG(svc_actor_tracer,       "x"),
	MSG(svc_damagemobj,         "x"),
	MSG(svc_wadinfo,            "x"),
	MSG(svc_wadchunk,           "x"),
	MSG(svc_compressed,         "x"),
	MSG(svc_launcher_challenge, "x"),
	MSG(svc_challenge,          "x"),
	MSG(svc_connectclient,		"x")
   };

   size_t i;

   for(i = 0; i < sizeof(clc_messages)/sizeof(*clc_messages); i++)
   {
      clc_info[clc_messages[i].id] = clc_messages[i];
   }

   for(i = 0; i < sizeof(svc_messages)/sizeof(*svc_messages); i++)
   {
      svc_info[svc_messages[i].id] = svc_messages[i];
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
   WSAStartup( 0x0101, &wsad );
#endif

   net_socket = UDPsocket ();
   BindToLocalPort (net_socket, localport);
   if (ioctlsocket (net_socket, FIONBIO, &_true) == -1)
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


