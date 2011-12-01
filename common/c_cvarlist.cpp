// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2010 by The Odamex Team.
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
//	Common console variables, used by both client and server.
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"

// Server settings
// ---------------

// Game mode
CVAR (sv_gametype, "0", "Sets the game mode, values are:\n" \
                        "// 0 = Cooperative\n" \
                        "// 1 = Deathmatch\n" \
                        "// 2 = Team Deathmatch\n" \
                        "// 3 = Capture The Flag", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)

// (Coop/Teamplay/CTF): Players can injure others on the same team
CVAR (sv_friendlyfire, "1", "When set, players can injure others on the same team, it is ignored in deathmatch", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// (Teamplay/CTF): Game ends when team score is reached
CVAR (sv_scorelimit, "5", "Game ends when team score is reached in Teamplay/CTF", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// (Teamplay/CTF) When disabled, treat team spawns like normal deathmatch spawns.
CVAR (sv_teamspawns, "1", "When disabled, treat team spawns like normal deathmatch spawns in Teamplay/CTF", 
      CVAR_SERVERARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)
// Cheat code usage is allowed
CVAR (sv_allowcheats, "0", "Allow usage of cheats in all game modes", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Exit switch/teleports are usable
CVAR (sv_allowexit, "1", "Allow use of Exit switch/teleports in all game modes", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Players can jump
CVAR (sv_allowjump, "0", "Allows players to jump when set in all game modes", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Give double ammo regardless of difficulty
CVAR (sv_doubleammo, "0", "Give double ammo regardless of difficulty", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Makes water movement more realistic
CVAR_FUNC_DECL (sv_forcewater, "0", "Makes water more realistic (boom maps at the moment)", 
                CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Look up/down is allowed
CVAR (sv_freelook,		"0", "Allow Looking up and down", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Names of players appear in the FOV
CVAR (sv_allowtargetnames, "0", "When set, names of players appear in the FOV", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Game ends on frag limit being reached
CVAR (sv_fraglimit,		"0", "Sets the amount of frags a player can accumulate before the game ends", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters are at nightmare speed
CVAR (sv_fastmonsters,		"0", "Monsters are at nightmare speed", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Allow exit switch at maxfrags, must click to exit
CVAR (sv_fragexitswitch,   "0", "Allow exit switches to become usable on fraglimit hit", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Players will have infinite ammunition
CVAR (sv_infiniteammo,		"0", "Infinite ammo for all players", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Items will respawn after time
CVAR (sv_itemsrespawn,		"0", "Items will respawn after a fixed period, see sv_itemrespawntime", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// If itemrespawn is on, items will respawn after this time. (in seconds)
CVAR (sv_itemrespawntime,	"30", "If sv_itemsrespawn is set, items will respawn after this time (in seconds)", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)
// Monsters will respawn after time
CVAR (sv_monstersrespawn,	"0", "Monsters will respawn after a period of time", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Monsters are not present
CVAR (sv_nomonsters,		"0", "No monsters will be present", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// Skill level
CVAR (sv_skill,             "3", "Sets the skill level, values are:\n" \
                                 "// 0 - No things mode\n" \
                                 "// 1 - I'm Too Young To Die\n" \
                                 "// 2 - Hey, Not Too Rough\n" \
                                 "// 3 - Hurt Me Plenty\n" \
                                 "// 4 - Ultra-Violence\n" \
                                 "// 5 - Nightmare", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH | CVAR_NOENABLEDISABLE)
// Game ends on time limit being reached
CVAR_FUNC_DECL (sv_timelimit,		"0", "Sets the time limit for the game to end, must be greater than 1", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

// Speedhack code (server related)
CVAR (sv_speedhackfix,		"0", "", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// Weapons stay
CVAR (sv_weaponstay,		"1", "Weapons stay after pickup", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// [SL] 2011-05-11 - Allow reconciliation for players on lagged connections
CVAR (sv_unlag,				"1", "Allow reconciliation for players on lagged connections", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)
// [ML] allow weapon bob changing
CVAR (sv_allownobob, "0", "Allow weapon bob changing", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// [ML] Allow clients to adjust amount of red pain screen intensity
CVAR (sv_allowredscreen, "0","Allow clients to adjust amount of red pain screen intensity", 
	  CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

// [SL] Allow PWO
CVAR (sv_allowpwo, "0", "Allow clients to set their preferences for automatic weapon swithching",
	CVAR_SERVERINFO | CVAR_SERVERARCHIVE)

// Compatibility options for vanilla
// ---------------------------------

// Enable/disable infinitely tall actors
CVAR (co_realactorheight, "0", "Enable/Disable infinitely tall actors", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// [Spleen] When enabled, monsters can get pushed or thrusted off of ledges, like in boom
CVAR (co_allowdropoff, "0", "When enabled, monsters can get pushed or thrusted off of ledges, like in boom", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// [ML] When enabled, activate correct impact of projectiles and bullets on surfaces (spawning puffs,explosions)
CVAR (co_fixweaponimpacts, "0", "When enabled, activate correct impact of projectiles and bullets on surfaces (spawning puffs,explosions)", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// [ML] When enabled, additional checks are made on two-sided lines, allows additional 
// silent bfg tricks, and the player will "oof" on two-sided lines
CVAR (co_boomlinecheck, "0", "additional checks are made on two-sided lines, allows additional silent bfg tricks, and the player will \"oof\" on two-sided lines", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Enable/disable the "level 8 full sound at far distances" feature
CVAR (co_level8soundfeature, "0", "Enable/disable the \"level 8 full sound at far distances\" feature", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_LATCH)

// Enable/disable zdoom-based gravity and physics interactions
CVAR (co_zdoomphys, "0", "Enable/disable zdoom-based gravity and physics interactions", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// 0 - Play the switch resetting sound at map location (0,0) like the vanilla bug
// 1 - switch sounds attenuate with distance like plats and doors.
CVAR (co_zdoomswitches, "0", "Play the switch resetting sound at map location (0,0) like the vanilla bug", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Use ZDoom's sound curve instead of vanilla Doom's
CVAR (co_zdoomsoundcurve, "0", "Use ZDoom's sound curve instead of vanilla Doom's", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Turns off the west-facing silent spawns vanilla bug
CVAR (co_nosilentspawns, "0", "Turns off the west-facing silent spawns vanilla bug", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)

// Dead player's view follows the actor who killed them
CVAR (cl_deathcam, "1", "Dead player's view follows the actor who killed them", 
      CVAR_ARCHIVE)

// Movebob
CVAR (cl_nobob, "0", "Movebob", 
      CVAR_CLIENTARCHIVE | CVAR_CLIENTINFO)

CVAR (cl_rockettrails, "0", "Rocket trails on/off (currently unused)", CVAR_ARCHIVE)

// [AM] Force a player to respawn.
CVAR (sv_forcerespawn, "0", "Force a player to respawn.", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
// [AM] Force a player to respawn after a set amount of time
CVAR (sv_forcerespawntime, "30", "Force a player to respawn after a set amount of time", 
      CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL (sv_gravity, "800", "Gravity of the environment", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO)
CVAR_FUNC_DECL (sv_aircontrol, "0.00390625", "How much control the player has in the air", 
      CVAR_ARCHIVE | CVAR_SERVERARCHIVE | CVAR_SERVERINFO | CVAR_NOENABLEDISABLE)


// Misc stuff
// ----------

// debugging mode
CVAR (developer, "0", "debugging mode", 
      CVAR_NULL)
// Port (???)
CVAR (port, "0", "Display currently used port number", 
      CVAR_NOSET | CVAR_NOENABLEDISABLE)
// File compression (???)
CVAR (filecompression,	"1", "", 
      CVAR_ARCHIVE)
// Chase camera settings
CVAR (chase_height,		"-8", "Height of chase camera", 
      CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chase_dist,		"90", "Chase camera distance", 
      CVAR_CLIENTARCHIVE | CVAR_NOENABLEDISABLE)
// Generate centerview when -mlook encountered?
CVAR (lookspring,		"1", "Generate centerview when mlook encountered", 
      CVAR_CLIENTARCHIVE)
// Allows players to walk through other players
CVAR (sv_unblockplayers, "0", "Allows players to walk through other players", 
      CVAR_ARCHIVE | CVAR_LATCH | CVAR_SERVERINFO)
// [Spleen] Allow custom WAD directories to be specified in a cvar
CVAR (waddirs, "", "Allow custom WAD directories to be specified", 
      CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

VERSION_CONTROL (c_cvarlist_cpp, "$Id$")
