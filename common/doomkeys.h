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
#define KEY_F13 				0x126
#define KEY_F14 				0x127
#define KEY_F15 				0x128

#define KEY_BACKSPACE		0x08
#define KEY_PAUSE				0x14

#define KEY_EQUALS			0x3D
#define KEY_MINUS				0x2D

#define KEY_LSHIFT			0x130
#define KEY_LCTRL				0x132
#define KEY_LALT				0x134

#define KEY_RSHIFT         0x12F
#define KEY_RCTRL				0x131
#define KEY_RALT				0x133
#define KEY_LWIN				0x137
#define KEY_RWIN				0x138
#define KEY_HELP				0x13B
#define KEY_PRINT				0x13C
#define KEY_SYSRQ				0x13D	
#define KEY_BREAK				0x13E

#define KEY_INS 				0x115
#define KEY_DEL 				0x7F
#define KEY_END 				0x117
#define KEY_HOME				0x116
#define KEY_PGUP				0x118
#define KEY_PGDN			0x119

// new keys:

#define KEY_CAPSLOCK    (0x80 | 0x3A)
#define KEY_NUMLOCK     (0x80 | 0x45)
#define KEY_SCRLCK      (0x80 | 0x46)

#define KEYP_0          0x100
#define KEYP_1          0x101
#define KEYP_2          0x102
#define KEYP_3          0x103
#define KEYP_4          0x104
#define KEYP_5          0x105
#define KEYP_6          0x106
#define KEYP_7          0x107
#define KEYP_8          0x108
#define KEYP_9          0x109

#define KEYP_DIVIDE     0x10B
#define KEYP_PLUS       0x10E
#define KEYP_MINUS      0x10D
#define KEYP_MULTIPLY   0x10C
#define KEYP_PERIOD     0x10A
#define KEYP_EQUALS     0x110
#define KEYP_ENTER      0x10F

// Joystick and mouse buttons are now sent
// in ev_keyup and ev_keydown instead of
// ev_mouse and ev_joystick. This makes
// binding commands to them *much* simpler.

#define KEY_MOUSE1				0x143
#define KEY_MOUSE2				0x144
#define KEY_MOUSE3				0x145
#define KEY_MOUSE4				0x146
#define KEY_MOUSE5				0x173 //[Xyltol 07/21/2011] - Add Mouse4 and Mouse5 support (MOUSE4 was defined but didn't actually work, heh).
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
#define NUM_KEYS				0x174


#define SCANCODE_TO_KEYS_ARRAY {                                                            \
	0, 0, 0, 0, 'a',                                                        /* 0 - 9 */     \
	'b', 'c', 'd', 'e', 'f',                                                                \
	'g', 'h', 'i', 'j', 'k',                                                /* 10 - 19 */   \
	'l', 'm', 'n', 'o', 'p',                                                                \
	'q', 'r', 's', 't', 'u',                                                /* 20 - 29 */   \
	'v', 'w', 'x', 'y', 'z',                                                                \
	'1', '2', '3', '4', '5',                                                /* 30 - 39 */   \
	'6', '7', '8', '9', '0',                                                                \
	KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB, KEY_SPACE,               /* 40 - 49 */   \
	KEY_MINUS, KEY_EQUALS, '[', ']', '\\',                                                  \
	'#', ';', '\'', '`', ',',                                               /* 50 - 59 */   \
	'.', '/', KEY_CAPSLOCK, KEY_F1, KEY_F2,                                                 \
	KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7,                                 /* 60 - 69 */   \
	KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,                                              \
	KEY_PRINT, KEY_SCRLCK, KEY_PAUSE, KEY_INS, KEY_HOME,                    /* 70 - 79 */   \
	KEY_PGUP, KEY_DEL, KEY_END, KEY_PGDN, KEY_RIGHTARROW,                                   \
	KEY_LEFTARROW, KEY_DOWNARROW, KEY_UPARROW, KEY_NUMLOCK, KEYP_DIVIDE,    /* 80 - 89 */   \
	KEYP_MULTIPLY, KEYP_MINUS, KEYP_PLUS, KEYP_ENTER, KEYP_1,                               \
	KEYP_2, KEYP_3, KEYP_4, KEYP_5, KEYP_6,                                 /* 90 - 99 */   \
	KEYP_7, KEYP_8, KEYP_9, KEYP_0, KEYP_PERIOD,                                            \
	0, 0, 0, KEYP_EQUALS, KEY_F13,                                          /* 100 - 109 */ \
	KEY_F14, KEY_F15, 0, 0, 0,                                                              \
	0, 0, 0, 0, 0,                                                          /* 110 - 119 */ \
	0, 0, KEY_HELP, 0, 0,                                                                   \
	0, 0, 0, 0, 0,                                                          /* 120 - 129 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 130 - 139 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 140 - 149 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, KEY_SYSRQ,                                                  /* 150 - 159 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 160 - 169 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 170 - 179 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 180 - 189 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 190 - 199 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 200 - 209 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 210 - 219 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, KEY_LCTRL,                                                  /* 220 - 229 */ \
	KEY_LSHIFT, KEY_LALT, KEY_LWIN, KEY_RCTRL, KEY_RSHIFT,                                  \
	KEY_RALT, KEY_RWIN, 0, 0, 0,                                            /* 230 - 239 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 240 - 249 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 250 - 259 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 260 - 269 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 270 - 279 */ \
	0, 0, 0, 0, 0,                                                                          \
	0, 0, 0, 0, 0,                                                          /* 280 - 286 */ \
	0, 0                                                                                    \
}


#endif	// __DOOMKEYS_H__
