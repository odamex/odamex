// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_BIND
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "doomtype.h"
#include "doomdef.h"
#include "m_ostring.h"
#include "hashtable.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "g_level.h"
#include "gstrings.h"
#include "hu_stuff.h"
#include "cl_demo.h"
#include "d_player.h"

extern NetDemo netdemo;

FKeyBindings Bindings, DoubleBindings, NetDemoBindings;

/* Most of these bindings are equivalent
 * to the original DOOM's keymappings.
 */
FBinding DefaultBindings[] =
{
	{"grave", "toggleconsole"},		// <- This is new
	{"1", "impulse 1"},
	{"2", "impulse 2"},
	{"3", "impulse 3"},
	{"4", "impulse 4"},
	{"5", "impulse 5"},
	{"6", "impulse 6"},
	{"7", "impulse 7"},
	{"8", "impulse 8"},
	{"-", "sizedown"},
	{"+", "sizeup"},
	{"leftctrl", "+attack"},
	{"leftalt", "+strafe"},
	{"leftshift", "+speed"},
	{"rightshift", "+speed"},
	{"space", "+use"},
	{"e", "+use"},
	{"rightarrow", "+right"},
	{"leftarrow", "+left"},
	{"w", "+forward"},
	{"s", "+back"},
	{"a", "+moveleft"},
	{"d", "+moveright"},
#ifdef _XBOX
	{"hat1right", "messagemode2"},
	{"hat1left", "spynext"},
	{"hat1up", "messagemode"},
	{"hat1down", "impulse 3"},
	{"joy1", "+use"},
	{"joy2", "weapnext"},
	{"joy3", "+jump"},
	{"joy4", "weapprev"},
	{"joy5", "togglemap"},
	{"joy6", "+showscores"},
	{"joy7", "+speed"},
	{"joy8", "+attack"},
	{"joy10", "toggleconsole"},
	{"joy12", "centerview"},
#else
	{"mouse1", "+attack"},
	{"mouse2", "+strafe"},
	{"mouse3", "+forward"},
	{"mouse4", "+jump"},		// <- So is this <- change to jump
	{"mouse5", "+speed"},		// <- new for +speed
	{"joy1", "+attack"},
	{"joy2", "+strafe"},
	{"joy3", "+speed"},
	{"joy4", "+use"},
	{"mwheelup", "weapprev"},
	{"mwheeldown", "weapnext"},
#endif
	{"f1", "menu_help"},	
	{"f2", "menu_save"},
	{"f3", "menu_load"},
	{"f4", "menu_options"},		// <- Since we don't have a separate sound menu anymore
	{"f5", "menu_display"},		// <- More useful than just changing the detail level
	{"f6", "quicksave"},	
	{"f7", "menu_endgame"},	
	{"f8", "togglemessages"},
	{"f9", "quickload"},
	{"f10", "menu_quit"},
	{"tab", "togglemap"},
	{"pause", "pause"},
	{"sysrq", "screenshot"},	// <- Also known as the Print Screen key
	{"print", "screenshot"},	// <- AZERTY equivalent
	{"t", "messagemode"},
	{"enter", "messagemode"},
	{"y", "messagemode2"},	
	{"\\", "+showscores"},		// <- Another new command
	{"f11", "bumpgamma"},
	{"f12", "spynext"},
	{"pgup", "vote_yes"},		// <- New for voting
	{"pgdn", "vote_no"},		// <- New for voting
	{"home", "ready"},
	{"end", "spectate"},
	{"m", "changeteams"},
	{ NULL, NULL }
	//{"", ""},
};	

/* Special bindings when it comes 
 * to Odamex's demo playbacking.
 */
FBinding DefaultNetDemoBindings[]  =
{
	{"leftarrow", "netrew"},
	{"rightarrow", "netff"},
	{"uparrow", "netprevmap"},
	{"downarrow", "netnextmap"},
	{"space", "netpause"},
	{ NULL, NULL }
};

/* Special bindings when it comes
 * to Odamex's demo playbacking.
 */
FBinding DefaultDoubleBindings[] =
{
	{"a", "lol"},
	{ NULL, NULL }
};

static std::string AutoMapBindings[NUM_KEYS];	// Ch0wW : Addition of automap rebindings
static int DClickTime[NUM_KEYS];
static byte DClicked[(NUM_KEYS+7)/8];
static bool KeysDown[NUM_KEYS];

typedef OHashTable<OString, int> NameToKeyCodeTable;
static NameToKeyCodeTable nameToKeyCode = NameToKeyCodeTable();

typedef OHashTable<int, OString> KeyCodeToNameTable;
static KeyCodeToNameTable keyCodeToName = KeyCodeToNameTable();

static void buildKeyCodeTables()
{
	nameToKeyCode.clear();
	nameToKeyCode.insert(std::make_pair("backspace", KEY_BACKSPACE));
	nameToKeyCode.insert(std::make_pair("tab", KEY_TAB));
	nameToKeyCode.insert(std::make_pair("enter", KEY_ENTER));
	nameToKeyCode.insert(std::make_pair("pause", KEY_PAUSE));
	nameToKeyCode.insert(std::make_pair("escape", KEY_ESCAPE));
	nameToKeyCode.insert(std::make_pair("space", KEY_SPACE));
	nameToKeyCode.insert(std::make_pair("!", '!'));
	nameToKeyCode.insert(std::make_pair("\"", '\"'));
	nameToKeyCode.insert(std::make_pair("#", '#'));
	nameToKeyCode.insert(std::make_pair("$", '$'));
	nameToKeyCode.insert(std::make_pair("&", '&'));
	nameToKeyCode.insert(std::make_pair("\'", '\''));
	nameToKeyCode.insert(std::make_pair("(", '('));
	nameToKeyCode.insert(std::make_pair(")", ')'));
	nameToKeyCode.insert(std::make_pair("*", '*'));
	nameToKeyCode.insert(std::make_pair("+", '+'));
	nameToKeyCode.insert(std::make_pair(",", ','));
	nameToKeyCode.insert(std::make_pair("-", '-'));
	nameToKeyCode.insert(std::make_pair(".", '.'));
	nameToKeyCode.insert(std::make_pair("/", '/'));
	nameToKeyCode.insert(std::make_pair("0", '0'));
	nameToKeyCode.insert(std::make_pair("1", '1'));
	nameToKeyCode.insert(std::make_pair("2", '2'));
	nameToKeyCode.insert(std::make_pair("3", '3'));
	nameToKeyCode.insert(std::make_pair("4", '4'));
	nameToKeyCode.insert(std::make_pair("5", '5'));
	nameToKeyCode.insert(std::make_pair("6", '6'));
	nameToKeyCode.insert(std::make_pair("7", '7'));
	nameToKeyCode.insert(std::make_pair("8", '8'));
	nameToKeyCode.insert(std::make_pair("9", '9'));
	nameToKeyCode.insert(std::make_pair(":", ':'));
	nameToKeyCode.insert(std::make_pair(";", ';'));
	nameToKeyCode.insert(std::make_pair("<", '<'));
	nameToKeyCode.insert(std::make_pair("=", '='));
	nameToKeyCode.insert(std::make_pair(">", '>'));
	nameToKeyCode.insert(std::make_pair("?", '?'));
	nameToKeyCode.insert(std::make_pair("@", '@'));
	nameToKeyCode.insert(std::make_pair("[", '['));
	nameToKeyCode.insert(std::make_pair("\\", '\\'));
	nameToKeyCode.insert(std::make_pair("]", ']'));
	nameToKeyCode.insert(std::make_pair("^", '^'));
	nameToKeyCode.insert(std::make_pair("_", '_'));
	nameToKeyCode.insert(std::make_pair("grave", '`'));
	nameToKeyCode.insert(std::make_pair("a", 'a'));
	nameToKeyCode.insert(std::make_pair("b", 'b'));
	nameToKeyCode.insert(std::make_pair("c", 'c'));
	nameToKeyCode.insert(std::make_pair("d", 'd'));
	nameToKeyCode.insert(std::make_pair("e", 'e'));
	nameToKeyCode.insert(std::make_pair("f", 'f'));
	nameToKeyCode.insert(std::make_pair("g", 'g'));
	nameToKeyCode.insert(std::make_pair("h", 'h'));
	nameToKeyCode.insert(std::make_pair("i", 'i'));
	nameToKeyCode.insert(std::make_pair("j", 'j'));
	nameToKeyCode.insert(std::make_pair("k", 'k'));
	nameToKeyCode.insert(std::make_pair("l", 'l'));
	nameToKeyCode.insert(std::make_pair("m", 'm'));
	nameToKeyCode.insert(std::make_pair("n", 'n'));
	nameToKeyCode.insert(std::make_pair("o", 'o'));
	nameToKeyCode.insert(std::make_pair("p", 'p'));
	nameToKeyCode.insert(std::make_pair("q", 'q'));
	nameToKeyCode.insert(std::make_pair("r", 'r'));
	nameToKeyCode.insert(std::make_pair("s", 's'));
	nameToKeyCode.insert(std::make_pair("t", 't'));
	nameToKeyCode.insert(std::make_pair("u", 'u'));
	nameToKeyCode.insert(std::make_pair("v", 'v'));
	nameToKeyCode.insert(std::make_pair("w", 'w'));
	nameToKeyCode.insert(std::make_pair("x", 'x'));
	nameToKeyCode.insert(std::make_pair("y", 'y'));
	nameToKeyCode.insert(std::make_pair("z", 'z'));
	nameToKeyCode.insert(std::make_pair("kp0", KEYP_0));
	nameToKeyCode.insert(std::make_pair("kp1", KEYP_1));
	nameToKeyCode.insert(std::make_pair("kp2", KEYP_2));
	nameToKeyCode.insert(std::make_pair("kp3", KEYP_3));
	nameToKeyCode.insert(std::make_pair("kp4", KEYP_4));
	nameToKeyCode.insert(std::make_pair("kp5", KEYP_5));
	nameToKeyCode.insert(std::make_pair("kp6", KEYP_6));
	nameToKeyCode.insert(std::make_pair("kp7", KEYP_7));
	nameToKeyCode.insert(std::make_pair("kp8", KEYP_8));
	nameToKeyCode.insert(std::make_pair("kp9", KEYP_9));
	nameToKeyCode.insert(std::make_pair("kp.", KEYP_PERIOD));
	nameToKeyCode.insert(std::make_pair("kp/", KEYP_DIVIDE));
	nameToKeyCode.insert(std::make_pair("kp*", KEYP_MULTIPLY));
	nameToKeyCode.insert(std::make_pair("kp-", KEYP_MINUS));
	nameToKeyCode.insert(std::make_pair("kp+", KEYP_PLUS));
	nameToKeyCode.insert(std::make_pair("kpenter", KEYP_ENTER));
	nameToKeyCode.insert(std::make_pair("kp=", KEYP_EQUALS));
	nameToKeyCode.insert(std::make_pair("uparrow", KEY_UPARROW));
	nameToKeyCode.insert(std::make_pair("downarrow", KEY_DOWNARROW));
	nameToKeyCode.insert(std::make_pair("rightarrow", KEY_RIGHTARROW));
	nameToKeyCode.insert(std::make_pair("leftarrow", KEY_LEFTARROW));
	nameToKeyCode.insert(std::make_pair("ins", KEY_INS));
	nameToKeyCode.insert(std::make_pair("home", KEY_HOME));
	nameToKeyCode.insert(std::make_pair("end", KEY_END));
	nameToKeyCode.insert(std::make_pair("pgup", KEY_PGUP));
	nameToKeyCode.insert(std::make_pair("pgdn", KEY_PGDN));
	nameToKeyCode.insert(std::make_pair("f1", KEY_F1));
	nameToKeyCode.insert(std::make_pair("f2", KEY_F2));
	nameToKeyCode.insert(std::make_pair("f3", KEY_F3));
	nameToKeyCode.insert(std::make_pair("f4", KEY_F4));
	nameToKeyCode.insert(std::make_pair("f5", KEY_F5));
	nameToKeyCode.insert(std::make_pair("f6", KEY_F6));
	nameToKeyCode.insert(std::make_pair("f7", KEY_F7));
	nameToKeyCode.insert(std::make_pair("f8", KEY_F8));
	nameToKeyCode.insert(std::make_pair("f9", KEY_F9));
	nameToKeyCode.insert(std::make_pair("f10", KEY_F10));
	nameToKeyCode.insert(std::make_pair("f11", KEY_F11));
	nameToKeyCode.insert(std::make_pair("f12", KEY_F12));
	nameToKeyCode.insert(std::make_pair("f13", KEY_F13));
	nameToKeyCode.insert(std::make_pair("f14", KEY_F14));
	nameToKeyCode.insert(std::make_pair("f15", KEY_F15));
	nameToKeyCode.insert(std::make_pair("capslock", KEY_CAPSLOCK));
	nameToKeyCode.insert(std::make_pair("numlock", KEY_NUMLOCK));
	nameToKeyCode.insert(std::make_pair("scroll", KEY_SCRLCK));
	nameToKeyCode.insert(std::make_pair("rightshift", KEY_RSHIFT));
	nameToKeyCode.insert(std::make_pair("leftshift", KEY_LSHIFT));
	nameToKeyCode.insert(std::make_pair("rightctrl", KEY_RCTRL));
	nameToKeyCode.insert(std::make_pair("leftctrl", KEY_LCTRL));
	nameToKeyCode.insert(std::make_pair("rightalt", KEY_RALT));
	nameToKeyCode.insert(std::make_pair("leftalt", KEY_LALT));
	nameToKeyCode.insert(std::make_pair("lwin", KEY_LWIN));
	nameToKeyCode.insert(std::make_pair("rwin", KEY_RWIN));
	nameToKeyCode.insert(std::make_pair("help", KEY_HELP));
	nameToKeyCode.insert(std::make_pair("print", KEY_PRINT));
	nameToKeyCode.insert(std::make_pair("sysrq", KEY_SYSRQ));
	nameToKeyCode.insert(std::make_pair("break", KEY_BREAK));
	nameToKeyCode.insert(std::make_pair("mouse1", KEY_MOUSE1));
	nameToKeyCode.insert(std::make_pair("mouse2", KEY_MOUSE2));
	nameToKeyCode.insert(std::make_pair("mouse3", KEY_MOUSE3));
	nameToKeyCode.insert(std::make_pair("mouse4", KEY_MOUSE4));
	nameToKeyCode.insert(std::make_pair("mouse5", KEY_MOUSE5));
	nameToKeyCode.insert(std::make_pair("mwheelup", KEY_MWHEELUP));
	nameToKeyCode.insert(std::make_pair("mwheeldown", KEY_MWHEELDOWN));
	nameToKeyCode.insert(std::make_pair("joy1", KEY_JOY1));
	nameToKeyCode.insert(std::make_pair("joy2", KEY_JOY2));
	nameToKeyCode.insert(std::make_pair("joy3", KEY_JOY3));
	nameToKeyCode.insert(std::make_pair("joy4", KEY_JOY4));
	nameToKeyCode.insert(std::make_pair("joy5", KEY_JOY5));
	nameToKeyCode.insert(std::make_pair("joy6", KEY_JOY6));
	nameToKeyCode.insert(std::make_pair("joy7", KEY_JOY7));
	nameToKeyCode.insert(std::make_pair("joy8", KEY_JOY8));
	nameToKeyCode.insert(std::make_pair("joy9", KEY_JOY9));
	nameToKeyCode.insert(std::make_pair("joy10", KEY_JOY10));
	nameToKeyCode.insert(std::make_pair("joy11", KEY_JOY11));
	nameToKeyCode.insert(std::make_pair("joy12", KEY_JOY12));
	nameToKeyCode.insert(std::make_pair("joy13", KEY_JOY13));
	nameToKeyCode.insert(std::make_pair("joy14", KEY_JOY14));
	nameToKeyCode.insert(std::make_pair("joy15", KEY_JOY15));
	nameToKeyCode.insert(std::make_pair("joy16", KEY_JOY16));
	nameToKeyCode.insert(std::make_pair("joy17", KEY_JOY17));
	nameToKeyCode.insert(std::make_pair("joy18", KEY_JOY18));
	nameToKeyCode.insert(std::make_pair("joy19", KEY_JOY19));
	nameToKeyCode.insert(std::make_pair("joy20", KEY_JOY20));
	nameToKeyCode.insert(std::make_pair("joy21", KEY_JOY21));
	nameToKeyCode.insert(std::make_pair("joy22", KEY_JOY22));
	nameToKeyCode.insert(std::make_pair("joy23", KEY_JOY23));
	nameToKeyCode.insert(std::make_pair("joy24", KEY_JOY24));
	nameToKeyCode.insert(std::make_pair("joy25", KEY_JOY25));
	nameToKeyCode.insert(std::make_pair("joy26", KEY_JOY26));
	nameToKeyCode.insert(std::make_pair("joy27", KEY_JOY27));
	nameToKeyCode.insert(std::make_pair("joy28", KEY_JOY28));
	nameToKeyCode.insert(std::make_pair("joy29", KEY_JOY29));
	nameToKeyCode.insert(std::make_pair("joy30", KEY_JOY30));
	nameToKeyCode.insert(std::make_pair("joy31", KEY_JOY31));
	nameToKeyCode.insert(std::make_pair("joy32", KEY_JOY32));
	nameToKeyCode.insert(std::make_pair("hat1up", KEY_HAT1));
	nameToKeyCode.insert(std::make_pair("hat1right", KEY_HAT2));
	nameToKeyCode.insert(std::make_pair("hat1down", KEY_HAT3));
	nameToKeyCode.insert(std::make_pair("hat1left", KEY_HAT4));
	nameToKeyCode.insert(std::make_pair("hat2up", KEY_HAT5));
	nameToKeyCode.insert(std::make_pair("hat2right", KEY_HAT6));
	nameToKeyCode.insert(std::make_pair("hat2down", KEY_HAT7));
	nameToKeyCode.insert(std::make_pair("hat2left", KEY_HAT8));

	keyCodeToName.clear();
	for (NameToKeyCodeTable::const_iterator it = nameToKeyCode.begin(); it != nameToKeyCode.end(); ++it)
		keyCodeToName.insert(std::make_pair(it->second, it->first));
}

static int GetKeyFromName(const char *name)
{
	if (nameToKeyCode.empty())
		buildKeyCodeTables();

	// Names of the form #xxx are translated to key xxx automatically
	if (name[0] == '#' && name[1] != 0) {
		return atoi (name + 1);
	}

	NameToKeyCodeTable::const_iterator it = nameToKeyCode.find(name);
	if (it != nameToKeyCode.end())
		return it->second;
	return 0;
}

static const char* KeyName(int key)
{
	if (keyCodeToName.empty())
		buildKeyCodeTables();

	KeyCodeToNameTable::const_iterator it = keyCodeToName.find(key);
	if (it != keyCodeToName.end())
		return it->second.c_str();

	static char name[5];
	sprintf(name, "#%d", key);
	return name;
}

void FKeyBindings::SetBindingType(std::string cmd)
{
	command = cmd;
}

void FKeyBindings::DoBind(const char *key, const char *bind)
{
	int keynum = GetKeyFromName(key);
	if (keynum != 0)
	{
		this->Binds[keynum] = bind;
	}
}

void FKeyBindings::UnbindAll()
{
	for (int i = 0; i < NUM_KEYS; i++)
	{
		this->Binds[i] = "";
	}
}

void FKeyBindings::UnbindKey(const char *key)
{
	int i;

	if ((i = GetKeyFromName(key)))
	{
		this->Binds[i] = "";

		for (i = 0; i < NUM_KEYS; i++) {
			if (Bindings.Binds[i].length())
				Printf(PRINT_HIGH, "%s %s\n", KeyName(i), C_QuoteString(Bindings.Binds[i]).c_str());
		}

	}
	else
	{
		Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(key).c_str());
		return;
	}
}

size_t FKeyBindings::GetLength() {

	return Binds->length();
}

void FKeyBindings::ArchiveBindings(FILE *f)
{
	int i;

	for (i = 0; i < NUM_KEYS; i++)
	{
		if (Binds[i].length())
			fprintf(f, "%s %s %s\n",
				command.c_str(),
				C_QuoteString(KeyName(i)).c_str(),
				C_QuoteString(Binds[i]).c_str());
	}
}

void FKeyBindings::SetBinds(const FBinding *binds)
{
	while (binds->Key)
	{
		DoBind(binds->Key, binds->Bind);
		binds++;
	}
}

int FKeyBindings::GetKeysForCommand(const char *cmd, int *first, int *second)
{
	int c, i;

	*first = *second = c = i = 0;

	while (i < NUM_KEYS && c < 2) {
		if (Binds[i].length() && !stricmp(cmd, Binds[i].c_str())) {
			if (c++ == 0)
				*first = i;
			else
				*second = i;
		}
		i++;
	}
	return c;
}

void FKeyBindings::ChangeBinding(const char *str, int newone)
{
	// Check which bindings that are already set. If both binding slots are taken,
	// erase all bindings and reassign the new one and the secondary binding to the key instead.
	int first = -1;
	int second = -1;

	GetKeysForCommand(str, &first, &second);

	if (newone == first || newone == second)
	{
		return;
	}
	else if (first > -1 && second > -1)
	{
		UnbindACommand(str);
		Binds[newone] = str;
		Binds[second] = str;
	}
	else
	{
		Binds[newone] = str;
	}
}

const char* FKeyBindings::GetBinding(int key)
{
	return Binds[key].c_str();
}

std::string FKeyBindings::GetBind(int key)
{
	return Binds[key];
}

void FKeyBindings::SetBind(int key, char *value) {
	this->Binds[key] = value;
}

void FKeyBindings::UnbindACommand(const char *str)
{
	int i;
	
		for (i = 0; i < NUM_KEYS; i++) {
		if (Binds[i].length() && !stricmp(str, Binds[i].c_str())) {
			Binds[i] = "";	
		}
	}
}

/*
GetKeyStringsFromCommand
Finds binds from a command and returns it into a std::string .
- If TRUE, second arg returns up to 2 keys. ("x OR y")
*/
std::string FKeyBindings::GetKeyStringsFromCommand(char *cmd, bool bTwoEntries)
{
	int first = -1;
	int second = -1;

	GetKeysForCommand(cmd, &first, &second);

	if (!first && !second)
		return "<???>";

	if (bTwoEntries)
		return C_NameKeys(first, second);
	else
	{
		if (!first && second)
			return KeyName(second);
		else
			return KeyName(first);
	}
	return "<???>";
}

void FKeyBindings::BindAKey(size_t argc, char **argv, char *msg)
{
	int i;

	if (argc > 1)
	{
		i = GetKeyFromName(argv[1]);
		if (!i)
		{
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
			return;
		}
		if (argc == 2)
		{
			Printf(PRINT_HIGH, "%s = %s\n", argv[1], C_QuoteString(Binds[i]).c_str());
		}
		else
		{
			Binds[i] = argv[2];
		}
	}
	else
	{

		Printf(PRINT_HIGH, "%s\n", msg);

		for (i = 0; i < NUM_KEYS; i++)
		{
			if (Binds[i].length())
				Printf(PRINT_HIGH, "%s %s\n", KeyName(i), C_QuoteString(Binds[i]).c_str());
		}
	}
}

BEGIN_COMMAND(unbindall)
{
	Bindings.UnbindAll();
	DoubleBindings.UnbindAll();
	NetDemoBindings.UnbindAll();
}
END_COMMAND(unbindall)

BEGIN_COMMAND(unbind)
{
	if (argc > 1)
	{
		Bindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(unbind)

BEGIN_COMMAND(bind)
{
	Bindings.BindAKey(argc, argv, "Current key bindings");
}
END_COMMAND(bind)

BEGIN_COMMAND(undoublebind)
{
	if (argc > 1)
	{
		DoubleBindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(undoublebind)

BEGIN_COMMAND(doublebind)
{
	DoubleBindings.BindAKey(argc, argv, "Current key doublebindings");
}
END_COMMAND(doublebind)

BEGIN_COMMAND(unnetdemobind)
{
	if (argc > 1)
	{
		NetDemoBindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(unnetdemobind)

BEGIN_COMMAND(netdemobind)
{
	NetDemoBindings.BindAKey(argc, argv, "Current netdemokey bindings");
}
END_COMMAND(netdemobind)

BEGIN_COMMAND (binddefaults)
{
	C_BindDefaults();
}
END_COMMAND (binddefaults)


void BIND_Init(void)
{
	Bindings.SetBindingType("bind");
	DoubleBindings.SetBindingType("doublebind");
	NetDemoBindings.SetBindingType("netdemobind");
}

// Bind default bindings
void C_BindDefaults(void)
{
	NetDemoBindings.SetBinds(DefaultNetDemoBindings);
	Bindings.SetBinds(DefaultBindings);
	DoubleBindings.SetBinds(DefaultDoubleBindings);
}

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

	std::string *binding;

	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	binding = &NetDemoBindings.Binds[ev->data1];

	// hardcode the pause key to also control netpause
	if (iequals(binding->c_str(), "pause"))
		binding = &NetDemoBindings.Binds[GetKeyFromName("space")];

	// nothing bound to this key specific to netdemos?
	if (binding->empty())
		return false;

	if (ev->type == ev_keydown)
		AddCommandString(*binding);

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
		binding = &DoubleBindings.Binds[ev->data1];
		DClicked[dclickspot] |= dclickmask;
	} else {
		if (ev->type == ev_keydown) {
			// Key pressed for a normal press
			binding = &Bindings.Binds[ev->data1];
			DClickTime[ev->data1] = level.time + 20;
		} else if (DClicked[dclickspot] & dclickmask) {
			// Key released from a double click
			binding = &DoubleBindings.Binds[ev->data1];
			DClicked[dclickspot] &= ~dclickmask;
			DClickTime[ev->data1] = 0;
		} else {
			// Key released from a normal press
			binding = &Bindings.Binds[ev->data1];
		}
	}

	if (!binding->length())
		binding = &Bindings.Binds[ev->data1];

	if (binding->length() && (HU_ChatMode() == CHAT_INACTIVE || ev->data1 < 256))
	{
		if (ev->type == ev_keydown)
		{
			AddCommandString (*binding);
			KeysDown[ev->data1] = true;
		}
		else
		{
			size_t achar = binding->find_first_of('+');

			if (achar == std::string::npos)
				return false;

			if (achar == 0 || (*binding)[achar - 1] <= ' ')
			{
				(*binding)[achar] = '-';
				AddCommandString (*binding);
				(*binding)[achar] = '+';
			}

			KeysDown[ev->data1] = false;
		}
		return true;
	}
	return false;
}

//
// C_ReleaseKeys
//
// Calls the key-release action for all bound keys that are currently
// being held down.
//
void C_ReleaseKeys()
{
	for (int i = 0; i < NUM_KEYS; i++)
	{
		if (!KeysDown[i])
			continue;

		KeysDown[i] = false;
		std::string *binding = &Bindings.Binds[i];
		if (binding->empty())
			continue;

		size_t achar = binding->find_first_of('+');

		if (achar != std::string::npos &&
			(achar == 0 || (*binding)[achar - 1] <= ' '))
		{
			(*binding)[achar] = '-';
			AddCommandString(*binding);
			(*binding)[achar] = '+';
		}
	}

	HU_ReleaseKeyStates();
}

std::string C_NameKeys(int first, int second)
{
	if (!first && !second)
		return "???";

	std::string out;

	if (first)
	{
		out += KeyName(first);
		if (second)out += " or ";
	}

	if (second)
	{
		out += KeyName(second);
	}

	return out;
}

VERSION_CONTROL (c_bind_cpp, "$Id$")
