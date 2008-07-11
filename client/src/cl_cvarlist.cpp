// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: cl_cvarlist.cpp 971 2008-07-03 00:56:27Z russellrice $
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom).
// Copyright (C) 2006-2008 by The Odamex Team.
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

// Gameplay/Other
// --------------

// TODO: document
CVAR_FUNC_DECL (cl_freelook, "0", CVAR_CLIENTINFO | CVAR_ARCHIVE)
CVAR_FUNC_DECL (sv_freelook, "0", CVAR_SERVERINFO)

// Maximum number of clients who can connect to the server
CVAR (maxclients,       "0", CVAR_SERVERINFO | CVAR_LATCH)
// Maximum amount of players who can join the game, others are spectators
CVAR (maxplayers,		"0", CVAR_SERVERINFO | CVAR_LATCH)

// Video and Renderer
// ------------------

// Column optimization method
CVAR (r_columnmethod, "1", CVAR_CLIENTINFO | CVAR_ARCHIVE)
// Detail level?
CVAR_FUNC_DECL (r_detail, "0", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Disables all texturing of walls
CVAR (r_drawflat, "0", CVAR_CLIENTINFO)
// Draw player sprites
CVAR (r_drawplayersprites, "1", CVAR_CLIENTINFO)
// Stretch sky textures
CVAR (r_stretchsky, "1", CVAR_CLIENTINFO | CVAR_ARCHIVE)
// TODO: document
CVAR (r_viewsize, "0", CVAR_CLIENTINFO | CVAR_NOSET | CVAR_NOENABLEDISABLE)
// Default video dimensions and bitdepth
CVAR (vid_defwidth, "320", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (vid_defheight, "200", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
CVAR (vid_defbits, "8", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)
// Frames per second counter
CVAR (vid_fps, "0", CVAR_CLIENTINFO)
// Fullscreen mode
CVAR (vid_fullscreen, "0", CVAR_CLIENTINFO | CVAR_ARCHIVE)
// Older (Doom-style) FPS counter
CVAR (vid_ticker, "0", CVAR_CLIENTINFO)
// Resizes the window by a scale factor
CVAR_FUNC_DECL (vid_winscale, "1.0", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)


VERSION_CONTROL (c_cvarlist_cpp, "$Id: cl_cvarlist.cpp 971 2008-07-03 00:56:27Z russellrice $")
