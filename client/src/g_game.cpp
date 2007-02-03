// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2007 by The Odamex Team.
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
#include "f_finale.h"
#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"
#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"
#include "d_main.h"
#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"
#include "c_console.h"
#include "c_cvars.h"
#include "c_bind.h"
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
#include "cl_main.h"
#include "gi.h"

#define SAVESTRINGSIZE	24

#define TURN180_TICKS	9				// [RH] # of ticks to complete a turn180

BOOL	G_CheckDemoStatus (void);
void	G_ReadDemoTiccmd ();
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

void	CL_RunTics (void);

CVAR (skill, "1", CVAR_ARCHIVE); // [Toke - todo] should not be cvar on client
CVAR (deathmatch, "1", CVAR_ARCHIVE);  // [Toke - todo] should not be cvar on client

EXTERN_CVAR (novert)



CVAR (chasedemo, "0", 0);

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

bool	 		viewactive;

BOOL 						netgame;				// only true if packets are broadcast
BOOL						multiplayer;
std::vector<player_t>		players;
player_t					nullplayer;				// the null player

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

EXTERN_CVAR(nomonsters)
EXTERN_CVAR(fastmonsters)
EXTERN_CVAR(freelook)

byte			consoleplayer_id;			// player taking events and displaying
byte			displayplayer_id;			// view being displayed
int 			gametic;

char			demoname[256];
BOOL 			demorecording;
BOOL 			demoplayback;
BOOL 			netdemo;
BOOL			demonew;				// [RH] Only used around G_InitNew for demos
int				iffdemover;
byte*			demobuffer;
byte			*demo_p, *demo_e;
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

CVAR (cl_run,		"1",	CVAR_ARCHIVE)		// Always run? // [Toke - Defaults]
CVAR (invertmouse,	"0",	CVAR_ARCHIVE)		// Invert mouse look down/up?
CVAR (lookstrafe,	"0",	CVAR_ARCHIVE)		// Always strafe with mouse?
CVAR (m_pitch,		"1.0",	CVAR_ARCHIVE)		// Mouse speeds
CVAR (m_yaw,		"1.0",	CVAR_ARCHIVE)
CVAR (m_forward,	"1.0",	CVAR_ARCHIVE)
CVAR (m_side,		"2.0",	CVAR_ARCHIVE)
CVAR (displaymouse,	"0",	CVAR_ARCHIVE)		// [Toke - Mouse] added for mouse menu

int 			turnheld;								// for accelerative turning

// [Toke - Mouse] new mouse stuff
unsigned int	mousexleft;
int	mousex;
unsigned int	mouseydown;
int	mousey;
float			zdoomsens;


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
	size_t translation[MAXPLAYERS];
	size_t size = players.size();

	if(id >= MAXPLAYERS)
		return nullplayer;

	size_t tid = translation[id];
	if(tid < size
	&& players[tid].id == id)
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

// [RH] Name of screenshot file to generate (usually NULL)
std::string		shotfile;

// [Fly] don't allow to change turbo in csDoom
void G_SetDefaultTurbo (void)
{
	forwardmove[0] = (int)(normforwardmove[0]);
	forwardmove[1] = (int)(normforwardmove[1]);
	sidemove[0] = (int)(normsidemove[0]);
	sidemove[1] = (int)(normsidemove[1]);
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

static int turntick;

BEGIN_COMMAND (turn180)
{
	turntick = TURN180_TICKS;
}
END_COMMAND (turn180)

//
// [RH] WeapNext and WeapPrev commands
//

static const char *weaponnames[] =
{
	"Fist",
	"Pistol",
	"Shotgun",
	"Chaingun",
	"Rocket Launcher",
	"Plasma Gun",
	"BFG9000",
	"Chainsaw",
	"Super Shotgun",
	"Chainsaw"
};

BEGIN_COMMAND (weapnext)
{
	player_t *plyr = m_Instigator->player;
	gitem_t *item = FindItem (weaponnames[plyr->readyweapon]);
	int selected_weapon;
	int i, index;

	if (!item)
		return;

	selected_weapon = ITEM_INDEX(item);

	for (i = 1; i <= num_items; i++)
	{
		index = (selected_weapon + i) % num_items;
		if (!(itemlist[index].flags & IT_WEAPON))
			continue;
		if (!plyr->weaponowned[itemlist[index].offset])
			continue;
		if (!plyr->ammo[weaponinfo[itemlist[index].offset].ammo])
			continue;
		Impulse = itemlist[index].offset + 50;
		return;
	}
}
END_COMMAND (weapnext)

BEGIN_COMMAND (weapprev)
{
	player_t *plyr = m_Instigator->player;
	gitem_t *item = FindItem (weaponnames[plyr->readyweapon]);
	int selected_weapon;
	int i, index;

	if (!item)
		return;

	selected_weapon = ITEM_INDEX(item);

	for (i = 1; i <= num_items; i++) {
		index = (selected_weapon + num_items - i) % num_items;
		if (!(itemlist[index].flags & IT_WEAPON))
			continue;
		if (!plyr->weaponowned[itemlist[index].offset])
			continue;
		if (!plyr->ammo[weaponinfo[itemlist[index].offset].ammo])
			continue;
		Impulse = itemlist[index].offset + 50;
		return;
	}
}
END_COMMAND (weapprev)

BEGIN_COMMAND (spynext)
{
	extern bool ctfmode, teamplaymode;
	
	size_t curr;
	size_t s = players.size();
	for(curr = 0; curr < s; curr++)
		if(players[curr].id == displayplayer().id)
			break;
	
	for(size_t i = 1; i < s; i++)
	{
		curr = (curr+1)%s;
		
		if(!players[curr].mo)
			continue;
			
		if(!deathmatch || ((teamplaymode || ctfmode) && players[curr].userinfo.team == consoleplayer().userinfo.team))
		{
			displayplayer_id = players[curr].id;
			break;
		}
	}
}
END_COMMAND (spynext)

extern constate_e ConsoleState;

//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer.
// If recording a demo, write it out
//
void G_BuildTiccmd (ticcmd_t *cmd)
{
	int 		strafe;
	int 		speed;
	int 		tspeed;
	int 		forward;
	int 		side;
	int			look;
	int			fly;

	ticcmd_t	*base;

	base = I_BaseTiccmd (); 			// empty, or external driver

	memcpy (cmd,base,sizeof(*cmd));

	strafe = Actions[ACTION_STRAFE];
	speed = Actions[ACTION_SPEED];
	if (cl_run)
		speed ^= 1;

	forward = side = look = fly = 0;

	// [RH] only use two stage accelerative turning on the keyboard
	//		and not the joystick, since we treat the joystick as
	//		the analog device it is.
	if ((Actions[ACTION_LEFT]) || (Actions[ACTION_RIGHT]))
		turnheld += 1;
	else
		turnheld = 0;

	if (turnheld < SLOWTURNTICS)
		tspeed = 2; 			// slow turn
	else
		tspeed = speed;

	// let movement keys cancel each other out
	if (strafe)
	{
		if (Actions[ACTION_RIGHT])
			side += sidemove[speed];
		if (Actions[ACTION_LEFT])
			side -= sidemove[speed];
	}
	else
	{
		if (Actions[ACTION_RIGHT])
			cmd->ucmd.yaw -= angleturn[tspeed];
		if (Actions[ACTION_LEFT])
			cmd->ucmd.yaw += angleturn[tspeed];
	}

	if (Actions[ACTION_LOOKUP])
		look += lookspeed[speed];
	if (Actions[ACTION_LOOKDOWN])
		look -= lookspeed[speed];

	if (Actions[ACTION_MOVEUP])
		fly += flyspeed[speed];
	if (Actions[ACTION_MOVEDOWN])
		fly -= flyspeed[speed];

	if (Actions[ACTION_KLOOK])
	{
		if (Actions[ACTION_FORWARD])
			look += lookspeed[speed];
		if (Actions[ACTION_BACK])
			look -= lookspeed[speed];
	}
	else
	{
		if (Actions[ACTION_FORWARD])
			forward += forwardmove[speed];
		if (Actions[ACTION_BACK])
			forward -= forwardmove[speed];
	}

	if (Actions[ACTION_MOVERIGHT])
		side += sidemove[speed];
	if (Actions[ACTION_MOVELEFT])
		side -= sidemove[speed];

	// buttons
	if (Actions[ACTION_ATTACK] && ConsoleState == c_up) // john - only add attack when console up
		cmd->ucmd.buttons |= BT_ATTACK;

	if (Actions[ACTION_USE])
		cmd->ucmd.buttons |= BT_USE;

	if (Actions[ACTION_JUMP])
		cmd->ucmd.buttons |= BT_JUMP;

	// [RH] Handle impulses. If they are between 1 and 7,
	//		they get sent as weapon change events.
	if (Impulse >= 1 && Impulse <= 7)
	{
		cmd->ucmd.buttons |= BT_CHANGE;
		cmd->ucmd.buttons |= (Impulse - 1) << BT_WEAPONSHIFT;
	}
	else
	{
		cmd->ucmd.impulse = Impulse;
	}
	Impulse = 0;

	// [RH] Scale joystick moves to full range of allowed speeds
	if (strafe || lookstrafe)
		side += (MAXPLMOVE * joyxmove) / 256;
	else
		cmd->ucmd.yaw -= (angleturn[1] * joyxmove) / 256;

	// [RH] Scale joystick moves over full range
	if (Actions[ACTION_MLOOK])
	{
		if (invertmouse)
			look -= (joyymove * 32767) / 256;
		else
			look += (joyymove * 32767) / 256;
	}
	else
	{
		forward += (MAXPLMOVE * joyymove) / 256;
	}

	if ((Actions[ACTION_MLOOK]) || freelook)
	{
		int val;

		val = (int)((float)(mousey * 16) * m_pitch);
		if (invertmouse)
			look -= val;
		else
			look += val;
	}
	else
	{
		if (novert == 0) // [Toke - Mouse] acts like novert.exe
		{
			forward += (int)((float)mousey * m_forward);
		}
	}

	if (sendcenterview)
	{
		sendcenterview = false;
		look = -32768;
	}
	else
	{
		if (look > 32767)
			look = 32767;
		else if (look < -32767)
			look = -32767;
	}

	if (strafe || lookstrafe)
		side += (int)((float)mousex * m_side);
	else
		cmd->ucmd.yaw -= (int)((float)(mousex*0x8) * m_yaw);

	mousex = mousey = 0;

	if (forward > MAXPLMOVE)
		forward = MAXPLMOVE;
	else if (forward < -MAXPLMOVE)
		forward = -MAXPLMOVE;
	if (side > MAXPLMOVE)
		side = MAXPLMOVE;
	else if (side < -MAXPLMOVE)
		side = -MAXPLMOVE;

	cmd->ucmd.forwardmove += forward;
	cmd->ucmd.sidemove += side;
	cmd->ucmd.pitch = look;
	cmd->ucmd.upmove = fly;

	// special buttons
	if (sendpause)
	{
		sendpause = false;
		cmd->ucmd.buttons = BT_SPECIAL | BTS_PAUSE;
	}

	if (sendsave)
	{
		sendsave = false;
		cmd->ucmd.buttons = BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT);
	}

	cmd->ucmd.forwardmove <<= 8;
	cmd->ucmd.sidemove <<= 8;

	// [RH] 180-degree turn overrides all other yaws
	if (turntick) {
		turntick--;
		cmd->ucmd.yaw = (ANG180 / TURN180_TICKS) >> 16;
	}

	joyxmove = 0;
	joyymove = 0;
}


// [RH] Spy mode has been separated into two console commands.
//		One goes forward; the other goes backward.
// denis - todo - should this be used somewhere?
/*static void ChangeSpy (void)
{
	consoleplayer().camera = players[displayplayer].mo;
	S_UpdateSounds(consoleplayer().camera);
	ST_Start();		// killough 3/7/98: switch status bar views too
}*/


//
// G_Responder
// Get info needed to make ticcmd_ts for the players.
//
BOOL G_Responder (event_t *ev)
{
	// any other key pops up menu if in demos
	// [RH] But only if the key isn't bound to a "special" command
	if (gameaction == ga_nothing &&
		(demoplayback || gamestate == GS_DEMOSCREEN))
	{
		const char *cmd = C_GetBinding (ev->data1);

		if (ev->type == ev_keydown)
		{

			if (!cmd || (
				strnicmp (cmd, "menu_", 5) &&
				stricmp (cmd, "toggleconsole") &&
				stricmp (cmd, "sizeup") &&
				stricmp (cmd, "sizedown") &&
				stricmp (cmd, "togglemap") &&
				stricmp (cmd, "spynext") &&
				stricmp (cmd, "chase") &&
				stricmp (cmd, "+showscores") &&
				stricmp (cmd, "bumpgamma") &&
				stricmp (cmd, "screenshot")))
			{
				S_Sound (CHAN_VOICE, "switches/normbutn", 1, ATTN_NONE);
				M_StartControlPanel ();
				return true;
			}
			else
			{
				return C_DoKey (ev);
			}
		}
		if (cmd && cmd[0] == '+')
			return C_DoKey (ev);

		return false;
	}

	if (gamestate == GS_LEVEL || gamestate == GS_INTERMISSION)
	{
		if (HU_Responder (ev))
			return true;		// chat ate the event
		if (ST_Responder (ev))
			return true;		// status window ate it
		if (!viewactive)
			if (AM_Responder (ev))
				return true;	// automap ate it
	}
	else if (gamestate == GS_FINALE)
	{
		if (F_Responder (ev))
			return true;		// finale ate the event
	}

	switch (ev->type)
	{
	  case ev_keydown:
		if (C_DoKey (ev))
			return true;
		break;

	  case ev_keyup:
		C_DoKey (ev);
		break;

	  // [Toke - Mouse] New mouse code
	  case ev_mouse:
		zdoomsens = (float)(mouse_sensitivity / 10);

		if (mouse_type == 0)
		{
			if (dynres_state == 0)
			{
				mousex = (unsigned int)(ev->data2 * (mouse_sensitivity + 5) / 10); // [Toke - Mouse] Marriage of origonal and zdoom mouse code, functions like doom2.exe code
				mousey = (unsigned int)(ev->data3 * (mouse_sensitivity + 5) / 10);
			}
			else if (dynres_state == 1)
			{
				mousexleft = ev->data2;
				mousexleft = -mousexleft;
				mousex = (unsigned int)pow((ev->data2 * (mouse_sensitivity + 5) / 10), dynresval);

				if (ev->data2 < 0)
				{
					mousexleft = (unsigned int)pow((mousexleft * (mouse_sensitivity + 5) / 10), dynresval);
					mousex = -mousexleft;
				}

				mouseydown = ev->data3;
				mouseydown = -mouseydown;
				mousey = (unsigned int)pow((ev->data3 * (mouse_sensitivity + 5) / 10), dynresval);

				if (ev->data3 < 0)
				{
					mouseydown = (unsigned int)pow((mouseydown * (mouse_sensitivity + 5) / 10), dynresval);
					mousey = -mouseydown;
				}
			}
		}
		else if (mouse_type == 1)
		{
			if (dynres_state == 0)
			{
				mousex = (unsigned int)(ev->data2 * (zdoomsens)); // [Toke - Mouse] Zdoom mouse code
				mousey = (unsigned int)(ev->data3 * (zdoomsens));
			}
			else if (dynres_state == 1)
			{
				mousexleft = ev->data2;
				mousexleft = -mousexleft;
				mousex = (unsigned int)pow((ev->data2 * (zdoomsens)), dynresval);

				if (ev->data2 < 0)
				{
					mousexleft = (unsigned int)pow((mousexleft * (zdoomsens)), dynresval);
					mousex = -mousexleft;
				}

				mouseydown = ev->data3;
				mouseydown = -mouseydown;
				mousey = (unsigned int)pow((ev->data3 * (zdoomsens)), dynresval);

				if (ev->data3 < 0)
				{
					mouseydown = (unsigned int)pow((mouseydown * (zdoomsens)), dynresval);
					mousey = -mouseydown;
				}
			}
		}

		if (displaymouse == 1)
			Printf(PRINT_MEDIUM, "%d ", mousex);

		break;

	  case ev_joystick:
		joyxmove = ev->data2;
		joyymove = ev->data3;
		break;

	}

	// [RH] If the view is active, give the automap a chance at
	// the events *last* so that any bound keys get precedence.

	if (gamestate == GS_LEVEL && viewactive)
		return AM_Responder (ev);

	if (ev->type == ev_keydown ||
		ev->type == ev_mouse ||
		ev->type == ev_joystick)
		return true;
	else
		return false;
}


int netin;
int netout;
int outrate;

BEGIN_COMMAND(netstat)
{
    Printf (PRINT_HIGH, "in = %d  out = %d", netin, netout);
}
END_COMMAND(netstat)


//
// G_Ticker
// Make ticcmd_ts for the players.
//
extern DCanvas *page;
extern int connecttimeout;

void G_Ticker (void)
{
	int 		buf;
	gamestate_t	oldgamestate;
	size_t i;

	// Run client tics;
	CL_RunTics ();

	// do player reborns if needed
	if(serverside)
		for (i = 0; i < players.size(); i++) 
			if (players[i].ingame() && players[i].playerstate == PST_REBORN) 
				G_DoReborn (players[i]);

	// do things to change the game state
	oldgamestate = gamestate;
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
			G_DoPlayDemo ();
			break;
		case ga_completed:
			G_DoCompleted ();
			break;
		case ga_victory:
		    gameaction = ga_nothing;
			break;
		case ga_worlddone:
			G_DoWorldDone ();
			break;
		case ga_screenshot:
			M_ScreenShot(shotfile.c_str());
			gameaction = ga_nothing;
			break;
		case ga_fullconsole:
			C_FullConsole ();
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

    // get commands
    buf = gametic%BACKUPTICS;
	memcpy (&consoleplayer().cmd, &consoleplayer().netcmds[buf], sizeof(ticcmd_t));

    static int realrate = 0;
    int packet_size;

	if (demoplayback)
	{
		// play all player commands
		G_ReadDemoTiccmd();
	}
    else if (connected)
    {
       while ((packet_size = NET_GetPacket()) )
       {
		   // denis - don't accept candy from strangers
		   if(!NET_CompareAdr(serveraddr, net_from))
			  break;

           realrate += packet_size;
		   last_received = gametic;
		   noservermsgs = false;

		   CL_ReadPacketHeader();
           CL_ParseCommands();

		   if (gameaction == ga_fullconsole) // Host_EndGame was called
			   return;
       }

       if (!(gametic%TICRATE))
       {
          netin = realrate;
          realrate = 0;
       }

       if (!noservermsgs)
		   CL_SendCmd();  // send console commands to the server

       CL_SaveCmd();      // save console commands

       if (!(gametic%TICRATE))
       {
           netout = outrate;
           outrate = 0;
       }

	   if (gametic - last_received > 65)
		   noservermsgs = true;
	}
	else if (NET_GetPacket() )
	{
		// denis - don't accept candy from strangers
		if((gamestate == GS_DOWNLOAD || gamestate == GS_CONNECTING)
			&& NET_CompareAdr(serveraddr, net_from))
		{
			int type = MSG_ReadLong();

			if(type == CHALLENGE)
			{
				CL_PrepareConnect();
			}
			else if(type == 0)
			{
				if (!CL_Connect())
					memset (&serveraddr, 0, sizeof(serveraddr));

				connecttimeout = 0;
			}
			else
			{
				// we are already connected to this server, quit first
				MSG_WriteMarker(&net_buffer, clc_disconnect);
				NET_SendPacket(net_buffer, serveraddr);
			}
		}
	}

	// do main actions
	switch (gamestate)
	{
	case GS_LEVEL:
		if(!demoplayback)
			CL_PredictMove ();
		P_Ticker ();
		ST_Ticker ();
		AM_Ticker ();
		break;

	case GS_INTERMISSION:
		ST_Ticker ();
		WI_Ticker ();
		break;

	case GS_FINALE:
		F_Ticker ();
		break;

	case GS_DEMOSCREEN:
		D_PageTicker (); 
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
void G_PlayerReborn (player_t &p)
{
/*	player_t*	p;
	int 		i;
	int			fragcount;	// [RH] Cumulative frags
	int			deathcount;	 // [Toke - Scores - deaths]
	int 		killcount;
	int			ping;
	int			GameTime;
	int			points; // [Toke - score]
	int			id;
	userinfo_t	userinfo;	// [RH] Save userinfo

	p = &player;

	id = p->id;
	points = p->points; // [Toke - score]
	fragcount = p->fragcount;
	deathcount = p->deathcount; // [Toke - Scores - deaths]
	killcount = p->killcount;
	ping = p->ping;
	GameTime = p->GameTime;
	
	memcpy (&userinfo, &p->userinfo, sizeof(userinfo));

	p->camera = p->mo = p->attacker = AActor::AActorPtr();
	memset (p, 0, sizeof(*p));
	p->camera = p->mo = p->attacker = AActor::AActorPtr();

	p->id = id;
//	p->state = state;
	p->points = points; // [Toke - score]
	p->fragcount = fragcount;
	p->deathcount = deathcount; // [Toke - Scores - deaths]
	p->killcount = killcount;
	p->ping = ping;
	p->GameTime = GameTime;
	
	memcpy (&p->userinfo, &userinfo, sizeof(userinfo));

	p->usedown = p->attackdown = true;	// don't do anything immediately
	p->playerstate = PST_LIVE;
	p->health = deh.StartHealth;		// [RH] Used to be MAXHEALTH
	p->readyweapon = p->pendingweapon = wp_pistol;
	p->weaponowned[wp_fist] = true;
	p->weaponowned[wp_pistol] = true;
	p->ammo[am_clip] = deh.StartBullets; // [RH] Used to be 50

	for (i = 0; (i < NUMAMMO); i++)
		p->maxammo[i] = maxammo[i];*/

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
	subsector_t*		ss;
	unsigned			an;
	AActor* 			mo;
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

	// spawn a teleport fog
	an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT;

	mo = new AActor (x+20*finecosine[an], y+20*finesine[an], z, MT_TFOG);

	if (level.time)
		S_Sound (mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);	// don't start sound on first frame

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

void G_DeathMatchSpawnPlayer (player_t &player)
{
	int selections;
	mapthing2_t *spot;
	
	if(!serverside)
		return;
	
	//if (!ctfmode)
	{
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
		
		
	}
	/*else
	{
		selections = 0;
		
		if (player.userinfo.team == TEAM_BLUE)  // [Toke - CTF - starts]
		{
			selections = blueteam_p - blueteamstarts;
			
			if (selections < 1)
				I_Error ("No blue team starts");
		}
		
		if (player.userinfo.team == TEAM_RED)  // [Toke - CTF - starts]
		{
			selections = redteam_p - redteamstarts;
			
			if (selections < 1)
				I_Error ("No red team starts");
		}
		
		if (player.userinfo.team == TEAM_GOLD)  // [Toke - CTF - starts]
		{
			selections = goldteam_p - goldteamstarts;
			
			if (selections < 1)
				I_Error ("No gold team starts");
		}
		
		if (selections < 1)
			I_Error ("No team starts");
		
		spot = CTF_SelectTeamPlaySpot (player, selections);  // [Toke - Teams]
		
		if (!spot && !playerstarts.empty())
			spot = &playerstarts[player.id%playerstarts.size()];
		
		else 
		{
			if (player.id < 4)
				spot->type = player.id+1;
			else
				spot->type = player.id+4001-4;
		}
	}*/
	
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
	
	// spawn at random spot if in death match
	if (deathmatch)
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
   // SoM: THIS CRASHES A LOT
   if(filename && *filename)
	   shotfile = filename;
   else
      shotfile = "";

	gameaction = ga_screenshot;
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
// DEMO RECORDING
//
#define DEMOMARKER				0x80

static usercmd_t LastUserCmd;

static void MakeEmptyUserCmd (void)
{
	memset (&LastUserCmd, 0, sizeof(usercmd_t));
}

void G_ReadDemoTiccmd ()
{
	if(demoversion == LMP_DOOM_1_9 || demoversion == LMP_DOOM_1_9_1)
	{
		for(size_t i = 0; i < players.size(); i++)
		{
			if ((demoversion == LMP_DOOM_1_9 && demo_e - demo_p < 4)
			|| (demoversion == LMP_DOOM_1_9_1 && demo_e - demo_p < 5)
			|| (*demo_p == DEMOMARKER))
			{
				// end of demo data stream 
				G_CheckDemoStatus (); 
				return; 
			}
			
			usercmd_t *ucmd = &players[i].cmd.ucmd;

			ucmd->forwardmove = ((signed char)*demo_p++)<<8;
			ucmd->sidemove = ((signed char)*demo_p++)<<8;
			if(demoversion == LMP_DOOM_1_9)
				ucmd->yaw = ((unsigned char)*demo_p++)<<8;
			else
			{
				ucmd->yaw = ((unsigned short)*demo_p++);
				ucmd->yaw |= ((unsigned short)*demo_p++)<<8;
			}
			ucmd->buttons = (unsigned char)*demo_p++;
		}
		
		return;
	}
	
	if(demoversion == ZDOOM_FORM)
	{
		for(size_t i = 0; i < players.size(); i++)
		{
			static int clonecount = 0;
			int id = DEM_BAD;
			ticcmd_t *cmd = &players[i].cmd;

			while (!clonecount && id != DEM_USERCMD && id != DEM_USERCMDCLONE)
			{
				if (!demorecording && demo_p >= zdembodyend)
				{
					// nothing left in the BODY chunk, so end playback.
					G_CheckDemoStatus ();
					break;
				}

				id = ReadByte (&demo_p);

				switch (id)
				{
				case DEM_STOP:
					// end of demo stream
					G_CheckDemoStatus ();
					break;
				case DEM_USERCMD:
					UnpackUserCmd (&cmd->ucmd, &demo_p);
					memcpy (&LastUserCmd, &cmd->ucmd, sizeof(usercmd_t));
					break;
				case DEM_USERCMDCLONE:
					clonecount = ReadByte (&demo_p) + 1;
					break;

				case DEM_DROPPLAYER:
		//			i = ReadByte (&demo_p);
		//			if (players.valid(i))
		//				players[i].state = player_t::disconnected;

				default:
		//			Net_DoCommand (id, &demo_p, player);
					break;
				}
			}
			if (clonecount)
			{
				clonecount--;
				memcpy (&cmd->ucmd, &LastUserCmd, sizeof(usercmd_t));
			}
		}
	}
}

BOOL stoprecording;


extern byte *lenspot;

void G_WriteDemoTiccmd (ticcmd_t *cmd, int player, int buf)
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

std::string defdemoname;

void G_DeferedPlayDemo (char *name)
{
	defdemoname = name;
	gameaction = ga_playdemo;
}

BEGIN_COMMAND(playdemo)
{
	if(argc > 1)
		G_DeferedPlayDemo(argv[1]);
	else
		Printf(PRINT_HIGH, "usage: playdemo [lump or filename]");
}
END_COMMAND(playdemo)

// [RH] Process all the information in a FORM ZDEM
//		until a BODY chunk is entered.
BOOL G_ProcessIFFDemo (char *mapname)
{
	BOOL headerHit = false;
	BOOL bodyHit = false;
	int numPlayers = 0;
	int id, len;
	size_t i;
	byte *nextchunk;

	demoplayback = true;

	players.clear();

	len = ReadLong (&demo_p);
	zdemformend = demo_p + len + (len & 1);

	// Check to make sure this is a ZDEM chunk file.
	// TODO: Support multiple FORM ZDEMs in a CAT. Might be useful.

	id = ReadLong (&demo_p);
	if (id != ZDEM_ID) {
		Printf (PRINT_HIGH, "Not an ZDEM demo file\n");
		return true;
	}

	// Process all chunks until a BODY chunk is encountered.

	while (demo_p < zdemformend && !bodyHit) {
		id = ReadLong (&demo_p);
		len = ReadLong (&demo_p);
		nextchunk = demo_p + len + (len & 1);
		if (nextchunk > zdemformend) {
			Printf (PRINT_HIGH, "Demo is mangled\n");
			return true;
		}

		switch (id) {
			case ZDHD_ID:
				headerHit = true;

				iffdemover = ReadWord (&demo_p);	// ZDoom version demo was created with
				if (ReadWord (&demo_p) > GAMEVER) {		// Minimum ZDoom version
					Printf (PRINT_HIGH, "Demo requires newer software version\n");
					return true;
				}
				memcpy (mapname, demo_p, 8);	// Read map name
				mapname[8] = 0;
				demo_p += 8;
				//rngseed = ReadLong (&demo_p);
				//consoleplayer = displayplayer = *demo_p++;
				break;

			case VARS_ID:
				cvar_t::C_ReadCVars (&demo_p);
				break;

			case UINF_ID:
				i = ReadByte (&demo_p);
				if (!players[i].ingame()) {
					//players[i].state = player_t::playing;
					numPlayers++;
				}
				D_ReadUserInfoStrings (i, &demo_p, false);
				break;

			case BODY_ID:
				bodyHit = true;
				zdembodyend = demo_p + len;
				break;
		}

		if (!bodyHit)
			demo_p = nextchunk;
	}

	if (!numPlayers) {
		Printf (PRINT_HIGH, "Demo has no players\n");
		return true;
	}

	if (!bodyHit) {
		zdembodyend = NULL;
		Printf (PRINT_HIGH, "Demo has no BODY chunk\n");
		return true;
	}

	if (numPlayers > 1)
		multiplayer = netgame = netdemo = true;

	return false;
}

void G_DoPlayDemo (void)
{
	char mapname[9];

	CL_QuitNetGame();

	gameaction = ga_nothing;
	int bytelen;

	// [RH] Allow for demos not loaded as lumps
	int demolump = W_CheckNumForName (defdemoname.c_str());
	if (demolump >= 0) {
		demobuffer = demo_p = (byte *)W_CacheLumpNum (demolump, PU_STATIC);
		bytelen = W_LumpLength(demolump);
	} else {
		FixPathSeparator (defdemoname);
		DefaultExtension (defdemoname, ".lmp");
		bytelen = M_ReadFile (defdemoname.c_str(), &demobuffer);
		demo_p = demobuffer;
	}

	demo_e = demo_p + bytelen;

	if(bytelen < 4)
	{
		Z_Free(demobuffer);
		Printf (PRINT_HIGH, "Demo file too short\n");
		gameaction = ga_nothing;
		return;
	}
		
	Printf (PRINT_HIGH, "Playing demo %s\n", defdemoname.c_str());

	cvar_t::C_BackupCVars ();		// [RH] Save cvars that might be affected by demo
	MakeEmptyUserCmd ();
	
	if(demo_p[0] == DOOM_1_4_DEMO
	|| demo_p[0] == DOOM_1_5_DEMO
	|| demo_p[0] == DOOM_1_6_DEMO
	|| demo_p[0] == DOOM_1_7_DEMO
	|| demo_p[0] == DOOM_1_8_DEMO
	|| demo_p[0] == DOOM_1_9_DEMO
	|| demo_p[0] == DOOM_1_9p_DEMO
	|| demo_p[0] == DOOM_1_9_1_DEMO)
	{
		if(bytelen < 14)
		{
			Z_Free(demobuffer);
			Printf (PRINT_HIGH, "DOOM 1.9 Demo: file too short\n");
			gameaction = ga_nothing;
			return;
		}
		
		demoversion = *demo_p++ == DOOM_1_9_1_DEMO ? LMP_DOOM_1_9_1 : LMP_DOOM_1_9;
		float s = (float)((*demo_p++)+1);
		skill = s;
		byte episode = *demo_p++;
		byte map = *demo_p++;
		deathmatch = *demo_p++;
		respawnmonsters = *demo_p++;
		fastmonsters = *demo_p++;
		nomonsters = *demo_p++;
		byte who = *demo_p++;
		
		players.clear();

		for (size_t i=0 ; i < MAXPLAYERS_VANILLA; i++) 
		{
			if(*demo_p++)
			{
				players.push_back(player_t());
				players.back().playerstate = PST_REBORN;
				players.back().id = i + 1;
			}
		}
		
		player_t &con = idplayer(who + 1);
		
		if(!validplayer(con))
		{
			Z_Free(demobuffer);
			Printf (PRINT_HIGH, "DOOM 1.9 Demo: invalid console player %d of %d\n", (int)who+1, players.size());
			gameaction = ga_nothing;
			return;
		}
		
		consoleplayer_id = displayplayer_id = con.id;

		if(players.size() > 1)
		{
			netgame = true;
			netdemo = true;
			multiplayer = true;
		}
		else
		{
			netgame = false;
			netdemo = false;
			multiplayer = false;
		}

		char mapname[32];
		
		if(gameinfo.flags & GI_MAPxx)
			sprintf(mapname, "MAP%02d", (int)map);
		else
			sprintf(mapname, "E%dM%d", (int)episode, (int)map);

		serverside = true;

		G_InitNew (mapname);
				
		usergame = false;
		demoplayback = true;
		
		// comatibility
		freelook = "0";

		return;
	}
	
	if (ReadLong (&demo_p) != FORM_ID) {
			Printf (PRINT_HIGH, "Unknown demo format.\n");
			gameaction = ga_nothing;
	} else if (G_ProcessIFFDemo (mapname)) {
		gameaction = ga_nothing;
		demoplayback = false;
	} else {
		demoversion = ZDOOM_FORM;
		// don't spend a lot of time in loadlevel
		precache = false;
		demonew = true;
		G_InitNew (mapname);
		C_HideConsole ();
		demonew = false;
		precache = true;

		usergame = false;
		demoplayback = true;
	}
}

//
// G_TimeDemo
//
void G_TimeDemo (char* name)
{
	nodrawers = Args.CheckParm ("-nodraw");
	noblit = Args.CheckParm ("-noblit");
	timingdemo = true;

	defdemoname = name;
	gameaction = ga_playdemo;
}


/*
===================
=
= G_CheckDemoStatus
=
= Called after a death or level completion to allow demos to be cleaned up
= Returns true if a new demo loop action will take place
===================
*/

BOOL G_CheckDemoStatus (void)
{
	if (demoplayback)
	{
		extern int starttime;
		int endtime = 0;

		if (timingdemo)
			endtime = I_GetTimePolled () - starttime;

		cvar_t::C_RestoreCVars ();		// [RH] Restore cvars demo might have changed

		Z_Free (demobuffer);
		
		demoplayback = false;
		netdemo = false;
		netgame = false;
		multiplayer = false;
		serverside = false;
		
		players.clear();

		if (singledemo || timingdemo) {
			if (timingdemo)
				// Trying to get back to a stable state after timing a demo
				// seems to cause problems. I don't feel like fixing that
				// right now.
				I_FatalError ("timed %i gametics in %i realtics (%.1f fps)", gametic,
							  endtime, (float)gametic/(float)endtime*(float)TICRATE);
			else
				Printf (PRINT_HIGH, "Demo ended.\n");
			gameaction = ga_fullconsole;
			timingdemo = false;
			return false;
		} else {
			D_AdvanceDemo ();
		}

		return true;
	}

	if (demorecording)
	{
		byte *formlen;

		WriteByte (DEM_STOP, &demo_p);
		//FinishChunk (&demo_p);
		formlen = demobuffer + 4;
		WriteLong (demo_p - demobuffer - 8, &formlen);

		M_WriteFile (demoname, demobuffer, demo_p - demobuffer);
		free (demobuffer);
		demorecording = false;
		stoprecording = false;
		Printf (PRINT_HIGH, "Demo %s recorded\n", demoname);
	}

	return false;
}

BOOL CheckIfExitIsGood (AActor *self)
{
	return false;
}


VERSION_CONTROL (g_game_cpp, "$Id$")

