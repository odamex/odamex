// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2024 by The Odamex Team.
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

#pragma once

#include "d_event.h"
#include "m_resfile.h"

extern std::string LOG_FILE;

//
// D_DoomMain()
// Not a globally visible function, just included for source reference,
// calls all startup code, parses command line options.
// If not overrided by user input, calls N_AdvanceDemo.
//
void D_DoomMain(void);

void D_LoadResourceFiles(const OWantFiles& newwadfiles, const OWantFiles& newpatchfiles);
bool D_DoomWadReboot(const OWantFiles& newwadfiles, const OWantFiles& newpatchfiles);

    // Called by IO functions when input is detected.
void D_PostEvent(const event_t* ev);

//
// BASE LEVEL
//
void D_PageTicker(void);
void D_PageDrawer(void);
void D_AdvanceDemo(void);
void D_StartTitle(void);
void D_DisplayTicker(void);

// [RH] Set this to something to draw an icon during the next screen refresh.
extern const char *D_DrawIcon;

void D_AddSearchDir(std::vector<std::string> &dirs, const char *dir, const char separator);
void D_AddPlatformSearchDirs(std::vector<std::string>& dirs);
void D_LoadResolvedPatches();
std::string D_CleanseFileName(const std::string &filename, const std::string &ext = "");

extern OResFiles wadfiles;
extern OResFiles patchfiles;
extern OWantFiles missingfiles;

extern bool capfps;
extern float maxfps;
void STACK_ARGS D_ClearTaskSchedulers();
void D_RunTics(void (*sim_func)(), void(*display_func)());

void D_AddWadCommandLineFiles(OWantFiles& out);
void D_AddDehCommandLineFiles(OWantFiles& out);

std::string D_GetTitleString();

void D_Init();
void STACK_ARGS D_Shutdown();
