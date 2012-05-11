// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2000-2006 by Sergey Makovkin (CSDoom .62)
// Copyright (C) 2006-2012 by The Odamex Team
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

// Max packet size to send and receive, in bytes
#define	MAX_UDP_PACKET	1400

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

class buf_t
{
public:
	byte	*data;
	size_t	allocsize, cursize, readpos;
	bool	overflowed;  // set to true if the buffer size failed

public:

	void WriteByte(byte b)
	{
		byte *buf = SZ_GetSpace(sizeof(b));
		
		if(!overflowed)
		{
			*buf = b;
		}
	}

	void WriteShort(short s)
	{
		byte *buf = SZ_GetSpace(sizeof(s));
		
		if(!overflowed)
		{
			buf[0] = s&0xff;
			buf[1] = s>>8;
		}
	}

	void WriteLong(int l)
	{
		byte *buf = SZ_GetSpace(sizeof(l));
		
		if(!overflowed)
		{
			buf[0] = l&0xff;
			buf[1] = (l>>8)&0xff;
			buf[2] = (l>>16)&0xff;
			buf[3] = l>>24;
		}
	}

	void WriteString(const char *c)
	{
		if(c && *c)
		{
			size_t l = strlen(c);
			byte *buf = SZ_GetSpace(l + 1);

			if(!overflowed)
			{
				memcpy(buf, c, l + 1);
			}
		}
		else
			WriteByte(0);
	}

	void WriteChunk(const char *c, unsigned l, int startpos = 0)
	{
		byte *buf = SZ_GetSpace(l);

		if(!overflowed)
		{
			memcpy(buf, c + startpos, l);
		}
	}

	int ReadByte()
	{
		if(readpos+1 > cursize)
		{
			overflowed = true;
			return -1;
		}
		return (unsigned char)data[readpos++];
	}

	int NextByte()
	{
		if(readpos+1 > cursize)
		{
			overflowed = true;
			return -1;
		}
		return (unsigned char)data[readpos];
	}

	byte *ReadChunk(size_t size)
	{
		if(readpos+size > cursize)
		{
			overflowed = true;
			return NULL;
		}
		size_t oldpos = readpos;
		readpos += size;
		return data+oldpos;
	}

	int ReadShort()
	{
		if(readpos+2 > cursize)
		{
			overflowed = true;
			return -1;
		}
		size_t oldpos = readpos;
		readpos += 2;
		return (short)(data[oldpos] + (data[oldpos+1]<<8));
	}

	int ReadLong()
	{
		if(readpos+4 > cursize)
		{
			overflowed = true;
			return -1;
		}
		size_t oldpos = readpos;
		readpos += 4;
		return data[oldpos] +
				(data[oldpos+1]<<8) +
				(data[oldpos+2]<<16)+
				(data[oldpos+3]<<24);
	}

	const char *ReadString()
	{
		byte *begin = data + readpos;

		while(ReadByte() > 0);

		if(overflowed)
		{
			return "";
		}

		return (const char *)begin;
	}

	size_t BytesLeftToRead()
	{
		return overflowed || cursize < readpos ? 0 : cursize - readpos;
	}

	size_t BytesRead()
	{
		return readpos;
	}

	byte *ptr()
	{
		return data;
	}

	size_t size()
	{
		return cursize;
	}
	
	size_t maxsize()
	{
		return allocsize;
	}

	void setcursize(size_t len)
	{
		cursize = len > allocsize ? allocsize : len;
	}

	void clear()
	{
		cursize = 0;
		readpos = 0;
		overflowed = false;
	}

	void resize(size_t len)
	{
		if(data)
			delete[] data;
		
		data = new byte[len];
		allocsize = len;
		cursize = 0;
		readpos = 0;
		overflowed = false;
	}

	byte *SZ_GetSpace(size_t length)
	{
		if (cursize + length >= allocsize)
		{
			clear();
			overflowed = true;
		}

		byte *ret = data + cursize;
		cursize += length;

		return ret;
	}

	buf_t &operator =(const buf_t &other)
	{
		if (this == &other)
            return *this;

		if(data)
			delete[] data;
		
		data = new byte[other.allocsize];
		allocsize = other.allocsize;
		cursize = other.cursize;
		overflowed = other.overflowed;
		readpos = other.readpos;

		if(!overflowed)
			for(size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];

		return *this;
	}
	
	buf_t()
		: data(0), allocsize(0), cursize(0), readpos(0), overflowed(false)
	{
	}
	buf_t(size_t len)
		: data(new byte[len]), allocsize(len), cursize(0), readpos(0), overflowed(false)
	{
	}
	buf_t(const buf_t &other)
		: data(new byte[other.allocsize]), allocsize(other.allocsize), cursize(other.cursize), readpos(other.readpos), overflowed(other.overflowed)
		
	{
		if(!overflowed)
			for(size_t i = 0; i < cursize; i++)
				data[i] = other.data[i];
	}
	~buf_t()
	{
		if(data)
			delete[] data;
	}
};

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

#endif
