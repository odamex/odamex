// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
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
//	Common descriptors for commands on the menu.
//  That way, responders will be way cleaner depending on what platform they are.
//
//-----------------------------------------------------------------------------

#pragma once

        // Movement Keys
bool Key_IsUpKey(int key, bool numlock);
bool Key_IsDownKey(int key, bool numlock);
bool Key_IsLeftKey(int key, bool numlock);
bool Key_IsRightKey(int key, bool numlock);

bool Key_IsPageUpKey(int key, bool numlock);
bool Key_IsPageDownKey(int key, bool numlock);
bool Key_IsHomeKey(int key, bool numlock);
bool Key_IsEndKey(int key, bool numlock);
bool Key_IsInsKey(int key, bool numlock);
bool Key_IsDelKey(int key, bool numlock);

bool Key_IsAcceptKey(int key);
bool Key_IsCancelKey(int key);
bool Key_IsMenuKey(int key);
bool Key_IsYesKey(int key);
bool Key_IsNoKey(int key);
bool Key_IsUnbindKey(int key);

bool Key_IsSpyPrevKey(int key);
bool Key_IsSpyNextKey(int key);

bool Key_IsTabulationKey(int key);
