// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1993-1996 by id Software, Inc.
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
//  Internally used data structures for key definitions
//
//-----------------------------------------------------------------------------

#pragma once

//
// DOOM keyboard definition.
// Most key data are simple ascii (uppercased).
//

// These map directly to SDL 2.0 keycodes
#define OKEY_BACKSPACE       0x00000008
#define OKEY_CAPSLOCK        0x40000039
#define OKEY_DEL             0x0000007F
#define OKEY_DOWNARROW       0x40000051
#define OKEY_END             0x4000004D
#define OKEY_ENTER           0x0000000D
#define OKEY_EQUALS          0x0000003D
#define OKEY_ESCAPE          0x0000001B
#define OKEY_TILDE           0x000000B2
#define OKEY_F1              0x4000003A
#define OKEY_F2              0x4000003B
#define OKEY_F3              0x4000003C
#define OKEY_F4              0x4000003D
#define OKEY_F5              0x4000003E
#define OKEY_F6              0x4000003F
#define OKEY_F7              0x40000040
#define OKEY_F8              0x40000041
#define OKEY_F9              0x40000042
#define OKEY_F10             0x40000043
#define OKEY_F11             0x40000044
#define OKEY_F12             0x40000045
#define OKEY_F13             0x40000068
#define OKEY_F14             0x40000069
#define OKEY_F15             0x4000006A
#define OKEY_HELP            0x40000075
#define OKEY_HOME			 0x4000004A
#define OKEY_INS             0x40000049
#define OKEY_LALT            0x400000E2
#define OKEY_LCTRL           0x400000E0
#define OKEY_LEFTARROW       0x40000050
#define OKEY_LSHIFT          0x400000E1
#define OKEY_LWIN            0x400000E3
#define OKEY_MINUS           0x0000002D
#define OKEY_NUMLOCK         0x40000053
#define OKEY_PAUSE           0x40000048
#define OKEY_PGDN            0x4000004E
#define OKEY_PGUP            0x4000004B
#define OKEY_PRINT           0x40000046
#define OKEY_RALT            0x400000E6
#define OKEY_RCTRL           0x400000E4
#define OKEY_RIGHTARROW		 0x4000004F
#define OKEY_RSHIFT          0x400000E5
#define OKEY_RWIN            0x400000E7
#define OKEY_SCRLCK          0x40000047
#define OKEY_SPACE           0x00000020
#define OKEY_SYSRQ           0x4000009A
#define OKEY_TAB             0x00000009
#define OKEY_UPARROW         0x40000052
#define OKEYP_0              0x40000062
#define OKEYP_1              0x40000059
#define OKEYP_2              0x4000005A
#define OKEYP_3              0x4000005B
#define OKEYP_4              0x4000005C
#define OKEYP_5              0x4000005D
#define OKEYP_6              0x4000005E
#define OKEYP_7              0x4000005F
#define OKEYP_8              0x40000060
#define OKEYP_9              0x40000061
#define OKEYP_DIVIDE         0x40000054
#define OKEYP_ENTER          0x40000058
#define OKEYP_EQUALS         0x40000067
#define OKEYP_MINUS          0x40000056
#define OKEYP_MULTIPLY       0x40000055
#define OKEYP_PERIOD         0x40000063
#define OKEYP_PLUS           0x40000057

// These are keymods that indicate what modifiers are engaged
// when the key events are made.
// These also link up to SDL2.0
#define OMOD_NONE						0x0000
#define OMOD_LSHIFT					0x0001
#define	OMOD_RSHIFT					0x0002
#define OMOD_LCTRL					0x0040
#define OMOD_RCTRL					0x0080
#define OMOD_LALT						0x0100
#define OMOD_RALT						0x0200
#define OMOD_LGUI						0x0400
#define OMOD_RGUI						0x0800
#define OMOD_NUM						0x1000
#define OMOD_CAPS						0x2000
#define OMOD_MODE						0x4000
#define OMOD_SCROLL					0x8000
#define OMOD_CTRL						(OMOD_LCTRL		| OMOD_RCTRL)
#define OMOD_SHIFT					(OMOD_LSHIFT	| OMOD_RSHIFT)
#define OMOD_ALT						(OMOD_LALT		| OMOD_RALT)
#define OMOD_GUI						(OMOD_LGUI		| OMOD_RGUI)
#define OMOD_RESERVED				OMOD_SCROLL /* This is for source-level compatibility with SDL 2.0.0. */

// These are custom to Odamex and not SDL 2.0 keycodes
// Mouse and joystick button presses are mapped to their own keycodes.
#define OKEY_MOUSE1          0x20000000
#define OKEY_MOUSE2          0x20000001
#define OKEY_MOUSE3          0x20000002
#define OKEY_MOUSE4          0x20000003
#define OKEY_MOUSE5          0x20000004
#define OKEY_MWHEELDOWN      0x20000005
#define OKEY_MWHEELUP        0x20000006
#define OKEY_JOY1            0x20000010
#define OKEY_JOY2            0x20000011
#define OKEY_JOY3            0x20000012
#define OKEY_JOY4            0x20000013
#define OKEY_JOY5            0x20000014
#define OKEY_JOY6            0x20000015
#define OKEY_JOY7            0x20000016
#define OKEY_JOY8            0x20000017
#define OKEY_JOY9            0x20000018
#define OKEY_JOY10           0x20000019
#define OKEY_JOY11           0x2000001A
#define OKEY_JOY12           0x2000001B
#define OKEY_JOY13           0x2000001C
#define OKEY_JOY14           0x2000001D
#define OKEY_JOY15           0x2000001E
#define OKEY_JOY16           0x2000001F
#define OKEY_JOY17           0x20000020
#define OKEY_JOY18           0x20000021
#define OKEY_JOY19           0x20000022
#define OKEY_JOY20           0x20000023
#define OKEY_JOY21           0x20000024
#define OKEY_JOY22           0x20000025
#define OKEY_JOY23           0x20000026
#define OKEY_JOY24           0x20000027
#define OKEY_JOY25           0x20000028
#define OKEY_JOY26           0x20000029
#define OKEY_JOY27           0x2000002A
#define OKEY_JOY28           0x2000002B
#define OKEY_JOY29           0x2000002C
#define OKEY_JOY30           0x2000002D
#define OKEY_JOY31           0x2000002E
#define OKEY_JOY32           0x2000002F
#define OKEY_HAT1            0x20000030
#define OKEY_HAT2            0x20000031
#define OKEY_HAT3            0x20000032
#define OKEY_HAT4            0x20000033
#define OKEY_HAT5            0x20000034
#define OKEY_HAT6            0x20000035
#define OKEY_HAT7            0x20000036
#define OKEY_HAT8			 0x20000037
