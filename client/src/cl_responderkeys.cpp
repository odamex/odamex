// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2015 by The Odamex Team.
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

#include "doomdef.h"
#include "doomkeys.h"
#include "cl_responderkeys.h"
#include "i_input.h"

FResponderKey keypress;

//
// FResponderKey::IsUpKey
//
bool FResponderKey::IsUpKey(int key)
{
    switch (platform) 
    {
        default:
            break;     
    }

    return (key == KEY_HAT1 || key == KEY_UPARROW || key == KEYP_8 );
}

//
// FResponderKey::IsDownKey
//
bool FResponderKey::IsDownKey(int key)
{
    switch (platform) 
    {
        default:
            break;   
    }

    return (key == KEY_HAT3 || key == KEY_DOWNARROW || key == KEYP_2 );
}

//
// FResponderKey::IsLeftKey
//
bool FResponderKey::IsLeftKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_LEFTARROW || key == KEYP_4);

    switch (platform) 
    {
        default:
            break;          
    }
    
    return (key == KEY_HAT4 || keyboard );
}

//
// FResponderKey::IsRightKey
//
bool FResponderKey::IsRightKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_RIGHTARROW || key == KEYP_6);

    switch (platform) 
    {
        case PF_WII:
            return keyboard;
        default:
            break;            
    }
    
    return (key == KEY_HAT2 || keyboard);
}

bool FResponderKey::IsPageUpKey(int key)
{
    switch (platform) 
    {
        default:
            break;            
    }
    
    return (key == KEY_PGUP);
}

bool FResponderKey::IsPageDownKey(int key)
{
    switch (platform) 
    {
        default:
            break;            
    }
    
    return (key == KEY_PGDN);
}

//
// FResponderKey::IsEnterKey
//
bool FResponderKey::IsEnterKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_ENTER || key == KEYP_ENTER);

    switch (platform) 
    {
        case PF_WII:
            return (key == KEY_JOY10 || keyboard);
        default:
            break;   
    }
    
    return key == KEY_JOY1 || keyboard;
}

//
// FResponderKey::IsReturnKey
//
bool FResponderKey::IsReturnKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_ESCAPE);

    switch (platform) 
    {   
        case PF_WII:
            return (key == KEY_JOY11 || key == KEY_JOY1 || keyboard);
        default:
            break;   
    }
    
    return (key == KEY_JOY2 || keyboard);
}

//
// FResponderKey::IsYesKey
//
bool FResponderKey::IsYesKey(int key)
{
    switch (platform) 
    { 
        case PF_WII:
            return (key == KEY_JOY10 || key == KEY_JOY1);		// A | (a)
        default:
            break;   
    }
    
    return (key == KEY_JOY1);

}

//
// FResponderKey::IsNoKey
//
bool FResponderKey::IsNoKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_ESCAPE);

    switch (platform) 
    {
        case PF_WII:
            return (key == KEY_JOY11 || key == KEY_JOY2 || keyboard);
        default:
            break;   
    }
    
    return (key == KEY_JOY2 || keyboard);

}

//
// FResponderKey::IsMenuKey
//
bool FResponderKey::IsMenuKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_ESCAPE);

    switch (platform) 
    {
        case PF_WII:
           // if (I_WhatWiiController() == WIICTRL_WIIMOTE)
                return (key == KEY_JOY7 || key == KEY_JOY19 || keyboard); // (HOME on Wiimote | START - Pro Controller)
          /*  else if (I_WhatWiiController() == WIICTRL_GAMECUBE)
                return (key == KEY_JOY7 || keyboard);                      // Start*/
        break;
        case PF_XBOX:
            return (key == KEY_JOY9 || keyboard);
        default:
            break;   
    }
    
    return (keyboard);
}

bool FResponderKey::IsUnbindKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_BACKSPACE);

    switch (platform) 
    {
        case PF_WII:
            return keyboard;
        case PF_XBOX:
            return (key == KEY_JOY9 || keyboard); // START
        default:
            break;   
    }
    
    return (keyboard);
}