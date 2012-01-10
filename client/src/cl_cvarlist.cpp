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
//	Client console variables
//
//-----------------------------------------------------------------------------

#include "c_cvars.h"
#include "s_sound.h"
#include "d_netinf.h"

// Automap
// -------

CVAR (am_rotate,			"0", "",		CVAR_ARCHIVE)
CVAR (am_overlay,			"0", "",		CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_showsecrets,		"1", "",		CVAR_ARCHIVE)
CVAR (am_showmonsters,		"1", "",		CVAR_ARCHIVE)
CVAR (am_showtime,			"1", "",		CVAR_ARCHIVE)
CVAR (am_classicmapstring,  "0", "",        CVAR_ARCHIVE)
CVAR (am_usecustomcolors,	"0", "",		CVAR_ARCHIVE)
CVAR (am_ovshare,           "0", "",        CVAR_ARCHIVE)

CVAR (am_backcolor,			"00 00 3a", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_yourcolor,		    "d8 e8 fc", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_wallcolor,		    "00 8b ff", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_tswallcolor,		"10 32 7e", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_fdwallcolor,		"1a 1a 8a", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_cdwallcolor,		"00 00 5a", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_thingcolor,		"9f d3 ff", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_gridcolor,		    "44 44 88", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_xhaircolor,		"80 80 80", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_notseencolor,	    "00 22 6e", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_lockedcolor,	    "bb bb bb", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_exitcolor,		    "ff ff 00", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_teleportcolor,	    "ff a3 00", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

CVAR (am_ovyourcolor,		"d8 e8 fc", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovwallcolor,		"00 8b ff", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovtswallcolor,		"10 32 7e", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovfdwallcolor,		"1a 1a 8a", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovcdwallcolor,		"00 00 5a", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovthingcolor,		"9f d3 ff", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovgridcolor,		"44 44 88", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovxhaircolor,		"80 80 80", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovnotseencolor,	"00 22 6e", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovlockedcolor,	    "bb bb bb", "",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovexitcolor,		"ff ff 00", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (am_ovteleportcolor,	"ff a3 00", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// Console
// -------

CVAR (print_stdout, "0", "", CVAR_ARCHIVE)
CVAR (con_notifytime, "3", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (con_scrlock, "1", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (con_midtime, "3", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)



CVAR_FUNC_DECL (msg0color, "6", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (msg1color, "5", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (msg2color, "2", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (msg3color, "3", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (msg4color, "3", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (msgmidcolor, "5", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// Intermission
// ------------

// Determines whether to draw the scores on intermission.
CVAR (wi_newintermission, "0", "whether to draw the scores on intermission", CVAR_ARCHIVE)


// Menus
// -----

CVAR (ui_dimamount, "0.7", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (ui_dimcolor, "00 00 00", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (ui_transred, "0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (ui_transgreen, "0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR_FUNC_DECL (ui_transblue, "0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// Gameplay/Other
// --------------

// GhostlyDeath <August 1, 2008> -- Join/Part Sound
// [ML] 8/20/2010 - Join sound, part sound
CVAR (cl_connectalert, "1", "Plays a sound when a player joins", CVAR_ARCHIVE)
CVAR (cl_disconnectalert, "1", "Plays a sound when a player quits", CVAR_ARCHIVE)

CVAR (cl_switchweapon, "1", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)	// WPSW_ALWAYS
CVAR (cl_weaponpref1, "5", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref2, "8", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref3, "3", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref4, "2", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref5, "1", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref6, "7", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref7, "4", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref8, "6", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_weaponpref9, "0", "", CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

#ifdef GCONSOLE
	CVAR_FUNC_DECL (use_joystick, "1", "", CVAR_ARCHIVE)
#else
	CVAR_FUNC_DECL (use_joystick, "0", "", CVAR_ARCHIVE)
#endif
CVAR_FUNC_DECL (joy_active, "0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_strafeaxis, "0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_forwardaxis, "1", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_turnaxis, "2", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_lookaxis, "3", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_sensitivity, "10.0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (joy_freelook, "0", "", CVAR_ARCHIVE)
CVAR (joy_invert, "0", "", CVAR_ARCHIVE)

// NES - Currently unused. Make some use of these if possible.
//CVAR (i_remapkeypad, "1", CVAR_ARCHIVE)
//CVAR (use_mouse, "1", CVAR_ARCHIVE)
//CVAR (joy_speedmultiplier, "1", CVAR_ARCHIVE)
//CVAR (joy_xsensitivity, "1", CVAR_ARCHIVE)
//CVAR (joy_ysensitivity, "-1", CVAR_ARCHIVE)
//CVAR (joy_xthreshold, "0.15", CVAR_ARCHIVE)
//CVAR (joy_ythreshold, "0.15", CVAR_ARCHIVE)

CVAR (show_messages, "1", "", CVAR_ARCHIVE)

// Rate of client updates
CVAR_FUNC_DECL (rate, "200", "Rate of client updates in multiplayer mode", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Maximum number of clients who can connect to the server
CVAR (sv_maxclients,       "0", "maximum clients who can connect to server", CVAR_SERVERINFO | CVAR_LATCH)
// Maximum amount of players who can join the game, others are spectators
CVAR (sv_maxplayers,		"0", "maximum players who can join the game, others are spectators", CVAR_SERVERINFO | CVAR_LATCH)

CVAR_FUNC_DECL (cl_autoaim,	"5000", "",		CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// [SL] 2011-05-11 - Client opt-in/out for serverside unlagging
CVAR (cl_unlag,				"1", "client opt-in/out for server unlagging",		CVAR_USERINFO | CVAR_ARCHIVE)

// [SL] 2011-09-01 - Server will send svc_moveplayer updates every N tics
CVAR_FUNC_DECL (cl_updaterate, "2",	"Update players every N tics",	CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

#ifdef _XBOX // Because Xbox players may be unable to communicate for now -- Hyper_Eye
	CVAR (cl_name,		"Xbox Player", "",	CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#else
	CVAR (cl_name,		"Player", "",	CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#endif
CVAR (cl_color,		"40 cf 00", "",	CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_gender,	"male", "",		CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_skin,		"base", "",		CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (cl_team,		"blue", "",		CVAR_USERINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

CVAR (chasedemo, "0", "", CVAR_NULL)

CVAR (cl_run,		"1", "Always run",	CVAR_ARCHIVE)		// Always run? // [Toke - Defaults]

// Mouse settings
// --------------
CVAR (mouse_type,			"0", 	"",	CVAR_ARCHIVE)
CVAR (mouse_sensitivity,	"35.0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

CVAR_FUNC_DECL (cl_mouselook, "0", "", CVAR_CLIENTINFO | CVAR_ARCHIVE)

CVAR (m_pitch,				"0.25",	"",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (m_yaw,				"1.0",	"",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (m_forward,			"1.0",	"",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (m_side,				"2.0", 	"",	CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (novert,				"0", 	"",	CVAR_ARCHIVE)
CVAR (invertmouse,			"0",	"Invert mouse",	CVAR_ARCHIVE)
CVAR (lookstrafe,			"0",	"Strafe with mouse",	CVAR_ARCHIVE)
CVAR (mouse_acceleration,	"0", 	"", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (mouse_threshold,		"0", 	"", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (m_filter,				"0",	"Smooth mouse input", CVAR_ARCHIVE)
CVAR (dynres_state,			"0", 	"",	CVAR_ARCHIVE)
CVAR (dynresval,			"1.0",	"",CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (displaymouse,			"0",	"Display mouse values",	CVAR_ARCHIVE)		// [Toke - Mouse] added for mouse menu


CVAR (idmypos, "0", "Shows current player position on map", CVAR_NULL)

// Heads up display
// ----------------

CVAR (hud_revealsecrets, "0", "", CVAR_ARCHIVE)
CVAR (hud_transparency, "0.5", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (hud_scale, "0", "", CVAR_ARCHIVE)
CVAR_FUNC_DECL (hud_scaletext, "2", "Scale notify text at high resolutions", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (hud_targetnames, "1", "", CVAR_ARCHIVE)
CVAR (hud_usehighresboard, "1", "",	CVAR_ARCHIVE)
CVAR (hud_fullhudtype, "0","The fullscreen hud to display: 0=zdoom,1=odamex",CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

CVAR (hud_crosshairdim, "0", "Crosshair transparency", CVAR_ARCHIVE)      // Crosshair transparency
CVAR (hud_crosshairscale, "0", "Crosshair scaling", CVAR_ARCHIVE)    // Crosshair scaling
CVAR (hud_crosshairhealth, "0", "Color of crosshair represents health level", CVAR_ARCHIVE)
CVAR_FUNC_DECL (hud_targetcount, "2", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)  // Show target counts

#ifdef _XBOX
CVAR (chatmacro0, "Hi.", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)                       // A
CVAR (chatmacro1, "I'm ready to kick butt!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)   // B
CVAR (chatmacro2, "Help!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)                     // X
CVAR (chatmacro3, "GG", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)                        // Y
CVAR (chatmacro4, "No", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)                       // Black
CVAR (chatmacro5, "Yes", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)                        // White
CVAR (chatmacro6, "I'll take care of it.", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)     // Left Trigger
CVAR (chatmacro7, "Come here!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)                // Right Trigger
CVAR (chatmacro8, "Thanks for the game. Bye.", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE) // Start
CVAR (chatmacro9, "I am on Xbox and can only use chat macros.", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE) // Back
#else
// GhostlyDeath <November 2, 2008> -- someone had the order wrong (0-9!)
CVAR (chatmacro1, "I'm ready to kick butt!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro2, "I'm OK.", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro3, "I'm not looking too good!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro4, "Help!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro5, "You suck!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro6, "Next time, scumbag...", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro7, "Come here!", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro8, "I'll take care of it.", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro9, "Yes", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (chatmacro0, "No", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#endif

// Sound and music
// ---------------

CVAR_FUNC_DECL (snd_sfxvolume, "0.5", "Sound volume", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)      // Sound volume
CVAR_FUNC_DECL (snd_musicvolume, "0.5", "Music volume", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)    // Music volume
CVAR (snd_crossover, "0", "Stereo switch", CVAR_ARCHIVE)                                         // Stereo switch
CVAR (snd_samplerate, "22050", "Samplerate", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)             // Sample rate
CVAR (snd_timeout, "0", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)					// Clean up finished sounds
BEGIN_CUSTOM_CVAR (snd_channels, "12", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)     // Number of channels available
{
	S_Stop();
	S_Init (snd_sfxvolume, snd_musicvolume);
}
END_CUSTOM_CVAR (snd_channels)
CVAR_FUNC_DECL (snd_musicsystem, "1", "Music subsystem preference", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (snd_musicdevice, "", "Music output device for the chosen music subsystem", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// Status bar
// ----------

CVAR_FUNC_DECL (st_scale, "1", "", CVAR_ARCHIVE)

// Video and Renderer
// ------------------

// Gamma correction level, 1 - 4
CVAR_FUNC_DECL (gammalevel, "1", "Gamma correction level, 1 - 4", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Type of crosshair, 0 means none
CVAR_FUNC_DECL (hud_crosshair, "0", "Type of crosshair, 0 means no crosshair", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Column optimization method
CVAR (r_columnmethod, "1", "Column optimization method", CVAR_CLIENTINFO | CVAR_ARCHIVE)
// Detail level (affects performance)
CVAR_FUNC_DECL (r_detail, "0", "Detail level (affects performance)", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Disables all texturing of walls
CVAR (r_drawflat, "0", "Disables all texturing of walls", CVAR_CLIENTINFO)
// Draw player sprites
CVAR (r_drawplayersprites, "1", "Draw player sprites", CVAR_CLIENTINFO)
// Draw particles
CVAR (r_particles, "1","Draw particles",CVAR_CLIENTINFO)
// Stretch sky textures. (0 - always off, 1 - always on, 2 - auto)
CVAR_FUNC_DECL (r_stretchsky, "2", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Invulnerability sphere changes the palette of the sky
CVAR (r_skypalette, "0", "Invulnerability sphere changes the palette of the sky", CVAR_ARCHIVE)

#ifdef _XBOX // The burn wipe works better in 720p
CVAR (r_wipetype, "2", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#else
CVAR (r_wipetype, "1", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#endif
CVAR (r_showendoom, "1", "", CVAR_ARCHIVE)   // [ML] 1/5/10: Add endoom support

// [ML] Value of red pain intensity shift
CVAR_FUNC_DECL (r_painintensity, "1", "Value of red pain intensity shift", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

// TODO: document
CVAR (r_viewsize, "0", "", CVAR_CLIENTINFO | CVAR_NOSET | CVAR_NOENABLEDISABLE)
#ifdef GCONSOLE
	// Standard SDTV resolution is the default on game consoles
	CVAR (vid_defwidth, "640", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
	CVAR (vid_defheight, "480", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#else
	// Default video dimensions
	CVAR (vid_defwidth, "320", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
	CVAR (vid_defheight, "200", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
#endif
// Default bitdepth
CVAR (vid_defbits, "8", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Force video mode
CVAR (autoadjust_video_settings, "1", "", CVAR_CLIENTINFO | CVAR_ARCHIVE)
// Frames per second counter
CVAR (vid_fps, "0", "", CVAR_CLIENTINFO)
// Fullscreen mode
#ifdef GCONSOLE
	CVAR_FUNC_DECL (vid_fullscreen, "1", "", CVAR_CLIENTINFO | CVAR_ARCHIVE)
#else
	CVAR_FUNC_DECL (vid_fullscreen, "0", "", CVAR_CLIENTINFO | CVAR_ARCHIVE)
#endif
// TODO: document
CVAR_FUNC_DECL (screenblocks, "10", "", CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Older (Doom-style) FPS counter
CVAR (vid_ticker, "0", "", CVAR_CLIENTINFO)
// Resizes the window by a scale factor
CVAR_FUNC_DECL (vid_winscale, "1.0", "", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Overscan
CVAR_FUNC_DECL (vid_overscan, "1.0", "Overscan", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

VERSION_CONTROL (cl_cvarlist_cpp, "$Id$")
