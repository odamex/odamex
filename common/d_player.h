// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	D_PLAYER
//
//-----------------------------------------------------------------------------


#ifndef __D_PLAYER_H__
#define __D_PLAYER_H__

#include <list>
#include <vector>
#include <queue>

#include <time.h>

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

#include "d_net.h"

// The player data structure depends on a number
// of other structs: items (internal inventory),
// animation states (closely tied to the sprites
// used to represent them, unfortunately).
#include "d_items.h"
#include "p_pspr.h"

// In addition, the player is just a special
// case of the generic moving object/actor.
#include "actor.h"

#include "d_netinf.h"
#include "i_net.h"
#include "huffman.h"

#include "p_snapshot.h"
#include "d_netcmd.h"

//
// Player states.
//
typedef enum
{
	// Connecting or hacking
	PST_CONTACT,

	// Stealing or pirating
	PST_DOWNLOAD,

	// Staling or loitering
	PST_SPECTATE,

	// Spying or remote server administration
	PST_STEALTH_SPECTATE,

	// Playing or camping.
	PST_LIVE,

	// Dead on the ground, view follows killer.
	PST_DEAD,

	// Ready to restart/respawn???
	PST_REBORN,

	// These are cleaned up at the end of a frame
	PST_DISCONNECT,

    // [BC] Entered the game
	PST_ENTER

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
	CF_NOCLIP			= 1,		// No clipping, walk through barriers.
	CF_GODMODE			= 2,		// No damage, no health loss.
	CF_NOMOMENTUM		= 4,		// Not really a cheat, just a debug aid.
	CF_NOTARGET			= 8,		// [RH] Monsters don't target unless damaged.
	CF_FLY				= 16,		// [RH] Flying player
	CF_CHASECAM			= 32,		// [RH] Put camera behind player
	CF_FROZEN			= 64,		// [RH] Don't let the player move
	CF_REVERTPLEASE		= 128		// [RH] Stick camera in player's head if he moves
} cheat_t;

#define MAX_PLAYER_SEE_MOBJ	0x7F

//
// Extended player object info: player_t
//
class player_s
{
public:
	void Serialize (FArchive &arc);

	bool ingame() const
	{
		return playerstate == PST_LIVE ||
				playerstate == PST_DEAD ||
				playerstate == PST_REBORN ||
				playerstate == PST_ENTER;
	}

	// player identifier on server
	byte		id;

	// current player state, see playerstate_t
	byte		playerstate;

	AActor::AActorPtr	mo;

	struct ticcmd_t cmd;	// the ticcmd currently being processed
	std::queue<NetCommand> cmdqueue;	// all received ticcmds

	// [RH] who is this?
	UserInfo	userinfo;

	// FOV in degrees
	float		fov;
	// Focal origin above r.z
	fixed_t		viewz;
	fixed_t		prevviewz;
	// Base height above floor for viewz.
	fixed_t		viewheight;
    // Bob/squat speed.
	fixed_t		deltaviewheight;
    // bounded/scaled total momentum.
	fixed_t		bob;

    // This is only used between levels,
    // mo->health is used during levels.
	int			health;
	int			armorpoints;
    // Armor type is 0-2.
	int			armortype;

    // Power ups. invinc and invis are tic counters.
	int			powers[NUMPOWERS];
	bool		cards[NUMCARDS];
	bool		backpack;

	// [Toke - CTF] Points in a special game mode
	int			points;
	// [Toke - CTF - Carry] Remembers the flag when grabbed
	bool		flags[NUMFLAGS];

    // Frags, deaths, monster kills
	int			fragcount;
	int			deathcount;
	int			killcount, itemcount, secretcount;		// for intermission

    // Is wp_nochange if not changing.
	weapontype_t	pendingweapon;
	weapontype_t	readyweapon;

	bool		weaponowned[NUMWEAPONS];
	int			ammo[NUMAMMO];
	int			maxammo[NUMAMMO];

    // True if button down last tic.
	int			attackdown, usedown;

	// Bit flags, for cheats and debug.
    // See cheat_t, above.
	int			cheats;

	// Refired shots are less accurate.
	short		refire;

	// For screen flashing (red or bright).
	int			damagecount, bonuscount;

    // Who did damage (NULL for floors/ceilings).
	AActor::AActorPtrCounted attacker;

    // So gun flashes light up areas.
	int			extralight;
										// Current PLAYPAL, ???
	int			fixedcolormap;			//  can be set to REDCOLORMAP for pain, etc.

	int			xviewshift;				// [RH] view shift (for earthquakes)

	int         psprnum;
	pspdef_t	psprites[NUMPSPRITES];	// Overlay view sprites (gun, etc).

	int			jumpTics;				// delay the next jump for a moment

	int			death_time;				// [SL] Record time of death to enforce respawn delay if needed 
	fixed_t		oldvelocity[3];			// [RH] Used for falling damage

	AActor::AActorPtr camera;			// [RH] Whose eyes this player sees through

	int			air_finished;			// [RH] Time when you start drowning

	int			GameTime;				// [Dash|RD] Length of time that this client has been in the game.
	time_t		JoinTime;				// [Dash|RD] Time this client joined.
    int         ping;                   // [Fly] guess what :)
	int         last_received;

	int         tic;					// gametic last update for player was received
	
	PlayerSnapshotManager snapshots;	// Previous player positions

	byte		spying;					// [SL] id of player being spynext'd by this player
	bool		spectator;				// [GhostlyDeath] spectating?
//	bool		deadspectator;			// [tm512] spectating as a dead player?
	int			joinafterspectatortime; // Nes - Join after spectator time.
	int			timeout_callvote;       // [AM] Tic when a vote last finished.
	int			timeout_vote;           // [AM] Tic when a player last voted.

	bool		ready;					// [AM] Player is ready.
	int			timeout_ready;          // [AM] Tic when a player last toggled his ready state.

    byte		prefcolor[4];			// Nes - Preferred color. Server only.

	argb_t		blend_color;			// blend color for the sector the player is in
	bool		doreborn;

	// For flood protection
	struct LastMessage_s
	{
		dtime_t		Time;
		std::string	Message;
	} LastMessage;

	// denis - things that are pending to be sent to this player
	std::queue<AActor::AActorPtr> to_spawn;

	// denis - client structure is here now for a 1:1
	struct client_t
	{
		netadr_t    address;

		buf_t       netbuf;
		buf_t       reliablebuf;

		// protocol version supported by the client
		short		version;
		short		majorversion;	// GhostlyDeath -- Major
		short		minorversion;	// GhostlyDeath -- Minor

		// for reliable protocol
		buf_t       relpackets; // save reliable packets here
		int         packetbegin[256]; // the beginning of a packet
		int         packetsize[256]; // the size of a packet
		int         packetseq[256];
		int         sequence;
		int         last_sequence;
		byte        packetnum;

		int         rate;
		int         reliable_bps;	// bytes per second
		int         unreliable_bps;

		int			last_received;	// for timeouts

		int			lastcmdtic, lastclientcmdtic;

		std::string	digest;			// randomly generated string that the client must use for any hashes it sends back
		bool        allow_rcon;     // allow remote admin
		bool		displaydisconnect; // display disconnect message when disconnecting

		huffman_server	compressor;	// denis - adaptive huffman compression

		class download_t
		{
		public:
			std::string name;
			unsigned int next_offset;

			download_t() : name(""), next_offset(0) {}
			download_t(const download_t& other) : name(other.name), next_offset(other.next_offset) {}
		}download;

		client_t()
		{
			// GhostlyDeath -- Initialize to Zero
			memset(&address, 0, sizeof(netadr_t));
			version = 0;
			majorversion = 0;
			minorversion = 0;
			for (size_t i = 0; i < 256; i++)
			{
				packetbegin[i] = 0;
				packetsize[i] = 0;
				packetseq[i] = 0;
			}
			sequence = 0;
			last_sequence = 0;
			packetnum = 0;
			rate = 0;
			reliable_bps = 0;
			unreliable_bps = 0;
			last_received = 0;
			lastcmdtic = 0;
			lastclientcmdtic = 0;


			// GhostlyDeath -- done with the {}
			netbuf = MAX_UDP_PACKET;
			reliablebuf = MAX_UDP_PACKET;
			relpackets = MAX_UDP_PACKET*50;
			digest = "";
			allow_rcon = false;
			displaydisconnect = true;
		/*
		huffman_server	compressor;	// denis - adaptive huffman compression*/
		}
		client_t(const client_t &other)
			: address(other.address),
			netbuf(other.netbuf),
			reliablebuf(other.reliablebuf),
			version(other.version),
			majorversion(other.majorversion),
			minorversion(other.minorversion),
			relpackets(other.relpackets),
			sequence(other.sequence),
			last_sequence(other.last_sequence),
			packetnum(other.packetnum),
			rate(other.rate),
			reliable_bps(other.reliable_bps),
			unreliable_bps(other.unreliable_bps),
			last_received(other.last_received),
			lastcmdtic(other.lastcmdtic),
			lastclientcmdtic(other.lastclientcmdtic),
			digest(other.digest),
			allow_rcon(false),
			displaydisconnect(true),
			compressor(other.compressor),
			download(other.download)
		{
				memcpy(packetbegin, other.packetbegin, sizeof(packetbegin));
				memcpy(packetsize, other.packetsize, sizeof(packetsize));
				memcpy(packetseq, other.packetseq, sizeof(packetseq));
		}
	} client;

	struct ticcmd_t netcmds[BACKUPTICS];

	int GetPlayerNumber() const
	{
		return id - 1;
	}

	player_s();
	player_s &operator =(const player_s &other);
	
	~player_s();


};

typedef player_s player_t;
typedef player_t::client_t client_t;

// Bookkeeping on players - state.
typedef std::list<player_t> Players;
extern Players players;

// Player taking events, and displaying.
player_t		&consoleplayer();
player_t		&displayplayer();
player_t		&listenplayer();
player_t		&idplayer(byte id);
player_t		&nameplayer(const std::string &netname);
bool			validplayer(player_t &ref);

extern byte consoleplayer_id;
extern byte displayplayer_id;

//

// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct wbplayerstruct_s
{
	BOOL		in;			// whether the player is in game

	// Player stats, kills, collected items etc.
	int			skills;
	int			sitems;
	int			ssecret;
	int			stime;
	int			fragcount;	// [RH] Cumulative frags for this player
	int			score;		// current score on entry, modified on return

} wbplayerstruct_t;

typedef struct wbstartstruct_s
{
	int			epsd;	// episode # (0-2)

	char		current[9];	// [RH] Name of map just finished
	char		next[9];	// next level, [RH] actual map name

	char		lname0[9];
	char		lname1[9];

	int			maxkills;
	int			maxitems;
	int			maxsecret;
	int			maxfrags;

	// the par time
	int			partime;

	// index of this player in game
	unsigned	pnum;

	std::vector<wbplayerstruct_s> plyr;
} wbstartstruct_t;

#endif // __D_PLAYER_H__



