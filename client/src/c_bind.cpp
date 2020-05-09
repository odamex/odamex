// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id$
//
// Copyright (C) 1998-2006 by Randy Heit (ZDoom 1.22).
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
//	C_BIND
//
//-----------------------------------------------------------------------------


#include <stdio.h>
#include <stdlib.h>

#include "doomtype.h"
#include "doomdef.h"
#include "m_ostring.h"
#include "cmdlib.h"
#include "c_dispatch.h"
#include "c_bind.h"
#include "g_level.h"
#include "hu_stuff.h"
#include "cl_demo.h"
#include "d_player.h"
#include "i_input.h"
#include "hashtable.h"

extern NetDemo netdemo;

/* Most of these bindings are equivalent
 * to the original DOOM's keymappings.
 */
OBinding DefaultBindings[] =
{
	{"tilde", "toggleconsole"},
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
	{"uparrow", "+forward"},
	{"backarrow", "+back"},
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
OBinding DefaultNetDemoBindings[] =
{
	{"leftarrow", "netrew"},
	{"rightarrow", "netff"},
	{"uparrow", "netprevmap"},
	{"downarrow", "netnextmap"},
	{"space", "netpause"},
	{ NULL, NULL }
};

/* Special bindings for the automap
 */
OBinding DefaultAutomapBindings[] =
{
	{ "g", "am_grid" },
	{ "m", "am_setmark" },
	{ "c", "am_clearmarks" },
	{ "f", "am_togglefollow" },
	{ "+", "+am_zoomin" },
	{ "kp+", "+am_zoomin" },
	{ "-", "+am_zoomout" },
	{ "kp-", "+am_zoomout" },
	{ "uparrow", "+am_panup"},
	{ "downarrow", "+am_pandown"},
	{ "leftarrow", "+am_panleft"},
	{ "rightarrow", "+am_panright"},
	{ "0", "am_big"},
	{ NULL, NULL }
};

OKeyBindings Bindings, DoubleBindings, AutomapBindings;
OKeyBindings NetDemoBindings;

struct KeyState
{
	KeyState() :
		double_click_time(0), double_clicked(false), key_down(false)
	{}

	int double_click_time;
	bool double_clicked;
	bool key_down;
};

typedef OHashTable<int, KeyState> KeyStateTable;
static KeyStateTable KeyStates;


void OKeyBindings::SetBindingType(std::string cmd)
{
	command = cmd;
}

void OKeyBindings::UnbindKey(const char* key)
{
	int keycode = I_GetKeyFromName(StdStringToLower(key));
	if (keycode)
		this->Binds.erase(*key);
	else
		Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(key).c_str());
}

void OKeyBindings::UnbindAll()
{
	this->Binds.clear();
}

void OKeyBindings::BindAKey(size_t argc, char** argv, char* msg)
{
	if (argc > 1)
	{
		std::string key_name = StdStringToLower(argv[1]);
		int key = I_GetKeyFromName(key_name);
		if (!key)
		{
			Printf(PRINT_HIGH, "Unknown key %s\n", C_QuoteString(argv[1]).c_str());
		}
		else
		{
			if (argc == 2)
				Printf(PRINT_HIGH, "%s = %s\n", key_name.c_str(), C_QuoteString(Binds[key]).c_str());
			else
				Binds[key] = argv[2];
		}
	}
	else
	{
		Printf(PRINT_HIGH, "%s\n", msg);
		for (BindingTable::const_iterator it = Binds.begin(); it != Binds.end(); ++it)
		{
			int key = it->first;
			const std::string& binding = it->second;
			if (!binding.empty())
				Printf(PRINT_HIGH, "%s = %s\n", I_GetKeyName(key).c_str(), C_QuoteString(binding).c_str());
		}
	}
}

void OKeyBindings::DoBind(const char* key, const char* bind)
{
	int keynum = I_GetKeyFromName(StdStringToLower(key));
	if (keynum != 0)
	{
		this->Binds[keynum] = bind;
	}
}

void OKeyBindings::SetBinds(const OBinding* binds)
{
	while (binds->Key)
	{
		DoBind(binds->Key, binds->Bind);
		binds++;
	}
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
	if (iequals(Bindings.Binds[ev->data1], "pause"))
		binding = &NetDemoBindings.Binds[I_GetKeyFromName("space")];

	// nothing bound to this key specific to netdemos?
	if (binding->empty())
		return false;

	if (ev->type == ev_keydown)
		AddCommandString(*binding, ev->data1);

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
		AddCommandString("spyprev", ev->data1);
		return true;
	}
	if (ev->type == ev_keydown && ev->data1 == KEY_MWHEELDOWN)
	{
		AddCommandString("spynext", ev->data1);
		return true;
	}

	return false;
}


bool C_DoKey(event_t* ev, OKeyBindings* binds, OKeyBindings* doublebinds)
{
	if (ev->type != ev_keydown && ev->type != ev_keyup)
		return false;

	const std::string* binding = NULL;
	int key = ev->data1;

	KeyState& key_state = KeyStates[key];
	if (doublebinds != NULL && ev->type == ev_keydown && key_state.double_click_time > level.time)
	{
		// Key pressed for a double click
		binding = &doublebinds->Binds[key];
		key_state.double_clicked = true;
	}
	else
	{
		if (ev->type == ev_keydown)
		{
			// Key pressed for a normal press
			binding = &binds->Binds[key];
			key_state.double_click_time = level.time + 20;
		}
		else if (doublebinds != NULL && key_state.double_clicked)
		{
			// Key released from a double click
			binding = &doublebinds->Binds[key];
			key_state.double_click_time = 0;
			key_state.double_clicked = false;
		} else {
			// Key released from a normal press
			binding = &binds->Binds[key];
		}
	}

	if (binding->empty())
		binding = &binds->Binds[key];

	if (!binding->empty() && (HU_ChatMode() == CHAT_INACTIVE || key < 256))
	{
		if (ev->type == ev_keydown)
		{
			AddCommandString(*binding, key);
			key_state.key_down = true;
		}
		else if (ev->type == ev_keyup)
		{
			key_state.key_down = false;

			size_t achar = binding->find_first_of('+');
			if (achar == std::string::npos)
				return false;

			if (achar == 0 || (*binding)[achar - 1] <= ' ')
			{
				std::string action_release(*binding);
				action_release[achar] = '-';
				AddCommandString(action_release, key);
			}
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
	for (KeyStateTable::iterator it = KeyStates.begin(); it != KeyStates.end(); ++it)
	{
		int key = it->first;
		KeyState& key_state = it->second;
		if (key_state.key_down)
		{
			key_state.key_down = false;
			std::string *binding = &Bindings.Binds[key];
			if (!binding->empty())
			{
				size_t achar = binding->find_first_of('+');
				if (achar != std::string::npos && (achar == 0 || (*binding)[achar - 1] <= ' '))
				{
					std::string action_release(*binding);
					action_release[achar] = '-';
					AddCommandString(action_release, key);
				}
			}
		}
	}

	HU_ReleaseKeyStates();
}

void OKeyBindings::ArchiveBindings(FILE* f)
{
	int i;

	for (BindingTable::const_iterator it = Binds.begin(); it != Binds.end(); ++it)
	{
		int key = it->first;
		const std::string& binding = it->second;
		if (!Binds.empty())
			fprintf(f, "%s %s %s\n", command.c_str(), C_QuoteString(I_GetKeyName(key)).c_str(), C_QuoteString(binding).c_str());
	}
}


int OKeyBindings::GetKeysForCommand(const char* cmd, int* first, int* second)
{
	int c = 0;
	*first = *second = 0;

	for (BindingTable::const_iterator it = Binds.begin(); it != Binds.end(); ++it)
	{
		int key = it->first;
		const std::string& binding = it->second;
		if (!binding.empty() && stricmp(cmd, binding.c_str()) == 0)
		{
			c++;
			if (c == 1)
			{
				*first = key;
			}
			else
			{
				*second = key;
				break;
			}
		}
	}
	return c;
}


std::string OKeyBindings::GetNameKeys(int first, int second)
{
	if (!first && !second)
		return "???";

	std::string out;

	if (first)
	{
		out += I_GetKeyName(first);
		if (second)
			out += " or ";
	}

	if (second)
	{
		out += I_GetKeyName(second);
	}

	return out;
}


void OKeyBindings::UnbindACommand(const char* str)
{
	for (BindingTable::iterator it = Binds.begin(); it != Binds.end(); ++it)
	{
		const std::string& binding = it->second;
		if (!binding.empty() && stricmp(str, binding.c_str()) == 0)
		{
			Binds.erase(it);
			it = Binds.begin();		// restart iteration since the container was modified during iteration
		}
	}
}


void OKeyBindings::ChangeBinding (const char *str, int newone)
{
	// Check which bindings that are already set. If both binding slots are taken,
	// erase all bindings and reassign the new one and the secondary binding to the key instead.
	int first = 0;
	int second = 0;

	GetKeysForCommand(str, &first, &second);

	if (newone == first || newone == second)
	{
		return;
	}
	else if (first > 0 && second > 0)
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


std::string OKeyBindings::GetBind (int key)
{
	return Binds[key];
}

const char* OKeyBindings::GetBinding(int key)
{
	return Binds[key].c_str();
}

/*
C_GetKeyStringsFromCommand
Finds binds from a command and returns it into a std::string .
- If TRUE, second arg returns up to 2 keys. ("x OR y")
*/
std::string OKeyBindings::GetKeynameFromCommand(char *cmd, bool bTwoEntries)
{
	int first = -1;
	int second = -1;

	GetKeysForCommand(cmd, &first, &second);

	if (!first && !second)
		return "<??\?>";

	if (bTwoEntries)
		return GetNameKeys(first, second);
	else
	{
		if (!first && second)
			return I_GetKeyName(second);
		else
			return I_GetKeyName(first);
	}
	return "<??\?>";
}


void C_BindingsInit(void)
{
	Bindings.SetBindingType("bind");
	DoubleBindings.SetBindingType("doublebind");
	AutomapBindings.SetBindingType("automapbind");
}

// Bind default bindings
void C_BindDefaults(void)
{
	Bindings.SetBinds(DefaultBindings);
	AutomapBindings.SetBinds(DefaultAutomapBindings);
}


// Bind command
BEGIN_COMMAND(bind)
{
	Bindings.BindAKey(argc, argv, "Current key bindings: ");
}
END_COMMAND(bind)

BEGIN_COMMAND(unbind)
{
	if (argc > 1) {
		Bindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(unbind)


// Doublebind command
BEGIN_COMMAND(doublebind)
{
	DoubleBindings.BindAKey(argc, argv, "Current doublebindings: ");
}
END_COMMAND(doublebind)

BEGIN_COMMAND(undoublebind)
{
	if (argc > 1) {
		DoubleBindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(undoublebind)

// Automapbind command
BEGIN_COMMAND(automapbind)
{
	AutomapBindings.BindAKey(argc, argv, "Current automap bindings: ");
}
END_COMMAND(automapbind)

BEGIN_COMMAND(unautomapbind)
{
	if (argc > 1) {
		AutomapBindings.UnbindKey(argv[1]);
	}
}
END_COMMAND(unautomapbind)

// Other commands
BEGIN_COMMAND(binddefaults)
{
	C_BindDefaults();
}
END_COMMAND(binddefaults)

BEGIN_COMMAND(unbindall)
{
	Bindings.UnbindAll();
	DoubleBindings.UnbindAll();
	AutomapBindings.UnbindAll();
}
END_COMMAND(unbindall)

VERSION_CONTROL (c_bind_cpp, "$Id$")
