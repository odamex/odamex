// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
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



//
// Player states.
//
typedef enum
{
	// Playing or camping.
	PST_LIVE,
	// Dead on the ground, view follows killer.
	PST_DEAD,
	// Ready to restart/respawn???
	PST_REBORN			

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

	AActor::AActorPtr	mo;

	byte		id;
	BYTE		playerstate;
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
	bool		backpack;

	// [Toke - CTF] Points in a special game mode
	int			points;
	// [Toke - CTF - Carry] Remembers the flag when grabbed
	bool		flags[NUMFLAGS];

    // Frags, deaths, monster kills
	int			fragcount;
	int			deathcount;
	int			killcount;

    // Is wp_nochange if not changing.
	weapontype_t	pendingweapon;
	weapontype_t	readyweapon;
	
	int			weaponowned[NUMWEAPONS];
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

	int			GameTime;				// [Dash|RD] Length of time that this client has been in the game.
    int         ping;					// [csDoom] guess what :)
	int         last_received;

	fixed_t     real_origin[3];       // coordinates and velocity which
	fixed_t     real_velocity[3];     // a client got from the server
	int         tic;                  // and that was on tic "tic"
	
	struct ticcmd_t netcmds[BACKUPTICS];

	player_s() : playerstate(PST_LIVE), fragcount(0), deathcount(0), pendingweapon(wp_pistol), readyweapon(wp_pistol), cheats(0)
	{
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
		
		GameTime = other.GameTime;
		ping = other.ping;

		last_received = other.last_received;

		for(i = 0; i < 3; i++)
		{
			real_origin[i] = other.real_origin[i];
			real_velocity[i] = other.real_velocity[i];
		}

		tic = other.tic;
	
		for(i = 0; i < BACKUPTICS; i++)
			netcmds[i] = other.netcmds[i];
		
		return *this;
	}
};

typedef player_s player_t;

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


