// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 2006-2019 by The Odamex Team.
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

#ifndef __CPLATFORM_H__
#define __CPLATFORM_H__

struct FResponderKey
{
    public:

        // Movement Keys
        bool IsUpKey (int key);
        bool IsDownKey (int key);
        bool IsLeftKey (int key);
        bool IsRightKey (int key);

        bool IsPageUpKey (int key);
        bool IsPageDownKey (int key);

        bool IsEnterKey (int key);
        bool IsReturnKey (int key);
        bool IsMenuKey  (int key);
        bool IsYesKey (int key);
        bool IsNoKey (int key);
        bool IsUnbindKey (int key);
};

extern FResponderKey keypress;

#endif //__CPLATFORM_H__