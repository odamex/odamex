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


#include "odamex.h"

#include "doomdef.h"
#include "doomkeys.h"
#include "cl_responderkeys.h"
#include "i_input.h"

//
// Key_IsUpKey
//
bool Key_IsUpKey(int key)
{
    switch (platform)
    {
        case PF_SWITCH:
            return (key == OKEY_JOY12);    
    default:
        break;
    }

    return (key == OKEY_HAT1 || key == OKEY_UPARROW || key == OKEYP_8 || key == OKEY_JOY12);
}

//
// Key_IsDownKey
//
bool Key_IsDownKey(int key)
{
    switch (platform)
    {
        case PF_SWITCH:
            return (key == OKEY_JOY13);   
    default:
        break;
    }

    return (key == OKEY_HAT3 || key == OKEY_DOWNARROW || key == OKEYP_2 || key == OKEY_JOY13);
}

//
// Key_IsLeftKey
//
bool Key_IsLeftKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_LEFTARROW || key == OKEYP_4);

    switch (platform)
    {
        case PF_SWITCH:
            return (key == OKEY_JOY14);   
    default:
        break;
    }

    return (key == OKEY_HAT4 || keyboard || key == OKEY_JOY14);
}

//
// Key_IsRightKey
//
bool Key_IsRightKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_RIGHTARROW || key == OKEYP_6);

    switch (platform)
    {
    case PF_WII:
        return keyboard;
    case PF_SWITCH:
        return (key == OKEY_JOY15);  
    default:
        break;
    }

    return (key == OKEY_HAT2 || keyboard || key == OKEY_JOY15);
}

bool Key_IsPageUpKey(int key)
{
    bool keyboard = (key == OKEY_PGUP);

    switch (platform)
    {
        case PF_XBOX:
            return (key == OKEY_JOY7 || keyboard);
        case PF_SWITCH:
            return (key == OKEY_JOY11);  
        default:
            break;
    }

    return (keyboard || key == OKEY_JOY10);
}

bool Key_IsPageDownKey(int key)
{
    bool keyboard = (key == OKEY_PGDN);

    switch (platform)
    {
        case PF_XBOX:
            return (key == OKEY_JOY8 || keyboard);
        case PF_SWITCH:
            return (key == OKEY_JOY10);  
    default:
        break;
    }

    return (keyboard || key == OKEY_JOY11);
}

//
// Key_IsAcceptKey
//
bool Key_IsAcceptKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ENTER || key == OKEYP_ENTER);

    switch (platform)
    {
        case PF_WII:
            return (key == OKEY_JOY10 || keyboard);
        case PF_SWITCH:
            return (key == OKEY_JOY2);  
        default:
            break;
    }

    return key == OKEY_JOY1 || keyboard;
}

//
// Key_IsCancelKey
//
bool Key_IsCancelKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ESCAPE);

    switch (platform)
    {
        case PF_WII:
            return (key == OKEY_JOY11 || key == OKEY_JOY1 || keyboard);
        case PF_SWITCH:
            return (key == OKEY_JOY1);  
        default:
            break;
    }

    return (key == OKEY_JOY2 || keyboard);
}

//
// Key_IsYesKey
//
bool Key_IsYesKey(int key)
{
    switch (platform)
    {
    case PF_WII:
        return (key == OKEY_JOY10 || key == OKEY_JOY1); // A | (a)
    case PF_SWITCH:
            return (key == OKEY_JOY2);  
    default:
        break;
    }

    return (key == OKEY_JOY1);
}

//
// Key_IsNoKey
//
bool Key_IsNoKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ESCAPE);

    switch (platform)
    {
    case PF_WII:
        return (key == OKEY_JOY11 || key == OKEY_JOY2 || keyboard);
    case PF_SWITCH:
        return (key == OKEY_JOY2); 
   default:
        break;
    }

    return (key == OKEY_JOY2 || keyboard);
}

//
// Key_IsMenuKey
//
bool Key_IsMenuKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_ESCAPE);

    switch (platform)
    {
    case PF_WII:
        // if (I_WhatWiiController() == WIICTRL_WIIMOTE)
        return (key == OKEY_JOY7 || key == OKEY_JOY19 || keyboard); // (HOME on Wiimote | START - Pro Controller)
                                                                  /*  else if (I_WhatWiiController() == WIICTRL_GAMECUBE)
                return (key == OKEY_JOY7 || keyboard);                      // Start*/
        break;
    case PF_XBOX:
        return (key == OKEY_JOY9 || keyboard);
        case PF_SWITCH:
            return (key == OKEY_JOY7);    // (+)
    default:
        break;
    }

    return (keyboard || key == OKEY_JOY7);
}

bool Key_IsUnbindKey(int key)
{
    // Default Keyboard press
    bool keyboard = (key == OKEY_BACKSPACE);

    switch (platform)
    {
    case PF_WII:
        return keyboard;
    case PF_XBOX:
        return (key == OKEY_JOY9 || keyboard); // ?
    case PF_SWITCH:
        return (key == OKEY_JOY4); // X
    default:
        break;
    }

    return (keyboard || key == OKEY_JOY4);
}

bool Key_IsSpyNextKey(int key)
{
    // Default Keyboard press
    bool mouse = (key == OKEY_MWHEELDOWN);

    switch (platform)
    {
    case PF_WII:
        return mouse;
    case PF_XBOX:
        return (mouse); // TODO : Need to be RIGHT
    case PF_SWITCH:
        return (key == OKEY_JOY15);   // DPAD-RIGHT
    default:
        break;
    }

    return (mouse || key == OKEY_JOY15);
}

bool Key_IsSpyPrevKey(int key)
{
    // Default Keyboard press
    bool mouse = (key == OKEY_MWHEELUP);

    switch (platform)
    {
    case PF_WII:
        return mouse;
    case PF_XBOX:
        return (mouse); // TODO : Need to be LEFT
    case PF_SWITCH:
        return (key == OKEY_JOY13);   // DPAD-LEFT
    default:
        break;
    }

    return (mouse || key == OKEY_JOY14);
} 

bool Key_IsTabulationKey(int key)
{
    bool keyboard = (key == OKEY_TAB);

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
        return OKEY_JOY3;
    default:
        break;
    }
    return false;
}
#endif
*/
