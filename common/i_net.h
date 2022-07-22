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

#pragma once

#include "huffman.h"


// Default buffer size for a UDP packet.
// This constant seems to be used as a default buffer size and should
// probably not be considered a reasonable MTU.
#define MAX_UDP_PACKET 8192

// Maximum safe size for a packet transmitted over UDP.
// This number comes from Steamworks and seems to be a reasonable default.
#define MAX_UDP_SIZE 1200

#define SERVERPORT  10666
#define CLIENTPORT  10667

#define PLAYER_FULLBRIGHTFRAME 70

#define PROTO_CHALLENGE -5560020  // Signals challenger wants protobufs.
#define MSG_CHALLENGE 5560020     // Signals challenger wants MSG protocol.
#define LAUNCHER_CHALLENGE 777123 // csdl challenge
#define VERSION 65                // GhostlyDeath -- this should remain static from now on

/**
 * @brief Types of client buffers.
 */
enum clientBuf_e
{
	CLBUF_RELIABLE,
	CLBUF_NET,
};

/**
 * @brief Compression is enabled for this packet
 */
#define SVF_COMPRESSED BIT(0)

/**
 * @brief Unused flags - if any of these are set, we have a problem.
 */
#define SVF_UNUSED_MASK BIT_MASK(1, 7)

/**
 * @brief svc_*: Transmit all possible data.
 */
#define SVC_MSG_ALL BIT_MASK(0, 7)

/**
 * @brief svc_levellocals: Level time.
 */
#define SVC_LL_TIME BIT(0)

/**
 * @brief svc_levellocals: All level stat totals.
 */
#define SVC_LL_TOTALS BIT(1)

/**
 * @brief svc_levellocals: Found secrets.
 */
#define SVC_LL_SECRETS BIT(2)

/**
 * @brief svc_levellocals: Found items.
 */
#define SVC_LL_ITEMS BIT(3)

/**
 * @brief svc_levellocals: Killed monsters.
 */
#define SVC_LL_MONSTERS BIT(4)

/**
 * @brief svc_levellocals: Respawned monsters.
 */
#define SVC_LL_MONSTER_RESPAWNS BIT(5)

/**
 * @brief svc_spawnmobj: Persist flags.
 */
#define SVC_SM_FLAGS BIT(0)

/**
 * @brief svc_spawnmobj: Run corpse logic.
 */
#define SVC_SM_CORPSE BIT(1)

/**
 * @brief svc_spawnmobj: Odamex-specific flags.
 */
#define SVC_SM_OFLAGS BIT(2)

/**
 * @brief svc_updatemobj: Supply mobj position and random index.
 */
#define SVC_UM_POS_RND BIT(0)

/**
 * @brief svc_updatemobj: Supply mobj momentum and angle.
 */
#define SVC_UM_MOM_ANGLE BIT(1)

/**
 * @brief svc_updatemobj: Supply movedir and movecount.
 */
#define SVC_UM_MOVEDIR BIT(2)

/**
 * @brief svc_updatemobj: Supply target.
 */
#define SVC_UM_TARGET BIT(3)

/**
 * @brief svc_updatemobj: Supply tracer.
 */
#define SVC_UM_TRACER BIT(4)

/**
 * @brief svc_playermembers: Spectator status.
 */
#define SVC_PM_SPECTATOR BIT(0)

/**
 * @brief svc_playermembers: Ready status.
 */ 
#define SVC_PM_READY BIT(1)

/**
 * @brief svc_playermembers: Number of lives.
 */
#define SVC_PM_LIVES BIT(2)

/**
 * @brief svc_playermembers: Damage done to monsters.
 */
#define SVC_PM_DAMAGE BIT(3)

/**
 * @brief svc_playermembers: "Score" members like frags, etc.
 */
#define SVC_PM_SCORE BIT(4)

/**
 * @brief svc_playermembers: Cheats & flags.
 */
#define SVC_PM_CHEATS BIT(5)

extern int   localport;
extern int   msg_badread;

// network message info
struct msg_info_t
{
	int id;
	const char *msgName;
	const char *msgFormat; // 'b'=byte, 'n'=short, 'N'=long, 's'=string

	const char *getName() { return msgName ? msgName : ""; }
};

// network messages
enum svc_t
{
	svc_noop,
	svc_disconnect,
	svc_playerinfo, // weapons, ammo, maxammo, raisedweapon for local player
	svc_moveplayer,
	svc_updatelocalplayer,
	svc_levellocals, // [AM] Persist one or more level locals
	svc_pingrequest, // [SL] 2011-05-11 timestamp
	svc_updateping,
	svc_spawnmobj,
	svc_disconnectclient,
	svc_loadmap,
	svc_consoleplayer,
	svc_explodemissile,
	svc_removemobj,
	svc_userinfo,
	svc_updatemobj,
	svc_spawnplayer,
	svc_damageplayer,
	svc_killmobj,
	svc_fireweapon,
	svc_updatesector,
	svc_print,
	svc_playermembers,
	svc_teammembers,
	svc_activateline,
	svc_movingsector,
	svc_playsound,
	svc_reconnect,
	svc_exitlevel,
	svc_touchspecial,
	svc_forceteam, // [Toke] Allows server to change a clients team setting.
	svc_switch,
	svc_say, // [AM] Similar to a broadcast print except we know who said it.
	svc_spawnhiddenplayer, // [denis] when client can't see player
	svc_updatedeaths,
	svc_ctfrefresh,     // [Toke - CTF]
	svc_ctfevent,       // [Toke - CTF]
	svc_secretevent,    // [Ch0wW] informs clients of a secret discovered
	svc_serversettings, // [Toke] - informs clients of server settings
	svc_connectclient,
	svc_midprint,
	svc_servergametic,   // [SL] 2011-05-11
	svc_inttimeleft,     // [ML] For intermission timer
	svc_fullupdatedone,  // [SL] Inform client the full update is over
	svc_railtrail,       // [SL] Draw railgun trail and play sound
	svc_playerstate,     // [SL] Health, armor, and weapon of a player
	svc_levelstate,      // [AM] Broadcast level state to client
	svc_resetmap,        // [AM] Server is resetting the map
	svc_playerqueuepos,  // Notify clients of player queue postion
	svc_fullupdatestart, // Inform client the full update has started
	svc_lineupdate, // Sync client with any line property changes - e.g. SetLineTexture,
	                // SetLineBlocking, SetLineSpecial, etc.
	svc_sectorproperties,
	svc_linesideupdate,
	svc_mobjstate,
	svc_damagemobj,
	svc_executelinespecial,
	svc_executeacsspecial,
	svc_thinkerupdate,
	svc_vote_update,       // [AM] - Send the latest voting state to the client.
	svc_maplist,           // [AM] - Return a maplist status.
	svc_maplist_update,    // [AM] - Send the entire maplist to the client in chunks.
	svc_maplist_index,     // [AM] - Send the current and next map index to the client.
	svc_toast,
	svc_hordeinfo,
	svc_netdemocap = 100,  // netdemos - NullPoint
	svc_netdemostop = 101, // netdemos - NullPoint
	svc_netdemoloadsnap = 102, // netdemos - NullPoint
};

static const size_t svc_max = 255;

enum ThinkerType
{
	TT_Scroller,
	TT_FireFlicker,
	TT_Flicker,
	TT_LightFlash,
	TT_Strobe,
	TT_Glow,
	TT_Glow2,
	TT_Phased,
};

// network messages
enum clc_t
{
	clc_abort,
	clc_reserved1,
	clc_disconnect,
	clc_say,
	clc_move,      // send cmds
	clc_userinfo,  // send userinfo
	clc_pingreply, // [SL] 2011-05-11 - timestamp
	clc_rate,
	clc_ack,
	clc_rcon,
	clc_rcon_password,
	clc_changeteam, // [NightFang] - Change your team
	                // [Toke - Teams] Made this actualy work
	clc_ctfcommand,
	clc_spectate,       // denis
	clc_wantwad,        // denis - name, hash
	clc_kill,           // denis - suicide
	clc_cheat,          // denis - handle cheat codes.
	clc_callvote,       // [AM] - Calling a vote
	clc_maplist,        // [AM] - Maplist status request.
	clc_maplist_update, // [AM] - Request the entire maplist from the server.
	clc_getplayerinfo,
	clc_netcmd,  // [AM] Send a string command to the server.
	clc_spy,     // [SL] Tell server to send info about this player
	clc_privmsg, // [AM] Targeted chat to a specific player.
};

static const size_t clc_max = 255;

extern msg_info_t clc_info[clc_max + 1];
extern msg_info_t svc_info[svc_max + 1];

namespace google
{
namespace protobuf
{
class Message;
}
} // namespace google

typedef struct
{
   byte    ip[4];
   unsigned short  port;
   unsigned short  pad;
} netadr_t;

extern  netadr_t  net_from;  // address of who sent the packet


class buf_t
{
public:
	byte	*data;
	size_t	allocsize, cursize, readpos;
	bool	overflowed;  // set to true if the buffer size failed

    // Buffer seeking flags
    typedef enum
    {
         BT_SSET // From beginning
        ,BT_SCUR // From current position
        ,BT_SEND // From end
    } seek_loc_t;

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

	//
	// Write an unsigned varint to the wire.
	//
	// Use these when you want to send an int and are reasonably sure that
	// the int will usually be small.
	//
	// [AM] If you make a change to this, please also duplicate that change
	//      into varint.c in the tools directory.
	//
	// https://developers.google.com/protocol-buffers/docs/encoding#varints
	//
	void WriteUnVarint(unsigned int v)
	{
		for (;;)
		{
			// Our next byte contains 7 bits of the number.
			int out = v & 0x7F;

			// Any bits left?
			v >>= 7;
			if (v == 0)
			{
				// We're done, bail after writing this byte.
				WriteByte(out);
				return;
			}

			// Flag the last bit to indicate more is coming.
			out |= 0x80;
			WriteByte(out);
		}
	}

	void WriteVarint(int v)
	{
		// Zig-zag encoding for negative numbers.
		WriteUnVarint((v << 1) ^ (v >> 31));
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
		return data[readpos++];
	}

	int NextByte()
	{
		if(readpos+1 > cursize)
		{
			overflowed = true;
			return -1;
		}
		return data[readpos];
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

	//
	// Read an unsigned varint off the wire.
	//
	// [AM] If you make a change to this, please also duplicate that change
	//      into varint.c in the tools directory.
	//
	// https://developers.google.com/protocol-buffers/docs/encoding#varints
	//
	unsigned int ReadUnVarint()
	{
		unsigned char b;
		unsigned int out = 0;
		unsigned int offset = 0;

		for (;;)
		{
			// Read part of the variant.
			b = ReadByte();
			if (overflowed)
				return -1;

			// Shove the first seven bits into our output variable.
			out |= (unsigned int)(b & 0x7F) << offset;
			offset += 7;

			// Is the flag bit set?
			if (!(b & 0x80))
			{
				// Nope, we're done.
				return out;
			}

			if (offset >= 32)
			{
				// Our variant int is too big - overflow us.
				overflowed = true;
				return -1;
			}
		}
	}

	int ReadVarint()
	{
		unsigned int uv = ReadUnVarint();
		if (overflowed)
			return -1;

		// Zig-zag encoding for negative numbers.
		return (uv >> 1) ^ (0U - (uv & 1));
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

    size_t SetOffset (const size_t &offset, const seek_loc_t &loc)
    {
        switch (loc)
        {
            case BT_SSET:
            {
                if (offset > cursize)
                {
                    overflowed = true;
                    return 0;
                }

                readpos = offset;
            }
            break;

            case BT_SCUR:
            {
                if (readpos+offset > cursize)
                {
                    overflowed = true;
                    return 0;
                }

                readpos += offset;
            }

            case BT_SEND:
            {
                if ((int)(readpos-offset) < 0)
                {
                    // lies, an underflow occured
                    overflowed = true;
                    return 0;
                }

                readpos -= offset;
            }
        }

        return readpos;
    }

	size_t BytesLeftToRead() const
	{
		return overflowed || cursize < readpos ? 0 : cursize - readpos;
	}

	size_t BytesRead() const
	{
		return readpos;
	}

	byte *ptr()
	{
		return data;
	}

	size_t size() const
	{
		return cursize;
	}
	
	size_t maxsize() const
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

	void resize(size_t len, bool clearbuf = true)
	{
		byte *olddata = data;
		data = new byte[len];
		allocsize = len;
		
		if (!clearbuf)
		{
			if (cursize < allocsize)
			{
				memcpy(data, olddata, cursize);
			}
			else
			{
				clear();
				overflowed = true;
				Printf (PRINT_HIGH, "buf_t::resize(): overflow\n");
			}
		}
		else
		{
			clear();
		}

		delete[] olddata;
	}

	byte *SZ_GetSpace(size_t length)
	{
		if (cursize + length >= allocsize)
		{
			clear();
			overflowed = true;
#if defined(ODAMEX_DEBUG)
			Printf (PRINT_HIGH, "SZ_GetSpace: overflow\n");
#endif
		}

		byte *ret = data + cursize;
		cursize += length;

		return ret;
	}

	buf_t &operator =(const buf_t &other)
	{
	    // Avoid self-assignment
		if (this == &other)
            return *this;

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
		delete[] data;
		data = NULL;
	}
};

extern buf_t net_message;

void CloseNetwork (void);
void InitNetCommon(void);
void I_SetPort(netadr_t &addr, int port);
bool NetWaitOrTimeout(size_t ms);

char *NET_AdrToString (netadr_t a);
bool NET_StringToAdr (const char *s, netadr_t *a);
bool NET_CompareAdr (netadr_t a, netadr_t b);
int  NET_GetPacket (void);
int NET_SendPacket (buf_t &buf, netadr_t &to);
std::string NET_GetLocalAddress (void);

void SZ_Clear (buf_t *buf);
void SZ_Write (buf_t *b, const void *data, int length);
void SZ_Write (buf_t *b, const byte *data, int startpos, int length);

void MSG_WriteByte (buf_t *b, byte c);
void MSG_WriteMarker (buf_t *b, svc_t c);
void MSG_WriteMarker (buf_t *b, clc_t c);
void MSG_WriteShort (buf_t *b, short c);
void MSG_WriteLong (buf_t *b, int c);
void MSG_WriteUnVarint(buf_t* b, unsigned int uv);
void MSG_WriteVarint(buf_t* b, int v);
void MSG_WriteBool(buf_t *b, bool);
void MSG_WriteFloat(buf_t *b, float);
void MSG_WriteString (buf_t *b, const char *s);
void MSG_WriteHexString(buf_t *b, const char *s);
void MSG_WriteChunk (buf_t *b, const void *p, unsigned l);
void MSG_WriteSVC(buf_t* b, const google::protobuf::Message& msg);
void MSG_BroadcastSVC(const clientBuf_e buf, const google::protobuf::Message& msg,
                      const int skipPlayer = -1);

int MSG_BytesLeft(void);
int MSG_NextByte (void);

int MSG_ReadByte (void);
void *MSG_ReadChunk (const size_t &size);
int MSG_ReadShort (void);
int MSG_ReadLong (void);
unsigned int MSG_ReadUnVarint();
int MSG_ReadVarint();
bool MSG_ReadBool(void);
float MSG_ReadFloat(void);
const char *MSG_ReadString (void);

template <typename MSG>
bool MSG_ReadProto(MSG& msg)
{
	size_t size = MSG_ReadUnVarint();
	void* data = MSG_ReadChunk(size);
	if (!msg.ParseFromArray(data, size))
	{
		return false;
	}
	return true;
}

size_t MSG_SetOffset (const size_t &offset, const buf_t::seek_loc_t &loc);

bool MSG_DecompressMinilzo ();
bool MSG_CompressMinilzo (buf_t &buf, size_t start_offset, size_t write_gap);

bool MSG_DecompressAdaptive (huffman &huff);
bool MSG_CompressAdaptive (huffman &huff, buf_t &buf, size_t start_offset, size_t write_gap);
