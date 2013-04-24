// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	SDL input handler
//
//-----------------------------------------------------------------------------


#ifndef __I_INPUT_H__
#define __I_INPUT_H__

#include "doomtype.h"

#define MOUSE_DOOM 0
#define MOUSE_ODAMEX 1
#define MOUSE_ZDOOM_DI 2

bool I_InitInput (void);
void STACK_ARGS I_ShutdownInput (void);
void I_PauseMouse();
void I_ResumeMouse();

int I_GetJoystickCount();
std::string I_GetJoystickNameFromIndex (int index);
bool I_OpenJoystick();
void I_CloseJoystick();

void I_GetEvent (void);

void I_EnableKeyRepeat();
void I_DisableKeyRepeat();

#if defined WIN32 && !defined _XBOX
void BackupGDIMouseSettings();
void STACK_ARGS RestoreGDIMouseSettings();
#endif

#endif
