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
//	D_PLAYER
//
//-----------------------------------------------------------------------------



#ifndef __D_PLAYER_H__
#define __D_PLAYER_H__

#include <vector>
#include <queue>

#include <time.h>

// Finally, for odd reasons, the player input
// is buffered within the player data struct,
// as commands per game tick.
#include "d_ticcmd.h"

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
	PST_DISCONNECT
} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
	// No clipping, walk through barriers.
	CF_NOCLIP			= 1,
	// No damage, no health loss.
	CF_GODMODE			= 2,
	// Not really a cheat, just a debug aid.
	CF_NOMOMENTUM		= 4,
	// [RH] Monsters don't target
	CF_NOTARGET			= 8,
	// [RH] Flying player
	CF_FLY				= 16,
	// [RH] Put camera behind player
	CF_CHASECAM			= 32,
	// [RH] Don't let the player move
	CF_FROZEN			= 64,
	// [RH] Stick camera in player's head if he moves
	CF_REVERTPLEASE		= 128
} cheat_t;

#define MAX_PLAYER_SEE_MOBJ	0x7F

//
// Extended player object info: player_t
//
class player_s
{
public:
	void Serialize (FArchive &arc);

	bool ingame()
	{
		return playerstate == PST_LIVE ||
				playerstate == PST_DEAD ||
				playerstate == PST_REBORN;
	}

	// player identifier on server
	byte		id;
	
	// current player state, see playerstate_t
	byte		playerstate;

	AActor::AActorPtr	mo;

	struct ticcmd_t	cmd;

	// [RH] who is this?
	userinfo_t	userinfo;
	
	// FOV in degrees
	float		fov;
	// Focal origin above r.z
	fixed_t		viewz;
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

	// [Toke - CTF]
	bool		flags[NUMFLAGS];
	int			points;
	bool		backpack;
	
    // Frags, deaths, monster kills
	int			fragcount;
	int			deathcount;
	int			killcount;

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
    int			refire;
	
	// For screen flashing (red or bright).
	int			damagecount, bonuscount;
	
    // Who did damage (NULL for floors/ceilings).
	AActor::AActorPtr attacker;
	
    // So gun flashes light up areas.
	int			extralight;

    // Current PLAYPAL, ???
    //  can be set to REDCOLORMAP for pain, etc.
	int			fixedcolormap;
	
	// Overlay view sprites (gun, etc).
	pspdef_t	psprites[NUMPSPRITES];

	int			respawn_time;			// [RH] delay respawning until this tic
	fixed_t		oldvelocity[3];			// [RH] Used for falling damage
	
	AActor::AActorPtr camera;			// [RH] Whose eyes this player sees through

	int			air_finished;			// [RH] Time when you start drowning

	time_t	JoinTime;					// [Dash|RD] Time this client joined.
    int         ping;                   // [Fly] guess what :)

    bool		spectator;			// [GhostlyDeath] spectating?
    int			joinafterspectatortime; // Nes - Join after spectator time.
    
    int			prefcolor;			// Nes - Preferred color. Server only.
    
    // For flood protection
    struct
    {
        QWORD Time;
        std::string Message;
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
	}client;

	player_s()
	{
		size_t i;

		// GhostlyDeath -- Initialize EVERYTHING
		mo = AActor::AActorPtr();
		id = 0;
		memset(&cmd, 0, sizeof(ticcmd_t));
		memset(&userinfo, 0, sizeof(userinfo_t));
		fov = 0.0;
		viewz = 0 << FRACBITS;
		viewheight = 0 << FRACBITS;
		deltaviewheight = 0 << FRACBITS;
		bob = 0 << FRACBITS;
		health = 0;
		armorpoints = 0;
		armortype = 0;
		for (i = 0; i < NUMPOWERS; i++)
			powers[i] = 0;
		for (i = 0; i < NUMCARDS; i++)
			cards[i] = false;
		backpack = false;
		points = 0;
		for (i = 0; i < NUMFLAGS; i++)
			flags[i] = false;
		fragcount = 0;
		deathcount = 0;
		killcount = 0;
		for (i = 0; i < NUMWEAPONS; i++)
			weaponowned[i] = false;
		for (i = 0; i < NUMAMMO; i++)
		{
			ammo[i] = 0;
			maxammo[i] = 0;
		}
		attackdown = 0;
		usedown = 0;
		refire = 0;
		damagecount = 0;
		bonuscount = 0;
		attacker = AActor::AActorPtr();
		extralight = 0;
		fixedcolormap = 0;
		memset(psprites, 0, sizeof(pspdef_t) * NUMPSPRITES);
		respawn_time = 0;
		for (i = 0; i < 3; i++)
			oldvelocity[i] = 0 << FRACBITS;
		camera = AActor::AActorPtr();
		air_finished = 0;
		ping = 0;
		
		// GhostlyDeath -- Do what was above incase
		playerstate = PST_CONTACT;
		pendingweapon = wp_pistol;
		readyweapon = wp_pistol;
		cheats = 0;
		spectator = false;
		joinafterspectatortime = level.time - TICRATE*5;
		prefcolor = 0;
		
		LastMessage.Time = 0;
		LastMessage.Message = "";
	}

	player_s &operator =(const player_s &other)
	{
		size_t i;

		id = other.id;
		playerstate = other.playerstate;
		mo = other.mo;
		cmd = other.cmd;
		userinfo = other.userinfo;
		fov = other.fov;
		viewz = other.viewz;
		viewheight = other.viewheight;
		deltaviewheight = other.deltaviewheight;
		bob = other.bob;

		health = other.health;
		armorpoints = other.armorpoints;
		armortype = other.armortype;

		for(i = 0; i < NUMPOWERS; i++)
			powers[i] = other.powers[i];

		for(i = 0; i < NUMCARDS; i++)
			cards[i] = other.cards[i];

		for(i = 0; i < NUMFLAGS; i++)
			flags[i] = other.flags[i];

		points = other.points;
		backpack = other.backpack;

		fragcount = other.fragcount;
		deathcount = other.deathcount;
		killcount = other.killcount;

		pendingweapon = other.pendingweapon;
		readyweapon = other.readyweapon;

		for(i = 0; i < NUMWEAPONS; i++)
			weaponowned[i] = other.weaponowned[i];
		for(i = 0; i < NUMAMMO; i++)
			ammo[i] = other.ammo[i];
		for(i = 0; i < NUMAMMO; i++)
			maxammo[i] = other.maxammo[i];
		
		attackdown = other.attackdown;
		usedown = other.usedown;

		cheats = other.cheats;

		refire = other.refire;
    
		damagecount = other.damagecount;
		bonuscount = other.bonuscount;

		attacker = other.attacker;

		extralight = other.extralight;
		fixedcolormap = other.fixedcolormap;

		for(i = 0; i < NUMPSPRITES; i++)
			psprites[i] = other.psprites[i];
	
		respawn_time = other.respawn_time;

		oldvelocity[0] = other.oldvelocity[0];
		oldvelocity[1] = other.oldvelocity[1];
		oldvelocity[2] = other.oldvelocity[2];

		camera = other.camera;
		air_finished = other.air_finished;
		
		JoinTime = other.JoinTime;
		ping = other.ping;
		
		spectator = other.spectator;
		joinafterspectatortime = other.joinafterspectatortime;
		
		prefcolor = other.prefcolor;
		
        LastMessage.Time = other.LastMessage.Time;
		LastMessage.Message = other.LastMessage.Message;
		
		client = other.client;

		return *this;
	}
};

typedef player_s player_t;
typedef player_t::client_t client_t;

// Bookkeeping on players - state.
extern std::vector<player_t> players;

// Player taking events, and displaying.
player_t		&consoleplayer();
player_t		&displayplayer();
player_t		&idplayer(size_t id);
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


