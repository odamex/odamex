// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2013 by The Odamex Team.
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
#include "m_fileio.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"
#include "i_input.h"
#include "hardware.h"
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
#include "gstrings.h"
#include "r_data.h"
#include "r_sky.h"
#include "r_draw.h"
#include "g_game.h"
#include "g_level.h"
#include "cl_main.h"
#include "cl_demo.h"
#include "gi.h"
#include "hu_mousegraph.h"

#ifdef _XBOX
#include "i_xbox.h"
#endif

#include <math.h> // for pow()

#include <sstream>

#define SAVESTRINGSIZE	24

#define TURN180_TICKS	9				// [RH] # of ticks to complete a turn180

BOOL	G_CheckDemoStatus (void);
void	G_ReadDemoTiccmd ();
void	G_WriteDemoTiccmd ();
void	G_PlayerReborn (player_t &player);

void	G_DoNewGame (void);
void	G_DoLoadGame (void);
//void	G_DoPlayDemo (bool justStreamInput = false);
void	G_DoCompleted (void);
void	G_DoVictory (void);
void	G_DoWorldDone (void);
void	G_DoSaveGame (void);

void	CL_RunTics (void);

bool	C_DoNetDemoKey(event_t *ev);
bool	C_DoSpectatorKey(event_t *ev);

EXTERN_CVAR (sv_skill)
EXTERN_CVAR (novert)
EXTERN_CVAR (sv_monstersrespawn)
EXTERN_CVAR (sv_itemsrespawn)
EXTERN_CVAR (sv_weaponstay)
EXTERN_CVAR (sv_keepkeys)
EXTERN_CVAR (co_nosilentspawns)
EXTERN_CVAR (co_allowdropoff)

EXTERN_CVAR (chasedemo)

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

// Describes if a network game is being played
BOOL			network_game;
// Use only for demos, it is a old variable for the old network code
BOOL			netgame;
// Describes if this is a multiplayer game or not
BOOL			multiplayer;
// The player vector, contains all player information
std::vector<player_t>		players;

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

EXTERN_CVAR(sv_nomonsters)
EXTERN_CVAR(sv_fastmonsters)
EXTERN_CVAR(sv_freelook)
EXTERN_CVAR(sv_allowjump)
EXTERN_CVAR(co_realactorheight)
EXTERN_CVAR(co_zdoomphys)
EXTERN_CVAR(co_fixweaponimpacts)
EXTERN_CVAR(co_blockmapfix)
EXTERN_CVAR (dynresval) // [Toke - Mouse] Dynamic Resolution Value
EXTERN_CVAR (dynres_state) // [Toke - Mouse] Dynamic Resolution on/off
EXTERN_CVAR (mouse_type) // [Toke - Mouse] Zdoom or standard mouse code
EXTERN_CVAR (m_filter)
EXTERN_CVAR (hud_mousegraph)
EXTERN_CVAR (cl_predictpickup)

CVAR_FUNC_IMPL(cl_mouselook)
{
	// Nes - center the view
	AddCommandString("centerview");

	// Nes - update skies
	R_InitSkyMap ();
}

char			demoname[256];
BOOL 			demorecording;
BOOL 			demoplayback;
BOOL			democlassic;
BOOL			demonew;				// [RH] Only used around G_InitNew for demos

extern bool		simulated_connection;

int				iffdemover;
byte*			demobuffer;
byte			*demo_p, *demo_e;
size_t			maxdemosize;
byte*			zdemformend;			// end of FORM ZDEM chunk
byte*			zdembodyend;			// end of ZDEM BODY chunk
BOOL 			singledemo; 			// quit after playing a demo from cmdline
int			demostartgametic;
FILE *recorddemo_fp;

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

EXTERN_CVAR (cl_run)
EXTERN_CVAR (invertmouse)
EXTERN_CVAR (lookstrafe)
EXTERN_CVAR (m_pitch)
EXTERN_CVAR (m_yaw)
EXTERN_CVAR (m_forward)
EXTERN_CVAR (m_side)
EXTERN_CVAR (displaymouse)

int 			turnheld;								// for accelerative turning

// mouse values are used once
int 			mousex;
int 			mousey;

// [Toke - Mouse] new mouse stuff
int	mousexleft;
int	mouseydown;

// Joystick values are repeated
// Store a value for each of the analog axis controls -- Hyper_Eye
int				joyforward;
int				joystrafe;
int				joyturn;
int				joylook;

EXTERN_CVAR (joy_forwardaxis)
EXTERN_CVAR (joy_strafeaxis)
EXTERN_CVAR (joy_turnaxis)
EXTERN_CVAR (joy_lookaxis)
EXTERN_CVAR (joy_sensitivity)
EXTERN_CVAR (joy_invert)
EXTERN_CVAR (joy_freelook)

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

player_t		&listenplayer()
{
	return displayplayer();
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

BEGIN_COMMAND (turnspeeds)
{
	if (argc == 1)
	{
		Printf (PRINT_HIGH, "Current turn speeds: %ld %ld %ld\n",
				angleturn[0], angleturn[1], angleturn[2]);
	}
	else
	{
		size_t i;
		for (i = 1; i <= 3 && i < argc; i++)
			angleturn[i-1] = atoi (argv[i]);

		if (i <= 2)
			angleturn[1] = angleturn[0] * 2;
		if (i <= 3)
			angleturn[2] = angleturn[0] / 2;
	}
}
END_COMMAND (turnspeeds)

static int turntick;
BEGIN_COMMAND (turn180)
{
	turntick = TURN180_TICKS;
}
END_COMMAND (turn180)

weapontype_t P_GetNextWeapon(player_t *player, bool forward);
BEGIN_COMMAND (weapnext)
{
	weapontype_t newweapon = P_GetNextWeapon(&consoleplayer(), true);
	if (newweapon != wp_nochange)
		Impulse = int(newweapon) + 50;
}
END_COMMAND (weapnext)

BEGIN_COMMAND (weapprev)
{
	weapontype_t newweapon = P_GetNextWeapon(&consoleplayer(), false);
	if (newweapon != wp_nochange)
		Impulse = int(newweapon) + 50;
}
END_COMMAND (weapprev)

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

	// GhostlyDeath -- USE takes us out of spectator mode
	if ((&consoleplayer())->spectator && Actions[ACTION_USE] && connected)
	{
		AddCommandString("join");
	}

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

	// Joystick analog strafing -- Hyper_Eye
	side += (int)(((float)joystrafe / (float)SHRT_MAX) * sidemove[speed]);

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

	// Joystick analog look -- Hyper_Eye
	if(joy_freelook && sv_freelook)
	{
		if (joy_invert)
			look += (int)(((float)joylook / (float)SHRT_MAX) * lookspeed[speed]);
		else
			look -= (int)(((float)joylook / (float)SHRT_MAX) * lookspeed[speed]);
	}

	if (Actions[ACTION_MOVERIGHT])
		side += sidemove[speed];
	if (Actions[ACTION_MOVELEFT])
		side -= sidemove[speed];

	// buttons
	if (Actions[ACTION_ATTACK] && ConsoleState == c_up && !headsupactive) // john - only add attack when console up
		cmd->ucmd.buttons |= BT_ATTACK;

	if (Actions[ACTION_USE])
		cmd->ucmd.buttons |= BT_USE;

	if (Actions[ACTION_JUMP])
		cmd->ucmd.buttons |= BT_JUMP;

	// [RH] Handle impulses. If they are between 1 and 7,
	//		they get sent as weapon change events.
	if (Impulse >= 1 && Impulse <= 8)
	{
		cmd->ucmd.buttons |= BT_CHANGE;
		cmd->ucmd.buttons |= (Impulse - 1) << BT_WEAPONSHIFT;
	}
	else
	{
		cmd->ucmd.impulse = Impulse;
	}
	Impulse = 0;

	// [SL] 2012-03-31 - Let the server know when the client is predicting a
	// weapon change due to a weapon pickup
	if (!serverside && cl_predictpickup)
	{
		if (!cmd->ucmd.impulse && !(cmd->ucmd.buttons & BT_CHANGE) &&
			consoleplayer().pendingweapon != wp_nochange)
			cmd->ucmd.impulse = 50 + static_cast<int>(consoleplayer().pendingweapon);
	}

	if (strafe || lookstrafe)
		side += (int)(((float)joyturn / (float)SHRT_MAX) * sidemove[speed]);
	else
		cmd->ucmd.yaw -= (short)((((float)joyturn / (float)SHRT_MAX) * angleturn[1]) * (joy_sensitivity / 10));

	if (Actions[ACTION_MLOOK])
	{
		if (joy_invert)
			look += (int)(((float)joyforward / (float)SHRT_MAX) * lookspeed[speed]);
		else
			look -= (int)(((float)joyforward / (float)SHRT_MAX) * lookspeed[speed]);
	}
	else
	{
		forward -= (int)(((float)joyforward / (float)SHRT_MAX) * forwardmove[speed]);
	}

	if ((Actions[ACTION_MLOOK]) || (cl_mouselook && sv_freelook))
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
		look = CENTERVIEW;
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
		cmd->ucmd.yaw -= (int)((float)(mousex*0x8) * m_yaw) / ticdup;

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


void G_ConvertMouseSettings(int old_type, int new_type)
{
	if (old_type == new_type)
		return;

	if (new_type == MOUSE_ODAMEX)
		new_type = MOUSE_DOOM;

	// first convert to ZDoom settings
	if (old_type == MOUSE_DOOM)
	{
		mouse_sensitivity.Set((mouse_sensitivity + 5.0f) / 40.0f);
		m_pitch.Set(m_pitch * 4.0f);
	}
	else if (old_type == MOUSE_ODAMEX)
	{
		mouse_sensitivity.Set(mouse_sensitivity / 40.0f);
		m_pitch.Set(m_pitch * 4.0f);
	}

	// convert to the destination type
	if (new_type == MOUSE_DOOM)
	{
		mouse_sensitivity.Set((mouse_sensitivity * 40.0f) - 5.0f);
		m_pitch.Set(m_pitch * 0.25f);
	}
	else if (new_type == MOUSE_ODAMEX)
	{
		mouse_sensitivity.Set(mouse_sensitivity * 40.0f);
		m_pitch.Set(m_pitch * 0.25f);
	}
}

int G_DoomMouseScaleX(int x)
{
	return int(x * (mouse_sensitivity + 5.0f) / 10.0f);
}

int G_DoomMouseScaleY(int y)
{
	return G_DoomMouseScaleX(y); // identical scaling for x and y
}

int G_ZDoomDIMouseScaleX(int x)
{
	return int(x * 4.0f * mouse_sensitivity);
}

int G_ZDoomDIMouseScaleY(int y)
{
	return int(y * mouse_sensitivity);
}

void G_ProcessMouseMovementEvent(const event_t *ev)
{
	static int prevx = 0, prevy = 0;
	int evx = ev->data2;
	int evy = ev->data3;

	if (m_filter)
	{
		// smooth out the mouse input
		evx = (evx + prevx) / 2;
		evy = (evy + prevy) / 2;
	}
	prevx = evx;
	prevy = evy;

	int (*scalexfunc)(int) = NULL;
	int (*scaleyfunc)(int) = NULL;

	if (mouse_type == MOUSE_DOOM)
	{
		scalexfunc = &G_DoomMouseScaleX;
		scaleyfunc = &G_DoomMouseScaleY;
	}
	else if (mouse_type == MOUSE_ZDOOM_DI)
	{
		scalexfunc = &G_ZDoomDIMouseScaleX;
		scaleyfunc = &G_ZDoomDIMouseScaleY;
	}
	else
		return;	// invalid mouse type

	if (dynres_state)
	{
		if (evx < 0)
			mousex = -int(pow((double)(*scalexfunc)(-evx), (double)dynresval));
		else
			mousex = int(pow((double)(*scalexfunc)(evx), (double)dynresval));

		if (evy < 0)
			mousey = -int(pow((double)(*scaleyfunc)(-evy), (double)dynresval));
		else
			mousey = int(pow((double)(*scaleyfunc)(evy), (double)dynresval));
	}
	else
	{
		mousex = (*scalexfunc)(evx);
		mousey = (*scaleyfunc)(evy);
	}
}

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
				stricmp (cmd, "screenshot") &&
                stricmp (cmd, "stepmode") &&
                stricmp (cmd, "step")))
			{
				S_Sound (CHAN_INTERFACE, "switches/normbutn", 1, ATTN_NONE);
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
		if (C_DoNetDemoKey(ev))	// netdemo playback ate the event
			return true;
		if (C_DoSpectatorKey(ev))
			return true;

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
		G_ProcessMouseMovementEvent(ev);

		if (hud_mousegraph)
			mousegraph.append(mousex, mousey);

		break;

	  case ev_joystick:
	  	if(ev->data1 == 0) // Axis Movement
		{
			if(ev->data2 == joy_strafeaxis) // Strafe
				joystrafe = ev->data3;
			else if(ev->data2 == joy_forwardaxis) // Move
				joyforward = ev->data3;
			else if(ev->data2 == joy_turnaxis) // Turn
				joyturn = ev->data3;
			else if(ev->data2 == joy_lookaxis) // Look
				joylook = ev->data3;
			else
				break; // The default case will be to treat the analog control as a button -- Hyper_Eye
		}

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

void P_MovePlayer (player_t *player);
void P_CalcHeight (player_t *player);
void P_DeathThink (player_t *player);
void CL_SimulateWorld();
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
			if (players[i].ingame() && (players[i].playerstate == PST_REBORN || players[i].playerstate == PST_ENTER))
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
		case ga_fullresetlevel:
			gameaction = ga_nothing;
			break;
		case ga_resetlevel:
			gameaction = ga_nothing;
			break;
		case ga_newgame:
			G_DoNewGame ();
			break;
		case ga_loadgame:
			G_DoLoadGame ();
			break;
		case ga_savegame:
			G_DoSaveGame ();
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
			I_ScreenShot(shotfile);
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

    // get commands
    buf = gametic%BACKUPTICS;
	memcpy (&consoleplayer().cmd, &consoleplayer().netcmds[buf], sizeof(ticcmd_t));

    static int realrate = 0;
    int packet_size;

	if (demoplayback)
		G_ReadDemoTiccmd(); // play all player commands
	if (demorecording)
		G_WriteDemoTiccmd(); // read in all player commands

	if(netdemo.isPlaying())
	{
		netdemo.readMessages(&net_message);
	}

	if (connected && !simulated_connection)
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

			if (netdemo.isRecording())
				netdemo.capture(&net_message);

			CL_ParseCommands();

			if (gameaction == ga_fullconsole) // Host_EndGame was called
				return;
		}

		if (!(gametic%TICRATE))
		{
			netin = realrate;
			realrate = 0;
		}

		CL_SaveCmd();      // save console commands
		if (!noservermsgs)
			CL_SendCmd();  // send console commands to the server

		if (!(gametic%TICRATE))
		{
			netout = outrate;
			outrate = 0;
		}

		if (gametic - last_received > 65)
			noservermsgs = true;
	}
	else if (NET_GetPacket() && !simulated_connection)
	{
		// denis - don't accept candy from strangers
		if((gamestate == GS_DOWNLOAD || gamestate == GS_CONNECTING)
			&& NET_CompareAdr(serveraddr, net_from))
		{
			if (netdemo.isRecording())
				netdemo.capture(&net_message);

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

	if (netdemo.isRecording())
		netdemo.writeMessages();

	// check for special buttons
	if(serverside && consoleplayer().ingame())
    {
		player_t &player = consoleplayer();

		if (player.cmd.ucmd.buttons & BT_SPECIAL)
		{
			switch (player.cmd.ucmd.buttons & BT_SPECIALMASK)
			{
			  case BTS_PAUSE:
				paused ^= 1;
				if (paused)
					S_PauseSound ();
				else
					S_ResumeSound ();
				break;

			  case BTS_SAVEGAME:
				if (!savedescription[0])
					strcpy (savedescription, "NET GAME");
				savegameslot =  (player.cmd.ucmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT;
				gameaction = ga_savegame;
				break;
			}
		}
    }

	// do main actions
	switch (gamestate)
	{
	case GS_LEVEL:
		if(clientside && !serverside)
		{
			if (!consoleplayer().mo)
			{
				// [SL] 2011-12-14 - Spawn message from server has not arrived
				// yet.  Fake it and hope it arrives soon.
				AActor *mobj = new AActor (0, 0, 0, MT_PLAYER);
				mobj->flags &= ~MF_SOLID;
				mobj->flags2 |= MF2_DONTDRAW;
				consoleplayer().mo = mobj->ptr();
				consoleplayer().mo->player = &consoleplayer();
				G_PlayerReborn(consoleplayer());
				DPrintf("Did not receive spawn for consoleplayer.\n");
			}

			CL_SimulateWorld();
			CL_PredictWorld();
		}
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
	z = mthing->z << FRACBITS;

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

		if (level.time)
			S_Sound (mo, CHAN_VOICE, "misc/teleport", 1, ATTN_NORM);	// don't start sound on first frame
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

void G_DeathMatchSpawnPlayer (player_t &player)
{
	int selections;
	mapthing2_t *spot;

	if(!serverside || sv_gametype == GM_COOP)
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
		player.mo->player = NULL;

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
	strcpy (savename, name);
	gameaction = ga_loadgame;
}


void G_DoLoadGame (void)
{
    unsigned int i;
	char text[16];
	size_t res;

	gameaction = ga_nothing;

	FILE *stdfile = fopen (savename, "rb");
	if (stdfile == NULL)
	{
		Printf (PRINT_HIGH, "Could not read savegame '%s'\n", savename);
		return;
	}

	fseek (stdfile, SAVESTRINGSIZE, SEEK_SET);	// skip the description field
	res = fread (text, 16, 1, stdfile);
	if (strncmp (text, SAVESIG, 16))
	{
		Printf (PRINT_HIGH, "Savegame '%s' is from a different version\n", savename);

		fclose(stdfile);

		return;
	}
	res = fread (text, 8, 1, stdfile);
	text[8] = 0;

	/*bglobal.RemoveAllBots (true);*/

	FLZOFile savefile (stdfile, FFile::EReading);

	if (!savefile.IsOpen ())
		I_Error ("Savegame '%s' is corrupt\n", savename);

	Printf (PRINT_HIGH, "Loading savegame '%s'...\n", savename);

	FArchive arc (savefile);

	{
		byte vars[4096], *vars_p;
		unsigned int len;
		vars_p = vars;
		len = arc.ReadCount ();
		arc.Read (vars, len);
		cvar_t::C_ReadCVars (&vars_p);
	}

	// dearchive all the modifications
	G_SerializeSnapshots (arc);
	P_SerializeRNGState (arc);
	P_SerializeACSDefereds (arc);

	CL_QuitNetGame();

	netgame = false;
	multiplayer = false;

	// load a base level
	savegamerestore = true;		// Use the player actors in the savegame
	serverside = true;
	G_InitNew (text);
	displayplayer_id = consoleplayer_id = 1;
	savegamerestore = false;

	arc >> level.time;


	for (i = 0; i < NUM_WORLDVARS; i++)
		arc >> ACS_WorldVars[i];

	for (i = 0; i < NUM_GLOBALVARS; i++)
		arc >> ACS_GlobalVars[i];

	arc >> text[9];

	arc.Close ();

	if (text[9] != 0x1d)
		I_Error ("Bad savegame");
}


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string
//
void G_SaveGame (int slot, char *description)
{
	savegameslot = slot;
	strcpy (savedescription, description);
	sendsave = true;
}

void G_BuildSaveName (std::string &name, int slot)
{
    std::stringstream ssName;

#ifdef _XBOX
	std::string path = xbox_GetSavePath(name, slot);
#else
	std::string path = I_GetUserFileName ((const char *)name.c_str());
#endif

	ssName << path;
    ssName << "odasv";
	ssName << slot;
	ssName << ".ods";

    name = ssName.str();
}

void G_DoSaveGame (void)
{
	std::string name;
	char *description;
	size_t res;
	int i;

	G_SnapshotLevel ();

	G_BuildSaveName (name, savegameslot);
	description = savedescription;

	FILE *stdfile = fopen (name.c_str(), "wb");

	if (stdfile == NULL)
	{
        return;
	}

#ifdef _XBOX
	xbox_WriteSaveMeta(name.substr(0, name.rfind(PATHSEPCHAR)), description);
#endif

	Printf (PRINT_HIGH, "Saving game to '%s'...\n", name.c_str());

	res = fwrite (description, SAVESTRINGSIZE, 1, stdfile);
	res = fwrite (SAVESIG, 16, 1, stdfile);
	res = fwrite (level.mapname, 8, 1, stdfile);

	FLZOFile savefile (stdfile, FFile::EWriting, true);
	FArchive arc (savefile);

	{
		byte vars[4096], *vars_p;
		vars_p = vars;

		cvar_t::C_WriteCVars (&vars_p, CVAR_SERVERINFO);
		arc.WriteCount (vars_p - vars);
		arc.Write (vars, vars_p - vars);
	}

	G_SerializeSnapshots (arc);
	P_SerializeRNGState (arc);
	P_SerializeACSDefereds (arc);

	arc << level.time;

	for (i = 0; i < NUM_WORLDVARS; i++)
		arc << ACS_WorldVars[i];

	for (i = 0; i < NUM_GLOBALVARS; i++)
		arc << ACS_GlobalVars[i];


	arc << (BYTE)0x1d;			// consistancy marker

	gameaction = ga_nothing;
	savedescription[0] = 0;

	Printf (PRINT_HIGH, "%s\n", GStrings(GGSAVED));
	arc.Close ();

    if (level.info->snapshot != NULL)
    {
        delete level.info->snapshot;
        level.info->snapshot = NULL;
    }
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
		int demostep = (demoversion == LMP_DOOM_1_9_1) ? 5 : 4;

		for(size_t i = 0; i < players.size(); i++)
		{
			if ((demo_e - demo_p < demostep) || (*demo_p == DEMOMARKER))
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
bool G_RecordDemo (const char* name)
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

//
// G_PlayDemo
//

std::string defdemoname;

void G_DeferedPlayDemo (const char *name)
{
	defdemoname = name;
	gameaction = ga_playdemo;
}

void RecordCommand(int argc, char **argv)
{
	if(argc > 2)
	{
		demorecordfile = std::string(argv[2]);

		if (gamestate != GS_STARTUP)
		{
			if(G_RecordDemo(demorecordfile.c_str()))
			{
				players.clear();
				players.push_back(player_t());
				players.back().playerstate = PST_REBORN;
				players.back().id = 1;

				player_t &con = idplayer(1);
				consoleplayer_id = displayplayer_id = con.id;

				serverside = true;

				G_InitNew(argv[1]);
				G_BeginRecording();
			}
		}
		else
		{
			strncpy (startmap, argv[1], 8);
			autostart = true;
			autorecord = true;
		}
	}
	else
		Printf(PRINT_HIGH, "Usage: recordvanilla map file\n");
}

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

BEGIN_COMMAND(playdemo)
{
	if(argc > 1)
	{
		extern bool lastWadRebootSuccess;
		if(lastWadRebootSuccess)
		{
			G_DeferedPlayDemo(argv[1]);
		}
		else
		{
			Printf(PRINT_HIGH, "Cannot play demo because WAD didn't load\n");
			Printf(PRINT_HIGH, "Use the 'wad' command\n");
		}
	}
	else
		Printf(PRINT_HIGH, "Usage: playdemo lumpname or file\n");
}
END_COMMAND(playdemo)

BEGIN_COMMAND(streamdemo)
{
	if(argc > 1)
	{
		G_DeferedPlayDemo(argv[1]);
		G_DoPlayDemo(true);
	}
	else
		Printf(PRINT_HIGH, "Usage: playdemo lumpname or file\n");
}
END_COMMAND(streamdemo)

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
		multiplayer = netgame = true;

	return false;
}

void G_DoPlayDemo (bool justStreamInput)
{
	char mapname[9];

	if(!justStreamInput)
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
		M_AppendExtension (defdemoname, ".lmp");
		bytelen = M_ReadFile (defdemoname, &demobuffer);
		demo_p = demobuffer;
	}

	demo_e = demo_p + bytelen;

	if(bytelen < 4)
	{
		if(bytelen)
		{
			Z_Free(demobuffer);
			Printf (PRINT_HIGH, "Demo file too short\n");
		}
		gameaction = ga_fullconsole;
		return;
	}

	Printf (PRINT_HIGH, "Playing demo %s\n", defdemoname.c_str());

	// [SL] 2012-12-26 - Backup any cvars that need to be set to default to
	// ensure demo compatibility. CVAR_SERVERINFO cvars is a handy superset
	// of those cvars
	cvar_t::C_BackupCVars(CVAR_SERVERINFO);
	cvar_t::C_SetCVarsToDefaults(CVAR_SERVERINFO);
	MakeEmptyUserCmd ();
	demostartgametic = gametic;

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
			gameaction = ga_fullconsole;
			return;
		}

		democlassic = true;

		demoversion = *demo_p++ == DOOM_1_9_1_DEMO ? LMP_DOOM_1_9_1 : LMP_DOOM_1_9;
		float s = (float)((*demo_p++)+1);
		sv_skill = s;
		byte episode = *demo_p++;
		byte map = *demo_p++;
		int deathmatch = *demo_p++;
		if (deathmatch == 2)
		{
			// Altdeath
			sv_gametype.Set(GM_DM);
			sv_weaponstay.Set(0.0f);
			sv_itemsrespawn.Set(1.0f);
		}
		else if (deathmatch == 1)
		{
			// Classic deathmatch
			sv_gametype.Set(GM_DM);
			sv_weaponstay.Set(1.0f);
			sv_itemsrespawn.Set(0.0f);
		}
		else
		{
			// Co-op
			sv_gametype.Set(GM_COOP);
			sv_weaponstay.Set(1.0f);
			sv_itemsrespawn.Set(0.0f);
		}

		sv_monstersrespawn = *demo_p++;
		sv_fastmonsters = *demo_p++;
		sv_nomonsters = *demo_p++;
		byte who = *demo_p++;

		if(!justStreamInput)
			players.clear();

		for (size_t i=0 ; i < MAXPLAYERS_VANILLA; i++)
		{
			if(*demo_p++ && !justStreamInput)
			{
				players.push_back(player_t());
				players.back().playerstate = PST_REBORN;
				players.back().id = i + 1;
			}
		}

		if(!justStreamInput)
		{
    		player_t &con = idplayer(who + 1);

    		if(!validplayer(con))
    		{
    			Z_Free(demobuffer);
    			Printf (PRINT_HIGH, "DOOM 1.9 Demo: invalid console player %d of %d\n", (int)who+1, players.size());
    			gameaction = ga_fullconsole;
    			return;
    		}

    		consoleplayer_id = displayplayer_id = con.id;

    		//int pcol[4] = {(0x0000FF00), (0x006060B0), (0x00B0B030), (0x00C00000)};
    		//char pnam[4][MAXPLAYERNAME] = {"GREEN", "INDIGO", "BROWN", "RED"};

    		if(players.size() > 1)
    		{
    			netgame = true;
    			multiplayer = true;

    			for (size_t i = 0; i < players.size(); i++) {
    				if (players[i].ingame()) {
    					//strcpy(players[i].userinfo.netname, pnam[i]);
    					//players[i].userinfo.team = TEAM_NONE;
    					//players[i].userinfo.gender = GENDER_NEUTER;
    					//players[i].userinfo.color = pcol[i];
    					//players[i].userinfo.skin = 0;
    					//players[i].GameTime = 0;
    					//R_BuildPlayerTranslation (players[i].id, players[i].userinfo.color);
    					R_BuildClassicPlayerTranslation (players[i].id, i);
    				}
    			}
    		}
    		else
    		{
    			netgame = false;
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
		}
		demoplayback = true;

		return;
	} else {
		democlassic = false;
	}

	if(demo_p[0] >= DOOM_BOOM_DEMO_START ||
	   demo_p[0] <= DOOM_BOOM_DEMO_END)
	{
		// [SL] 2011-08-03 - Version 1 of Odamex netdemos get detected by this
		// code as a Boom format demo.  Since neither can be played using the
		// -playdemo paramter, inform the user.  This could probably be handled
		// more robustly.
		Printf (PRINT_HIGH, "Unsupported demo format.  If you are trying to play an Odamex netdemo, please use the netplay command\n");
        gameaction = ga_nothing;
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
		extern bool demotest;

		if (timingdemo)
			endtime = I_GetTimePolled () - starttime;

		Z_Free (demobuffer);

		demoplayback = false;
		netgame = false;
		multiplayer = false;
		serverside = false;

		cvar_t::C_RestoreCVars ();		// [RH] Restore cvars demo might have changed

		if (demotest) {
			AActor *mo = idplayer(1).mo;

			if (mo)
				Printf(PRINT_HIGH, "demotest:%x %x %x %x\n", mo->angle, mo->x, mo->y, mo->z);
			else
				Printf(PRINT_HIGH, "demotest:no player\n");
		}


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
		if(recorddemo_fp)
		{
			fputc(DEM_STOP, recorddemo_fp);
			fclose(recorddemo_fp);
			recorddemo_fp = NULL;
		}

		demorecording = false;
		Printf (PRINT_HIGH, "Demo %s recorded\n", demoname);
	}

	return false;
}


VERSION_CONTROL (g_game_cpp, "$Id$")
