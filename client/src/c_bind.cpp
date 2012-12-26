// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
// Copyright (C) 2006-2012 by The Odamex Team.
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
//	C_BIND
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "doomtype.h"
#include "doomdef.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "g_level.h"
#include "gstrings.h"
#include "hu_stuff.h"
#include "cl_demo.h"
#include "d_player.h"

extern NetDemo netdemo;

/* Most of these bindings are equivalent
 * to the original DOOM's keymappings.
 */
char DefBindings[] =
	"bind grave toggleconsole; "			// <- This is new
	"bind 1 \"impulse 1\"; "
	"bind 2 \"impulse 2\"; "
	"bind 3 \"impulse 3\"; "
	"bind 4 \"impulse 4\"; "
	"bind 5 \"impulse 5\"; "
	"bind 6 \"impulse 6\"; "
	"bind 7 \"impulse 7\"; "
	"bind 8 \"impulse 8\"; "
	"bind - sizedown; "
	"bind = sizeup; "
	"bind leftctrl +attack; "
	"bind leftalt +strafe; "
	"bind leftshift +speed; "
	"bind space +use; "
	"bind rightarrow +right; "
	"bind leftarrow +left; "
	"bind uparrow +forward; "
	"bind downarrow +back; "
	"bind , +moveleft; "
	"bind . +moveright; "
#ifdef _XBOX // Alternative defaults for Xbox
	"bind hat1right messagemode2; "
	"bind hat1left spynext; "
	"bind hat1up messagemode; "
	"bind hat1down \"impulse 3\"; "
	"bind joy1 +use; "
	"bind joy2 weapnext; "
	"bind joy3 +jump; "
	"bind joy4 weapprev; "
	"bind joy5 togglemap; "
	"bind joy6 +showscores; "
	"bind joy7 +speed; "
	"bind joy8 +attack; "
	"bind joy10 toggleconsole; "
	"bind joy12 centerview; "
#else
	"bind mouse1 +attack; "
	"bind mouse2 +strafe; "
	"bind mouse3 +forward; "
	"bind mouse4 +jump; "				// <- So is this <- change to jump
	"bind mouse5 +speed; "				// <- new for +speed
	"bind joy1 +attack; "
	"bind joy2 +strafe; "
	"bind joy3 +speed; "
	"bind joy4 +use; "
#endif
	"bind capslock \"toggle cl_run\"; "	// <- This too
	"bind f1 menu_help; "
	"bind f2 menu_save; "
	"bind f3 menu_load; "
	"bind f4 menu_options; "			// <- Since we don't have a separate sound menu anymore
	"bind f5 menu_display; "			// <- More useful than just changing the detail level
	"bind f6 quicksave; "
	"bind f7 menu_endgame; "
	"bind f8 togglemessages; "
	"bind f9 quickload; "
	"bind f10 menu_quit; "
	"bind tab togglemap; "
	"bind pause pause; "
	"bind sysrq screenshot; "			// <- Also known as the Print Screen key
	"bind t messagemode; "
	"bind y messagemode2; "
	"bind \\\\ +showscores; "				// <- Another new command
	"bind f11 bumpgamma; "
	"bind f12 spynext; "
	"bind pgup vote_yes; "				// <- New for voting
	"bind pgdn vote_no; "				// <- New for voting
	"bind home ready; "
	"bind end spectate; "
	"bind m changeteams ";


// SoM: ok... I hate randy heit I have no idea how to translate between ascii codes to these
// so I get to re-write the entire system. YES I RE-DID ALL OF THIS BY HAND
// SoM: note: eat shit and die Randy Heit this uses SDL key codes now!
const char *KeyNames[NUM_KEYS] = {
// :: Begin ASCII mapped keycodes
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 00 - 07
"backspace","tab",   NULL,    NULL,    NULL, "enter",    NULL,    NULL, // 08 - 0F
   NULL,    NULL,    NULL, "pause",    NULL,    NULL,    NULL,    NULL, // 10 - 17
   NULL,    NULL,    NULL,"escape",    NULL,    NULL,    NULL,    NULL, // 18 - 1F
"space",     "!",    "\"",     "#",     "$",    NULL,     "&",     "'", // 20 - 27
    "(",     ")",     "*",     "+",     ",",     "-",     ".",     "/", // 28 - 2F
    "0",     "1",     "2",     "3",     "4",     "5",     "6",     "7", // 30 - 37
    "8",     "9",     ":",     ";",     "<",     "=",     ">",     "?", // 38 - 3F
    "@",    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 40 - 47
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 48 - 4F
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 50 - 57
   NULL,    NULL,    NULL,     "[",    "\\",     "]",     "^",     "_", // 58 - 5F
"grave",     "a",     "b",     "c",     "d",     "e",     "f",     "g", // 60 - 67
    "h",     "i",     "j",     "k",     "l",     "m",     "n",     "o", // 68 - 6F
    "p",     "q",     "r",     "s",     "t",     "u",     "v",     "w", // 70 - 77
    "x",     "y",     "z",    NULL,    NULL,    NULL,    NULL,   "del", // 78 - 7F
// :: End ASCII mapped keycodes
// :: Begin World keycodes
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 80 - 87
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 88 - 8F
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 90 - 97
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // 98 - 9F
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // A0 - A7
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // A8 - AF
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // B0 - B7
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // B8 - BF
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // C0 - C7
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // C8 - CF
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // D0 - D7
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // D8 - DF
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // E0 - E7
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // E8 - EF
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // F0 - F7
   NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL,    NULL, // F8 - FF
// :: End world keycodes
  "kp0",   "kp1",   "kp2",   "kp3",   "kp4",   "kp5",   "kp6",   "kp7", // 0100 - 0107
  "kp8",   "kp9",   "kp.",   "kp/",   "kp*",   "kp-",   "kp+", "kpenter", // 0108 - 010F
  "kp=","uparrow","downarrow","rightarrow","leftarrow","ins","home","end", // 0110 - 0117
 "pgup",  "pgdn",    "f1",    "f2",    "f3",    "f4",    "f5",    "f6", // 0118 - 011F
   "f7",    "f8",    "f9",   "f10",   "f11",   "f12",   "f13",   "f14", // 0120 - 0127
  "f15",    NULL,    NULL,    NULL,"numlock","capslock","scroll", "rightshift", // 0128 - 012F
"leftshift", "rightctrl", "leftctrl", "rightalt", "leftalt",    NULL,    NULL,  "lwin", // 0130 - 0137
 "rwin",    NULL,    NULL,  "help", "print", "sysrq", "break",    NULL,  // 0138 - 013F
   NULL,    NULL,    NULL,    // 0140 - 0142

	// non-keyboard buttons that can be bound
   // 0143 - 0146 & 0173
	"mouse1",	"mouse2",	"mouse3",	"mouse4",			// 5 mouse buttons
   // 0147 - 014A
   "mwheelup",	"mwheeldown",NULL,		NULL,			// the wheel and some extra space
   // 014B - 014E
	"joy1",		"joy2",		"joy3",		"joy4",			// 32 joystick buttons
   // 014F - 0152
	"joy5",		"joy6",		"joy7",		"joy8",
   // 0153 - 0156
	"joy9",		"joy10",	"joy11",	"joy12",
   // 0157 - 015A
	"joy13",	"joy14",	"joy15",	"joy16",
   // 015B - 015E
	"joy17",	"joy18",	"joy19",	"joy20",
   // 015F - 0162
	"joy21",	"joy22",	"joy23",	"joy24",
   // 0163 - 0166
	"joy25",	"joy26",	"joy27",	"joy28",
   // 0167 - 016A
	"joy29",	"joy30",	"joy31",	"joy32",
  // 016B - 016E
	"hat1up",	"hat1right","hat1down",	"hat1left",
  // 016F - 0172
	"hat2up",	"hat2right","hat2down",	"hat2left", "mouse5"
};

static std::string Bindings[NUM_KEYS];
static std::string DoubleBindings[NUM_KEYS];
static std::string NetDemoBindings[NUM_KEYS];
static int DClickTime[NUM_KEYS];
static byte DClicked[(NUM_KEYS+7)/8];

static int GetKeyFromName (const char *name)
{
	int i;

	// Names of the form #xxx are translated to key xxx automatically
	if (name[0] == '#' && name[1] != 0) {
		return atoi (name + 1);
	}

	// Otherwise, we scan the KeyNames[] array for a matching name
	for (i = 0; i < NUM_KEYS; i++) {
		if (KeyNames[i] && !stricmp (KeyNames[i], name))
			return i;
	}
	return 0;
}

static const char *KeyName (int key)
{
	static char name[5];

	if (KeyNames[key])
		return KeyNames[key];

	sprintf (name, "#%d", key);
	return name;
}

BEGIN_COMMAND (unbindall)
{
	int i;

	for (i = 0; i < NUM_KEYS; i++)
		Bindings[i] = "";

	for (i = 0; i < NUM_KEYS; i++)
		DoubleBindings[i] = "";
}
END_COMMAND (unbindall)

BEGIN_COMMAND (unbind)
{
	int i;

	if (argc > 1)
	{
		if ( (i = GetKeyFromName (argv[1])) )
			Bindings[i] = "";
		else
			Printf (PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
	}
}
END_COMMAND (unbind)

BEGIN_COMMAND (bind)
{
	int i;

	if (argc > 1) {
		i = GetKeyFromName (argv[1]);
		if (!i) {
			Printf (PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
			return;
		}
		if (argc == 2) {
			Printf (PRINT_HIGH, "%s = %s\n", argv[1], C_QuoteString(Bindings[i]).c_str());
		} else {
			Bindings[i] = argv[2];
		}
	} else {
		Printf (PRINT_HIGH, "Current key bindings:\n");

		for (i = 0; i < NUM_KEYS; i++) {
			if (Bindings[i].length())
				Printf (PRINT_HIGH, "%s %s\n", KeyName(i), C_QuoteString(Bindings[i]).c_str());
		}
	}
}
END_COMMAND (bind)

BEGIN_COMMAND (undoublebind)
{
	int i;

	if (argc > 1)
	{
		if ( (i = GetKeyFromName (argv[1])) )
			DoubleBindings[i] = "";
		else
			Printf (PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
	}
}
END_COMMAND (undoublebind)

BEGIN_COMMAND (doublebind)
{
	int i;

	if (argc > 1)
	{
		i = GetKeyFromName (argv[1]);
		if (!i)
		{
			Printf (PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
			return;
		}
		if (argc == 2)
		{
			Printf (PRINT_HIGH, "%s = %s\n", argv[1], C_QuoteString(DoubleBindings[i]).c_str());
		}
		else
		{
			DoubleBindings[i] = argv[2];
		}
	}
	else
	{
		Printf (PRINT_HIGH, "Current key doublebindings:\n");

		for (i = 0; i < NUM_KEYS; i++)
		{
			if (DoubleBindings[i].length())
				Printf (PRINT_HIGH, "%s %s\n", KeyName(i), C_QuoteString(DoubleBindings[i]).c_str());
		}
	}
}
END_COMMAND (doublebind)

BEGIN_COMMAND (binddefaults)
{
	AddCommandString (DefBindings);
}
END_COMMAND (binddefaults)

//
// C_DoNetDemoKey
//
// [SL] 2012-03-29 - Handles the hard-coded key bindings used during
// NetDemo playback.  Returns false if the key pressed is not
// bound to any netdemo command.
//
bool C_DoNetDemoKey (event_t *ev)
{
	if (!netdemo.isPlaying() && !netdemo.isPaused())
		return false;

	static bool initialized = false;

	if (!initialized)
	{
		NetDemoBindings[GetKeyFromName("leftarrow")]	= "netrew";
		NetDemoBindings[GetKeyFromName("rightarrow")]	= "netff";
		NetDemoBindings[GetKeyFromName("uparrow")]		= "netprevmap";
		NetDemoBindings[GetKeyFromName("downarrow")]	= "netnextmap";
		NetDemoBindings[GetKeyFromName("space")]		= "netpause";

		initialized = true;
	}

	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	std::string *binding = &NetDemoBindings[ev->data1];
	
	// nothing bound to this key specific to netdemos?
	if (binding->empty())
		return false;

	if (ev->type == ev_keydown)
		AddCommandString(binding->c_str());

	return true;
}

//
// C_DoSpectatorKey
//
// [SL] 2012-09-14 - Handles the hard-coded key bindings used while spectating
// or during NetDemo playback.  Returns false if the key pressed is not
// bound to any spectating command such as spynext.
//
bool C_DoSpectatorKey (event_t *ev)
{
	if (!consoleplayer().spectator && !netdemo.isPlaying() && !netdemo.isPaused())
		return false;

	if (ev->type == ev_keydown && ev->data1 == KEY_MWHEELUP)
	{
		AddCommandString("spyprev");
		return true;
	}
	if (ev->type == ev_keydown && ev->data1 == KEY_MWHEELDOWN)
	{
		AddCommandString("spynext");
		return true;
	}

	return false;	
}

BOOL C_DoKey (event_t *ev)
{
	std::string *binding;
	int dclickspot;
	byte dclickmask;

	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	dclickspot = ev->data1 >> 3;
	dclickmask = 1 << (ev->data1 & 7);

	if (DClickTime[ev->data1] > level.time && ev->type == ev_keydown) {
		// Key pressed for a double click
		binding = &DoubleBindings[ev->data1];
		DClicked[dclickspot] |= dclickmask;
	} else {
		if (ev->type == ev_keydown) {
			// Key pressed for a normal press
			binding = &Bindings[ev->data1];
			DClickTime[ev->data1] = level.time + 20;
		} else if (DClicked[dclickspot] & dclickmask) {
			// Key released from a double click
			binding = &DoubleBindings[ev->data1];
			DClicked[dclickspot] &= ~dclickmask;
			DClickTime[ev->data1] = 0;
		} else {
			// Key released from a normal press
			binding = &Bindings[ev->data1];
		}
	}

	if (!binding->length())
		binding = &Bindings[ev->data1];

	if (binding->length() && (headsupactive == 0 || ev->data1 < 256))
	{
		if (ev->type == ev_keydown)
		{
			AddCommandString (binding->c_str());
		}
		else
		{
			size_t achar = binding->find_first_of('+');

			if (achar == std::string::npos)
				return false;

			if (achar == 0 || (*binding)[achar - 1] <= ' ')
			{
				(*binding)[achar] = '-';
				AddCommandString (binding->c_str());
				(*binding)[achar] = '+';
			}
		}
		return true;
	}
	return false;
}

void C_ArchiveBindings (FILE *f)
{
	int i;

	fprintf (f, "unbindall\n");
	for (i = 0; i < NUM_KEYS; i++)
	{
		if (Bindings[i].length())
			fprintf (f, "bind %s %s\n",
					C_QuoteString(KeyName(i)).c_str(),
					C_QuoteString(Bindings[i]).c_str());
	}
	for (i = 0; i < NUM_KEYS; i++)
	{
		if (DoubleBindings[i].length())
			fprintf (f, "doublebind %s %s\n", 
					C_QuoteString(KeyName(i)).c_str(),
					C_QuoteString(DoubleBindings[i]).c_str());
	}
}

int C_GetKeysForCommand (const char *cmd, int *first, int *second)
{
	int c, i;

	*first = *second = c = i = 0;

	while (i < NUM_KEYS && c < 2) {
		if (Bindings[i].length() && !stricmp (cmd, Bindings[i].c_str())) {
			if (c++ == 0)
				*first = i;
			else
				*second = i;
		}
		i++;
	}
	return c;
}

std::string C_NameKeys (int first, int second)
{
	if(!first && !second)
		return "???";

	std::string out;

	if(first)
	{
		out += KeyName(first);
		if(second)out += " or ";
	}

	if(second)
	{
		out += KeyName(second);
	}

	return out;
}

void C_UnbindACommand (const char *str)
{
	int i;

	for (i = 0; i < NUM_KEYS; i++) {
		if (Bindings[i].length() && !stricmp (str, Bindings[i].c_str())) {
			Bindings[i] = "";
		}
	}
}

void C_ChangeBinding (const char *str, int newone)
{
	// Check which bindings that are already set. If both binding slots are taken,
	// erase all bindings and reassign the new one and the secondary binding to the key instead.
	int first = -1;
	int second = -1;

	C_GetKeysForCommand(str, &first, &second);

	if (newone == first || newone == second)
	{
		return;
	}
	else if (first > -1 && second > -1)
	{
		C_UnbindACommand(str);
		Bindings[newone] = str;
		Bindings[second] = str;
	}
	else
	{
		Bindings[newone] = str;
	}
}

const char *C_GetBinding (int key)
{
	return Bindings[key].c_str();
}

VERSION_CONTROL (c_bind_cpp, "$Id$")

