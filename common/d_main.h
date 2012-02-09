// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: d_main.h 1852 2010-09-04 23:53:26Z ladna $
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	System specific interface stuff.
//
//-----------------------------------------------------------------------------


#ifndef __D_MAIN__
#define __D_MAIN__

#include "d_event.h"

#include <vector>
#include <string>

//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
void D_DoomMain(void);

std::string BaseFileSearch(std::string file, std::string ext = "",
                           std::string hash = "");
std::vector<size_t> D_DoomWadReboot(
	const std::vector<std::string> &wadnames,
    const std::vector<std::string> &patch_files = std::vector<std::string>(),
    std::vector<std::string> needhashes = std::vector<std::string>()
);

// Called by IO functions when input is detected.
void D_PostEvent(const event_t* ev);

//
// BASE LEVEL
//
void D_PageTicker(void);
void D_PageDrawer(void);
void D_AdvanceDemo(void);
void D_StartTitle(void);


// [RH] Set this to something to draw an icon during the next screen refresh.
extern const char *D_DrawIcon;

void D_AddSearchDir(std::vector<std::string> &dirs, const char *dir, const char separator);

#endif


