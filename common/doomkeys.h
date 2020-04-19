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
//  Internally used data structures for key definitions
//
//-----------------------------------------------------------------------------

#ifndef __DOOMKEYS_H__
#define __DOOMKEYS_H__

//
// DOOM keyboard definition.
// Most key data are simple ascii (uppercased).
//

// These map directly to SDL 2.0 keycodes
#define KEY_BACKSPACE       0x00000008
#define KEY_CAPSLOCK        0x40000039
#define KEY_DEL             0x0000007F
#define KEY_DOWNARROW       0x40000051
#define KEY_END             0x4000004D
#define KEY_ENTER           0x0000000D
#define KEY_EQUALS          0x0000003D
#define KEY_ESCAPE          0x0000001B
#define KEY_F1              0x4000003A
#define KEY_F2              0x4000003B
#define KEY_F3              0x4000003C
#define KEY_F4              0x4000003D
#define KEY_F5              0x4000003E
#define KEY_F6              0x4000003F
#define KEY_F7              0x40000040
#define KEY_F8              0x40000041
#define KEY_F9              0x40000042
#define KEY_F10             0x40000043
#define KEY_F11             0x40000044
#define KEY_F12             0x40000045
#define KEY_F13             0x40000068
#define KEY_F14             0x40000069
#define KEY_F15             0x4000006A
#define KEY_HELP            0x40000075
#define KEY_HOME            0x4000004A
#define KEY_INS             0x40000049
#define KEY_LALT            0x400000E2
#define KEY_LCTRL           0x400000E0
#define KEY_LEFTARROW       0x40000050
#define KEY_LSHIFT          0x400000E1
#define KEY_LWIN            0x400000E3
#define KEY_MINUS           0x0000002D
#define KEY_NUMLOCK         0x40000053
#define KEY_PAUSE           0x40000048
#define KEY_PGDN            0x4000004E
#define KEY_PGUP            0x4000004B
#define KEY_PRINT           0x40000046
#define KEY_RALT            0x400000E6
#define KEY_RCTRL           0x400000E4
#define KEY_RIGHTARROW      0x4000004F
#define KEY_RSHIFT          0x400000E5
#define KEY_RWIN            0x400000E7
#define KEY_SCRLCK          0x40000047
#define KEY_SPACE           0x00000020
#define KEY_SYSRQ           0x4000009A
#define KEY_TAB             0x00000009
#define KEY_UPARROW         0x40000052
#define KEYP_0              0x40000062
#define KEYP_1              0x40000059
#define KEYP_2              0x4000005A
#define KEYP_3              0x4000005B
#define KEYP_4              0x4000005C
#define KEYP_5              0x4000005D
#define KEYP_6              0x4000005E
#define KEYP_7              0x4000005F
#define KEYP_8              0x40000060
#define KEYP_9              0x40000061
#define KEYP_DIVIDE         0x40000054
#define KEYP_ENTER          0x40000058
#define KEYP_EQUALS         0x40000067
#define KEYP_MINUS          0x40000056
#define KEYP_MULTIPLY       0x40000055
#define KEYP_PERIOD         0x40000063
#define KEYP_PLUS           0x40000057


// These are custom to Odamex and not SDL 2.0 keycodes
// Mouse and joystick button presses are mapped to their own keycodes.
#define KEY_MOUSE1          0x20000000
#define KEY_MOUSE2          0x20000001
#define KEY_MOUSE3          0x20000002
#define KEY_MOUSE4          0x20000003
#define KEY_MOUSE5          0x20000004
#define KEY_MWHEELDOWN      0x20000005
#define KEY_MWHEELUP        0x20000006
#define KEY_JOY1            0x20000010
#define KEY_JOY2            0x20000011
#define KEY_JOY3            0x20000012
#define KEY_JOY4            0x20000013
#define KEY_JOY5            0x20000014
#define KEY_JOY6            0x20000015
#define KEY_JOY7            0x20000016
#define KEY_JOY8            0x20000017
#define KEY_JOY9            0x20000018
#define KEY_JOY10           0x20000019
#define KEY_JOY11           0x2000001A
#define KEY_JOY12           0x2000001B
#define KEY_JOY13           0x2000001C
#define KEY_JOY14           0x2000001D
#define KEY_JOY15           0x2000001E
#define KEY_JOY16           0x2000001F
#define KEY_JOY17           0x20000020
#define KEY_JOY18           0x20000021
#define KEY_JOY19           0x20000022
#define KEY_JOY20           0x20000023
#define KEY_JOY21           0x20000024
#define KEY_JOY22           0x20000025
#define KEY_JOY23           0x20000026
#define KEY_JOY24           0x20000027
#define KEY_JOY25           0x20000028
#define KEY_JOY26           0x20000029
#define KEY_JOY27           0x2000002A
#define KEY_JOY28           0x2000002B
#define KEY_JOY29           0x2000002C
#define KEY_JOY30           0x2000002D
#define KEY_JOY31           0x2000002E
#define KEY_JOY32           0x2000002F
#define KEY_HAT1            0x20000030
#define KEY_HAT2            0x20000031
#define KEY_HAT3            0x20000032
#define KEY_HAT4            0x20000033
#define KEY_HAT5            0x20000034
#define KEY_HAT6            0x20000035
#define KEY_HAT7            0x20000036
#define KEY_HAT8            0x20000037

#endif	// __DOOMKEYS_H__