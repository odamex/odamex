// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62)
// Copyright (C) 2006-2007 by The Odamex Team
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


#ifndef __I_NET_H__
#define __I_NET_H__

#define	MAX_UDP_PACKET 8192
#define MASTERPORT     15000
#define LAUNCHERPORT   12999

typedef unsigned char byte;

#define CHALLENGE          5560020  // challenge
#define SERVER_CHALLENGE   5560020  // doomsv challenge
#define LAUNCHER_CHALLENGE 777123  // csdl challenge

extern int localport;
extern int msg_badread;

typedef struct
{
   byte ip[4];
   unsigned short port;
   unsigned short pad;
} netadr_t;

extern netadr_t net_from;  // address of who sent the packet


typedef struct buf_s
{
   byte		*data;
   size_t	maxsize;
   size_t	cursize;
   bool		overflowed;  // set to true if the buffer size failed

   void clear()
   {
	   cursize = 0;
	   overflowed = false;
   }

   buf_s(const buf_s &other) : data(0), maxsize(other.maxsize), cursize(other.cursize), overflowed(other.overflowed)
   {
	   data = new byte[maxsize];
	   memcpy(data, other.data, maxsize);
   }
   buf_s(size_t n)
   {
	   data = new byte[n];
	   maxsize = n;
	   clear();
   }
   ~buf_s()
   {
	   if(data)
		delete[] data;
   }

	buf_s &operator =(const buf_s &other)
	{
		if(data)
			delete[] data;
		
		data = new byte[other.maxsize];
		maxsize = other.maxsize;
		cursize = other.cursize;
		overflowed = other.overflowed;

		if(!overflowed)
			for(size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];

		return *this;
	}

} buf_t;

extern buf_t net_message;

void CloseNetwork(void);
void InitNetCommon(void);
void I_SetPort(netadr_t &addr, int port);
void I_DoSelect(void);

char *NET_AdrToString(netadr_t a, bool displayport = true);
bool NET_StringToAdr(char *s, netadr_t *a);
bool NET_CompareAdr(netadr_t a, netadr_t b);
int  NET_GetPacket(void);
void NET_SendPacket(int length, byte *data, netadr_t to);

void SZ_Clear(buf_t *buf);
void SZ_Write(buf_t *b, void *data, int length);

void MSG_WriteByte(buf_t *b, int c);
void MSG_WriteShort(buf_t *b, int c);
void MSG_WriteLong(buf_t *b, int c);
void MSG_WriteString(buf_t *b, char *s);

int MSG_ReadByte(void);
int MSG_ReadShort(void);
int MSG_ReadLong(void);
char *MSG_ReadString(void);

int MSG_BytesLeft (void);

#endif
