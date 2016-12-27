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
// Common console variables, used by both client and server.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"

// Server settings
// ---------------

// Game mode
CVAR_RANGE(			sv_gametype, "0", "Sets the game mode, values are:\n" \
					"// 0 = Cooperative\n" \
					"// 1 = Deathmatch\n" \
					"// 2 = Team Deathmatch\n" \
					"// 3 = Capture The Flag",
					CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 3.0f)

CVAR(				sv_friendlyfire, "1", "When set, players can injure others on the same team, " \
					"it is ignored in deathmatch",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_scorelimit, "5", "Game ends when team score is reached in Teamplay/CTF",
					CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 255.0f)

CVAR(				sv_teamspawns, "1", "When disabled, treat team spawns like normal deathmatch " \
					"spawns in Teamplay/CTF",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

CVAR(				sv_allowcheats, "0", "Allow usage of cheats in all game modes",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowexit, "1", "Allow use of Exit switch/teleports in all game modes",
					CVARTYPE_BOOL,  CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowjump, "0", "Allows players to jump when set in all game modes",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_doubleammo, "0", "Give double ammo regardless of difficulty",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_weapondamage, "1.0", "Amount to multiply player weapon damage by",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 100.0f)

CVAR_FUNC_DECL(		sv_forcewater, "0", "Makes water more realistic (boom maps at the moment)",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR(				sv_freelook, "0", "Allow Looking up and down",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowtargetnames, "0", "When set, names of players appear in the FOV",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_fraglimit,     "0", "Sets the amount of frags a player can accumulate before the game ends",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 65536.0f)

CVAR(				sv_fastmonsters, "0", "Monsters are at nightmare speed",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_monsterdamage, "1.0", "Amount to multiply monster weapon damage by",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 100.0f)

// OLD: Allow exit switch at maxfrags, must click to exit
// NEW: When enabled, exit switch will kill the player who flips it
// [ML] NOTE: Behavior was changed October 2012, see bug
CVAR(				sv_fragexitswitch, "0", "When enabled, exit switch will kill the player who flips it",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_infiniteammo, "0", "Infinite ammo for all players",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_itemsrespawn, "0", "Items will respawn after a fixed period, see sv_itemrespawntime",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR_RANGE(			sv_itemrespawntime, "30", "If sv_itemsrespawn is set, items will respawn after this " \
					"time (in seconds)",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 500.0f)

CVAR(				sv_spawnmpthings, "1", "Spawn multiplayer things on the map",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(				sv_monstersrespawn,  "0", "Monsters will respawn after a period of time",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_nomonsters, "0", "No monsters will be present",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)


CVAR_RANGE(			sv_monstershealth, "1.0", "Amount to multiply monster health by",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE,
					0.0f, 100.0f)

CVAR_RANGE(			sv_skill,"3", "Sets the skill level, values are:\n" \
					"// 0 - No things mode\n" \
					"// 1 - I'm Too Young To Die\n" \
					"// 2 - Hey, Not Too Rough\n" \
					"// 3 - Hurt Me Plenty\n" \
					"// 4 - Ultra-Violence\n" \
					"// 5 - Nightmare",
					CVARTYPE_BYTE, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE, 0.0f, 5.0f)


CVAR_RANGE_FUNC_DECL(sv_timelimit, "0", "Sets the time limit for the game to end (in minutes)",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 32768.0f)

CVAR_RANGE_FUNC_DECL(sv_intermissionlimit, "10", "Sets the time limit for the intermission to end (in seconds)",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 32768.0f)

CVAR(				sv_weaponstay,    "1", "Weapons stay after pickup",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(				sv_keepkeys, "0", "Keep keys on death",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR(				sv_unlag, "1", "Allow reconciliation for players on lagged connections",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

CVAR_RANGE(			sv_maxunlagtime, "1.0", "Cap the maxiumum time allowed for player reconciliation (in seconds)",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(				sv_allowmovebob, "0", "Allow weapon & view bob changing",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR(				sv_allowredscreen, "0","Allow clients to adjust amount of red pain screen intensity",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

CVAR(				sv_allowpwo, "0", "Allow clients to set their preferences for automatic weapon swithching",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

CVAR_FUNC_DECL(		sv_allowwidescreen, "1", "Allow clients to use true widescreen",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR(				sv_allowshowspawns, "1", "Allow clients to see spawn points as particle fountains",
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_SERVERARCHIVE | CVAR_LATCH)

CVAR(				sv_forcerespawn, "0", "Force a player to respawn",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

CVAR_RANGE(			sv_forcerespawntime, "30", "Force a player to respawn after a set amount of time (in seconds)",
					CVARTYPE_WORD, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 32768.0f)

CVAR_RANGE(			sv_spawndelaytime, "0.0", "Force a player to wait a period (in seconds) before respawning",
					CVARTYPE_FLOAT, CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE, 0.0f, 32768.0f)

CVAR(				sv_unblockplayers, "0", "Allows players to walk through other players",
					CVARTYPE_BOOL, CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)

CVAR(				sv_hostname, "Untitled Odamex Server", "Server name to appear on masters, clients and launchers",
					CVARTYPE_STRING, CVAR_SERVERARCHIVE | CVAR_NOENABLEDISABLE | CVAR_SERVERINFO)

					
CVAR(				sv_coopspawnvoodoodolls, "1", "Spawn voodoo dolls in cooperative mode", 
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_LATCH)
					
CVAR(				sv_coopunassignedvoodoodolls, "1", "", 
					CVARTYPE_BOOL, CVAR_SERVERINFO | CVAR_LATCH)
					
CVAR(				sv_coopunassignedvoodoodollsfornplayers, "255", "", 
					CVARTYPE_WORD, CVAR_SERVERINFO | CVAR_LATCH)

// Compatibility options
// ---------------------------------

	// Fixes to Vanilla
	//------------------------------
	CVAR(			co_realactorheight, "0", "Enable/Disable infinitely tall actors",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

	CVAR(			co_nosilentspawns, "0", "Turns off the west-facing silent spawns vanilla bug",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_fixweaponimpacts, "0", "Corrected behavior for impact of projectiles and bullets on surfaces",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_blockmapfix, "0", "Fix the blockmap collision bug",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)


	// Boom-compatibility changes
	//------------------------------
	CVAR(			co_boomphys, "0", "Use a finer-grained, faster, and more accurate test for actors, " \
					"sectors and lines",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_allowdropoff, "0", "Allow monsters can get pushed or thrusted off of ledges",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)



	// ZDoom-compatibility changes
	//------------------------------
	CVAR(			co_zdoomphys, "0", "Enable/disable ZDoom-based gravity and physics interactions",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_zdoomsound, "0", "Enable sound attenuation curve + attenuation of switch sounds with distance",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_fineautoaim, "0", "Increase precision of vertical auto-aim",
					CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)

	CVAR(			co_globalsound, "0", "Make pickup sounds global", CVARTYPE_BOOL, CVAR_ARCHIVE | CVAR_SERVERINFO)


// Client options
// ---------------------

CVAR(				cl_deathcam, "1", "Dead player's view follows the actor who killed them",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				cl_predictsectors, "1", "Move floors and ceilings immediately instead of waiting for confirmation",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				cl_predictpickup, "1", "Predict weapon pickups",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR_RANGE(			cl_movebob, "1.0", "Adjust weapon and movement bobbing",
					CVARTYPE_FLOAT, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, 0.0f, 1.0f)

CVAR(				cl_rockettrails, "0", "Rocket trails on/off (currently unused)",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)


CVAR_RANGE_FUNC_DECL(sv_gravity, "800", "Gravity of the environment",
					CVARTYPE_WORD, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
					0.0f, 32768.0f)

CVAR_RANGE_FUNC_DECL(sv_aircontrol, "0.00390625", "How much control the player has in the air",
					CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
					0.0f, 32768.0f)

CVAR_RANGE_FUNC_DECL(sv_splashfactor, "1.0", "Rocket explosion thrust effect?",
					CVARTYPE_FLOAT, CVAR_ARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE,
					0.01f, 100.0f)

CVAR(               cl_waddownloaddir, "", "Set custom WAD download directory",
					CVARTYPE_STRING, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)

// Misc stuff
// ----------

CVAR(				developer, "0", "Debugging mode",
					CVARTYPE_BOOL, CVAR_NULL)

CVAR_RANGE_FUNC_DECL(language, "0", "",
					CVARTYPE_INT, CVAR_ARCHIVE, 0.0f, 256.0f)

CVAR(				port, "0", "Display currently used network port number",
					CVARTYPE_INT, CVAR_NOSET | CVAR_NOENABLEDISABLE)

CVAR_RANGE(			chase_height, "-8", "Height of chase camera",
					CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, -32768.0f, 32768.0f)

CVAR_RANGE(			chase_dist, "90", "Chase camera distance",
					CVARTYPE_WORD, CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE, -32768.0f, 32768.0f)

CVAR(				lookspring, "1", "Generate centerview when mlook encountered",
					CVARTYPE_BOOL, CVAR_CLIENTARCHIVE)

CVAR(				waddirs, "", "Allow custom WAD directories to be specified",
					CVARTYPE_STRING, CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// Experimental settings (all categories)
// =======================================


VERSION_CONTROL (c_cvarlist_cpp, "$Id$")
