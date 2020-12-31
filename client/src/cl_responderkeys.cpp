// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//	Common descriptors for commands on the menu.
//  This was done to make console ports key handlers easier on the menu/console.
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
        case PF_SWITCH:
            return (key == KEY_JOY14);    
    default:
        break;
    }

    return (key == KEY_HAT1 || key == KEY_UPARROW || key == KEYP_8 || key == KEY_JOY12);
}

//
// FResponderKey::IsDownKey
//
bool FResponderKey::IsDownKey(int key)
{
    switch (platform)
    {
        case PF_SWITCH:
            return (key == KEY_JOY16);   
    default:
        break;
    }

    return (key == KEY_HAT3 || key == KEY_DOWNARROW || key == KEYP_2 || key == KEY_JOY13);
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
        case PF_SWITCH:
            return (key == KEY_JOY13);   
    default:
        break;
    }

    return (key == KEY_HAT4 || keyboard || key == KEY_JOY14);
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
    case PF_SWITCH:
        return (key == KEY_JOY15);  
    default:
        break;
    }

    return (key == KEY_HAT2 || keyboard || key == KEY_JOY15);
}

bool FResponderKey::IsPageUpKey(int key)
{
    bool keyboard = (key == KEY_PGUP);

    switch (platform)
    {
        case PF_XBOX:
            return (key == KEY_JOY7 || keyboard);
        case PF_SWITCH:
            return (key == KEY_JOY7);  
        default:
            break;
    }

    return (keyboard || key == KEY_JOY10);
}

bool FResponderKey::IsPageDownKey(int key)
{
    bool keyboard = (key == KEY_PGDN);

    switch (platform)
    {
        case PF_XBOX:
            return (key == KEY_JOY8 || keyboard);
        case PF_SWITCH:
            return (key == KEY_JOY8);  
    default:
        break;
    }

    return (keyboard || key == KEY_JOY11);
}

//
// FResponderKey::IsAcceptKey
//
bool FResponderKey::IsAcceptKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_ENTER || key == KEYP_ENTER);

    switch (platform)
    {
        case PF_WII:
            return (key == KEY_JOY10 || keyboard);
        case PF_SWITCH:
            return (key == KEY_JOY1);  
        default:
            break;
    }

    return key == KEY_JOY1 || keyboard;
}

//
// FResponderKey::IsCancelKey
//
bool FResponderKey::IsCancelKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == KEY_ESCAPE);

    switch (platform)
    {
        case PF_WII:
            return (key == KEY_JOY11 || key == KEY_JOY1 || keyboard);
        case PF_SWITCH:
            return (key == KEY_JOY2);  
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
        return (key == KEY_JOY10 || key == KEY_JOY1); // A | (a)
    case PF_SWITCH:
            return (key == KEY_JOY1);  
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
    case PF_SWITCH:
        return (key == KEY_JOY2); 
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
        case PF_SWITCH:
            return (key == KEY_JOY11);    // (+)
    default:
        break;
    }

    return (keyboard || key == KEY_JOY7);
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
        return (key == KEY_JOY9 || keyboard); // ?
    case PF_SWITCH:
        return (key == KEY_JOY3); // X
    default:
        break;
    }

    return (keyboard || key == KEY_JOY4);
}

bool FResponderKey::IsSpyNextKey(int key)
{
    // Default Keyboard press
    bool mouse = (key == KEY_MWHEELDOWN);

    switch (platform)
    {
    case PF_WII:
        return mouse;
    case PF_XBOX:
        return (mouse); // TODO : Need to be RIGHT
    case PF_SWITCH:
        return (key == KEY_JOY15);   // DPAD-RIGHT
    default:
        break;
    }

    return (mouse || key == KEY_JOY15);
}

bool FResponderKey::IsSpyPrevKey(int key)
{
    // Default Keyboard press
    bool mouse = (key == KEY_MWHEELUP);

    switch (platform)
    {
    case PF_WII:
        return mouse;
    case PF_XBOX:
        return (mouse); // TODO : Need to be LEFT
    case PF_SWITCH:
        return (key == KEY_JOY13);   // DPAD-LEFT
    default:
        break;
    }

    return (mouse || key == KEY_JOY14);
} 

bool FResponderKey::IsTabulationKey(int key)
{
    bool keyboard = (key == KEY_TAB);

    switch (platform)
    {
        case PF_WII:
            return keyboard;
        case PF_XBOX:
            return keyboard;
        default:
            break;
    }

    return (keyboard);
}

/*#ifdef GCONSOLE
bool FResponderKey::IsOSKeyboardKey(int key)
{
    switch (platform)
    {
    case PF_SWITCH:
        return KEY_JOY3;
    default:
        break;
    }
    return false;
}
#endif
*/
