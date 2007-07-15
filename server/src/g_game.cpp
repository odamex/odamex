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
//	G_GAME
//
//-----------------------------------------------------------------------------


#include "version.h"
#include "minilzo.h"
#include "m_alloc.h"
#include "doomdef.h"
#include "doomstat.h"
#include "d_protocol.h"
#include "d_netinf.h"
#include "z_zone.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_random.h"
#include "i_system.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "d_main.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_dispatch.h"
#include "v_video.h"
#include "w_wad.h"
#include "p_local.h"
#include "s_sound.h"
#include "dstrings.h"
#include "r_data.h"
#include "r_sky.h"
#include "r_draw.h"
#include "g_game.h"
#include "g_level.h"
#include "sv_main.h"

#include "sv_ctf.h"

#define SAVESTRINGSIZE	24

#define TURN180_TICKS	9				// [RH] # of ticks to complete a turn180

BOOL	G_CheckDemoStatus (void);
void	G_ReadDemoTiccmd (ticcmd_t* cmd, int player);
void	G_WriteDemoTiccmd (ticcmd_t* cmd, int player, int buf);
void	G_PlayerReborn (player_t &player);

void	G_DoReborn (player_t &playernum);

void	G_DoNewGame (void);
void	G_DoLoadGame (void);
void	G_DoPlayDemo (void);
void	G_DoCompleted (void);
void	G_DoVictory (void);
void	G_DoWorldDone (void);
void	G_DoSaveGame (void);

void	SV_GameTics (void);


CVAR (chasedemo, "0", 0) // removeme

gameaction_t	gameaction;
gamestate_t 	gamestate = GS_STARTUP;
BOOL 			respawnmonsters;

BOOL 			paused;
BOOL 			sendpause;				// send a pause event next tic
BOOL			sendsave;				// send a save event next tic
BOOL 			usergame;				// ok to save / end game
BOOL			sendcenterview;			// send a center view event next tic

BOOL			timingdemo; 			// if true, exit with report on completion
BOOL 			nodrawers;				// for comparative timing purposes
BOOL 			noblit; 				// for comparative timing purposes

BOOL	 		viewactive;

BOOL 						netgame;				// only true if packets are broadcast
BOOL						multiplayer;
std::vector<player_t>		players;
player_t					nullplayer;				// null player

byte			consoleplayer_id;			// player taking events and displaying
byte			displayplayer_id;			// view being displayed
int 			gametic;

char			demoname[256];
BOOL 			demorecording;
BOOL 			demoplayback;
BOOL 			netdemo;
BOOL			demonew;				// [RH] Only used around G_InitNew for demos
int				demover;
byte*			demobuffer;
byte*			demo_p;
size_t			maxdemosize;
byte*			zdemformend;			// end of FORM ZDEM chunk
byte*			zdembodyend;			// end of ZDEM BODY chunk
BOOL 			singledemo; 			// quit after playing a demo from cmdline

BOOL 			precache = true;		// if true, load all graphics at start

wbstartstruct_t wminfo; 				// parms for world map / intermission

byte*			savebuffer;


#define MAXPLMOVE				(forwardmove[1])

#define TURBOTHRESHOLD	12800

float	 		normforwardmove[2] = {0x19, 0x32};		// [RH] For setting turbo from console
float	 		normsidemove[2] = {0x18, 0x28};			// [RH] Ditto

fixed_t			forwardmove[2], sidemove[2];
fixed_t 		angleturn[3] = {640, 1280, 320};		// + slow turn
fixed_t			flyspeed[2] = {1*256, 3*256};
int				lookspeed[2] = {450, 512};

#define SLOWTURNTICS	6

int 			turnheld;								// for accelerative turning

// mouse values are used once
int 			mousex;
int 			mousey;

// joystick values are repeated
// [RH] now, if the joystick is enabled, it will generate an event every tick
//		so the values here are reset to zero after each tic build (in case
//		use_joystick gets set to 0 when the joystick is off center)
int 			joyxmove;
int 			joyymove;

int 			savegameslot;
char			savedescription[32];

player_t		&consoleplayer()
{
	return idplayer(consoleplayer_id);
}

player_t		&displayplayer()
{
	return idplayer(displayplayer_id);
}

player_t		&idplayer(size_t id)
{
	// attempt a quick cached resolution
	size_t translation[MAXPLAYERS] = {0};
	size_t size = players.size();

	if(id >= MAXPLAYERS)
		return nullplayer;

	size_t tid = translation[id];
	if(tid < size && players[tid].id == id)
		return players[tid];

	// full search
	for(size_t i = 0; i < size; i++)
	{
		if(players[i].id == id)
		{
			translation[id] = i;
			return players[i];
		}
	}

	return nullplayer;
}

bool validplayer(player_t &ref)
{
	if (&ref == &nullplayer)
		return false;

	if(players.empty())
		return false;

	return true;
}

/* [RH] Impulses: Temporary hack to get weapon changing
 * working with keybindings until I can get the
 * inventory system working.
 *
 * So this turned out to not be so temporary. It *will*
 * change, though.
 */
int Impulse;

BEGIN_COMMAND (impulse)
{
	if (argc > 1)
		Impulse = atoi (argv[1]);
}
END_COMMAND (impulse)

BEGIN_COMMAND (centerview)
{
	sendcenterview = true;
}
END_COMMAND (centerview)

BEGIN_COMMAND (pause)
{
	sendpause = true;
}
END_COMMAND (pause)

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd (ticcmd_t *cmd)
{
}


// [RH] Spy mode has been separated into two console commands.
//		One goes forward; the other goes backward.
/*
static void ChangeSpy (void)
{
}
*/

//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
BOOL G_Responder (event_t *ev)
{
	return false;
}


//
// G_Ticker
// Make ticcmd_ts for the players.
//
extern DCanvas *page;
int mapchange;

void G_Ticker (void)
{
	size_t i;

	SV_GameTics ();

	// do player reborns if needed
	if(serverside)
		for (i = 0; i < players.size(); i++)
			if (players[i].ingame() && players[i].playerstate == PST_REBORN)
				G_DoReborn (players[i]);

	// do things to change the game state
	gamestate_t oldgamestate = gamestate;
	while (gameaction != ga_nothing)
	{
		switch (gameaction)
		{
		case ga_loadlevel:
			G_DoLoadLevel (-1);
			break;
		case ga_newgame:
			G_DoNewGame ();
			break;
		case ga_loadgame:
			gameaction = ga_nothing;
			break;
		case ga_savegame:
			gameaction = ga_nothing;
			break;
		case ga_playdemo:
			gameaction = ga_nothing;
			break;
		case ga_completed:
			G_DoCompleted ();
			break;
		case ga_victory:
		    gameaction = ga_nothing;
			break;
		case ga_worlddone:
			//G_DoWorldDone ();
			break;
		case ga_screenshot:
			gameaction = ga_nothing;
			break;
		case ga_fullconsole:
//			C_FullConsole ();
			gameaction = ga_nothing;
			break;
		case ga_nothing:
			break;
		}
		C_AdjustBottom ();
	}

	if (oldgamestate == GS_DEMOSCREEN && oldgamestate != gamestate && page)
	{
		delete page;
		page = NULL;
	}

	// do main actions
	switch (gamestate)
	{
	case GS_LEVEL:
		P_Ticker ();
		SV_RemoveCorpses ();
		break;

	case GS_INTERMISSION:
		mapchange--; // denis - todo - check if all players are ready, proceed immediately
		if (!mapchange)
			G_ChangeMap ();
	    break;

	default:
		break;
	}

	SV_WriteCommands();

	// send packets, rotating the send order
	// so that players[0] does not always get an advantage
	{
		static size_t fair_send = 0;
		size_t num_players = players.size();

		for (i = 0; i < num_players; i++)
		{
			SV_SendPacket(players[(i+fair_send)%num_players]);
		}

		if(++fair_send >= num_players)
			fair_send = 0;
	}

	SV_ClearClientsBPS();

	SV_CheckTimeouts();
}


//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Mobj
//

//
// G_PlayerFinishLevel
// Call when a player completes a level.
//
void G_PlayerFinishLevel (player_t &player)
{
	player_t *p;

	p = &player;

	memset (p->powers, 0, sizeof (p->powers));
	memset (p->cards, 0, sizeof (p->cards));
	p->mo->flags &= ~MF_SHADOW; 		// cancel invisibility
	p->extralight = 0;					// cancel gun flashes
	p->fixedcolormap = 0;				// cancel ir goggles
	p->damagecount = 0; 				// no palette changes
	p->bonuscount = 0;
}


//
// G_PlayerReborn
// Called after a player dies
// almost everything is cleared and initialized
//
void G_PlayerReborn (player_t &p) // [Toke - todo] clean this function
{
	size_t i;
	for (i = 0; i < NUMAMMO; i++)
	{
		p.maxammo[i] = maxammo[i];
		p.ammo[i] = 0;
	}
	for (i = 0; i < NUMWEAPONS; i++)
		p.weaponowned[i] = false;
	for (i = 0; i < NUMCARDS; i++)
		p.cards[i] = false;
	for (i = 0; i < NUMPOWERS; i++)
		p.powers[i] = false;
	for (i = 0; i < NUMFLAGS; i++)
		p.flags[i] = false;
	p.backpack = false;

	p.usedown = p.attackdown = true;	// don't do anything immediately
	p.playerstate = PST_LIVE;
	p.health = deh.StartHealth;		// [RH] Used to be MAXHEALTH
	p.armortype = 0;
	p.armorpoints = 0;
	p.readyweapon = p.pendingweapon = wp_pistol;
	p.weaponowned[wp_fist] = true;
	p.weaponowned[wp_pistol] = true;
	p.ammo[am_clip] = deh.StartBullets; // [RH] Used to be 50

	p.respawn_time = level.time;
}

//
// G_CheckSpot
// Returns false if the player cannot be respawned
// at the given mapthing2_t spot
// because something is occupying it
//
void P_SpawnPlayer (player_t &player, mapthing2_t* mthing);

bool G_CheckSpot (player_t &player, mapthing2_t *mthing)
{
	fixed_t 			x;
	fixed_t 			y;
	fixed_t				z, oldz;
	subsector_t*		ss;
	size_t 				i;

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;
	z = mthing->z << FRACBITS;

	ss = R_PointInSubsector (x,y);
	z += ss->sector->floorheight;

	if (!player.mo)
	{
		// first spawn of level, before corpses
		for (i = 0; i < players.size() && (&players[i] != &player); i++)
			if (players[i].mo && players[i].mo->x == x && players[i].mo->y == y)
				return false;
		return true;
	}

	oldz = player.mo->z;	// [RH] Need to save corpse's z-height
	player.mo->z = z;		// [RH] Checks are now full 3-D

	// killough 4/2/98: fix bug where P_CheckPosition() uses a non-solid
	// corpse to detect collisions with other players in DM starts
	//
	// Old code:
	// if (!P_CheckPosition (players[playernum].mo, x, y))
	//    return false;

	player.mo->flags |=  MF_SOLID;
	i = P_CheckPosition(player.mo, x, y);
	player.mo->flags &= ~MF_SOLID;
	player.mo->z = oldz;	// [RH] Restore corpse's height
	if (!i)
		return false;

	return true;
}


//
// G_DeathMatchSpawnPlayer
// Spawns a player at one of the random death match spots
// called at level load and each death
//

// [RH] Returns the distance of the closest player to the given mapthing2_t.
// denis - todo - should this be used somewhere?
/*static fixed_t PlayersRangeFromSpot (mapthing2_t *spot)
{
	fixed_t closest = MAXINT;
	fixed_t distance;

	for (size_t i = 0; i < players.size(); i++)
	{
		if (!players[i].ingame() || !players[i].mo || players[i].health <= 0)
			continue;

		distance = P_AproxDistance (players[i].mo->x - spot->x * FRACUNIT,
									players[i].mo->y - spot->y * FRACUNIT);

		if (distance < closest)
			closest = distance;
	}

	return closest;
}*/


// [RH] Select a deathmatch spawn spot at random (original mechanism)
static mapthing2_t *SelectRandomDeathmatchSpot (player_t &player, int selections)
{
	int i, j;

	for (j = 0; j < 20; j++)
	{
		i = P_Random () % selections;
		if (G_CheckSpot (player, &deathmatchstarts[i]) )
		{
			return &deathmatchstarts[i];
		}
	}

	// [RH] return a spot anyway, since we allow telefragging when a player spawns
	return &deathmatchstarts[i];
}

void G_TeamSpawnPlayer (player_t &player) // [Toke - CTF - starts] Modified this function to accept teamplay starts
{
	int selections;
	mapthing2_t *spot;

	selections = 0;

	if (player.userinfo.team == TEAM_BLUE)  // [Toke - CTF - starts]
		selections = blueteam_p - blueteamstarts;

	if (player.userinfo.team == TEAM_RED)  // [Toke - CTF - starts]
		selections = redteam_p - redteamstarts;

	if (player.userinfo.team == TEAM_GOLD)  // [Toke - CTF - starts]
		selections = goldteam_p - goldteamstarts;

	// denis - fall back to deathmatch spawnpoints, if no team ones available
	if (selections < 1)
	{
		selections = deathmatch_p - deathmatchstarts;
		spot = SelectRandomDeathmatchSpot (player, selections);
	}
	else
	{
		spot = CTF_SelectTeamPlaySpot (player, selections);  // [Toke - Teams]
	}
	
	if (selections < 1)
		I_Error ("No appropriate team starts");

	if (!spot && !playerstarts.empty())
		spot = &playerstarts[player.id%playerstarts.size()];
	else
	{
		if (player.id < 4)
			spot->type = player.id+1;
		else
			spot->type = player.id+4001-4;
	}

	P_SpawnPlayer (player, spot);
}

void G_DeathMatchSpawnPlayer (player_t &player)
{
	int selections;
	mapthing2_t *spot;

	if(!deathmatch)
		return;

	if(teamplay || ctfmode)
	{
		G_TeamSpawnPlayer (player);
		return;
	}

	selections = deathmatch_p - deathmatchstarts;
	// [RH] We can get by with just 1 deathmatch start
	if (selections < 1)
		I_Error ("No deathmatch starts");

	// [Toke - dmflags] Old location of DF_SPAWN_FARTHEST
	spot = SelectRandomDeathmatchSpot (player, selections);

	if (!spot && !playerstarts.empty())
	{
		// no good spot, so the player will probably get stuck
		spot = &playerstarts[player.id%playerstarts.size()];
	}
	else
	{
		if (player.id < 4)
			spot->type = player.id+1;
		else
			spot->type = player.id+4001-4;	// [RH] > 4 players
	}

	P_SpawnPlayer (player, spot);
}

//
// G_DoReborn
//
void G_DoReborn (player_t &player)
{
	if(!serverside)
		return;

	if (!multiplayer)
	{
		// reload the level from scratch
		gameaction = ga_loadlevel;
		return;
	}

	// respawn at the start
	// first disassociate the corpse
	if (player.mo)
	{
		player.mo->player = NULL;
		player.mo = AActor::AActorPtr();
	}

	if(!serverside)
		return;

	// spawn at random team spot if in team game
	if(teamplay || ctfmode)
	{
		G_TeamSpawnPlayer (player);
		return;
	}

	// spawn at random spot if in death match
	if(deathmatch)
	{
		G_DeathMatchSpawnPlayer (player);
		return;
	}

	if(playerstarts.empty())
		I_Error("No player starts");

	unsigned int playernum = player.id - 1;

	if (G_CheckSpot (player, &playerstarts[playernum%playerstarts.size()]) )
	{
		P_SpawnPlayer (player, &playerstarts[playernum%playerstarts.size()]);
		return;
	}

	// try to spawn at one of the other players' spots
	for (size_t i = 0; i < playerstarts.size(); i++)
	{
		if (G_CheckSpot (player, &playerstarts[i]) )
		{
			P_SpawnPlayer (player, &playerstarts[i]);
			return;
		}
	}

	// he's going to be inside something.  Too bad.
	P_SpawnPlayer (player, &playerstarts[playernum%playerstarts.size()]);
}


void G_ScreenShot (char *filename)
{
//	shotfile = filename;
//	gameaction = ga_screenshot;
}





//
// G_InitFromSavegame
// Can be called by the startup code or the menu task.
//
extern BOOL setsizeneeded;
void R_ExecuteSetViewSize (void);

char savename[256];

void G_LoadGame (char* name)
{
	strcpy (savename, name);
	gameaction = ga_loadgame;
}


void G_DoLoadGame (void)
{
}


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame (int slot, char *description)
{
}

void G_BuildSaveName (char *name, int slot)
{
}

void G_DoSaveGame (void)
{
}


//
// G_RecordDemo
//
void G_RecordDemo (char* name)
{
}


// [RH] Demos are now saved as IFF FORMs. I've also removed support
//		for earlier ZDEMs since I didn't want to bother supporting
//		something that probably wasn't used much (if at all).

void G_BeginRecording (void)
{
}


//
// G_PlayDemo
//

void G_DeferedPlayDemo (char *name)
{
}


// [RH] Process all the information in a FORM ZDEM
//		until a BODY chunk is entered.
BOOL G_ProcessIFFDemo (char *mapname)
{
	return false;
}

void G_DoPlayDemo (void)
{
}

//
// G_TimeDemo
//
void G_TimeDemo (char* name)
{
}


//
// G_CheckDemoStatus
//
// Called after a death or level completion to allow demos to be cleaned up
// Returns true if a new demo loop action will take place
//

BOOL G_CheckDemoStatus (void)
{
	return false;
}

EXTERN_CVAR (fraglimit)
EXTERN_CVAR (allowexit)
EXTERN_CVAR (fragexitswitch)

BOOL CheckIfExitIsGood (AActor *self)
{
	if (self == NULL)
		return false;

	// [Toke - dmflags] Old location of DF_NO_EXIT
    // [ML] 04/4/06: Check for fragexitswitch - seems a bit hacky

    unsigned int i;

    for(i = 0; i < players.size(); i++)
        if(players[i].fragcount == fraglimit)
            break;

    if (deathmatch && self)
    {
        if (!allowexit && fragexitswitch && i == players.size())
            return false;

        if (!allowexit && !fragexitswitch)
            return false;
    }

	if(self->player)
		Printf (PRINT_HIGH, "%s exited the level.\n", self->player->userinfo.netname);

    return true;

}


VERSION_CONTROL (g_game_cpp, "$Id$")

