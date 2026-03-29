// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2026 by The Odamex Team.
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
//   Player setup menu, including team, gender, and color selection.
//
//-----------------------------------------------------------------------------

#pragma once

void M_PlayerSetupInit();
void M_PlayerSetupShutdown();
void M_PlayerSetupOpen(int& currentItem);
void M_PlayerSetupTicker();
void M_PlayerSetupDrawer(int currentItem);
bool M_PlayerSetupIndicatorPosition(int currentItem, int& x, int& y);
void M_PlayerSetupResponder(int ch, int ch2, bool numlock, int& currentItem);
