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

// Video
// -----

// Frames per second counter
CVAR (vid_fps, "0", CVAR_CLIENTINFO)
// Older (Doom-style) FPS counter
CVAR (vid_ticker, "0", CVAR_CLIENTINFO)
// Fullscreen mode
CVAR (vid_fullscreen, "0", CVAR_CLIENTINFO | CVAR_ARCHIVE)

// Function CVARs
// --------------

CVAR_FUNC_DECL(vid_winscale, "1.0", CVAR_CLIENTINFO | CVAR_ARCHIVE | CVAR_NOENABLEDISABLE)

VERSION_CONTROL (c_cvarlist_cpp, "$Id: cl_cvarlist.cpp 971 2008-07-03 00:56:27Z russellrice $")
