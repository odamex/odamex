// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2020 by The Odamex Team.
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
//
// GUI Abstraction to SDL.
//
//-----------------------------------------------------------------------------

#ifndef __I_GUI_H__
#define __I_GUI_H__

extern "C" {
	#include "microui.h"
}

void I_InitGUI();
void I_QuitGUI();
void I_DrawGUI();
void UI_SelectIWAD();

#endif  // __I_GUI_H__
