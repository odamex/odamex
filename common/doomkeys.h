// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: doomdef.h 1166 2008-10-07 22:42:10Z Nes $
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Copyright (C) 2006-2010 by The Odamex Team.
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
// This is the stuff configured by Setup.Exe.
// Most key data are simple ascii (uppercased).
//
// SoM: YES I RE-DID ALL OF THIS BY HAND
#define KEY_RIGHTARROW     0x113
#define KEY_LEFTARROW      0x114
#define KEY_UPARROW        0x111
#define KEY_DOWNARROW      0x112
#define KEY_ESCAPE         0x1B
#define KEY_ENTER          0x0D
#define KEY_SPACE          0x20
#define KEY_TAB 				0x09
#define KEY_F1					0x11A
#define KEY_F2					0x11B
#define KEY_F3					0x11C
#define KEY_F4					0x11D
#define KEY_F5					0x11E
#define KEY_F6					0x11F
#define KEY_F7					0x120
#define KEY_F8					0x121
#define KEY_F9					0x122
#define KEY_F10 				0x123
#define KEY_F11 				0x124
#define KEY_F12 				0x125

#define KEY_BACKSPACE		0x08
#define KEY_PAUSE				0x14

#define KEY_EQUALS			0x3D
#define KEY_MINUS				0x2D

#define KEY_LSHIFT			0x130
#define KEY_LCTRL				0x132
#define KEY_LALT				0x134

#define KEY_RSHIFT         KEY_LSHIFT
#define KEY_RCTRL				KEY_LCTRL
#define KEY_RALT				KEY_LALT

#define KEY_INS 				0x115
#define KEY_DEL 				0x7F
#define KEY_END 				0x117
#define KEY_HOME				0x116
#define KEY_PGUP				0x118
#define KEY_PGDN				0x119

// new keys:

#define KEY_CAPSLOCK    (0x80+0x3a)
#define KEY_SCRLCK      (0x80+0x46)

#define KEYP_0          0
#define KEYP_1          KEY_END
#define KEYP_2          KEY_DOWNARROW
#define KEYP_3          KEY_PGDN
#define KEYP_4          KEY_LEFTARROW
#define KEYP_5          '5'
#define KEYP_6          KEY_RIGHTARROW
#define KEYP_7          KEY_HOME
#define KEYP_8          KEY_UPARROW
#define KEYP_9          KEY_PGUP

#define KEYP_DIVIDE     '/'
#define KEYP_PLUS       '+'
#define KEYP_MINUS      '-'
#define KEYP_MULTIPLY   '*'
#define KEYP_PERIOD     0
#define KEYP_EQUALS     KEY_EQUALS
#define KEYP_ENTER      KEY_ENTER

// Joystick and mouse buttons are now sent
// in ev_keyup and ev_keydown instead of
// ev_mouse and ev_joystick. This makes
// binding commands to them *much* simpler.

#define KEY_MOUSE1				0x143
#define KEY_MOUSE2				0x144
#define KEY_MOUSE3				0x145
#define KEY_MOUSE4				0x146
#define KEY_MWHEELUP          0x147
#define KEY_MWHEELDOWN        0x148

#define KEY_JOY1				0x14B
#define KEY_JOY2				0x14C
#define KEY_JOY3				0x14D
#define KEY_JOY4				0x14E
#define KEY_JOY5				0x14F
#define KEY_JOY6				0x150
#define KEY_JOY7				0x151
#define KEY_JOY8				0x152
#define KEY_JOY9				0x153
#define KEY_JOY10				0x154
#define KEY_JOY11				0x155
#define KEY_JOY12				0x156
#define KEY_JOY13				0x157
#define KEY_JOY14				0x158
#define KEY_JOY15				0x159
#define KEY_JOY16				0x15A
#define KEY_JOY17				0x15B
#define KEY_JOY18				0x15C
#define KEY_JOY19				0x15D
#define KEY_JOY20				0x15E
#define KEY_JOY21				0x15F
#define KEY_JOY22				0x160
#define KEY_JOY23				0x161
#define KEY_JOY24				0x162
#define KEY_JOY25				0x163
#define KEY_JOY26				0x164
#define KEY_JOY27				0x165
#define KEY_JOY28				0x166
#define KEY_JOY29				0x167
#define KEY_JOY30				0x168
#define KEY_JOY31				0x169
#define KEY_JOY32				0x16A
#define KEY_HAT1				0x16B
#define KEY_HAT2				0x16C
#define KEY_HAT3				0x16D
#define KEY_HAT4				0x16E
#define KEY_HAT5				0x16F
#define KEY_HAT6				0x170
#define KEY_HAT7				0x171
#define KEY_HAT8				0x172
#define NUM_KEYS				0x173

#endif	// __DOOMKEYS_H__
