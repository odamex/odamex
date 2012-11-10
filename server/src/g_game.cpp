// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2012 by The Odamex Team.
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
#include "gstrings.h"
#include "r_data.h"
#include "r_sky.h"
#include "r_draw.h"
#include "g_game.h"
#include "g_level.h"
#include "sv_main.h"
#include "p_ctf.h"
#include "gi.h"

#define SAVESTRINGSIZE	24

#define TURN180_TICKS	9				// [RH] # of ticks to complete a turn180

BOOL	G_CheckDemoStatus (void);
void	G_ReadDemoTiccmd (ticcmd_t* cmd, int player);
void	G_WriteDemoTiccmd (ticcmd_t* cmd, int player, int buf);
void	G_PlayerReborn (player_t &player);

void	G_DoNewGame (void);
void	G_DoLoadGame (void);
void	G_DoPlayDemo (void);
void	G_DoCompleted (void);
void	G_DoVictory (void);
void	G_DoWorldDone (void);
void	G_DoSaveGame (void);

EXTERN_CVAR (sv_timelimit)
EXTERN_CVAR (sv_keepkeys)
EXTERN_CVAR (co_nosilentspawns)

gameaction_t	gameaction;
gamestate_t 	gamestate = GS_STARTUP;
BOOL 			respawnmonsters;

BOOL 			paused;
BOOL 			sendpause;				// send a pause event next tic
BOOL			sendsave;				// send a save event next tic
BOOL 			usergame;				// ok to save / end game
BOOL			sendcenterview;			// send a center view event next tic
BOOL			menuactive;				// only to make sure p_tick doesn't bitch

BOOL			timingdemo; 			// if true, exit with report on completion
BOOL 			nodrawers;				// for comparative timing purposes
BOOL 			noblit; 				// for comparative timing purposes

BOOL	 		viewactive;

// Describes if a network game is being played
BOOL			network_game;
// Use only for demos, it is a old variable for the old network code
BOOL			netgame;
// Describes if this is a multiplayer game or not
BOOL			multiplayer;
// The player vector, contains all player information
std::vector<player_t>		players;
// The null player
player_t		nullplayer;

byte			consoleplayer_id;			// player taking events and displaying
byte			displayplayer_id;			// view being displayed
int 			gametic;
bool			singleplayerjustdied = false;	// Nes - When it's okay for single-player servers to reload.

enum demoversion_t
{
	LMP_DOOM_1_9,
	LMP_DOOM_1_9_1, // longtics hack
	ZDOOM_FORM
}demoversion;

#define DOOM_1_4_DEMO		0x68
#define DOOM_1_5_DEMO		0x69
#define DOOM_1_6_DEMO		0x6A
#define DOOM_1_7_DEMO		0x6B
#define DOOM_1_8_DEMO		0x6C
#define DOOM_1_9_DEMO		0x6D
#define DOOM_1_9p_DEMO		0x6E
#define DOOM_1_9_1_DEMO		0x6F

#define DOOM_BOOM_DEMO_START	0xC8
#define DOOM_BOOM_DEMO_END	0xD6

FILE *recorddemo_fp;

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_monstersrespawn)

char			demoname[256];
BOOL 			demorecording;
BOOL 			demoplayback;
BOOL			democlassic;
BOOL 			netdemo;
BOOL			demonew;				// [RH] Only used around G_InitNew for demos
int				demover;
byte*			demobuffer;
byte			*demo_p, *demo_e;
size_t			maxdemosize;
byte*			zdemformend;			// end of FORM ZDEM chunk
byte*			zdembodyend;			// end of ZDEM BODY chunk
BOOL 			singledemo; 			// quit after playing a demo from cmdline
int			demostartgametic;

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

/* [RH] Impulses: Temporary hack to get weapon changing
 * working with keybindings until I can get the
 * inventory system working.
 *
 *	So this turned out to not be so temporary. It *will*
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

//
// G_WriteDemoTiccmd
//
void G_WriteDemoTiccmd ()
{
    byte demo_tmp[8];

    int demostep = (demoversion == LMP_DOOM_1_9_1) ? 5 : 4;

    for(size_t i = 0; i < players.size(); i++)
    {
        byte *demo_p = demo_tmp;
        usercmd_t *cmd = &players[i].cmd.ucmd;

        *demo_p++ = cmd->forwardmove >> 8;
        *demo_p++ = cmd->sidemove >> 8;

        // If this is a longtics demo, record in higher resolution

        if (LMP_DOOM_1_9_1 == demoversion)
        {
            *demo_p++ = (cmd->yaw & 0xff);
            *demo_p++ = (cmd->yaw >> 8) & 0xff;
        }
        else
        {
            *demo_p++ = cmd->yaw >> 8;
            cmd->yaw = ((unsigned char)*(demo_p-1))<<8;
        }

        *demo_p++ = cmd->buttons;

        size_t res = fwrite(demo_tmp, demostep, 1, recorddemo_fp);
    }
}

//
// G_RecordDemo
//
bool G_RecordDemo (char* name)
{
    strcpy (demoname, name);
    strcat (demoname, ".lmp");

    if(recorddemo_fp)
    {
        fclose(recorddemo_fp);
        recorddemo_fp = NULL;
    }

    recorddemo_fp = fopen(demoname, "w");

    if(!recorddemo_fp)
    {
        Printf(PRINT_HIGH, "Could not open file %s for writing\n", demoname);
        return false;
    }

    usergame = false;
    demorecording = true;
    demostartgametic = gametic;

    return true;
}
//
// G_BeginRecording
//
void G_BeginRecording (void)
{
    byte demo_tmp[32];
    demo_p = demo_tmp;

    // Save the right version code for this demo

    if (demoversion == LMP_DOOM_1_9_1) // denis - TODO!!!
    {
        *demo_p++ = DOOM_1_9_1_DEMO;
    }
    else
    {
        *demo_p++ = DOOM_1_9_DEMO;
    }

    democlassic = true;

    int episode;
    int mapid;
    if(gameinfo.flags & GI_MAPxx)
    {
        episode = 1;
        mapid = atoi(level.mapname + 3);
    }
    else
    {
        episode = level.mapname[1] - '0';
        mapid = level.mapname[3] - '0';
    }

    *demo_p++ = sv_skill.asInt() - 1;
    *demo_p++ = episode;
    *demo_p++ = mapid;
    *demo_p++ = sv_gametype.asInt();
    *demo_p++ = sv_monstersrespawn.asInt();
    *demo_p++ = sv_fastmonsters.asInt();
    *demo_p++ = sv_nomonsters.asInt();
    *demo_p++ = 0;

    *demo_p++ = 1;
    *demo_p++ = 0;
    *demo_p++ = 0;
    *demo_p++ = 0;

    size_t res = fwrite(demo_tmp, 13, 1, recorddemo_fp);
}

EXTERN_CVAR(sv_maxplayers)

void RecordCommand(int argc, char **argv)
{
	if(argc > 2)
	{
		int ingame = 0;
		for(size_t i = 0; i < players.size(); i++)
		{
			if(players[i].ingame())
				ingame++;
		}

		if(!ingame)
		{
			Printf(PRINT_HIGH, "cannot record with no players");
			return;
		}

		sv_maxplayers.Set(4.0f);

		if(G_RecordDemo(argv[2]))
		{
			G_InitNew(argv[1]);
			G_BeginRecording();
		}
	}
	else
		Printf(PRINT_HIGH, "Usage: recordvanilla map file\n");
}
/*
BEGIN_COMMAND(recordvanilla)
{
	//G_CheckDemoStatus();
	demoversion = LMP_DOOM_1_9;
	RecordCommand(argc, argv);
}
END_COMMAND(recordvanilla)

BEGIN_COMMAND(recordlongtics)
{
	//G_CheckDemoStatus();
	demoversion = LMP_DOOM_1_9_1;
	RecordCommand(argc, argv);
}
END_COMMAND(recordlongtics)

BEGIN_COMMAND(stopdemo)
{
	G_CheckDemoStatus ();
}
END_COMMAND(stopdemo)
*/

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

	// do player reborns if needed
	if(serverside)
		for (i = 0; i < players.size(); i++)
			if (players[i].ingame() && (players[i].playerstate == PST_REBORN || players[i].playerstate == PST_ENTER))
				G_DoReborn (players[i]);

	// do things to change the game state
	while (gameaction != ga_nothing)
	{
		switch (gameaction)
		{
		case ga_loadlevel:
			G_DoLoadLevel (-1);
			break;
		case ga_fullresetlevel:
			G_DoResetLevel(true);
			break;
		case ga_resetlevel:
			G_DoResetLevel(false);
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

	if(demorecording)
		G_WriteDemoTiccmd();

	// do main actions
	switch (gamestate)
	{
	case GS_LEVEL:
		P_Ticker ();
		break;

	case GS_INTERMISSION:
	{
		mapchange--; // denis - todo - check if all players are ready, proceed immediately
		if (!mapchange)
        {
			G_ChangeMap ();
            //intcd_oldtime = 0;
        }
	}
    break;

	default:
		break;
	}
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

	if(p->mo)
		p->mo->flags &= ~MF_SHADOW; 	// cancel invisibility

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
	if (!sv_keepkeys)
	{
		for (i = 0; i < NUMCARDS; i++)
			p.cards[i] = false;
	}
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

	p.death_time = 0;
	p.tic = 0;
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
	unsigned			an;
	AActor* 			mo;
	size_t 				i;
	fixed_t 			xa,ya;

	x = mthing->x << FRACBITS;
	y = mthing->y << FRACBITS;
	z = P_FloorHeight(x, y);

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

	// spawn a teleport fog
	if (!player.spectator)	// ONLY IF THEY ARE NOT A SPECTATOR
	{
		// emulate out-of-bounds access to finecosine / finesine tables
		// which cause west-facing player spawns to have the spawn-fog
		// and its sound located off the map in vanilla Doom.

		// borrowed from Eternity Engine

		// haleyjd: There was a weird bug with this statement:
		//
		// an = (ANG45 * (mthing->angle/45)) >> ANGLETOFINESHIFT;
		//
		// Even though this code stores the result into an unsigned variable, most
		// compilers seem to ignore that fact in the optimizer and use the resulting
		// value directly in a lea instruction. This causes the signed mapthing_t
		// angle value to generate an out-of-bounds access into the fine trig
		// lookups. In vanilla, this accesses the finetangent table and other parts
		// of the finesine table, and the result is what I call the "ninja spawn,"
		// which is missing the fog and sound, as it spawns somewhere out in the
		// far reaches of the void.

		if (co_nosilentspawns)
		{
			an = ( ANG45 * ((unsigned int)mthing->angle/45) ) >> ANGLETOFINESHIFT;
			xa = finecosine[an];
			ya = finesine[an];
		}
		else
		{
			angle_t mtangle = (angle_t)(mthing->angle / 45);

			an = ANG45 * mtangle;

			switch(mtangle)
			{
				case 4: // 180 degrees (0x80000000 >> 19 == -4096)
					xa = finetangent[2048];
					ya = finetangent[0];
					break;
				case 5: // 225 degrees (0xA0000000 >> 19 == -3072)
					xa = finetangent[3072];
					ya = finetangent[1024];
					break;
				case 6: // 270 degrees (0xC0000000 >> 19 == -2048)
					xa = finesine[0];
					ya = finetangent[2048];
					break;
				case 7: // 315 degrees (0xE0000000 >> 19 == -1024)
					xa = finesine[1024];
					ya = finetangent[3072];
					break;
				default: // everything else works properly
					xa = finecosine[an >> ANGLETOFINESHIFT];
					ya = finesine[an >> ANGLETOFINESHIFT];
					break;
			}
		}

		mo = new AActor (x+20*xa, y+20*ya, z, MT_TFOG);

		// send new object
		SV_SpawnMobj(mo);
	}

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
}

// [RH] Select the deathmatch spawn spot farthest from everyone.
static mapthing2_t *SelectFarthestDeathmatchSpot (int selections)
{
	fixed_t bestdistance = 0;
	mapthing2_t *bestspot = NULL;
	int i;

	for (i = 0; i < selections; i++)
	{
		fixed_t distance = PlayersRangeFromSpot (&deathmatchstarts[i]);

		if (distance > bestdistance)
		{
			bestdistance = distance;
			bestspot = &deathmatchstarts[i];
		}
	}

	return bestspot;
}

*/

// [RH] Select a deathmatch spawn spot at random (original mechanism)
static mapthing2_t *SelectRandomDeathmatchSpot (player_t &player, int selections)
{
	int i = 0, j;

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

// [Toke] Randomly selects a team spawn point
// [AM] Moved out of CTF gametype and cleaned up.
static mapthing2_t *SelectRandomTeamSpot(player_t &player, int selections)
{
	size_t i;

	switch (player.userinfo.team)
	{
	case TEAM_BLUE:
		for (size_t j = 0; j < MaxBlueTeamStarts; ++j)
		{
			i = M_Random() % selections;
			if (G_CheckSpot(player, &blueteamstarts[i]))
			{
				return &blueteamstarts[i];
			}
		}
		return &blueteamstarts[i];
	case TEAM_RED:
		for (size_t j = 0; j < MaxRedTeamStarts; ++j)
		{
			i = M_Random() % selections;
			if (G_CheckSpot(player, &redteamstarts[i]))
			{
				return &redteamstarts[i];
			}
		}
		return &redteamstarts[i];
	default:
		// This team doesn't have a dedicated spawn point.  Fallthrough
		// to using a deathmatch spawn point.
		break;
	}

	return SelectRandomDeathmatchSpot(player, selections);
}

void G_TeamSpawnPlayer(player_t &player) // [Toke - CTF - starts] Modified this function to accept teamplay starts
{
	int selections;
	mapthing2_t *spot = NULL;

	selections = 0;

	if (player.userinfo.team == TEAM_BLUE)  // [Toke - CTF - starts]
		selections = blueteam_p - blueteamstarts;

	if (player.userinfo.team == TEAM_RED)  // [Toke - CTF - starts]
		selections = redteam_p - redteamstarts;

	// denis - fall back to deathmatch spawnpoints, if no team ones available
	if (selections < 1)
	{
		selections = deathmatch_p - deathmatchstarts;

		if (selections)
		{
			spot = SelectRandomDeathmatchSpot(player, selections);
		}
	}
	else
	{
		spot = SelectRandomTeamSpot(player, selections);  // [Toke - Teams]
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

	if(sv_gametype == GM_COOP)
		return;

	if(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
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

	// respawn at the start
	// first disassociate the corpse
	if (player.mo)
		player.mo->player = NULL;

	// spawn at random team spot if in team game
	if(sv_gametype == GM_TEAMDM || sv_gametype == GM_CTF)
	{
		G_TeamSpawnPlayer (player);
		return;
	}

	// spawn at random spot if in death match
	if (sv_gametype != GM_COOP)
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

void G_BuildSaveName (std::string &name, int slot)
{
}

void G_DoSaveGame (void)
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


VERSION_CONTROL (g_game_cpp, "$Id$")

