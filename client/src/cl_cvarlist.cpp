	// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
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
//	Client console variables
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"
#include "s_sound.h"
#include "i_music.h"
#include "i_input.h"
#include "d_netinf.h"

// Automap
// -------

CVAR(					am_rotate, "0", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(				am_overlay, "0", "",
						CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 3.0f)

CVAR(					am_showsecrets, "1", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(					am_showmonsters, "1", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(					am_showtime, "1", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(					am_classicmapstring, "0", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(					am_usecustomcolors, "0", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(					am_ovshare, "0", "",
						CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(					am_backcolor, "00 00 3a", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_yourcolor, "d8 e8 fc", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_wallcolor, "00 8b ff", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_tswallcolor, "10 32 7e", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_fdwallcolor, "1a 1a 8a", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_cdwallcolor, "00 00 5a", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_thingcolor, "9f d3 ff", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_gridcolor, "44 44 88", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_xhaircolor, "80 80 80", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_notseencolor, "00 22 6e", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_lockedcolor, "bb bb bb", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_exitcolor, "ff ff 00", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_teleportcolor, "ff a3 00", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovyourcolor, "d8 e8 fc", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovwallcolor, "00 8b ff", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovtswallcolor, "10 32 7e", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovfdwallcolor, "1a 1a 8a", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovcdwallcolor, "00 00 5a", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovthingcolor, "9f d3 ff", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovgridcolor, "44 44 88", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovxhaircolor, "80 80 80", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovnotseencolor, "00 22 6e", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovlockedcolor, "bb bb bb", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovexitcolor, "ff ff 00", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(					am_ovteleportcolor, "ff a3 00", "",
						CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)


// Console
// -------

CVAR(				print_stdout, "0", "Print console text to stdout",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(			con_notifytime, "3", "Number of seconds to display messages to top of the HUD",
					CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 10.0f)

CVAR_RANGE(			con_midtime, "3", "Number of seconds to display messages in the middle of the screen",
					CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 10.0f)

CVAR_RANGE(			con_scrlock, "1", "",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR_RANGE(			con_buffersize, "1024", "Size of console scroll-back buffer",
					CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 512.0f, 65536.0f)

CVAR_RANGE_FUNC_DECL(msg0color, "6", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 22.0f)

CVAR_RANGE_FUNC_DECL(msg1color, "5", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 22.0f)

CVAR_RANGE_FUNC_DECL(msg2color, "2", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 22.0f)

CVAR_RANGE_FUNC_DECL(msg3color, "3", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 22.0f)

CVAR_RANGE_FUNC_DECL(msg4color, "8", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 22.0f)

CVAR_RANGE_FUNC_DECL(msgmidcolor, "5", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 22.0f)

// Intermission
// ------------

// Determines whether to draw the scores on intermission.
CVAR(				wi_newintermission, "0", "Draw the scores on intermission",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)


// Menus
// -----

CVAR_RANGE(			ui_dimamount, "0.7", "",
					CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(				ui_dimcolor, "00 00 00", "",
					CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(ui_transred, "0", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR_RANGE_FUNC_DECL(ui_transgreen, "0", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR_RANGE_FUNC_DECL(ui_transblue, "0", "",
					CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)


// Gameplay/Other
// --------------

CVAR(				cl_connectalert, "1", "Plays a sound when a player joins",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				cl_disconnectalert, "1", "Plays a sound when a player quits",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(			cl_switchweapon, "1", "Switch upon weapon pickup (0 = never, 1 = always, " \
					"2 = use weapon preferences)",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR_RANGE(			cl_weaponpref_fst, "0", "Weapon preference level for fists",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_csw, "3", "Weapon preference level for chainsaw",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_pis, "4", "Weapon preference level for pistol",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_sg, "5", "Weapon preference level for shotgun",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_ssg, "7", "Weapon preference level for super shotgun",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_cg, "6", "Weapon preference level for chaingun",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_rl, "1", "Weapon preference level for rocket launcher",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_pls, "8", "Weapon preference level for plasma rifle",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

CVAR_RANGE(			cl_weaponpref_bfg, "2", "Weapon preference level for BFG9000",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 8.0f)

#ifdef GCONSOLE
CVAR_FUNC_DECL(		use_joystick, "1", "",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
#else
CVAR_FUNC_DECL(		use_joystick, "0", "",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
#endif

CVAR_FUNC_DECL(		joy_active, "0", "Selects the joystick device to use",
					CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR (joy_strafeaxis, "0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_forwardaxis, "1", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_turnaxis, "2", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_lookaxis, "3", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_sensitivity, "10.0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_freelook, "0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE)
CVAR (joy_invert, "0", "", CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE)


CVAR(				show_messages, "1", "",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				mute_spectators, "0", "Mute spectators chat until next disconnect",
					CVARTYPE_BOOL, CVAR_NULL)

CVAR(				mute_enemies, "0", "Mute enemy players chat until next disconnect",
					CVARTYPE_BOOL, CVAR_NULL)


// Maximum number of clients who can connect to the server
CVAR (sv_maxclients,       "0", "maximum clients who can connect to server", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum amount of players who can join the game, others are spectators
CVAR (sv_maxplayers,		"0", "maximum players who can join the game, others are spectators", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Maximum number of players that can be on a team
CVAR (sv_maxplayersperteam, "0", "Maximum number of players that can be on a team", CVARTYPE_BYTE, CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)


// Netcode Settings
// --------------

CVAR_RANGE_FUNC_DECL(rate, "200", "Rate of client updates in multiplayer mode",
					CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 7.0f, 2000.0f)

CVAR(				cl_unlag, "1", "client opt-in/out for server unlagging",
					CVARTYPE_BOOL, CVAR_USERINFO | CVAR_CLIENTARCHIVE)

CVAR_RANGE(			cl_updaterate, "1",	"Update players every N tics",
					CVARTYPE_BYTE, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 3.0f)

CVAR_RANGE_FUNC_DECL(cl_interp, "1", "Interpolate enemy player positions",
					CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 4.0f)

CVAR_RANGE(			cl_prednudge,	"0.70", "Smooth out collisions",
					CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.05f, 1.0f)

CVAR(				cl_predictlocalplayer, "1", "Predict local player position",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				cl_predictweapons, "1", "Draw weapon effects immediately",
					CVARTYPE_BOOL, CVAR_USERINFO | CVAR_CLIENTARCHIVE)

CVAR(				cl_netgraph, "0", "Show a graph of network related statistics",
					CVARTYPE_BOOL, CVAR_NULL)

CVAR(				cl_forcedownload, "0", "Forces the client to download the last WAD file when connecting " \
											"to a server, even if the client already has that file " \
											"(requires developer 1).",
					CVARTYPE_BOOL, CVAR_NULL)

// Client Preferences
// ------------------

#ifdef _XBOX // Because Xbox players may be unable to communicate for now -- Hyper_Eye
CVAR_FUNC_DECL(		cl_name, "Xbox Player", "",
					CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
#else
CVAR_FUNC_DECL(		cl_name, "Player", "",
					CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
#endif

CVAR(				cl_color, "40 cf 00", "",
					CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(				cl_gender, "male", "",
					CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(		cl_team, "blue", "",
					CVARTYPE_STRING, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(			cl_autoaim,	"5000", "",
					CVARTYPE_INT, CVAR_USERINFO | CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 5000.0f)

CVAR(				chasedemo, "0", "",
					CVARTYPE_BOOL, CVAR_NULL)

CVAR(				cl_run, "0", "Always run",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)		// Always run? // [Toke - Defaults]

CVAR(				cl_showspawns, "0", "Show spawn points as particle fountains",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE | CVAR_LATCH)

// Netdemo Preferences
// --------------------

// Netdemo format string
CVAR_FUNC_DECL(		cl_netdemoname, "Odamex_%g_%d_%t_%w_%m",
					"Default netdemo name.  Parses the following tokens:\n// " \
					"%d: date in YYYYMMDD format\n// %t: time in HHMMSS format\n// " \
					"%n: player name\n// %g: gametype\n// %w: WAD file loaded; " \
					"either the first PWAD or the IWAD\n// %m: Map lump\n// %%: Literal percent sign",
					CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

// Screenshot format string
CVAR_FUNC_DECL(		cl_screenshotname, "Odamex_%g_%d_%t",
					"Default screenshot name.  Parses the following tokens:\n// " \
					"%d: date in YYYYMMDD format\n// %t: time in HHMMSS format\n// " \
					"%n: player name\n// %g: gametype\n// %w: WAD file loaded; " \
					"either the first PWAD or the IWAD\n// %m: Map lump\n// %%: Literal percent sign",
					CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(				cl_autorecord, "0", "Automatically record netdemos",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				cl_autoscreenshot, "0", "Automatically capture a screenshot at the end of a match.",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				cl_splitnetdemos, "0", "Create separate netdemos for each map",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

// Mouse settings
// --------------

//
// C_GetDefaultMouseDriver()
//
// Allows the default value for music_driver to change depending on
// compile-time factors (eg, OS)
//
static char *C_GetDefaultMouseDriver()
{
	static char str[4];

	int driver_id = SDL_MOUSE_DRIVER;

	#ifdef _WIN32
	driver_id = RAW_WIN32_MOUSE_DRIVER;
	#endif

	sprintf(str, "%i", driver_id);
	return str;
}

CVAR_FUNC_DECL(	mouse_driver, C_GetDefaultMouseDriver(), "Mouse driver backend",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(		mouse_type, "0", "Use vanilla Doom mouse sensitivity or ZDoom mouse sensitivity",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE(		mouse_sensitivity, "35.0", "Overall mouse sensitivity",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 500.0f)

CVAR_FUNC_DECL(	cl_mouselook, "0", "Look up or down with mouse",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		m_pitch, "0.25", "Vertical mouse sensitivity",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 500.0f)

CVAR_RANGE(		m_yaw, "1.0", "",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR_RANGE(		m_forward, "1.0", "",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR_RANGE(		m_side, "2.0", "",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR(			novert, "0", "Disable vertical mouse movement",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			invertmouse, "0", "Invert vertical mouse movement",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			lookstrafe, "0", "Strafe with mouse",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		mouse_acceleration, "0", "",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 500.0f)

CVAR_RANGE(		mouse_threshold, "0", "",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 500.0f)

CVAR(			m_filter, "0", "Smooth mouse input",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			dynres_state, "0", "",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		dynresval, "1.0", "",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 100.0f)

CVAR(			hud_mousegraph, "0", "Display mouse values",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			idmypos, "0", "Shows current player position on map",
				CVARTYPE_BOOL, CVAR_NULL)

// Heads up display
// ----------------
CVAR(			hud_crosshairdim, "0", "Crosshair transparency",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			hud_crosshairscale, "0", "Crosshair scaling",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(	hud_crosshaircolor, "ff 00 00", "Crosshair color",
                CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(			hud_crosshairhealth, "0", "Color of crosshair represents health level",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		hud_fullhudtype, "1","Fullscreen HUD to display:\n// 0: ZDoom HUD\n// 1: New Odamex HUD",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE(		hud_gamemsgtype, "2", "Game message type",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 2.0f)

CVAR(			hud_revealsecrets, "0", "Print HUD message when secrets are discovered",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			hud_scale, "0", "HUD scaling",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			hud_scalescoreboard, "0", "Scoreboard scaling",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE(		hud_scaletext, "2", "Scaling multiplier for chat and midprint",
                CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 4.0f)

CVAR_RANGE(		hud_targetcount, "2", "Number of players to reveal",
                CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 32.0f)

CVAR(			hud_targetnames, "1", "Show names of players you're aiming at",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			hud_timer, "1", "Show the HUD timer",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		hud_transparency, "0.5", "HUD transparency",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(			hud_heldflag, "1", "Show the held flag border",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

#ifdef _XBOX
CVAR (chatmacro0, "Hi.", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)                       // A
CVAR (chatmacro1, "I'm ready to kick butt!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)   // B
CVAR (chatmacro2, "Help!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)                     // X
CVAR (chatmacro3, "GG", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)                        // Y
CVAR (chatmacro4, "No", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)                       // Black
CVAR (chatmacro5, "Yes", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)                        // White
CVAR (chatmacro6, "I'll take care of it.", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)     // Left Trigger
CVAR (chatmacro7, "Come here!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)                // Right Trigger
CVAR (chatmacro8, "Thanks for the game. Bye.", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE) // Start
CVAR (chatmacro9, "I am on Xbox and can only use chat macros.", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE) // Back
#else
// GhostlyDeath <November 2, 2008> -- someone had the order wrong (0-9!)
CVAR (chatmacro1, "I'm ready to kick butt!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro2, "I'm OK.", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro3, "I'm not looking too good!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro4, "Help!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro5, "You suck!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro6, "Next time, scumbag...", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro7, "Come here!", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro8, "I'll take care of it.", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro9, "Yes", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro0, "No", "",	CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
#endif

// Sound and music
// ---------------

CVAR_RANGE_FUNC_DECL(snd_sfxvolume, "0.5", "Sound effect volume",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE_FUNC_DECL(snd_musicvolume, "0.5", "Music volume",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE(		snd_announcervolume, "1.0", "Announcer volume",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE(		snd_voxtype, "2", "Voice announcer type",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 1.0f, 2.0f)

CVAR(			snd_gamesfx, "1", "Game SFX", CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			snd_crossover, "0", "Stereo switch",	CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		snd_samplerate, "22050", "Audio samplerate",
				CVARTYPE_INT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 11025.0f, 192000.0f)

CVAR_RANGE_FUNC_DECL(snd_channels, "12", "Number of channels for sound effects",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 4.0f, 32.0f)

//
// C_GetDefaultMuiscSystem()
//
// Allows the default value for snd_musicsystem to change depending on
// compile-time factors (eg, OS)
//
static char *C_GetDefaultMusicSystem()
{
	static char str[4];

	MusicSystemType defaultmusicsystem = MS_SDLMIXER;
	#ifdef OSX
	defaultmusicsystem = MS_AUDIOUNIT;
	#endif

	#if defined _WIN32 && !defined _XBOX
	defaultmusicsystem = MS_PORTMIDI;
	#endif

	// don't overflow str
	if (int(defaultmusicsystem) > 999 || int(defaultmusicsystem) < 0)
		defaultmusicsystem = MS_NONE;

	sprintf(str, "%i", defaultmusicsystem);
	return str;
}

CVAR_FUNC_DECL(	snd_musicsystem, C_GetDefaultMusicSystem(), "Music subsystem preference",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR(			snd_musicdevice, "", "Music output device for the chosen music subsystem",
				CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)


// Status bar
// ----------

CVAR_FUNC_DECL (st_scale, "1", "",	CVARTYPE_BYTE, CVAR_CLIENTARCHIVE)

// Video and Renderer
// ------------------

CVAR_FUNC_DECL(	gammalevel, "1", "Gamma correction level",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(vid_gammatype, "0", "Select between Doom and ZDoom gamma correction",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR_RANGE_FUNC_DECL(hud_crosshair, "0", "Type of crosshair, 0 means no crosshair",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR(			r_flashhom, "0", "Draws flashing colors where there is HOM",
				CVARTYPE_BOOL, CVAR_NULL)

CVAR(			r_drawflat, "0", "Disables all texturing of walls, floors and ceilings",
				CVARTYPE_BOOL, CVAR_NULL)

#if 0
CVAR(			r_drawhitboxes, "0", "Draws a box outlining every actor's hitboxes",
				CVARTYPE_BOOL, CVAR_NULL)
#endif

CVAR_RANGE(		r_drawplayersprites, "1", "Weapon Transparency",
				CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(			r_particles, "1", "Draw particles",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE_FUNC_DECL(r_stretchsky, "2", "Stretch sky textures. (0 - always off, 1 - always on, 2 - auto)",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 2.0f)

CVAR(			r_skypalette, "0", "Invulnerability sphere changes the palette of the sky",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(	r_forceenemycolor, "0", "Changes the color of all enemies to the color specified by r_enemycolor",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(	r_enemycolor, "40 cf 00", "",
				CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	r_forceteamcolor, "0", "Changes the color of all teammates to the color specified by r_teamcolor",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(	r_teamcolor, "40 cf 00", "",
				CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

#ifdef _XBOX // The burn wipe works better in 720p
CVAR_RANGE(		r_wipetype, "2", "",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 3.0f)
#else
CVAR_RANGE(		r_wipetype, "1", "",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 3.0f)
#endif

CVAR(			r_showendoom, "0", "Display the ENDDOOM text after quitting",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)   // [ML] 1/5/10: Add endoom support

CVAR(			r_loadicon, "1", "Display the disk icon when loading data from disk",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(		r_painintensity, "1", "Intensity of red pain effect",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(			r_viewsize, "0", "Set to the current video resolution",
				CVARTYPE_STRING, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	vid_defwidth, "640", "",
				CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	vid_defheight, "480", "",
				CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	vid_widescreen, "0", "Use wide field-of-view with widescreen video modes",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			vid_autoadjust, "1", "Force fullscreen resolution to the closest availible video mode.",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(			vid_displayfps, "0", "Display frames per second",
				CVARTYPE_BOOL, CVAR_NULL)

CVAR(			vid_ticker, "0", "Vanilla Doom frames per second indicator",
				CVARTYPE_BOOL, CVAR_NULL)

CVAR_FUNC_DECL(	vid_maxfps, "35", "Maximum framerate (0 indicates unlimited framerate)",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL(	vid_vsync, "0", "Enable/Disable vertical refresh sync (vsync)",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

#ifdef GCONSOLE
CVAR_FUNC_DECL(	vid_fullscreen, "1", "Full screen video mode",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
#else
CVAR_FUNC_DECL(	vid_fullscreen, "0", "Full screen video mode",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)
#endif

CVAR_FUNC_DECL(	vid_32bpp, "0", "Enable 32-bit color rendering",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(	vid_320x200, "0", "Enable 320x200 video emulation",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_FUNC_DECL(	vid_640x400, "0", "Enable 640x400 video emulation",
				CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

// Optimize rendering functions based on CPU vectorization support
// Can be of "detect" or "none" or "mmx","sse2","altivec" depending on availability; case-insensitive.
CVAR_FUNC_DECL(	r_optimize, "detect", "Rendering optimizations",
				CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_RANGE_FUNC_DECL(screenblocks, "10", "Selects the size of the visible window",
				CVARTYPE_BYTE, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 3.0f, 12.0f)

CVAR_RANGE_FUNC_DECL(vid_overscan, "1.0", "Overscan matting (as a percentage of the screen area)",
				CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.5f, 1.0f)


VERSION_CONTROL (cl_cvarlist_cpp, "$Id$")

