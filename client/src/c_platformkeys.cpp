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
#include "c_platformkeys.h"
#include "i_input.h"

Platform_Responder keypress;

//
// Platform_Responder::IsUpKey
//
bool Platform_Responder::IsUpKey(int key)
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY14);            
    }

    return (key == KEY_HAT1 || key == KEY_UPARROW || key == KEYP_8 );
}

//
// Platform_Responder::IsDownKey
//
bool Platform_Responder::IsDownKey(int key)
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY16);            
    }

    return (key == KEY_HAT3 || key == KEY_DOWNARROW || key == KEYP_2 );
}

//
// Platform_Responder::IsLeftKey
//
bool Platform_Responder::IsLeftKey(int key) 
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY13);            
    }
    
    return (key == KEY_HAT4 || key == KEY_LEFTARROW || key == KEYP_4 );
}

//
// Platform_Responder::IsRightKey
//
bool Platform_Responder::IsRightKey(int key) 
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY15);            
    }
    
    return (key == KEY_HAT2 || key == KEY_RIGHTARROW || key == KEYP_6 );
}

//
// Platform_Responder::IsEnterKey
//
bool Platform_Responder::IsEnterKey(int key) 
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY1);     
        case PF_WII:
            return (key == KEY_JOY10 || key == KEY_JOY1 || key == KEY_ENTER || key == KEYP_ENTER);
    }
    
    return (key == KEY_JOY1 || key == KEY_ENTER || key == KEYP_ENTER );
}

//
// Platform_Responder::IsReturnKey
//
bool Platform_Responder::IsReturnKey(int key) 
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY2);     
        case PF_WII:
            return (key == KEY_JOY11 || key == KEY_JOY1 || key == KEY_ESCAPE);
    }
    
    return (key == KEY_JOY2 || key == KEY_ESCAPE );
}

bool Platform_Responder::IsYesKey(int key)
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY4);     
        case PF_WII:
            return (key == KEY_JOY10 || key == KEY_JOY1);
    }
    
    return (key == KEY_JOY1 || key == KEY_ESCAPE /*|| toupper(key2) == 'Y'*/ );

}

bool Platform_Responder::IsNoKey(int key)
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY2);     
        case PF_WII:
            return (key == KEY_JOY11 || key == KEY_JOY2 || key == KEY_ESCAPE || toupper(key2) == 'N');
    }
    
    return (key == KEY_JOY2 || key == KEY_ESCAPE /*|| toupper(key2) == 'N'*/ );

}

bool Platform_Responder::IsMenuKey(int key)
{
    switch (platform) 
    {
        case PF_SWITCH:
            return (key == KEY_JOY11);    // (+)
        case PF_WII:
            if (I_WhatWiiController() == WIICTRL_WIIMOTE)
                return (key == KEY_ESCAPE || key == KEY_JOY7 || key == KEY_JOY19 ); // (HOME on Wiimote | START - Pro Controller)
            else if (I_WhatWiiController() == WIICTRL_GAMECUBE)
                return (key == KEY_ESCAPE || key == KEY_JOY7);                      // Start
        break;
        case PF_XBOX:
            return (key == KEY_ESCAPE || key == KEY_JOY9);
    }
    
    return (key == KEY_ESCAPE);
}